#ifndef MARKY_SELECTOR_H
#define MARKY_SELECTOR_H

/*
  marky - A Markov chain generator.
  Copyright (C) 2011  Nicholas Parker

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

#include "link.h"
#include "scorer.h"

namespace marky {
	/* Given a list of links, a scorer for those links, and the current
	 * state of the backend, selects a link from the list and returns it,
	 * or returns an empty pointer if no link could be selected, such as
	 * due to an empty list. */
	typedef std::function<link_t
		(const links_t& links, const scorer_t& scorer, const state_t& cur_state)> selector_t;

	namespace selectors {
		/* Always returns the best link by score.
		 * Equivalent to best_weighted with a very high weight_factor. */
		selector_t best_always();

		/* Returns a random link, regardless of score.
		 * Equivalent to best_weighted with a very low weight_factor. */
		selector_t random();

		/* Randomly selects a link, weighted by score.
		 * 'weight_factor' modifies how the weighing is exaggerated.
		 * factor > 128:
		 *   More weight to higher-scoring links (less random), 255 = best_always()
		 * factor < 128:
		 *   More weight to lesser-scoring links (more random), 0 = random() */
		selector_t best_weighted(uint8_t weight_factor = 128);
	}
}

#endif
