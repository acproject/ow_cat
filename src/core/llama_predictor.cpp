#include "core/llama_predictor.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef ENABLE_LLAMA_CPP
#include <llama.h>
#endif

namespace owcat {
namespace core {

// 内部使用的生成参数结构体
struct GenerationParams {
    float temperature = 0.7f;
    float top_p = 0.9f;
    int top_k = 40;
};

#ifdef ENABLE_LLAMA_CPP
class LlamaPredictor::Impl {
public:
    Impl() : model_(nullptr), ctx_(nullptr), model_loaded_(false) {
        // 初始化llama.cpp
        llama_backend_init(false);
    }
    
    ~Impl() {
        unloadModel();
        llama_backend_free();
    }
    
    bool initialize(const std::string& model_path) {
        spdlog::info("Initializing LlamaPredictor with model: {}", model_path);
        
        model_path_ = model_path;
        
        // 检查模型文件是否存在
        std::ifstream file(model_path);
        if (!file.good()) {
            spdlog::error("Model file not found: {}", model_path);
            return false;
        }
        
        // 加载模型
        if (!loadModel()) {
            spdlog::error("Failed to load model: {}", model_path);
            return false;
        }
        
        // 预热模型
        warmupModel();
        
        spdlog::info("LlamaPredictor initialized successfully");
        return true;
    }
    
    void shutdown() {
        unloadModel();
    }
    
    bool loadModel() {
        // 设置模型参数
        llama_model_params model_params = llama_model_default_params();
        model_params.n_gpu_layers = 0; // CPU only for compatibility
        model_params.use_mmap = true;
        model_params.use_mlock = false;
        
        // 加载模型
        model_ = llama_load_model_from_file(model_path_.c_str(), model_params);
        if (!model_) {
            spdlog::error("Failed to load model from: {}", model_path_);
            return false;
        }
        
        // 创建上下文
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.seed = -1; // random seed
        ctx_params.n_ctx = 2048; // context size
        ctx_params.n_threads = std::thread::hardware_concurrency();
        ctx_params.n_threads_batch = ctx_params.n_threads;
        
        ctx_ = llama_new_context_with_model(model_, ctx_params);
        if (!ctx_) {
            spdlog::error("Failed to create llama context");
            llama_free_model(model_);
            model_ = nullptr;
            return false;
        }
        
        model_loaded_ = true;
        return true;
    }
    
    void unloadModel() {
        if (ctx_) {
            llama_free(ctx_);
            ctx_ = nullptr;
        }
        
        if (model_) {
            llama_free_model(model_);
            model_ = nullptr;
        }
        
        model_loaded_ = false;
    }
    
    std::string generateText(const std::string& prompt, int max_tokens) {
        if (!model_loaded_) {
            spdlog::warn("Model not loaded, cannot generate text");
            return "";
        }
        
        try {
            // 预处理输入
            std::string processed_prompt = preprocessInput(prompt);
            
            // 将文本转换为tokens
            std::vector<llama_token> tokens = tokenize(processed_prompt);
            if (tokens.empty()) {
                spdlog::warn("Failed to tokenize prompt: {}", processed_prompt);
                return "";
            }
            
            // 生成文本
            std::vector<llama_token> generated_tokens;
            
            // 评估prompt tokens
            if (llama_decode(ctx_, llama_batch_get_one(tokens.data(), tokens.size(), 0, 0)) != 0) {
                spdlog::error("Failed to decode prompt tokens");
                return "";
            }
            
            // 生成新的tokens
            for (int i = 0; i < max_tokens; ++i) {
                llama_token next_token = sampleNextToken();
                
                if (next_token == llama_token_eos(model_)) {
                    break; // 遇到结束符
                }
                
                generated_tokens.push_back(next_token);
                
                // 将新token添加到上下文
                if (llama_decode(ctx_, llama_batch_get_one(&next_token, 1, tokens.size() + i, 0)) != 0) {
                    spdlog::warn("Failed to decode generated token at position {}", i);
                    break;
                }
            }
            
            // 将tokens转换为文本
            std::string generated_text = detokenize(generated_tokens);
            
            // 后处理输出
            return postprocessOutput(generated_text);
            
        } catch (const std::exception& e) {
            spdlog::error("Error in generateText: {}", e.what());
            return "";
        }
    }
    
