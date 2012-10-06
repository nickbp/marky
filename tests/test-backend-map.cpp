/*
  marky - A Markov chain generator.
  Copyright (C) 2011-2012  Nicholas Parker

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

using namespace marky;

#define INIT_STATE(backend, scorer)                            \
    ASSERT_TRUE(backend.increment_link(scorer, "a", "b"));    \
    ASSERT_TRUE(backend.increment_link(scorer, "a", "b"));    \
    ASSERT_TRUE(backend.increment_link(scorer, "a", "b"));    \
    ASSERT_TRUE(backend.increment_link(scorer, "a", "c"));    \
    ASSERT_TRUE(backend.increment_link(scorer, "b", "c"));    \
    ASSERT_TRUE(backend.increment_link(scorer, "b", "c"));    \
    ASSERT_TRUE(backend.increment_link(scorer, "c", "a"));

#define CHECK_LINK(SCORER, STATE, LINK, PREV, NEXT, SCORE)    \
    ASSERT_TRUE((bool)LINK);                                \
    EXPECT_EQ(PREV, LINK->prev);                            \
    EXPECT_EQ(NEXT, LINK->next);                            \
    EXPECT_EQ(SCORE, LINK->score(SCORER, STATE));

TEST(Map, get_prev) {
    Backend_Map backend;
    scorer_t scorer = scorers::no_adj();
    selector_t selector = selectors::best_always();

    link_t link;
    EXPECT_TRUE(backend.get_next(selector, scorer, "a", link));
    EXPECT_FALSE((bool)link);
    EXPECT_TRUE(backend.get_next(selector, scorer, "b", link));
    EXPECT_FALSE((bool)link);
    EXPECT_TRUE(backend.get_next(selector, scorer, "c", link));
    EXPECT_FALSE((bool)link);

    INIT_STATE(backend, scorer);

    state_t state(new _state_t(0,0));

    EXPECT_TRUE(backend.get_prev(selector, scorer, "a", link));
    CHECK_LINK(scorer, state, link, "a", "b", 3);

    EXPECT_TRUE(backend.get_prev(selector, scorer, "b", link));
    CHECK_LINK(scorer, state, link, "b", "c", 2);

    EXPECT_TRUE(backend.get_prev(selector, scorer, "c", link));
    CHECK_LINK(scorer, state, link, "c", "a", 1);
}

TEST(Map, get_next) {
    Backend_Map backend;
    scorer_t scorer = scorers::no_adj();
    selector_t selector = selectors::best_always();

    link_t link;
    EXPECT_TRUE(backend.get_next(selector, scorer, "a", link));
    EXPECT_FALSE((bool)link);
    EXPECT_TRUE(backend.get_next(selector, scorer, "b", link));
    EXPECT_FALSE((bool)link);
    EXPECT_TRUE(backend.get_next(selector, scorer, "c", link));
    EXPECT_FALSE((bool)link);

    INIT_STATE(backend, scorer);

    state_t state(new _state_t(0,0));

    EXPECT_TRUE(backend.get_next(selector, scorer, "a", link));
    CHECK_LINK(scorer, state, link, "c", "a", 1);

    EXPECT_TRUE(backend.get_next(selector, scorer, "b", link));
    CHECK_LINK(scorer, state, link, "a", "b", 3);

    EXPECT_TRUE(backend.get_next(selector, scorer, "c", link));
    CHECK_LINK(scorer, state, link, "b", "c", 2);
}

TEST(Map, get_random) {
    Backend_Map backend;

    link_t rand;
    EXPECT_TRUE(backend.get_random(rand));
    EXPECT_FALSE((bool)rand);

    /* each link loses a point if it's not updated within 2 increments */
    scorer_t scorer = scorers::link_adj(2);

    /* add two links, assume we'll be getting back one of them */
    ASSERT_TRUE(backend.increment_link(scorer, "a", "b"));
    ASSERT_TRUE(backend.increment_link(scorer, "c", "d"));
    state_t state(new _state_t(2,2));

    EXPECT_TRUE(backend.get_random(rand));
    ASSERT_TRUE((bool)rand);
    if (rand->prev == "a") {
        EXPECT_EQ("b", rand->next);
        EXPECT_EQ(0, rand->score(scorer, state));
    } else {
        EXPECT_EQ("c", rand->prev);
        EXPECT_EQ("d", rand->next);
        EXPECT_EQ(1, rand->score(scorer, state));
    }
}

