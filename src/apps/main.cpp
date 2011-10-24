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

#include "marky.h"

#include <stdlib.h> /* rand() */

namespace marky {
	link_t select_best(const links_t& links, const scorer_t& scorer,
			const state_t& state) {
		if (links->empty()) { return link_t(); }

		_links_t::const_iterator best_iter = links->begin();
		score_t best_score = (*best_iter)->score(scorer, state);

		const _links_t::const_iterator& end = links->end();
		for (_links_t::const_iterator iter = links->begin();
			 iter != end; ++iter) {
			score_t score = (*iter)->score(scorer, state);
			if (score > best_score) {
				best_iter = iter;
				best_score = score;
			}
		}

		return *best_iter;
	}

	link_t select_weighted(const links_t& links, const scorer_t& scorer,
			const state_t& state, double /*weight_factor*/) {
		/* first pass: get sum score from which to derive 'select' */
		score_t sum_score = 0;
		const _links_t::const_iterator& end = links->end();
		for (_links_t::const_iterator iter = links->begin();
			 iter != end; ++iter) {
			sum_score += (*iter)->score(scorer, state);
		}

		score_t select = rand() * sum_score / (double)RAND_MAX;

		/* second pass: subtract scores from select, return when select hits 0 */
		const _links_t::const_iterator& end = links->end();
		for (_links_t::const_iterator iter = links->begin();
			 iter != end; ++iter) {
			score_t s = (*iter)->score(scorer, state);
			if (select < s) {
				return *iter;
			}
			select -= s;
		}
		return link_t();
	}
}

marky::selector_t marky::selectors::best_always() {
	return std::bind(&marky::select_best,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3);
}

marky::selector_t marky::selectors::best_weighted(double weight_factor/*=0.0*/) {
	return std::bind(&marky::select_weighted,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, weight_factor);
}
