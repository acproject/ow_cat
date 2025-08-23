#pragma once

#ifdef _WIN32

#include "platform/platform_manager.h"
#include <windows.h>
#include <imm.h>
#include <memory>
#include <thread>
#include <mutex>

namespace owcat {
namespace platform {
namespace windows {

/**
 * @brief Windows IME适配器
 * 
 * 实现Windows平台的输入法集成，使用Windows IME API。
 * 支持文本组合、候选窗口显示、键盘事件处理等功能。
 */
class WindowsImeAdapter : public PlatformManager {
public:
    WindowsImeAdapter();
    ~WindowsImeAdapter() override;
    
    // PlatformManager接口实现
    bool initialize() override;
    void shutdown() override;
    
    bool registerInputMethod(const std::string& service_name, 
                           const std::string& display_name) override;
    void unregisterInputMethod() override;
    
    void setInputMethodEnabled(bool enabled) override;
    bool isInputMethodEnabled() const override;
    
    void setInputMethodActive(bool active) override;
    bool isInputMethodActive() const override;
    
    void startComposition() override;
    void updateComposition(const std::string& composition_text, int cursor_pos) override;
    void endComposition() override;
    
    void commitText(const std::string& text) override;
    
    void showCandidateWindow(const core::CandidateList& candidates,
                           const CandidateWindowPosition& position) override;
    void hideCandidateWindow() override;
    void updateCandidateSelection(int selected_index) override;
    
    bool getCursorPosition(int& x, int& y) override;
    std::string getCurrentApplication() override;
    
    void setKeyEventCallback(PlatformKeyEventCallback callback) override;
    void setStateChangeCallback(PlatformStateChangeCallback callback) override;
    void setFocusChangeCallback(PlatformFocusChangeCallback callback) override;
    
    std::string getPlatformName() const override;
    std::string getPlatformVersion() const override;
    bool isFeatureSupported(const std::string& feature) const override;
    
    std::map<std::string, std::string> getPlatformConfig() const override;
    void setPlatformConfig(const std::map<std::string, std::string>& config) override;
    
    // Windows特定方法
    
    /**
     * @brief 处理Windows消息
     * @param hwnd 窗口句柄
     * @param msg 消息类型
     * @param wParam wParam参数
     * @param lParam lParam参数
     * @return 消息处理结果
     */
    LRESULT handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    /**
     * @brief 获取IME窗口句柄
     * @return 窗口句柄
     */
    HWND getImeWindow() const;
    
    /**
     * @brief 设置IME窗口位置
     * @param x X坐标
     * @param y Y坐标
     */
    void setImeWindowPosition(int x, int y);
    
    /**
     * @brief 获取当前输入上下文
     * @return 输入上下文句柄
     */
    HIMC getCurrentInputContext() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Windows IME消息处理器
 * 
 * 处理Windows IME相关的系统消息，包括组合文本更新、
 * 候选窗口显示、键盘事件等。
 */
class WindowsImeMessageHandler {
public:
    explicit WindowsImeMessageHandler(WindowsImeAdapter* adapter);
    ~WindowsImeMessageHandler();
    
    /**
     * @brief 安装消息钩子
     * @return 是否安装成功
     */
    bool installHook();
    
    /**
     * @brief 卸载消息钩子
     */
    void uninstallHook();
    
    /**
     * @brief 处理IME消息
     * @param hwnd 窗口句柄
     * @param msg 消息类型
     * @param wParam wParam参数
     * @param lParam lParam参数
     * @return 消息处理结果
     */
    LRESULT processImeMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
private:
    WindowsImeAdapter* adapter_;
    HHOOK keyboard_hook_;
    HHOOK mouse_hook_;
    HWND ime_window_;
    std::thread message_thread_;
    std::mutex message_mutex_;
    bool hook_installed_;
    
