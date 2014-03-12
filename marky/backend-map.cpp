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

#include <time.h>

#include "backend-map.h"
#include "rand-util.h"

//#define READ_DEBUG_ENABLED
//#define WRITE_DEBUG_ENABLED

#if (defined(READ_DEBUG_ENABLED) || defined(WRITE_DEBUG_ENABLED))
#include "config.h"
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

marky::Backend_Map::Backend_Map()
    : prevs(), nexts(), snippets(), random_snippet(snippets.end()) { }

marky::State marky::Backend_Map::create_state() {
    return State(time(NULL), 0);
}

bool marky::Backend_Map::store_state(const State& /*state*/, scorer_t /*scorer*/) {
    /* not applicable */
    return true;
}

bool marky::Backend_Map::get_random(const State& /*state*/, scorer_t /*scorer*/, word_t& word) {
    if (prevs.empty()) {
        word = IBackend::LINE_END;
        return true;
    }

    if (random_snippet == snippets.end()) {
        /* seek words_iter to a random location in words */
        unsigned int offset = pick_rand(snippets.size());
        random_snippet = snippets.begin();
        for (unsigned int i = 0; i < offset; ++i) {
            ++random_snippet;
        }
    }

    /* put a little effort into finding a non-end/start word */
    word = random_snippet->second->words.front();
    if (word == IBackend::LINE_START) {
        word = random_snippet->second->words.back();
    }

    /* increment for a future get_random() call. */
    ++random_snippet;
    return true;
}

bool marky::Backend_Map::get_prev(const State& state, selector_t selector,
        scorer_t scorer, const words_t& search_words, word_t& prev) {
#ifdef READ_DEBUG_ENABLED
    DEBUG("get_prev(%s)", str(search_words).c_str());
#endif
#if 0
    for (words_to_snippets_t::const_iterator witer = prevs.begin();
         witer != prevs.end(); ++witer) {
        const snippet_ptr_set_t& snippets = *witer->second;
        for (snippet_ptr_set_t::const_iterator siter = snippets.begin();
             siter != snippets.end(); ++siter) {
            DEBUG("  prevs%s = snippet(%s, %lu)", str(witer->first).c_str(),
                    str((*siter)->words).c_str(), (*siter)->score(scorer, state));
        }
    }
#endif
    words_to_snippets_t::const_iterator iter = prevs.find(search_words);
    if (iter == prevs.end()) {
        if (search_words.size() >= 2) {
            words_t search_words_shortened(search_words);
            search_words_shortened.pop_back();
#ifdef READ_DEBUG_ENABLED
            DEBUG("get_prev -> %s", str(search_words_shortened).c_str());
#endif
            /* recurse with shorter search */
            return get_prev(state, selector, scorer, search_words_shortened, prev);
        } else {
#ifdef READ_DEBUG_ENABLED
            DEBUG("    prev_snippet -> NOTFOUND");
#endif
            prev = IBackend::LINE_START;
        }
    } else {
        const words_t& prev_snippet = selector(*iter->second, scorer, state)->words;
#ifdef READ_DEBUG_ENABLED
        const snippet_ptr_set_t& snippets = *iter->second;
        for (snippet_ptr_set_t::const_iterator siter = snippets.begin();
             siter != snippets.end(); ++siter) {
            DEBUG("  prevs%s = snippet(%s, %lu)", str(search_words).c_str(),
                    str((*siter)->words).c_str(), (*siter)->score(scorer, state));
        }
        DEBUG("    prev_snippet -> %s", str(prev_snippet).c_str());
#endif
        prev = prev_snippet.front();
    }
    return true;
}

bool marky::Backend_Map::get_next(const State& state, selector_t selector,
        scorer_t scorer, const words_t& search_words, word_t& next) {
#ifdef READ_DEBUG_ENABLED
    DEBUG("get_next(%s)", str(search_words).c_str());
#endif
#if 0
    for (words_to_snippets_t::const_iterator witer = nexts.begin();
         witer != nexts.end(); ++witer) {
        const snippet_ptr_set_t& snippets = *witer->second;
        for (snippet_ptr_set_t::const_iterator siter = snippets.begin();
             siter != snippets.end(); ++siter) {
            DEBUG("  nexts%s = snippet(%s, %lu)", str(witer->first).c_str(),
                    str((*siter)->words).c_str(), (*siter)->score(scorer, state));
        }
    }
#endif
    words_to_snippets_t::const_iterator iter = nexts.find(search_words);
    if (iter == nexts.end()) {
        if (search_words.size() >= 2) {
            words_t search_words_shortened(++search_words.begin(), search_words.end());
#ifdef READ_DEBUG_ENABLED
            DEBUG("  get_next -> %s", str(search_words_shortened).c_str());
#endif
            /* recurse with shorter search */
            return get_next(state, selector, scorer, search_words_shortened, next);
        } else {
#ifdef READ_DEBUG_ENABLED
            DEBUG("    next_snippet -> NOTFOUND");
#endif
            next = IBackend::LINE_END;
        }
    } else {
        const words_t& next_snippet = selector(*iter->second, scorer, state)->words;
#ifdef READ_DEBUG_ENABLED
        const snippet_ptr_set_t& snippets = *iter->second;
        for (snippet_ptr_set_t::const_iterator siter = snippets.begin();
             siter != snippets.end(); ++siter) {
            DEBUG("  nexts%s = snippet(%s, %lu)", str(search_words).c_str(),
                    str((*siter)->words).c_str(), (*siter)->score(scorer, state));
        }
        DEBUG("    next_snippet -> %s", str(next_snippet).c_str());
#endif
        next = next_snippet.back();
    }
    return true;
}

