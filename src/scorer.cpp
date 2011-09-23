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

#include "scorer.h"

namespace marky {
	score_t score_no_adj(score_t score, const _state_t&, const state_t&) {
		return score;/* THAT WAS EASY... */
	}

	score_t score_link_adj(score_t score, const _state_t& score_state,
			const state_t& cur_state, size_t subtract_factor) {
		double ret = score - ((cur_state->link - score_state.link) / (double)subtract_factor);
		return (ret < 0) ? 0 : ret;/* implicit floor */
	}

	score_t score_time_adj(score_t score, const _state_t& score_state,
			const state_t& cur_state, time_t subtract_factor) {
		double ret = score - ((cur_state->time - score_state.time) / (double)subtract_factor);
		return (ret < 0) ? 0 : ret;/* implicit floor */
	}
}

marky::scorer_t marky::scorers::no_adj() {
	return std::bind(&marky::score_no_adj,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

marky::scorer_t marky::scorers::link_adj(size_t score_decrement_links) {
	if (score_decrement_links == 0) {
		return no_adj();
	}
	return std::bind(&marky::score_link_adj,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
			score_decrement_links);
}

marky::scorer_t marky::scorers::time_adj(time_t score_decrement_time) {
	if (score_decrement_time == 0) {
		return no_adj();
	}
	return std::bind(&marky::score_time_adj,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
			score_decrement_time);
}
