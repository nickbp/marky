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
	const link_t& select_best(const links_t& links) {
		links_t::const_iterator best_iter = links.begin();
		for (links_t::const_iterator iter = links.begin();
			 iter != links.end(); ++iter) {
			if (iter->score > best_iter->score) {
				best_iter = iter;
			}
		}
		return *best_iter;
	}

	const link_t& select_weighted(const links_t& links, double /*weight_factor*/) {
		return select_best(links);//TODO
	}
}

marky::selector_t marky::selectors::best_always() {
	return std::bind(&marky::select_best, std::placeholders::_1);
}

marky::selector_t marky::selectors::best_weighted(double weight_factor/*=0.0*/) {
	return std::bind(&marky::select_weighted, std::placeholders::_1, weight_factor);
}
