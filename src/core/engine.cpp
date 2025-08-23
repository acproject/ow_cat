#include "core/engine.h"
#include "core/pinyin_converter.h"
#include "core/dictionary_manager.h"
#include "core/prediction_engine.h"
#include <spdlog/spdlog.h>
#include <memory>
#include <algorithm>

namespace owcat {
namespace core {

class Engine::Impl {
public:
    explicit Impl(const EngineConfig& config)
        : config_(config)
        , state_(InputState::IDLE)
        , pinyin_converter_(std::make_unique<PinyinConverter>())
        , dictionary_manager_(std::make_unique<DictionaryManager>(config.dictionary_path))
        , prediction_engine_(config.enable_prediction ? 
            std::make_unique<PredictionEngine>(config.model_path) : nullptr)
    {
    }

    bool initialize() {
        spdlog::info("Initializing input method engine...");

        // 初始化拼音转换器
        if (!pinyin_converter_->initialize()) {
            spdlog::error("Failed to initialize pinyin converter");
            return false;
        }

        // 初始化词库管理器
        if (!dictionary_manager_->initialize()) {
            spdlog::error("Failed to initialize dictionary manager");
            return false;
        }

        // 初始化预测引擎（如果启用）
        if (prediction_engine_ && !prediction_engine_->initialize()) {
            spdlog::warn("Failed to initialize prediction engine, continuing without AI prediction");
            prediction_engine_.reset();
        }

        spdlog::info("Input method engine initialized successfully");
        return true;
    }

    void shutdown() {
        spdlog::info("Shutting down input method engine...");
        
        if (prediction_engine_) {
            prediction_engine_->shutdown();
        }
        
        if (dictionary_manager_) {
            dictionary_manager_->shutdown();
        }
        
        clearComposition();
        spdlog::info("Input method engine shut down");
    }

    bool processInput(const InputEvent& event) {
        switch (event.type) {
            case InputEventType::KEY_PRESS:
                return handleKeyPress(event);
            case InputEventType::CANDIDATE_SELECT:
                return handleCandidateSelect(event);
            case InputEventType::COMMIT_TEXT:
                return handleCommitText(event);
            case InputEventType::CLEAR_COMPOSITION:
                clearComposition();
                return true;
            default:
                return false;
        }
    }

    bool handleKeyPress(const InputEvent& event) {
        char ch = static_cast<char>(event.key_code);
        
        // 处理特殊按键
        if (event.key_code == 8) { // Backspace
            if (!composition_.empty()) {
                pinyin_converter_->removeLastChar();
                updateCandidates();
                return true;
            }
            return false;
        }
        
        if (event.key_code == 27) { // Escape
            clearComposition();
            return true;
        }
        
        if (event.key_code == 13) { // Enter
            if (!composition_.empty()) {
                commitComposition();
                return true;
            }
            return false;
        }
        
        // 处理数字键选择候选词
        if (ch >= '1' && ch <= '9') {
            int index = ch - '1';
            if (index < static_cast<int>(candidates_.size())) {
                selectCandidate(index);
                return true;
            }
        }
        
        // 处理字母输入
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            ch = std::tolower(ch);
            if (pinyin_converter_->addChar(ch)) {
                updateCandidates();
                setState(InputState::COMPOSING);
                return true;
            }
        }
        
        return false;
    }

    bool handleCandidateSelect(const InputEvent& event) {
        try {
            int index = std::stoi(event.data);
            return selectCandidate(index);
        } catch (const std::exception&) {
            return false;
        }
    }

    bool handleCommitText(const InputEvent& event) {
        if (commit_callback_) {
            commit_callback_(event.data);
        }
        clearComposition();
        return true;
    }

    bool selectCandidate(int index) {
        if (index < 0 || index >= static_cast<int>(candidates_.size())) {
            return false;
        }
        
        const auto& candidate = candidates_[index];
        
        // 学习用户选择
        if (config_.enable_learning) {
            dictionary_manager_->updateWordFrequency(candidate.text, candidate.pinyin);
        }
        
        // 提交选择的候选词
        if (commit_callback_) {
            commit_callback_(candidate.text);
        }
        
        clearComposition();
        return true;
    }

