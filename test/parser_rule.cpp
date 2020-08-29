// Copyright (C) 2018 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/parser/parser.hpp>

#include <gtest/gtest.h>


using namespace boost::parser;

constexpr rule<struct flat_rule_tag> flat_rule = "flat_rule";
constexpr auto flat_rule_def = string("abc") | string("def");
BOOST_PARSER_DEFINE_RULES(flat_rule);

constexpr rule<struct recursive_rule_tag> recursive_rule = "recursive_rule";
constexpr auto recursive_rule_def = string("abc") >> -('a' >> recursive_rule);
BOOST_PARSER_DEFINE_RULES(recursive_rule);

TEST(parser, no_attribute_rules)
{
    {
        std::string const str = "xyz";
        EXPECT_FALSE(parse(str, flat_rule));
        EXPECT_FALSE(parse(str, recursive_rule));
    }
    {
        std::string const str = "def";
        bool const flat_result{parse(str, flat_rule)};
        EXPECT_TRUE(flat_result);
        EXPECT_FALSE(parse(str, recursive_rule));
    }
    {
        std::string const str = "abc";
        EXPECT_TRUE(parse(str, flat_rule));
        EXPECT_TRUE(parse(str, recursive_rule));
    }
    {
        std::string const str = "abcaabc";
        EXPECT_FALSE(parse(str, flat_rule));
        EXPECT_TRUE(parse(str, recursive_rule));
    }
    {
        std::string const str = "abcaabc";
        auto first = str.c_str();
        EXPECT_TRUE(parse(first, boost::text::null_sentinel{}, flat_rule));
        first = str.c_str();
        EXPECT_TRUE(parse(first, boost::text::null_sentinel{}, recursive_rule));
    }
}

constexpr rule<struct flat_string_rule_tag, std::string> flat_string_rule =
    "flat_string_rule";
constexpr auto flat_string_rule_def = string("abc") | string("def");
BOOST_PARSER_DEFINE_RULES(flat_string_rule);

constexpr rule<struct recursive_string_rule_tag, std::string>
    recursive_string_rule = "recursive_string_rule";
constexpr auto recursive_string_rule_def = string("abc") >>
                                           -('a' >> recursive_string_rule);
BOOST_PARSER_DEFINE_RULES(recursive_string_rule);

TEST(parser, string_attribute_rules)
{
    {
        std::string const str = "xyz";
        EXPECT_FALSE(parse(str, flat_string_rule));
        EXPECT_FALSE(parse(str, recursive_string_rule));
    }
    {
        std::string const str = "def";
        auto const flat_result = parse(str, flat_string_rule);
        EXPECT_TRUE(flat_result);
        EXPECT_EQ(*flat_result, "def");
        EXPECT_FALSE(parse(str, recursive_string_rule));
    }
    {
        std::string const str = "abc";
        EXPECT_EQ(*parse(str, flat_string_rule), "abc");
        EXPECT_EQ(*parse(str, recursive_string_rule), "abc");
    }
    {
        std::string const str = "abcaabc";
        EXPECT_FALSE(parse(str, flat_string_rule));
        EXPECT_FALSE(parse(str, recursive_string_rule));
    }
    {
        std::string const str = "abcaabc";
        auto first = str.c_str();
        EXPECT_EQ(
            *parse(first, boost::text::null_sentinel{}, flat_string_rule),
            "abc");
        first = str.c_str();
        EXPECT_EQ(
            *parse(first, boost::text::null_sentinel{}, recursive_string_rule),
            "abcabc");
    }
}

constexpr rule<struct flat_vector_rule_tag, std::vector<char>>
    flat_vector_rule = "flat_vector_rule";
constexpr auto flat_vector_rule_def = string("abc") | string("def");
BOOST_PARSER_DEFINE_RULES(flat_vector_rule);

