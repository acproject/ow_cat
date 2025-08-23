#include "core/prediction_engine.h"
#include "core/llama_predictor.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <fstream>

namespace owcat {
namespace core {

class PredictionEngine::Impl {
public:
    explicit Impl(const std::string& model_path)
        : model_path_(model_path), prediction_threshold_(0.5), initialized_(false) {
        llama_predictor_ = std::make_unique<LlamaPredictor>();
    }
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize() {
        spdlog::info("Initializing prediction engine with model: {}", model_path_);
        
        if (model_path_.empty()) {
            spdlog::warn("No model path specified, prediction engine will be disabled");
            return true; // 允许在没有模型的情况下运行
        }
        
        // 检查模型文件是否存在
        std::ifstream model_file(model_path_);
        if (!model_file.good()) {
            spdlog::warn("Model file not found: {}, prediction engine will be disabled", model_path_);
            return true; // 允许在没有模型的情况下运行
        }
        
        // 初始化LlamaPredictor
        if (!llama_predictor_->initialize(model_path_)) {
            spdlog::error("Failed to initialize llama predictor");
            return false;
        }
        
        initialized_ = true;
        spdlog::info("Prediction engine initialized successfully");
        return true;
    }
    
    void shutdown() {
        if (llama_predictor_) {
            llama_predictor_->shutdown();
        }
        initialized_ = false;
    }
    
    CandidateList predictNextWords(const std::string& context, int max_predictions) {
        CandidateList predictions;
        
        if (!isAvailable()) {
            return predictions;
        }
        
        try {
            // 使用LlamaPredictor生成预测
            std::string generated_text = llama_predictor_->generateText(context, max_predictions * 10);
            
            if (generated_text.empty()) {
                return predictions;
            }
            
            // 解析生成的文本，提取候选词
            auto words = parseGeneratedText(generated_text, context);
            
            // 计算每个词的概率分数
            for (const auto& word : words) {
                if (predictions.size() >= static_cast<size_t>(max_predictions)) {
                    break;
                }
                
                double probability = llama_predictor_->getNextWordProbability(context, word);
                if (probability >= prediction_threshold_) {
                    // 生成拼音（简化实现，实际应该使用拼音转换器）
                    std::string pinyin = generatePinyin(word);
                    predictions.emplace_back(word, pinyin, probability, 0, true);
                }
            }
            
            // 按概率排序
            std::sort(predictions.begin(), predictions.end(), 
                     [](const Candidate& a, const Candidate& b) {
                         return a.score > b.score;
                     });
                     
        } catch (const std::exception& e) {
            spdlog::error("Error in predictNextWords: {}", e.what());
        }
        
        return predictions;
    }
    
