#include <gtest/gtest.h>
#include <marky/scorer.h>

#include <math.h> //ceil()

using namespace marky;

#define INIT_STATE(LAST_STATE, THIS_STATE, seconds, links)	\
	_state_t LAST_STATE(0, 0), THIS_STATE(seconds, links);

// -- NO ADJ

TEST(NoAdj, no_adj) {
	scorer_t scorer = scorers::no_adj();
	INIT_STATE(last_state, this_state, 20, 20);

	EXPECT_EQ(0, scorer(0, last_state, this_state));
	EXPECT_EQ(50, scorer(50, last_state, this_state));
	EXPECT_EQ(324, scorer(324, last_state, this_state));
}

// -- LINK ADJ

TEST(LinkAdj, link_adj_zero) {
	/* should be same as no_adj */
	scorer_t scorer = scorers::link_adj(0);
	INIT_STATE(last_state, this_state, 20, 20);

	EXPECT_EQ(0, scorer(0, last_state, this_state));
	EXPECT_EQ(50, scorer(50, last_state, this_state));
	EXPECT_EQ(324, scorer(324, last_state, this_state));
}

TEST(LinkAdj, link_adj_one) {
	scorer_t scorer = scorers::link_adj(1);
	INIT_STATE(last_state, this_state, 20, 20);

	EXPECT_EQ(0, scorer(0, last_state, this_state));
	EXPECT_EQ(50-20, scorer(50, last_state, this_state));
	EXPECT_EQ(324-20, scorer(324, last_state, this_state));
}

TEST(LinkAdj, link_adj_five) {
	scorer_t scorer = scorers::link_adj(5);
	INIT_STATE(last_state, this_state, 20, 20);

	EXPECT_EQ(0, scorer(0, last_state, this_state));
	EXPECT_EQ(floor(50-(20/5.)), scorer(50, last_state, this_state));
	EXPECT_EQ(floor(324-(20/5.)), scorer(324, last_state, this_state));
}

TEST(LinkAdj, link_adj_seven) {
	scorer_t scorer = scorers::link_adj(7);

	/* check that score is decremented only when 7 is reached */

	{
		INIT_STATE(last_state, this_state, 1, 1);

		EXPECT_EQ(0, scorer(0, last_state, this_state));
		EXPECT_EQ(50, scorer(50, last_state, this_state));
		EXPECT_EQ(324, scorer(324, last_state, this_state));
	}

	{
		INIT_STATE(last_state, this_state, 6, 6);

		EXPECT_EQ(0, scorer(0, last_state, this_state));
		EXPECT_EQ(50, scorer(50, last_state, this_state));
		EXPECT_EQ(324, scorer(324, last_state, this_state));
	}

	{
		INIT_STATE(last_state, this_state, 7, 7);

		EXPECT_EQ(0, scorer(0, last_state, this_state));
		EXPECT_EQ(49, scorer(50, last_state, this_state));
		EXPECT_EQ(323, scorer(324, last_state, this_state));
	}

	{
		INIT_STATE(last_state, this_state, 8, 8);

		EXPECT_EQ(0, scorer(0, last_state, this_state));
		EXPECT_EQ(49, scorer(50, last_state, this_state));
		EXPECT_EQ(323, scorer(324, last_state, this_state));
	}

	{
		INIT_STATE(last_state, this_state, 20, 20);

		EXPECT_EQ(0, scorer(0, last_state, this_state));
		EXPECT_EQ(ceil(50-(20/7.)), scorer(50, last_state, this_state));
		EXPECT_EQ(ceil(324-(20/7.)), scorer(324, last_state, this_state));
	}
}

// -- TIME ADJ

TEST(TimeAdj, time_adj_zero) {
	/* should be same as no_adj */
	scorer_t scorer = scorers::time_adj(0);
	INIT_STATE(last_state, this_state, 20, 20);

	EXPECT_EQ(0, scorer(0, last_state, this_state));
	EXPECT_EQ(50, scorer(50, last_state, this_state));
	EXPECT_EQ(324, scorer(324, last_state, this_state));
}

TEST(TimeAdj, time_adj_one) {
	scorer_t scorer = scorers::time_adj(1);
	INIT_STATE(last_state, this_state, 20, 20);

	EXPECT_EQ(0, scorer(0, last_state, this_state));
	EXPECT_EQ(50-20, scorer(50, last_state, this_state));
	EXPECT_EQ(324-20, scorer(324, last_state, this_state));
}

TEST(TimeAdj, time_adj_five) {
	scorer_t scorer = scorers::time_adj(5);
	INIT_STATE(last_state, this_state, 20, 20);

	EXPECT_EQ(0, scorer(0, last_state, this_state));
	EXPECT_EQ(floor(50-(20/5.)), scorer(50, last_state, this_state));
	EXPECT_EQ(floor(324-(20/5.)), scorer(324, last_state, this_state));
}

TEST(TimeAdj, time_adj_seven) {
	scorer_t scorer = scorers::time_adj(7);
	INIT_STATE(last_state, this_state, 20, 20);

	EXPECT_EQ(0, scorer(0, last_state, this_state));
	EXPECT_EQ(ceil(50-(20/7.)), scorer(50, last_state, this_state));
	EXPECT_EQ(ceil(324-(20/7.)), scorer(324, last_state, this_state));
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}
