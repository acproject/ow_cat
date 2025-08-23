#include "core/pinyin_converter.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace owcat {
namespace core {

// 标准拼音表
static const std::vector<std::string> STANDARD_PINYINS = {
    "a", "ai", "an", "ang", "ao",
    "ba", "bai", "ban", "bang", "bao", "bei", "ben", "beng", "bi", "bian", "biao", "bie", "bin", "bing", "bo", "bu",
    "ca", "cai", "can", "cang", "cao", "ce", "cen", "ceng", "cha", "chai", "chan", "chang", "chao", "che", "chen", "cheng", "chi", "chong", "chou", "chu", "chuai", "chuan", "chuang", "chui", "chun", "chuo", "ci", "cong", "cou", "cu", "cuan", "cui", "cun", "cuo",
    "da", "dai", "dan", "dang", "dao", "de", "deng", "di", "dian", "diao", "die", "ding", "diu", "dong", "dou", "du", "duan", "dui", "dun", "duo",
    "e", "en", "er",
    "fa", "fan", "fang", "fei", "fen", "feng", "fo", "fou", "fu",
    "ga", "gai", "gan", "gang", "gao", "ge", "gei", "gen", "geng", "gong", "gou", "gu", "gua", "guai", "guan", "guang", "gui", "gun", "guo",
    "ha", "hai", "han", "hang", "hao", "he", "hei", "hen", "heng", "hong", "hou", "hu", "hua", "huai", "huan", "huang", "hui", "hun", "huo",
    "ji", "jia", "jian", "jiang", "jiao", "jie", "jin", "jing", "jiong", "jiu", "ju", "juan", "jue", "jun",
    "ka", "kai", "kan", "kang", "kao", "ke", "ken", "keng", "kong", "kou", "ku", "kua", "kuai", "kuan", "kuang", "kui", "kun", "kuo",
    "la", "lai", "lan", "lang", "lao", "le", "lei", "leng", "li", "lia", "lian", "liang", "liao", "lie", "lin", "ling", "liu", "long", "lou", "lu", "luan", "lue", "lun", "luo", "lv",
    "ma", "mai", "man", "mang", "mao", "me", "mei", "men", "meng", "mi", "mian", "miao", "mie", "min", "ming", "miu", "mo", "mou", "mu",
    "na", "nai", "nan", "nang", "nao", "ne", "nei", "nen", "neng", "ni", "nian", "niang", "niao", "nie", "nin", "ning", "niu", "nong", "nu", "nuan", "nue", "nuo", "nv",
    "o", "ou",
    "pa", "pai", "pan", "pang", "pao", "pei", "pen", "peng", "pi", "pian", "piao", "pie", "pin", "ping", "po", "pou", "pu",
    "qi", "qia", "qian", "qiang", "qiao", "qie", "qin", "qing", "qiong", "qiu", "qu", "quan", "que", "qun",
    "ran", "rang", "rao", "re", "ren", "reng", "ri", "rong", "rou", "ru", "ruan", "rui", "run", "ruo",
    "sa", "sai", "san", "sang", "sao", "se", "sen", "seng", "sha", "shai", "shan", "shang", "shao", "she", "shen", "sheng", "shi", "shou", "shu", "shua", "shuai", "shuan", "shuang", "shui", "shun", "shuo", "si", "song", "sou", "su", "suan", "sui", "sun", "suo",
    "ta", "tai", "tan", "tang", "tao", "te", "teng", "ti", "tian", "tiao", "tie", "ting", "tong", "tou", "tu", "tuan", "tui", "tun", "tuo",
    "wa", "wai", "wan", "wang", "wei", "wen", "weng", "wo", "wu",
    "xi", "xia", "xian", "xiang", "xiao", "xie", "xin", "xing", "xiong", "xiu", "xu", "xuan", "xue", "xun",
    "ya", "yan", "yang", "yao", "ye", "yi", "yin", "ying", "yo", "yong", "you", "yu", "yuan", "yue", "yun",
    "za", "zai", "zan", "zang", "zao", "ze", "zei", "zen", "zeng", "zha", "zhai", "zhan", "zhang", "zhao", "zhe", "zhen", "zheng", "zhi", "zhong", "zhou", "zhu", "zhua", "zhuai", "zhuan", "zhuang", "zhui", "zhun", "zhuo", "zi", "zong", "zou", "zu", "zuan", "zui", "zun", "zuo"
};

PinyinConverter::PinyinConverter() {
    // 初始化有效字符映射
    for (char c = 'a'; c <= 'z'; ++c) {
        valid_chars_[c] = true;
    }
}

PinyinConverter::~PinyinConverter() = default;

bool PinyinConverter::initialize() {
    spdlog::info("Initializing pinyin converter...");
    
    if (!loadPinyinData()) {
        spdlog::error("Failed to load pinyin data");
        return false;
    }
    
    spdlog::info("Pinyin converter initialized with {} valid pinyins", valid_pinyins_.size());
    return true;
}

bool PinyinConverter::loadPinyinData() {
    // 加载标准拼音表
    valid_pinyins_ = STANDARD_PINYINS;
    
    // 构建拼音映射表
    pinyin_map_.clear();
    for (const auto& pinyin : valid_pinyins_) {
        pinyin_map_[pinyin] = true;
    }
    
    return true;
}

bool PinyinConverter::addChar(char ch) {
    // 检查字符是否有效
    if (valid_chars_.find(ch) == valid_chars_.end()) {
        return false;
    }
    
    // 添加字符到缓冲区
    std::string test_pinyin = current_pinyin_ + ch;
    
    // 检查是否有以此开头的有效拼音
    bool has_valid_prefix = false;
    for (const auto& pinyin : valid_pinyins_) {
        if (pinyin.length() >= test_pinyin.length() && 
            pinyin.substr(0, test_pinyin.length()) == test_pinyin) {
            has_valid_prefix = true;
            break;
        }
    }
    
    if (has_valid_prefix) {
        current_pinyin_ += ch;
        return true;
    }
    
    return false;
}

bool PinyinConverter::removeLastChar() {
    if (current_pinyin_.empty()) {
        return false;
    }
    
    current_pinyin_.pop_back();
    return true;
}

void PinyinConverter::clear() {
    current_pinyin_.clear();
}

const std::string& PinyinConverter::getCurrentPinyin() const {
    return current_pinyin_;
}

std::vector<std::vector<std::string>> PinyinConverter::getPinyinSegments() const {
    std::vector<std::vector<std::string>> results;
    
    if (current_pinyin_.empty()) {
        return results;
    }
    
    std::vector<std::string> current;
    segmentPinyinRecursive(current_pinyin_, 0, current, results);
    
    return results;
}

void PinyinConverter::segmentPinyinRecursive(
    const std::string& pinyin,
    size_t start,
    std::vector<std::string>& current,
    std::vector<std::vector<std::string>>& results
) const {
    if (start >= pinyin.length()) {
        if (!current.empty()) {
            results.push_back(current);
        }
        return;
    }
    
    // 尝试不同长度的拼音
    for (size_t len = 1; len <= pinyin.length() - start; ++len) {
        std::string segment = pinyin.substr(start, len);
        
        if (isValidPinyin(segment)) {
            current.push_back(segment);
            segmentPinyinRecursive(pinyin, start + len, current, results);
            current.pop_back();
        }
    }
}

bool PinyinConverter::isValidPinyin(const std::string& pinyin) const {
    return pinyin_map_.find(pinyin) != pinyin_map_.end();
}

std::vector<std::string> PinyinConverter::getPinyinPrefixes(const std::string& pinyin) const {
    std::vector<std::string> prefixes;
    
    for (const auto& valid_pinyin : valid_pinyins_) {
        if (valid_pinyin.length() >= pinyin.length() && 
            valid_pinyin.substr(0, pinyin.length()) == pinyin) {
            prefixes.push_back(valid_pinyin);
        }
    }
    
    return prefixes;
}

std::string PinyinConverter::normalizePinyin(const std::string& pinyin) const {
    std::string normalized = pinyin;
    
    // 转换为小写
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // 移除特殊字符（如果有的话）
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), 
                                   [](char c) { return !std::isalpha(c); }), 
                    normalized.end());
    
    return normalized;
}

} // namespace core
} // namespace owcat