TEST(parser, vector_attribute_rules)
{
    {
        std::string const str = "xyz";
        std::vector<char> chars;
        EXPECT_FALSE(parse(str, flat_vector_rule, chars));
    }
    {
        std::string const str = "def";
        std::vector<char> chars;
        EXPECT_TRUE(parse(str, flat_vector_rule, chars));
        EXPECT_EQ(chars, std::vector<char>({'d', 'e', 'f'}));
    }
    {
        std::string const str = "abc";
        EXPECT_EQ(
            *parse(str, flat_vector_rule), std::vector<char>({'a', 'b', 'c'}));
    }
    {
        std::string const str = "abcaabc";
        EXPECT_FALSE(parse(str, flat_vector_rule));
    }
    {
        std::string const str = "abcaabc";
        auto first = str.c_str();
        EXPECT_EQ(
            *parse(first, boost::text::null_sentinel{}, flat_vector_rule),
            std::vector<char>({'a', 'b', 'c'}));
    }
    {
        std::string const str = "abcaabc";
        auto first = str.c_str();
        EXPECT_EQ(
            callback_parse(
                first, boost::text::null_sentinel{}, flat_vector_rule, int{}),
            true);
    }
}

constexpr callback_rule<struct callback_vector_rule_tag, std::vector<char>>
    callback_vector_rule = "callback_vector_rule";
constexpr auto callback_vector_rule_def = string("abc") | string("def");
BOOST_PARSER_DEFINE_RULES(callback_vector_rule);

constexpr callback_rule<struct callback_void_rule_tag> callback_void_rule =
    "callback_void_rule";
constexpr auto callback_void_rule_def = string("abc") | string("def");
BOOST_PARSER_DEFINE_RULES(callback_void_rule);

struct callbacks_t
{
    void operator()(
        boost::hana::basic_type<callback_vector_rule_tag>,
        std::vector<char> && vec) const
    {
        all_results.push_back(std::move(vec));
    }
    void operator()(boost::hana::basic_type<callback_void_rule_tag>) const
    {
        void_callback_called = true;
    }

    mutable std::vector<std::vector<char>> all_results;
    mutable bool void_callback_called = false;
};

