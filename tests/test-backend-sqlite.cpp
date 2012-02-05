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
#include <marky/backend-sqlite.h>
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

#define INIT_STATE(backend, scorer)							\
	ASSERT_TRUE(backend->increment_link(scorer, "a", "b"));	\
	ASSERT_TRUE(backend->increment_link(scorer, "a", "b"));	\
	ASSERT_TRUE(backend->increment_link(scorer, "a", "b"));	\
	ASSERT_TRUE(backend->increment_link(scorer, "a", "c"));	\
	ASSERT_TRUE(backend->increment_link(scorer, "b", "c"));	\
	ASSERT_TRUE(backend->increment_link(scorer, "b", "c"));	\
	ASSERT_TRUE(backend->increment_link(scorer, "c", "a"));

#define CHECK_LINK(SCORER, STATE, LINK, PREV, NEXT, SCORE)	\
	ASSERT_TRUE((bool)LINK);								\
	EXPECT_EQ(PREV, LINK->prev);							\
	EXPECT_EQ(NEXT, LINK->next);							\
	EXPECT_EQ(SCORE, LINK->score(SCORER, STATE));

TEST_F(SQLite, get_prev) {
	backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
	ASSERT_TRUE((bool)backend);
	scorer_t scorer = scorers::no_adj();
	selector_t selector = selectors::best_always();

	link_t link;
	EXPECT_TRUE(backend->get_next(selector, scorer, "a", link));
	EXPECT_FALSE((bool)link);
	EXPECT_TRUE(backend->get_next(selector, scorer, "b", link));
	EXPECT_FALSE((bool)link);
	EXPECT_TRUE(backend->get_next(selector, scorer, "c", link));
	EXPECT_FALSE((bool)link);

	INIT_STATE(backend, scorer);

	state_t state(new _state_t(0,0));

	EXPECT_TRUE(backend->get_prev(selector, scorer, "a", link));
	CHECK_LINK(scorer, state, link, "a", "b", 3);

	EXPECT_TRUE(backend->get_prev(selector, scorer, "b", link));
	CHECK_LINK(scorer, state, link, "b", "c", 2);

	EXPECT_TRUE(backend->get_prev(selector, scorer, "c", link));
	CHECK_LINK(scorer, state, link, "c", "a", 1);
}

TEST_F(SQLite, get_next) {
	backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
	ASSERT_TRUE((bool)backend);
	scorer_t scorer = scorers::no_adj();
	selector_t selector = selectors::best_always();

	link_t link;
	EXPECT_TRUE(backend->get_next(selector, scorer, "a", link));
	EXPECT_FALSE((bool)link);
	EXPECT_TRUE(backend->get_next(selector, scorer, "b", link));
	EXPECT_FALSE((bool)link);
	EXPECT_TRUE(backend->get_next(selector, scorer, "c", link));
	EXPECT_FALSE((bool)link);

	INIT_STATE(backend, scorer);

	state_t state(new _state_t(0,0));

	EXPECT_TRUE(backend->get_next(selector, scorer, "a", link));
	CHECK_LINK(scorer, state, link, "c", "a", 1);

	EXPECT_TRUE(backend->get_next(selector, scorer, "b", link));
	CHECK_LINK(scorer, state, link, "a", "b", 3);

	EXPECT_TRUE(backend->get_next(selector, scorer, "c", link));
	CHECK_LINK(scorer, state, link, "b", "c", 2);
}

TEST_F(SQLite, get_random) {
	backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
	ASSERT_TRUE((bool)backend);

	link_t rand;
	EXPECT_TRUE(backend->get_random(rand));
	EXPECT_FALSE((bool)rand);

	/* each link loses a point if it's not updated within 2 increments */
	scorer_t scorer = scorers::link_adj(2);

	/* just add one link, since this is truly random */
	ASSERT_TRUE(backend->increment_link(scorer, "c", "d"));
	state_t state(new _state_t(2,2));

	EXPECT_TRUE(backend->get_random(rand));
	EXPECT_TRUE((bool)rand);
	CHECK_LINK(scorer, state, rand, "c", "d", 0);
}

