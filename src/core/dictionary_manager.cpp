#include "core/dictionary_manager.h"
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace owcat {
namespace core {

class DictionaryManager::Impl {
public:
    explicit Impl(const std::string& db_path) 
        : db_path_(db_path), db_(nullptr) {
    }
    
    ~Impl() {
        if (db_) {
            sqlite3_close(db_);
        }
    }
    
    bool initialize() {
        spdlog::info("Initializing dictionary manager with database: {}", db_path_);
        
        // 打开数据库
        int rc = sqlite3_open(db_path_.c_str(), &db_);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to open database: {}", sqlite3_errmsg(db_));
            return false;
        }
        
        // 创建表结构
        if (!createTables()) {
            spdlog::error("Failed to create database tables");
            return false;
        }
        
        // 加载系统词库
        if (!loadSystemDictionary()) {
            spdlog::warn("Failed to load system dictionary, continuing with empty dictionary");
        }
        
        spdlog::info("Dictionary manager initialized successfully");
        return true;
    }
    
    void shutdown() {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }
    
    bool createTables() {
        const char* create_words_table = R"(
            CREATE TABLE IF NOT EXISTS words (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                word TEXT NOT NULL,
                pinyin TEXT NOT NULL,
                frequency INTEGER DEFAULT 1,
                is_user_word INTEGER DEFAULT 0,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(word, pinyin)
            )
        )";
        
        const char* create_index_pinyin = R"(
            CREATE INDEX IF NOT EXISTS idx_pinyin ON words(pinyin)
        )";
        
        const char* create_index_frequency = R"(
            CREATE INDEX IF NOT EXISTS idx_frequency ON words(frequency DESC)
        )";
        
        char* err_msg = nullptr;
        
        // 创建words表
        int rc = sqlite3_exec(db_, create_words_table, nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to create words table: {}", err_msg);
            sqlite3_free(err_msg);
            return false;
        }
        
        // 创建拼音索引
        rc = sqlite3_exec(db_, create_index_pinyin, nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to create pinyin index: {}", err_msg);
            sqlite3_free(err_msg);
            return false;
        }
        
        // 创建频率索引
        rc = sqlite3_exec(db_, create_index_frequency, nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to create frequency index: {}", err_msg);
            sqlite3_free(err_msg);
            return false;
        }
        
        return true;
    }
    
    bool loadSystemDictionary() {
        // 这里可以加载预置的系统词库
        // 为了演示，我们添加一些基本的词汇
        const std::vector<std::pair<std::string, std::string>> basic_words = {
            {"你好", "ni hao"},
            {"世界", "shi jie"},
            {"中国", "zhong guo"},
            {"输入法", "shu ru fa"},
            {"计算机", "ji suan ji"},
            {"程序", "cheng xu"},
            {"软件", "ruan jian"},
            {"开发", "kai fa"},
            {"技术", "ji shu"},
            {"人工智能", "ren gong zhi neng"}
        };
        
        const char* insert_sql = "INSERT OR IGNORE INTO words (word, pinyin, frequency, is_user_word) VALUES (?, ?, ?, 0)";
        sqlite3_stmt* stmt;
        
        int rc = sqlite3_prepare_v2(db_, insert_sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare insert statement: {}", sqlite3_errmsg(db_));
            return false;
        }
        
        for (const auto& [word, pinyin] : basic_words) {
            sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, pinyin.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, 100); // 默认频率
            
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                spdlog::warn("Failed to insert word '{}': {}", word, sqlite3_errmsg(db_));
            }
            
            sqlite3_reset(stmt);
        }
        
        sqlite3_finalize(stmt);
        return true;
    }
    
    CandidateList searchByPinyin(const std::string& pinyin, int max_results) const {
        CandidateList candidates;
        
        const char* sql = R"(
            SELECT word, pinyin, frequency 
            FROM words 
            WHERE pinyin LIKE ? OR pinyin LIKE ?
            ORDER BY frequency DESC, length(word) ASC
            LIMIT ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare search statement: {}", sqlite3_errmsg(db_));
            return candidates;
        }
        
        std::string exact_match = pinyin;
        std::string prefix_match = pinyin + "%";
        
        sqlite3_bind_text(stmt, 1, exact_match.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, prefix_match.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, max_results);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string word = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string word_pinyin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int frequency = sqlite3_column_int(stmt, 2);
            
            double score = calculateScore(word, word_pinyin, frequency, pinyin);
            candidates.emplace_back(word, word_pinyin, score, frequency, false);
        }
        
        sqlite3_finalize(stmt);
        return candidates;
    }
    
    CandidateList searchByPinyinSequence(const std::vector<std::string>& pinyins, int max_results) const {
        CandidateList candidates;
        
        if (pinyins.empty()) {
            return candidates;
        }
        
        // 构建查询条件
        std::string pinyin_pattern;
        for (size_t i = 0; i < pinyins.size(); ++i) {
            if (i > 0) pinyin_pattern += " ";
            pinyin_pattern += pinyins[i];
        }
        
        return searchByPinyin(pinyin_pattern, max_results);
    }
    
    CandidateList fuzzySearch(const std::string& partial_pinyin, int max_results) const {
        CandidateList candidates;
        
        const char* sql = R"(
            SELECT word, pinyin, frequency 
            FROM words 
            WHERE pinyin LIKE ?
            ORDER BY frequency DESC, length(word) ASC
            LIMIT ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare fuzzy search statement: {}", sqlite3_errmsg(db_));
            return candidates;
        }
        
        std::string pattern = "%" + partial_pinyin + "%";
        sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, max_results);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string word = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string word_pinyin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int frequency = sqlite3_column_int(stmt, 2);
            
            double score = calculateScore(word, word_pinyin, frequency, partial_pinyin);
            candidates.emplace_back(word, word_pinyin, score, frequency, false);
        }
        
        sqlite3_finalize(stmt);
        return candidates;
    }
    
    bool addUserWord(const std::string& word, const std::string& pinyin, int frequency) {
        const char* sql = "INSERT OR REPLACE INTO words (word, pinyin, frequency, is_user_word) VALUES (?, ?, ?, 1)";
        sqlite3_stmt* stmt;
        
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare add user word statement: {}", sqlite3_errmsg(db_));
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, pinyin.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, frequency);
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            spdlog::error("Failed to add user word: {}", sqlite3_errmsg(db_));
            return false;
        }
        
        spdlog::debug("Added user word: {} ({})", word, pinyin);
        return true;
    }
    
    bool updateWordFrequency(const std::string& word, const std::string& pinyin) {
        const char* sql = "UPDATE words SET frequency = frequency + 1, updated_at = CURRENT_TIMESTAMP WHERE word = ? AND pinyin = ?";
        sqlite3_stmt* stmt;
        
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare update frequency statement: {}", sqlite3_errmsg(db_));
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, pinyin.c_str(), -1, SQLITE_STATIC);
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            spdlog::error("Failed to update word frequency: {}", sqlite3_errmsg(db_));
            return false;
        }
        
        return true;
    }
    
    bool removeUserWord(const std::string& word, const std::string& pinyin) {
        const char* sql = "DELETE FROM words WHERE word = ? AND pinyin = ? AND is_user_word = 1";
        sqlite3_stmt* stmt;
        
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare remove user word statement: {}", sqlite3_errmsg(db_));
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, pinyin.c_str(), -1, SQLITE_STATIC);
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            spdlog::error("Failed to remove user word: {}", sqlite3_errmsg(db_));
            return false;
        }
        
        return true;
    }
    
    std::string getStatistics() const {
        const char* sql = R"(
            SELECT 
                COUNT(*) as total_words,
                COUNT(CASE WHEN is_user_word = 1 THEN 1 END) as user_words,
                COUNT(CASE WHEN is_user_word = 0 THEN 1 END) as system_words,
                AVG(frequency) as avg_frequency
            FROM words
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            return "Failed to get statistics";
        }
        
        std::stringstream ss;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int total = sqlite3_column_int(stmt, 0);
            int user = sqlite3_column_int(stmt, 1);
            int system = sqlite3_column_int(stmt, 2);
            double avg_freq = sqlite3_column_double(stmt, 3);
            
            ss << "Dictionary Statistics:\n";
            ss << "  Total words: " << total << "\n";
            ss << "  User words: " << user << "\n";
            ss << "  System words: " << system << "\n";
            ss << "  Average frequency: " << std::fixed << std::setprecision(2) << avg_freq;
        }
        
        sqlite3_finalize(stmt);
        return ss.str();
    }
    
