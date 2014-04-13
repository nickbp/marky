#ifndef MARKY_SELECTOR_H
#define MARKY_SELECTOR_H

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
#include "scorer.h"

namespace marky {
    /* Given a list of snippets, a scorer for those snippets, and the current
     * state of the backend, selects a snippet from the list and returns it,
     * or returns an empty pointer if no snippet could be selected, such as
     * due to an empty list. */
    typedef std::function<snippet_t (const snippet_ptr_set_t& snippets,
            const scorer_t& scorer, const State& cur_state)> selector_t;

    namespace selectors {
        /* Returns a Selector which always selects the best snippets by score,
         * with zero randomness (unless two scores are equal).
         *
         * Equivalent to best_weighted with a very high weight_factor. */
        selector_t best_always();

        /* Returns a Selector which always selects snippets randomly, regardless
         * of score, with extreme randomness.
         *
         * Equivalent to best_weighted with a very low weight_factor. */
        selector_t random();

        /* Returns a Selector which selects snippets with a custom degree of
         * randomness.
         *
         * 'weight_factor' modifies how the weighing is exaggerated.
         * factor > 128:
         *   More weight to higher-scoring snippets (less random), 255 = best_always()
         * factor < 128:
         *   More weight to lesser-scoring snippets (more random), 0 = random() */
        selector_t best_weighted(uint8_t weight_factor = 128);
    }
}

#endif
