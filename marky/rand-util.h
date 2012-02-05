#ifndef MARKY_RAND_UTIL_H
#define MARKY_RAND_UTIL_H

/*
  marky - A Markov chain generator.
  Copyright (C) 2012  Nicholas Parker

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

namespace marky {
	/* Return a value within [0,max).
	 * This has internal state and assumes it won't be simultaneously called
	 * across threads, but it shouldn't conflict with other running code. */
	size_t pick_rand(size_t max);
}

#endif
