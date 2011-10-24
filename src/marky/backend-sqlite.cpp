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

#include "backend-sqlite.h"

marky::Backend_SQLite::Backend_SQLite(const std::string& db_file_path) {
	//TODO
}

bool marky::Backend_SQLite::get_random(scorer_t scorer, link_t& random) {
	return false;//TODO
}

bool marky::Backend_SQLite::get_prev(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& prev) {
	return false;//TODO
}
bool marky::Backend_SQLite::get_next(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& next) {
	return false;//TODO
}

bool marky::Backend_SQLite::increment_link(scorer_t scorer,
		const word_t& first, const word_t& second) {
	return false;//TODO
}

bool marky::Backend_SQLite::prune(scorer_t scorer) {
	return false;//TODO
}
