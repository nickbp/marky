#ifndef MARKY_LINK_H
#define MARKY_LINK_H

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

#include <stddef.h>//size_t
#include <time.h>//time_t

#include <string>
#include <list>

namespace marky {
	typedef std::string word_t;
	typedef size_t score_t;

	typedef struct _info_t {
		time_t time;
		size_t link;
	} info_t;

	typedef struct _link_t {
		word_t first;
		word_t second;

		info_t info;
		score_t score;
	} link_t;
	typedef std::list<link_t> links_t;
}

#endif
