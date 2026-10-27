#include "graphics/d3d11_renderer.hpp"
#include "lab/lab_session.hpp"
#include "lab/motion_session.hpp"

#include <shellapi.h>
#include <windowsx.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace reality;
using namespace std::chrono_literals;
constexpr wchar_t main_class[] = L"GenesisLaboratoryWindow";
constexpr wchar_t scene_class[] = L"GenesisD3D11Viewport";

enum Control : int {
    SelectA = 101,
    SelectB,
    SelectLot,
    Reserve,
    Begin,
    Cancel,
    Step,
    Play,
    Save,
    Load,
    MainGate,
    DetourGate,
    Goal,
    InspectNearby
};
struct Options {
    bool smoke = false;
    bool gpu_debug = false;
    bool motion = false;
    bool resume_smoke = false;
    int adapter = -1;
    std::filesystem::path save;
    std::filesystem::path report;
    std::filesystem::path capture;
};

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::wstring_view travel_label(TravelState state) noexcept {
    switch (state) {
    case TravelState::Idle:
        return L"Gotowy";
    case TravelState::Following:
        return L"Ruch do celu";
    case TravelState::Arrived:
        return L"Cel osiągnięty";
    case TravelState::NoPath:
        return L"Brak drogi do celu";
    case TravelState::InvalidEndpoint:
        return L"Cel niedostępny";
    case TravelState::BudgetExceeded:
        return L"Brak budżetu planowania";
    case TravelState::Waiting:
        return L"Czeka: przeszkoda";
    }
    return L"Nieprawidłowy stan ruchu";
}

std::wstring wide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    UINT page = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    auto size =
        MultiByteToWideChar(page, flags, text.data(), static_cast<int>(text.size()), nullptr, 0);
    // Windows system_error messages may use the system code page, unlike domain text.
    if (size == 0 && GetLastError() == ERROR_NO_UNICODE_TRANSLATION) {
        page = CP_ACP;
        flags = 0;
        size = MultiByteToWideChar(page, flags, text.data(), static_cast<int>(text.size()), nullptr,
                                   0);
    }
    require(size != 0, "UTF-8 conversion failed");
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    require(MultiByteToWideChar(page, flags, text.data(), static_cast<int>(text.size()),
                                result.data(), size) != 0,
            "UTF-8 conversion failed");
    return result;
}

std::string utf8(std::wstring_view text) {
    if (text.empty()) {
        return {};
    }
    const auto size =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    require(size != 0, "UTF-16 conversion failed");
    std::string result(static_cast<std::size_t>(size), '\0');
    require(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                static_cast<int>(text.size()), result.data(), size, nullptr,
                                nullptr) != 0,
            "UTF-16 conversion failed");
    return result;
}

struct LocalFreeDeleter {
    void operator()(wchar_t** value) const noexcept { LocalFree(value); }
};
void options(Options& result) {
    bool custom_save = false;
    wchar_t module[32768]{};
    const auto length = GetModuleFileNameW(nullptr, module, 32768);
    require(length != 0 && length < 32768, "executable path unavailable");
    const auto directory = std::filesystem::path{module}.parent_path();
    result.save = directory / "lab-world.save";
    result.report = directory / "genesis-lab.log";
    result.capture = directory / "genesis-lab.png";
    int count = 0;
    const std::unique_ptr<wchar_t*, LocalFreeDeleter> args{
        CommandLineToArgvW(GetCommandLineW(), &count)};
    require(args != nullptr, "CommandLineToArgvW failed");
    // Establish error presentation before validating values, even if an earlier value is bad.
    for (int index = 1; index < count; ++index) {
        const std::wstring_view argument{args.get()[index]};
        if (argument == L"--smoke-test") {
            result.smoke = true;
        } else if (argument == L"--report" && index + 1 < count) {
            result.report = args.get()[++index];
        } else if ((argument == L"--save" || argument == L"--capture" ||
                    argument == L"--adapter") &&
                   index + 1 < count) {
            ++index;
        }
    }
    for (int index = 1; index < count; ++index) {
        const std::wstring_view argument{args.get()[index]};
        if (argument == L"--smoke-test") {
            result.smoke = true;
        } else if (argument == L"--gpu-debug") {
            result.gpu_debug = true;
        } else if (argument == L"--motion") {
            result.motion = true;
        } else if (argument == L"--resume-smoke") {
            result.resume_smoke = true;
        } else {
            require(index + 1 < count, "option needs a value");
            const std::wstring value{args.get()[++index]};
            if (argument == L"--save") {
                result.save = value;
                custom_save = true;
            } else if (argument == L"--report") {
                result.report = value;
            } else if (argument == L"--capture") {
                result.capture = value;
            } else if (argument == L"--adapter") {
                std::size_t consumed = 0;
                result.adapter = std::stoi(value, &consumed);
                require(consumed == value.size() && result.adapter >= 0, "invalid adapter index");
            } else {
                throw std::invalid_argument("unknown application option");
            }
        }
    }
    if (result.motion && !custom_save) {
        result.save = directory / L"motion-world.save";
    }
    require(!result.resume_smoke || (result.motion && result.smoke),
            "resume-smoke requires motion smoke mode");
}