bool marky::Backend_Map::update_snippets(const State& state, scorer_t scorer,
        const words_to_counts::map_t& line_windows) {
#ifdef WRITE_DEBUG_ENABLED
    DEBUG("update_score -> %lu windows", line_windows.size());
#endif
    for (words_to_counts::map_t::const_iterator line_window_iter = line_windows.begin();
         line_window_iter != line_windows.end(); ++line_window_iter) {
        window_to_snippet_t::iterator cur_snippet_iter = snippets.find(line_window_iter->first);
        if (cur_snippet_iter != snippets.end()) {
            /* readjust/increment scores */
            cur_snippet_iter->second->increment(scorer, state, line_window_iter->second);
#ifdef WRITE_DEBUG_ENABLED
            DEBUG("  EXISTS: score increment %s", cur_snippet_iter->second->str().c_str());
#endif
            continue;
        }

        /* window is new, create and add to maps */
        snippet_t snippet(new Snippet(line_window_iter->first, state.time, state.count, line_window_iter->second));
#ifdef WRITE_DEBUG_ENABLED
        DEBUG("  NEW: %s", snippet->str().c_str());
#endif
        snippets[line_window_iter->first] = snippet;
        random_snippet = snippets.end();/* invalidate after map modification */

        /* nexts table: window[:-1] -> window[-1] */
        words_t words_subset = line_window_iter->first;
        words_subset.pop_back();// all except back
#ifdef WRITE_DEBUG_ENABLED
        DEBUG("  NEW: nexts %s -> %s", str(words_subset).c_str(), str(snippet->words).c_str());
#endif
        words_to_snippets_t::iterator nexts_iter = nexts.find(words_subset);
        if (nexts_iter == nexts.end()) {
            nexts_iter = nexts.insert(std::make_pair(words_subset, snippets_ptr_t(new snippet_ptr_set_t))).first;
        }
        nexts_iter->second->insert(snippet);

        /* prevs table: window[1:] -> window[0] */
        words_subset.push_back(line_window_iter->first.back());
        words_subset.pop_front();// all except front (from all except back)
#ifdef WRITE_DEBUG_ENABLED
        DEBUG("  NEW: prevs %s -> %s", str(words_subset).c_str(), str(snippet->words).c_str());
#endif
        words_to_snippets_t::iterator prevs_iter = prevs.find(words_subset);
        if (prevs_iter == prevs.end()) {
            prevs_iter = prevs.insert(std::make_pair(words_subset, snippets_ptr_t(new snippet_ptr_set_t))).first;
        }
        prevs_iter->second->insert(snippet);
    }

    return true;
}

bool marky::Backend_Map::prune(const State& state, scorer_t scorer) {
    std::list<window_to_snippet_t::iterator> to_erase;
    for (window_to_snippet_t::iterator snippets_iter = snippets.begin();
         snippets_iter != snippets.end(); ++snippets_iter) {
        snippet_t snippet = snippets_iter->second;
        if (snippet->score(scorer, state) > 0) {
            /* this snippet still has a score, doesn't need pruning */
            continue;
        }

        /* mark for removal from snippets */
        to_erase.push_back(snippets_iter);

        const words_t& words = snippets_iter->second->words;

        /* remove from nexts (find matching prev) */
        words_t words_subset = words;
        words_subset.pop_back();// all except back
        words_to_snippets_t::iterator nexts_iter = nexts.find(words_subset);
        if (nexts_iter != nexts.end()) {
            nexts_iter->second->erase(snippet);
            if (nexts_iter->second->empty()) {
                nexts.erase(nexts_iter);
            }
        }

        /* remove from prevs (find matching next) */
        words_subset.push_back(words.back());
        words_subset.pop_front();// all except front (from all except back)
        words_to_snippets_t::iterator prevs_iter = prevs.find(words_subset);
        if (prevs_iter != prevs.end()) {
            prevs_iter->second->erase(snippet);
            if (prevs_iter->second->empty()) {
                prevs.erase(prevs_iter);
            }
        }
    }
    if (!to_erase.empty()) {
        for (std::list<window_to_snippet_t::iterator>::const_iterator to_erase_iter = to_erase.begin();
             to_erase_iter != to_erase.end(); ++to_erase_iter) {
            snippets.erase(*to_erase_iter);
        }
        random_snippet = snippets.end();/* invalidate iter after modification */
    }

#ifdef WRITE_DEBUG_ENABLED
    DEBUG("AFTER PRUNE:");
    for (window_to_snippet_t::const_iterator witer = snippets.begin();
         witer != snippets.end(); ++witer) {
        DEBUG("  snippets%s = snippet(%s, %lu)", str(witer->first).c_str(),
                str(witer->second->words).c_str(), witer->second->score(scorer, state));
    }
    for (words_to_snippets_t::const_iterator witer = prevs.begin();
         witer != prevs.end(); ++witer) {
        const snippet_ptr_set_t& snippets = *witer->second;
        for (snippet_ptr_set_t::const_iterator siter = snippets.begin();
             siter != snippets.end(); ++siter) {
            DEBUG("  prevs%s = snippet(%s, %lu)", str(witer->first).c_str(),
                    str((*siter)->words).c_str(), (*siter)->score(scorer, state));
        }
    }
    for (words_to_snippets_t::const_iterator witer = nexts.begin();
         witer != nexts.end(); ++witer) {
        const snippet_ptr_set_t& snippets = *witer->second;
        for (snippet_ptr_set_t::const_iterator siter = snippets.begin();
             siter != snippets.end(); ++siter) {
            DEBUG("  nexts%s = snippet(%s, %lu)", str(witer->first).c_str(),
                    str((*siter)->words).c_str(), (*siter)->score(scorer, state));
        }
    }
#endif
    return true;
}
