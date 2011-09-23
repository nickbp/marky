#ifndef MARKY_PROCESSOR_H
#define MARKY_PROCESSOR_H

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
	typedef std::list<line_t> lines_t;

	class Processor {
	public:
		Processor(backend_t backend, selector_t selector, scorer_t scorer)
			: backend(backend), selector(selector), scorer(scorer) { }

		bool insert(const line_t& line);

		bool produce(line_t& line, const word_t& search = word_t(),
				size_t length_limit_chars = 1000);

	private:
		bool grow(line_t& line, const word_t& left, const word_t& right,
				size_t length_limit_chars);

		const backend_t backend;
		const selector_t selector;
		const scorer_t scorer;
	};
}

#endif