class Application final {
  public:
    Application(HINSTANCE instance, const Options& configuration, std::ostream& log)
        : instance_(instance), configuration_(configuration), log_(log) {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.hInstance = instance_;
        window_class.lpfnWndProc = window_proc;
        window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        window_class.lpszClassName = main_class;
        require(RegisterClassExW(&window_class) != 0, "register main window class failed");
        window_class.lpfnWndProc = scene_proc;
        window_class.hbrBackground = nullptr;
        window_class.lpszClassName = scene_class;
        require(RegisterClassExW(&window_class) != 0, "register viewport class failed");
    }

    ~Application() {
        renderer_.reset();
        if (IsWindow(window_)) {
            DestroyWindow(window_);
        }
        if (font_) {
            DeleteObject(font_);
        }
        UnregisterClassW(scene_class, instance_);
        UnregisterClassW(main_class, instance_);
    }
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run() {
        if (configuration_.motion) {
            motion_ = std::make_unique<MotionSession>(session_);
            camera_.zoom(5);
        }
        const auto caption =
            motion_ ? L"Genesis | M03 — ruch i nawigacja" : L"Genesis | M02 — laboratorium";
        window_ =
            CreateWindowExW(WS_EX_CONTROLPARENT, main_class, caption,
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                            1160, motion_ ? 800 : 720, nullptr, nullptr, instance_, this);
        require(window_ != nullptr, "create main window failed");
        viewport_ = CreateWindowExW(0, scene_class, L"Scena 3D", WS_CHILD | WS_VISIBLE, 0, 0, 800,
                                    650, window_, nullptr, instance_, this);
        require(viewport_ != nullptr, "create viewport failed");
        font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH, L"Segoe UI");
        require(font_ != nullptr, "create UI font failed");
        title_ = control(L"STATIC", L"GENESIS / LABORATORIUM M02", 0, 0);
        adapter_ = control(L"STATIC", L"", 0, 0);
        status_ = control(L"STATIC", L"", 0, 0);
        selected_ = control(L"STATIC", L"", 0, 0);
        buttons_[0] = control(L"BUTTON", L"A — niebieski", SelectA, BS_PUSHBUTTON | WS_TABSTOP);
        buttons_[1] = control(L"BUTTON", L"B — bursztynowy", SelectB, BS_PUSHBUTTON | WS_TABSTOP);
        buttons_[2] = control(L"BUTTON", L"Materiał", SelectLot, BS_PUSHBUTTON | WS_TABSTOP);
        buttons_[3] = control(L"BUTTON", L"Rezerwuj [R]", Reserve, BS_PUSHBUTTON | WS_TABSTOP);
        buttons_[4] = control(L"BUTTON", L"Rozpocznij [B]", Begin, BS_PUSHBUTTON | WS_TABSTOP);
        buttons_[5] = control(L"BUTTON", L"Przerwij [C]", Cancel, BS_PUSHBUTTON | WS_TABSTOP);
        buttons_[6] = control(L"BUTTON", L"+1 s [N]", Step, BS_PUSHBUTTON | WS_TABSTOP);
        buttons_[7] = control(L"BUTTON", L"Odtwarzaj [Spacja]", Play, BS_PUSHBUTTON | WS_TABSTOP);
        buttons_[8] = control(L"BUTTON", L"Zapisz [S]", Save, BS_PUSHBUTTON | WS_TABSTOP);
        buttons_[9] = control(L"BUTTON", L"Wczytaj [L]", Load, BS_PUSHBUTTON | WS_TABSTOP);
        if (motion_) {
            SetWindowTextW(title_, L"GENESIS / LABORATORIUM M03");
            SetWindowTextW(buttons_[8], L"Zapisz [F5]");
            SetWindowTextW(buttons_[9], L"Wczytaj [F9]");
            gate_buttons_[0] =
                control(L"BUTTON", L"Główna [G]", MainGate, BS_PUSHBUTTON | WS_TABSTOP);
            gate_buttons_[1] =
                control(L"BUTTON", L"Objazd [H]", DetourGate, BS_PUSHBUTTON | WS_TABSTOP);
            goal_button_ =
                control(L"BUTTON", L"Cel na drugim brzegu [T]", Goal, BS_PUSHBUTTON | WS_TABSTOP);
        }
        feedback_ = control(L"STATIC", L"Gotowe. Wybierz wykonawcę i zarezerwuj materiał.", 0, 0);
        progress_ = control(L"STATIC", L"", 0, 0);
        help_ = control(L"STATIC",
                        L"Kamera: Q/E obrót, ↑/↓ nachylenie, +/- zoom.\nKliknij bryłę, aby ją "
                        L"zaznaczyć.\nStatyczny układ testowy; bez ruchu i fizyki.",
                        0, 0);
        if (motion_) {
            SetWindowTextW(
                help_,
                L"WASD: ruch | PPM: cel | F: osoba obok\nG/H: przejścia | T: cel | Spacja: "
                L"czas\n1/2: postać | Q/E, ↑/↓, +/-: kamera\nF5/F9: zapis | M01: diagnostyka");
            SetWindowTextW(feedback_,
                           L"Fizyka 2,5D. Magazyn i czynności M01 pozostają diagnostyką.");
        }
        inspector_ = control(L"EDIT", L"", 0,
                             WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL |
                                 WS_TABSTOP);
        layout();
        renderer_ = std::make_unique<D3D11Renderer>(viewport_, log_, configuration_.adapter,
                                                    configuration_.gpu_debug);
        SetWindowTextW(
            adapter_,
            (L"GPU: " + renderer_->adapter_name() + L"\nD3D11 | geometria proceduralna").c_str());
        update_panel();
        ShowWindow(window_, SW_SHOWNORMAL);
        UpdateWindow(window_);
        auto previous = std::chrono::steady_clock::now();
        bool quit = false;
        unsigned frames = 0;
        while (!quit) {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    quit = true;
                    break;
                }
                if (motion_ && (message.message == WM_KEYDOWN || message.message == WM_KEYUP) &&
                    movement_key(message.wParam, message.message == WM_KEYDOWN)) {
                    continue;
                }
                if (message.message == WM_KEYDOWN && shortcut(message.wParam, message.lParam)) {
                    continue;
                }
                if (!IsDialogMessageW(window_, &message)) {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            }
            if (fatal_) {
                std::rethrow_exception(fatal_);
            }
            if (quit) {
                break;
            }
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed =
                rebase_time_ ? 0ns
                             : std::chrono::duration_cast<std::chrono::nanoseconds>(now - previous);
            rebase_time_ = false;
            previous = now;
            if (minimized_ || sizing_) {
                MsgWaitForMultipleObjectsEx(0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                previous = std::chrono::steady_clock::now();
                continue;
            }
            try {
                if (motion_) {
                    apply_movement();
                    pacer_.update(*motion_, elapsed);
                } else {
                    pacer_.update(session_, elapsed);
                }
            } catch (const std::exception& error) {
                pacer_.set_running(false);
                feedback(L"Błąd symulacji: " + wide(error.what()));
            }
            update_panel();
            draw_scene();
            if (!renderer_->present()) {
                MsgWaitForMultipleObjectsEx(0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            }
            ++frames;
            if (configuration_.smoke && frames == 2) {
                if (motion_) {
                    motion_smoke_test();
                } else {
                    smoke_test();
                }
            }
        }
        renderer_->check_diagnostics();
        log_ << "normal_shutdown=1\n";
        return 0;
    }

