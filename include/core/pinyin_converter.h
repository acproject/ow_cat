#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <map>

namespace owcat {
namespace core {

/**
 * 拼音转换器
 * 负责将键盘输入转换为拼音，并提供拼音分割功能
 */
class PinyinConverter {
public:
    PinyinConverter();
    ~PinyinConverter();

    /**
     * 初始化转换器
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * 添加字符到当前拼音缓冲区
     * @param ch 输入字符
     * @return 是否添加成功
     */
    bool addChar(char ch);

    /**
     * 删除最后一个字符
     * @return 是否删除成功
     */
    bool removeLastChar();

    /**
     * 清空拼音缓冲区
     */
    void clear();

    /**
     * 获取当前拼音字符串
     * @return 拼音字符串
     */
    const std::string& getCurrentPinyin() const;

    /**
     * 获取所有可能的拼音分割方案
     * @return 拼音分割方案列表
     */
    std::vector<std::vector<std::string>> getPinyinSegments() const;

    /**
     * 验证拼音是否有效
     * @param pinyin 拼音字符串
     * @return 是否有效
     */
    bool isValidPinyin(const std::string& pinyin) const;

    /**
     * 获取拼音的所有可能前缀
     * @param pinyin 拼音字符串
     * @return 前缀列表
     */
    std::vector<std::string> getPinyinPrefixes(const std::string& pinyin) const;

    /**
     * 将拼音转换为标准格式
     * @param pinyin 原始拼音
     * @return 标准格式拼音
     */
    std::string normalizePinyin(const std::string& pinyin) const;

private:
    /**
     * 加载拼音数据
     * @return 是否加载成功
     */
    bool loadPinyinData();

    /**
     * 递归分割拼音
     * @param pinyin 待分割的拼音
     * @param start 起始位置
     * @param current 当前分割结果
     * @param results 所有分割结果
     */
    void segmentPinyinRecursive(
        const std::string& pinyin,
        size_t start,
        std::vector<std::string>& current,
        std::vector<std::vector<std::string>>& results
    ) const;

private:
    std::string current_pinyin_;                    // 当前拼音缓冲区
    std::vector<std::string> valid_pinyins_;        // 有效拼音列表
    std::map<std::string, bool> pinyin_map_;        // 拼音映射表
    std::map<char, bool> valid_chars_;              // 有效字符映射
};

} // namespace core
} // namespace owcat