#define INC_STATE(STATE) ++state->time; ++state->link;

TEST_F(SQLite, scoreadj_prune) {
	backend_t backend = Backend_SQLite::create_backend(SQLITE_DB_PATH);
	ASSERT_TRUE((bool)backend);
	/* each link loses a point if it's not updated within 2 increments */
	scorer_t scorer = scorers::link_adj(2);

	backend->increment_link(scorer, "a", "b");
	backend->increment_link(scorer, "a", "b");
	backend->increment_link(scorer, "a", "b");
	state_t state(new _state_t(3,3));

	selector_t selector = selectors::best_always();
	link_t link;

	EXPECT_TRUE(backend->get_prev(selector, scorer, "a", link));
	CHECK_LINK(scorer, state, link, "a", "b", 3);

	backend->increment_link(scorer, "c", "d");
	INC_STATE(state)/* keep our state in sync with backend's state */

	EXPECT_TRUE(backend->get_prev(selector, scorer, "a", link));
	CHECK_LINK(scorer, state, link, "a", "b", 2);
	EXPECT_TRUE(backend->get_prev(selector, scorer, "c", link));
	CHECK_LINK(scorer, state, link, "c", "d", 1);

	backend->increment_link(scorer, "c", "d");
	INC_STATE(state)

	EXPECT_TRUE(backend->get_prev(selector, scorer, "a", link));
	CHECK_LINK(scorer, state, link, "a", "b", 2);
	EXPECT_TRUE(backend->get_prev(selector, scorer, "c", link));
	CHECK_LINK(scorer, state, link, "c", "d", 2);

	backend->increment_link(scorer, "c", "d");
	INC_STATE(state)

	EXPECT_TRUE(backend->get_prev(selector, scorer, "a", link));
	CHECK_LINK(scorer, state, link, "a", "b", 1);
	EXPECT_TRUE(backend->get_prev(selector, scorer, "c", link));
	CHECK_LINK(scorer, state, link, "c", "d", 3);

	backend->increment_link(scorer, "c", "d");
	INC_STATE(state)

	EXPECT_TRUE(backend->get_prev(selector, scorer, "a", link));
	CHECK_LINK(scorer, state, link, "a", "b", 1);
	EXPECT_TRUE(backend->get_prev(selector, scorer, "c", link));
	CHECK_LINK(scorer, state, link, "c", "d", 4);

	backend->prune(scorer);/* deletes nothing */

	EXPECT_TRUE(backend->get_prev(selector, scorer, "a", link));
	CHECK_LINK(scorer, state, link, "a", "b", 1);
	EXPECT_TRUE(backend->get_prev(selector, scorer, "c", link));
	CHECK_LINK(scorer, state, link, "c", "d", 4);

	backend->increment_link(scorer, "c", "d");
	INC_STATE(state)

	EXPECT_TRUE(backend->get_prev(selector, scorer, "a", link));
	CHECK_LINK(scorer, state, link, "a", "b", 0);
	EXPECT_TRUE(backend->get_prev(selector, scorer, "c", link));
	CHECK_LINK(scorer, state, link, "c", "d", 5);

	backend->prune(scorer);/* deletes a-b */

	EXPECT_TRUE(backend->get_prev(selector, scorer, "a", link));
	EXPECT_FALSE((bool)link);
	EXPECT_TRUE(backend->get_next(selector, scorer, "b", link));
	EXPECT_FALSE((bool)link);

	EXPECT_TRUE(backend->get_prev(selector, scorer, "c", link));
	CHECK_LINK(scorer, state, link, "c", "d", 5);

	EXPECT_TRUE(backend->get_random(link));
	CHECK_LINK(scorer, state, link, "c", "d", 5);
}

//TODO tests against icacheable functs

int main(int argc, char **argv) {
	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}