    std::string commitComposition() {
        std::string result = composition_;
        if (commit_callback_ && !result.empty()) {
            commit_callback_(result);
        }
        clearComposition();
        return result;
    }

    void clearComposition() {
        composition_.clear();
        candidates_.clear();
        pinyin_converter_->clear();
        setState(InputState::IDLE);
        
        if (candidate_callback_) {
            candidate_callback_(candidates_);
        }
    }

    void updateCandidates() {
        composition_ = pinyin_converter_->getCurrentPinyin();
        candidates_.clear();
        
        if (composition_.empty()) {
            setState(InputState::IDLE);
            if (candidate_callback_) {
                candidate_callback_(candidates_);
            }
            return;
        }
        
        // 从词库获取候选词
        auto dict_candidates = dictionary_manager_->searchByPinyin(composition_, config_.max_candidates);
        candidates_.insert(candidates_.end(), dict_candidates.begin(), dict_candidates.end());
        
        // 如果启用AI预测，添加预测结果
        if (prediction_engine_ && prediction_engine_->isAvailable()) {
            auto predicted_candidates = prediction_engine_->predictFromPinyin(
                composition_, "", 
                std::max(1, config_.max_candidates - static_cast<int>(candidates_.size()))
            );
            
            // 过滤重复候选词
            for (const auto& pred_candidate : predicted_candidates) {
                bool duplicate = false;
                for (const auto& existing : candidates_) {
                    if (existing.text == pred_candidate.text) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate && pred_candidate.score >= config_.prediction_threshold) {
                    candidates_.push_back(pred_candidate);
                }
            }
        }
        
        // 按得分排序
        std::sort(candidates_.begin(), candidates_.end(), 
                  [](const Candidate& a, const Candidate& b) {
                      return a.score > b.score;
                  });
        
        // 限制候选词数量
        if (candidates_.size() > static_cast<size_t>(config_.max_candidates)) {
            candidates_.resize(config_.max_candidates);
        }
        
        setState(InputState::SELECTING);
        
        if (candidate_callback_) {
            candidate_callback_(candidates_);
        }
    }

    void setState(InputState new_state) {
        if (state_ != new_state) {
            state_ = new_state;
            if (state_change_callback_) {
                state_change_callback_(state_);
            }
        }
    }

public:
    EngineConfig config_;
    InputState state_;
    std::string composition_;
    CandidateList candidates_;
    
    std::unique_ptr<PinyinConverter> pinyin_converter_;
    std::unique_ptr<DictionaryManager> dictionary_manager_;
    std::unique_ptr<PredictionEngine> prediction_engine_;
    
    CandidateCallback candidate_callback_;
    CommitCallback commit_callback_;
    StateChangeCallback state_change_callback_;
};

// Engine implementation
Engine::Engine(const EngineConfig& config)
    : pImpl(std::make_unique<Impl>(config))
{
}

Engine::~Engine() = default;

bool Engine::initialize() {
    return pImpl->initialize();
}

void Engine::shutdown() {
    pImpl->shutdown();
}

bool Engine::processInput(const InputEvent& event) {
    return pImpl->processInput(event);
}

const CandidateList& Engine::getCandidates() const {
    return pImpl->candidates_;
}

const std::string& Engine::getComposition() const {
    return pImpl->composition_;
}

InputState Engine::getState() const {
    return pImpl->state_;
}

bool Engine::selectCandidate(int index) {
    return pImpl->selectCandidate(index);
}

std::string Engine::commitComposition() {
    return pImpl->commitComposition();
}

void Engine::clearComposition() {
    pImpl->clearComposition();
}

void Engine::setCandidateCallback(CandidateCallback callback) {
    pImpl->candidate_callback_ = std::move(callback);
}

void Engine::setCommitCallback(CommitCallback callback) {
    pImpl->commit_callback_ = std::move(callback);
}

void Engine::setStateChangeCallback(StateChangeCallback callback) {
    pImpl->state_change_callback_ = std::move(callback);
}

void Engine::updateConfig(const EngineConfig& config) {
    pImpl->config_ = config;
}

const EngineConfig& Engine::getConfig() const {
    return pImpl->config_;
}

} // namespace core
} // namespace owcat