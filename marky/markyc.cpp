/*
  marky - A Markov chain generator.
  Copyright (C) 2014  Nicholas Parker

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

#include <assert.h>
#include <string.h>

#include "markyc.h"
#include "build-config.h"
#include "config.h"

#include "backend-map.h"
#include "backend-cache.h"
#ifdef BUILD_BACKEND_SQLITE
#include "backend-sqlite.h"
#endif
#include "selector.h"
#include "scorer.h"
#include "marky.h"

struct marky_Backend {
    marky_Backend(const marky::backend_t& backend) : wrapped(backend) { }
    const marky::backend_t wrapped;
};
struct marky_Backend_Cacheable {
    marky_Backend_Cacheable(const marky::cacheable_t& backend) : wrapped(backend) { }
    const marky::cacheable_t wrapped;
};
struct marky_Selector {
    marky_Selector(marky::selector_t selector) : wrapped(selector) { }
    marky::selector_t wrapped;
};
struct marky_Scorer {
    marky_Scorer(marky::scorer_t scorer) : wrapped(scorer) { }
    marky::scorer_t wrapped;
};
struct marky_Marky {
    marky_Marky(marky::backend_t backend, marky::selector_t selector,
            marky::scorer_t scorer, size_t look_size)
        : wrapped(backend, selector, scorer, look_size) { }
    marky::Marky wrapped;
};

// WORDS

marky_words_t* marky_words_new(size_t words_count) {
    marky_words_t* words = new marky_words_t();
    if (words == NULL) {
        return NULL;
    }
    words->words = (char**)calloc(words_count, sizeof(char*));
    if (words->words == NULL) {
        delete words;
        return NULL;
    }
    for (size_t i = 0; i < words_count; ++i) {
        words->words[i] = NULL;
    }
    words->words_count = words_count;
    return words;
}

void marky_words_free(marky_words_t* line) {
    if (line == NULL) {
        return;
    }
    assert((line->words == NULL) == (line->words_count == 0));
    if (line->words != NULL) {
        for (size_t i = 0; i < line->words_count; ++i) {
            free(line->words[i]);
        }
        free(line->words);
    }
    delete line;
}

// MARKY FRONTEND

marky_Marky* marky_new(marky_Backend* backend, marky_Selector* selector,
        marky_Scorer* scorer, size_t look_size) {
    assert(backend != NULL);
    assert(selector != NULL);
    assert(scorer != NULL);
    return new marky_Marky(backend->wrapped, selector->wrapped, scorer->wrapped, look_size);
}

void marky_free(marky_Marky* marky) {
    delete marky;
}

namespace {
    void words_to_cpp(const marky_words_t& line, marky::words_t& line_cpp) {
        for (size_t i = 0; i < line.words_count; ++i) {
            line_cpp.push_back(std::string(line.words[i]));
        }
    }

    marky_words_t* words_to_c(const marky::words_t& line_cpp) {
        marky_words_t* line = marky_words_new(line_cpp.size());
        if (line == NULL) {
            return NULL;
        }
        size_t position = 0;
        for (marky::words_t::const_iterator iter = line_cpp.begin();
             iter != line_cpp.end(); ++iter) {
            line->words[position] = (char*)malloc(iter->size() + 1);
            if (line->words[position] == NULL) {
                marky_words_free(line);
                return NULL;
            }
            strncpy(line->words[position], iter->c_str(), iter->size() + 1);
            ++position;
        }
        return line;
    }

    template <typename IN_TYPE, typename OUT_TYPE>
    OUT_TYPE* check_ptr(const std::shared_ptr<IN_TYPE>& in_ptr) {
        if ((bool)in_ptr) {
            return new OUT_TYPE(in_ptr);
        } else {
            return NULL;
        }
    }
}

int marky_insert(marky_Marky* marky, const marky_words_t* line) {
    assert(marky != NULL);
    assert(line != NULL);
    marky::words_t line_cpp;
    words_to_cpp(*line, line_cpp);
    if (marky->wrapped.insert(line_cpp)) {
        return MARKY_SUCCESS;
    } else {
        return MARKY_FAILURE;
    }
}

int marky_produce(marky_Marky* marky,
        marky_words_t** line_out, const marky_words_t* search/*=NULL*/,
        size_t length_limit_words/*=100*/, size_t length_limit_chars/*=1000*/) {
    assert(marky != NULL);
    marky::words_t search_cpp;
    if (search != NULL) {
        words_to_cpp(*search, search_cpp);
    }
    marky::words_t line_out_cpp;
    if (!marky->wrapped.produce(line_out_cpp, search_cpp, length_limit_words, length_limit_chars)) {
        return MARKY_FAILURE;
    }
    if (line_out_cpp.empty()) {
        /* nothing found */
        *line_out = NULL;
        return MARKY_SUCCESS;
    }

    *line_out = words_to_c(line_out_cpp);
    if (*line_out == NULL)  {
        /* malloc failed */
        return MARKY_FAILURE;
    }
    return MARKY_SUCCESS;
}