private:
    double calculateScore(const std::string& word, const std::string& word_pinyin, 
                         int frequency, const std::string& input_pinyin) const {
        double score = 0.0;
        
        // 频率得分 (0-50)
        score += std::min(50.0, frequency / 10.0);
        
        // 长度得分 (shorter words get higher score)
        score += std::max(0.0, 20.0 - word.length());
        
        // 拼音匹配得分 (0-30)
        if (word_pinyin == input_pinyin) {
            score += 30.0; // 完全匹配
        } else if (word_pinyin.find(input_pinyin) == 0) {
            score += 20.0; // 前缀匹配
        } else if (word_pinyin.find(input_pinyin) != std::string::npos) {
            score += 10.0; // 包含匹配
        }
        
        return score;
    }
    
public:
    std::string db_path_;
    sqlite3* db_;
};

// DictionaryManager implementation
DictionaryManager::DictionaryManager(const std::string& db_path)
    : pImpl(std::make_unique<Impl>(db_path)) {
}

DictionaryManager::~DictionaryManager() = default;

bool DictionaryManager::initialize() {
    return pImpl->initialize();
}

void DictionaryManager::shutdown() {
    pImpl->shutdown();
}

CandidateList DictionaryManager::searchByPinyin(const std::string& pinyin, int max_results) const {
    return pImpl->searchByPinyin(pinyin, max_results);
}