    double getNextWordProbability(const std::string& context, const std::string& word) {
        if (!model_loaded_) {
            return 0.0;
        }
        
        try {
            // 将上下文和词汇转换为tokens
            std::vector<llama_token> context_tokens = tokenize(context);
            std::vector<llama_token> word_tokens = tokenize(word);
            
            if (context_tokens.empty() || word_tokens.empty()) {
                return 0.0;
            }
            
            // 评估上下文
            if (llama_decode(ctx_, llama_batch_get_one(context_tokens.data(), context_tokens.size(), 0, 0)) != 0) {
                return 0.0;
            }
            
            // 计算词汇的概率
            double total_prob = 1.0;
            for (size_t i = 0; i < word_tokens.size(); ++i) {
                float* logits = llama_get_logits_ith(ctx_, -1);
                if (!logits) {
                    return 0.0;
                }
                
                // 应用softmax获取概率分布
                int vocab_size = llama_n_vocab(model_);
                std::vector<float> probs(vocab_size);
                softmax(logits, probs.data(), vocab_size);
                
                // 获取当前token的概率
                if (word_tokens[i] < vocab_size) {
                    total_prob *= probs[word_tokens[i]];
                } else {
                    return 0.0;
                }
                
                // 如果不是最后一个token，继续解码
                if (i < word_tokens.size() - 1) {
                    if (llama_decode(ctx_, llama_batch_get_one(&word_tokens[i], 1, context_tokens.size() + i, 0)) != 0) {
                        return 0.0;
                    }
                }
            }
            
            return total_prob;
            
        } catch (const std::exception& e) {
            spdlog::error("Error in getNextWordProbability: {}", e.what());
            return 0.0;
        }
    }
    
    double calculatePerplexity(const std::string& text) {
        if (!model_loaded_) {
            return std::numeric_limits<double>::infinity();
        }
        
        try {
            std::vector<llama_token> tokens = tokenize(text);
            if (tokens.size() < 2) {
                return std::numeric_limits<double>::infinity();
            }
            
            double log_likelihood = 0.0;
            int token_count = 0;
            
            // 计算每个token的对数似然
            for (size_t i = 1; i < tokens.size(); ++i) {
                // 评估前i个tokens
                if (llama_decode(ctx_, llama_batch_get_one(tokens.data(), i, 0, 0)) != 0) {
                    continue;
                }
                
                float* logits = llama_get_logits_ith(ctx_, -1);
                if (!logits) {
                    continue;
                }
                
                // 计算softmax
                int vocab_size = llama_n_vocab(model_);
                std::vector<float> probs(vocab_size);
                softmax(logits, probs.data(), vocab_size);
                
                // 添加当前token的对数概率
                if (tokens[i] < vocab_size && probs[tokens[i]] > 0) {
                    log_likelihood += std::log(probs[tokens[i]]);
                    token_count++;
                }
            }
            
            if (token_count == 0) {
                return std::numeric_limits<double>::infinity();
            }
            
            // 计算困惑度
            double avg_log_likelihood = log_likelihood / token_count;
            return std::exp(-avg_log_likelihood);
            
        } catch (const std::exception& e) {
            spdlog::error("Error in calculatePerplexity: {}", e.what());
            return std::numeric_limits<double>::infinity();
        }
    }
    
    bool isModelLoaded() const {
        return model_loaded_;
    }
    
    std::string getModelInfo() const {
        if (!model_loaded_) {
            return "Model not loaded";
        }
        
        std::stringstream ss;
        ss << "Model: " << model_path_ << "\n";
        ss << "Context size: " << llama_n_ctx(ctx_) << "\n";
        ss << "Vocabulary size: " << llama_n_vocab(model_) << "\n";
        ss << "Embedding size: " << llama_n_embd(model_);
        
        return ss.str();
    }
    
    void setGenerationParams(const GenerationParams& params) {
        generation_params_ = params;
    }
    
    GenerationParams getGenerationParams() const {
        return generation_params_;
    }
    
    void warmupModel() {
        if (!model_loaded_) {
            return;
        }
        
        spdlog::info("Warming up model...");
        
        // 使用简单的提示进行预热
        std::string warmup_prompt = "你好";
        generateText(warmup_prompt, 5);
        
        spdlog::info("Model warmup completed");
    }
    
private:
    std::vector<llama_token> tokenize(const std::string& text) {
        if (!model_) {
            return {};
        }
        
        std::vector<llama_token> tokens;
        tokens.resize(text.length() + 1);
        
        int n_tokens = llama_tokenize(model_, text.c_str(), text.length(), tokens.data(), tokens.size(), false, false);
        
        if (n_tokens < 0) {
            tokens.resize(-n_tokens);
            n_tokens = llama_tokenize(model_, text.c_str(), text.length(), tokens.data(), tokens.size(), false, false);
        }
        
        if (n_tokens >= 0) {
            tokens.resize(n_tokens);
        } else {
            tokens.clear();
        }
        
        return tokens;
    }
    
    std::string detokenize(const std::vector<llama_token>& tokens) {
        if (!model_ || tokens.empty()) {
            return "";
        }
        
        std::string result;
        for (llama_token token : tokens) {
            char buffer[256];
            int length = llama_token_to_piece(model_, token, buffer, sizeof(buffer));
            if (length > 0) {
                result.append(buffer, length);
            }
        }
        
        return result;
    }
    
