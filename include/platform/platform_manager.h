#pragma once

#include "core/types.h"
#include <memory>
#include <string>
#include <functional>

namespace owcat {
namespace platform {

/**
 * @brief 平台特定的输入法状态
 */
enum class PlatformInputState {
    DISABLED,    // 输入法已禁用
    ENABLED,     // 输入法已启用但未激活
    ACTIVE,      // 输入法已激活
    COMPOSING    // 正在组合输入
};

/**
 * @brief 平台特定的键盘事件信息
 */
struct PlatformKeyEvent {
    uint32_t key_code;        // 平台相关的键码
    uint32_t scan_code;       // 扫描码
    uint32_t modifiers;       // 修饰键状态
    bool is_key_down;         // 是否为按下事件
    bool is_repeat;           // 是否为重复事件
    std::string text;         // 如果有的话，键盘输入的文本
};

/**
 * @brief 候选窗口位置信息
 */
struct CandidateWindowPosition {
    int x, y;                 // 窗口位置
    int width, height;        // 窗口大小
    bool follow_cursor;       // 是否跟随光标
};

/**
 * @brief 平台适配层回调函数类型
 */
using PlatformKeyEventCallback = std::function<bool(const PlatformKeyEvent&)>;
using PlatformStateChangeCallback = std::function<void(PlatformInputState)>;
using PlatformFocusChangeCallback = std::function<void(bool has_focus)>;

/**
 * @brief 平台适配层管理器接口
 * 
 * 这个类提供了跨平台的输入法集成接口，封装了不同操作系统的输入法API。
 * 支持Windows IME、macOS Input Method Kit、Linux IBus/Fcitx等。
 */
class PlatformManager {
public:
    virtual ~PlatformManager() = default;
    
    /**
     * @brief 初始化平台适配层
     * @return 是否初始化成功
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief 关闭平台适配层
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief 注册输入法服务
     * @param service_name 输入法服务名称
     * @param display_name 显示名称
     * @return 是否注册成功
     */
    virtual bool registerInputMethod(const std::string& service_name, 
                                   const std::string& display_name) = 0;
    
    /**
     * @brief 注销输入法服务
     */
    virtual void unregisterInputMethod() = 0;
    
    /**
     * @brief 启用/禁用输入法
     * @param enabled 是否启用
     */
    virtual void setInputMethodEnabled(bool enabled) = 0;
    
    /**
     * @brief 获取输入法启用状态
     * @return 是否已启用
     */
    virtual bool isInputMethodEnabled() const = 0;
    
    /**
     * @brief 激活/取消激活输入法
     * @param active 是否激活
     */
    virtual void setInputMethodActive(bool active) = 0;
    
    /**
     * @brief 获取输入法激活状态
     * @return 是否已激活
     */
    virtual bool isInputMethodActive() const = 0;
    
    /**
     * @brief 开始文本组合
     */
    virtual void startComposition() = 0;
    
    /**
     * @brief 更新组合文本
     * @param composition_text 组合中的文本
     * @param cursor_pos 光标位置
     */
    virtual void updateComposition(const std::string& composition_text, int cursor_pos) = 0;
    
    /**
     * @brief 结束文本组合
     */
    virtual void endComposition() = 0;
    
    /**
     * @brief 提交文本到应用程序
     * @param text 要提交的文本
     */
    virtual void commitText(const std::string& text) = 0;
    
    /**
     * @brief 显示候选窗口
     * @param candidates 候选列表
     * @param position 窗口位置
     */
    virtual void showCandidateWindow(const core::CandidateList& candidates,
                                   const CandidateWindowPosition& position) = 0;
    
    /**
     * @brief 隐藏候选窗口
     */
    virtual void hideCandidateWindow() = 0;
    
    /**
     * @brief 更新候选窗口选择
     * @param selected_index 选中的候选项索引
     */
    virtual void updateCandidateSelection(int selected_index) = 0;
    
    /**
     * @brief 获取当前光标位置
     * @param x 光标X坐标
     * @param y 光标Y坐标
     * @return 是否成功获取
     */
    virtual bool getCursorPosition(int& x, int& y) = 0;
    
    /**
     * @brief 获取当前应用程序信息
     * @return 应用程序名称或标识
     */
    virtual std::string getCurrentApplication() = 0;
    
    /**
     * @brief 设置键盘事件回调
     * @param callback 回调函数
     */
    virtual void setKeyEventCallback(PlatformKeyEventCallback callback) = 0;
    
    /**
     * @brief 设置状态变化回调
     * @param callback 回调函数
     */
    virtual void setStateChangeCallback(PlatformStateChangeCallback callback) = 0;
    
    /**
     * @brief 设置焦点变化回调
     * @param callback 回调函数
     */
    virtual void setFocusChangeCallback(PlatformFocusChangeCallback callback) = 0;
    
    /**
     * @brief 获取平台名称
     * @return 平台名称字符串
     */
    virtual std::string getPlatformName() const = 0;
    
    /**
     * @brief 获取平台版本信息
     * @return 版本信息字符串
     */
    virtual std::string getPlatformVersion() const = 0;
    
    /**
     * @brief 检查平台是否支持特定功能
     * @param feature 功能名称
     * @return 是否支持
     */
    virtual bool isFeatureSupported(const std::string& feature) const = 0;
    
    /**
     * @brief 获取平台特定的配置选项
     * @return 配置选项映射
     */
    virtual std::map<std::string, std::string> getPlatformConfig() const = 0;
    
    /**
     * @brief 设置平台特定的配置选项
     * @param config 配置选项映射
     */
    virtual void setPlatformConfig(const std::map<std::string, std::string>& config) = 0;
};

/**
 * @brief 创建平台管理器实例
 * @return 平台管理器智能指针
 */
std::unique_ptr<PlatformManager> createPlatformManager();

/**
 * @brief 获取当前平台类型
 * @return 平台类型字符串
 */
std::string getCurrentPlatform();

/**
 * @brief 检查当前平台是否支持输入法集成
 * @return 是否支持
 */
bool isPlatformSupported();

} // namespace platform
} // namespace owcat