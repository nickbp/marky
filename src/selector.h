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
	/* Select a link from a list according to some algorithm or criteria. */
	typedef std::function<link_t
		(const links_t& links, const scorer_t& scorer, const state_t& cur_state)> selector_t;

	namespace selectors {
		/* Always returns the best link by score.
		 * Equivalent to best_weighted with a very high weight_factor. */
		selector_t best_always();

		/* Randomly selects a link, weighted by score.
		 * 'weight_factor' modifies how the weighing is exaggerated.
		 * factor > 0: More weight to higher-scoring links
		 * factor < 0: More weight to lesser-scoring links */
		selector_t best_weighted(double weight_factor = 0.0);
	}
}

#endif
