#ifdef _WIN32

#include "platform/windows/windows_ime_adapter.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace owcat {
namespace platform {
namespace windows {

// 全局钩子句柄
static HHOOK g_keyboard_hook = nullptr;
static HHOOK g_mouse_hook = nullptr;
static WindowsImeMessageHandler* g_handler_instance = nullptr;

// 键盘钩子过程
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_handler_instance) {
        KBDLLHOOKSTRUCT* kbd_struct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        
        PlatformKeyEvent event;
        event.key_code = kbd_struct->vkCode;
        event.scan_code = kbd_struct->scanCode;
        event.is_key_down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        event.is_repeat = false; // 简化处理
        event.modifiers = 0;
        
        // 检查修饰键状态
        if (GetKeyState(VK_CONTROL) & 0x8000) event.modifiers |= 0x01; // CTRL
        if (GetKeyState(VK_SHIFT) & 0x8000) event.modifiers |= 0x02;   // SHIFT
        if (GetKeyState(VK_MENU) & 0x8000) event.modifiers |= 0x04;    // ALT
        if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000) event.modifiers |= 0x08; // WIN
        
        // 处理键盘事件
        if (g_handler_instance->processKeyboardEvent(event)) {
            return 1; // 阻止事件传递
        }
    }
    
    return CallNextHookEx(g_keyboard_hook, nCode, wParam, lParam);
}

// 鼠标钩子过程
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_handler_instance) {
        MSLLHOOKSTRUCT* mouse_struct = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        
        // 处理鼠标事件（主要用于候选窗口交互）
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
            g_handler_instance->processMouseEvent(mouse_struct->pt.x, mouse_struct->pt.y, wParam == WM_LBUTTONDOWN);
        }
    }
    
    return CallNextHookEx(g_mouse_hook, nCode, wParam, lParam);
}

// WindowsImeMessageHandler::Impl 实现
class WindowsImeMessageHandler::Impl {
public:
    Impl(WindowsImeAdapter* adapter) 
        : adapter_(adapter), hook_installed_(false) {
    }
    
    ~Impl() {
        uninstallHook();
    }
    
    bool installHook() {
        if (hook_installed_) {
            return true;
        }
        
        spdlog::info("Installing Windows IME message hooks");
        
        // 安装键盘钩子
        g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, 
                                         GetModuleHandle(nullptr), 0);
        if (!g_keyboard_hook) {
            spdlog::error("Failed to install keyboard hook: {}", GetLastError());
            return false;
        }
        
        // 安装鼠标钩子
        g_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc,
                                      GetModuleHandle(nullptr), 0);
        if (!g_mouse_hook) {
            spdlog::error("Failed to install mouse hook: {}", GetLastError());
            UnhookWindowsHookEx(g_keyboard_hook);
            g_keyboard_hook = nullptr;
            return false;
        }
        
        g_handler_instance = this;
        hook_installed_ = true;
        
        spdlog::info("Windows IME message hooks installed successfully");
        return true;
    }
    
    void uninstallHook() {
        if (!hook_installed_) {
            return;
        }
        
        spdlog::info("Uninstalling Windows IME message hooks");
        
        if (g_keyboard_hook) {
            UnhookWindowsHookEx(g_keyboard_hook);
            g_keyboard_hook = nullptr;
        }
        
        if (g_mouse_hook) {
            UnhookWindowsHookEx(g_mouse_hook);
            g_mouse_hook = nullptr;
        }
        
        g_handler_instance = nullptr;
        hook_installed_ = false;
        
        spdlog::info("Windows IME message hooks uninstalled");
    }
    
    LRESULT processImeMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_IME_STARTCOMPOSITION:
                return handleStartComposition(hwnd, wParam, lParam);
                
            case WM_IME_COMPOSITION:
                return handleComposition(hwnd, wParam, lParam);
                
            case WM_IME_ENDCOMPOSITION:
                return handleEndComposition(hwnd, wParam, lParam);
                
            case WM_IME_NOTIFY:
                return handleImeNotify(hwnd, wParam, lParam);
                
            case WM_IME_REQUEST:
                return handleImeRequest(hwnd, wParam, lParam);
                
            case WM_IME_SETCONTEXT:
                return handleSetContext(hwnd, wParam, lParam);
                
            case WM_INPUTLANGCHANGE:
                return handleInputLanguageChange(hwnd, wParam, lParam);
                
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    
    bool processKeyboardEvent(const PlatformKeyEvent& event) {
        if (!adapter_) {
            return false;
        }
        
        // 检查是否是输入法激活键（例如 Ctrl+Space）
        if (event.is_key_down && event.key_code == VK_SPACE && (event.modifiers & 0x01)) {
            bool current_state = adapter_->isInputMethodActive();
            adapter_->setInputMethodActive(!current_state);
            spdlog::debug("Input method toggled: {}", !current_state);
            return true;
        }
        
        // 如果输入法未激活，不处理其他键盘事件
        if (!adapter_->isInputMethodActive()) {
            return false;
        }
        
        // 处理特殊键
        if (event.is_key_down) {
            switch (event.key_code) {
                case VK_ESCAPE:
                    // 取消组合
                    if (adapter_->pImpl->composing_) {
                        adapter_->endComposition();
                        return true;
                    }
                    break;
                    
                case VK_RETURN:
                    // 提交当前组合
                    if (adapter_->pImpl->composing_) {
                        adapter_->commitText(adapter_->pImpl->composition_text_);
                        adapter_->endComposition();
                        return true;
                    }
                    break;
                    
                case VK_BACK:
                    // 删除字符
                    if (adapter_->pImpl->composing_) {
                        // 这里应该调用核心引擎的删除字符方法
                        spdlog::debug("Backspace in composition mode");
                        return true;
                    }
                    break;
                    
                case VK_UP:
                case VK_DOWN:
                case VK_LEFT:
                case VK_RIGHT:
                    // 候选选择
                    if (adapter_->pImpl->composing_) {
                        spdlog::debug("Arrow key in composition mode: {}", event.key_code);
                        return true;
                    }
                    break;
                    
                default:
                    // 普通字符输入
                    if (isInputChar(event.key_code)) {
                        char ch = static_cast<char>(event.key_code);
                        if (event.modifiers & 0x02) { // Shift
                            ch = std::toupper(ch);
                        } else {
                            ch = std::tolower(ch);
                        }
                        
                        if (!adapter_->pImpl->composing_) {
                            adapter_->startComposition();
                        }
                        
                        // 这里应该调用核心引擎的字符处理方法
                        spdlog::debug("Input character: '{}'", ch);
                        return true;
                    }
                    break;
            }
        }
        
        return false;
    }
    
    void processMouseEvent(int x, int y, bool is_left_button) {
        // 处理候选窗口的鼠标点击
        spdlog::debug("Mouse event at ({}, {}), left button: {}", x, y, is_left_button);
    }
    
