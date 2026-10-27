#include "persistence/world_snapshot.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace reality {

std::uint64_t snapshot_checksum(std::string_view bytes) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char byte : bytes) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

namespace {

std::string word(std::istream& input) {
    std::string value;
    if (!(input >> value)) {
        throw std::runtime_error("truncated snapshot");
    }
    return value;
}

void expect(std::istream& input, std::string_view expected) {
    if (word(input) != expected) {
        throw std::runtime_error("unexpected snapshot section");
    }
}

std::uint64_t number(std::istream& input, int base = 10) {
    const auto text = word(input);
    std::uint64_t result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result, base);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
        throw std::runtime_error("invalid snapshot integer");
    }
    return result;
}

bool boolean(std::istream& input) {
    const auto value = number(input);
    if (value > 1) {
        throw std::runtime_error("invalid snapshot boolean");
    }
    return value != 0;
}

std::size_t count(std::istream& input) {
    const auto value = number(input);
    if (value > record_limit) {
        throw std::runtime_error("snapshot record limit exceeded");
    }
    return static_cast<std::size_t>(value);
}

void write_request(std::ostream& out, const WorkRequest& request) {
    out << request.command.value << ' ' << request.actor.value << ' ' << request.lot.value << ' '
        << request.required.value << ' ' << request.consume_on_start.value << ' '
        << request.duration_ticks;
}

WorkRequest read_request(std::istream& input) {
    return WorkRequest{CommandId{number(input)}, EntityId{number(input)}, EntityId{number(input)},
                       Quantity{number(input)},  Quantity{number(input)}, number(input)};
}

[[noreturn]] void io_error(const char* operation) {
    throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
}

struct HandleCloser {
    void operator()(void* handle) const noexcept { CloseHandle(handle); }
};

class PendingFile final {
  public:
    explicit PendingFile(const std::filesystem::path& path) noexcept : path_(path) {}
    ~PendingFile() {
        if (!committed_) {
            DeleteFileW(path_.c_str());
        }
    }
    PendingFile(const PendingFile&) = delete;
    PendingFile& operator=(const PendingFile&) = delete;
    void commit() noexcept { committed_ = true; }

  private:
    const std::filesystem::path& path_;
    bool committed_ = false;
};

} // namespace

std::string serialize_world(const WorkWorld& world) {
    world.validate();
    std::ostringstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out.imbue(std::locale::classic());
    const auto capacity =
        std::any_of(world.resources_.begin(), world.resources_.end(),
                    [](const auto& lot) { return lot.kind == ResourceKind::Capacity; });
    const auto typed = std::any_of(world.resources_.begin(), world.resources_.end(),
                                   [](const auto& lot) { return lot.material.value != 0; });
    const auto version = typed || !world.transfers_.empty() ? 3 : capacity ? 2 : 1;
    out << "GENESIS_WORK_WORLD " << version << "\nCLOCK " << world.clock_.step().count() << ' '
        << world.clock_.tick_count() << "\nREGISTRY " << world.entities_.next_id() << ' '
        << world.entities_.slots().size() << '\n';
    for (const auto& slot : world.entities_.slots()) {
        out << slot.id.value << ' ' << static_cast<std::uint64_t>(slot.kind) << ' '
            << slot.generation << ' ' << slot.occupied << '\n';
    }
    out << "ACTORS " << world.actors_.size() << '\n';
    for (const auto& actor : world.actors_) {
        out << actor.id.value << ' ' << actor.alive << '\n';
    }
    out << "RESOURCES " << world.resources_.size() << '\n';
    for (const auto& lot : world.resources_) {
        out << lot.id.value << ' ' << lot.owner.value << ' ' << lot.initial.value << ' '
            << lot.quantity.value << ' ' << lot.lost.value << ' ' << lot.active;
        if (version >= 2)
            out << ' ' << static_cast<std::uint64_t>(lot.kind);
        if (version >= 3)
            out << ' ' << lot.material.value;
        out << '\n';
    }
    out << "ACTIONS " << world.actions_.size() << '\n';
    for (const auto& action : world.actions_) {
        out << action.id.value << ' ';
        write_request(out, action.request);
        out << ' ' << action.consumed.value << ' ' << static_cast<std::uint64_t>(action.state)
            << ' ' << action.started_at << ' ' << action.due_at << ' ' << action.ended_at << ' '
            << static_cast<std::uint64_t>(action.reason) << '\n';
    }
    out << "COMMANDS " << world.commands_.size() << '\n';
    for (const auto& command : world.commands_) {
        write_request(out, command.request);
        out << ' ' << static_cast<std::uint64_t>(command.result.status) << ' '
            << command.result.action.value << '\n';
    }
    if (version >= 3) {
        out << "TRANSFERS " << world.transfers_.size() << '\n';
        for (const auto& record : world.transfers_) {
            const auto& r = record.request;
            out << r.command.value << ' ' << r.actor.value << ' ' << r.source.value << ' '
                << r.destination.value << ' ' << r.quantity.value << ' '
                << static_cast<std::uint64_t>(record.result.status) << ' ' << record.tick << ' '
                << record.event_sequence << '\n';
        }
    }
    out << "EVENTS " << world.events_.size() << '\n';
    for (const auto& event : world.events_) {
        out << event.sequence << ' ' << event.tick << ' ' << static_cast<std::uint64_t>(event.kind)
            << ' ' << event.subject.value << ' ' << event.quantity.value << '\n';
    }
    out << "END\n";
    const auto body = out.str();
    out << "CHECKSUM " << std::hex << std::setw(16) << std::setfill('0') << snapshot_checksum(body)
        << '\n';
    auto result = out.str();
    if (result.size() > snapshot_byte_limit) {
        throw std::length_error("snapshot byte limit exceeded");
    }
    return result;
}