  private:
    HINSTANCE instance_;
    const Options& configuration_;
    std::ostream& log_;
    HWND window_ = nullptr;
    HWND viewport_ = nullptr;
    HWND title_ = nullptr, adapter_ = nullptr, status_ = nullptr, selected_ = nullptr;
    HWND feedback_ = nullptr, help_ = nullptr, inspector_ = nullptr;
    HWND progress_ = nullptr;
    std::array<HWND, 10> buttons_{};
    HFONT font_ = nullptr;
    std::unique_ptr<D3D11Renderer> renderer_;
    LabSession session_;
    std::unique_ptr<MotionSession> motion_;
    std::array<bool, 4> movement_keys_{};
    std::array<bool, 4> pending_keys_{};
    std::array<HWND, 2> gate_buttons_{};
    HWND goal_button_ = nullptr;
    SimulationPacer pacer_;
    LabCamera camera_;
    std::vector<SceneTriangle> scene_;
    int selection_ = 0;
    bool minimized_ = false, sizing_ = false;
    bool rebase_time_ = false;
    std::exception_ptr fatal_;
    std::string inspector_text_;
    std::wstring status_text_;

    HWND control(const wchar_t* type, const wchar_t* text, int id, DWORD style) {
        auto handle =
            CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 20, 20, window_,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
        require(handle != nullptr, "create control failed");
        SendMessageW(handle, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
        return handle;
    }

    void layout() {
        if (!viewport_) {
            return;
        }
        RECT rectangle{};
        require(GetClientRect(window_, &rectangle) != 0, "client rectangle unavailable");
        const int width = std::max(1, static_cast<int>(rectangle.right) - 360);
        const int height = std::max(1, static_cast<int>(rectangle.bottom));
        MoveWindow(viewport_, 0, 0, width, height, TRUE);
        if (renderer_) {
            renderer_->resize(static_cast<unsigned>(width), static_cast<unsigned>(height));
        }
        const int left = width + 14;
        const auto place = [&](HWND handle, int x, int y, int w, int h) {
            if (handle) {
                MoveWindow(handle, x, y, w, h, TRUE);
            }
        };
        place(title_, left, 12, 334, 24);
        place(adapter_, left, 42, 334, 48);
        place(status_, left, 98, 334, 48);
        place(selected_, left, 150, 334, 24);
        place(buttons_[0], left, 180, 158, 30);
        place(buttons_[1], left + 166, 180, 168, 30);
        place(buttons_[2], left, 216, 334, 28);
        place(buttons_[3], left, 252, 106, 32);
        place(buttons_[4], left + 112, 252, 110, 32);
        place(buttons_[5], left + 228, 252, 106, 32);
        place(buttons_[6], left, 292, 106, 32);
        place(buttons_[7], left + 112, 292, 222, 32);
        place(buttons_[8], left, 332, 158, 30);
        place(buttons_[9], left + 166, 332, 168, 30);
        place(progress_, left, 370, 334, 24);
        place(feedback_, left, 396, 334, 48);
        place(help_, left, 450, 334, 68);
        place(inspector_, left, 524, 334, std::max(60, height - 538));
        if (motion_) {
            place(gate_buttons_[0], left, 450, 158, 28);
            place(gate_buttons_[1], left + 166, 450, 168, 28);
            place(goal_button_, left, 482, 334, 28);
            place(help_, left, 516, 334, 88);
            place(inspector_, left, 610, 334, std::max(35, height - 622));
        }
    }

    void feedback(const std::wstring& message) {
        SetWindowTextW(feedback_, message.c_str());
        log_ << "feedback=" << utf8(message) << '\n';
        log_.flush();
    }

    void update_panel() {
        const auto tick = session_.world().clock().tick_count();
        std::wostringstream status;
        status << (pacer_.running() ? L"ODTWARZANIE" : L"PAUZA") << L" | krok: " << tick
               << L"\nCzas świata: " << session_.world().clock().elapsed().count() / 1000000
               << L" ms | minimalizacja = pauza";
        if (status.str() != status_text_) {
            status_text_ = status.str();
            SetWindowTextW(status_, status_text_.c_str());
            SetWindowTextW(buttons_[7],
                           pacer_.running() ? L"Pauza [Spacja]" : L"Odtwarzaj [Spacja]");
        }
        const auto id = selection_ == 2 ? session_.lot()
                                        : session_.actors()[static_cast<std::size_t>(selection_)];
        const auto label = (selection_ == 2   ? L"Materiał"
                            : selection_ == 0 ? L"Wykonawca A"
                                              : L"Wykonawca B") +
                           std::wstring{L" | ID: "} + std::to_wstring(id.value);
        SetWindowTextW(selected_, label.c_str());
        std::wstring progress = L"Brak czynności wybranego wykonawcy.";
        if (selection_ == 2) {
            const auto stock = session_.world().resource(session_.lot()).value();
            progress = L"Ilość: " + std::to_wstring(stock.record.quantity.value) +
                       L" | rezerwacja: " + std::to_wstring(stock.reserved.value) + L" | wolne: " +
                       std::to_wstring(stock.available.value);
        } else if (const auto action =
                       session_.latest_action(static_cast<std::size_t>(selection_))) {
            const auto elapsed = action->state == ActionState::Running ? tick - action->started_at
                                 : action->state == ActionState::Completed
                                     ? action->request.duration_ticks
                                 : action->due_at != 0 ? action->ended_at - action->started_at
                                                       : 0;
            const auto percent =
                static_cast<unsigned>(100.0L * static_cast<long double>(elapsed) /
                                      static_cast<long double>(action->request.duration_ticks));
            progress = wide(describe(action->state)) + L" | " + std::to_wstring(percent) +
                       L"% | zużyto " + std::to_wstring(action->consumed.value) + L"/" +
                       std::to_wstring(action->request.required.value);
        }
        SetWindowTextW(progress_, progress.c_str());
        for (int index = 3; index <= 5; ++index) {
            EnableWindow(buttons_[static_cast<std::size_t>(index)], selection_ < 2);
        }
        EnableWindow(buttons_[6], !pacer_.running());
        if (motion_) {
            SetWindowTextW(gate_buttons_[0],
                           motion_->gate_closed(0) ? L"Główna: ZAMK. [G]" : L"Główna: OTW. [G]");
            SetWindowTextW(gate_buttons_[1],
                           motion_->gate_closed(1) ? L"Objazd: ZAMK. [H]" : L"Objazd: OTW. [H]");
            if (selection_ < 2) {
                const auto& actor = motion_->actors()[static_cast<std::size_t>(selection_)];
                SetWindowTextW(progress_, (std::wstring{travel_label(actor.state)} + L" | trasa " +
                                           std::to_wstring(actor.next_point) + L"/" +
                                           std::to_wstring(actor.route.points.size()))
                                              .c_str());
            }
        }
        const auto text =
            (motion_ ? motion_->inspect() : std::string{}) + session_.world().inspect();
        if (text != inspector_text_) {
            inspector_text_ = text;
            std::string display;
            for (const char character : text) {
                if (character == '\n') {
                    display += '\r';
                }
                display += character;
            }
            SetWindowTextW(inspector_, wide(display).c_str());
        }
    }

    void draw_scene() {
        const auto resource = session_.world().resource(session_.lot()).value();
        LabSceneView view;
        view.selected = selection_;
        view.quantity = resource.record.quantity.value;
        view.reserved = resource.reserved.value;
        for (std::size_t actor = 0; actor < 2; ++actor) {
            const auto action = session_.latest_action(actor);
            view.running[actor] = action && action->state == ActionState::Running;
        }
        const auto aspect =
            static_cast<float>(renderer_->width()) / static_cast<float>(renderer_->height());
        const auto frame_fraction =
            motion_ && pacer_.running()
                ? std::clamp(static_cast<double>(pacer_.pending().count()) /
                                 static_cast<double>(session_.world().clock().step().count()),
                             0.0, 1.0)
                : 1.0;
        scene_ = motion_ ? build_motion_scene(*motion_, selection_, camera_, aspect, frame_fraction)
                         : build_lab_scene(view, camera_, aspect);
        renderer_->draw(scene_);
    }

    void scene_click(int x, int y) {
        if (!renderer_) {
            return;
        }
        const auto picked = pick_scene(
            scene_, 2.0F * static_cast<float>(x) / static_cast<float>(renderer_->width()) - 1.0F,
            1.0F - 2.0F * static_cast<float>(y) / static_cast<float>(renderer_->height()));
        if (picked) {
            clear_movement();
            selection_ = *picked;
            log_ << "scene_selection=" << selection_ << '\n';
            update_panel();
        }
        SetFocus(window_);
    }

    bool movement_key(WPARAM key, bool down) {
        std::size_t index = 0;
        switch (key) {
        case 'W':
            index = 0;
            break;
        case 'A':
            index = 1;
            break;
        case 'S':
            index = 2;
            break;
        case 'D':
            index = 3;
            break;
        default:
            return false;
        }
        if (down && !movement_keys_[index]) {
            pending_keys_[index] = true;
        }
        movement_keys_[index] = down;
        return true;
    }
    void clear_movement() noexcept {
        movement_keys_ = {};
        pending_keys_ = {};
        if (motion_) {
            motion_->clear_input();
        }
    }
    void apply_movement() {
        if (selection_ < 2) {
            motion_->set_input(
                static_cast<std::size_t>(selection_),
                camera_.ground_direction(
                    static_cast<float>(movement_keys_[3]) - static_cast<float>(movement_keys_[1]),
                    static_cast<float>(movement_keys_[0]) - static_cast<float>(movement_keys_[2])));
            if (std::any_of(pending_keys_.begin(), pending_keys_.end(),
                            [](bool key) { return key; })) {
                motion_->queue_input(
                    static_cast<std::size_t>(selection_),
                    camera_.ground_direction(static_cast<float>(pending_keys_[3]) -
                                                 static_cast<float>(pending_keys_[1]),
                                             static_cast<float>(pending_keys_[0]) -
                                                 static_cast<float>(pending_keys_[2])));
                pending_keys_ = {};
            }
        }
    }
    void scene_goal(int x, int y) {
        if (!motion_ || !renderer_ || selection_ >= 2) {
            return;
        }
        try {
            const auto point = camera_.ground_point(
                2.0F * static_cast<float>(x) / static_cast<float>(renderer_->width()) - 1.0F,
                1.0F - 2.0F * static_cast<float>(y) / static_cast<float>(renderer_->height()),
                static_cast<float>(renderer_->width()) / static_cast<float>(renderer_->height()));
            clear_movement();
            motion_->set_goal(static_cast<std::size_t>(selection_), point);
            feedback(L"Cel z podłoża: " +
                     std::wstring{travel_label(
                         motion_->actors()[static_cast<std::size_t>(selection_)].state)} +
                     L".");
        } catch (const std::exception& error) {
            feedback(L"Błąd celu: " + wide(error.what()));
        }
        SetFocus(window_);
        update_panel();
    }

    void motion_smoke_test() {
        const auto send = [&](int id) {
            SendMessageW(window_, WM_COMMAND, static_cast<WPARAM>(id), 0);
        };
        draw_scene();
        auto before = configuration_.capture;
        before += L".before.png";
        require(renderer_->capture(before) > 1000, "motion initial frame is empty");
        if (configuration_.resume_smoke) {
            motion_->load(configuration_.save);
            motion_->advance(300);
            motion_->save(configuration_.save);
        } else {
            const auto aspect =
                static_cast<float>(renderer_->width()) / static_cast<float>(renderer_->height());
            const auto point = camera_.project({-7, .6F, -4}, aspect);
            const auto x =
                static_cast<int>((point.x + 1) * .5F * static_cast<float>(renderer_->width()));
            const auto y =
                static_cast<int>((1 - point.y) * .5F * static_cast<float>(renderer_->height()));
            SendMessageW(viewport_, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
            require(selection_ == 1, "motion picking failed");
            send(SelectA);
            const auto position = motion_->actors()[0].position;
            SendMessageW(window_, WM_KEYDOWN, 'D', 0);
            SendMessageW(window_, WM_KEYUP, 'D', 0);
            send(Step);
            require(std::abs(distance(position, motion_->actors()[0].position) - .15) < 1e-9,
                    "short native key pulse was lost or repeated");
            const auto held_start = motion_->actors()[0].position;
            SendMessageW(window_, WM_KEYDOWN, 'D', 0);
            send(Step);
            SendMessageW(window_, WM_KEYUP, 'D', 0);
            require(distance(held_start, motion_->actors()[0].position) > 2.9,
                    "native directional key did not move actor");
            SendMessageW(window_, WM_KEYDOWN, 'W', 0);
            SendMessageW(window_, WM_ACTIVATEAPP, FALSE, 0);
            const auto stopped = motion_->actors()[0].position;
            send(Step);
            require(stopped == motion_->actors()[0].position, "focus loss left movement held");
            send(Reserve);
            send(Begin);
            send(Goal);
            motion_->advance(10);
            const auto prior = motion_->presentation_positions(0)[0];
            const auto halfway = motion_->presentation_positions(.5)[0];
            const auto current = motion_->actors()[0].position;
            require(distance(prior, halfway) > 0 && distance(halfway, current) > 0,
                    "moving actor has no intermediate render pose");
            const auto aspect_now =
                static_cast<float>(renderer_->width()) / static_cast<float>(renderer_->height());
            renderer_->draw(build_motion_scene(*motion_, selection_, camera_, aspect_now, .5));
            auto interpolated = configuration_.capture;
            interpolated += L".interpolated.png";
            require(renderer_->capture(interpolated) > 1000, "interpolated GPU scene is empty");
            log_ << "interpolation_sample previous_x=" << prior.x_m << " halfway_x=" << halfway.x_m
                 << " current_x=" << current.x_m << '\n';
            send(MainGate);
            send(Save);
            const auto snapshot = motion_->serialize();
            motion_->advance(1);
            send(Load);
            require(motion_->serialize() == snapshot,
                    "native motion load did not restore stale route");
            motion_->advance(300);
        }
        require(motion_->actors()[0].state == TravelState::Arrived &&
                    session_.latest_action(0)->state == ActionState::Completed,
                "motion continuation failed");
        log_ << "continuation_checksum=" << snapshot_checksum(motion_->serialize()) << '\n';
        const auto tick = session_.world().clock().tick_count();
        require(SetWindowPos(window_, nullptr, 0, 0, 1000, 760, SWP_NOMOVE | SWP_NOZORDER) != 0,
                "motion resize failed");
        ShowWindow(window_, SW_MINIMIZE);
        require(minimized_, "motion minimize failed");
        ShowWindow(window_, SW_RESTORE);
        require(!minimized_ && session_.world().clock().tick_count() == tick,
                "motion restore changed time");
        draw_scene();
        require(renderer_->capture(configuration_.capture) > 1000, "motion final frame is empty");
        static_cast<void>(renderer_->present());
        renderer_->check_diagnostics();
        log_ << motion_->inspect() << "MOTION SMOKE PASS\n";
        log_.flush();
        PostMessageW(window_, WM_CLOSE, 0, 0);
    }

    void command(int id) {
        try {
            if (id == SelectA || id == SelectB || id == SelectLot) {
                clear_movement();
                selection_ = id - SelectA;
            } else if (id == Play) {
                if (motion_) {
                    motion_->synchronize_presentation();
                }
                pacer_.set_running(!pacer_.running());
                rebase_time_ = true;
                feedback(L"Zmieniono odtwarzanie czasu świata.");
            } else if (id == Step) {
                if (pacer_.running()) {
                    feedback(L"Najpierw zatrzymaj odtwarzanie.");
                    return;
                }
                if (motion_) {
                    apply_movement();
                    motion_->advance(20);
                } else {
                    session_.advance(20);
                }
                feedback(L"Wykonano 20 kroków: +1 sekunda świata.");
            } else if (id == Save) {
                if (motion_) {
                    motion_->save(configuration_.save);
                } else {
                    session_.save(configuration_.save);
                }
                feedback(L"Zapisano świat: " + configuration_.save.filename().wstring());
            } else if (id == Load) {
                if (motion_) {
                    motion_->load(configuration_.save);
                } else {
                    session_.load(configuration_.save);
                }
                clear_movement();
                pacer_.reset();
                selection_ = 0;
                rebase_time_ = true;
                feedback(L"Wczytano świat. Symulacja zatrzymana.");
            } else if (motion_ && (id == MainGate || id == DetourGate)) {
                const std::size_t gate = id == MainGate ? 0 : 1;
                const auto changed = motion_->set_gate(gate, !motion_->gate_closed(gate));
                feedback(changed ? L"Zmieniono geometrię; trasy wymagają aktualizacji."
                                 : L"Odmowa: postać zajmuje przejście.");
            } else if (motion_ && id == InspectNearby && selection_ < 2) {
                const auto target =
                    motion_->interaction_target(static_cast<std::size_t>(selection_));
                feedback(target ? L"Kontakt lokalny z osobą ID " + std::to_wstring(target->value) +
                                      L". Podgląd testowy, bez dialogu."
                                : L"Brak osoby w zasięgu 1,5 m z wolnym dojściem.");
            } else if (motion_ && id == Goal && selection_ < 2) {
                clear_movement();
                const auto index = static_cast<std::size_t>(selection_);
                motion_->set_goal(index,
                                  {motion_->actors()[index].position.x_m < 0 ? 7.0 : -7.0, 0});
                feedback(L"Cel: " + std::wstring{travel_label(motion_->actors()[index].state)} +
                         L".");
            } else if (selection_ < 2) {
                const auto actor = static_cast<std::size_t>(selection_);
                if (id == Reserve) {
                    feedback(L"Rezerwacja: " + wide(describe(session_.reserve(actor).status)));
                } else if (id == Begin || id == Cancel) {
                    const auto result =
                        id == Begin ? session_.begin(actor) : session_.cancel(actor);
                    switch (result) {
                    case Transition::Changed:
                        feedback(L"Wykonano zmianę czynności.");
                        break;
                    case Transition::Unchanged:
                        feedback(L"Czynność ma już ten stan; bez nowego skutku.");
                        break;
                    case Transition::Missing:
                        feedback(L"Brak czynności wybranego wykonawcy.");
                        break;
                    case Transition::InvalidState:
                        feedback(L"Ta faza czynności nie pozwala na operację.");
                        break;
                    case Transition::TimeOverflow:
                        feedback(L"Termin wykracza poza zakres zegara.");
                        break;
                    }
                }
            }
            update_panel();
        } catch (const std::exception& error) {
            pacer_.set_running(false);
            feedback(L"Błąd: " + wide(error.what()));
            update_panel();
        }
    }

    bool shortcut(WPARAM key, LPARAM state) {
        const bool repeated = (state & (static_cast<LPARAM>(1) << 30)) != 0;
        int action = 0;
        if (motion_) {
            if (key == VK_F5) {
                if (!repeated) {
                    command(Save);
                }
                return true;
            }
            if (key == VK_F9) {
                if (!repeated) {
                    command(Load);
                }
                return true;
            }
            if (key == 'G' || key == 'H' || key == 'T' || key == 'F') {
                if (!repeated) {
                    command(key == 'G'   ? MainGate
                            : key == 'H' ? DetourGate
                            : key == 'F' ? InspectNearby
                                         : Goal);
                }
                return true;
            }
        }
        switch (key) {
        case 'R':
            action = Reserve;
            break;
        case 'B':
            action = Begin;
            break;
        case 'C':
            action = Cancel;
            break;
        case 'S':
            action = Save;
            break;
        case 'L':
            action = Load;
            break;
        case 'N':
            action = Step;
            break;
        case VK_SPACE:
            action = Play;
            break;
        case '1':
            action = SelectA;
            break;
        case '2':
            action = SelectB;
            break;
        case '3':
            action = SelectLot;
            break;
        case 'Q':
            camera_.orbit(-0.1F);
            return true;
        case 'E':
            camera_.orbit(0.1F);
            return true;
        case VK_UP:
            camera_.tilt(0.08F);
            return true;
        case VK_DOWN:
            camera_.tilt(-0.08F);
            return true;
        case VK_OEM_PLUS:
        case VK_ADD:
            camera_.zoom(-0.5F);
            return true;
        case VK_OEM_MINUS:
        case VK_SUBTRACT:
            camera_.zoom(0.5F);
            return true;
        default:
            return false;
        }
        if (!repeated) {
            command(action);
        }
        return true;
    }

    void smoke_test() {
        const auto send = [&](int id) {
            SendMessageW(window_, WM_COMMAND, static_cast<WPARAM>(id), 0);
        };
        const auto point = camera_.project({3, 1, 0}, static_cast<float>(renderer_->width()) /
                                                          static_cast<float>(renderer_->height()));
        const auto x =
            static_cast<int>((point.x + 1) * 0.5F * static_cast<float>(renderer_->width()));
        const auto y =
            static_cast<int>((1 - point.y) * 0.5F * static_cast<float>(renderer_->height()));
        SendMessageW(viewport_, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
        require(selection_ == 1, "scene picking did not select actor B");
        draw_scene();
        auto before = configuration_.capture;
        before += L".before.png";
        require(renderer_->capture(before) > 1000, "GPU frame contains no visible geometry");
        send(SelectA);
        send(Reserve);
        send(SelectB);
        send(Reserve);
        require(session_.world().commands().size() == 2 &&
                    session_.world().commands().back().result.status ==
                        RequestStatus::InsufficientResource,
                "UI contention scenario failed");
        send(SelectA);
        send(Begin);
        send(Step);
        send(Step);
        require(session_.world().clock().tick_count() == 40, "manual simulation time mismatch");
        const auto running = serialize_world(session_.world());
        send(Save);
        send(Step);
        send(Load);
        require(serialize_world(session_.world()) == running,
                "UI save/load failed to restore running work");
        send(Step);
        send(Step);
        send(Step);
        require(session_.world().resource(session_.lot())->record.quantity.value == 0 &&
                    session_.latest_action(0)->state == ActionState::Completed,
                "UI completion balance failed");
        const auto completed = serialize_world(session_.world());
        send(Begin);
        require(serialize_world(session_.world()) == completed,
                "UI repeated begin changed completion");
        send(Save);
        const auto old_width = renderer_->width();
        require(SetWindowPos(window_, nullptr, 0, 0, 1000, 650, SWP_NOMOVE | SWP_NOZORDER) != 0,
                "resize test failed");
        require(renderer_->width() != old_width, "viewport failed to resize");
        const auto tick = session_.world().clock().tick_count();
        ShowWindow(window_, SW_MINIMIZE);
        require(IsIconic(window_) && minimized_, "window did not minimize");
        ShowWindow(window_, SW_RESTORE);
        require(!IsIconic(window_) && !minimized_ && session_.world().clock().tick_count() == tick,
                "restore changed simulation");
        draw_scene();
        require(renderer_->capture(configuration_.capture) > 1000, "resized GPU frame is empty");
        static_cast<void>(renderer_->present());
        renderer_->check_diagnostics();
        log_ << "FINAL " << session_.world().inspect() << "SMOKE PASS\n";
        log_.flush();
        PostMessageW(window_, WM_CLOSE, 0, 0);
    }

    static Application* binding(HWND window, UINT message, LPARAM parameter) {
        if (message == WM_NCCREATE) {
            auto* self = static_cast<Application*>(
                reinterpret_cast<CREATESTRUCTW*>(parameter)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            return self;
        }
        return reinterpret_cast<Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM word,
                                        LPARAM parameter) noexcept {
        auto* self = binding(window, message, parameter);
        if (!self) {
            return DefWindowProcW(window, message, word, parameter);
        }
        try {
            switch (message) {
            case WM_KEYDOWN:
            case WM_KEYUP:
                if (self->motion_ && self->movement_key(word, message == WM_KEYDOWN)) {
                    return 0;
                }
                break;
            case WM_ACTIVATEAPP:
                if (!word) {
                    self->clear_movement();
                }
                return 0;
            case WM_COMMAND:
                self->command(LOWORD(word));
                return 0;
            case WM_SIZE:
                if (self->motion_) {
                    self->motion_->synchronize_presentation();
                }
                self->rebase_time_ = true;
                if (word == SIZE_MINIMIZED) {
                    self->minimized_ = true;
                    self->log_ << "minimized=1 tick=" << self->session_.world().clock().tick_count()
                               << '\n';
                    self->log_.flush();
                } else {
                    if (self->minimized_) {
                        self->log_
                            << "restored=1 tick=" << self->session_.world().clock().tick_count()
                            << '\n';
                        self->log_.flush();
                    }
                    self->minimized_ = false;
                    self->layout();
                }
                return 0;
            case WM_ENTERSIZEMOVE:
                self->sizing_ = true;
                return 0;
            case WM_EXITSIZEMOVE:
                if (self->motion_) {
                    self->motion_->synchronize_presentation();
                }
                self->sizing_ = false;
                self->rebase_time_ = true;
                return 0;
            case WM_GETMINMAXINFO:
                reinterpret_cast<MINMAXINFO*>(parameter)->ptMinTrackSize = {
                    1000, self->configuration_.motion ? 760 : 650};
                return 0;
            case WM_CLOSE:
                DestroyWindow(window);
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProcW(window, message, word, parameter);
            }
            return DefWindowProcW(window, message, word, parameter);
        } catch (...) {
            self->fatal_ = std::current_exception();
            return 0;
        }
    }

    static LRESULT CALLBACK scene_proc(HWND window, UINT message, WPARAM word,
                                       LPARAM parameter) noexcept {
        auto* self = binding(window, message, parameter);
        if (!self) {
            return DefWindowProcW(window, message, word, parameter);
        }
        try {
            if (message == WM_LBUTTONDOWN) {
                self->scene_click(GET_X_LPARAM(parameter), GET_Y_LPARAM(parameter));
                return 0;
            }
            if (message == WM_RBUTTONDOWN) {
                self->scene_goal(GET_X_LPARAM(parameter), GET_Y_LPARAM(parameter));
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
            return DefWindowProcW(window, message, word, parameter);
        } catch (...) {
            self->fatal_ = std::current_exception();
            return 0;
        }
    }
};

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    Options configuration;
    std::ofstream log;
    bool apartment = false;
    try {
        options(configuration);
        log.open(configuration.report, std::ios::binary | std::ios::trunc);
        require(log.is_open(), "cannot open application report");
        log.exceptions(std::ios::badbit | std::ios::failbit);
        require(SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)),
                "COM initialization failed");
        apartment = true;
        require(SetProcessDPIAware() != 0, "DPI awareness initialization failed");
        int result = 0;
        {
            Application app{instance, configuration, log};
            result = app.run();
        }
        CoUninitialize();
        return result;
    } catch (const std::exception& error) {
        log.exceptions(std::ios::goodbit);
        if (!log.is_open() && !configuration.report.empty()) {
            log.clear();
            log.open(configuration.report, std::ios::binary | std::ios::trunc);
        }
        if (log.is_open() && log.good()) {
            log << "ERROR: " << utf8(wide(error.what())) << '\n';
            log.flush();
        }
        if (!configuration.smoke) {
            MessageBoxW(nullptr, wide(error.what()).c_str(), L"Genesis — błąd",
                        MB_OK | MB_ICONERROR);
        }
        if (apartment) {
            CoUninitialize();
        }
        return 1;
    }
}
