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
#include <marky/scorer.h>

using namespace marky;

#define STATE(state, num) \
    state.time = num; \
    state.count = num;

static const words_t words({"a", "b"});

TEST(Snippet, score_get_noadj) {
    scorer_t scorer = scorers::no_adj();
    Snippet snippet(words, 0, 0);/* initial score = 1 */

    State state(0,0);
    EXPECT_EQ(1, snippet.score(scorer, state));

    STATE(state, 500);
    EXPECT_EQ(1, snippet.score(scorer, state));
}

TEST(Snippet, score_get_linkadj) {
    scorer_t scorer = scorers::word_adj(5);
    Snippet snippet(words, 0, 0);/* initial score = 1 */

    State state(0,0);
    EXPECT_EQ(1, snippet.score(scorer, state));

    STATE(state, 4);
    EXPECT_EQ(1, snippet.score(scorer, state));

    STATE(state, 5);
    EXPECT_EQ(0, snippet.score(scorer, state));

    STATE(state, 500);
    EXPECT_EQ(0, snippet.score(scorer, state));
}

TEST(Snippet, score_inc_noadj) {
    scorer_t scorer = scorers::no_adj();
    Snippet snippet(words, 0, 0);/* initial score = 1 */

    State state(0,0);
    EXPECT_EQ(1, snippet.score(scorer, state));

    snippet.increment(scorer, state);

    STATE(state, 500);
    EXPECT_EQ(2, snippet.score(scorer, state));
}

TEST(Snippet, score_inc_linkadj) {
    scorer_t scorer = scorers::word_adj(5);/* lose a point after 5s of no activity */
    Snippet snippet(words, 0, 0);/* initial score = 1 */

    State state(0,0);
    EXPECT_EQ(1, snippet.score(scorer, state));

    snippet.increment(scorer, state);
    EXPECT_EQ(2, snippet.score(scorer, state));

    STATE(state, 4);
    EXPECT_EQ(2, snippet.score(scorer, state));

    STATE(state, 5);
    EXPECT_EQ(1, snippet.score(scorer, state));/* -1 */

    snippet.increment(scorer, state);/* +1, set state clock to 5 */
    EXPECT_EQ(2, snippet.score(scorer, state));

    STATE(state, 9);
    EXPECT_EQ(2, snippet.score(scorer, state));

    STATE(state, 10);
    EXPECT_EQ(1, snippet.score(scorer, state));/* -1 */

    STATE(state, 14);
    EXPECT_EQ(1, snippet.score(scorer, state));

    STATE(state, 15);
    EXPECT_EQ(0, snippet.score(scorer, state));/* -1 */



    STATE(state, 17);
    snippet.increment(scorer, state);/* +1, set state clock to 17 */
    EXPECT_EQ(1, snippet.score(scorer, state));

    STATE(state, 21);
    EXPECT_EQ(1, snippet.score(scorer, state));

    STATE(state, 22);
    EXPECT_EQ(0, snippet.score(scorer, state));



    STATE(state, 26);
    snippet.increment(scorer, state);/* +1, set state clock to 26 */
    STATE(state, 27);
    snippet.increment(scorer, state);/* +1, set state clock to 27 */
    STATE(state, 28);
    snippet.increment(scorer, state);/* +1, set state clock to 28 */

    STATE(state, 32);
    EXPECT_EQ(3, snippet.score(scorer, state));
    STATE(state, 33);
    EXPECT_EQ(2, snippet.score(scorer, state));/* don't start decrementing until 28+5 */
    STATE(state, 37);
    EXPECT_EQ(2, snippet.score(scorer, state));
    STATE(state, 38);
    EXPECT_EQ(1, snippet.score(scorer, state));
    STATE(state, 42);
    EXPECT_EQ(1, snippet.score(scorer, state));
    STATE(state, 43);
    EXPECT_EQ(0, snippet.score(scorer, state));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
