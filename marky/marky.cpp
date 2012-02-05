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

#include "marky.h"
#include <assert.h>

marky::Marky::Marky(backend_t backend, selector_t selector, scorer_t scorer)
	: backend(backend), selector(selector), scorer(scorer) {
	assert(backend);
	assert(selector);
	assert(scorer);
}

bool marky::Marky::insert(const line_t& line) {
	if (line.empty()) { return true; }

	line_t::const_iterator
		last_word = line.begin(),
		cur_word = ++line.begin();

	while (cur_word != line.end()) {
		if (!backend->increment_link(scorer, *last_word, *cur_word)) {
			return false;
		}
		++last_word;
		++cur_word;
	}
	return true;
}

bool marky::Marky::produce(line_t& line, const word_t& search/*=word_t()*/,
		size_t length_limit_words/*=0*/, size_t length_limit_chars/*=0*/) {
	if (search.empty()) {
		link_t rand;
		if (!backend->get_random(rand)) {/* backend err */
			return false;
		}
		if (!rand) {/* no data */
			return true;
		}
		line.push_back(rand->prev);
		line.push_back(rand->next);
		return grow(line, length_limit_words, length_limit_chars);
	} else {
		line.push_back(search);
		if (!grow(line, length_limit_words, length_limit_chars)) {/* backend err */
			line.clear();
			return false;
		} else if (line.size() == 1) {/* didn't find 'search' */
			line.clear();
		}
		return true;
	}
}

#define CHECK_LIMIT(size, limit) (limit == 0 || size < limit)
#define CHECK_CHAR_LIMIT(char_size, limit) (limit == 0 || char_size < limit)

bool marky::Marky::grow(line_t& line,
		size_t length_limit_words/*=0*/, size_t length_limit_chars/*=0*/) {
	if (line.empty()) { return false; }
	/* flags marking whether we've hit a dead end in either direction: */
	bool left_dead = false, right_dead = false;
	size_t char_size = 0;
	for (line_t::const_iterator iter = line.begin();
		 iter != line.end(); ++iter) {
		char_size += iter->size();/* ignore space between words */
	}
	link_t link;
	while (!left_dead || !right_dead) {
		if (!CHECK_LIMIT(line.size(), length_limit_words) ||
				!CHECK_LIMIT(char_size, length_limit_chars)) {
			break;
		}

		if (!right_dead) {
			if (!backend->get_prev(selector, scorer, line.back(), link)) {
				return false;
			}
			if (link) {
				const word_t& word = link->next;
				char_size += word.size();/* ignore space between words */
				line.push_back(word);
			} else {
				right_dead = true;
			}
		}

		if (!CHECK_LIMIT(line.size(), length_limit_words) ||
				!CHECK_LIMIT(char_size, length_limit_chars)) {
			break;
		}

		if (!left_dead) {
			if (!backend->get_next(selector, scorer, line.front(), link)) {
				return false;
			}
			if (link) {
				const word_t& word = link->prev;
				char_size += word.size();/* ignore space between words */
				line.push_front(word);
			} else {
				left_dead = true;
			}
		}
	}
	return true;
}
