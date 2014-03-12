#ifndef MARKY_SCORER_H
#define MARKY_SCORER_H

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

#include <functional>

#include "snippet.h"

namespace marky {
    namespace scorers {
        /* No adjustment; scores just increment sequentially as words are encountered. */
        scorer_t no_adj();

        /* Adjusts scores to slowly decrease as other words are encountered.
         * score_decrement_words is the number of words which equate to a point decrease.
         * For example, a value of 100 means that a given snippet loses one point after
         * 100 other words have appeared.
         * If decrement is 0, the resulting scorer will be equivalent to no_adj(). */
        scorer_t word_adj(size_t score_decrement_words);

        /* Adjusts scores to slowly decrease as time passes.
         * score_decrement_seconds is the number of seconds which equate to a point decrease.
         * For example, a value of 100 means that a given snippet loses one point after
         * 100 seconds have transpired.
         * If decrement is 0, the resulting scorer will be equivalent to no_adj(). */
        scorer_t time_adj(size_t score_decrement_seconds);
    }
}

#endif
