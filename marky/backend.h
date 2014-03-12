#ifndef MARKY_BACKEND_H
#define MARKY_BACKEND_H

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

#include "snippet.h"
#include "scorer.h"
#include "selector.h"

namespace marky {
    class words_to_counts {
      public:
        typedef std::unordered_map<words_t, size_t> map_t;
        void increment(const words_t& words) {
            map_[words]++;
        }
        const map_t& map() const {
            return map_;
        }
      private:
        map_t map_;
    };

    /* Base interface for storing/retrieving strings of words from some kind of
     * storage. */
    class IBackend {
      public:
        typedef std::shared_ptr<snippet_ptr_set_t> snippets_ptr_t;

        /* A "word" which marks the start of a line when passed to
         * update_scores(). */
        static const word_t LINE_START;

        /* A "word" which marks the end of a line when passed to
         * update_scores(). */
        static const word_t LINE_END;

        virtual ~IBackend() { }

        /* Creates a starting state object. The state may be retrieved from the
         * backend's storage from a previous run, or may be created from scratch
         * with reasonable values. */
        virtual State create_state() = 0;

        /* Stores the final state object, before the backend is destroyed.
         * This may be a no-op if the Backend lacks persistent storage.
         * Return false in the event of a backend error. */
        virtual bool store_state(const State& state, scorer_t scorer) = 0;

        /* Gets a random available word, or LINE_END if none was available.
         * This word may be grown out into a line using get_prev()/get_next().
         * Return false in the event of a backend error. */
        virtual bool get_random(const State& state, scorer_t scorer,
                word_t& word) = 0;

        /* Finds a word that precedes 'search_words' or a subset thereof, or
         * LINE_START if none was found.
         * Return false in the event of a backend error. */
        virtual bool get_prev(const State& state, selector_t selector,
                scorer_t scorer, const words_t& search_words, word_t& prev) = 0;

        /* Finds a word that follows 'search_words' or a subset thereof, or
         * LINE_END if none was found.
         * Return false in the event of a backend error. */
        virtual bool get_next(const State& state, selector_t selector,
                scorer_t scorer, const words_t& search_words, word_t& next) = 0;

        /* For a given set of snippets, updates their scores, creating new
         * records if necessary. The list is treated as being a fragment of a
         * longer sequence, where word adjacency simply needs to be recorded
         * between the words of the fragment. Returns false in the event of a
         * backend error. */
        virtual bool update_snippets(const State& state, scorer_t scorer,
                const words_to_counts::map_t& line_windows) = 0;

        /* Prunes/updates existing records according to the rules of the scorer.
         * Any records with an adjusted score of 0 are removed. This is assumed
         * to only be called periodically, so it's not necessarily fast. */
        virtual bool prune(const State& state, scorer_t scorer) = 0;
    };
    typedef std::shared_ptr<IBackend> backend_t;

    /* Backends which support being passed to Backend_Cache should implement
     * this interface. */
    class ICacheable : public IBackend {
      public:
        typedef std::unordered_map<words_t, snippet_t> words_to_snippet_t;

        virtual ~ICacheable() { }

        /* Get all snippets which end with 'words', or an empty list if no
         * snippet is found. Return false in the event of a backend error. */
        virtual bool get_prevs(const words_t& words, snippet_ptr_set_t& out) = 0;

        /* Get all snippets which start with 'words', or an empty list if no
         * snippet is found. Return false in the event of a backend error. */
        virtual bool get_nexts(const words_t& words, snippet_ptr_set_t& out) = 0;

        /* Get a snippet for each of the requested sets of words, only
         * populating the output map with found entries. Return false in the
         * event of a backend error. */
        virtual bool get_snippets(const words_to_counts::map_t& windows,
                words_to_snippet_t& out) = 0;

        /* Update the backend with the provided snippet data and state. This is
         * conceptually for flushing the result of several increment_snippet()s. */
        virtual bool flush(const State& state, scorer_t scorer,
                const snippet_ptr_set_t& snippets) = 0;
    };
    typedef std::shared_ptr<ICacheable> cacheable_t;
}

#endif
