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
#include <vector>

#include "backend-cache.h"

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

marky::Backend_Cache::Backend_Cache(cacheable_t backend)
  : wrapme(backend) { }

marky::Backend_Cache::~Backend_Cache() { }

marky::State marky::Backend_Cache::create_state() {
    return wrapme->create_state();
}

bool marky::Backend_Cache::store_state(const State& state, scorer_t scorer) {
    if (!changed_words.empty()) {
        /* flush our changes to wrapme */
        snippet_ptr_set_t flushme;
        for (window_to_snippet_t::iterator iter = changed_words.begin();
             iter != changed_words.end(); ++iter) {
            flushme.insert(iter->second);
        }
        wrapme->flush(state, scorer, flushme);
    }
    return wrapme->store_state(state, scorer);
}

bool marky::Backend_Cache::get_random(const State& state, scorer_t scorer,
        word_t& random) {
    if (!changed_words.empty()) {
        /* flush our changes to wrapme */
        if (!store_state(state, scorer)) {
            return false;
        }
    }
    /* just pass to wrapme, to select across full dataset */
    return wrapme->get_random(state, scorer, random);
}

bool marky::Backend_Cache::get_prev(const State& state, selector_t selector,
        scorer_t scorer, const words_t& words, word_t& prev) {
#ifdef READ_DEBUG_ENABLED
    DEBUG("get_prev(%s)", str(words).c_str());
#endif
    /*
      TODO to avoid memory explosion, consider wiping get_x data whenever
      size reaches some N (also tuning)
    */

    words_to_snippets_t::iterator got_iter = got_prevs.find(words);
    if (got_iter == got_prevs.end()) {
        /* create get cache of all matching prevs */
        snippets_ptr_t got(new snippet_ptr_set_t);
        if (!wrapme->get_prevs(words, *got)) {
            /* backend failure */
            return false;
        }

        if (got->empty()) {
#ifdef READ_DEBUG_ENABLED
            DEBUG("    prev_snippet -> NOTFOUND (continue)");
#endif
            /* flag as nothing found (see pick_snippet), add to got_prevs */
            got.reset();
            got_iter = got_prevs.insert(std::make_pair(words, got)).first;
        } else {
            /* add to got_words pool (see increment_link) */
            for (snippet_ptr_set_t::const_iterator iter = got->begin();
                 iter != got->end(); ++iter) {
                got_words[(*iter)->words] = *iter;
            }
            /* also add to got_prevs */
            got_iter = got_prevs.insert(std::make_pair(words, got)).first;
        }
    }

    words_to_snippets_t::iterator changed_iter = changed_prevs.find(words);
    snippet_t prev_snippet;
    pick_snippet(got_iter->second,
            (changed_iter == changed_prevs.end()) ? snippets_ptr_t() : changed_iter->second,
            state, selector, scorer, words, prev_snippet);
    if ((bool)prev_snippet) {
        prev = prev_snippet->words.front();
    } else {
        if (words.size() >= 2) {
            /* try a shorter prefix */
            words_t search_words_shortened(words);
            search_words_shortened.pop_back();
#ifdef READ_DEBUG_ENABLED
            DEBUG("  get_prev -> %s", str(search_words_shortened).c_str());
#endif
            return get_prev(state, selector, scorer, search_words_shortened, prev);
        } else {
            prev = IBackend::LINE_START;
        }
    }
    return true;
}

bool marky::Backend_Cache::get_next(const State& state, selector_t selector,
        scorer_t scorer, const words_t& words, word_t& next) {
#ifdef READ_DEBUG_ENABLED
    DEBUG("get_next(%s)", str(words).c_str());
#endif
    /*
      TODO to avoid memory explosion, consider wiping got_x data whenever
      size reaches some N (also tuning)
    */

    words_to_snippets_t::iterator got_iter = got_nexts.find(words);
    if (got_iter == got_nexts.end()) {
        /* create get cache of all matching nexts */
        snippets_ptr_t got(new snippet_ptr_set_t);
        if (!wrapme->get_nexts(words, *got)) {
            /* backend failure */
            return false;
        }

        if (got->empty()) {
#ifdef READ_DEBUG_ENABLED
            DEBUG("    next_snippet -> NOTFOUND (continue)");
#endif
            /* flag as nothing found (see pick_snippet), add to got_nexts */
            got.reset();
            got_iter = got_nexts.insert(std::make_pair(words, got)).first;
        } else {
            /* add to got_words pool (see increment_link) */
            for (snippet_ptr_set_t::const_iterator iter = got->begin();
                 iter != got->end(); ++iter) {
                got_words[(*iter)->words] = *iter;
            }
            /* also add to got_nexts */
            got_iter = got_nexts.insert(std::make_pair(words, got)).first;
        }
    }

    words_to_snippets_t::iterator changed_iter = changed_nexts.find(words);
    snippet_t next_snippet;
    pick_snippet(got_iter->second,
            (changed_iter == changed_nexts.end()) ? snippets_ptr_t() : changed_iter->second,
            state, selector, scorer, words, next_snippet);
    if ((bool)next_snippet) {
        next = next_snippet->words.back();
    } else {
        if (words.size() >= 2) {
            /* try a shorter suffix */
            words_t search_words_shortened(++words.begin(), words.end());
#ifdef READ_DEBUG_ENABLED
            DEBUG("  get_next -> %s", str(search_words_shortened).c_str());
#endif
            return get_next(state, selector, scorer, search_words_shortened, next);
        } else {
            next = IBackend::LINE_END;
        }
    }
    return true;
}

