#ifndef MARKY_BACKEND_SQLITE_H
#define MARKY_BACKEND_SQLITE_H

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

#include "backend.h"

struct sqlite3;

namespace marky {
	/* A backend which uses a sqlite3 database for storing persistent state. */
	class Backend_SQLite : public IBackend {
	public:
		/* Returns a SQLite backend, or an empty ptr if there was an error
		 * when creating it. */
		static backend_t create(const std::string& db_file_path);

		virtual ~Backend_SQLite();

		bool get_random(scorer_t scorer, link_t& random);

		bool get_prev(selector_t selector, scorer_t scorer,
				const word_t& word, link_t& prev);
		bool get_next(selector_t selector, scorer_t scorer,
				const word_t& word, link_t& next);

		bool increment_link(scorer_t scorer,
				const word_t& first, const word_t& second);

		bool prune(scorer_t scorer);

	private:
		Backend_SQLite(const std::string& db_file_path);
		bool init();

		const std::string path;
		sqlite3* db;
		state_t state;
	};
}

#endif