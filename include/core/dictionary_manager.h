#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <memory>

namespace owcat {
namespace core {

/**
 * 词库管理器
 * 负责词库的加载、查询、更新和用户词汇学习
 */
class DictionaryManager {
public:
    explicit DictionaryManager(const std::string& db_path);
    ~DictionaryManager();

    // 禁用拷贝和移动
    DictionaryManager(const DictionaryManager&) = delete;
    DictionaryManager& operator=(const DictionaryManager&) = delete;
    DictionaryManager(DictionaryManager&&) = delete;
    DictionaryManager& operator=(DictionaryManager&&) = delete;

    /**
     * 初始化词库
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * 关闭词库
     */
    void shutdown();

    /**
     * 根据拼音查询候选词
     * @param pinyin 拼音字符串
     * @param max_results 最大结果数
     * @return 候选词列表
     */
    CandidateList searchByPinyin(const std::string& pinyin, int max_results = 10) const;

    /**
     * 根据拼音序列查询候选词
     * @param pinyins 拼音序列
     * @param max_results 最大结果数
     * @return 候选词列表
     */
    CandidateList searchByPinyinSequence(const std::vector<std::string>& pinyins, int max_results = 10) const;

    /**
     * 模糊查询候选词
     * @param partial_pinyin 部分拼音
     * @param max_results 最大结果数
     * @return 候选词列表
     */
    CandidateList fuzzySearch(const std::string& partial_pinyin, int max_results = 10) const;

    /**
     * 添加用户词汇
     * @param word 词汇
     * @param pinyin 拼音
     * @param frequency 初始频率
     * @return 是否添加成功
     */
    bool addUserWord(const std::string& word, const std::string& pinyin, int frequency = 1);

    /**
     * 更新词汇使用频率
     * @param word 词汇
     * @param pinyin 拼音
     * @return 是否更新成功
     */
    bool updateWordFrequency(const std::string& word, const std::string& pinyin);

    /**
     * 删除用户词汇
     * @param word 词汇
     * @param pinyin 拼音
     * @return 是否删除成功
     */
    bool removeUserWord(const std::string& word, const std::string& pinyin);

    /**
     * 学习用户输入
     * @param text 用户输入的文本
     * @param pinyin_sequence 对应的拼音序列
     * @return 是否学习成功
     */
    bool learnUserInput(const std::string& text, const std::vector<std::string>& pinyin_sequence);

    /**
     * 获取词汇详细信息
     * @param word 词汇
     * @param pinyin 拼音
     * @return 词汇信息，如果不存在返回空
     */
    std::unique_ptr<Candidate> getWordInfo(const std::string& word, const std::string& pinyin) const;

    /**
     * 导入词库文件
     * @param file_path 词库文件路径
     * @param format 文件格式（"txt", "csv", "json"）
     * @return 是否导入成功
     */
    bool importDictionary(const std::string& file_path, const std::string& format = "txt");

    /**
     * 导出用户词库
     * @param file_path 导出文件路径
     * @param format 文件格式（"txt", "csv", "json"）
     * @return 是否导出成功
     */
    bool exportUserDictionary(const std::string& file_path, const std::string& format = "txt") const;

    /**
     * 获取词库统计信息
     * @return 统计信息字符串
     */
    std::string getStatistics() const;

    /**
     * 清理低频词汇
     * @param min_frequency 最小频率阈值
     * @return 清理的词汇数量
     */
    int cleanupLowFrequencyWords(int min_frequency = 1);

private:
    /**
     * 创建数据库表
     * @return 是否创建成功
     */
    bool createTables();

    /**
     * 加载系统词库
     * @return 是否加载成功
     */
    bool loadSystemDictionary();

    /**
     * 计算词汇得分
     * @param candidate 候选词
     * @param context 上下文
     * @return 得分
     */
    double calculateScore(const Candidate& candidate, const std::string& context = "") const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace core
} // namespace owcat