void marky::Backend_Cache::pick_snippet(snippets_ptr_t got_snippets, snippets_ptr_t changed_snippets,
        const State& state, selector_t selector,
        scorer_t scorer, const words_t& words, snippet_t& out) {
    /*
      NOTE:
      the strategy here is to merge between changed/get at the time of get_x().
      we could also do this merging at increment_link(), but I'd expect the
      following effects if this switches to that strategy:

      - if only lots of increments are called, increment_link will be slower
      since we'll need to get all data matching the prev/next to do the merge.
      this would also increase memory usage/cache size quite a bit.
      - if only lots of gets are called, gets wont really be faster since the
      current checks against changed_map are pretty quick, especially since
      changed_map would be empty until changes are made
      - if even numbers of both are called, it'd be a wash. but i doubt this
      would come up in practice (expect most people would do bulk inserts
      followed by periodic gets)
      - if we're careful to avoid duplicates across the two, it'll be wash,
      mostly at the expense of increment_link
    */

    snippets_ptr_t selectme;
    if ((bool)got_snippets && (bool)changed_snippets) {
        /* entries found in get cache AND changed values, merge and add both.
           this shouldn't happen too often, most users will probably be either
           reading OR writing, not both simultaneously. */
        window_to_snippet_t merged;

        {
            for (snippet_ptr_set_t::const_iterator snippet_iter = got_snippets->begin();
                 snippet_iter != got_snippets->end(); ++snippet_iter) {
                merged[(*snippet_iter)->words] = *snippet_iter;
            }
        }
        {
            /* do this SECOND to override any matching entries in got_iter */
            for (snippet_ptr_set_t::const_iterator snippet_iter = changed_snippets->begin();
                 snippet_iter != changed_snippets->end(); ++snippet_iter) {
                merged[(*snippet_iter)->words] = *snippet_iter;
            }
        }

        /* select from combined list */
        selectme.reset(new snippet_ptr_set_t);
        for (window_to_snippet_t::const_iterator merge_iter = merged.begin();
             merge_iter != merged.end(); ++merge_iter) {
            selectme->insert(merge_iter->second);
        }
#ifdef READ_DEBUG_ENABLED
        DEBUG("  search%s = merged(%lu)", str(words).c_str(), selectme->size());
#endif
    } else if ((bool)got_snippets) {
        /* entries found in got cache, and NOT changed values */
        selectme = got_snippets;
#ifdef READ_DEBUG_ENABLED
        DEBUG("  search%s = got(%lu)", str(words).c_str(), selectme->size());
#endif
    } else if ((bool)changed_snippets) {
        /* entries found in changed values, and NOT get cache */
        selectme = changed_snippets;
#ifdef READ_DEBUG_ENABLED
        DEBUG("  search%s = changed(%lu)", str(words).c_str(), selectme->size());
#endif
    } else {
        /* nothing found, produce empty ptr */
#ifdef READ_DEBUG_ENABLED
        DEBUG("  search%s = NOTFOUND", str(words).c_str());
#endif
        out.reset();
        return;
    }

    out = selector(*selectme, scorer, state);
#ifdef READ_DEBUG_ENABLED
    for (snippet_ptr_set_t::const_iterator siter = selectme->begin();
         siter != selectme->end(); ++siter) {
        DEBUG("  search%s = snippet(%s, %lu)", str(words).c_str(),
                str((*siter)->words).c_str(), (*siter)->score(scorer, state));
    }
    DEBUG("    snippet -> %s", str(out->words).c_str());
#endif
}