    // 静态回调函数
    static LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK imeWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // 消息处理方法
    void handleKeyboardMessage(const KBDLLHOOKSTRUCT* kbd_struct, WPARAM wParam);
    void handleMouseMessage(const MSLLHOOKSTRUCT* mouse_struct, WPARAM wParam);
    void handleImeComposition(HWND hwnd, LPARAM lParam);
    void handleImeStartComposition(HWND hwnd);
    void handleImeEndComposition(HWND hwnd);
    void handleImeNotify(HWND hwnd, WPARAM wParam, LPARAM lParam);
};

/**
 * @brief Windows候选窗口管理器
 * 
 * 管理Windows平台的候选窗口显示，包括窗口创建、
 * 位置调整、候选项渲染等。
 */
class WindowsCandidateWindow {
public:
    WindowsCandidateWindow();
    ~WindowsCandidateWindow();
    
    /**
     * @brief 创建候选窗口
     * @return 是否创建成功
     */
    bool create();
    
    /**
     * @brief 销毁候选窗口
     */
    void destroy();
    
    /**
     * @brief 显示候选窗口
     * @param candidates 候选列表
     * @param position 窗口位置
     */
    void show(const core::CandidateList& candidates, const CandidateWindowPosition& position);
    
    /**
     * @brief 隐藏候选窗口
     */
    void hide();
    
    /**
     * @brief 更新候选选择
     * @param selected_index 选中索引
     */
    void updateSelection(int selected_index);
    
    /**
     * @brief 获取窗口句柄
     * @return 窗口句柄
     */
    HWND getWindowHandle() const;
    
    /**
     * @brief 设置窗口样式
     * @param style 样式配置
     */
    void setWindowStyle(const std::map<std::string, std::string>& style);
    
private:
    HWND window_handle_;
    HFONT font_handle_;
    core::CandidateList current_candidates_;
    int selected_index_;
    CandidateWindowPosition current_position_;
    bool is_visible_;
    
    // 窗口样式配置
    COLORREF background_color_;
    COLORREF text_color_;
    COLORREF selected_background_color_;
    COLORREF selected_text_color_;
    COLORREF border_color_;
    int border_width_;
    int item_height_;
    int window_padding_;
    
    // 静态窗口过程
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // 绘制方法
    void paintWindow(HDC hdc);
    void paintCandidate(HDC hdc, const core::Candidate& candidate, const RECT& rect, bool selected);
    void calculateWindowSize(int& width, int& height);
    void adjustWindowPosition(int& x, int& y, int width, int height);
};

/**
 * @brief Windows系统集成工具
 * 
 * 提供Windows系统相关的工具函数，包括注册表操作、
 * 系统信息获取、权限检查等。
 */
class WindowsSystemIntegration {
public:
    /**
     * @brief 注册输入法到系统
     * @param ime_path 输入法程序路径
     * @param ime_name 输入法名称
     * @return 是否注册成功
     */
    static bool registerImeToSystem(const std::string& ime_path, const std::string& ime_name);
    
    /**
     * @brief 从系统注销输入法
     * @param ime_name 输入法名称
     * @return 是否注销成功
     */
    static bool unregisterImeFromSystem(const std::string& ime_name);
    
    /**
     * @brief 检查是否有管理员权限
     * @return 是否有管理员权限
     */
    static bool hasAdministratorPrivileges();
    
    /**
     * @brief 获取Windows版本信息
     * @return 版本信息字符串
     */
    static std::string getWindowsVersion();
    
    /**
     * @brief 获取当前活动窗口信息
     * @return 窗口信息
     */
    static std::string getActiveWindowInfo();
    
    /**
     * @brief 检查IME功能是否可用
     * @return 是否可用
     */
    static bool isImeAvailable();
    
    /**
     * @brief 获取系统默认字体
     * @return 字体句柄
     */
    static HFONT getSystemDefaultFont();
    
    /**
     * @brief 获取系统DPI设置
     * @return DPI值
     */
    static int getSystemDpi();
    
private:
    // 注册表操作辅助函数
    static bool writeRegistryString(HKEY hkey, const std::string& subkey, 
                                  const std::string& value_name, const std::string& value);
    static bool deleteRegistryKey(HKEY hkey, const std::string& subkey);
    static std::string readRegistryString(HKEY hkey, const std::string& subkey, 
                                        const std::string& value_name);
};

} // namespace windows
} // namespace platform
} // namespace owcat

#endif // _WIN32