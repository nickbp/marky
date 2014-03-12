/*
  marky - A Markov chain generator.
  Copyright (C) 2011-2014  Nicholas Parker

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gtest/gtest.h>
#include <marky/string-pack.h>
#include <marky/backend.h>

using namespace marky;

TEST(StringPack, pack_single) {
    words_t words;
    words.push_back("hello");
    std::ostringstream oss;
    pack(words, oss);
    EXPECT_EQ("hello", oss.str());
}

TEST(StringPack, unpack_single) {
    std::string word("hello");
    words_t words;
    unpack(word, words);
    EXPECT_EQ(1, words.size());
    words_t::const_iterator iter = words.begin();
    EXPECT_EQ("hello", *(iter++));
}

TEST(StringPack, pack_multi) {
    words_t words;
    words.push_back("hello");
    words.push_back("world");
    std::ostringstream oss;
    pack(words, oss);
    EXPECT_EQ("hello,world", oss.str());
}

TEST(StringPack, unpack_multi) {
    std::string word("hello,world");
    words_t words;
    unpack(word, words);
    EXPECT_EQ(2, words.size());
    words_t::const_iterator iter = words.begin();
    EXPECT_EQ("hello", *(iter++));
    EXPECT_EQ("world", *(iter++));
}

TEST(StringPack, pack_sentence_start) {
    words_t words;
    words.push_back(IBackend::LINE_START);
    words.push_back("world");
    std::ostringstream oss;
    pack(words, oss);
    EXPECT_EQ(",world", oss.str());
}

TEST(StringPack, unpack_sentence_start) {
    std::string word(",world");
    words_t words;
    unpack(word, words);
    EXPECT_EQ(2, words.size());
    words_t::const_iterator iter = words.begin();
    EXPECT_EQ(IBackend::LINE_START, *(iter++));
    EXPECT_EQ("world", *(iter++));
}

TEST(StringPack, pack_sentence_end) {
    words_t words;
    words.push_back("world");
    words.push_back(IBackend::LINE_END);
    std::ostringstream oss;
    pack(words, oss);
    EXPECT_EQ("world,", oss.str());
}

TEST(StringPack, unpack_sentence_end) {
    std::string word("world,");
    words_t words;
    unpack(word, words);
    EXPECT_EQ(2, words.size());
    words_t::const_iterator iter = words.begin();
    EXPECT_EQ("world", *(iter++));
    EXPECT_EQ(IBackend::LINE_END, *(iter++));
}

TEST(StringPack, pack_sentence_both) {
    words_t words;
    words.push_back(IBackend::LINE_START);
    words.push_back("world");
    words.push_back(IBackend::LINE_END);
    std::ostringstream oss;
    pack(words, oss);
    EXPECT_EQ(",world,", oss.str());
}

TEST(StringPack, unpack_sentence_both) {
    std::string word(",world,");
    words_t words;
    unpack(word, words);
    EXPECT_EQ(3, words.size());
    words_t::const_iterator iter = words.begin();
    EXPECT_EQ(IBackend::LINE_START, *(iter++));
    EXPECT_EQ("world", *(iter++));
    EXPECT_EQ(IBackend::LINE_END, *(iter++));
}

TEST(StringPack, pack_delim_word_start) {
    words_t words;
    words.push_back("same");
    words.push_back(",OP");
    std::ostringstream oss;
    pack(words, oss);
    EXPECT_EQ("same,\\,OP", oss.str());
}

TEST(StringPack, unpack_delim_word_start) {
    std::string word("same,\\,OP");
    words_t words;
    unpack(word, words);
    EXPECT_EQ(2, words.size());
    words_t::const_iterator iter = words.begin();
    EXPECT_EQ("same", *(iter++));
    EXPECT_EQ(",OP", *(iter++));
}

TEST(StringPack, pack_delim_word_end) {
    words_t words;
    words.push_back("I'm");
    words.push_back("fine,");
    words.push_back("thanks");
    std::ostringstream oss;
    pack(words, oss);
    EXPECT_EQ("I'm,fine\\,,thanks", oss.str());
}

TEST(StringPack, unpack_delim_word_end) {
    std::string word("I'm,fine\\,,thanks");
    words_t words;
    unpack(word, words);
    EXPECT_EQ(3, words.size());
    words_t::const_iterator iter = words.begin();
    EXPECT_EQ("I'm", *(iter++));
    EXPECT_EQ("fine,", *(iter++));
    EXPECT_EQ("thanks", *(iter++));
}

TEST(StringPack, pack_delim_line_end) {
    words_t words;
    words.push_back("hey!");
    words.push_back("hi!,");
    std::ostringstream oss;
    pack(words, oss);
    EXPECT_EQ("hey!,hi!\\,", oss.str());
}

TEST(StringPack, unpack_delim_line_end) {
    std::string word("hey!,hi!\\,");
    words_t words;
    unpack(word, words);
    EXPECT_EQ(2, words.size());
    words_t::const_iterator iter = words.begin();
    EXPECT_EQ("hey!", *(iter++));
    EXPECT_EQ("hi!,", *(iter++));
}

TEST(StringPack, unpack_illegal_escape_line_end) {
    /* just give up and pass it through: */
    std::string word("hey!,hello\\");
    words_t words;
    unpack(word, words);
    EXPECT_EQ(2, words.size());
    words_t::const_iterator iter = words.begin();
    EXPECT_EQ("hey!", *(iter++));
    EXPECT_EQ("hello\\", *(iter++));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