private:
    LRESULT handleStartComposition(HWND hwnd, WPARAM wParam, LPARAM lParam) {
        spdlog::debug("WM_IME_STARTCOMPOSITION");
        if (adapter_) {
            adapter_->startComposition();
        }
        return 0;
    }
    
    LRESULT handleComposition(HWND hwnd, WPARAM wParam, LPARAM lParam) {
        spdlog::debug("WM_IME_COMPOSITION, lParam: 0x{:x}", lParam);
        
        HIMC himc = ImmGetContext(hwnd);
        if (!himc) {
            return DefWindowProc(hwnd, WM_IME_COMPOSITION, wParam, lParam);
        }
        
        if (lParam & GCS_COMPSTR) {
            // 获取组合字符串
            LONG len = ImmGetCompositionStringW(himc, GCS_COMPSTR, nullptr, 0);
            if (len > 0) {
                std::vector<wchar_t> buffer(len / sizeof(wchar_t) + 1);
                ImmGetCompositionStringW(himc, GCS_COMPSTR, buffer.data(), len);
                
                std::wstring wstr(buffer.data(), len / sizeof(wchar_t));
                std::string composition = wstringToUtf8(wstr);
                
                if (adapter_) {
                    adapter_->updateComposition(composition, composition.length());
                }
            }
        }
        
        if (lParam & GCS_RESULTSTR) {
            // 获取结果字符串
            LONG len = ImmGetCompositionStringW(himc, GCS_RESULTSTR, nullptr, 0);
            if (len > 0) {
                std::vector<wchar_t> buffer(len / sizeof(wchar_t) + 1);
                ImmGetCompositionStringW(himc, GCS_RESULTSTR, buffer.data(), len);
                
                std::wstring wstr(buffer.data(), len / sizeof(wchar_t));
                std::string result = wstringToUtf8(wstr);
                
                if (adapter_) {
                    adapter_->commitText(result);
                }
            }
        }
        
        ImmReleaseContext(hwnd, himc);
        return 0;
    }
    
    LRESULT handleEndComposition(HWND hwnd, WPARAM wParam, LPARAM lParam) {
        spdlog::debug("WM_IME_ENDCOMPOSITION");
        if (adapter_) {
            adapter_->endComposition();
        }
        return 0;
    }
    
    LRESULT handleImeNotify(HWND hwnd, WPARAM wParam, LPARAM lParam) {
        spdlog::debug("WM_IME_NOTIFY, wParam: 0x{:x}", wParam);
        
        switch (wParam) {
            case IMN_OPENCANDIDATE:
                spdlog::debug("IMN_OPENCANDIDATE");
                break;
                
            case IMN_CLOSECANDIDATE:
                spdlog::debug("IMN_CLOSECANDIDATE");
                if (adapter_) {
                    adapter_->hideCandidateWindow();
                }
                break;
                
            case IMN_CHANGECANDIDATE:
                spdlog::debug("IMN_CHANGECANDIDATE");
                break;
                
            case IMN_SETOPENSTATUS:
                spdlog::debug("IMN_SETOPENSTATUS");
                break;
        }
        
        return DefWindowProc(hwnd, WM_IME_NOTIFY, wParam, lParam);
    }
    
    LRESULT handleImeRequest(HWND hwnd, WPARAM wParam, LPARAM lParam) {
        spdlog::debug("WM_IME_REQUEST, wParam: 0x{:x}", wParam);
        
        switch (wParam) {
            case IMR_COMPOSITIONWINDOW:
                // 返回组合窗口位置
                if (lParam) {
                    COMPOSITIONFORM* comp_form = reinterpret_cast<COMPOSITIONFORM*>(lParam);
                    comp_form->dwStyle = CFS_POINT;
                    comp_form->ptCurrentPos.x = 100; // 简化的位置
                    comp_form->ptCurrentPos.y = 100;
                    return sizeof(COMPOSITIONFORM);
                }
                break;
                
            case IMR_CANDIDATEWINDOW:
                // 返回候选窗口位置
                if (lParam) {
                    CANDIDATEFORM* cand_form = reinterpret_cast<CANDIDATEFORM*>(lParam);
                    cand_form->dwStyle = CFS_CANDIDATEPOS;
                    cand_form->ptCurrentPos.x = 100;
                    cand_form->ptCurrentPos.y = 120;
                    return sizeof(CANDIDATEFORM);
                }
                break;
        }
        
        return DefWindowProc(hwnd, WM_IME_REQUEST, wParam, lParam);
    }
    
    LRESULT handleSetContext(HWND hwnd, WPARAM wParam, LPARAM lParam) {
        spdlog::debug("WM_IME_SETCONTEXT, wParam: {}, lParam: 0x{:x}", wParam, lParam);
        
        // 禁用系统默认的候选窗口和组合窗口
        lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
        lParam &= ~ISC_SHOWUICANDIDATEWINDOW;
        
        return DefWindowProc(hwnd, WM_IME_SETCONTEXT, wParam, lParam);
    }
    
    LRESULT handleInputLanguageChange(HWND hwnd, WPARAM wParam, LPARAM lParam) {
        spdlog::debug("WM_INPUTLANGCHANGE, wParam: 0x{:x}, lParam: 0x{:x}", wParam, lParam);
        
        // 检查是否切换到我们的输入法
        HKL hkl = reinterpret_cast<HKL>(lParam);
        wchar_t ime_name[256];
        if (ImmGetDescriptionW(hkl, ime_name, sizeof(ime_name) / sizeof(wchar_t))) {
            std::string ime_name_str = wstringToUtf8(ime_name);
            spdlog::info("Input language changed to: {}", ime_name_str);
        }
        
        return DefWindowProc(hwnd, WM_INPUTLANGCHANGE, wParam, lParam);
    }
    
    bool isInputChar(DWORD vk_code) const {
        // 检查是否是可输入字符
        return (vk_code >= 'A' && vk_code <= 'Z') ||
               (vk_code >= '0' && vk_code <= '9') ||
               vk_code == VK_SPACE ||
               vk_code == VK_OEM_COMMA ||  // ,
               vk_code == VK_OEM_PERIOD || // .
               vk_code == VK_OEM_1 ||      // ;
               vk_code == VK_OEM_2 ||      // /
               vk_code == VK_OEM_3 ||      // `
               vk_code == VK_OEM_4 ||      // [
               vk_code == VK_OEM_5 ||      // \
               vk_code == VK_OEM_6 ||      // ]
               vk_code == VK_OEM_7;        // '
    }
    
private:
    WindowsImeAdapter* adapter_;
    bool hook_installed_;
};

// WindowsImeMessageHandler 实现
WindowsImeMessageHandler::WindowsImeMessageHandler(WindowsImeAdapter* adapter)
    : pImpl(std::make_unique<Impl>(adapter)) {
}

WindowsImeMessageHandler::~WindowsImeMessageHandler() = default;

bool WindowsImeMessageHandler::installHook() {
    return pImpl->installHook();
}

void WindowsImeMessageHandler::uninstallHook() {
    pImpl->uninstallHook();
}

LRESULT WindowsImeMessageHandler::processImeMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return pImpl->processImeMessage(hwnd, msg, wParam, lParam);
}

bool WindowsImeMessageHandler::processKeyboardEvent(const PlatformKeyEvent& event) {
    return pImpl->processKeyboardEvent(event);
}

void WindowsImeMessageHandler::processMouseEvent(int x, int y, bool is_left_button) {
    pImpl->processMouseEvent(x, y, is_left_button);
}

} // namespace windows
} // namespace platform
} // namespace owcat

#endif // _WIN32