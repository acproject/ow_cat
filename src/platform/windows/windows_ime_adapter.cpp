#ifdef _WIN32

#include "platform/windows/windows_ime_adapter.h"
#include <spdlog/spdlog.h>
#include <codecvt>
#include <locale>
#include <sstream>
#include <algorithm>
#include <shellapi.h>
#include <versionhelpers.h>

namespace owcat {
namespace platform {
namespace windows {

// 全局变量用于消息钩子
static WindowsImeAdapter* g_ime_adapter = nullptr;
static WindowsImeMessageHandler* g_message_handler = nullptr;

// 字符串转换工具
std::wstring utf8ToWstring(const std::string& utf8_str) {
    if (utf8_str.empty()) return L"";
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], (int)utf8_str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], (int)utf8_str.size(), &wstr[0], size_needed);
    return wstr;
}

std::string wstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

// WindowsImeAdapter::Impl 实现
class WindowsImeAdapter::Impl {
public:
    Impl() : enabled_(false), active_(false), composing_(false),
             ime_window_(nullptr), input_context_(nullptr),
             message_handler_(nullptr), candidate_window_(nullptr) {
    }
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize() {
        spdlog::info("Initializing Windows IME adapter");
        
        // 创建消息处理器
        message_handler_ = std::make_unique<WindowsImeMessageHandler>(static_cast<WindowsImeAdapter*>(this));
        if (!message_handler_->installHook()) {
            spdlog::error("Failed to install IME message hook");
            return false;
        }
        
        // 创建候选窗口
        candidate_window_ = std::make_unique<WindowsCandidateWindow>();
        if (!candidate_window_->create()) {
            spdlog::error("Failed to create candidate window");
            return false;
        }
        
        // 设置全局指针
        g_ime_adapter = static_cast<WindowsImeAdapter*>(this);
        g_message_handler = message_handler_.get();
        
        spdlog::info("Windows IME adapter initialized successfully");
        return true;
    }
    
    void shutdown() {
        if (message_handler_) {
            message_handler_->uninstallHook();
            message_handler_.reset();
        }
        
        if (candidate_window_) {
            candidate_window_->destroy();
            candidate_window_.reset();
        }
        
        if (input_context_) {
            ImmReleaseContext(ime_window_, input_context_);
            input_context_ = nullptr;
        }
        
        g_ime_adapter = nullptr;
        g_message_handler = nullptr;
    }
    
    bool registerInputMethod(const std::string& service_name, const std::string& display_name) {
        service_name_ = service_name;
        display_name_ = display_name;
        
        // 在Windows中，我们不需要显式注册IME，而是通过消息钩子工作
        spdlog::info("Registered input method: {} ({})", display_name, service_name);
        return true;
    }
    
    void unregisterInputMethod() {
        service_name_.clear();
        display_name_.clear();
        spdlog::info("Unregistered input method");
    }
    
    void setInputMethodEnabled(bool enabled) {
        enabled_ = enabled;
        spdlog::debug("Input method enabled: {}", enabled);
        
        if (state_change_callback_) {
            PlatformInputState state = enabled ? PlatformInputState::ENABLED : PlatformInputState::DISABLED;
            state_change_callback_(state);
        }
    }
    
    bool isInputMethodEnabled() const {
        return enabled_;
    }
    
    void setInputMethodActive(bool active) {
        active_ = active;
        spdlog::debug("Input method active: {}", active);
        
        if (state_change_callback_) {
            PlatformInputState state;
            if (!enabled_) {
                state = PlatformInputState::DISABLED;
            } else if (composing_) {
                state = PlatformInputState::COMPOSING;
            } else if (active) {
                state = PlatformInputState::ACTIVE;
            } else {
                state = PlatformInputState::ENABLED;
            }
            state_change_callback_(state);
        }
    }
    
    bool isInputMethodActive() const {
        return active_;
    }
    
    void startComposition() {
        composing_ = true;
        spdlog::debug("Started composition");
        
        // 获取当前活动窗口的输入上下文
        HWND active_window = GetForegroundWindow();
        if (active_window) {
            ime_window_ = active_window;
            input_context_ = ImmGetContext(active_window);
        }
        
        if (state_change_callback_) {
            state_change_callback_(PlatformInputState::COMPOSING);
        }
    }
    
    void updateComposition(const std::string& composition_text, int cursor_pos) {
        composition_text_ = composition_text;
        cursor_position_ = cursor_pos;
        
        spdlog::debug("Updated composition: '{}' at position {}", composition_text, cursor_pos);
        
        // 更新IME组合字符串
        if (input_context_) {
            std::wstring wtext = utf8ToWstring(composition_text);
            
            // 设置组合字符串
            ImmSetCompositionStringW(input_context_, GCS_COMPSTR, 
                                   wtext.c_str(), wtext.length() * sizeof(wchar_t),
                                   nullptr, 0);
            
            // 设置光标位置
            COMPOSITIONFORM comp_form;
            comp_form.dwStyle = CFS_POINT;
            comp_form.ptCurrentPos.x = cursor_pos * 10; // 简化的位置计算
            comp_form.ptCurrentPos.y = 0;
            ImmSetCompositionWindow(input_context_, &comp_form);
        }
    }
    
