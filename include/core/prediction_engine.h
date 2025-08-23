#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <memory>

namespace owcat {
namespace core {

class LlamaPredictor;

/**
 * 预测引擎
 * 负责基于上下文和历史输入进行智能预测
 */
class PredictionEngine {
public:
    explicit PredictionEngine(const std::string& model_path = "");
    ~PredictionEngine();

    // 禁用拷贝和移动
    PredictionEngine(const PredictionEngine&) = delete;
    PredictionEngine& operator=(const PredictionEngine&) = delete;
    PredictionEngine(PredictionEngine&&) = delete;
    PredictionEngine& operator=(PredictionEngine&&) = delete;

    /**
     * 初始化预测引擎
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * 关闭预测引擎
     */
    void shutdown();

    /**
     * 基于上下文预测下一个词
     * @param context 上下文文本
     * @param max_predictions 最大预测数量
     * @return 预测候选词列表
     */
    CandidateList predictNext(const std::string& context, int max_predictions = 5) const;

    /**
     * 基于部分输入预测完整词汇
     * @param partial_input 部分输入
     * @param context 上下文
     * @param max_predictions 最大预测数量
     * @return 预测候选词列表
     */
    CandidateList predictCompletion(
        const std::string& partial_input,
        const std::string& context = "",
        int max_predictions = 5
    ) const;

    /**
     * 基于拼音预测词汇
     * @param pinyin 拼音输入
     * @param context 上下文
     * @param max_predictions 最大预测数量
     * @return 预测候选词列表
     */
    CandidateList predictFromPinyin(
        const std::string& pinyin,
        const std::string& context = "",
        int max_predictions = 5
    ) const;

    /**
     * 学习用户输入模式
     * @param input_sequence 输入序列
     * @param context 上下文
     * @return 是否学习成功
     */
    bool learnInputPattern(const std::vector<std::string>& input_sequence, const std::string& context = "");

    /**
     * 更新预测模型
     * @param training_data 训练数据
     * @return 是否更新成功
     */
    bool updateModel(const std::vector<std::string>& training_data);

    /**
     * 设置预测阈值
     * @param threshold 阈值（0.0-1.0）
     */
    void setPredictionThreshold(double threshold);

    /**
     * 获取预测阈值
     * @return 当前阈值
     */
    double getPredictionThreshold() const;

    /**
     * 检查预测引擎是否可用
     * @return 是否可用
     */
    bool isAvailable() const;

    /**
     * 获取模型信息
     * @return 模型信息字符串
     */
    std::string getModelInfo() const;

private:
    /**
     * 加载预测模型
     * @return 是否加载成功
     */
    bool loadModel();

    /**
     * 预处理输入文本
     * @param text 原始文本
     * @return 预处理后的文本
     */
    std::string preprocessText(const std::string& text) const;

    /**
     * 后处理预测结果
     * @param raw_predictions 原始预测结果
     * @return 处理后的候选词列表
     */
    CandidateList postprocessPredictions(const std::vector<std::string>& raw_predictions) const;

private:
    std::unique_ptr<LlamaPredictor> llama_predictor_;
    std::string model_path_;
    double prediction_threshold_;
    bool is_initialized_;
};

} // namespace core
} // namespace owcat