#define INC_STATE(STATE) ++state->time; ++state->link;

TEST(Map, scoreadj_prune) {
    Backend_Map backend;
    /* each link loses a point if it's not updated within 2 increments */
    scorer_t scorer = scorers::link_adj(2);

    backend.increment_link(scorer, "a", "b");
    backend.increment_link(scorer, "a", "b");
    backend.increment_link(scorer, "a", "b");
    state_t state(new _state_t(3,3));

    selector_t selector = selectors::best_always();
    link_t link;

    EXPECT_TRUE(backend.get_prev(selector, scorer, "a", link));
    CHECK_LINK(scorer, state, link, "a", "b", 3);

    backend.increment_link(scorer, "c", "d");
    INC_STATE(state)/* keep our state in sync with backend's state */

    EXPECT_TRUE(backend.get_prev(selector, scorer, "a", link));
    CHECK_LINK(scorer, state, link, "a", "b", 2);
    EXPECT_TRUE(backend.get_prev(selector, scorer, "c", link));
    CHECK_LINK(scorer, state, link, "c", "d", 1);

    backend.increment_link(scorer, "c", "d");
    INC_STATE(state)

    EXPECT_TRUE(backend.get_prev(selector, scorer, "a", link));
    CHECK_LINK(scorer, state, link, "a", "b", 2);
    EXPECT_TRUE(backend.get_prev(selector, scorer, "c", link));
    CHECK_LINK(scorer, state, link, "c", "d", 2);

    backend.increment_link(scorer, "c", "d");
    INC_STATE(state)

    EXPECT_TRUE(backend.get_prev(selector, scorer, "a", link));
    CHECK_LINK(scorer, state, link, "a", "b", 1);
    EXPECT_TRUE(backend.get_prev(selector, scorer, "c", link));
    CHECK_LINK(scorer, state, link, "c", "d", 3);

    backend.increment_link(scorer, "c", "d");
    INC_STATE(state)

    EXPECT_TRUE(backend.get_prev(selector, scorer, "a", link));
    CHECK_LINK(scorer, state, link, "a", "b", 1);
    EXPECT_TRUE(backend.get_prev(selector, scorer, "c", link));
    CHECK_LINK(scorer, state, link, "c", "d", 4);

    backend.prune(scorer);/* deletes nothing */

    EXPECT_TRUE(backend.get_prev(selector, scorer, "a", link));
    CHECK_LINK(scorer, state, link, "a", "b", 1);
    EXPECT_TRUE(backend.get_prev(selector, scorer, "c", link));
    CHECK_LINK(scorer, state, link, "c", "d", 4);

    backend.increment_link(scorer, "c", "d");
    INC_STATE(state)

    EXPECT_TRUE(backend.get_prev(selector, scorer, "a", link));
    CHECK_LINK(scorer, state, link, "a", "b", 0);
    EXPECT_TRUE(backend.get_prev(selector, scorer, "c", link));
    CHECK_LINK(scorer, state, link, "c", "d", 5);

    backend.prune(scorer);/* deletes a-b */

    EXPECT_TRUE(backend.get_prev(selector, scorer, "a", link));
    EXPECT_FALSE((bool)link);
    EXPECT_TRUE(backend.get_next(selector, scorer, "b", link));
    EXPECT_FALSE((bool)link);

    EXPECT_TRUE(backend.get_prev(selector, scorer, "c", link));
    CHECK_LINK(scorer, state, link, "c", "d", 5);

    EXPECT_TRUE(backend.get_random(link));
    CHECK_LINK(scorer, state, link, "c", "d", 5);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
