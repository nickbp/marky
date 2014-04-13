#ifndef MARKY_MARKY_H
#define MARKY_MARKY_H

/*
  marky - A Markov chain generator.
  Copyright (C) 2011-2014  Nicholas Parker

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
    /* Marky is a simple but fairly modular library for creating markov chains
     * from arbitrary text. There are likely better libraries available; this is
     * just a toy project. */
    class Marky {
    public:
        /* Sets up a Marky instance using the provided components and a look
         * size. The choice of components will determine how Marky scores and
         * stores any input. */
        Marky(backend_t backend, selector_t selector, scorer_t scorer,
                size_t look_size);
        virtual ~Marky();

        /* Adds the line (and its inter-word snippets) to the dataset.
         * Returns false in the event of some error. */
        bool insert(const words_t& line);

        /* Produces a line from the search word(s), or from a random word if the
         * search words are unspecified.
         *
         * 'length_limit_words' and 'length_limit_chars' each allow specifying
         * APPROXIMATE limits on the length of the result. If either limit is
         * set to zero, that limit is disabled. One of the two limits MUST
         * always be non-zero, to avoid infinite loops.
         *
         * Produces an empty line if the search words (if any) weren't found, or
         * if no data was available. Returns false in the event of an error. */
        bool produce(words_t& line, const words_t& search = words_t(),
                size_t length_limit_words = 100,
                size_t length_limit_chars = 1000);

        /* Tells the underlying backend to clean up any stale (score=0) snippets
         * it may have lying around. This may be called periodically to free up
         * resources. */
        bool prune_backend();

    private:
        /* Grows a line in both directions until length has been reached. */
        bool grow(words_t& line,
                size_t length_limit_words = 0, size_t length_limit_chars = 0);

        const backend_t backend;
        const selector_t selector;
        const scorer_t scorer;
        const size_t look_size;

        State state;
    };
}

#endif
