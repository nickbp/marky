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
#include <marky/backend-map.h>
#include <marky/config.h>

using namespace marky;

static void init_data_1(const State& state, IBackend& backend, const scorer_t& scorer) {
    marky::words_to_counts counts;
    counts.increment({"a", "b"});
    counts.increment({"a", "b"});
    counts.increment({"a", "b"});
    counts.increment({"a", "c"});
    counts.increment({"b", "c"});
    counts.increment({"b", "c"});
    counts.increment({"c", "a"});
    ASSERT_TRUE(backend.update_snippets(state, scorer, counts.map()));
}

static void init_data_2(const State& state, IBackend& backend, const scorer_t& scorer) {
    marky::words_to_counts counts;
    counts.increment({"a", "b", "c"});
    counts.increment({"a", "b", "c"});
    counts.increment({"a", "b", "c"});
    counts.increment({"a", "c", "d"});
    counts.increment({"b", "c", "d"});
    counts.increment({"b", "c", "d"});
    counts.increment({"c", "a", "b"});
    ASSERT_TRUE(backend.update_snippets(state, scorer, counts.map()));
}

static marky::words_to_counts::map_t to_map(const words_t& words) {
    marky::words_to_counts::map_t map;
    map[words] = 1;
    return map;
}

#define CHECK_WORD(SCORER, STATE, WORD, PREV, NEXT, SCORE)      \
    ASSERT_NE((bool)WORD);                                      \
    EXPECT_EQ(PREV, WORD->prev);                                \
    EXPECT_EQ(NEXT, WORD->next);                                \
    EXPECT_EQ(SCORE, WORD->score(SCORER, STATE));

static void test_get_prev(bool insert_2) {
    Backend_Map backend;
    scorer_t scorer = scorers::no_adj();
    selector_t selector = selectors::best_always();
    State state(0,0);

    word_t word;
    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"c", "a", "x"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"c", "a"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"c"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);

    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"a", "b", "x"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"a", "b"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"a"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);

    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b", "c", "x"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b", "c"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);

    if (insert_2) {
        init_data_2(state, backend, scorer);

        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"g"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);

        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"c", "a", "x"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"c", "a"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"c"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);

        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"a", "b", "x"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"a"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);

        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b", "c", "x"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b", "c"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);
    } else {
        init_data_1(state, backend, scorer);

        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"g"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);

        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"c", "a", "x"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"c"}, word));
        EXPECT_EQ("b", word);

        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"a", "b", "x"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"a"}, word));
        EXPECT_EQ("c", word);

        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b", "c", "x"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b", "c"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b"}, word));
        EXPECT_EQ("a", word);
    }
}

TEST(Map, get_prev_1) {
    test_get_prev(false);
}
TEST(Map, get_prev_2) {
    test_get_prev(true);
}

static void test_get_next(bool insert_2) {
    Backend_Map backend;
    scorer_t scorer = scorers::no_adj();
    selector_t selector = selectors::best_always();
    State state(0,0);

    word_t word;
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"x", "c", "a"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c", "a"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"x", "a", "b"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a", "b"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"b"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"x", "b", "c"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"b", "c"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);

    if (insert_2) {
        init_data_2(state, backend, scorer);

        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"g"}, word));
        EXPECT_EQ(IBackend::LINE_END, word);

        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"x", "c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));
        EXPECT_EQ(IBackend::LINE_END, word); /* no sub-entries are entered by the backend */

        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"x", "a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"b"}, word));
        EXPECT_EQ(IBackend::LINE_END, word);

        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"x", "b", "c"}, word));
        EXPECT_EQ("d", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"b", "c"}, word));
        EXPECT_EQ("d", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));
        EXPECT_EQ(IBackend::LINE_END, word);
    } else {
        init_data_1(state, backend, scorer);

        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"g"}, word));
        EXPECT_EQ(IBackend::LINE_END, word);

        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"x", "c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));
        EXPECT_EQ("b", word);

        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"x", "a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"b"}, word));
        EXPECT_EQ("c", word);

        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"x", "b", "c"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"b", "c"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));
        EXPECT_EQ("a", word);
    }
}

TEST(Map, get_next_1) {
    test_get_next(false);
}
TEST(Map, get_next_2) {
    test_get_next(true);
}

TEST(Map, get_random) {
    Backend_Map backend;
    /* each word loses a point if it's not updated within 2 increments */
    scorer_t scorer = scorers::word_adj(2);
    State state(0,0);

    word_t rand;
    EXPECT_TRUE(backend.get_random(state, scorer, rand));
    EXPECT_EQ(IBackend::LINE_END, rand);

    /* add one word, make sure we don't get back a blank */
    ASSERT_TRUE(backend.update_snippets(state, scorer, to_map({"a"})));

    EXPECT_TRUE(backend.get_random(state, scorer, rand));
    EXPECT_NE(IBackend::LINE_END, rand);

    /* add two words, assume we'll be getting back one of them */
    marky::words_to_counts::map_t map;
    map[{"a", "b"}] = 1;
    map[{"c", "d"}] = 1;
    ASSERT_TRUE(backend.update_snippets(state, scorer, map));

    EXPECT_TRUE(backend.get_random(state, scorer, rand));
    EXPECT_NE(IBackend::LINE_END, rand);
}

#define INC_STATE(state) DEBUG("INC %lu", state.count); ++state.time; ++state.count;

TEST(Map, scoreadj_prune) {
    Backend_Map backend;
    /* each word loses a point if it's not updated within 2 increments */
    scorer_t scorer = scorers::word_adj(2);

    State state(0,0);
    backend.update_snippets(state, scorer, to_map({"a", "b"}));
    INC_STATE(state);//0
    backend.update_snippets(state, scorer, to_map({"a", "b"}));
    INC_STATE(state);//1
    backend.update_snippets(state, scorer, to_map({"a", "b"}));
    INC_STATE(state);//2

    selector_t selector = selectors::best_always();
    word_t word;

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));//score=3
    EXPECT_EQ("b", word);

    backend.update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//3

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));//score=2
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));//score=1
    EXPECT_EQ("d", word);

    backend.update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//4

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));//score=2
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));//score=2
    EXPECT_EQ("d", word);

    backend.update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//5

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));//score=1
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));//score=3
    EXPECT_EQ("d", word);

    backend.update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//6

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));//score=1
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));//score=4
    EXPECT_EQ("d", word);

    backend.prune(state, scorer);/* deletes nothing */

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));//score=1
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));//score=4
    EXPECT_EQ("d", word);

    backend.update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//7

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));//score=0
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));//score=5
    EXPECT_EQ("d", word);

    backend.prune(state, scorer);/* deletes a-b */

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"a"}, word));//notfound
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend.get_prev(state, selector, scorer, {"b"}, word));//notfound
    EXPECT_EQ(IBackend::LINE_START, word);

    EXPECT_TRUE(backend.get_next(state, selector, scorer, {"c"}, word));//score=5
    EXPECT_EQ("d", word);

    EXPECT_TRUE(backend.get_random(state, scorer, word));
    EXPECT_NE(IBackend::LINE_END, word);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
