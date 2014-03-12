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
#include <marky/backend-cache.h>
#include <marky/backend-sqlite.h>
#include <marky/config.h>
#include <unistd.h> //unlink()

using namespace marky;

#define SQLITE_DB_PATH "sqlite_test.db"

class SQLite : public testing::Test {
protected:
    /* called before every test */
    virtual void SetUp() {
        unlink(SQLITE_DB_PATH);
    }

    /* called after every test */
    virtual void TearDown() {
        unlink(SQLITE_DB_PATH);
    }
};

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

static void test_get_prev(backend_t backend, bool insert_2) {
    ASSERT_TRUE((bool)backend);
    scorer_t scorer = scorers::no_adj();
    selector_t selector = selectors::best_always();
    State state(0,0);

    word_t word;
    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"c", "a", "x"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"c", "a"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"c"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);

    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"a", "b", "x"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"a", "b"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"a"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);

    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b", "c", "x"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b", "c"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);
    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b"}, word));
    EXPECT_EQ(IBackend::LINE_START, word);

    if (insert_2) {
        init_data_2(state, *backend, scorer);

        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"g"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);

        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"c", "a", "x"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"c", "a"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"c"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);

        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"a", "b", "x"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"a"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);

        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b", "c", "x"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b", "c"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);
    } else {
        init_data_1(state, *backend, scorer);

        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"g"}, word));
        EXPECT_EQ(IBackend::LINE_START, word);

        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"c", "a", "x"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"c"}, word));
        EXPECT_EQ("b", word);

        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"a", "b", "x"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"a"}, word));
        EXPECT_EQ("c", word);

        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b", "c", "x"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b", "c"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b"}, word));
        EXPECT_EQ("a", word);
    }
}

TEST_F(SQLite, get_prev_1_direct) {
    backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
    test_get_prev(backend, false);
}
TEST_F(SQLite, get_prev_1_cached) {
    cacheable_t backend = Backend_SQLite::create_cacheable(SQLITE_DB_PATH);
    ASSERT_TRUE((bool)backend);
    backend_t cache(new Backend_Cache(backend));
    test_get_prev(cache, false);
}
TEST_F(SQLite, get_prev_2_direct) {
    backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
    test_get_prev(backend, true);
}
TEST_F(SQLite, get_prev_2_cached) {
    cacheable_t backend = Backend_SQLite::create_cacheable(SQLITE_DB_PATH);
    ASSERT_TRUE((bool)backend);
    backend_t cache(new Backend_Cache(backend));
    test_get_prev(cache, true);
}

static void test_get_next(backend_t backend, bool insert_2) {
    ASSERT_TRUE((bool)backend);
    scorer_t scorer = scorers::no_adj();
    selector_t selector = selectors::best_always();
    State state(0,0);

    word_t word;
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"x", "c", "a"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c", "a"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"x", "a", "b"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a", "b"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"b"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"x", "b", "c"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"b", "c"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));
    EXPECT_EQ(IBackend::LINE_END, word);

    if (insert_2) {
        init_data_2(state, *backend, scorer);

        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"g"}, word));
        EXPECT_EQ(IBackend::LINE_END, word);

        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"x", "c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));
        EXPECT_EQ(IBackend::LINE_END, word); /* no sub-entries are entered by the backend */

        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"x", "a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"b"}, word));
        EXPECT_EQ(IBackend::LINE_END, word);

        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"x", "b", "c"}, word));
        EXPECT_EQ("d", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"b", "c"}, word));
        EXPECT_EQ("d", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));
        EXPECT_EQ(IBackend::LINE_END, word);
    } else {
        init_data_1(state, *backend, scorer);

        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"g"}, word));
        EXPECT_EQ(IBackend::LINE_END, word);

        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"x", "c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c", "a"}, word));
        EXPECT_EQ("b", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));
        EXPECT_EQ("b", word);

        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"x", "a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a", "b"}, word));
        EXPECT_EQ("c", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"b"}, word));
        EXPECT_EQ("c", word);

        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"x", "b", "c"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"b", "c"}, word));
        EXPECT_EQ("a", word);
        EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));
        EXPECT_EQ("a", word);
    }
}

TEST_F(SQLite, get_next_1_direct) {
    backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
    test_get_next(backend, false);
}
TEST_F(SQLite, get_next_1_cached) {
    cacheable_t backend = Backend_SQLite::create_cacheable(SQLITE_DB_PATH);
    ASSERT_TRUE((bool)backend);
    backend_t cache(new Backend_Cache(backend));
    test_get_next(cache, false);
}
TEST_F(SQLite, get_next_2_direct) {
    backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
    test_get_next(backend, true);
}
TEST_F(SQLite, get_next_2_cached) {
    cacheable_t backend = Backend_SQLite::create_cacheable(SQLITE_DB_PATH);
    ASSERT_TRUE((bool)backend);
    backend_t cache(new Backend_Cache(backend));
    test_get_next(cache, true);
}

