#pragma once

#include <string>
#include <vector>
#include <memory>

namespace owcat {
namespace core {

/**
 * Llama.cpp预测器
 * 封装llama.cpp库，提供AI预测功能
 */
class LlamaPredictor {
public:
    explicit LlamaPredictor(const std::string& model_path);
    ~LlamaPredictor();

    // 禁用拷贝和移动
    LlamaPredictor(const LlamaPredictor&) = delete;
    LlamaPredictor& operator=(const LlamaPredictor&) = delete;
    LlamaPredictor(LlamaPredictor&&) = delete;
    LlamaPredictor& operator=(LlamaPredictor&&) = delete;

    /**
     * 初始化模型
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * 关闭模型
     */
    void shutdown();

    /**
     * 生成文本预测
     * @param prompt 提示文本
     * @param max_tokens 最大生成token数
     * @param temperature 温度参数（控制随机性）
     * @param top_p 核采样参数
     * @return 生成的文本列表
     */
    std::vector<std::string> generateText(
        const std::string& prompt,
        int max_tokens = 50,
        float temperature = 0.7f,
        float top_p = 0.9f
    ) const;

    /**
     * 获取下一个词的概率分布
     * @param context 上下文
     * @param candidates 候选词列表
     * @return 每个候选词的概率
     */
    std::vector<float> getNextWordProbabilities(
        const std::string& context,
        const std::vector<std::string>& candidates
    ) const;

    /**
     * 计算文本的困惑度
     * @param text 文本
     * @return 困惑度值
     */
    float calculatePerplexity(const std::string& text) const;

    /**
     * 检查模型是否已加载
     * @return 是否已加载
     */
    bool isLoaded() const;

    /**
     * 获取模型信息
     * @return 模型信息
     */
    std::string getModelInfo() const;

    /**
     * 设置生成参数
     * @param temperature 温度
     * @param top_p 核采样参数
     * @param top_k 顶k采样参数
     */
    void setGenerationParams(float temperature, float top_p, int top_k = 40);

    /**
     * 预热模型（进行一次推理以加载到GPU内存）
     * @return 是否预热成功
     */
    bool warmup();

private:
    /**
     * 加载模型文件
     * @return 是否加载成功
     */
    bool loadModel();

    /**
     * 释放模型资源
     */
    void unloadModel();

    /**
     * 预处理输入文本
     * @param text 原始文本
     * @return 预处理后的文本
     */
    std::string preprocessInput(const std::string& text) const;

    /**
     * 后处理输出文本
     * @param text 原始输出
     * @return 处理后的文本
     */
    std::string postprocessOutput(const std::string& text) const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace core
} // namespace owcat