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

#include <time.h>

#include "backend.h"

marky::Backend_Map::Backend_Map() {
	state.reset(new _state_t(time(NULL), 0));
}

bool marky::Backend_Map::get_random(link_t& random) {
	/* cheat: just return the most recently updated link
	   (easier than figuring out a random value from an unordered_map) */
	if (last_link) {
		random = last_link;
		return true;
	}
	return false;
}
bool marky::Backend_Map::get_prev(link_t& prev,
		selector_t selector, scorer_t scorer,
		const word_t& word) {
	word_to_links_t::const_iterator iter = prevs.find(word);
	if (iter == prevs.end()) {
		prev.reset();
		return true;
	}
	prev = selector(iter->second, scorer, state);
	return true;
}
bool marky::Backend_Map::get_next(link_t& next,
		selector_t selector, scorer_t scorer,
		const word_t& word) {
	word_to_links_t::const_iterator iter = nexts.find(word);
	if (iter == nexts.end()) {
		next.reset();
		return true;
	}
	next = selector(iter->second, scorer, state);
	return true;
}
bool marky::Backend_Map::increment_link(scorer_t scorer,
		const word_t& first, const word_t& second) {
	std::pair<word_t, word_t> findme(first, second);
	words_to_link_t::const_iterator iter = words.find(findme);

	time_t now = time(NULL);
	state->time = now;

	if (iter == words.end()) {
		/* link is new, create and add to maps */
		link_t link(new Link(first, second, now, state->link));
		words.insert(std::make_pair(findme, link));

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
		last_link = link;
	} else {
		/* link already exists, readjust/increment score */
		iter->second->increment(scorer, state);
		last_link = iter->second;
	}

	++state->link;

	return true;
}

bool marky::Backend_Map::prune(scorer_t scorer) {
	for (words_to_link_t::iterator iter = words.begin();
		 iter != words.end(); ++iter) {
		score_t score = iter->second->readjust(scorer, state);
		if (score == 0) {
			/* remove from maps */
			words.erase(iter);
			prevs.erase(iter->second->prev);//TODO this removes multiple links! need to scan for just this one
			nexts.erase(iter->second->next);//TODO this removes multiple links! need to scan for just this one
		}
	}
	return true;
}
