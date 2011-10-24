#include <gtest/gtest.h>
#include <marky/selector.h>

using namespace marky;

#define INIT_STATE(LINKS, SCORER, STATE)		\
	links_t LINKS(new _links_t());				\
	scorer_t SCORER = marky::scorers::no_adj();	\
	state_t STATE(new _state_t(0,0));

void check_distribution(marky::selector_t sel,
		size_t score_a, size_t score_b, size_t score_c,
		double distrib_a, double distrib_b, double distrib_c) {
	INIT_STATE(links, scorer, state);

	link_t a(new Link("a", "b", 0, 0, score_a)),
		b(new Link("b", "c", 0, 0, score_b)),
		c(new Link("c", "a", 0, 0, score_c));
	links->push_back(a);
	links->push_back(b);
	links->push_back(c);

	/* repeat sel() a bunch, check output distribution */
	const size_t pick_count = 1000;
	size_t picked_a = 0, picked_b = 0, picked_c = 0;
	for (size_t i = 0; i < pick_count; ++i) {
		link_t picked = sel(links, scorer, state);
		if (picked == a) {
			++picked_a;
		} else if (picked == b) {
			++picked_b;
		} else if (picked == c) {
			++picked_c;
		} else {
			EXPECT_TRUE(false);
		}
	}

	std::cout << "a=" << picked_a << " b=" << picked_b << " c=" << picked_c << std::endl;
	/* give a wide range to avoid false errors due to randomness */
	EXPECT_NEAR(picked_a / (double)pick_count, distrib_a, 0.1);
	EXPECT_NEAR(picked_b / (double)pick_count, distrib_b, 0.1);
	EXPECT_NEAR(picked_c / (double)pick_count, distrib_c, 0.1);
}

// -- BEST ALWAYS

TEST(BestAlways, empty) {
	INIT_STATE(links, scorer, state);
	marky::selector_t sel = marky::selectors::best_always();

	EXPECT_TRUE(sel(links, scorer, state) == link_t());
}

TEST(BestAlways, one) {
	INIT_STATE(links, scorer, state);
	marky::selector_t sel = marky::selectors::best_always();

	links->push_back(link_t(new Link("a", "b", 0, 0, 1)));
	link_t pickme = links->back();

	EXPECT_TRUE(sel(links, scorer, state) == pickme);
}

TEST(BestAlways, many) {
	INIT_STATE(links, scorer, state);
	marky::selector_t sel = marky::selectors::best_always();

	links->push_back(link_t(new Link("a", "b", 0, 0, 1)));
	links->push_back(link_t(new Link("b", "c", 0, 0, 2)));
	links->push_back(link_t(new Link("c", "a", 0, 0, 3)));
	link_t pickme = links->back();

	EXPECT_TRUE(sel(links, scorer, state) == pickme);
}

// -- RANDOM

TEST(Random, empty) {
	INIT_STATE(links, scorer, state);
	marky::selector_t sel = marky::selectors::random();

	EXPECT_TRUE(sel(links, scorer, state) == link_t());
}

TEST(Random, one) {
	INIT_STATE(links, scorer, state);
	marky::selector_t sel = marky::selectors::random();

	links->push_back(link_t(new Link("a", "b", 0, 0, 1)));
	link_t pickme = links->back();

	EXPECT_TRUE(sel(links, scorer, state) == pickme);
}

TEST(Random, many) {
	marky::selector_t sel = marky::selectors::random();
	check_distribution(sel, 1, 2, 3, 1./3, 1./3, 1./3);
}

// -- BEST WEIGHTED

TEST(BestWeighted, empty) {
	INIT_STATE(links, scorer, state);
	marky::selector_t sel = marky::selectors::best_weighted();

	EXPECT_TRUE(sel(links, scorer, state) == link_t());
}

TEST(BestWeighted, one) {
	INIT_STATE(links, scorer, state);
	marky::selector_t sel = marky::selectors::best_weighted();

	links->push_back(link_t(new Link("a", "b", 0, 0, 1)));
	link_t pickme = links->back();

	EXPECT_TRUE(sel(links, scorer, state) == pickme);
}

TEST(BestWeighted, many) {
	marky::selector_t sel = marky::selectors::best_weighted();
	check_distribution(sel, 1, 2, 3, 1./6, 2./6, 3./6);
}

TEST(BestWeighted, maxfactor) {
	/* shortcuts to best_always */
	marky::selector_t sel = marky::selectors::best_weighted(255);
	check_distribution(sel, 1, 2, 3, 0, 0, 1);
}

TEST(BestWeighted, minfactor) {
	/* shortcuts to random */
	marky::selector_t sel = marky::selectors::best_weighted(0);
	check_distribution(sel, 1, 2, 3, 1./3, 1./3, 1./3);
}

TEST(BestWeighted, DISABLED_bigfactor) {//TODO
	/* avoid best_always shortcut */
	marky::selector_t sel = marky::selectors::best_weighted(254);
	check_distribution(sel, 1, 2, 3, 0, 0, 1);
}

TEST(BestWeighted, DISABLED_smallfactor) {//TODO
	/* avoid random shortcut */
	marky::selector_t sel = marky::selectors::best_weighted(1);
	check_distribution(sel, 1, 2, 3, 1./3, 1./3, 1./3);
}


int main(int argc, char **argv) {
	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}