    CandidateList completePartialInput(const std::string& partial_text, int max_completions) {
        CandidateList completions;
        
        if (!isAvailable() || partial_text.empty()) {
            return completions;
        }
        
        try {
            // 构建提示文本
            std::string prompt = "请补全以下文本：" + partial_text;
            std::string generated_text = llama_predictor_->generateText(prompt, max_completions * 20);
            
            if (generated_text.empty()) {
                return completions;
            }
            
            // 解析补全结果
            auto completions_text = parseCompletions(generated_text, partial_text);
            
            for (const auto& completion : completions_text) {
                if (completions.size() >= static_cast<size_t>(max_completions)) {
                    break;
                }
                
                double score = calculateCompletionScore(partial_text, completion);
                if (score >= prediction_threshold_) {
                    std::string pinyin = generatePinyin(completion);
                    completions.emplace_back(completion, pinyin, score, 0, true);
                }
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Error in completePartialInput: {}", e.what());
        }
        
        return completions;
    }
    
    CandidateList predictFromPinyin(const std::string& pinyin_sequence, const std::string& context, int max_predictions) {
        CandidateList predictions;
        
        if (!isAvailable()) {
            return predictions;
        }
        
        try {
            // 构建包含拼音信息的提示
            std::string prompt = "根据拼音'" + pinyin_sequence + "'和上下文'" + context + "'，预测可能的中文词汇：";
            std::string generated_text = llama_predictor_->generateText(prompt, max_predictions * 15);
            
            if (generated_text.empty()) {
                return predictions;
            }
            
            // 解析预测结果
            auto predicted_words = parsePinyinPredictions(generated_text, pinyin_sequence);
            
            for (const auto& word : predicted_words) {
                if (predictions.size() >= static_cast<size_t>(max_predictions)) {
                    break;
                }
                
                double score = calculatePinyinScore(word, pinyin_sequence, context);
                if (score >= prediction_threshold_) {
                    predictions.emplace_back(word, pinyin_sequence, score, 0, true);
                }
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Error in predictFromPinyin: {}", e.what());
        }
        
        return predictions;
    }
    
    void learnUserPattern(const std::string& input_sequence, const std::string& selected_text) {
        if (!isAvailable()) {
            return;
        }
        
        // 记录用户选择模式，用于后续优化
        user_patterns_[input_sequence].push_back(selected_text);
        
        // 限制存储的模式数量
        if (user_patterns_.size() > 1000) {
            // 删除最旧的模式
            auto it = user_patterns_.begin();
            user_patterns_.erase(it);
        }
        
        spdlog::debug("Learned user pattern: {} -> {}", input_sequence, selected_text);
    }
    
    bool updateModel(const std::string& new_model_path) {
        if (new_model_path == model_path_) {
            return true; // 相同路径，无需更新
        }
        
        // 检查新模型文件是否存在
        std::ifstream model_file(new_model_path);
        if (!model_file.good()) {
            spdlog::error("New model file not found: {}", new_model_path);
            return false;
        }
        
        // 关闭当前模型
        if (initialized_) {
            llama_predictor_->shutdown();
            initialized_ = false;
        }
        
        // 更新模型路径
        model_path_ = new_model_path;
        
        // 重新初始化
        return initialize();
    }
    
    void setPredictionThreshold(double threshold) {
        prediction_threshold_ = std::max(0.0, std::min(1.0, threshold));
        spdlog::info("Prediction threshold set to: {}", prediction_threshold_);
    }
    
    double getPredictionThreshold() const {
        return prediction_threshold_;
    }
    
    bool isAvailable() const {
        return initialized_ && llama_predictor_ && llama_predictor_->isModelLoaded();
    }
    
    std::string getModelInfo() const {
        if (!isAvailable()) {
            return "Model not loaded";
        }
        
        return llama_predictor_->getModelInfo();
    }
    
private:
    std::vector<std::string> parseGeneratedText(const std::string& generated_text, const std::string& context) {
        std::vector<std::string> words;
        
        // 简化实现：按空格和标点分割文本
        std::string current_word;
        for (char c : generated_text) {
            if (std::isspace(c) || std::ispunct(c)) {
                if (!current_word.empty()) {
                    words.push_back(current_word);
                    current_word.clear();
                }
            } else {
                current_word += c;
            }
        }
        
        if (!current_word.empty()) {
            words.push_back(current_word);
        }
        
        // 过滤掉与上下文重复的词
        words.erase(std::remove_if(words.begin(), words.end(),
                                  [&context](const std::string& word) {
                                      return context.find(word) != std::string::npos;
                                  }), words.end());
        
        return words;
    }
    
    std::vector<std::string> parseCompletions(const std::string& generated_text, const std::string& partial_text) {
        std::vector<std::string> completions;
        
        // 查找包含部分文本的完整词汇
        std::istringstream iss(generated_text);
        std::string word;
        
        while (iss >> word) {
            if (word.find(partial_text) == 0 && word.length() > partial_text.length()) {
                completions.push_back(word);
            }
        }
        
        // 去重
        std::sort(completions.begin(), completions.end());
        completions.erase(std::unique(completions.begin(), completions.end()), completions.end());
        
        return completions;
    }
    
    std::vector<std::string> parsePinyinPredictions(const std::string& generated_text, const std::string& pinyin_sequence) {
        std::vector<std::string> predictions;
        
        // 简化实现：提取中文字符序列
        std::string current_word;
        for (size_t i = 0; i < generated_text.length(); ) {
            unsigned char c = generated_text[i];
            
            // 检查是否为中文字符（UTF-8编码）
            if ((c & 0x80) && i + 2 < generated_text.length()) {
                current_word += generated_text.substr(i, 3);
                i += 3;
            } else {
                if (!current_word.empty()) {
                    predictions.push_back(current_word);
                    current_word.clear();
                }
                i++;
            }
        }
        
        if (!current_word.empty()) {
            predictions.push_back(current_word);
        }
        
        return predictions;
    }
    
    double calculateCompletionScore(const std::string& partial_text, const std::string& completion) {
        if (completion.length() <= partial_text.length()) {
            return 0.0;
        }
        
        // 基于长度和相似度计算分数
        double length_score = 1.0 / (completion.length() - partial_text.length() + 1);
        double base_score = 0.7; // 基础分数
        
        return base_score + length_score * 0.3;
    }
    
    double calculatePinyinScore(const std::string& word, const std::string& pinyin_sequence, const std::string& context) {
        double base_score = 0.6;
        
        // 检查用户历史模式
        auto it = user_patterns_.find(pinyin_sequence);
        if (it != user_patterns_.end()) {
            for (const auto& pattern : it->second) {
                if (pattern == word) {
                    base_score += 0.3; // 用户历史选择加分
                    break;
                }
            }
        }
        
        // 上下文相关性加分（简化实现）
        if (!context.empty() && context.find(word) == std::string::npos) {
            base_score += 0.1;
        }
        
        return std::min(1.0, base_score);
    }
    
    std::string generatePinyin(const std::string& word) {
        // 简化实现：返回占位符拼音
        // 实际应该使用专门的拼音转换库
        return "pinyin";
    }
    
public:
    std::string model_path_;
    std::unique_ptr<LlamaPredictor> llama_predictor_;
    double prediction_threshold_;
    bool initialized_;
    std::unordered_map<std::string, std::vector<std::string>> user_patterns_;
};

// PredictionEngine implementation
PredictionEngine::PredictionEngine(const std::string& model_path)
    : pImpl(std::make_unique<Impl>(model_path)) {
}

PredictionEngine::~PredictionEngine() = default;

bool PredictionEngine::initialize() {
    return pImpl->initialize();
}

void PredictionEngine::shutdown() {
    pImpl->shutdown();
}

CandidateList PredictionEngine::predictNextWords(const std::string& context, int max_predictions) {
    return pImpl->predictNextWords(context, max_predictions);
}

CandidateList PredictionEngine::completePartialInput(const std::string& partial_text, int max_completions) {
    return pImpl->completePartialInput(partial_text, max_completions);
}

CandidateList PredictionEngine::predictFromPinyin(const std::string& pinyin_sequence, const std::string& context, int max_predictions) {
    return pImpl->predictFromPinyin(pinyin_sequence, context, max_predictions);
}

void PredictionEngine::learnUserPattern(const std::string& input_sequence, const std::string& selected_text) {
    pImpl->learnUserPattern(input_sequence, selected_text);
}

bool PredictionEngine::updateModel(const std::string& new_model_path) {
    return pImpl->updateModel(new_model_path);
}

void PredictionEngine::setPredictionThreshold(double threshold) {
    pImpl->setPredictionThreshold(threshold);
}

double PredictionEngine::getPredictionThreshold() const {
    return pImpl->getPredictionThreshold();
}

bool PredictionEngine::isAvailable() const {
    return pImpl->isAvailable();
}

std::string PredictionEngine::getModelInfo() const {
    return pImpl->getModelInfo();
}

} // namespace core
} // namespace owcat