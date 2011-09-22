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

#include "processor.h"

bool marky::Processor::insert(const lines_t& lines) {
	info_t cur_info;
	if (!backend->get_info(cur_info)) {
		return false;
	}
	for (lines_t::const_iterator line_iter = lines.begin();
		 line_iter != lines.end(); ++line_iter) {
		const line_t& line = *line_iter;
		if (line.empty()) { continue; }

		line_t::const_iterator
			last_word = line.begin(),
			cur_word = ++line.begin();

		while (cur_word != line.end()) {
			++cur_info.link;
			link_t link;
			if (backend->get_by_link(link, *last_word, *cur_word)) {
				/* update score/info in link */
				link.score = scorer(link, cur_info) + 1;

				link.info.time = cur_info.time;
				link.info.link = cur_info.link;
			} else {
				/* init fresh link */
				link.first = *last_word;
				link.second = *cur_word;

				link.score = 1;

				link.info.time = cur_info.time;
				link.info.link = cur_info.link;
			}
			if (!backend->update_link(link)) {
				return false;
			}
			++last_word;
			++cur_word;
		}
	}
	return backend->update_info(cur_info);
}

bool marky::Processor::produce(line_t& line,
		const word_t& search/*=word_t()*/,
		size_t length_limit/*=0*/) {
	if (search.empty()) {
		link_t rand;
		if (!backend->get_random(rand)) {
			return false;
		}
		return grow(line, rand.first, rand.second, length_limit);
	} else {
		return grow(line, search, search, length_limit);
	}
}

bool marky::Processor::grow(line_t& line,
		const word_t& left, const word_t& right,
		size_t length_limit/*=0*/) {
	//TODO grow outward from this link until hit limit or empty prev/next
	if (length_limit == 0) {
		for (;;) {
			//return false when hit empty prev/next:
			//if (!grow_right && !grow_left) { break; }
		}
	} else {
		while (line.size() < length_limit) {
			//grow_right
			if (line.size() >= length_limit) { break; }
			//grow_left
		}
	}
}