int marky_prune_backend(marky_Marky* marky) {
    assert(marky != NULL);
    if (marky->wrapped.prune_backend()) {
        return MARKY_SUCCESS;
    } else {
        return MARKY_FAILURE;
    }
}

// BACKENDS

marky_Backend* marky_backend_new_map(void) {
    marky::backend_t backend(new marky::Backend_Map());
    return check_ptr<marky::IBackend, marky_Backend>(backend);
}
marky_Backend* marky_backend_new_cache(marky_Backend_Cacheable* backend) {
    assert(backend != NULL);
    return check_ptr<marky::IBackend, marky_Backend>(marky::backend_t(new marky::Backend_Cache(backend->wrapped)));
}
marky_Backend* marky_backend_new_sqlite_direct(char* db_file_path) {
    assert(db_file_path != NULL);
#ifdef BUILD_BACKEND_SQLITE
    return check_ptr<marky::IBackend, marky_Backend>(marky::Backend_SQLite::create_backend(db_file_path));
#else
    return NULL;
#endif
}
marky_Backend_Cacheable* marky_backend_new_sqlite_cacheable(char* db_file_path) {
    assert(db_file_path != NULL);
#ifdef BUILD_BACKEND_SQLITE
    return check_ptr<marky::ICacheable, marky_Backend_Cacheable>(marky::Backend_SQLite::create_cacheable(db_file_path));
#else
    return NULL;
#endif
}
int marky_has_sqlite(void) {
    return config::has_sqlite() ? MARKY_SUCCESS : MARKY_FAILURE;
}

void marky_backend_free(marky_Backend* backend) {
    delete backend;
}
void marky_backend_cacheable_free(marky_Backend_Cacheable* backend) {
    delete backend;
}

// SELECTORS

marky_Selector* marky_selector_new_best_always(void) {
    return new marky_Selector(marky::selectors::best_always());
}
marky_Selector* marky_selector_new_random(void) {
    return new marky_Selector(marky::selectors::random());
}
marky_Selector* marky_selector_new_best_weighted(uint8_t weight_factor/*=128*/) {
    return new marky_Selector(marky::selectors::best_weighted(weight_factor));
}

void marky_selector_free(marky_Selector* selector) {
    delete selector;
}

// SCORERS

marky_Scorer* marky_scorer_new_no_adj(void) {
    return new marky_Scorer(marky::scorers::no_adj());
}
marky_Scorer* marky_scorer_new_word_adj(size_t score_decrement_words) {
    return new marky_Scorer(marky::scorers::word_adj(score_decrement_words));
}
marky_Scorer* marky_scorer_new_time_adj(size_t score_decrement_seconds) {
    return new marky_Scorer(marky::scorers::time_adj(score_decrement_seconds));
}

void marky_scorer_free(marky_Scorer* scorer) {
    delete scorer;
}
