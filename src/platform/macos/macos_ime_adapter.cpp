#ifdef __APPLE__

#include "platform/macos/macos_ime_adapter.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>

namespace owcat {
namespace platform {
namespace macos {

// MacOSImeAdapter::Impl 实现
class MacOSImeAdapter::Impl {
public:
    Impl() : enabled_(false), active_(false), composing_(false),
             imk_server_(nullptr), current_controller_(nullptr) {
    }
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize() {
        spdlog::info("Initializing macOS IME adapter");
        
        // 创建系统集成工具
        system_integration_ = std::make_unique<MacOSSystemIntegration>();
        
        // 检查辅助功能权限
        if (!system_integration_->checkAccessibilityPermission()) {
            spdlog::warn("Accessibility permission not granted, requesting...");
            if (!system_integration_->requestAccessibilityPermission()) {
                spdlog::error("Failed to obtain accessibility permission");
                return false;
            }
        }
        
        // 创建候选窗口
        candidate_window_ = std::make_unique<MacOSCandidateWindow>();
        if (!candidate_window_->create()) {
            spdlog::error("Failed to create candidate window");
            return false;
        }
        
        // 注册通知
        system_integration_->registerForNotifications();
        
        spdlog::info("macOS IME adapter initialized successfully");
        return true;
    }
    
    void shutdown() {
        if (system_integration_) {
            system_integration_->unregisterFromNotifications();
        }
        
        if (candidate_window_) {
            candidate_window_->destroy();
            candidate_window_.reset();
        }
        
        if (imk_server_) {
            destroy_imk_server(imk_server_);
            imk_server_ = nullptr;
        }
        
        current_controller_ = nullptr;
        system_integration_.reset();
    }
    
    bool registerInputMethod(const std::string& service_name, const std::string& display_name) {
        service_name_ = service_name;
        display_name_ = display_name;
        
        spdlog::info("Registering input method: {} ({})", display_name, service_name);
        
        // 创建IMK服务器
        std::string connection_name = service_name + "_Connection";
        imk_server_ = create_imk_server(service_name.c_str(), connection_name.c_str());
        
        if (!imk_server_) {
            spdlog::error("Failed to create IMK server");
            return false;
        }
        
        // 注册输入法到系统
        std::string bundle_path = "/Applications/OwCat.app"; // 简化的路径
        if (!system_integration_->registerInputMethod(service_name, bundle_path)) {
            spdlog::warn("Failed to register input method to system (may require manual installation)");
        }
        
        spdlog::info("Input method registered successfully");
        return true;
    }
    
    void unregisterInputMethod() {
        if (!service_name_.empty()) {
            system_integration_->unregisterInputMethod(service_name_);
        }
        
        if (imk_server_) {
            destroy_imk_server(imk_server_);
            imk_server_ = nullptr;
        }
        
        service_name_.clear();
        display_name_.clear();
        
        spdlog::info("Input method unregistered");
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
        
        if (state_change_callback_) {
            state_change_callback_(PlatformInputState::COMPOSING);
        }
    }
    
    void updateComposition(const std::string& composition_text, int cursor_pos) {
        composition_text_ = composition_text;
        cursor_position_ = cursor_pos;
        
        spdlog::debug("Updated composition: '{}' at position {}", composition_text, cursor_pos);
        
        // 更新标记文本
        if (current_controller_) {
            set_marked_text_objc(current_controller_, composition_text.c_str(), cursor_pos);
        }
    }
    
    void endComposition() {
        composing_ = false;
        composition_text_.clear();
        cursor_position_ = 0;
        
        spdlog::debug("Ended composition");
        
        // 取消标记文本
        if (current_controller_) {
            unmark_text_objc(current_controller_);
        }
        
        if (state_change_callback_) {
            PlatformInputState state = active_ ? PlatformInputState::ACTIVE : 
                                      (enabled_ ? PlatformInputState::ENABLED : PlatformInputState::DISABLED);
            state_change_callback_(state);
        }
    }
    
