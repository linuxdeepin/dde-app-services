// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>
#include <QString>
#include <QVariant>

#include "../common/helper.hpp"

class HelperTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// 测试 JSON 对象验证
TEST_F(HelperTest, isValidTextJsonValue_ValidObjects)
{
    // 简单对象
    EXPECT_TRUE(isValidTextJsonValue(R"({"key":"value"})"));
    
    // 嵌套对象
    EXPECT_TRUE(isValidTextJsonValue(R"({"key1":{"key2":"value2"}})"));
    
    // 多个键值对
    EXPECT_TRUE(isValidTextJsonValue(R"({"key1":"value1","key2":"value2"})"));
    
    // 空对象
    EXPECT_TRUE(isValidTextJsonValue("{}"));
    
    // 对象包含数组
    EXPECT_TRUE(isValidTextJsonValue(R"({"key":["value1","value2"]})"));
}

// 测试 JSON 数组验证
TEST_F(HelperTest, isValidTextJsonValue_ValidArrays)
{
    // 字符串数组
    EXPECT_TRUE(isValidTextJsonValue(R"(["value1","value2"])"));
    
    // 数字数组
    EXPECT_TRUE(isValidTextJsonValue("[1,2,3]"));
    
    // 混合类型数组
    EXPECT_TRUE(isValidTextJsonValue(R"(["text",123,true,null])"));
    
    // 空数组
    EXPECT_TRUE(isValidTextJsonValue("[]"));
    
    // 嵌套数组
    EXPECT_TRUE(isValidTextJsonValue(R"([["a","b"],["c","d"]])"));
    
    // 对象数组
    EXPECT_TRUE(isValidTextJsonValue(R"([{"key1":"value1"},{"key2":"value2"}])"));
}

