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

#include "rand-util.h"

#include <sys/time.h>//gettimeofday()
#include <stdlib.h>//rand()

namespace {
    inline unsigned int get_seed() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (unsigned int)tv.tv_usec;
    }
}

size_t marky::pick_rand(size_t max) {
    static unsigned int seed = get_seed();
    return rand_r(&seed) % max;
}
