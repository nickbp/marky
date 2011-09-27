#ifndef MARKY_MARKY_H
#define MARKY_MARKY_H

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
#include "selector.h"

namespace marky {
	typedef std::list<word_t> line_t;

	/* Marky is a simple but fairly modular library for creating markov chains
	 * from arbitrary text. There are likely better libraries available; this is
	 * just a toy project. */
	class Marky {
	public:
		Marky(backend_t backend, selector_t selector, scorer_t scorer)
			: backend(backend), selector(selector), scorer(scorer) { }

		/* Adds the line (and its inter-word links) to the dataset. */
		bool insert(const line_t& line);

		/* Produces a line from the search word, or from a random word if the
		 * search word is unspecified. Returns false if the search word wasn't
		 * found or in the event of an error. */
		bool produce(line_t& line, const word_t& search = word_t(),
				size_t length_limit_chars = 1000);

	private:
		/* Grows a line in both directions until length has been reached. */
		bool grow(line_t& line, size_t length_limit_chars);

		const backend_t backend;
		const selector_t selector;
		const scorer_t scorer;
	};
}

#endif