static void test_get_random(backend_t backend) {
    ASSERT_TRUE((bool)backend);
    /* each link loses a point if it's not updated within 2 increments */
    scorer_t scorer = scorers::word_adj(2);
    State state(2,2);

    word_t rand;
    EXPECT_TRUE(backend->get_random(state, scorer, rand));
    EXPECT_EQ(IBackend::LINE_END, rand);

    /* just add one link, since this is truly random */
    ASSERT_TRUE(backend->update_snippets(state, scorer, to_map({"c", "d"})));

    EXPECT_TRUE(backend->get_random(state, scorer, rand));
    EXPECT_NE(IBackend::LINE_END, rand);
}

TEST_F(SQLite, get_random_direct) {
    backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
    test_get_random(backend);
}
TEST_F(SQLite, get_random_cached) {
    cacheable_t backend = Backend_SQLite::create_cacheable(SQLITE_DB_PATH);
    ASSERT_TRUE((bool)backend);
    backend_t cache(new Backend_Cache(backend));
    test_get_random(cache);
}

#define INC_STATE(STATE) DEBUG("INC %lu", state.count); ++state.time; ++state.count;

static void test_scoreadj_prune(backend_t backend) {
    ASSERT_TRUE((bool)backend);
    /* each word loses a point if it's not updated within 2 increments */
    scorer_t scorer = scorers::word_adj(2);

    State state(0,0);
    backend->update_snippets(state, scorer, to_map({"a", "b"}));
    INC_STATE(state);//0
    backend->update_snippets(state, scorer, to_map({"a", "b"}));
    INC_STATE(state);//1
    backend->update_snippets(state, scorer, to_map({"a", "b"}));
    INC_STATE(state);//2

    selector_t selector = selectors::best_always();
    word_t word;

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));//score=3
    EXPECT_EQ("b", word);

    backend->update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//3

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));//score=2
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));//score=1
    EXPECT_EQ("d", word);

    backend->update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//4

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));//score=2
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));//score=2
    EXPECT_EQ("d", word);

    backend->update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//5

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));//score=1
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));//score=3
    EXPECT_EQ("d", word);

    backend->update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//6

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));//score=1
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));//score=4
    EXPECT_EQ("d", word);

    backend->prune(state, scorer);/* deletes nothing */

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));//score=1
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));//score=4
    EXPECT_EQ("d", word);

    backend->update_snippets(state, scorer, to_map({"c", "d"}));
    INC_STATE(state);//7

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));//score=0
    EXPECT_EQ("b", word);
    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));//score=5
    EXPECT_EQ("d", word);

    backend->prune(state, scorer);/* deletes a-b */

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"a"}, word));//notfound
    EXPECT_EQ(IBackend::LINE_END, word);
    EXPECT_TRUE(backend->get_prev(state, selector, scorer, {"b"}, word));//notfound
    EXPECT_EQ(IBackend::LINE_START, word);

    EXPECT_TRUE(backend->get_next(state, selector, scorer, {"c"}, word));//score=5
    EXPECT_EQ("d", word);

    EXPECT_TRUE(backend->get_random(state, scorer, word));
    EXPECT_NE(IBackend::LINE_END, word);
}

TEST_F(SQLite, scoreadj_prune_direct) {
    backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
    test_scoreadj_prune(backend);
}
TEST_F(SQLite, scoreadj_prune_cached) {
    cacheable_t backend = Backend_SQLite::create_cacheable(SQLITE_DB_PATH);
    ASSERT_TRUE((bool)backend);
    backend_t cache(new Backend_Cache(backend));
    test_scoreadj_prune(cache);
}

