#include "core/runtime_metrics.hpp"
#include "graphics/d3d11_renderer.hpp"
#include "lab/lab_session.hpp"
#include "lab/traversal_diagnostics.hpp"
#include "lab/traversal_session.hpp"

#include <psapi.h>
#include <shellapi.h>
#include <windowsx.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace {
using namespace reality;
using namespace std::chrono_literals;
constexpr wchar_t window_class[] = L"GenesisTraversalMain";
constexpr wchar_t view_class[] = L"GenesisTraversalView";
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
std::wstring wide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                          static_cast<int>(text.size()), nullptr, 0);
    require(size > 0, "invalid UTF-8 UI text");
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    require(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                static_cast<int>(text.size()), result.data(), size) == size,
            "UTF-8 UI conversion failed");
    return result;
}
struct Options {
    std::filesystem::path save = std::filesystem::current_path() / L"traversal-world.save";
    std::filesystem::path report = std::filesystem::current_path() / L"traversal.log";
    std::filesystem::path capture = std::filesystem::current_path() / L"traversal.png";
    bool smoke = false, debug = false, resume = false, unattended = false;
    bool delivery = false, crossing = false, law = false, community = false, learning = false;
    bool two_methods = false;
    bool repair_smoke = false;
    std::filesystem::path diagnostics;
    unsigned measure_seconds = 0, warmup_seconds = 30;
    int adapter = -1;
};
void read_options(Options& options) {
    int count = 0;
    auto* args = CommandLineToArgvW(GetCommandLineW(), &count);
    require(args != nullptr, "cannot read traversal command line");
    struct Free {
        void operator()(wchar_t** pointer) const { LocalFree(pointer); }
    };
    std::unique_ptr<wchar_t*, Free> owned{args};
    bool save_given = false, report_given = false, warmup_given = false;
    for (int i = 1; i < count; ++i) {
        const std::wstring_view arg{args[i]};
        if (arg == L"--smoke-test" || arg == L"--measure-seconds" || arg == L"--warmup-seconds")
            options.unattended = true;
    }
    for (int i = 1; i < count; ++i) {
        const std::wstring_view arg{args[i]};
        if (arg == L"--smoke-test") {
            options.smoke = true;
            options.unattended = true;
        } else if (arg == L"--delivery") {
            options.delivery = true;
        } else if (arg == L"--crossing") {
            options.crossing = true;
        } else if (arg == L"--community") {
            options.community = true;
        } else if (arg == L"--learning") {
            options.learning = true;
        } else if (arg == L"--two-methods") {
            options.two_methods = options.community = options.learning = true;
        } else if (arg == L"--repair-smoke") {
            options.repair_smoke = true;
        } else if (arg == L"--law") {
            options.law = true;
        } else if (arg == L"--gpu-debug") {
            options.debug = true;
        } else if (arg == L"--resume-smoke") {
            options.resume = true;
        } else {
            require(i + 1 < count, "traversal option needs a value");
            const std::wstring value{args[++i]};
            if (arg == L"--save") {
                options.save = value;
                save_given = true;
            } else if (arg == L"--report") {
                options.report = value;
                report_given = true;
            } else if (arg == L"--capture") {
                options.capture = value;
            } else if (arg == L"--diagnostics") {
                options.diagnostics = value;
            } else if (arg == L"--measure-seconds" || arg == L"--warmup-seconds") {
                options.unattended = true;
                require(!value.empty() &&
                            std::all_of(value.begin(), value.end(),
                                        [](wchar_t c) { return c >= L'0' && c <= L'9'; }),
                        "invalid measurement duration");
                const auto seconds = std::stoul(value);
                require(seconds <= (arg == L"--measure-seconds" ? 1800UL : 300UL) &&
                            (arg != L"--measure-seconds" || seconds > 0),
                        "measurement duration outside supported range");
                if (arg == L"--measure-seconds")
                    options.measure_seconds = static_cast<unsigned>(seconds);
                else {
                    options.warmup_seconds = static_cast<unsigned>(seconds);
                    warmup_given = true;
                }
            } else if (arg == L"--adapter") {
                std::size_t used = 0;
                options.adapter = std::stoi(value, &used);
                require(used == value.size() && options.adapter >= 0, "invalid adapter");
            } else {
                throw std::invalid_argument("unknown traversal option");
            }
        }
    }
    require(!options.resume || options.smoke, "resume-smoke requires smoke-test");
    require(!options.learning || options.community, "learning requires --community");
    require(!options.repair_smoke || (options.two_methods && options.smoke),
            "repair-smoke requires two-methods and smoke-test");
    require(!options.two_methods ||
                (!options.delivery && !options.crossing && !options.law && !options.resume),
            "two-methods cannot combine with another scene or resume-smoke");
    require(!warmup_given || options.measure_seconds != 0, "warmup needs a measurement");
    require(options.measure_seconds == 0 || (!options.smoke && save_given && report_given),
            "measurement needs explicit --save and --report and cannot use smoke-test");
    if (options.diagnostics.empty())
        options.diagnostics = options.save.parent_path() / "traversal-diagnostics";
}
const wchar_t* state_label(TraversalState state) {
    switch (state) {
    case TraversalState::Idle:
        return L"Stoi";
    case TraversalState::Moving:
        return L"Podąża do celu";
    case TraversalState::Arrived:
        return L"Cel osiągnięty";
    case TraversalState::Waiting:
        return L"Oczekuje";
    case TraversalState::NoPath:
        return L"Brak dostępnego planu";
    case TraversalState::InvalidGoal:
        return L"Cel niedostępny";
    case TraversalState::BudgetExceeded:
        return L"Wyczerpano budżet planowania";
    }
    return L"Niepoprawny stan";
}
class TraversalWindow final {
  public:
    TraversalWindow(HINSTANCE instance, const Options& options, std::ostream& log)
        : instance_(instance), options_(options), log_(log),
          session_(options.two_methods ? make_two_methods_course()
                   : options.learning  ? make_learning_course()
                   : options.community ? make_community_course()
                   : options.law       ? make_law_course()
                   : options.crossing  ? make_crossing_course()
                   : options.delivery  ? make_delivery_course()
                                       : make_traversal_course()) {
        WNDCLASSEXW type{};
        type.cbSize = sizeof(type);
        type.hInstance = instance_;
        type.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        type.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        type.lpfnWndProc = main_proc;
        type.lpszClassName = window_class;
        require(RegisterClassExW(&type) != 0, "cannot register traversal window");
        type.lpfnWndProc = view_proc;
        type.hbrBackground = nullptr;
        type.lpszClassName = view_class;
        require(RegisterClassExW(&type) != 0, "cannot register traversal viewport");
        camera_.zoom(5);
    }
    ~TraversalWindow() {
        renderer_.reset();
        if (IsWindow(window_)) {
            DestroyWindow(window_);
        }
        if (font_) {
            DeleteObject(font_);
        }
        UnregisterClassW(view_class, instance_);
        UnregisterClassW(window_class, instance_);
    }
    TraversalWindow(const TraversalWindow&) = delete;
    TraversalWindow& operator=(const TraversalWindow&) = delete;
    int run() {
        window_ = CreateWindowExW(0, window_class, L"Genesis | ruch, poziomy i autonomia",
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 1200, options_.two_methods ? 920 : 820, nullptr,
                                  nullptr, instance_, this);
        require(window_ != nullptr, "cannot create traversal window");
        viewport_ = CreateWindowExW(0, view_class, L"Scena", WS_CHILD | WS_VISIBLE, 0, 0, 800, 760,
                                    window_, nullptr, instance_, this);
        require(viewport_ != nullptr, "cannot create traversal viewport");
        font_ = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH, L"Segoe UI");
        require(font_ != nullptr, "cannot create traversal font");
        title_ = control(L"STATIC", L"GENESIS / RUCH I AUTONOMIA", 0);
        status_ = control(L"STATIC", L"", 0);
        help_ = control(L"STATIC",
                        L"Spacja: czas | N: +1 s | X: zatrzymaj\nWASD: ruch | PPM: dokładny "
                        L"cel\nShift+PPM: podejdź w zasięg\n1–8 / Tab: postać | F2: jej "
                        L"autonomia\nF3: poproś o przejście\nF4: pobierz / oddaj ładunek\nF8: "
                        L"zamknij / otwórz przejście | R: napraw\nF7: "
                        L"przekaż wiedzę najbliższej osobie\nQ/E, "
                        L"↑/↓, +/-: kamera\nF5/F9: zapis i "
                        L"odczyt\nF6: migawka "
                        L"diagnostyczna\nPola: zielone — "
                        L"jedzenie, fioletowe — odpoczynek, złote — konserwacja.",
                        0);
        if (options_.learning)
            SetWindowTextW(help_, L"4 / 6: wykonawcy | Spacja: czas | N: +1 s\nF2: przejmij | WASD "
                                  L"/ PPM: ruch | R: praca\nI: wejście | O: moduły | P: próba przy "
                                  L"aparacie\nF7: przekaż pomiary w kontakcie\nF5/F9: zapis/odczyt "
                                  L"| Q/E, +/-: kamera\nTurkus: aparat; złoto: konserwacja.");
        if (options_.two_methods)
            SetWindowTextW(help_,
                           L"Cel: dostarcz 8 żywności na drugi brzeg.\nObjazd: 2, potem F2 "
                           L"uruchamia kuriera.\nNaprawa: 4, potem F2 uruchamia badanie.\nPo "
                           L"otwarciu mostu: 2, F2 uruchamia kuriera.\nSpacja: czas | 1–8: "
                           L"postać | F2: autonomia\nWASD / PPM: ruch | I/O/P: próba | R: "
                           L"praca\nF5/F9: zapis/odczyt | Q/E, +/-: kamera\nTurkus: aparat; "
                           L"złoto: konserwacja.");
        buttons_[0] = control(L"BUTTON", L"Odtwarzaj [Spacja]", 1001, BS_PUSHBUTTON);
        buttons_[1] = control(L"BUTTON", L"+1 s [N]", 1002, BS_PUSHBUTTON);
        buttons_[2] = control(L"BUTTON", L"Autonomia [F2]", 1003, BS_PUSHBUTTON);
        buttons_[3] = control(L"BUTTON", L"Zapisz [F5]", 1004, BS_PUSHBUTTON);
        buttons_[4] = control(L"BUTTON", L"Wczytaj [F9]", 1005, BS_PUSHBUTTON);
        feedback_ = control(
            L"STATIC",
            L"Start w pauzie. Niebieska postać jest wybrana; pozostałe mają własne zadania.", 0);
        inspector_ = control(L"EDIT", L"", 0,
                             WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL);
        layout();
        renderer_ =
            std::make_unique<D3D11Renderer>(viewport_, log_, options_.adapter, options_.debug);
        ShowWindow(window_, SW_SHOWNORMAL);
        UpdateWindow(window_);
        auto previous = std::chrono::steady_clock::now();
        const auto run_start = previous;
        auto previous_frame_end = run_start;
        const auto measurement_start = run_start + std::chrono::seconds{options_.warmup_seconds};
        const auto measurement_end =
            measurement_start + std::chrono::seconds{options_.measure_seconds};
        if (options_.measure_seconds != 0) {
            session_.save(options_.save);
            pacer_.set_running(true);
            log_ << "measure_seconds=" << options_.measure_seconds
                 << "\nwarmup_seconds=" << options_.warmup_seconds
                 << "\nmeasurement_initial_checksum=" << snapshot_checksum(session_.serialize())
                 << '\n';
        }
        measurement_started_ = true;
        unsigned frames = 0;
        for (;;) {
            const auto frame_start = std::chrono::steady_clock::now();
            if (options_.measure_seconds != 0 && frame_start >= measurement_end) {
                measurement_finished_ = true;
                break;
            }
            MSG message{};
            bool quit = false;
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    quit = true;
                    break;
                }
                if ((message.message == WM_KEYDOWN || message.message == WM_KEYUP) &&
                    key(message.wParam, message.message == WM_KEYDOWN,
                        (message.lParam & (1LL << 30)) != 0)) {
                    continue;
                }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (fatal_) {
                std::rethrow_exception(fatal_);
            }
            if (quit) {
                break;
            }
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed =
                rebase_ ? 0ns
                        : std::chrono::duration_cast<std::chrono::nanoseconds>(now - previous);
            previous = now;
            rebase_ = false;
            if (minimized_ || sizing_) {
                MsgWaitForMultipleObjectsEx(0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                rebase_ = true;
                continue;
            }
            apply_input();
            const auto before_tick = session_.world().clock().tick_count();
            const auto simulation_start = std::chrono::steady_clock::now();
            try {
                pacer_.update(session_, elapsed);
            } catch (const std::exception& error) {
                ++simulation_errors_;
                pacer_.set_running(false);
                log_ << "ERROR: simulation " << error.what() << '\n';
                feedback(L"Błąd symulacji: " + wide(error.what()));
            }
            const auto simulation_end = std::chrono::steady_clock::now();
            diagnostics_.observe(session_);
            const auto alerts = diagnostics_.new_stalls();
            for (const auto actor : alerts)
                if (automatic_captures_ < 8) {
                    ++automatic_captures_;
                    capture_diagnostic("suspected_stall_actor_" + std::to_string(actor.value));
                }
            panel();
            draw();
            if (!renderer_->present()) {
                ++occluded_frames_;
                MsgWaitForMultipleObjectsEx(0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            }
            const auto frame_end = std::chrono::steady_clock::now();
            if (options_.smoke ||
                (options_.measure_seconds != 0 && frame_start >= measurement_start)) {
                const auto milliseconds = [](auto duration) {
                    return std::chrono::duration<double, std::milli>{duration}.count();
                };
                metrics_.record(milliseconds(frame_end - previous_frame_end),
                                milliseconds(simulation_end - simulation_start),
                                milliseconds(pacer_.pending()));
                measured_ticks_ += session_.world().clock().tick_count() - before_tick;
                if (!first_sample_)
                    first_sample_ = previous_frame_end;
                last_sample_ = frame_end;
                if (frame_end >= next_memory_sample_) {
                    PROCESS_MEMORY_COUNTERS_EX memory{};
                    require(
                        GetProcessMemoryInfo(GetCurrentProcess(),
                                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),
                                             sizeof(memory)) != 0,
                        "cannot measure process memory");
                    const auto working = static_cast<std::uint64_t>(memory.WorkingSetSize);
                    if (first_working_set_ == 0)
                        first_working_set_ = working;
                    last_working_set_ = working;
                    peak_working_set_ = std::max(peak_working_set_, working);
                    peak_private_bytes_ = std::max(peak_private_bytes_,
                                                   static_cast<std::uint64_t>(memory.PrivateUsage));
                    next_memory_sample_ = frame_end + 1s;
                }
            }
            previous_frame_end = frame_end;
            if (options_.smoke && ++frames == 2) {
                smoke();
                PostQuitMessage(0);
            }
        }
        renderer_->check_diagnostics();
        if (options_.smoke || options_.measure_seconds != 0) {
            log_ << diagnostics_.report() << metrics_.report()
                 << "measurement_finished=" << measurement_finished_
                 << "\nmeasurement_interventions=" << measurement_interventions_
                 << "\nmeasurement_minimizations=" << measurement_minimizations_
                 << "\nmeasurement_resizes=" << measurement_resizes_
                 << "\noccluded_frames=" << occluded_frames_
                 << "\nsimulation_errors=" << simulation_errors_
                 << "\nmeasured_ticks=" << measured_ticks_ << "\nmeasured_wall_ms="
                 << (first_sample_ && last_sample_
                         ? std::chrono::duration<double, std::milli>{*last_sample_ - *first_sample_}
                               .count()
                         : 0)
                 << "\nviewport=" << renderer_->width() << 'x' << renderer_->height()
                 << "\nworking_set_first_bytes=" << first_working_set_
                 << "\nworking_set_last_bytes=" << last_working_set_
                 << "\nworking_set_peak_sampled_bytes=" << peak_working_set_
                 << "\nprivate_peak_sampled_bytes=" << peak_private_bytes_ << '\n';
            log_ << session_.inspect();
        }
        if (options_.measure_seconds != 0) {
            auto final_save = options_.save;
            final_save += L".final.save";
            session_.save(final_save);
            require(measurement_finished_ && simulation_errors_ == 0,
                    "measurement ended early or simulation failed");
        }
        log_ << "normal_shutdown=1\n";
        return 0;
    }

  private:
    HINSTANCE instance_;
    const Options& options_;
    std::ostream& log_;
    HWND window_ = nullptr, viewport_ = nullptr, title_ = nullptr, status_ = nullptr,
         help_ = nullptr, feedback_ = nullptr, inspector_ = nullptr;
    std::array<HWND, 5> buttons_{};
    HFONT font_ = nullptr;
    TraversalSession session_;
    SimulationPacer pacer_;
    LabCamera camera_;
    std::unique_ptr<D3D11Renderer> renderer_;
    std::vector<SceneTriangle> scene_;
    std::array<bool, 4> keys_{};
    int selection_ = 0;
    unsigned practice_input_ = 16;
    std::size_t practice_chain_ = 0;
    bool minimized_ = false, sizing_ = false, rebase_ = false;
    std::exception_ptr fatal_;
    std::string old_inspector_;
    TraversalDiagnostics diagnostics_;
    RuntimeMetrics metrics_;
    std::optional<std::filesystem::path> last_diagnostic_;
    std::optional<std::chrono::steady_clock::time_point> first_sample_, last_sample_;
    std::chrono::steady_clock::time_point next_memory_sample_{};
    std::uint64_t capture_sequence_ = 0, measured_ticks_ = 0;
    std::uint64_t first_working_set_ = 0, last_working_set_ = 0, peak_working_set_ = 0,
                  peak_private_bytes_ = 0;
    unsigned automatic_captures_ = 0, simulation_errors_ = 0, occluded_frames_ = 0,
             measurement_interventions_ = 0, measurement_minimizations_ = 0,
             measurement_resizes_ = 0;
    bool measurement_started_ = false, measurement_finished_ = false;
    void capture_diagnostic(std::string_view reason) {
        last_diagnostic_.reset();
        try {
            diagnostics_.observe(session_);
            std::filesystem::create_directories(options_.diagnostics);
            require(capture_sequence_ < std::numeric_limits<std::uint64_t>::max(),
                    "diagnostic sequence exhausted");
            const auto name = "case-" + std::to_string(GetTickCount64()) + "-" +
                              std::to_string(++capture_sequence_);
            const auto directory = options_.diagnostics / name;
            diagnostics_.capture(session_, directory, reason);
            last_diagnostic_ = directory;
            const auto encoded = directory.u8string();
            log_ << "diagnostic_capture="
                 << std::string_view{reinterpret_cast<const char*>(encoded.data()), encoded.size()}
                 << " reason=" << reason << '\n';
            feedback(L"Diagnostyka F6: " + directory.filename().wstring());
        } catch (const std::exception& error) {
            log_ << "diagnostic_error=" << error.what() << '\n';
            feedback(L"Błąd diagnostyki: " + wide(error.what()));
        }
    }
    HWND control(const wchar_t* type, const wchar_t* text, int id, DWORD style = 0) {
        auto window =
            CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, window_,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
        require(window != nullptr, "cannot create traversal control");
        SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        return window;
    }
    float aspect() const {
        return static_cast<float>(renderer_->width()) / static_cast<float>(renderer_->height());
    }
    double fraction() const {
        return pacer_.running()
                   ? std::clamp(static_cast<double>(pacer_.pending().count()) /
                                    static_cast<double>(session_.world().clock().step().count()),
                                0.0, 1.0)
                   : 1;
    }
    void feedback(const std::wstring& text) { SetWindowTextW(feedback_, text.c_str()); }
    void layout() {
        if (!viewport_) {
            return;
        }
        RECT rectangle{};
        GetClientRect(window_, &rectangle);
        const auto width = std::max(1L, rectangle.right - 340),
                   height = std::max(1L, rectangle.bottom);
        MoveWindow(viewport_, 0, 0, width, height, TRUE);
        int y = 12;
        const auto x = width + 12;
        const auto place = [&](HWND control, int h) {
            if (control) {
                MoveWindow(control, x, y, 316, h, TRUE);
            }
            y += h + 8;
        };
        place(title_, 25);
        place(status_, options_.learning ? 265 : options_.community ? 155 : 88);
        place(help_, options_.two_methods ? 170 : options_.learning ? 130 : 245);
        for (const auto button : buttons_) {
            place(button, 28);
        }
        place(feedback_, 66);
        if (inspector_) {
            MoveWindow(inspector_, x, y, 316, std::max(30L, rectangle.bottom - y - 12), TRUE);
        }
        if (renderer_) {
            renderer_->resize(static_cast<unsigned>(width), static_cast<unsigned>(height));
        }
        session_.synchronize_presentation();
        rebase_ = true;
    }
    void draw() {
        scene_ = build_traversal_scene(session_, selection_, camera_, aspect(), fraction());
        renderer_->draw(scene_);
    }
    bool community_panel() const {
        return options_.community && session_.actors().size() == 8 &&
               session_.stations().size() == 5 && session_.passages().size() == 1 &&
               session_.actors()[1].delivery && session_.actors()[1].delivery->source == 0 &&
               session_.actors()[1].delivery->destination == 1;
    }
    static const std::vector<DiscoveryChain>& practice_chains() {
        static const auto chains = [] {
            auto c = hypothesis_chains();
            std::erase_if(
                c, [](const auto& v) { return std::find(v.begin(), v.end(), 0) == v.end(); });
            return c;
        }();
        return chains;
    }
    static std::wstring chain_label(const DiscoveryChain& chain) {
        std::wstring label;
        for (auto module : chain) {
            if (!label.empty())
                label += L" → ";
            label += static_cast<wchar_t>(L'A' + module);
        }
        return label;
    }
    void panel() {
        const auto& actor = session_.actors().at(static_cast<std::size_t>(selection_));
        std::wostringstream text;
        text << L"Postać " << selection_ + 1 << L" | "
             << (actor.autonomous ? L"autonomia" : L"sterowanie");
        if (actor.cargo.value != 0) {
            const auto cargo = session_.world().resource(actor.cargo);
            if (cargo)
                text << L" | ładunek " << cargo->record.quantity.value;
        }
        text << L"\n" << state_label(actor.state);
        if (actor.delivery)
            text << L" | dostawa " << actor.delivery->delivered << L"/" << actor.delivery->quantity;
        text << L"\n"
             << L"Krok: " << session_.world().clock().tick_count() << L" | "
             << (pacer_.running() ? L"czas płynie" : L"pauza") << L"\nGPU: "
             << renderer_->adapter_name();
        if (community_panel()) {
            constexpr const wchar_t* roles[] = {L"podróżny",    L"kurier",         L"odbiorca",
                                                L"konserwator", L"opiekun części", L"mieszkaniec",
                                                L"odbiorca",    L"odpoczywający"};
            text << (options_.two_methods ? L"\nM05 | " : L"\nM04 | ") << roles[selection_]
                 << L"\nDostawa: " << session_.actors()[1].delivery->delivered << L"/8 | Posiłki: "
                 << session_.stations()[1].completed << L"\nNaprawy: "
                 << session_.stations()[2].completed << L" | Odpoczynek: "
                 << session_.stations()[3].completed + session_.stations()[4].completed
                 << L"\nMost: " << (session_.passages()[0].open ? L"otwarty" : L"zamknięty")
                 << L" | Obsługa podróżnego: "
                 << (session_.legal_service_allowed(0, 2) ? L"tak" : L"odmowa");
        }
        if (session_.has_practice()) {
            const auto v = session_.practice_view(static_cast<std::size_t>(selection_));
            text << L"\nPraktyka | cel " << v.required_input << L" → " << v.required_output
                 << L" | próby " << v.probes << L"/8";
            text << L"\n"
                 << (v.message.empty() ? (v.method_known ? L"Zna metodę" : L"Brak poznanej metody")
                                       : wide(v.message));
            if (v.method)
                text << L": " << chain_label(*v.method);
            if (v.local)
                text << L" | energia " << v.energy;
            text << L"\nPomiary: " << v.records;
            if (v.last)
                text << L" | ostatni " << v.last->input << L" → " << v.last->output << L" (krok "
                     << v.last->observed_tick << L")";
            if (!v.history.empty()) {
                const auto& e = v.history.back();
                text << L"\nŚwiadek " << e.measurement.source_actor.value << L", poznano w "
                     << e.learned_tick;
            }
            text << L"\nRęcznie: " << practice_input_ << L", "
                 << chain_label(practice_chains()[practice_chain_]);
        }
        SetWindowTextW(status_, text.str().c_str());
        SetWindowTextW(buttons_[0], pacer_.running() ? L"Pauza [Spacja]" : L"Odtwarzaj [Spacja]");
        const auto inspect = session_.inspect() + diagnostics_.report();
        if (inspect != old_inspector_) {
            std::string native_lines;
            native_lines.reserve(inspect.size() + 256);
            for (const auto c : inspect) {
                if (c == '\n')
                    native_lines.push_back('\r');
                native_lines.push_back(c);
            }
            const auto first_line = SendMessageW(inspector_, EM_GETFIRSTVISIBLELINE, 0, 0);
            DWORD selection_start = 0, selection_end = 0;
            SendMessageW(inspector_, EM_GETSEL, reinterpret_cast<WPARAM>(&selection_start),
                         reinterpret_cast<LPARAM>(&selection_end));
            SetWindowTextW(inspector_, wide(native_lines).c_str());
            SendMessageW(inspector_, EM_SETSEL, selection_start, selection_end);
            const auto new_first = SendMessageW(inspector_, EM_GETFIRSTVISIBLELINE, 0, 0);
            SendMessageW(inspector_, EM_LINESCROLL, 0, first_line - new_first);
            old_inspector_ = inspect;
        }
    }
    void clear_keys() {
        keys_ = {};
        session_.clear_input();
    }
    GroundPoint direction() const {
        return camera_.ground_direction(
            static_cast<float>(static_cast<int>(keys_[3]) - static_cast<int>(keys_[1])),
            static_cast<float>(static_cast<int>(keys_[0]) - static_cast<int>(keys_[2])));
    }
    void apply_input() { session_.set_input(static_cast<std::size_t>(selection_), direction()); }
    void choose(int selection) {
        clear_keys();
        selection_ = selection;
        SetFocus(viewport_);
    }
    bool key(WPARAM code, bool down, bool repeat) {
        if (down && !repeat && options_.measure_seconds != 0 && measurement_started_)
            ++measurement_interventions_;
        const auto key_index = code == 'W'   ? 0
                               : code == 'A' ? 1
                               : code == 'S' ? 2
                               : code == 'D' ? 3
                                             : -1;
        if (key_index >= 0) {
            keys_[static_cast<std::size_t>(key_index)] = down;
            if (down && !repeat) {
                session_.queue_input(static_cast<std::size_t>(selection_), direction());
            }
            return true;
        }
        if (!down) {
            return false;
        }
        if (code == 'Q') {
            camera_.orbit(-.12F);
            return true;
        }
        if (code == 'E') {
            camera_.orbit(.12F);
            return true;
        }
        if (code == VK_UP) {
            camera_.tilt(.06F);
            return true;
        }
        if (code == VK_DOWN) {
            camera_.tilt(-.06F);
            return true;
        }
        if (code == VK_OEM_PLUS || code == VK_ADD) {
            camera_.zoom(-.6F);
            return true;
        }
        if (code == VK_OEM_MINUS || code == VK_SUBTRACT) {
            camera_.zoom(.6F);
            return true;
        }
        if (repeat) {
            return false;
        }
        try {
            if (code >= L'1' && code <= L'8') {
                const auto candidate = static_cast<std::size_t>(code - L'1');
                if (candidate < session_.actors().size()) {
                    choose(static_cast<int>(candidate));
                }
            } else if (code == VK_TAB) {
                choose(static_cast<int>((static_cast<std::size_t>(selection_) + 1) %
                                        session_.actors().size()));
            } else if (code == VK_SPACE) {
                pacer_.set_running(!pacer_.running());
                session_.synchronize_presentation();
                rebase_ = true;
            } else if (code == 'N') {
                apply_input();
                session_.advance(20);
                session_.synchronize_presentation();
            } else if (code == 'X') {
                clear_keys();
                session_.stop(static_cast<std::size_t>(selection_));
            } else if (code == VK_F2) {
                clear_keys();
                session_.set_autonomous(
                    static_cast<std::size_t>(selection_),
                    !session_.actors()[static_cast<std::size_t>(selection_)].autonomous);
            } else if (code == 'I' || code == 'O' || code == 'P') {
                if (!session_.has_practice())
                    feedback(L"Ten świat nie zawiera aparatu do prób.");
                else if (code == 'I') {
                    practice_input_ = practice_input_ == 128 ? 16 : practice_input_ + 16;
                    feedback(L"Zmieniono wejście próby.");
                } else if (code == 'O') {
                    practice_chain_ = (practice_chain_ + 1) % practice_chains().size();
                    feedback(L"Zmieniono kolejność modułów.");
                } else if (session_.actors()[static_cast<std::size_t>(selection_)].autonomous)
                    feedback(L"Przejmij postać F2 przed ręczną próbą.");
                else {
                    const auto r = session_.practice_probe(static_cast<std::size_t>(selection_),
                                                           practice_chains()[practice_chain_],
                                                           practice_input_);
                    feedback(
                        r == DiscoveryOutcome::Accepted ? L"Próba przyjęta. Wynik po dwóch krokach."
                        : r == DiscoveryOutcome::OutOfReach ? L"Podejdź do aparatu i zatrzymaj się."
                        : r == DiscoveryOutcome::Busy       ? L"Urządzenie lub wykonawca zajęty."
                        : r == DiscoveryOutcome::NoEnergy
                            ? L"Brak energii urządzenia."
                            : L"Próba odrzucona: zakres lub budżet prób.");
                    log_ << "practice_probe_outcome=" << static_cast<unsigned>(r) << '\n';
                }
            } else if (code == VK_F3) {
                const auto response =
                    session_.request_clearance(static_cast<std::size_t>(selection_));
                const auto label = [&] {
                    switch (response) {
                    case ClearanceOutcome::None:
                        return L"brak osoby blokującej rozpoczęty krok";
                    case ClearanceOutcome::Accepted:
                        return L"zgoda na zrobienie miejsca";
                    case ClearanceOutcome::Controlled:
                        return L"odmowa — sterowanie ręczne";
                    case ClearanceOutcome::Busy:
                        return L"odmowa — zajęta postać";
                    case ClearanceOutcome::NoSpace:
                        return L"odmowa — brak bezpiecznego miejsca";
                    case ClearanceOutcome::Completed:
                        return L"przejście udostępnione";
                    case ClearanceOutcome::Expired:
                        return L"upłynął czas próby";
                    case ClearanceOutcome::Cancelled:
                        return L"manewr przerwany";
                    }
                    throw std::invalid_argument("invalid clearance feedback");
                }();
                feedback(std::wstring{L"Prośba: "} + label);
                log_ << "clearance_request_actor="
                     << session_.actors().at(static_cast<std::size_t>(selection_)).id.value
                     << " response=" << clearance_label(response) << '\n';
            } else if (code == VK_F4) {
                const auto index = static_cast<std::size_t>(selection_);
                if (session_.actors()[index].autonomous) {
                    feedback(L"Przejmij sterowanie F2 przed ręcznym przeładunkiem.");
                } else {
                    std::optional<std::size_t> nearest;
                    double distance = 1e9;
                    for (std::size_t i = 0; i < session_.stations().size(); ++i) {
                        const auto& station = session_.stations()[i];
                        const auto lot = session_.world().resource(station.lot);
                        if (!lot || lot->record.kind != ResourceKind::Material ||
                            lot->record.material.value == 0)
                            continue;
                        const auto point =
                            surface_feet(session_.geometry(), station.location,
                                         TraversalLimits{session_.actors()[index].size});
                        if (point && length(*point - session_.actors()[index].feet) < distance) {
                            distance = length(*point - session_.actors()[index].feet);
                            nearest = i;
                        }
                    }
                    const auto result = nearest ? session_.interact_cargo(index, *nearest)
                                                : CargoStatus::Unavailable;
                    const auto label = [&] {
                        switch (result) {
                        case CargoStatus::Transferred:
                            return L"materiał przekazany";
                        case CargoStatus::OutOfReach:
                            return L"podejdź bliżej stanowiska";
                        case CargoStatus::Busy:
                            return L"zatrzymaj się i dokończ rozpoczętą czynność";
                        case CargoStatus::Empty:
                            return L"brak dostępnego materiału";
                        case CargoStatus::Incompatible:
                            return L"niezgodny materiał";
                        case CargoStatus::Unavailable:
                            return L"brak typowanego stanowiska";
                        case CargoStatus::Denied:
                            return L"transfer odrzucony — sprawdź dziennik";
                        case CargoStatus::Returned:
                            return L"powrót do magazynu potwierdzony";
                        case CargoStatus::Blocked:
                            return L"przeszkoda blokuje dostęp";
                        }
                        throw std::invalid_argument("invalid cargo status");
                    }();
                    feedback(std::wstring{L"Ładunek: "} + label);
                    log_ << "cargo_interaction_actor=" << session_.actors()[index].id.value
                         << " result=" << static_cast<unsigned>(result) << '\n';
                }
            } else if (code == VK_F7) {
                const auto sender = static_cast<std::size_t>(selection_);
                std::optional<std::size_t> nearest;
                double distance = 2.000001;
                for (std::size_t i = 0; i < session_.actors().size(); ++i) {
                    if (i == sender)
                        continue;
                    const auto d =
                        length(session_.actors()[sender].feet - session_.actors()[i].feet);
                    if (d < distance) {
                        distance = d;
                        nearest = i;
                    }
                }
                const auto result = nearest ? session_.share_knowledge(sender, *nearest)
                                            : KnowledgeOutcome::OutOfReach;
                if (session_.has_practice() && nearest) {
                    const auto r = session_.share_practice(sender, *nearest);
                    if (r == DiscoveryOutcome::Accepted) {
                        feedback(
                            L"Rozpoczęto przekaz pomiarów; pozostańcie w kontakcie przez krok.");
                        log_ << "practice_share_started=1\n";
                        return true;
                    }
                }
                const auto label =
                    result == KnowledgeOutcome::Shared    ? L"przekazano obserwacje"
                    : result == KnowledgeOutcome::NoNews  ? L"brak nowszych obserwacji"
                    : result == KnowledgeOutcome::Blocked ? L"przeszkoda lub inna podpora"
                                                          : L"brak osoby w zasięgu 2 m";
                std::wostringstream message;
                message << L"Wiedza postaci " << session_.actors()[sender].id.value;
                if (nearest)
                    message << L" → " << session_.actors()[*nearest].id.value;
                message << L": " << label;
                std::optional<std::size_t> newest;
                const auto& beliefs = session_.actors()[sender].beliefs;
                for (std::size_t i = 0; i < beliefs.size(); ++i)
                    if (beliefs[i] &&
                        (!newest || beliefs[i]->observed_tick > beliefs[*newest]->observed_tick))
                        newest = i;
                std::optional<std::size_t> passage;
                const auto& passages = session_.actors()[sender].passage_beliefs;
                for (std::size_t i = 0; i < session_.passages().size(); ++i)
                    if (passages[i] && (!passage || passages[i]->observed_tick >
                                                        passages[*passage]->observed_tick))
                        passage = i;
                if (passage && (!newest || passages[*passage]->observed_tick >=
                                               beliefs[*newest]->observed_tick)) {
                    const auto& b = *passages[*passage];
                    message << L"\nPrzekonanie: przejście " << session_.passages()[*passage].id
                            << (b.open ? L" otwarte" : L" zamknięte") << L"\nŚwiadek "
                            << b.source_actor.value << L", obserwacja " << b.observed_tick;
                } else if (newest) {
                    const auto& b = *beliefs[*newest];
                    message << L"\nPrzekonanie o " << *newest << L": wolne " << b.available
                            << L"\nŚwiadek " << b.source_actor.value << L", obserwacja "
                            << b.observed_tick;
                } else
                    message << L"\nBrak obserwacji stanowisk.";
                feedback(message.str());
                log_ << "knowledge_sender=" << session_.actors()[sender].id.value
                     << " recipient=" << (nearest ? session_.actors()[*nearest].id.value : 0)
                     << " outcome=" << static_cast<unsigned>(result) << '\n';
            } else if (code == VK_F8) {
                const auto index = static_cast<std::size_t>(selection_);
                std::optional<std::size_t> nearest;
                double best = 1e9;
                for (std::size_t i = 0; i < session_.passages().size(); ++i) {
                    const auto distance =
                        length(session_.geometry().body(session_.passages()[i].surface).origin -
                               session_.actors()[index].feet);
                    if (distance < best) {
                        best = distance;
                        nearest = i;
                    }
                }
                const auto result =
                    nearest ? session_.interact_passage(index, session_.passages()[*nearest].id,
                                                        !session_.passages()[*nearest].open)
                            : PassageOutcome::Unavailable;
                const auto label =
                    result == PassageOutcome::Changed      ? L"stan zmieniony"
                    : result == PassageOutcome::Unchanged  ? L"bez zmiany"
                    : result == PassageOutcome::Controlled ? L"przejmij postać F2"
                    : result == PassageOutcome::Busy       ? L"zatrzymaj się i dokończ czynność"
                    : result == PassageOutcome::Occupied
                        ? L"odmowa: postać, Step lub ustąpienie na przejściu"
                    : result == PassageOutcome::OutOfReach ? L"podejdź do przyczółka"
                    : result == PassageOutcome::Worn       ? L"konieczna materialna naprawa"
                                                           : L"brak przejścia";
                feedback(std::wstring{L"Przejście: "} + label);
                log_ << "passage_interaction=" << static_cast<unsigned>(result) << '\n';
            } else if (code == 'R') {
                const auto index = static_cast<std::size_t>(selection_);
                std::optional<std::size_t> nearest;
                double best = 1e9;
                for (std::size_t i = 0; i < session_.stations().size(); ++i) {
                    const auto& station = session_.stations()[i];
                    if (station.kind != TraversalTask::Maintain)
                        continue;
                    const auto feet = surface_feet(session_.geometry(), station.location,
                                                   TraversalLimits{session_.actors()[index].size});
                    if (feet && length(*feet - session_.actors()[index].feet) < best) {
                        best = length(*feet - session_.actors()[index].feet);
                        nearest = i;
                    }
                }
                const auto started = nearest && session_.begin_maintenance(index, *nearest);
                feedback(started ? L"Naprawa rozpoczęta; koszt i praca w WorkWorld."
                         : session_.has_practice()
                             ? L"Naprawa wymaga dojścia, części, uprawnienia i udanej próby "
                               L"ustawienia [P]."
                             : L"Naprawa: przejmij postać, zatrzymaj się i podejdź do "
                               L"stanowiska z materiałem.");
                log_ << "maintenance_started=" << started << '\n';
            } else if (code == VK_F5) {
                session_.save(options_.save);
                feedback(L"Zapisano: " + options_.save.filename().wstring());
            } else if (code == VK_F6) {
                capture_diagnostic("manual_F6");
            } else if (code == VK_F9) {
                session_.load(options_.save);
                diagnostics_.reset();
                clear_keys();
                pacer_.reset();
                selection_ = std::min(selection_, static_cast<int>(session_.actors().size() - 1));
                rebase_ = true;
                feedback(L"Wczytano świat. Pauza.");
            } else {
                return false;
            }
        } catch (const std::invalid_argument& error) {
            feedback(L"Odmowa: " + wide(error.what()));
            log_ << "command_rejected=" << error.what() << '\n';
        }
        return true;
    }
    void click(int x, int y, bool right, bool approach) {
        if (options_.measure_seconds != 0 && measurement_started_)
            ++measurement_interventions_;
        if (!renderer_) {
            return;
        }
        SetFocus(viewport_);
        const auto nx = 2.0F * static_cast<float>(x) / static_cast<float>(renderer_->width()) - 1;
        const auto ny = 1 - 2.0F * static_cast<float>(y) / static_cast<float>(renderer_->height());
        const auto object = pick_scene(scene_, nx, ny);
        if (!right) {
            if (object && *object >= 0 &&
                static_cast<std::size_t>(*object) < session_.actors().size()) {
                choose(*object);
            }
            return;
        }
        auto target =
            pick_traversal_surface(session_, camera_, nx, ny, aspect(), fraction(), approach);
        if (object && *object != selection_ && *object >= 0 &&
            static_cast<std::size_t>(*object) < session_.actors().size()) {
            target = session_.actors()[static_cast<std::size_t>(*object)].support;
        }
        if (!target) {
            feedback(L"Brak powierzchni celu. Shift+PPM pozwala podejść do przeszkody.");
            return;
        }
        clear_keys();
        try {
            if (approach) {
                session_.approach(static_cast<std::size_t>(selection_), *target, 1);
            } else {
                session_.set_goal(static_cast<std::size_t>(selection_), *target);
            }
            feedback(approach ? L"Podejście w zasięg 1 m." : L"Wyznaczono dokładny cel.");
        } catch (const std::invalid_argument& error) {
            feedback(L"Cel odrzucony: " + wide(error.what()));
            log_ << "goal_rejected=" << error.what() << '\n';
        }
    }
    void crossing_smoke() {
        if (options_.resume) {
            session_.load(options_.save);
            session_.advance(25);
            session_.validate();
            log_ << "passage_resume=pass\n";
        } else {
            const auto stock =
                session_.world().resource(session_.stations()[2].lot)->record.quantity.value;
            const auto version = session_.geometry().version;
            SendMessageW(window_, WM_KEYDOWN, VK_F8, 0);
            require(!session_.passages()[0].open && session_.geometry().version == version + 1,
                    "F8 did not locally close bridge");
            draw();
            require(renderer_->capture(
                        std::filesystem::path{options_.capture.wstring() + L".closed.png"}) > 1000,
                    "closed crossing image empty");
            SendMessageW(window_, WM_KEYDOWN, 'R', 0);
            require(
                session_.actors()[0].action.value != 0 &&
                    session_.world().resource(session_.stations()[2].lot)->record.quantity.value ==
                        stock - 1,
                "R did not start material repair");
            session_.advance(79);
            require(!session_.passages()[0].open, "native unfinished repair opened bridge");
            session_.advance(1);
            require(session_.passages()[0].open && session_.stations()[2].completed == 1,
                    "native completed repair did not open bridge");
            log_ << "passage_shortcut=pass\nmaintenance_shortcut=pass\n";
            session_.save(options_.save);
            for (unsigned tick = 0; tick < 550 && session_.stations()[1].completed == 0; ++tick)
                session_.advance(1);
            require(session_.actors()[1].delivery->delivered > 0 &&
                        session_.stations()[1].completed > 0,
                    "native crossing did not deliver food and enable meal");
            log_ << "crossing_delivery=pass\n";
        }
        panel();
        std::wstring lines(static_cast<std::size_t>(GetWindowTextLengthW(inspector_)) + 1, L'\0');
        GetWindowTextW(inspector_, lines.data(), static_cast<int>(lines.size()));
        require(lines.find(L"passage_belief=") != std::wstring::npos &&
                    lines.find(L"passage=1") != std::wstring::npos,
                "native passage inspector lost truth or belief");
        log_ << "passage_inspector=pass\n";
        session_.validate();
        draw();
        require(renderer_->capture(options_.capture) > 1000, "crossing GPU image empty");
        renderer_->check_diagnostics();
        log_ << session_.inspect() << "TRAVERSAL SMOKE PASS\n";
    }
    void community_smoke() {
        if (!options_.resume) {
            const auto initial = session_;
            TraversalSession other{make_law_course()};
            other.save(options_.save);
            SendMessageW(window_, WM_KEYDOWN, VK_F9, 0);
            require(session_.actors().size() == 4, "foreign save fixture did not load through F9");
            panel();
            session_ = initial;
            panel();
            log_ << "community_foreign_save=pass\n";
        }
        if (options_.resume) {
            session_.load(options_.save);
            TraversalSession copy;
            copy.restore(session_.serialize());
            for (unsigned t = 0; t < 200; ++t) {
                session_.advance(1);
                copy.advance(1);
                session_.validate();
                copy.validate();
                require(session_.serialize() == copy.serialize(),
                        "community fresh-process continuation differs");
            }
            require(session_.serialize() == read_snapshot_bytes(std::filesystem::path{
                                                options_.save.wstring() + L".expected.save"}),
                    "fresh community process differs from original continuation");
            require(session_.actors()[1].delivery->delivered > 0 &&
                        session_.stations()[1].completed > 0,
                    "integrated continuation did not deliver food and feed residents");
            log_ << "community_resume=pass\n";
        } else {
            session_.advance(12);
            SendMessageW(window_, WM_KEYDOWN, VK_F8, 0);
            require(!session_.passages()[0].open && !session_.actors()[1].passage_beliefs[0],
                    "community closure leaked truth");
            log_ << "community_passage=pass\n";
            SendMessageW(window_, WM_KEYDOWN, VK_F4, 0);
            require(session_.incidents().size() == 1 && session_.actors()[4].testimonies.empty(),
                    "community physical theft lost witness separation");
            SendMessageW(window_, WM_KEYDOWN, VK_F7, 0);
            require(session_.actors()[3].beliefs[2] &&
                        session_.actors()[3].beliefs[2]->source_actor == session_.actors()[0].id,
                    "community F7 did not physically share stock observation");
            log_ << "community_share=pass\n";
            session_.advance(1);
            require(!session_.legal_service_allowed(0, 2),
                    "community custodian did not receive local report");
            SendMessageW(window_, WM_KEYDOWN, 'R', 0);
            require(session_.actors()[0].action.value == 0, "community legal refusal failed");
            SendMessageW(window_, WM_KEYDOWN, VK_F4, 0);
            session_.advance(1);
            require(session_.incidents()[0].returned == 4 && session_.legal_service_allowed(0, 2),
                    "community restitution did not reach custodian");
            log_ << "community_law=pass\n";
            SendMessageW(window_, WM_KEYDOWN, 'R', 0);
            require(session_.actors()[0].action.value != 0,
                    "community settled player could not help");
            session_.advance(226);
            session_.validate();
            require(session_.stations()[2].completed >= 2 && session_.stations()[3].completed > 0,
                    "community did not complete work and rest");
            session_.save(options_.save);
            auto expected = session_;
            expected.advance(200);
            expected.validate();
            expected.save(std::filesystem::path{options_.save.wstring() + L".expected.save"});
            log_ << "community_work=pass\n";
        }
        panel();
        std::wstring status(static_cast<std::size_t>(GetWindowTextLengthW(status_)) + 1, L'\0');
        GetWindowTextW(status_, status.data(), static_cast<int>(status.size()));
        require(status.find(L"M04") != std::wstring::npos &&
                    status.find(L"Posiłki:") != std::wstring::npos &&
                    status.find(L"Obsługa podróżnego:") != std::wstring::npos,
                "community panel lost visible consequences");
        draw();
        require(renderer_->capture(options_.capture) > 1000, "community GPU image empty");
        renderer_->check_diagnostics();
        log_ << "community_panel=pass\n" << session_.inspect() << "TRAVERSAL SMOKE PASS\n";
    }
    void law_smoke() {
        if (options_.resume) {
            session_.load(options_.save);
            require(!session_.legal_service_allowed(0, 0), "loaded institution forgot testimony");
            TraversalSession copy;
            copy.restore(session_.serialize());
            for (unsigned t = 0; t < 20; ++t) {
                session_.advance(1);
                copy.advance(1);
                require(session_.serialize() == copy.serialize(),
                        "native law continuation differs");
            }
            require(session_.interact_cargo(0, 0) == CargoStatus::Transferred,
                    "native physical return failed");
            require(!session_.legal_service_allowed(0, 0), "native remote closure leaked");
            session_.set_goal(1, {1, {1.2, -.8}});
            for (unsigned t = 0;
                 t < 200 && (session_.actors()[1].state != TraversalState::Arrived ||
                             session_.actors()[1].motion.speed > .01);
                 ++t)
                session_.advance(1);
            require(session_.actors()[1].testimonies.size() == 1,
                    "late witness saw historical return");
            log_ << "law_resume=pass\n";
        } else {
            selection_ = 0;
            SendMessageW(window_, WM_KEYDOWN, VK_F4, 0);
            require(session_.incidents().size() == 1 &&
                        session_.actors()[1].testimonies.size() == 1 &&
                        session_.actors()[2].testimonies.empty() &&
                        session_.legal_service_allowed(0, 0),
                    "F4 theft bypassed witness model");
            session_.set_goal(1, {1, {-3.5, 0}});
            for (unsigned t = 0;
                 t < 200 && (session_.actors()[1].state != TraversalState::Arrived ||
                             session_.actors()[1].motion.speed > .01);
                 ++t)
                session_.advance(1);
            selection_ = 1;
            SendMessageW(window_, WM_KEYDOWN, VK_F7, 0);
            require(!session_.legal_service_allowed(0, 0) && !session_.begin_maintenance(0, 0),
                    "native F7 did not cause service refusal");
            session_.save(options_.save);
            log_ << "law_pickup=pass\nlaw_exchange=pass\nlaw_refusal=pass\n";
        }
        panel();
        std::wstring lines(static_cast<std::size_t>(GetWindowTextLengthW(inspector_)) + 1, L'\0');
        GetWindowTextW(inspector_, lines.data(), static_cast<int>(lines.size()));
        require(lines.find(L"testimony_incident=") != std::wstring::npos &&
                    lines.find(L"incident_truth=") != std::wstring::npos &&
                    lines.find(L"allowed=0") != std::wstring::npos,
                "native law inspector lost truth, memory or reaction");
        session_.validate();
        draw();
        require(renderer_->capture(options_.capture) > 1000, "law GPU image empty");
        renderer_->check_diagnostics();
        log_ << "law_inspector=pass\n" << session_.inspect() << "TRAVERSAL SMOKE PASS\n";
    }
    void practice_smoke() {
        require(session_.has_practice() && session_.actors().size() == 8,
                "learning flag did not create the community apparatus");
        choose(3);
        SendMessageW(window_, WM_KEYDOWN, VK_F2, 0);
        session_.approach(3, session_.stations()[2].location, .7);
        session_.advance(40);
        session_.stop(3);
        session_.advance(20);
        log_ << "practice_manual_position=" << session_.actors()[3].feet.x << ' '
             << session_.actors()[3].feet.z << " speed=" << session_.actors()[3].motion.speed
             << " contact=" << session_.can_interact_at(3, session_.stations()[2].location) << '\n';
        const auto& chains = practice_chains();
        practice_chain_ = static_cast<std::size_t>(
            std::find(chains.begin(), chains.end(), DiscoveryChain{0, 0}) - chains.begin());
        require(practice_chain_ < chains.size(), "manual chain missing");
        SendMessageW(window_, WM_KEYDOWN, 'P', 0);
        require(session_.practice_view(3).stage == PracticeStage::Configuring,
                "P did not begin a physical pulse");
        SendMessageW(window_, WM_KEYDOWN, VK_F5, 0);
        const auto checkpoint = session_.serialize();
        auto expected = session_;
        expected.advance(2);
        SendMessageW(window_, WM_KEYDOWN, VK_F9, 0);
        session_.advance(2);
        require(session_.serialize() == expected.serialize(), "F5/F9 pulse continuation differs");
        SendMessageW(window_, WM_KEYDOWN, 'R', 0);
        require(session_.actors()[3].action.value != 0, "R bypassed or lost calibrated work");
        session_.advance(80);
        require(session_.stations()[2].completed > 0, "native maintenance not completed");
        SendMessageW(window_, WM_KEYDOWN, VK_F2, 0);
        session_.advance(240);
        require(session_.practice_view(3).method_known || session_.practice_view(5).method_known,
                "native community did not learn");
        const auto learned = session_.serialize();
        TraversalSession foreign;
        foreign.save(options_.save);
        SendMessageW(window_, WM_KEYDOWN, VK_F9, 0);
        panel();
        draw();
        require(!session_.has_practice(), "foreign save retained device");
        write_snapshot_atomic(learned, options_.save);
        SendMessageW(window_, WM_KEYDOWN, VK_F9, 0);
        require(session_.serialize() == learned,
                "learning state did not return after foreign save");
        SendMessageW(window_, WM_KEYDOWN, 'I', 0);
        SendMessageW(window_, WM_KEYDOWN, 'O', 0);
        panel();
        draw();
        std::wstring status(static_cast<std::size_t>(GetWindowTextLengthW(status_)) + 1, L'\0');
        GetWindowTextW(status_, status.data(), static_cast<int>(status.size()));
        require(status.find(L"Praktyka") != std::wstring::npos &&
                    status.find(L"Pomiary") != std::wstring::npos,
                "learning panel missing");
        require(renderer_->capture(options_.capture) > 1000, "practice GPU capture empty");
        session_.validate();
        renderer_->check_diagnostics();
        log_ << "practice_controls=pass\npractice_foreign_save=pass\npractice_panel=pass\npractice_"
                "checkpoint_bytes="
             << checkpoint.size() << '\n';
        for (std::size_t i : {3U, 5U}) {
            const auto v = session_.practice_view(i);
            log_ << "practice_actor=" << i << " probes=" << v.probes << " evidence=" << v.records
                 << " method=" << v.method_known << " reads=" << v.reads << '\n';
        }
        log_ << session_.inspect() << "TRAVERSAL SMOKE PASS\n";
    }
    void two_methods_smoke() {
        const auto start = std::chrono::steady_clock::now();
        const auto timing = [&](const char* stage) {
            log_ << "two_methods_time_" << stage << '='
                 << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
                 << '\n';
        };
        require(!session_.passages()[0].open && session_.stations()[2].wear == 1000 &&
                    !session_.actors()[1].autonomous && !session_.actors()[3].autonomous,
                "two-methods did not leave the initial choice to the player");
        {
            const auto repair = options_.repair_smoke;
            choose(repair ? 3 : 1);
            SendMessageW(window_, WM_KEYDOWN, VK_F2, 0);
            bool bridge = false, ferry = false, checkpointed = false;
            while (session_.world().clock().tick_count() < 1200) {
                if (repair && session_.passages()[0].open && !session_.actors()[1].autonomous) {
                    choose(1);
                    SendMessageW(window_, WM_KEYDOWN, VK_F2, 0);
                }
                session_.advance(1);
                const auto& courier = session_.actors()[1];
                bridge = bridge || courier.support.surface == session_.passages()[0].surface;
                ferry = ferry || courier.transfer == TransferStage::Riding;
                const auto ready = repair ? session_.actors()[3].action.value != 0
                                          : courier.transfer == TransferStage::Riding;
                if (ready && !checkpointed) {
                    if (repair)
                        require(
                            session_.actors()[3].task == TraversalTask::Maintain &&
                                session_.actors()[3].station == 2 &&
                                session_.world().action(session_.actors()[3].action)->request.lot ==
                                    session_.stations()[2].lot,
                            "native repair checkpoint is not maintenance at the bridge");
                    SendMessageW(window_, WM_KEYDOWN, VK_F5, 0);
                    timing("checkpoint_saved");
                    auto expected = session_;
                    expected.advance(3);
                    SendMessageW(window_, WM_KEYDOWN, VK_F9, 0);
                    timing("checkpoint_loaded");
                    session_.advance(3);
                    require(session_.serialize() == expected.serialize(),
                            "two-methods F5/F9 lost running work or a loaded ferry");
                    checkpointed = true;
                }
                if (session_.actors()[1].delivery->delivered == 8 &&
                    session_.stations()[1].completed > 0)
                    break;
            }
            require(checkpointed && session_.actors()[1].delivery->delivered == 8 &&
                        session_.stations()[1].completed > 0,
                    "native method did not physically deliver and feed the recipient");
            require(repair ? bridge && !ferry && session_.stations()[2].completed > 0 &&
                                 session_.practice_view(3).method_known
                           : ferry && !bridge && !session_.passages()[0].open &&
                                 session_.stations()[2].completed == 0,
                    "native method did not preserve its actual route and consequences");
            session_.validate();
            timing("delivered");
            log_ << (repair ? "two_methods_repair=pass\n" : "two_methods_detour=pass\n");
        }
        const auto saved = session_.serialize();
        TraversalSession foreign;
        foreign.save(options_.save);
        SendMessageW(window_, WM_KEYDOWN, VK_F9, 0);
        timing("foreign_loaded");
        panel();
        draw();
        require(!session_.has_practice(), "two-methods foreign save retained practice");
        write_snapshot_atomic(saved, options_.save);
        SendMessageW(window_, WM_KEYDOWN, VK_F9, 0);
        timing("return_loaded");
        require(session_.serialize() == saved, "two-methods foreign save failed to return");
        choose(3);
        panel();
        draw();
        std::wstring status(static_cast<std::size_t>(GetWindowTextLengthW(status_)) + 1, L'\0');
        GetWindowTextW(status_, status.data(), static_cast<int>(status.size()));
        require(status.find(L"M05") != std::wstring::npos &&
                    status.find(L"Dostawa: 8/8") != std::wstring::npos &&
                    status.find(L"Praktyka") != std::wstring::npos,
                "two-methods panel lost visible progress");
        require(renderer_->capture(options_.capture) > 1000, "two-methods image empty");
        renderer_->check_diagnostics();
        log_ << "two_methods_foreign_save=pass\ntwo_methods_panel=pass\n"
             << session_.inspect() << "TRAVERSAL SMOKE PASS\n";
    }
    void smoke() {
        if (options_.two_methods) {
            two_methods_smoke();
            return;
        }
        if (options_.learning) {
            practice_smoke();
            return;
        }
        if (options_.community) {
            community_smoke();
            return;
        }
        if (options_.law) {
            law_smoke();
            return;
        }
        if (options_.crossing) {
            crossing_smoke();
            return;
        }
        panel();
        std::wstring inspector_text(static_cast<std::size_t>(GetWindowTextLengthW(inspector_)) + 1,
                                    L'\0');
        GetWindowTextW(inspector_, inspector_text.data(), static_cast<int>(inspector_text.size()));
        require(inspector_text.find(L"\r\nactor=") != std::wstring::npos,
                "inspector does not separate actor records with native line endings");
        log_ << "inspector_records=pass\n";
        SendMessageW(inspector_, EM_LINESCROLL, 0, 3);
        const auto scroll_line = SendMessageW(inspector_, EM_GETFIRSTVISIBLELINE, 0, 0);
        require(scroll_line > 0, "inspector scroll fixture did not move");
        session_.advance(1);
        panel();
        require(SendMessageW(inspector_, EM_GETFIRSTVISIBLELINE, 0, 0) == scroll_line,
                "live inspector update reset the reading position");
        log_ << "inspector_scroll=pass\n";
        const auto before_request = session_.serialize();
        SendMessageW(window_, WM_KEYDOWN, VK_F3, 0);
        require(session_.serialize() == before_request,
                "irrelevant clearance request changed world");
        log_ << "clearance_shortcut=pass\n";
        SendMessageW(window_, WM_KEYDOWN, VK_F4, 0);
        require(session_.serialize() == before_request, "remote cargo shortcut changed world");
        log_ << "cargo_shortcut=pass\n";
        SendMessageW(window_, WM_KEYDOWN, VK_F7, 0);
        require(session_.serialize() == before_request,
                "irrelevant knowledge shortcut changed world");
        log_ << "knowledge_shortcut=pass\n";
        {
            // Isolated physical fixture exercises the actual window shortcut and
            // native inspector, then returns to the original smoke scenario.
            const auto saved_session = session_;
            const auto saved_selection = selection_;
            TraversalScene knowledge_scene;
            knowledge_scene.geometry.bodies = {{1, {}, 0, 4, 4, .3, 0, 0, true}};
            knowledge_scene.stations = {{{1, {0, 0}}, TraversalTask::Eat, 0}};
            knowledge_scene.stations[0].initial_material = {1};
            knowledge_scene.spawns = {{{1, {0, -.8}}, {}, false}, {{1, {-1.5, -.8}}, {}, false}};
            session_ = TraversalSession{knowledge_scene};
            require(session_.interact_cargo(0, 0) == CargoStatus::Empty,
                    "native witness did not physically observe empty station");
            selection_ = 0;
            SendMessageW(window_, WM_KEYDOWN, VK_F7, 0);
            const auto& belief = session_.actors()[1].beliefs[0];
            require(belief && belief->source_actor == session_.actors()[0].id &&
                        belief->observed_tick == 0 && belief->available == 0,
                    "native F7 did not deliver the eyewitness report");
            selection_ = 1;
            panel();
            std::wstring lines(static_cast<std::size_t>(GetWindowTextLengthW(inspector_)) + 1,
                               L'\0');
            GetWindowTextW(inspector_, lines.data(), static_cast<int>(lines.size()));
            require(lines.find(L"belief_station=0 available=0") != std::wstring::npos &&
                        lines.find(L"source_actor=") != std::wstring::npos &&
                        lines.find(L"observed_tick=0") != std::wstring::npos,
                    "native inspector does not expose belief provenance");
            session_.validate();
            log_ << "knowledge_exchange=pass\nknowledge_inspector=pass\n";
            session_ = saved_session;
            selection_ = saved_selection;
            panel();
        }
        if (options_.resume) {
            session_.load(options_.save);
            session_.advance(40);
            session_.validate();
        } else {
            draw();
            const auto feet = session_.actors().at(static_cast<std::size_t>(selection_)).feet;
            const auto self =
                camera_.project({static_cast<float>(feet.x), static_cast<float>(feet.y + .9),
                                 static_cast<float>(feet.z)},
                                aspect());
            const auto self_x =
                static_cast<int>((self.x + 1) * .5F * static_cast<float>(renderer_->width()));
            const auto self_y =
                static_cast<int>((1 - self.y) * .5F * static_cast<float>(renderer_->height()));
            const auto nx =
                2.0F * static_cast<float>(self_x) / static_cast<float>(renderer_->width()) - 1;
            const auto ny =
                1 - 2.0F * static_cast<float>(self_y) / static_cast<float>(renderer_->height());
            require(pick_scene(scene_, nx, ny) == selection_,
                    "self-picking fixture missed selected capsule");
            const auto underneath =
                pick_traversal_surface(session_, camera_, nx, ny, aspect(), fraction(), false);
            require(underneath.has_value() &&
                        *underneath !=
                            session_.actors()[static_cast<std::size_t>(selection_)].support,
                    "self-picking fixture did not separate surface from actor");
            SendMessageW(viewport_, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(self_x, self_y));
            require(session_.actors()[static_cast<std::size_t>(selection_)].goal == underneath,
                    "selected capsule replaced the surface destination");
            log_ << "self_surface_picking=pass\n";
            auto before = options_.capture;
            before += L".before.png";
            require(renderer_->capture(before) > 1000, "traversal initial GPU image empty");
            const auto point = camera_.project({7, 1.5F, 3.5F}, aspect());
            const auto x =
                static_cast<int>((point.x + 1) * .5F * static_cast<float>(renderer_->width()));
            const auto y =
                static_cast<int>((1 - point.y) * .5F * static_cast<float>(renderer_->height()));
            SendMessageW(viewport_, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(x, y));
            require(session_.actors()[0].goal && session_.actors()[0].goal->surface == 6,
                    "native upper-level goal picking failed");
            log_ << "picked_surface=6\n";
            SendMessageW(window_, WM_KEYDOWN, 'W', 0);
            apply_input();
            session_.advance(1);
            SendMessageW(window_, WM_KEYUP, 'W', 0);
            apply_input();
            session_.advance(20);
            require(!session_.actors()[0].goal, "native manual movement did not cancel goal");
            SendMessageW(viewport_, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(x, y));
            session_.advance(20);
            session_.validate();
            SendMessageW(window_, WM_KEYDOWN, VK_F5, 0);
            const auto saved = session_.serialize();
            SendMessageW(window_, WM_KEYDOWN, VK_F9, 0);
            require(session_.serialize() == saved, "native save/load reset traversal");
            session_.advance(40);
            session_.validate();
        }
        log_ << "continuation_checksum=" << snapshot_checksum(session_.serialize()) << '\n';
        const auto before_diagnostic = session_.serialize();
        SendMessageW(window_, WM_KEYDOWN, VK_F6, 0);
        require(last_diagnostic_.has_value(), "native F6 capture failed");
        auto diagnostic_world = session_;
        diagnostic_world.load(*last_diagnostic_ / "world.save");
        require(diagnostic_world.serialize() == before_diagnostic &&
                    session_.serialize() == before_diagnostic,
                "native F6 capture changed the world");
        draw();
        require(renderer_->capture(options_.capture) > 1000, "traversal final GPU image empty");
        const auto tick = session_.world().clock().tick_count();
        ShowWindow(window_, SW_MINIMIZE);
        require(minimized_, "minimize was not observed");
        ShowWindow(window_, SW_RESTORE);
        require(!minimized_ && session_.world().clock().tick_count() == tick,
                "restore advanced world clock");
        SetWindowPos(window_, nullptr, 0, 0, 1050, 760, SWP_NOMOVE | SWP_NOZORDER);
        renderer_->check_diagnostics();
        log_ << session_.inspect() << "minimized_restored_tick=" << tick
             << "\nTRAVERSAL SMOKE PASS\n";
    }
    static TraversalWindow* bind(HWND window, UINT message, LPARAM parameter) {
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(parameter);
            SetWindowLongPtrW(window, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }
        return reinterpret_cast<TraversalWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }
    static LRESULT CALLBACK main_proc(HWND window, UINT message, WPARAM word, LPARAM parameter) {
        auto* self = bind(window, message, parameter);
        if (!self) {
            return DefWindowProcW(window, message, word, parameter);
        }
        try {
            if (message == WM_SIZE) {
                if (self->measurement_started_ && self->options_.measure_seconds != 0)
                    ++self->measurement_resizes_;
                if (self->measurement_started_ && self->options_.measure_seconds != 0 &&
                    word == SIZE_MINIMIZED)
                    ++self->measurement_minimizations_;
                self->minimized_ = word == SIZE_MINIMIZED;
                self->clear_keys();
                if (!self->minimized_) {
                    self->layout();
                }
                self->rebase_ = true;
                return 0;
            }
            if (message == WM_ENTERSIZEMOVE) {
                self->sizing_ = true;
                self->clear_keys();
                return 0;
            }
            if (message == WM_EXITSIZEMOVE) {
                self->sizing_ = false;
                self->session_.synchronize_presentation();
                self->rebase_ = true;
                return 0;
            }
            if (message == WM_ACTIVATEAPP && !word) {
                self->clear_keys();
                return 0;
            }
            if (message == WM_KEYDOWN || message == WM_KEYUP) {
                if (self->key(word, message == WM_KEYDOWN, (parameter & (1LL << 30)) != 0)) {
                    return 0;
                }
            }
            if (message == WM_COMMAND) {
                const auto control = LOWORD(word);
                const auto code = control == 1001   ? VK_SPACE
                                  : control == 1002 ? 'N'
                                  : control == 1003 ? VK_F2
                                  : control == 1004 ? VK_F5
                                  : control == 1005 ? VK_F9
                                                    : 0;
                if (code) {
                    self->key(static_cast<WPARAM>(code), true, false);
                    return 0;
                }
            }
            if (message == WM_DESTROY) {
                PostQuitMessage(0);
                return 0;
            }
        } catch (...) {
            self->fatal_ = std::current_exception();
        }
        return DefWindowProcW(window, message, word, parameter);
    }
    static LRESULT CALLBACK view_proc(HWND window, UINT message, WPARAM word, LPARAM parameter) {
        auto* self = bind(window, message, parameter);
        if (!self) {
            return DefWindowProcW(window, message, word, parameter);
        }
        try {
            if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN) {
                self->click(GET_X_LPARAM(parameter), GET_Y_LPARAM(parameter),
                            message == WM_RBUTTONDOWN, (word & MK_SHIFT) != 0);
                return 0;
            }
            if (message == WM_MOUSEWHEEL) {
                self->camera_.zoom(-static_cast<float>(GET_WHEEL_DELTA_WPARAM(word)) / WHEEL_DELTA);
                return 0;
            }
            if (message == WM_KILLFOCUS) {
                self->clear_keys();
                return 0;
            }
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_PAINT) {
                PAINTSTRUCT paint{};
                BeginPaint(window, &paint);
                EndPaint(window, &paint);
                return 0;
            }
        } catch (...) {
            self->fatal_ = std::current_exception();
        }
        return DefWindowProcW(window, message, word, parameter);
    }
};
} // namespace
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    Options configuration;
    std::ofstream log;
    bool apartment = false;
    try {
        read_options(configuration);
        log.open(configuration.report, std::ios::binary | std::ios::trunc);
        require(log.is_open(), "cannot open traversal report");
        log.exceptions(std::ios::badbit | std::ios::failbit);
        require(SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)),
                "COM initialization failed");
        apartment = true;
        require(SetProcessDPIAware() != 0, "DPI awareness initialization failed");
        int result = 0;
        {
            TraversalWindow app{instance, configuration, log};
            result = app.run();
        }
        CoUninitialize();
        return result;
    } catch (const std::exception& error) {
        log.exceptions(std::ios::goodbit);
        if (log.is_open()) {
            log << "ERROR: " << error.what() << '\n';
            log.flush();
        }
        if (!configuration.unattended) {
            MessageBoxW(nullptr, wide(error.what()).c_str(), L"Genesis — błąd rozszerzenia ruchu",
                        MB_OK | MB_ICONERROR);
        }
        if (apartment) {
            CoUninitialize();
        }
        return 1;
    }
}
