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
#include "scorer.h"
#include "selector.h"

namespace marky {
	typedef std::list<word_t> line_t;
	typedef std::list<line_t> lines_t;

	class Processor {
	public:
		Processor(backend_t backend, scorer_t scorer, selector_t selector) : backend(backend) { }

		bool insert(const lines_t& lines);
		bool produce(line_t& line, const word_t& search = word_t(),
				size_t length_limit = 0);

	private:
		bool grow(line_t& line, const word_t& left, const word_t& right,
				size_t length_limit = 0);

		const backend_t backend;
		const scorer_t scorer;
		const selector_t selector;
	};
}

#endif