    void commitText(const std::string& text) {
        spdlog::debug("Committing text: '{}'", text);
        
        if (current_controller_) {
            insert_text_objc(current_controller_, text.c_str());
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
        if (current_controller_) {
            return static_cast<MacOSInputController*>(current_controller_)->getCursorPosition(x, y);
        }
        return false;
    }
    
    std::string getCurrentApplication() {
        return system_integration_->getFrontmostApplication();
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
        return "macOS";
    }
    
    std::string getPlatformVersion() const {
        return MacOSSystemIntegration::getMacOSVersion();
    }
    
    bool isFeatureSupported(const std::string& feature) const {
        // macOS支持的功能列表
        static const std::vector<std::string> supported_features = {
            "composition",
            "candidate_window",
            "cursor_tracking",
            "application_detection",
            "input_source_switching",
            "accessibility_api",
            "imk_integration"
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
    
    void* getIMKServer() const {
        return imk_server_;
    }
    
    void setCurrentInputController(void* controller) {
        current_controller_ = controller;
    }
    
    void* getCurrentInputController() const {
        return current_controller_;
    }
    
    bool handleKeyEvent(void* event) {
        if (!current_controller_) {
            return false;
        }
        
        return handle_key_event_objc(current_controller_, event);
    }
    
    void updateClientInfo(const std::string& bundle_id, const std::string& app_name) {
        current_bundle_id_ = bundle_id;
        current_app_name_ = app_name;
        
        spdlog::debug("Client info updated: {} ({})", app_name, bundle_id);
        
        if (focus_change_callback_) {
            focus_change_callback_(true); // 简化的焦点变化通知
        }
    }
    
    void calculateCandidateWindowPosition(int& x, int& y) {
        // 获取光标位置
        if (!getCursorPosition(x, y)) {
            // 如果无法获取光标位置，使用默认位置
            x = 100;
            y = 100;
        } else {
            // 在光标下方显示候选窗口
            y += 25;
        }
    }
    
public:
    bool enabled_;
    bool active_;
    bool composing_;
    std::string service_name_;
    std::string display_name_;
    std::string composition_text_;
    int cursor_position_;
    
    void* imk_server_;
    void* current_controller_;
    
    std::string current_bundle_id_;
    std::string current_app_name_;
    
    std::unique_ptr<MacOSCandidateWindow> candidate_window_;
    std::unique_ptr<MacOSSystemIntegration> system_integration_;
    
    PlatformKeyEventCallback key_event_callback_;
    PlatformStateChangeCallback state_change_callback_;
    PlatformFocusChangeCallback focus_change_callback_;
    
    std::map<std::string, std::string> platform_config_;
};

// MacOSImeAdapter 实现
MacOSImeAdapter::MacOSImeAdapter()
    : pImpl(std::make_unique<Impl>()) {
}

MacOSImeAdapter::~MacOSImeAdapter() = default;

bool MacOSImeAdapter::initialize() {
    return pImpl->initialize();
}

void MacOSImeAdapter::shutdown() {
    pImpl->shutdown();
}

bool MacOSImeAdapter::registerInputMethod(const std::string& service_name, const std::string& display_name) {
    return pImpl->registerInputMethod(service_name, display_name);
}

void MacOSImeAdapter::unregisterInputMethod() {
    pImpl->unregisterInputMethod();
}

void MacOSImeAdapter::setInputMethodEnabled(bool enabled) {
    pImpl->setInputMethodEnabled(enabled);
}

bool MacOSImeAdapter::isInputMethodEnabled() const {
    return pImpl->isInputMethodEnabled();
}

void MacOSImeAdapter::setInputMethodActive(bool active) {
    pImpl->setInputMethodActive(active);
}

bool MacOSImeAdapter::isInputMethodActive() const {
    return pImpl->isInputMethodActive();
}

void MacOSImeAdapter::startComposition() {
    pImpl->startComposition();
}

void MacOSImeAdapter::updateComposition(const std::string& composition_text, int cursor_pos) {
    pImpl->updateComposition(composition_text, cursor_pos);
}

void MacOSImeAdapter::endComposition() {
    pImpl->endComposition();
}

void MacOSImeAdapter::commitText(const std::string& text) {
    pImpl->commitText(text);
}

void MacOSImeAdapter::showCandidateWindow(const core::CandidateList& candidates,
                                        const CandidateWindowPosition& position) {
    pImpl->showCandidateWindow(candidates, position);
}

void MacOSImeAdapter::hideCandidateWindow() {
    pImpl->hideCandidateWindow();
}

void MacOSImeAdapter::updateCandidateSelection(int selected_index) {
    pImpl->updateCandidateSelection(selected_index);
}

bool MacOSImeAdapter::getCursorPosition(int& x, int& y) {
    return pImpl->getCursorPosition(x, y);
}

std::string MacOSImeAdapter::getCurrentApplication() {
    return pImpl->getCurrentApplication();
}

void MacOSImeAdapter::setKeyEventCallback(PlatformKeyEventCallback callback) {
    pImpl->setKeyEventCallback(callback);
}

void MacOSImeAdapter::setStateChangeCallback(PlatformStateChangeCallback callback) {
    pImpl->setStateChangeCallback(callback);
}

void MacOSImeAdapter::setFocusChangeCallback(PlatformFocusChangeCallback callback) {
    pImpl->setFocusChangeCallback(callback);
}

std::string MacOSImeAdapter::getPlatformName() const {
    return pImpl->getPlatformName();
}

std::string MacOSImeAdapter::getPlatformVersion() const {
    return pImpl->getPlatformVersion();
}

bool MacOSImeAdapter::isFeatureSupported(const std::string& feature) const {
    return pImpl->isFeatureSupported(feature);
}

std::map<std::string, std::string> MacOSImeAdapter::getPlatformConfig() const {
    return pImpl->getPlatformConfig();
}

void MacOSImeAdapter::setPlatformConfig(const std::map<std::string, std::string>& config) {
    pImpl->setPlatformConfig(config);
}

IMKServer* MacOSImeAdapter::getIMKServer() const {
    return static_cast<IMKServer*>(pImpl->getIMKServer());
}

void MacOSImeAdapter::setCurrentInputController(IMKInputController* controller) {
    pImpl->setCurrentInputController(controller);
}

IMKInputController* MacOSImeAdapter::getCurrentInputController() const {
    return static_cast<IMKInputController*>(pImpl->getCurrentInputController());
}

bool MacOSImeAdapter::handleKeyEvent(NSEvent* event) {
    return pImpl->handleKeyEvent(event);
}

void MacOSImeAdapter::updateClientInfo(const std::string& bundle_id, const std::string& app_name) {
    pImpl->updateClientInfo(bundle_id, app_name);
}

void MacOSImeAdapter::calculateCandidateWindowPosition(int& x, int& y) {
    pImpl->calculateCandidateWindowPosition(x, y);
}

} // namespace macos
} // namespace platform
} // namespace owcat

#endif // __APPLE__