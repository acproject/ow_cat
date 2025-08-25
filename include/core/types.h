#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace owcat {
namespace core {

// 候选词结构
struct Candidate {
    std::string text;           // 候选词文本
    std::string pinyin;         // 拼音
    double score;               // 得分
    int frequency;              // 使用频率
    bool is_prediction;         // 是否为AI预测结果
    
    // 默认构造函数
    Candidate() : text(""), pinyin(""), score(0.0), frequency(0), is_prediction(false) {}
    
    Candidate(const std::string& t, const std::string& p, double s = 0.0, int f = 0, bool pred = false)
        : text(t), pinyin(p), score(s), frequency(f), is_prediction(pred) {}
};

// 候选词列表
using CandidateList = std::vector<Candidate>;

// 输入状态
enum class InputState {
    IDLE,           // 空闲状态
    COMPOSING,      // 输入中
    SELECTING       // 选择候选词
};

// 输入事件类型
enum class InputEventType {
    KEY_PRESS,
    KEY_RELEASE,
    CANDIDATE_SELECT,
    COMMIT_TEXT,
    CLEAR_COMPOSITION
};

// 输入事件
struct InputEvent {
    InputEventType type;
    std::string data;
    int key_code;
    bool ctrl;
    bool shift;
    bool alt;
    
    InputEvent(InputEventType t, const std::string& d = "", int k = 0)
        : type(t), data(d), key_code(k), ctrl(false), shift(false), alt(false) {}
};

// 回调函数类型
using CandidateCallback = std::function<void(const CandidateList&)>;
using CommitCallback = std::function<void(const std::string&)>;
using StateChangeCallback = std::function<void(InputState)>;

// 配置选项
struct EngineConfig {
    std::string dictionary_path = "data/dictionary.db";
    std::string model_path = "models/qwen0.6b.gguf";
    int max_candidates = 9;
    bool enable_prediction = true;
    bool enable_learning = true;
    double prediction_threshold = 0.5;
    
    // 平台特定配置
    struct {
        bool use_system_ime = true;
        std::string layout = "qwerty";
    } platform;
};

} // namespace core
} // namespace owcat