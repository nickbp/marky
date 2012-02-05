/*
  marky - A Markov chain generator.
  Copyright (C) 2011-2012  Nicholas Parker

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

#include <time.h>

#include "backend-map.h"
#include "rand-util.h"

marky::Backend_Map::Backend_Map()
	: state(new _state_t(time(NULL), 0)), words(), words_iter(words.end()) { }

bool marky::Backend_Map::get_random(link_t& random) {
	/* haven't figured out a more efficient way of doing this.. */

	if (words.empty()) {
		random.reset();
		return true;
	}

	if (words_iter == words.end()) {
		/* seek words_iter to a random location in words */
		unsigned int offset = pick_rand(words.size());
		words_iter = words.begin();
		for (unsigned int i = 0; i < offset; ++i) {
			++words_iter;
		}
	}

	random = words_iter->second;

	/* increment for a future get_random() call. */
	++words_iter;
	return true;
}

bool marky::Backend_Map::get_prev(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& prev) {
	word_to_links_t::const_iterator iter = prevs.find(word);
	if (iter == prevs.end()) {
		prev.reset();
	} else {
		prev = selector(iter->second, scorer, state);
	}
	return true;
}
bool marky::Backend_Map::get_next(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& next) {
	word_to_links_t::const_iterator iter = nexts.find(word);
	if (iter == nexts.end()) {
		next.reset();
	} else {
		next = selector(iter->second, scorer, state);
	}
	return true;
}

bool marky::Backend_Map::increment_link(scorer_t scorer,
		const word_t& first, const word_t& second) {
	std::pair<word_t, word_t> findme(first, second);
	words_to_link_t::const_iterator iter = words.find(findme);

	/* update time BEFORE link is added */
	time_t now = time(NULL);
	state->time = now;

	if (iter != words.end()) {
		/* link already exists, readjust/increment score */
		iter->second->increment(scorer, state);
	} else {
		/* link is new, create and add to maps */
		link_t link(new Link(first, second, now, state->link));
		words.insert(std::make_pair(findme, link));
		words_iter = words.end();/* invalidate iter after modification */

		word_to_links_t::iterator prevs_iter = prevs.find(link->prev);
		if (prevs_iter == prevs.end()) {
			links_t links(new _links_t);
			prevs_iter = prevs.insert(std::make_pair(link->prev, links)).first;
		}
		prevs_iter->second->push_back(link);

		word_to_links_t::iterator nexts_iter = nexts.find(link->next);
		if (nexts_iter == nexts.end()) {
			links_t links(new _links_t);
			nexts_iter = nexts.insert(std::make_pair(link->next, links)).first;
		}
		nexts_iter->second->push_back(link);
	}

	/* increment link count AFTER link is added (first link gets id 0) */
	++state->link;

	return true;
}

bool marky::Backend_Map::prune(scorer_t scorer) {
	bool words_changed = false;
	const words_to_link_t::iterator& words_end = words.end();
	for (words_to_link_t::iterator words_iter = words.begin();
		 words_iter != words_end; ++words_iter) {
		if (words_iter->second->score(scorer, state) > 0) {
			/* this link still has a score, doesn't need pruning */
			continue;
		}

		/* remove from words */
		words.erase(words_iter);
		words_changed = true;

		const word_t&
			prev = words_iter->second->prev,
			next = words_iter->second->next;

		/* remove from prevs (find matching next) */
		word_to_links_t::iterator prevs_iter = prevs.find(prev);
		if (prevs_iter != prevs.end()) {
			const links_t& searchme = prevs_iter->second;
			const _links_t::iterator& links_end = searchme->end();
			for (_links_t::iterator links_iter = searchme->begin();
				 links_iter != links_end; ++links_iter) {
				if ((*links_iter)->next == next) {
					searchme->erase(links_iter);
					break;
				}
			}
		}

		/* remove from nexts (find matching prev) */
		word_to_links_t::iterator nexts_iter = nexts.find(next);
		if (nexts_iter != nexts.end()) {
			const links_t& searchme = nexts_iter->second;
			const _links_t::iterator& links_end = searchme->end();
			for (_links_t::iterator links_iter = searchme->begin();
				 links_iter != links_end; ++links_iter) {
				if ((*links_iter)->prev == prev) {
					searchme->erase(links_iter);
					break;
				}
			}
		}
	}
	if (words_changed) {
		words_iter = words.end();/* invalidate iter after modification */
	}
	return true;
}
