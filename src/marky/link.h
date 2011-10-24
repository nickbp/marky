#ifndef MARKY_LINK_H
#define MARKY_LINK_H

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

#include <stddef.h>//size_t
#include <time.h>//time_t

#include <string>
#include <memory>
#include <list>

namespace marky {
	typedef std::string word_t;
	typedef size_t score_t;

	/* A container for the current state of the backend.
	   Used by scorers to adjust a link's score */
	struct _state_t {
		_state_t(time_t time, size_t link)
			: time(time), link(link) { }
		time_t time;
		size_t link;
	};
	typedef std::shared_ptr<_state_t> state_t;

	class Link;
	/* Calculate the adjusted score for a link.
	   'score_state' is the backend state from the last time the link was encountered,
	   'cur_state' is the backend state from right now. */
	typedef std::function<score_t
		(score_t score, const _state_t& last_score_state, const _state_t& now_state)> scorer_t;

	class Link {
	public:
		Link(const word_t& prev, const word_t& next,
				time_t time, size_t link, score_t score = 1)
			: prev(prev), next(next), state_(time, link), score_(score) { }

		inline score_t score(scorer_t scorer, const state_t& cur_state) {
			return scorer(score_, state_, *cur_state);
		}
		inline void increment(scorer_t scorer, const state_t& cur_state) {
			score_ = 1 + score(scorer, cur_state);
			/* reset the state 'clock' to now */
			state_ = *cur_state;
		}

		const word_t prev;
		const word_t next;

	private:
		_state_t state_;
		score_t score_;
	};
	typedef std::shared_ptr<Link> link_t;

	typedef std::list<link_t> _links_t;
	typedef std::shared_ptr<_links_t> links_t;
}

#endif
