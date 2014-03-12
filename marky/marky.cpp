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

#include "marky.h"
#include <assert.h>

#include "config.h"
#ifdef DEBUG_ENABLED
#include <sstream>

static std::string str(const marky::words_t& words) {
    std::ostringstream oss;
    oss << "['";
    for (marky::words_t::const_iterator iter = words.begin();
         iter != words.end(); ) {
        oss << *iter;
        if (++iter != words.end()) {
            oss << "', '";
        }
    }
    oss << "']";
    return oss.str();
}
#endif

marky::Marky::Marky(backend_t backend, selector_t selector, scorer_t scorer,
        size_t look_size)
    : backend(backend), selector(selector), scorer(scorer),
      look_size(look_size), state(backend->create_state()) {
    assert(backend);
    assert(selector);
    assert(scorer);
}

marky::Marky::~Marky() {
    backend->store_state(state, scorer);
}

bool marky::Marky::insert(const words_t& line) {
    if (line.empty()) {
        return true;
    }

    /* update time BEFORE all scoring */
    state.time = time(NULL);/* = now */

    /*
      Eg given "A Good Dog", with look_size=2:

      forewards                            : backwards
      :                      "START" <- "A"
      "START,A" -> "Good", "START" -> "A"  : "START" <- "A,Good", "A" <- "Good"
      "A,Good" -> "Dog",   "A" -> "Good"   : "A" <- "Good,Dog",   "Good" <- "Dog"
      "Good,Dog" -> "END", "Good" -> "Dog" : "Good" <- "Dog,END", "Dog" <- "END"
      "Dog" -> "END"

      "A Good Dog", with look_size=1:

      forewards       : backwards
      "START" -> "A"  : "START" <- "A"
      "A" -> "Good"   : "A" <- "Good"
      "Good" -> "Dog" : "Good" <- "Dog"
      "Dog" -> "END"  : "Dog" <- "END"

      note how =2 just gives an extra window to scan in either direction!
      so we can just grow the sliding window over and over bam done
    */
    words_to_counts line_windows;
    const size_t line_size_with_endcaps = line.size() + 2;
    for (size_t window_size = 1;
         window_size <= look_size && window_size <= line_size_with_endcaps;
         ++window_size) {
        /* set up initial window */
        words_t line_window;
        line_window.push_back(IBackend::LINE_START);
        words_t::const_iterator line_iter = line.begin();
        while (line_iter != line.end() && line_window.size() <= window_size) {
            line_window.push_back(*line_iter);
            ++line_iter;
        }
        if (line_window.size() < window_size) {
            /* special case where window is exactly START, ..., END */
            line_window.push_back(IBackend::LINE_END);
            line_windows.increment(line_window);
            continue;
        }
        /* score the starting window */
        line_windows.increment(line_window);
        /* shift window until end, scoring along the way */
        while (line_iter != line.end()) {
            line_window.pop_front();
            line_window.push_back(*line_iter);
            line_windows.increment(line_window);
            ++line_iter;
        }
        /* score the ending window */
        line_window.pop_front();
        line_window.push_back(IBackend::LINE_END);
        line_windows.increment(line_window);
    }

    if (!backend->update_snippets(state, scorer, line_windows.map())) {
        return false;
    }

    /* increment line count AFTER, EACH line is added (first line gets id 0) */
    ++state.count;

    return true;
}

bool marky::Marky::produce(words_t& line, const words_t& search/*=words_t()*/,
        size_t length_limit_words/*=0*/, size_t length_limit_chars/*=0*/) {
    if (search.empty()) {
        word_t rand_word;
        if (!backend->get_random(state, scorer, rand_word)) {/* backend err */
            return false;
        }
        if (rand_word == IBackend::LINE_END) {/* no data */
            return true;
        }
        line.push_back(rand_word);
        return grow(line, length_limit_words, length_limit_chars);
    } else {
        line.insert(line.end(), search.begin(), search.end());
        if (!grow(line, length_limit_words, length_limit_chars)) {/* backend err */
            line.clear();
            return false;
        } else if (line.size() == 1) {/* didn't find 'search' */
            line.clear();
        }
        return true;
    }
}

bool marky::Marky::prune_backend() {
    return backend->prune(state, scorer);
}

#define CHECK_LIMIT(size, limit) (limit == 0 || size < limit)
#define CHECK_CHAR_LIMIT(char_size, limit) (limit == 0 || char_size < limit)

bool marky::Marky::grow(words_t& line,
        size_t length_limit_words/*=0*/, size_t length_limit_chars/*=0*/) {
#ifdef DEBUG_ENABLED
    DEBUG("line: %s", str(line).c_str());
#endif
    if (line.empty()) {
        return false;
    }
    /* flags marking whether we've hit a dead end in either direction: */
    bool left_dead = false, right_dead = false;
    size_t char_size = 0;
    for (words_t::const_iterator iter = line.begin();
         iter != line.end(); ++iter) {
        char_size += iter->size();/* ignore space between words */
    }

    words_t start_search_words, end_search_words;
    for (words_t::const_iterator start_iter = line.begin();
         start_iter != line.end() && start_search_words.size() < look_size;
         ++start_iter) {
        start_search_words.push_back(*start_iter);
    }
#ifdef DEBUG_ENABLED
    DEBUG("start_search_words: %s", str(start_search_words).c_str());
#endif
    for (words_t::const_reverse_iterator end_iter = line.rbegin();
         end_iter != line.rend() && end_search_words.size() < look_size;
         ++end_iter) {
        end_search_words.push_front(*end_iter);
    }
#ifdef DEBUG_ENABLED
    DEBUG("end_search_words: %s", str(end_search_words).c_str());
#endif

    word_t found_word;
    while (!left_dead || !right_dead) {
        if (!CHECK_LIMIT(line.size(), length_limit_words) ||
                !CHECK_LIMIT(char_size, length_limit_chars)) {
            break;
        }

        if (!right_dead) {
            /* add a word to the right side of 'line' */
            if (!backend->get_next(state, selector, scorer, end_search_words, found_word)) {
                return false;
            }
            if (found_word == IBackend::LINE_END) {
                /* end of line */
#ifdef DEBUG_ENABLED
                DEBUG("HIT LINE END.");
#endif
                right_dead = true;
            } else {
                /* shift search words: add the word we found */
                end_search_words.push_back(found_word);
                if (end_search_words.size() > look_size) {
                    end_search_words.pop_front();
                }
#ifdef DEBUG_ENABLED
                DEBUG("found next!: end_search_words=%s", str(end_search_words).c_str());
#endif

                char_size += found_word.size();/* ignore space between words */
                line.push_back(found_word);
            }
        }

        if (!CHECK_LIMIT(line.size(), length_limit_words) ||
                !CHECK_LIMIT(char_size, length_limit_chars)) {
            break;
        }

        if (!left_dead) {
            /* add a word to the left side of 'line' */
            if (!backend->get_prev(state, selector, scorer, start_search_words, found_word)) {
                return false;
            }
            if (found_word == IBackend::LINE_START) {
                /* start of line */
#ifdef DEBUG_ENABLED
                DEBUG("HIT LINE START.");
#endif
                left_dead = true;
            } else {
                /* shift search words: add the word we found */
                start_search_words.push_front(found_word);
                if (start_search_words.size() > look_size) {
                    start_search_words.pop_back();
                }
#ifdef DEBUG_ENABLED
                DEBUG("found prev!: start_search_words=%s", str(start_search_words).c_str());
#endif

                char_size += found_word.size();/* ignore space between words */
                line.push_front(found_word);
            }
        }
    }
    return true;
}
