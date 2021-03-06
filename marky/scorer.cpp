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

#include "scorer.h"
#include "config.h"

#include <math.h> //ceil()

using namespace std::placeholders;

namespace marky {
    score_t score_no_adj(score_t score) {
        return score;/* THAT WAS EASY... */
    }

    score_t score_word_adj(score_t score, const State& last_score_state,
            const State& now_state, size_t subtract_factor) {
        /* use ceil: avoid reducing score before factor has actually been reached
         * (5-word example: only decrement AFTER 5-word is reached) */
        double ret = ceil(score -
                ((now_state.count - last_score_state.count) / (double)subtract_factor));
        DEBUG("ceil(%lu - ((%lu - %lu) / %.02f)) = %.02f", score, now_state.count, last_score_state.count, (double)subtract_factor, ret);
        return (ret < 0) ? 0 : ret;
    }

    score_t score_time_adj(score_t score, const State& last_score_state,
            const State& now_state, time_t subtract_factor) {
        /* use ceil: avoid reducing score before factor has actually been reached
         * (5s example: only decrement AFTER 5s is reached) */
        double ret = ceil(score -
                ((now_state.time - last_score_state.time) / (double)subtract_factor));
        return (ret < 0) ? 0 : ret;
    }
}

marky::scorer_t marky::scorers::no_adj() {
    return std::bind(&marky::score_no_adj, _1);
}

marky::scorer_t marky::scorers::word_adj(size_t score_decrement_words) {
    if (score_decrement_words == 0) {
        return no_adj();
    }
    return std::bind(&marky::score_word_adj, _1, _2, _3, score_decrement_words);
}

marky::scorer_t marky::scorers::time_adj(size_t score_decrement_seconds) {
    if (score_decrement_seconds == 0) {
        return no_adj();
    }
    return std::bind(&marky::score_time_adj, _1, _2, _3, score_decrement_seconds);
}