static void test_get_prevs(bool insert_2) {
    cacheable_t backend = Backend_SQLite::create_cacheable(SQLITE_DB_PATH);
    ASSERT_TRUE((bool)backend);
    scorer_t scorer = scorers::no_adj();
    State state(0,0);

    snippet_ptr_set_t snippets;
    EXPECT_TRUE(backend->get_prevs({"c", "a", "x"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_prevs({"c", "a"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_prevs({"c"}, snippets));
    EXPECT_TRUE(snippets.empty());

    EXPECT_TRUE(backend->get_prevs({"a", "b", "x"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_prevs({"a", "b"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_prevs({"a"}, snippets));
    EXPECT_TRUE(snippets.empty());

    EXPECT_TRUE(backend->get_prevs({"b", "c", "x"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_prevs({"b", "c"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_prevs({"b"}, snippets));
    EXPECT_TRUE(snippets.empty());

    if (insert_2) {
        init_data_2(state, *backend, scorer);

        EXPECT_TRUE(backend->get_prevs({"g"}, snippets));
        EXPECT_TRUE(snippets.empty());

        EXPECT_TRUE(backend->get_prevs({"c", "a", "x"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"c", "a"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"c"}, snippets));
        EXPECT_TRUE(snippets.empty());

        EXPECT_TRUE(backend->get_prevs({"a", "b", "x"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"a", "b"}, snippets));
        ASSERT_EQ(1, snippets.size());
        ASSERT_EQ(3, (*snippets.begin())->words.size());
        EXPECT_EQ("c", (*snippets.begin())->words.front());
        EXPECT_TRUE(backend->get_prevs({"a"}, snippets));
        EXPECT_TRUE(snippets.empty());

        EXPECT_TRUE(backend->get_prevs({"b", "c", "x"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"b", "c"}, snippets));
        ASSERT_EQ(1, snippets.size());
        ASSERT_EQ(3, (*snippets.begin())->words.size());
        EXPECT_EQ("a", (*snippets.begin())->words.front());
        EXPECT_TRUE(backend->get_prevs({"b"}, snippets));
        EXPECT_TRUE(snippets.empty());
    } else {
        init_data_1(state, *backend, scorer);

        EXPECT_TRUE(backend->get_prevs({"g"}, snippets));
        EXPECT_TRUE(snippets.empty());

        EXPECT_TRUE(backend->get_prevs({"c", "a", "x"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"c", "a"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"c"}, snippets));
        EXPECT_EQ(2, snippets.size());

        EXPECT_TRUE(backend->get_prevs({"a", "b", "x"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"a", "b"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"a"}, snippets));
        ASSERT_EQ(1, snippets.size());
        ASSERT_EQ(2, (*snippets.begin())->words.size());
        EXPECT_EQ("c", (*snippets.begin())->words.front());

        EXPECT_TRUE(backend->get_prevs({"b", "c", "x"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"b", "c"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_prevs({"b"}, snippets));
        ASSERT_EQ(1, snippets.size());
        ASSERT_EQ(2, (*snippets.begin())->words.size());
        EXPECT_EQ("a", (*snippets.begin())->words.front());
    }
}

TEST_F(SQLite, get_prevs_1) {
    test_get_prevs(false);
}
TEST_F(SQLite, get_prevs_2) {
    test_get_prevs(true);
}

static void test_get_nexts(bool insert_2) {
    cacheable_t backend = Backend_SQLite::create_cacheable(SQLITE_DB_PATH);
    ASSERT_TRUE((bool)backend);
    scorer_t scorer = scorers::no_adj();
    State state(0,0);

    snippet_ptr_set_t snippets;
    EXPECT_TRUE(backend->get_nexts({"x", "c", "a"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_nexts({"c", "a"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_nexts({"a"}, snippets));
    EXPECT_TRUE(snippets.empty());

    EXPECT_TRUE(backend->get_nexts({"x", "a", "b"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_nexts({"a", "b"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_nexts({"b"}, snippets));
    EXPECT_TRUE(snippets.empty());

    EXPECT_TRUE(backend->get_nexts({"x", "b", "c"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_nexts({"b", "c"}, snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_nexts({"c"}, snippets));
    EXPECT_TRUE(snippets.empty());

    if (insert_2) {
        init_data_2(state, *backend, scorer);

        EXPECT_TRUE(backend->get_nexts({"g"}, snippets));
        EXPECT_TRUE(snippets.empty());

        EXPECT_TRUE(backend->get_nexts({"x", "c", "a"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_nexts({"c", "a"}, snippets));
        ASSERT_EQ(1, snippets.size());
        ASSERT_EQ(3, (*snippets.begin())->words.size());
        EXPECT_EQ("b", (*snippets.begin())->words.back());
        EXPECT_TRUE(backend->get_nexts({"a"}, snippets));
        EXPECT_TRUE(snippets.empty()); /* no sub-entries are entered by the backend */

        EXPECT_TRUE(backend->get_nexts({"x", "a", "b"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_nexts({"a", "b"}, snippets));
        ASSERT_EQ(1, snippets.size());
        ASSERT_EQ(3, (*snippets.begin())->words.size());
        EXPECT_EQ("c", (*snippets.begin())->words.back());
        EXPECT_TRUE(backend->get_nexts({"b"}, snippets));
        EXPECT_TRUE(snippets.empty());

        EXPECT_TRUE(backend->get_nexts({"x", "b", "c"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_nexts({"b", "c"}, snippets));
        ASSERT_EQ(1, snippets.size());
        ASSERT_EQ(3, (*snippets.begin())->words.size());
        EXPECT_EQ("d", (*snippets.begin())->words.back());
        EXPECT_TRUE(backend->get_nexts({"c"}, snippets));
        EXPECT_TRUE(snippets.empty());
    } else {
        init_data_1(state, *backend, scorer);

        EXPECT_TRUE(backend->get_nexts({"g"}, snippets));
        EXPECT_TRUE(snippets.empty());

        EXPECT_TRUE(backend->get_nexts({"x", "c", "a"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_nexts({"c", "a"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_nexts({"a"}, snippets));
        EXPECT_EQ(2, snippets.size());

        EXPECT_TRUE(backend->get_nexts({"x", "a", "b"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_nexts({"a", "b"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_nexts({"b"}, snippets));
        ASSERT_EQ(1, snippets.size());
        ASSERT_EQ(2, (*snippets.begin())->words.size());
        EXPECT_EQ("c", (*snippets.begin())->words.back());

        EXPECT_TRUE(backend->get_nexts({"x", "b", "c"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_nexts({"b", "c"}, snippets));
        EXPECT_TRUE(snippets.empty());
        EXPECT_TRUE(backend->get_nexts({"c"}, snippets));
        ASSERT_EQ(1, snippets.size());
        ASSERT_EQ(2, (*snippets.begin())->words.size());
        EXPECT_EQ("a", (*snippets.begin())->words.back());
    }
}

TEST_F(SQLite, get_nexts_1) {
    test_get_nexts(false);
}
TEST_F(SQLite, get_nexts_2) {
    test_get_nexts(true);
}

TEST_F(SQLite, get_snippets) {
    cacheable_t backend = Backend_SQLite::create_cacheable(SQLITE_DB_PATH);
    ASSERT_TRUE((bool)backend);
    scorer_t scorer = scorers::no_adj();
    State state(0,0);

    ICacheable::words_to_snippet_t snippets;
    EXPECT_TRUE(backend->get_snippets(to_map({"a", "b"}), snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_snippets(to_map({"a", "c"}), snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_snippets(to_map({"b", "c"}), snippets));
    EXPECT_TRUE(snippets.empty());
    EXPECT_TRUE(backend->get_snippets(to_map({"c", "a"}), snippets));
    EXPECT_TRUE(snippets.empty());

    init_data_1(state, *backend, scorer);

    EXPECT_TRUE(backend->get_snippets(to_map({"a", "b"}), snippets));
    ASSERT_EQ(1, snippets.size());
    ASSERT_EQ(2, snippets.begin()->second->words.size());
    EXPECT_EQ("b", snippets.begin()->second->words.back());
    EXPECT_TRUE(backend->get_snippets(to_map({"a", "c"}), snippets));
    ASSERT_EQ(1, snippets.size());
    ASSERT_EQ(2, snippets.begin()->second->words.size());
    EXPECT_EQ("c", snippets.begin()->second->words.back());
    EXPECT_TRUE(backend->get_snippets(to_map({"b", "c"}), snippets));
    ASSERT_EQ(1, snippets.size());
    ASSERT_EQ(2, snippets.begin()->second->words.size());
    EXPECT_EQ("c", snippets.begin()->second->words.back());
    EXPECT_TRUE(backend->get_snippets(to_map({"c", "a"}), snippets));
    ASSERT_EQ(1, snippets.size());
    ASSERT_EQ(2, snippets.begin()->second->words.size());
    EXPECT_EQ("a", snippets.begin()->second->words.back());

    marky::words_to_counts::map_t map;
    map[{"a", "b"}] = 1;
    map[{"a", "c"}] = 1;
    map[{"b", "x"}] = 1;
    map[{"b", "c"}] = 1;
    map[{"c", "a"}] = 1;
    EXPECT_TRUE(backend->get_snippets(map, snippets));
    ASSERT_EQ(4, snippets.size());

    ICacheable::words_to_snippet_t::const_iterator iter = snippets.find({"a","b"});
    ASSERT_TRUE(iter != snippets.end());
    EXPECT_EQ("a", iter->second->words.front());
    EXPECT_EQ("b", iter->second->words.back());

    iter = snippets.find({"a","c"});
    ASSERT_TRUE(iter != snippets.end());
    EXPECT_EQ("a", iter->second->words.front());
    EXPECT_EQ("c", iter->second->words.back());

    iter = snippets.find({"b","x"});
    ASSERT_TRUE(iter == snippets.end());

    iter = snippets.find({"b","c"});
    ASSERT_TRUE(iter != snippets.end());
    EXPECT_EQ("b", iter->second->words.front());
    EXPECT_EQ("c", iter->second->words.back());

    iter = snippets.find({"c","a"});
    ASSERT_TRUE(iter != snippets.end());
    EXPECT_EQ("c", iter->second->words.front());
    EXPECT_EQ("a", iter->second->words.back());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