WorkWorld deserialize_world(std::string_view bytes) {
    if (bytes.size() > snapshot_byte_limit) {
        throw std::runtime_error("snapshot byte limit exceeded");
    }
    const auto marker = bytes.rfind("\nCHECKSUM ");
    if (marker == std::string_view::npos) {
        throw std::runtime_error("missing snapshot checksum");
    }
    const auto body = bytes.substr(0, marker + 1);
    std::istringstream trailer{std::string{bytes.substr(marker + 1)}};
    trailer.imbue(std::locale::classic());
    expect(trailer, "CHECKSUM");
    const auto expected = number(trailer, 16);
    std::string extra;
    if (trailer >> extra || snapshot_checksum(body) != expected) {
        throw std::runtime_error("snapshot checksum mismatch");
    }
    std::istringstream input{std::string{body}};
    input.imbue(std::locale::classic());
    expect(input, "GENESIS_WORK_WORLD");
    const auto version = number(input);
    if (version != 1 && version != 2 && version != 3) {
        throw std::runtime_error("unsupported snapshot version");
    }
    expect(input, "CLOCK");
    const auto step = number(input);
    if (step == 0 || step > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        throw std::runtime_error("invalid snapshot step");
    }
    WorkWorld result{std::chrono::nanoseconds{static_cast<std::int64_t>(step)}};
    result.clock_.advance(number(input));
    expect(input, "REGISTRY");
    const auto next_id = number(input);
    std::vector<EntitySlot> slots(count(input));
    for (auto& slot : slots) {
        slot = EntitySlot{EntityId{number(input)}, static_cast<EntityKind>(number(input)),
                          number(input), boolean(input)};
    }
    result.entities_ = EntityRegistry::restore(next_id, std::move(slots));
    expect(input, "ACTORS");
    result.actors_.resize(count(input));
    for (auto& actor : result.actors_) {
        actor = ActorRecord{EntityId{number(input)}, boolean(input)};
    }
    expect(input, "RESOURCES");
    result.resources_.resize(count(input));
    for (auto& lot : result.resources_) {
        lot = ResourceRecord{EntityId{number(input)}, EntityId{number(input)},
                             Quantity{number(input)}, Quantity{number(input)},
                             Quantity{number(input)}, boolean(input)};
        if (version >= 2)
            lot.kind = static_cast<ResourceKind>(number(input));
        if (version >= 3)
            lot.material = {number(input)};
    }
    expect(input, "ACTIONS");
    result.actions_.resize(count(input));
    for (auto& action : result.actions_) {
        action = ActionRecord{EntityId{number(input)}, read_request(input),
                              Quantity{number(input)}, static_cast<ActionState>(number(input)),
                              number(input),           number(input),
                              number(input),           static_cast<CancelReason>(number(input))};
    }
    expect(input, "COMMANDS");
    result.commands_.resize(count(input));
    for (auto& command : result.commands_) {
        command = CommandRecord{
            read_request(input),
            RequestResult{static_cast<RequestStatus>(number(input)), EntityId{number(input)}}};
    }
    if (version >= 3) {
        expect(input, "TRANSFERS");
        result.transfers_.resize(count(input));
        for (auto& record : result.transfers_) {
            record.request = {{number(input)},
                              {number(input)},
                              {number(input)},
                              {number(input)},
                              {number(input)}};
            record.result.status = static_cast<TransferStatus>(number(input));
            record.tick = number(input);
            record.event_sequence = number(input);
        }
    }
    expect(input, "EVENTS");
    result.events_.resize(count(input));
    for (auto& event : result.events_) {
        event = WorldEvent{number(input), number(input), static_cast<EventKind>(number(input)),
                           EntityId{number(input)}, Quantity{number(input)}};
    }
    expect(input, "END");
    if (input >> extra) {
        throw std::runtime_error("unexpected snapshot trailing data");
    }
    result.validate();
    return result;
}

