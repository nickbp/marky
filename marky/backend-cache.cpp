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

#include "backend-cache.h"

marky::Backend_Cache::Backend_Cache(cacheable_t backend)
	: wrapme(backend), state(backend->state()) { }

marky::Backend_Cache::~Backend_Cache() {
	if (!changed_words.empty()) {
		/* flush our changes to wrapme */
		links_t flushme(new _links_t);
		const words_to_link_t::const_iterator end = changed_words.end();
		for (words_to_link_t::iterator iter = changed_words.begin();
			 iter != end; ++iter) {
			flushme->push_back(iter->second);
		}
		wrapme->flush(flushme, state);
	}
}

bool marky::Backend_Cache::get_random(link_t& random) {
	/* just pass to wrapme, to select across full dataset */
	return wrapme->get_random(random);
}

bool marky::Backend_Cache::get_prev(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& prev) {
	/*
	  TODO to avoid memory explosion, consider wiping get_x data whenever
	  size reaches some N (TODO tuning)
	*/

	word_to_links_t::iterator got_iter = got_prevs.find(word);
	if (got_iter == got_prevs.end()) {
		/* create get cache of all matching prevs */
		links_t got(new _links_t);
		if (!wrapme->get_prevs(word, got)) {
			/* backend failure */
			return false;
		}

		if (got->empty()) {
			got.reset();/* flag as nothing found (see pick_word) */
		} else {
			/* add to got_words pool (see increment_link) */
			const _links_t::const_iterator end = got->end();
			for (_links_t::const_iterator iter = got->begin();
				 iter != end; ++iter) {
				got_words[std::make_pair((*iter)->prev, (*iter)->next)] = *iter;
			}
		}
		/* also add to got_prevs */
		got_iter = got_prevs.insert(std::make_pair(word, got)).first;
	}

	return pick_word(got_iter, changed_prevs, selector, scorer, word, prev);
}
bool marky::Backend_Cache::get_next(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& next) {
	/*
	  TODO to avoid memory explosion, consider wiping got_x data whenever
	  size reaches some N (TODO tuning)
	*/

	word_to_links_t::iterator got_iter = got_nexts.find(word);
	if (got_iter == got_nexts.end()) {
		/* create get cache of all matching nexts */
		links_t got(new _links_t);
		if (!wrapme->get_nexts(word, got)) {
			/* backend failure */
			return false;
		}

		if (got->empty()) {
			got.reset();/* flag as nothing found (see pick_word) */
		} else {
			/* add to got_words pool (see increment_link) */
			const _links_t::const_iterator end = got->end();
			for (_links_t::const_iterator iter = got->begin();
				 iter != end; ++iter) {
				got_words[std::make_pair((*iter)->prev, (*iter)->next)] = *iter;
			}
		}
		/* also add to got_nexts */
		got_iter = got_nexts.insert(std::make_pair(word, got)).first;
	}

	return pick_word(got_iter, changed_nexts, selector, scorer, word, next);
}

bool marky::Backend_Cache::pick_word(const word_to_links_t::iterator& got_iter,
		word_to_links_t& changed_map, selector_t selector, scorer_t scorer,
		const word_t& word, link_t& out) {
	/*
	  NOTE:
	  the strategy here is to merge between changed/get at the time of get_x().
	  we could also do this merging at increment_link(), but I'd expect the
	  following effects if this switches to that strategy:

	  - if only lots of increments are called, increment_link will be slower
	  since we'll need to get all data matching the prev/next to do the merge.
	  this would also increase memory usage/cache size quite a bit.
	  - if only lots of gets are called, gets wont really be faster since the
	  current checks against changed_map are pretty quick, especially since
	  changed_map would be empty until changes are made
	  - if even numbers of both are called, it'd be a wash. but i doubt this
	  would come up in practice (expect most people would do bulk inserts
	  followed by periodic gets)
	  - if we're careful to avoid duplicates across the two, it'll be wash,
	  mostly at the expense of increment_link
	*/

	word_to_links_t::iterator changed_iter = changed_map.find(word);

	links_t selectme;
	if ((bool)got_iter->second && changed_iter != changed_map.end()) {
		/* entries found in get cache AND changed values, merge and add both.
		   this shouldn't happen too often, most users will probably be either
		   reading OR writing, not both simultaneously. */
		words_to_link_t merged;

		{
			const _links_t::const_iterator end = got_iter->second->end();
			for (_links_t::const_iterator link_iter = got_iter->second->begin();
				 link_iter != end; ++link_iter) {
				merged[std::make_pair((*link_iter)->prev, (*link_iter)->next)] = *link_iter;
			}
		}
		{
			/* do this SECOND to override any matching entries in got_iter */
			const _links_t::const_iterator end = changed_iter->second->end();
			for (_links_t::const_iterator link_iter = changed_iter->second->begin();
				 link_iter != end; ++link_iter) {
				merged[std::make_pair((*link_iter)->prev, (*link_iter)->next)] = *link_iter;
			}
		}

		selectme.reset(new _links_t);
		const words_to_link_t::const_iterator end = merged.end();
		for (words_to_link_t::const_iterator merge_iter = merged.begin();
			 merge_iter != end; ++merge_iter) {
			selectme->push_back(merge_iter->second);
		}
	} else if ((bool)got_iter->second) {
		/* entries found in got cache, and NOT changed values */
		selectme = got_iter->second;
	} else if (changed_iter != changed_map.end()) {
		/* entries found in changed values, and NOT get cache */
		selectme = changed_iter->second;
	} else {
		/* nothing found, produce empty ptr */
		out.reset();
		return true;
	}

	/* select from combined list */
	out = selector(selectme, scorer, state);
	return true;
}

