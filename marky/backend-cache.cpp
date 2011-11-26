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

#include "backend-cache.h"

marky::Backend_Cache::Backend_Cache() {
	//TODO how to handle sharing state with backend??
}

marky::Backend_Cache::~Backend_Cache() {
	//TODO flush to backend
}

bool marky::Backend_Cache::get_random(scorer_t scorer, link_t& random) {
	/* cheat: just return the most recently updated link
	   (easier than figuring out a random value from an unordered_map) */
	if (last_link &&
			last_link->score(scorer, state) != 0) {
		/* just in case, make sure the link still has a score
		   (if it doesn't, nothing does) */
		random = last_link;
	} else {
		random.reset();
	}
	return true;
}

bool marky::Backend_Cache::get_prev(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& prev) {
	//TODO this doesnt really work, since we're supposed to return a randomized matching entry each time -- possible solution would be to retrieve ALL nexts from backend, but it'd be messy

	if (missing_prevs.find(word) != missing_prevs.end()) {
		/* we've checked before, backend just lacks this entry */
		prev.reset();
		return true;
	}

	word_to_links_t::const_iterator iter = prevs.find(word);
	if (iter != prevs.end()) {
		/* found in cache */
		prev = selector(iter->second, scorer, state);
		return true;
	}

	if (!backend->get_prev(selector, scorer, word, prev)) {
		/* backend failure */
		return false;
	}

	/* create cache entry from backend */
	prevs[word] = prev;
	return true;
}
bool marky::Backend_Cache::get_next(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& next) {
	//TODO this doesnt really work, since we're supposed to return a randomized matching entry each time -- possible solution would be to retrieve ALL nexts from backend, but it'd be messy

	if (missing_nexts.find(word) != missing_nexts.end()) {
		/* we've checked before, db just lacks this entry */
		next.reset();
		return true;
	}

	word_to_links_t::const_iterator iter = nexts.find(word);
	if (iter != nexts.end()) {
		/* found in cache */
		next = selector(iter->second, scorer, state);
		return true;
	}

	if (!backend->get_next(selector, scorer, word, next)) {
		/* backend failure */
		return false;
	}

	/* create cache entry from backend */
	nexts[word] = next;
	return true;
}

bool marky::Backend_Cache::increment_link(scorer_t scorer,
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

bool marky::Backend_Cache::prune(scorer_t scorer) {
	const words_to_link_t::iterator& words_end = words.end();
	for (words_to_link_t::iterator words_iter = words.begin();
		 words_iter != words_end; ++words_iter) {
		score_t score = words_iter->second->score(scorer, state);
		if (score != 0) {
			continue;
		}

		/* remove from words */
		words.erase(words_iter);

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
	return true;
}
