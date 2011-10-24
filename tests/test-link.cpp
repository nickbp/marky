#include <gtest/gtest.h>
#include <marky/scorer.h>

using namespace marky;

#define STATE(state, num)						\
	state->time = num;							\
	state->link = num;

TEST(Link, score_get_noadj) {
	scorer_t scorer = scorers::no_adj();
	Link link("a", "b", 0, 0);/* initial score = 1 */

	state_t state(new _state_t(0,0));
	EXPECT_EQ(1, link.score(scorer, state));

	STATE(state, 500);
	EXPECT_EQ(1, link.score(scorer, state));
}

TEST(Link, score_get_linkadj) {
	scorer_t scorer = scorers::link_adj(5);
	Link link("a", "b", 0, 0);/* initial score = 1 */

	state_t state(new _state_t(0,0));
	EXPECT_EQ(1, link.score(scorer, state));

	STATE(state, 4);
	EXPECT_EQ(1, link.score(scorer, state));

	STATE(state, 5);
	EXPECT_EQ(0, link.score(scorer, state));

	STATE(state, 500);
	EXPECT_EQ(0, link.score(scorer, state));
}

TEST(Link, score_inc_noadj) {
	scorer_t scorer = scorers::no_adj();
	Link link("a", "b", 0, 0);/* initial score = 1 */

	state_t state(new _state_t(0,0));
	EXPECT_EQ(1, link.score(scorer, state));

	link.increment(scorer, state);

	STATE(state, 500);
	EXPECT_EQ(2, link.score(scorer, state));
}

TEST(Link, score_inc_linkadj) {
	scorer_t scorer = scorers::link_adj(5);/* lose a point after 5s of no activity */
	Link link("a", "b", 0, 0);/* initial score = 1 */

	state_t state(new _state_t(0,0));
	EXPECT_EQ(1, link.score(scorer, state));

	link.increment(scorer, state);
	EXPECT_EQ(2, link.score(scorer, state));

	STATE(state, 4);
	EXPECT_EQ(2, link.score(scorer, state));

	STATE(state, 5);
	EXPECT_EQ(1, link.score(scorer, state));/* -1 */

	link.increment(scorer, state);/* +1, set state clock to 5 */
	EXPECT_EQ(2, link.score(scorer, state));

	STATE(state, 9);
	EXPECT_EQ(2, link.score(scorer, state));

	STATE(state, 10);
	EXPECT_EQ(1, link.score(scorer, state));/* -1 */

	STATE(state, 14);
	EXPECT_EQ(1, link.score(scorer, state));

	STATE(state, 15);
	EXPECT_EQ(0, link.score(scorer, state));/* -1 */



	STATE(state, 17);
	link.increment(scorer, state);/* +1, set state clock to 17 */
	EXPECT_EQ(1, link.score(scorer, state));

	STATE(state, 21);
	EXPECT_EQ(1, link.score(scorer, state));

	STATE(state, 22);
	EXPECT_EQ(0, link.score(scorer, state));



	STATE(state, 26);
	link.increment(scorer, state);/* +1, set state clock to 26 */
	STATE(state, 27);
	link.increment(scorer, state);/* +1, set state clock to 27 */
	STATE(state, 28);
	link.increment(scorer, state);/* +1, set state clock to 28 */

	STATE(state, 32);
	EXPECT_EQ(3, link.score(scorer, state));
	STATE(state, 33);
	EXPECT_EQ(2, link.score(scorer, state));/* don't start decrementing until 28+5 */
	STATE(state, 37);
	EXPECT_EQ(2, link.score(scorer, state));
	STATE(state, 38);
	EXPECT_EQ(1, link.score(scorer, state));
	STATE(state, 42);
	EXPECT_EQ(1, link.score(scorer, state));
	STATE(state, 43);
	EXPECT_EQ(0, link.score(scorer, state));
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}