    llama_token sampleNextToken() {
        if (!ctx_) {
            return 0;
        }
        
        float* logits = llama_get_logits_ith(ctx_, -1);
        if (!logits) {
            return 0;
        }
        
        int vocab_size = llama_n_vocab(model_);
        
        // 应用温度采样
        if (generation_params_.temperature > 0) {
            for (int i = 0; i < vocab_size; ++i) {
                logits[i] /= generation_params_.temperature;
            }
        }
        
        // 创建候选token列表
        std::vector<llama_token_data> candidates;
        candidates.reserve(vocab_size);
        
        for (int i = 0; i < vocab_size; ++i) {
            candidates.push_back({i, logits[i], 0.0f});
        }
        
        llama_token_data_array candidates_p = {candidates.data(), candidates.size(), false};
        
        // 应用top-k采样
        if (generation_params_.top_k > 0) {
            llama_sample_top_k(ctx_, &candidates_p, generation_params_.top_k, 1);
        }
        
        // 应用top-p采样
        if (generation_params_.top_p < 1.0f) {
            llama_sample_top_p(ctx_, &candidates_p, generation_params_.top_p, 1);
        }
        
        // 采样token
        return llama_sample_token(ctx_, &candidates_p);
    }
    
    void softmax(const float* input, float* output, int size) {
        float max_val = *std::max_element(input, input + size);
        float sum = 0.0f;
        
        for (int i = 0; i < size; ++i) {
            output[i] = std::exp(input[i] - max_val);
            sum += output[i];
        }
        
        for (int i = 0; i < size; ++i) {
            output[i] /= sum;
        }
    }
    
    std::string preprocessInput(const std::string& input) {
        // 简单的预处理：去除多余空格
        std::string result = input;
        
        // 去除首尾空格
        result.erase(0, result.find_first_not_of(" \t\n\r"));
        result.erase(result.find_last_not_of(" \t\n\r") + 1);
        
        // 替换多个连续空格为单个空格
        std::string::size_type pos = 0;
        while ((pos = result.find("  ", pos)) != std::string::npos) {
            result.replace(pos, 2, " ");
        }
        
        return result;
    }
    
    std::string postprocessOutput(const std::string& output) {
        // 简单的后处理：清理输出
        std::string result = output;
        
        // 去除首尾空格
        result.erase(0, result.find_first_not_of(" \t\n\r"));
        result.erase(result.find_last_not_of(" \t\n\r") + 1);
        
        return result;
    }
    
public:
    std::string model_path_;
    llama_model* model_;
    llama_context* ctx_;
    bool model_loaded_;
    GenerationParams generation_params_;
};

#else
// 没有llama.cpp支持的空实现
class LlamaPredictor::Impl {
public:
    Impl() : model_loaded_(false) {}
    ~Impl() = default;
    
    bool initialize(const std::string& model_path) {
        spdlog::warn("LlamaPredictor: llama.cpp support not enabled, using dummy implementation");
        model_path_ = model_path;
        return true;
    }
    
    void shutdown() {}
    
    std::string generateText(const std::string& prompt, int max_tokens) {
        spdlog::debug("LlamaPredictor: generateText called (dummy implementation)");
        return "";
    }
    
    double getNextWordProbability(const std::string& context, const std::string& word) {
        return 0.0;
    }
    
    double calculatePerplexity(const std::string& text) {
        return std::numeric_limits<double>::infinity();
    }
    
    bool isModelLoaded() const {
        return false;
    }
    
    std::string getModelInfo() const {
        return "llama.cpp support not enabled";
    }
    
    void setGenerationParams(const GenerationParams& params) {
        generation_params_ = params;
    }
    
    GenerationParams getGenerationParams() const {
        return generation_params_;
    }
    
    void warmupModel() {}
    
public:
    std::string model_path_;
    bool model_loaded_;
    GenerationParams generation_params_;
};
#endif

// LlamaPredictor implementation
LlamaPredictor::LlamaPredictor(const std::string& model_path)
    : pImpl(std::make_unique<Impl>()) {
    pImpl->initialize(model_path);
}

LlamaPredictor::~LlamaPredictor() = default;

bool LlamaPredictor::initialize() {
    return true; // 已在构造函数中初始化
}

void LlamaPredictor::shutdown() {
    pImpl->shutdown();
}

std::vector<std::string> LlamaPredictor::generateText(
    const std::string& prompt,
    int max_tokens,
    float temperature,
    float top_p
) const {
    std::string result = pImpl->generateText(prompt, max_tokens);
    return {result}; // 将单个结果包装成vector
}

std::vector<float> LlamaPredictor::getNextWordProbabilities(
    const std::string& context,
    const std::vector<std::string>& candidates
) const {
    std::vector<float> probabilities;
    for (const auto& candidate : candidates) {
        probabilities.push_back(static_cast<float>(pImpl->getNextWordProbability(context, candidate)));
    }
    return probabilities;
}

float LlamaPredictor::calculatePerplexity(const std::string& text) const {
    return static_cast<float>(pImpl->calculatePerplexity(text));
}

bool LlamaPredictor::isLoaded() const {
    return pImpl->isModelLoaded();
}

std::string LlamaPredictor::getModelInfo() const {
    return pImpl->getModelInfo();
}

void LlamaPredictor::setGenerationParams(float temperature, float top_p, int top_k) {
    // 简化实现，暂时不使用内部的GenerationParams
}

bool LlamaPredictor::warmup() {
    // 简化实现
    return true;
}

// 移除了不匹配的方法实现

} // namespace core
} // namespace owcat