bool marky::Backend_Cache::update_snippets(const State& state, scorer_t scorer,
        const words_to_counts::map_t& line_windows) {
#ifdef WRITE_DEBUG_ENABLED
    DEBUG("update_snippets -> %lu windows", line_windows.size());
#endif
    /*
      TODO to avoid memory explosion, consider flushing changed data (and
      merging to got_x for non-empty entries) whenever changeset size reaches
      some N (also tuning)
    */
    words_to_counts::map_t windows_to_get;
    std::vector<snippet_t> snippets_to_index;
    for (words_to_counts::map_t::const_iterator line_window_iter = line_windows.begin();
         line_window_iter != line_windows.end(); ++line_window_iter) {
        const words_t& line_window = line_window_iter->first;
#ifdef WRITE_DEBUG_ENABLED
        DEBUG("update_snippet -> %lu", str(line_window).c_str());
#endif
        window_to_snippet_t::const_iterator changed_iter =
            changed_words.find(line_window);

        if (changed_iter != changed_words.end()) {
            /* snippet already in changed_words, just readjust/increment score */
            changed_iter->second->increment(scorer, state, line_window_iter->second);
#ifdef WRITE_DEBUG_ENABLED
            DEBUG("  EXISTS: score increment %s", changed_iter->second->str().c_str());
#endif
            continue;
        }

        window_to_snippet_t::const_iterator got_iter =
            got_words.find(line_window);
        if (got_iter != got_words.end()) {
            /* snippet found in got_words, copy its content then adjust
               (could move it, but that'd involve searching for it in
               got_nexts/got_prevs as well).
               changed_words overrides got_words. */
            snippet_t snippet(new Snippet(*got_iter->second));
            snippet->increment(scorer, state, line_window_iter->second);
            changed_words[line_window] = snippet;
            snippets_to_index.push_back(snippet);
        } else {
            /* snippet not found in cache, see if backend has it */
            windows_to_get.insert(*line_window_iter);
        }
    }

    /* bulk-query the backend for the snippets we don't already have cached,
       then add them to cache. */
    ICacheable::words_to_snippet_t snippets_from_backend;
    if (!wrapme->get_snippets(windows_to_get, snippets_from_backend)) {
        /* backend failure */
        return false;
    }

    /* create snippet objects for what we got back, creating new ones as needed. */
    for (words_to_counts::map_t::const_iterator line_window_iter = windows_to_get.begin();
         line_window_iter != windows_to_get.end(); ++line_window_iter) {
        ICacheable::words_to_snippet_t::const_iterator backend_snippet =
            snippets_from_backend.find(line_window_iter->first);
        snippet_t snippet;
        if (backend_snippet != snippets_from_backend.end()) {
            /* backend had it, increment its score */
            snippet = backend_snippet->second;
            snippet->increment(scorer, state, line_window_iter->second);
        } else {
            /* backend doesn't have it either. create a new entry. */
            snippet.reset(new Snippet(line_window_iter->first,
                            state.time, state.count, line_window_iter->second));
        }
        changed_words[snippet->words] = snippet;
        snippets_to_index.push_back(snippet);
    }

    /* for newly cached snippets, index them. */
    for (std::vector<snippet_t>::const_iterator snippet_iter = snippets_to_index.begin();
         snippet_iter != snippets_to_index.end(); ++snippet_iter) {
        const snippet_t& snippet = *snippet_iter;
        /* add the snippet to changed_prevs/changed_nexts */

        /* nexts table: window[:-1] -> window[-1] */
        words_t words_subset = snippet->words;
        words_subset.pop_back();// all except back
#ifdef WRITE_DEBUG_ENABLED
        DEBUG("  NEW: nexts %s -> %s", str(words_subset).c_str(), str(snippet->words).c_str());
#endif
        words_to_snippets_t::iterator nexts_iter =
            changed_nexts.find(words_subset);
        if (nexts_iter == changed_nexts.end()) {
            snippets_ptr_t snippets(new snippet_ptr_set_t);
            nexts_iter = changed_nexts.insert(
                std::make_pair(words_subset, snippets)).first;
        }
        nexts_iter->second->insert(snippet);

        /* prevs table: window[1:] -> window[0] */
        words_subset.push_back(snippet->words.back());
        words_subset.pop_front();// all except front (from all except back)
#ifdef WRITE_DEBUG_ENABLED
        DEBUG("  NEW: prevs %s -> %s", str(words_subset).c_str(), str(snippet->words).c_str());
#endif
        words_to_snippets_t::iterator prevs_iter =
            changed_prevs.find(words_subset);
        if (prevs_iter == changed_prevs.end()) {
            snippets_ptr_t snippets(new snippet_ptr_set_t);
            prevs_iter = changed_prevs.insert(
                std::make_pair(words_subset, snippets)).first;
        }
        prevs_iter->second->insert(snippet);
    }

    return true;
}

bool marky::Backend_Cache::prune(const State& state, scorer_t scorer) {
    if (changed_words.empty()) {
#ifdef WRITE_DEBUG_ENABLED
        DEBUG("nothing to flush");
#endif
        return wrapme->prune(state, scorer);
    }

    /* flush changes to wrapme */
    snippet_ptr_set_t flushme;

#ifdef WRITE_DEBUG_ENABLED
    DEBUG("%lu to flush", changed_words.size());
#endif
    for (window_to_snippet_t::const_iterator iter = changed_words.begin();
         iter != changed_words.end(); ++iter) {
#ifdef WRITE_DEBUG_ENABLED
        DEBUG("  flush: %s", iter->second->str().c_str());
#endif
        flushme.insert(iter->second);
    }

    if (!wrapme->flush(state, scorer, flushme)) {
        return false;
    }

    /* now just wipe the cache and start from scratch.
       TODO could someday have a merge from changed_* into got_*, but it's not
       clear if that'd benefit things too much. also this helps in terms of
       keeping the cache size under control */
    got_words.clear();
    got_prevs.clear();
    got_nexts.clear();
    changed_words.clear();
    changed_prevs.clear();
    changed_nexts.clear();

    return wrapme->prune(state, scorer);
}