TEST(parser, callback_rules)
{
    {
        std::string const str = "xyz";
        callbacks_t callbacks;
        EXPECT_FALSE(callback_parse(str, callback_vector_rule, callbacks));
        EXPECT_EQ(callbacks.all_results.size(), 0u);
    }
    {
        std::string const str = "abc";
        callbacks_t callbacks;
        EXPECT_TRUE(callback_parse(str, callback_vector_rule, callbacks));
        EXPECT_EQ(callbacks.all_results.size(), 1u);
        EXPECT_EQ(callbacks.all_results[0], std::vector<char>({'a', 'b', 'c'}));
    }
    {
        std::string const str = "def";
        callbacks_t callbacks;
        EXPECT_TRUE(callback_parse(str, callback_vector_rule, callbacks));
        EXPECT_EQ(callbacks.all_results.size(), 1u);
        EXPECT_EQ(callbacks.all_results[0], std::vector<char>({'d', 'e', 'f'}));
    }

    {
        std::string const str = "xyz";
        callbacks_t callbacks;
        EXPECT_FALSE(callback_parse(str, callback_void_rule, callbacks));
        EXPECT_FALSE(callbacks.void_callback_called);
    }
    {
        std::string const str = "abc";
        callbacks_t callbacks;
        EXPECT_TRUE(callback_parse(str, callback_void_rule, callbacks));
        EXPECT_TRUE(callbacks.void_callback_called);
    }
    {
        std::string const str = "def";
        callbacks_t callbacks;
        EXPECT_TRUE(callback_parse(str, callback_void_rule, callbacks));
        EXPECT_TRUE(callbacks.void_callback_called);
    }

    {
        std::string const str = "xyz";
        std::vector<std::vector<char>> all_results;
        auto record_results = [&all_results](std::vector<char> && vec) {
            all_results.push_back(std::move(vec));
        };
        auto callbacks = boost::hana::make_map(boost::hana::make_pair(
            boost::hana::type_c<callback_vector_rule_tag>, record_results));
        EXPECT_FALSE(callback_parse(str, callback_vector_rule, callbacks));
        EXPECT_EQ(all_results.size(), 0u);
    }
    {
        std::string const str = "abc";
        std::vector<std::vector<char>> all_results;
        auto record_results = [&all_results](std::vector<char> && vec) {
            all_results.push_back(std::move(vec));
        };
        auto callbacks = boost::hana::make_map(boost::hana::make_pair(
            boost::hana::type_c<callback_vector_rule_tag>, record_results));
        EXPECT_TRUE(callback_parse(str, callback_vector_rule, callbacks));
        EXPECT_EQ(all_results.size(), 1u);
        EXPECT_EQ(all_results[0], std::vector<char>({'a', 'b', 'c'}));
    }
    {
        std::string const str = "def";
        std::vector<std::vector<char>> all_results;
        auto callbacks = boost::hana::make_map(boost::hana::make_pair(
            boost::hana::type_c<callback_vector_rule_tag>,
            [&all_results](std::vector<char> && vec) {
                all_results.push_back(std::move(vec));
            }));
        EXPECT_TRUE(callback_parse(str, callback_vector_rule, callbacks));
        EXPECT_EQ(all_results.size(), 1u);
        EXPECT_EQ(all_results[0], std::vector<char>({'d', 'e', 'f'}));
    }

    {
        std::string const str = "xyz";
        bool void_callback_called = false;
        auto record_results = [&void_callback_called]() {
            void_callback_called = true;
        };
        auto callbacks = boost::hana::make_map(boost::hana::make_pair(
            boost::hana::type_c<callback_void_rule_tag>, record_results));
        EXPECT_FALSE(callback_parse(str, callback_void_rule, callbacks));
        EXPECT_FALSE(void_callback_called);
    }
    {
        std::string const str = "abc";
        bool void_callback_called = false;
        auto record_results = [&void_callback_called]() {
            void_callback_called = true;
        };
        auto callbacks = boost::hana::make_map(boost::hana::make_pair(
            boost::hana::type_c<callback_void_rule_tag>, record_results));
        EXPECT_TRUE(callback_parse(str, callback_void_rule, callbacks));
        EXPECT_TRUE(void_callback_called);
    }
    {
        std::string const str = "def";
        bool void_callback_called = false;
        auto callbacks = boost::hana::make_map(boost::hana::make_pair(
            boost::hana::type_c<callback_void_rule_tag>,
            [&void_callback_called]() { void_callback_called = true; }));
        EXPECT_TRUE(callback_parse(str, callback_void_rule, callbacks));
        EXPECT_TRUE(void_callback_called);
    }
}

TEST(parser, callback_rules_normal_parse)
{
    {
        std::string const str = "xyz";
        std::vector<char> chars;
        EXPECT_FALSE(parse(str, callback_vector_rule, chars));
    }
    {
        std::string const str = "abc";
        std::vector<char> chars;
        EXPECT_TRUE(parse(str, callback_vector_rule, chars));
        EXPECT_EQ(chars, std::vector<char>({'a', 'b', 'c'}));
    }
    {
        std::string const str = "def";
        std::vector<char> chars;
        EXPECT_TRUE(parse(str, callback_vector_rule, chars));
        EXPECT_EQ(chars, std::vector<char>({'d', 'e', 'f'}));
    }

    {
        std::string const str = "def";
        EXPECT_TRUE(parse(str, callback_void_rule));
    }

    {
        std::string const str = "xyz";
        auto const chars = parse(str, callback_vector_rule);
        EXPECT_FALSE(chars);
    }
    {
        std::string const str = "abc";
        auto const chars = parse(str, callback_vector_rule);
        EXPECT_TRUE(chars);
        EXPECT_EQ(chars, std::vector<char>({'a', 'b', 'c'}));
    }
    {
        std::string const str = "def";
        auto const chars = parse(str, callback_vector_rule);
        EXPECT_TRUE(chars);
        EXPECT_EQ(chars, std::vector<char>({'d', 'e', 'f'}));
    }

    {
        std::string const str = "xyz";
        EXPECT_FALSE(parse(str, callback_vector_rule));
    }
    {
        std::string const str = "abc";
        EXPECT_TRUE(parse(str, callback_vector_rule));
    }
    {
        std::string const str = "def";
        EXPECT_TRUE(parse(str, callback_vector_rule));
    }
}