bool marky::Backend_Cache::increment_link(scorer_t scorer,
		const word_t& first, const word_t& second) {
	/*
	  TODO to avoid memory explosion, consider flushing changed data (and
	  merging to got_x for non-empty entries) whenever changeset size reaches
	  some N (TODO tuning)
	*/

	std::pair<word_t, word_t> key(first,second);
	words_to_link_t::const_iterator changed_iter = changed_words.find(key);

	/* update time BEFORE link is added */
	time_t now = time(NULL);
	state->time = now;

	if (changed_iter != changed_words.end()) {
		/* link already in changed_words, just readjust/increment score */
		changed_iter->second->increment(scorer, state);
	} else {
		words_to_link_t::const_iterator got_iter = got_words.find(key);
		link_t link;
		if (got_iter != got_words.end()) {
			/* link found in got_words, copy its content then adjust
			   (could move it, but that'd involve searching for it in
			   got_nexts/got_prevs as well) */
			link.reset(new Link(*got_iter->second));
			link->increment(scorer, state);
		} else {
			/* link not found in cache, see if backend has it */
			if (!wrapme->get_link(first, second, link)) {
				/* backend failure */
				return false;
			}
			if ((bool)link) {
				/* backend had it, increment its score */
				link->increment(scorer, state);
			} else {
				/* backend doesn't have it either. create a new entry. */
				link.reset(new Link(first, second, now, state->link));
			}
		}
		changed_words[key] = link;

		/* add the link to changed_prevs/changed_nexts */

		word_to_links_t::iterator prevs_iter = changed_prevs.find(link->prev);
		if (prevs_iter == changed_prevs.end()) {
			links_t links(new _links_t);
			prevs_iter = changed_prevs.insert(std::make_pair(link->prev, links)).first;
		}
		prevs_iter->second->push_back(link);

		word_to_links_t::iterator nexts_iter = changed_nexts.find(link->next);
		if (nexts_iter == changed_nexts.end()) {
			links_t links(new _links_t);
			nexts_iter = changed_nexts.insert(std::make_pair(link->next, links)).first;
		}
		nexts_iter->second->push_back(link);
	}

	/* increment link count AFTER link is added (first link gets id 0) */
	++state->link;

	return true;
}

bool marky::Backend_Cache::prune(scorer_t scorer) {
	if (changed_words.empty()) {
		return true;
	}

	/* flush changes to wrapme */
	links_t flushme(new _links_t);

	const words_to_link_t::const_iterator end = changed_words.end();
	for (words_to_link_t::const_iterator iter = changed_words.end();
		 iter != end; ++iter) {
		flushme->push_back(iter->second);
	}

	if (!wrapme->flush(flushme, state)) {
		return false;
	}

	/* now just wipe the cache and start from scratch.
	   TODO could someday have a merge from changed_* into got_*, but it's not
	   clear if that'd benefit things too much. also this helps in terms of
	   keeping the cache size under control */

	changed_words.clear();
	changed_prevs.clear();
	changed_nexts.clear();

	return wrapme->prune(scorer);
}
