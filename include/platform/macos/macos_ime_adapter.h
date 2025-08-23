#pragma once

#ifdef __APPLE__

#include "platform/platform_manager.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

// Forward declarations for Objective-C classes
#ifdef __OBJC__
@class NSString;
@class NSArray;
@class NSDictionary;
@class NSWindow;
@class NSView;
@class IMKServer;
@class IMKInputController;
@class NSEvent;
#else
typedef struct objc_object NSString;
typedef struct objc_object NSArray;
typedef struct objc_object NSDictionary;
typedef struct objc_object NSWindow;
typedef struct objc_object NSView;
typedef struct objc_object IMKServer;
typedef struct objc_object IMKInputController;
typedef struct objc_object NSEvent;
#endif

namespace owcat {
namespace platform {
namespace macos {

/**
 * macOS输入法适配器
 * 使用Input Method Kit (IMK)框架实现
 */
class MacOSImeAdapter : public PlatformManager {
public:
    MacOSImeAdapter();
    ~MacOSImeAdapter() override;
    
    // PlatformManager接口实现
    bool initialize() override;
    void shutdown() override;
    
    bool registerInputMethod(const std::string& service_name, const std::string& display_name) override;
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
    
    // macOS特定方法
    IMKServer* getIMKServer() const;
    void setCurrentInputController(IMKInputController* controller);
    IMKInputController* getCurrentInputController() const;
    
    bool handleKeyEvent(NSEvent* event);
    void updateClientInfo(const std::string& bundle_id, const std::string& app_name);
    
    // 候选窗口位置计算
    void calculateCandidateWindowPosition(int& x, int& y);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * macOS输入控制器
 * 继承自IMKInputController，处理具体的输入事件
 */
class MacOSInputController {
public:
    MacOSInputController();
    ~MacOSInputController();
    
    bool initialize(MacOSImeAdapter* adapter, IMKServer* server, const std::string& delegate_name);
    void shutdown();
    
    // 输入事件处理
    bool handleKeyEvent(NSEvent* event);
    void insertText(const std::string& text);
    void setMarkedText(const std::string& text, int cursor_pos);
    void unmarkText();
    
    // 候选窗口管理
    void showCandidates(const core::CandidateList& candidates);
    void hideCandidates();
    void updateCandidateSelection(int selected_index);
    
    // 客户端信息
    std::string getClientBundleID() const;
    std::string getClientApplicationName() const;
    
    // 光标和选择范围
    bool getCursorPosition(int& x, int& y) const;
    bool getSelectedRange(int& start, int& length) const;
    
    // IMK回调方法（由Objective-C桥接调用）
    void onActivate();
    void onDeactivate();
    void onFocusChange(bool has_focus);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * macOS候选窗口
 * 使用NSWindow实现候选词显示
 */
class MacOSCandidateWindow {
public:
    MacOSCandidateWindow();
    ~MacOSCandidateWindow();
    
    bool create();
    void destroy();
    
    void show(const core::CandidateList& candidates, const CandidateWindowPosition& position);
    void hide();
    void updateSelection(int selected_index);
    
    void setWindowStyle(const std::map<std::string, std::string>& style);
    
    // 事件处理
    void onMouseClick(int x, int y);
    void onMouseMove(int x, int y);
    void onKeyEvent(int key_code, bool is_key_down);
    
    // 回调设置
    void setSelectionCallback(std::function<void(int)> callback);
    
    // 窗口属性
    NSWindow* getWindow() const;
    bool isVisible() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * macOS系统集成工具
 * 提供系统级别的功能，如输入法注册、系统信息获取等
 */
class MacOSSystemIntegration {
public:
    MacOSSystemIntegration();
    ~MacOSSystemIntegration();
    
    // 输入法管理
    bool registerInputMethod(const std::string& bundle_id, const std::string& bundle_path);
    bool unregisterInputMethod(const std::string& bundle_id);
    bool isInputMethodRegistered(const std::string& bundle_id);
    
    // 系统信息
    static std::string getMacOSVersion();
    static std::map<std::string, std::string> getSystemInfo();
    
    // 输入源管理
    std::vector<std::string> getInstalledInputSources();
    std::string getCurrentInputSource();
    bool setCurrentInputSource(const std::string& source_id);
    
    // 应用程序信息
    std::string getFrontmostApplication();
    std::vector<std::string> getRunningApplications();
    
    // 权限检查
    bool checkAccessibilityPermission();
    bool requestAccessibilityPermission();
    
    // 通知中心
    void registerForNotifications();
    void unregisterFromNotifications();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Objective-C桥接函数声明
extern "C" {
    // 这些函数将在Objective-C实现文件中定义
    void* create_imk_server(const char* bundle_id, const char* connection_name);
    void destroy_imk_server(void* server);
    
    void* create_input_controller(void* server, const char* delegate_name, void* client);
    void destroy_input_controller(void* controller);
    
    bool handle_key_event_objc(void* controller, void* event);
    void insert_text_objc(void* controller, const char* text);
    void set_marked_text_objc(void* controller, const char* text, int cursor_pos);
    void unmark_text_objc(void* controller);
    
    void* create_candidate_window();
    void destroy_candidate_window(void* window);
    void show_candidate_window(void* window, const char** candidates, int count, int x, int y);
    void hide_candidate_window(void* window);
    
    const char* get_macos_version();
    const char* get_frontmost_application();
    bool check_accessibility_permission();
    bool request_accessibility_permission();
}

} // namespace macos
} // namespace platform
} // namespace owcat

#endif // __APPLE__