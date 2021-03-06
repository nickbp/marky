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
#include <marky/selector.h>

using namespace marky;

static snippet_t make_snippet(const word_t& vala, const word_t& valb, time_t time, size_t count, score_t score) {
    words_t words;
    words.push_back(vala);
    words.push_back(valb);
    return snippet_t(new Snippet(words, time, count, score));
}

#define INIT_STATE(SNIPPETS, SCORER, STATE)        \
    snippet_ptr_set_t SNIPPETS;                    \
    scorer_t SCORER = marky::scorers::no_adj();    \
    State STATE(0,0);

void check_distribution(marky::selector_t sel,
        size_t score_a, size_t score_b, size_t score_c,
        double distrib_a, double distrib_b, double distrib_c) {
    INIT_STATE(snippets, scorer, state);

    snippet_t a(make_snippet("a", "b", 0, 0, score_a)),
        b(make_snippet("b", "c", 0, 0, score_b)),
        c(make_snippet("c", "a", 0, 0, score_c));
    snippets.insert(a);
    snippets.insert(b);
    snippets.insert(c);

    /* repeat sel() a bunch, check output distribution */
    const size_t pick_count = 1000;
    size_t picked_a = 0, picked_b = 0, picked_c = 0;
    for (size_t i = 0; i < pick_count; ++i) {
        snippet_t picked = sel(snippets, scorer, state);
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
    INIT_STATE(snippets, scorer, state);
    marky::selector_t sel = marky::selectors::best_always();

    EXPECT_TRUE(sel(snippets, scorer, state) == snippet_t());
}

TEST(BestAlways, one) {
    INIT_STATE(snippets, scorer, state);
    marky::selector_t sel = marky::selectors::best_always();

    snippets.insert(make_snippet("a", "b", 0, 0, 1));
    snippet_t pickme = *snippets.begin();

    EXPECT_TRUE(sel(snippets, scorer, state) == pickme);
}

TEST(BestAlways, many) {
    INIT_STATE(snippets, scorer, state);
    marky::selector_t sel = marky::selectors::best_always();

    snippets.insert(make_snippet("a", "b", 0, 0, 1));
    snippets.insert(make_snippet("b", "c", 0, 0, 2));
    snippets.insert(make_snippet("c", "a", 0, 0, 3));
    snippet_t pickme = *snippets.begin();

    EXPECT_TRUE(sel(snippets, scorer, state) == pickme);
}

// -- RANDOM

TEST(Random, empty) {
    INIT_STATE(snippets, scorer, state);
    marky::selector_t sel = marky::selectors::random();

    EXPECT_TRUE(sel(snippets, scorer, state) == snippet_t());
}

TEST(Random, one) {
    INIT_STATE(snippets, scorer, state);
    marky::selector_t sel = marky::selectors::random();

    snippets.insert(make_snippet("a", "b", 0, 0, 1));
    snippet_t pickme = *snippets.begin();

    EXPECT_TRUE(sel(snippets, scorer, state) == pickme);
}

TEST(Random, many) {
    marky::selector_t sel = marky::selectors::random();
    check_distribution(sel, 1, 2, 3, 1./3, 1./3, 1./3);
}

// -- BEST WEIGHTED

TEST(BestWeighted, empty) {
    INIT_STATE(snippets, scorer, state);
    marky::selector_t sel = marky::selectors::best_weighted();

    EXPECT_TRUE(sel(snippets, scorer, state) == snippet_t());
}

TEST(BestWeighted, one) {
    INIT_STATE(snippets, scorer, state);
    marky::selector_t sel = marky::selectors::best_weighted();

    snippets.insert(make_snippet("a", "b", 0, 0, 1));
    snippet_t pickme = *snippets.begin();

    EXPECT_TRUE(sel(snippets, scorer, state) == pickme);
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
