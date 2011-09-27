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

bool marky::Marky::produce(line_t& line,
		const word_t& search/*=word_t()*/,
		size_t length_limit_chars/*=1000*/) {
	if (search.empty()) {
		link_t rand;
		if (!backend->get_random(rand)) {
			return false;
		}
		line.push_back(rand->prev);
		line.push_back(rand->next);
		return grow(line, length_limit_chars);
	} else {
		line.push_back(search);
		return grow(line, length_limit_chars);
	}
}

bool marky::Marky::grow(line_t& line, size_t length_limit_chars) {
	if (line.empty()) { return false; }
	bool continue_left = true, continue_right = true;
	size_t char_size = 0;
	link_t link;
	while (char_size < length_limit_chars &&
			(continue_left || continue_right)) {
		if (continue_right) {
			if (!backend->get_next(link, selector, scorer, line.back())) {
				return false;
			}
			if (link) {
				const word_t& word = link->next;
				char_size += word.size();
				line.push_back(word);
			} else {
				continue_right = false;
			}
		}

		if (continue_left) {
			if (!backend->get_prev(link, selector, scorer, line.front())) {
				return false;
			}
			if (link) {
				const word_t& word = link->prev;
				char_size += word.size();
				line.push_front(word);
			} else {
				continue_left = false;
			}
		}
	}
	return true;
}
