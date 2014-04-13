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
#include <marky/markyc.h>
#include <marky/marky.h>
#include <marky/backend-map.h>
#include <marky/config.h>

TEST(Marky, disallow_no_limits) {
    marky::backend_t backend(new marky::Backend_Map());
    marky::scorer_t scorer = marky::scorers::no_adj();
    marky::selector_t selector = marky::selectors::best_always();
    marky::Marky marky(backend, selector, scorer, 1);

    marky::words_t line_out;
    EXPECT_FALSE(marky.produce(line_out, marky::words_t(), 0, 0));
}

TEST(Marky, basic_insert_get) {
    marky::backend_t backend(new marky::Backend_Map());
    marky::scorer_t scorer = marky::scorers::no_adj();
    marky::selector_t selector = marky::selectors::best_always();
    marky::Marky marky(backend, selector, scorer, 1);

    {
        marky::words_t line_in;
        line_in.push_back("a");
        line_in.push_back("b");
        line_in.push_back("c");
        EXPECT_TRUE(marky.insert(line_in));
    }

    marky::words_t line_out;

    {
        marky::words_t search;
        search.push_back("a");
        EXPECT_TRUE(marky.produce(line_out, search));
        EXPECT_FALSE(line_out.empty());

        line_out.clear();
        search.clear();
        search.push_back("d");
        EXPECT_TRUE(marky.produce(line_out, search));
        EXPECT_TRUE(line_out.empty());
    }

    EXPECT_TRUE(marky.produce(line_out));
    EXPECT_FALSE(line_out.empty());

    EXPECT_TRUE(marky.prune_backend());
}

static char* string_on_heap(const char* stack_string) {
    const size_t string_size = strlen(stack_string);
    char* out = (char*)malloc(string_size + 1);
    EXPECT_TRUE(out != NULL);
    strncpy(out, stack_string, string_size + 1);
    return out;
}

TEST(MarkyC, basic_insert_get) {
    marky_Backend* backend = marky_backend_new_map();
    marky_Scorer* scorer = marky_scorer_new_no_adj();
    marky_Selector* selector = marky_selector_new_best_always();

    marky_Marky* marky = marky_new(backend, selector, scorer, 1);

    /* marky_new says we can do this... */
    marky_backend_free(backend);
    marky_scorer_free(scorer);
    marky_selector_free(selector);
    backend = NULL;
    scorer = NULL;
    selector = NULL;
    /* and this... */
    marky_backend_free(backend);
    marky_scorer_free(scorer);
    marky_selector_free(selector);

    {
        marky_words_t* line_in = marky_words_new(3);
        ASSERT_TRUE(line_in != NULL);
        ASSERT_TRUE(line_in->words != NULL);
        ASSERT_EQ(3, line_in->words_count);

        line_in->words[0] = string_on_heap("a");
        line_in->words[1] = string_on_heap("b");
        line_in->words[2] = string_on_heap("c");
        marky_insert(marky, line_in);
        EXPECT_EQ(MARKY_SUCCESS, marky_insert(marky, line_in));

        marky_words_free(line_in);
        line_in = NULL;
    }

    marky_words_t* line_out = NULL;

    {
        marky_words_t* search = marky_words_new(1);
        ASSERT_TRUE(search != NULL);
        ASSERT_TRUE(search->words != NULL);
        ASSERT_EQ(1, search->words_count);
        search->words[0] = string_on_heap("a");

        EXPECT_EQ(MARKY_SUCCESS, marky_produce(marky, &line_out, search));
        EXPECT_TRUE(line_out->words != NULL);
        EXPECT_TRUE(line_out->words[0] != NULL);
        EXPECT_NE(0, line_out->words_count);

        marky_words_free(line_out);
        line_out = NULL;

        free(search->words[0]);
        search->words[0] = string_on_heap("d");

        EXPECT_EQ(MARKY_SUCCESS, marky_produce(marky, &line_out, search));
        EXPECT_TRUE(line_out == NULL);

        marky_words_free(search);
        search = NULL;

        marky_words_free(line_out);
        line_out = NULL;
    }

    EXPECT_EQ(MARKY_SUCCESS, marky_produce(marky, &line_out));
    EXPECT_TRUE(line_out->words != NULL);
    EXPECT_TRUE(line_out->words[0] != NULL);
    EXPECT_NE(0, line_out->words_count);

    marky_words_free(line_out);
    line_out = NULL;

    EXPECT_EQ(MARKY_SUCCESS, marky_prune_backend(marky));

    marky_free(marky);
    marky = NULL;
    marky_free(marky);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
