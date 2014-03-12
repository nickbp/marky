#ifndef MARKY_BACKEND_MAP_H
#define MARKY_BACKEND_MAP_H

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

#include <unordered_map>

#include "backend.h"

namespace marky {
    /* A simple one-off backend which loses all state upon destruction. */
    class Backend_Map : public IBackend {
    public:
        Backend_Map();

        State create_state();
        bool store_state(const State& state, scorer_t scorer);

        bool get_random(const State& state, scorer_t scorer, word_t& word);

        bool get_prev(const State& state, selector_t selector, scorer_t scorer,
                const words_t& search_words, word_t& prev);
        bool get_next(const State& state, selector_t selector, scorer_t scorer,
                const words_t& search_words, word_t& next);

        bool update_snippets(const State& state, scorer_t scorer,
                const words_to_counts::map_t& line_windows);

        bool prune(const State& state, scorer_t scorer);

    private:
        typedef std::unordered_map<words_t, snippets_ptr_t> words_to_snippets_t;
        words_to_snippets_t prevs;/* suffix words -> snippet containing previous word */
        words_to_snippets_t nexts;/* prefix words -> snippet containing next word */

        typedef std::unordered_map<words_t, snippet_t> window_to_snippet_t;
        window_to_snippet_t snippets;/* window -> snippet */
        window_to_snippet_t::const_iterator random_snippet;
    };
}

#endif
