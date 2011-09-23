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

#include "selector.h"

namespace marky {
	link_t select_best(const links_t& links, const scorer_t& scorer,
			const state_t& state) {
		if (links->empty()) { return link_t(); }

		_links_t::const_iterator best_iter = links->begin();
		score_t best_score = (*best_iter)->score(scorer, state);

		for (_links_t::const_iterator iter = links->begin();
			 iter != links->end(); ++iter) {
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
		return select_best(links, scorer, state);//TODO
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