CandidateList DictionaryManager::searchByPinyinSequence(const std::vector<std::string>& pinyins, int max_results) const {
    return pImpl->searchByPinyinSequence(pinyins, max_results);
}

CandidateList DictionaryManager::fuzzySearch(const std::string& partial_pinyin, int max_results) const {
    return pImpl->fuzzySearch(partial_pinyin, max_results);
}

bool DictionaryManager::addUserWord(const std::string& word, const std::string& pinyin, int frequency) {
    return pImpl->addUserWord(word, pinyin, frequency);
}

bool DictionaryManager::updateWordFrequency(const std::string& word, const std::string& pinyin) {
    return pImpl->updateWordFrequency(word, pinyin);
}

bool DictionaryManager::removeUserWord(const std::string& word, const std::string& pinyin) {
    return pImpl->removeUserWord(word, pinyin);
}

bool DictionaryManager::learnUserInput(const std::string& text, const std::vector<std::string>& pinyin_sequence) {
    // 简单实现：将整个文本作为一个词汇学习
    if (text.empty() || pinyin_sequence.empty()) {
        return false;
    }
    
    std::string combined_pinyin;
    for (size_t i = 0; i < pinyin_sequence.size(); ++i) {
        if (i > 0) combined_pinyin += " ";
        combined_pinyin += pinyin_sequence[i];
    }
    
    return addUserWord(text, combined_pinyin, 1);
}

std::unique_ptr<Candidate> DictionaryManager::getWordInfo(const std::string& word, const std::string& pinyin) const {
    // 实现获取词汇详细信息的逻辑
    auto candidates = pImpl->searchByPinyin(pinyin, 100);
    for (const auto& candidate : candidates) {
        if (candidate.text == word && candidate.pinyin == pinyin) {
            return std::make_unique<Candidate>(candidate);
        }
    }
    return nullptr;
}

bool DictionaryManager::importDictionary(const std::string& file_path, const std::string& format) {
    // 简单的TXT格式导入实现
    if (format == "txt") {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            spdlog::error("Failed to open dictionary file: {}", file_path);
            return false;
        }
        
        std::string line;
        int imported = 0;
        while (std::getline(file, line)) {
            // 假设格式为: word pinyin frequency
            std::istringstream iss(line);
            std::string word, pinyin;
            int frequency = 1;
            
            if (iss >> word >> pinyin >> frequency) {
                if (addUserWord(word, pinyin, frequency)) {
                    imported++;
                }
            }
        }
        
        spdlog::info("Imported {} words from {}", imported, file_path);
        return imported > 0;
    }
    
    spdlog::error("Unsupported dictionary format: {}", format);
    return false;
}

bool DictionaryManager::exportUserDictionary(const std::string& file_path, const std::string& format) const {
    // 简单的TXT格式导出实现
    if (format == "txt") {
        std::ofstream file(file_path);
        if (!file.is_open()) {
            spdlog::error("Failed to create export file: {}", file_path);
            return false;
        }
        
        // 导出用户词汇
        const char* sql = "SELECT word, pinyin, frequency FROM words WHERE is_user_word = 1 ORDER BY frequency DESC";
        sqlite3_stmt* stmt;
        
        int rc = sqlite3_prepare_v2(pImpl->db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare export statement: {}", sqlite3_errmsg(pImpl->db_));
            return false;
        }
        
        int exported = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string word = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string pinyin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int frequency = sqlite3_column_int(stmt, 2);
            
            file << word << " " << pinyin << " " << frequency << "\n";
            exported++;
        }
        
        sqlite3_finalize(stmt);
        spdlog::info("Exported {} user words to {}", exported, file_path);
        return exported > 0;
    }
    
    spdlog::error("Unsupported export format: {}", format);
    return false;
}

std::string DictionaryManager::getStatistics() const {
    return pImpl->getStatistics();
}

int DictionaryManager::cleanupLowFrequencyWords(int min_frequency) {
    const char* sql = "DELETE FROM words WHERE is_user_word = 1 AND frequency < ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(pImpl->db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare cleanup statement: {}", sqlite3_errmsg(pImpl->db_));
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, min_frequency);
    
    rc = sqlite3_step(stmt);
    int deleted = sqlite3_changes(pImpl->db_);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        spdlog::error("Failed to cleanup low frequency words: {}", sqlite3_errmsg(pImpl->db_));
        return 0;
    }
    
    spdlog::info("Cleaned up {} low frequency words", deleted);
    return deleted;
}

} // namespace core
} // namespace owcat