    void endComposition() {
        composing_ = false;
        composition_text_.clear();
        cursor_position_ = 0;
        
        spdlog::debug("Ended composition");
        
        if (input_context_) {
            // 结束组合
            ImmNotifyIME(input_context_, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
        }
        
        if (state_change_callback_) {
            PlatformInputState state = active_ ? PlatformInputState::ACTIVE : 
                                      (enabled_ ? PlatformInputState::ENABLED : PlatformInputState::DISABLED);
            state_change_callback_(state);
        }
    }
    
    void commitText(const std::string& text) {
        spdlog::debug("Committing text: '{}'", text);
        
        if (input_context_) {
            std::wstring wtext = utf8ToWstring(text);
            
            // 提交文本
            ImmSetCompositionStringW(input_context_, GCS_RESULTSTR,
                                   wtext.c_str(), wtext.length() * sizeof(wchar_t),
                                   nullptr, 0);
        } else {
            // 如果没有输入上下文，使用SendInput模拟键盘输入
            std::wstring wtext = utf8ToWstring(text);
            for (wchar_t ch : wtext) {
                INPUT input = {0};
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = 0;
                input.ki.wScan = ch;
                input.ki.dwFlags = KEYEVENTF_UNICODE;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }
    
    void showCandidateWindow(const core::CandidateList& candidates,
                           const CandidateWindowPosition& position) {
        if (candidate_window_) {
            candidate_window_->show(candidates, position);
        }
    }
    
    void hideCandidateWindow() {
        if (candidate_window_) {
            candidate_window_->hide();
        }
    }
    
    void updateCandidateSelection(int selected_index) {
        if (candidate_window_) {
            candidate_window_->updateSelection(selected_index);
        }
    }
    
    bool getCursorPosition(int& x, int& y) {
        POINT cursor_pos;
        if (GetCursorPos(&cursor_pos)) {
            x = cursor_pos.x;
            y = cursor_pos.y;
            return true;
        }
        return false;
    }
    
    std::string getCurrentApplication() {
        HWND active_window = GetForegroundWindow();
        if (!active_window) {
            return "Unknown";
        }
        
        // 获取窗口标题
        wchar_t window_title[256];
        GetWindowTextW(active_window, window_title, sizeof(window_title) / sizeof(wchar_t));
        
        // 获取进程名
        DWORD process_id;
        GetWindowThreadProcessId(active_window, &process_id);
        
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
        if (process) {
            wchar_t process_name[MAX_PATH];
            if (GetModuleBaseNameW(process, nullptr, process_name, MAX_PATH)) {
                CloseHandle(process);
                return wstringToUtf8(process_name) + " - " + wstringToUtf8(window_title);
            }
            CloseHandle(process);
        }
        
        return wstringToUtf8(window_title);
    }
    
    void setKeyEventCallback(PlatformKeyEventCallback callback) {
        key_event_callback_ = callback;
    }
    
    void setStateChangeCallback(PlatformStateChangeCallback callback) {
        state_change_callback_ = callback;
    }
    
    void setFocusChangeCallback(PlatformFocusChangeCallback callback) {
        focus_change_callback_ = callback;
    }
    
    std::string getPlatformName() const {
        return "Windows";
    }
    
    std::string getPlatformVersion() const {
        return WindowsSystemIntegration::getWindowsVersion();
    }
    
    bool isFeatureSupported(const std::string& feature) const {
        // Windows支持的功能列表
        static const std::vector<std::string> supported_features = {
            "composition",
            "candidate_window",
            "cursor_tracking",
            "application_detection",
            "keyboard_hook",
            "mouse_hook",
            "ime_integration"
        };
        
        return std::find(supported_features.begin(), supported_features.end(), feature) != supported_features.end();
    }
    
    std::map<std::string, std::string> getPlatformConfig() const {
        return platform_config_;
    }
    
    void setPlatformConfig(const std::map<std::string, std::string>& config) {
        platform_config_ = config;
        
        // 应用配置到候选窗口
        if (candidate_window_) {
            candidate_window_->setWindowStyle(config);
        }
    }
    
    LRESULT handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (message_handler_) {
            return message_handler_->processImeMessage(hwnd, msg, wParam, lParam);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    HWND getImeWindow() const {
        return ime_window_;
    }
    
    void setImeWindowPosition(int x, int y) {
        if (ime_window_) {
            SetWindowPos(ime_window_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    
    HIMC getCurrentInputContext() const {
        return input_context_;
    }
    
    // 处理键盘事件的内部方法
    bool handleKeyEvent(const PlatformKeyEvent& event) {
        if (key_event_callback_) {
            return key_event_callback_(event);
        }
        return false;
    }
    
public:
    bool enabled_;
    bool active_;
    bool composing_;
    std::string service_name_;
    std::string display_name_;
    std::string composition_text_;
    int cursor_position_;
    
    HWND ime_window_;
    HIMC input_context_;
    
    std::unique_ptr<WindowsImeMessageHandler> message_handler_;
    std::unique_ptr<WindowsCandidateWindow> candidate_window_;
    
    PlatformKeyEventCallback key_event_callback_;
    PlatformStateChangeCallback state_change_callback_;
    PlatformFocusChangeCallback focus_change_callback_;
    
    std::map<std::string, std::string> platform_config_;
};

// WindowsImeAdapter 实现
WindowsImeAdapter::WindowsImeAdapter()
    : pImpl(std::make_unique<Impl>()) {
}

WindowsImeAdapter::~WindowsImeAdapter() = default;

bool WindowsImeAdapter::initialize() {
    return pImpl->initialize();
}

void WindowsImeAdapter::shutdown() {
    pImpl->shutdown();
}

bool WindowsImeAdapter::registerInputMethod(const std::string& service_name, const std::string& display_name) {
    return pImpl->registerInputMethod(service_name, display_name);
}

void WindowsImeAdapter::unregisterInputMethod() {
    pImpl->unregisterInputMethod();
}

void WindowsImeAdapter::setInputMethodEnabled(bool enabled) {
    pImpl->setInputMethodEnabled(enabled);
}

bool WindowsImeAdapter::isInputMethodEnabled() const {
    return pImpl->isInputMethodEnabled();
}

void WindowsImeAdapter::setInputMethodActive(bool active) {
    pImpl->setInputMethodActive(active);
}

bool WindowsImeAdapter::isInputMethodActive() const {
    return pImpl->isInputMethodActive();
}

void WindowsImeAdapter::startComposition() {
    pImpl->startComposition();
}

void WindowsImeAdapter::updateComposition(const std::string& composition_text, int cursor_pos) {
    pImpl->updateComposition(composition_text, cursor_pos);
}

void WindowsImeAdapter::endComposition() {
    pImpl->endComposition();
}

void WindowsImeAdapter::commitText(const std::string& text) {
    pImpl->commitText(text);
}

void WindowsImeAdapter::showCandidateWindow(const core::CandidateList& candidates,
                                          const CandidateWindowPosition& position) {
    pImpl->showCandidateWindow(candidates, position);
}

void WindowsImeAdapter::hideCandidateWindow() {
    pImpl->hideCandidateWindow();
}

void WindowsImeAdapter::updateCandidateSelection(int selected_index) {
    pImpl->updateCandidateSelection(selected_index);
}

bool WindowsImeAdapter::getCursorPosition(int& x, int& y) {
    return pImpl->getCursorPosition(x, y);
}

std::string WindowsImeAdapter::getCurrentApplication() {
    return pImpl->getCurrentApplication();
}

void WindowsImeAdapter::setKeyEventCallback(PlatformKeyEventCallback callback) {
    pImpl->setKeyEventCallback(callback);
}

void WindowsImeAdapter::setStateChangeCallback(PlatformStateChangeCallback callback) {
    pImpl->setStateChangeCallback(callback);
}

void WindowsImeAdapter::setFocusChangeCallback(PlatformFocusChangeCallback callback) {
    pImpl->setFocusChangeCallback(callback);
}

std::string WindowsImeAdapter::getPlatformName() const {
    return pImpl->getPlatformName();
}

std::string WindowsImeAdapter::getPlatformVersion() const {
    return pImpl->getPlatformVersion();
}

bool WindowsImeAdapter::isFeatureSupported(const std::string& feature) const {
    return pImpl->isFeatureSupported(feature);
}

std::map<std::string, std::string> WindowsImeAdapter::getPlatformConfig() const {
    return pImpl->getPlatformConfig();
}

void WindowsImeAdapter::setPlatformConfig(const std::map<std::string, std::string>& config) {
    pImpl->setPlatformConfig(config);
}

LRESULT WindowsImeAdapter::handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return pImpl->handleWindowMessage(hwnd, msg, wParam, lParam);
}

HWND WindowsImeAdapter::getImeWindow() const {
    return pImpl->getImeWindow();
}

void WindowsImeAdapter::setImeWindowPosition(int x, int y) {
    pImpl->setImeWindowPosition(x, y);
}

HIMC WindowsImeAdapter::getCurrentInputContext() const {
    return pImpl->getCurrentInputContext();
}

} // namespace windows
} // namespace platform
} // namespace owcat

#endif // _WIN32