// 测试单个 JSON 值：带引号的字符串
TEST_F(HelperTest, isValidTextJsonValue_ValidStrings)
{
    // 普通字符串
    EXPECT_TRUE(isValidTextJsonValue(R"("hello")"));
    
    // 空字符串
    EXPECT_TRUE(isValidTextJsonValue(R"("")"));
    
    // 包含空格的字符串
    EXPECT_TRUE(isValidTextJsonValue(R"("hello world")"));
    
    // 包含数字的字符串
    EXPECT_TRUE(isValidTextJsonValue(R"("125")"));
    
    // 包含转义字符的字符串
    EXPECT_TRUE(isValidTextJsonValue(R"("hello \"world\"")"));
    
    // 包含反斜杠的字符串
    EXPECT_TRUE(isValidTextJsonValue(R"("C:\\path")"));
    
    // 包含换行符的字符串
    EXPECT_TRUE(isValidTextJsonValue(R"("line1\nline2")"));
}

// 测试单个 JSON 值：数字
TEST_F(HelperTest, isValidTextJsonValue_ValidNumbers)
{
    // 整数
    EXPECT_TRUE(isValidTextJsonValue("123"));
    
    // 负整数
    EXPECT_TRUE(isValidTextJsonValue("-456"));
    
    // 浮点数
    EXPECT_TRUE(isValidTextJsonValue("3.14"));
    
    // 负浮点数
    EXPECT_TRUE(isValidTextJsonValue("-223.56"));
    
    // 科学计数法
    EXPECT_TRUE(isValidTextJsonValue("1.23e10"));
    
    // 零
    EXPECT_TRUE(isValidTextJsonValue("0"));
}

// 测试单个 JSON 值：布尔值
TEST_F(HelperTest, isValidTextJsonValue_ValidBooleans)
{
    EXPECT_TRUE(isValidTextJsonValue("true"));
    EXPECT_TRUE(isValidTextJsonValue("false"));
}

// 测试单个 JSON 值：null
TEST_F(HelperTest, isValidTextJsonValue_ValidNull)
{
    EXPECT_TRUE(isValidTextJsonValue("null"));
}

// 测试拒绝不带引号的纯文本
TEST_F(HelperTest, isValidTextJsonValue_RejectPlainText)
{
    // 不带引号的单词
    EXPECT_FALSE(isValidTextJsonValue("hello"));
    EXPECT_FALSE(isValidTextJsonValue("application"));
    EXPECT_FALSE(isValidTextJsonValue("text"));
    
    // 不带引号的多个单词
    EXPECT_FALSE(isValidTextJsonValue("hello world"));
    
    // 不带引号的数字字符串（应该用引号）
    // 注意：纯数字如 "123" 是有效的 JSON 数字
}

// 测试拒绝格式错误的 JSON
TEST_F(HelperTest, isValidTextJsonValue_RejectMalformedJSON)
{
    // 数组格式错误
    EXPECT_FALSE(isValidTextJsonValue(R"([s", "d"])"));
    EXPECT_FALSE(isValidTextJsonValue(R"(["unclosed)"));
    EXPECT_FALSE(isValidTextJsonValue(R"("value1", "value2"])"));
    
    // 对象格式错误
    EXPECT_FALSE(isValidTextJsonValue(R"({key: "value"})"));  // 键没有引号
    EXPECT_FALSE(isValidTextJsonValue(R"({"key":value})"));   // 值没有引号
    EXPECT_FALSE(isValidTextJsonValue(R"({"key": "value")"));  // 未闭合
    
    // 字符串格式错误
    EXPECT_FALSE(isValidTextJsonValue(R"("unclosed)"));
    EXPECT_FALSE(isValidTextJsonValue(R"(unclosed")"));
    
    // 无效的布尔值
    EXPECT_FALSE(isValidTextJsonValue("True"));   // 大写
    EXPECT_FALSE(isValidTextJsonValue("FALSE"));  // 大写
    EXPECT_FALSE(isValidTextJsonValue("yes"));
    
    // 无效的 null
    EXPECT_FALSE(isValidTextJsonValue("NULL"));
    EXPECT_FALSE(isValidTextJsonValue("Null"));
    
    // 多个顶层值（不是数组）
    EXPECT_FALSE(isValidTextJsonValue(R"("value1" "value2")"));
}

// 测试包装方法的边界情况
TEST_F(HelperTest, isValidTextJsonValue_EdgeCases)
{
    // 空字符串
    EXPECT_FALSE(isValidTextJsonValue(""));
    
    // 只有空格
    EXPECT_FALSE(isValidTextJsonValue("   "));
    EXPECT_FALSE(isValidTextJsonValue("\n"));
    EXPECT_FALSE(isValidTextJsonValue("\t"));
    
    // 带前后空格的有效 JSON（应该被 trim 后验证）
    EXPECT_TRUE(isValidTextJsonValue(R"(  "hello"  )"));
    EXPECT_TRUE(isValidTextJsonValue("\n[1,2,3]\n"));
    EXPECT_TRUE(isValidTextJsonValue(R"(	{"key":"value"}	)"));
    
    // 只有引号
    EXPECT_FALSE(isValidTextJsonValue(R"(")"));
    
    // 只有括号
    EXPECT_FALSE(isValidTextJsonValue("["));
    EXPECT_FALSE(isValidTextJsonValue("]"));
    EXPECT_FALSE(isValidTextJsonValue("{"));
    EXPECT_FALSE(isValidTextJsonValue("}"));
}

// 测试 stringToQVariant 函数
TEST_F(HelperTest, stringToQVariant_ValidConversions)
{
    // 对象
    QVariant obj = stringToQVariant(R"({"key":"value"})");
    EXPECT_EQ(obj.typeId(), QMetaType::QVariantMap);
    
    // 数组
    QVariant arr = stringToQVariant(R"(["value1","value2"])");
    EXPECT_EQ(arr.typeId(), QMetaType::QVariantList);
    
    // 字符串
    QVariant str = stringToQVariant(R"("hello")");
    EXPECT_EQ(str.typeId(), QMetaType::QString);
    EXPECT_EQ(str.toString(), QString("hello"));
    
    // 数字
    QVariant num = stringToQVariant("123");
    EXPECT_TRUE(num.canConvert<int>());
    EXPECT_EQ(num.toInt(), 123);
    
    // 浮点数
    QVariant dbl = stringToQVariant("3.14");
    EXPECT_TRUE(dbl.canConvert<double>());
    EXPECT_DOUBLE_EQ(dbl.toDouble(), 3.14);
    
    // 布尔值
    QVariant boolTrue = stringToQVariant("true");
    EXPECT_EQ(boolTrue.typeId(), QMetaType::Bool);
    EXPECT_TRUE(boolTrue.toBool());
    
    QVariant boolFalse = stringToQVariant("false");
    EXPECT_EQ(boolFalse.typeId(), QMetaType::Bool);
    EXPECT_FALSE(boolFalse.toBool());
}

// 测试 qvariantToString 和 stringToQVariant 的往返转换
TEST_F(HelperTest, RoundTripConversion)
{
    // 字符串往返
    QString originalStr = "hello world";
    QVariant varStr(originalStr);
    QString jsonStr = qvariantToStringCompact(varStr);
    QVariant varStr2 = stringToQVariant(jsonStr);
    EXPECT_EQ(varStr2.toString(), originalStr);

    QString quotedStr = "he said \"hello\"";
    QVariant varQuoted(quotedStr);
    QString jsonQuoted = qvariantToStringCompact(varQuoted);
    QVariant varQuoted2 = stringToQVariant(jsonQuoted);
    EXPECT_EQ(varQuoted2.toString(), QString("\"%1\"").arg(quotedStr));
    
    // 数字往返
    int num = 123;
    QVariant varNum(num);
    QString jsonNum = qvariantToStringCompact(varNum);
    QVariant varNum2 = stringToQVariant(jsonNum);
    EXPECT_EQ(varNum2.toInt(), num);
    
    // 布尔值往返
    bool boolVal = true;
    QVariant varBool(boolVal);
    QString jsonBool = qvariantToStringCompact(varBool);
    QVariant varBool2 = stringToQVariant(jsonBool);
    EXPECT_EQ(varBool2.toBool(), boolVal);
}

// 测试特殊字符的处理
TEST_F(HelperTest, SpecialCharacters)
{
    // 字符串中的特殊字符应该被正确转义
    QString special = "line1\nline2\ttab";
    QVariant varSpecial(special);
    QString json = qvariantToStringCompact(varSpecial);
    
    // 验证 JSON 有效
    EXPECT_TRUE(isValidTextJsonValue(json));
    
    // 验证往返转换
    QVariant varSpecial2 = stringToQVariant(json);
    EXPECT_EQ(varSpecial2.toString(), special);
}
