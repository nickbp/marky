#ifndef MARKY_SNIPPET_H
#define MARKY_SNIPPET_H

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

#include <stddef.h>//size_t
#include <time.h>//time_t

#include <list>
#include <memory>
#include <string>
#include <unordered_set>

namespace marky {
    typedef std::string word_t;
    typedef std::list<word_t> words_t;

    typedef size_t score_t;

    /* A container for the current state of the backend.
       Used by scorers to adjust a snippet's score */
    class State {
      public:
        State(time_t time, size_t count)
            : time(time), count(count) { }

        time_t time;
        size_t count;
    };

    class Snippet;
    /* Calculate the adjusted score for a snippet.
     * 'score_state' is the backend state from the last time the snippet was encountered,
     * 'cur_state' is the backend state from right now. */
    typedef std::function<score_t (score_t score,
            const State& last_score_state,
            const State& now_state)> scorer_t;

    /* A list of words, paired with a scoring/state for that list.
     * This is used in the context of words -> word + scoring. */
    class Snippet {
      public:
        Snippet(const words_t& words, time_t time, size_t count, score_t score = 1)
            : words(words), state_(time, count), score_(score) { }

        /* get adjusted score according to the given state */
        inline score_t score(scorer_t scorer, const State& cur_state) {
            return scorer(score_, state_, cur_state);
        }
        /* increments score and adjusts according to the given state */
        inline score_t increment(scorer_t scorer, const State& cur_state, score_t inc_amount = 1) {
            /* give scorer our current state */
            score_ = inc_amount + score(scorer, cur_state);
            /* reset the state 'clock' to now */
            state_ = cur_state;
            return score_;
        }

        /* get current score as of last-seen state */
        inline score_t cur_score() const {
            return score_;
        }
        /* get current last-seen state */
        inline const State& cur_state() const {
            return state_;
        }

        inline bool operator==(const Snippet& other) const {
            return words == other.words;
        }

        std::string str() const;

        const words_t words;

      private:
        /* state_ holds the last time this snippet was seen, and its 'score',
         * which is incremented each time the snippet is encountered and
         * effectively decremented as other snippets appear */
        State state_;
        /* updated alongside state_ when increment() is called */
        score_t score_;
    };
    typedef std::shared_ptr<Snippet> snippet_t;
    typedef std::unordered_set<snippet_t> snippet_ptr_set_t;
}

namespace std {
    template<>
    struct hash<marky::words_t> {
        size_t operator()(const marky::words_t& words) const {
            switch (words.size()) {
                case 0:
                    return 0;
                case 1:
                    return std::hash<marky::word_t>()(words.front());
                default:
                    return std::hash<marky::word_t>()(words.front()) ^
                        std::hash<marky::word_t>()(words.back());
            }
        }
    };

    template<>
    struct hash<marky::Snippet> {
        size_t operator()(const marky::Snippet& snippet) const {
            return std::hash<marky::words_t>()(snippet.words);
        }
    };
}

#endif
