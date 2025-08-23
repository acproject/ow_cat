#pragma once

#include "types.h"
#include <memory>
#include <string>

namespace owcat {
namespace core {

class PinyinConverter;
class DictionaryManager;
class PredictionEngine;

/**
 * 输入法核心引擎
 * 负责协调各个组件，处理输入事件，生成候选词
 */
class Engine {
public:
    explicit Engine(const EngineConfig& config = EngineConfig{});
    ~Engine();

    // 禁用拷贝和移动
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    /**
     * 初始化引擎
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * 关闭引擎
     */
    void shutdown();

    /**
     * 处理输入事件
     * @param event 输入事件
     * @return 是否处理成功
     */
    bool processInput(const InputEvent& event);

    /**
     * 获取当前候选词列表
     * @return 候选词列表
     */
    const CandidateList& getCandidates() const;

    /**
     * 获取当前组合字符串（拼音）
     * @return 组合字符串
     */
    const std::string& getComposition() const;

    /**
     * 获取当前输入状态
     * @return 输入状态
     */
    InputState getState() const;

    /**
     * 选择候选词
     * @param index 候选词索引
     * @return 是否选择成功
     */
    bool selectCandidate(int index);

    /**
     * 提交当前组合
     * @return 提交的文本
     */
    std::string commitComposition();

    /**
     * 清除当前组合
     */
    void clearComposition();

    /**
     * 设置候选词回调
     * @param callback 回调函数
     */
    void setCandidateCallback(CandidateCallback callback);

    /**
     * 设置提交回调
     * @param callback 回调函数
     */
    void setCommitCallback(CommitCallback callback);

    /**
     * 设置状态变化回调
     * @param callback 回调函数
     */
    void setStateChangeCallback(StateChangeCallback callback);

    /**
     * 更新配置
     * @param config 新配置
     */
    void updateConfig(const EngineConfig& config);

    /**
     * 获取当前配置
     * @return 当前配置
     */
    const EngineConfig& getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace core
} // namespace owcat