void save_world(const WorkWorld& world, const std::filesystem::path& destination) {
    const auto bytes = serialize_world(world);
    write_snapshot_atomic(bytes, destination);
}

void write_snapshot_atomic(std::string_view bytes, const std::filesystem::path& destination) {
    if (bytes.size() > snapshot_byte_limit) {
        throw std::runtime_error("snapshot byte limit exceeded");
    }
    if (destination.empty() || destination.filename().empty()) {
        throw std::invalid_argument("snapshot destination needs a file name");
    }
    std::filesystem::path temporary;
    HANDLE raw = INVALID_HANDLE_VALUE;
    for (unsigned attempt = 0; attempt < 64; ++attempt) {
        temporary = destination.wstring() + L".tmp-" + std::to_wstring(GetCurrentProcessId()) +
                    L"-" + std::to_wstring(attempt);
        raw = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                          FILE_ATTRIBUTE_NORMAL, nullptr);
        if (raw != INVALID_HANDLE_VALUE) {
            break;
        }
        if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) {
            io_error("create snapshot temporary file");
        }
    }
    if (raw == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("snapshot temporary names exhausted");
    }
    PendingFile cleanup{temporary};
    {
        const std::unique_ptr<void, HandleCloser> handle{raw};
        std::size_t offset = 0;
        while (offset != bytes.size()) {
            DWORD written = 0;
            if (!WriteFile(handle.get(), bytes.data() + offset,
                           static_cast<DWORD>(bytes.size() - offset), &written, nullptr)) {
                io_error("write snapshot");
            }
            if (written == 0) {
                throw std::runtime_error("snapshot write made no progress");
            }
            offset += written;
        }
        if (!FlushFileBuffers(handle.get())) {
            io_error("flush snapshot");
        }
    }
    if (!MoveFileExW(temporary.c_str(), destination.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        io_error("replace snapshot");
    }
    cleanup.commit();
}

WorkWorld load_world(const std::filesystem::path& source) {
    return deserialize_world(read_snapshot_bytes(source));
}

std::string read_snapshot_bytes(const std::filesystem::path& source) {
    std::ifstream input{source, std::ios::binary};
    if (!input) {
        throw std::runtime_error("cannot open snapshot");
    }
    std::string bytes;
    char buffer[4096];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() != 0) {
        if (static_cast<std::size_t>(input.gcount()) > snapshot_byte_limit - bytes.size()) {
            throw std::runtime_error("snapshot byte limit exceeded");
        }
        bytes.append(buffer, static_cast<std::size_t>(input.gcount()));
    }
    if (!input.eof()) {
        throw std::runtime_error("cannot read snapshot");
    }
    return bytes;
}

} // namespace reality
