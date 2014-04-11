#ifndef MARKY_MARKY_C_H
#define MARKY_MARKY_C_H

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

#include <stddef.h>
#include <stdint.h>
//#include "build-config.h"

#define MARKY_SUCCESS 0
#define MARKY_FAILURE 1

/* This provides a C API to Marky. */
extern "C" {
    struct marky_Backend;
    struct marky_Backend_Cacheable;
    struct marky_Selector;
    struct marky_Scorer;
    struct marky_Marky;

    /* A list containing n=words_count words. */
    typedef struct marky_words {
        /* An array of C strings, or NULL if none/empty. Each entry is allocated
         * with malloc() and freed with free(). */
        char** words;
        /* The number of words in 'words', or 0 if none/empty. */
        size_t words_count;
    } marky_words_t;

    /* Returns a new Marky instance using the provided non-NULL components.
     * If any part of the instance creation fails, returns a NULL pointer.
     *
     * If the returned instance is non-NULL, it must later be freed with
     * marky_free(). The given components (Backend, Selector, Scorer) may be
     * safely freed immediately after a Marky instance has been created
     * successfully. If the creation fails, they must be freed manually. */
    marky_Marky* marky_new(marky_Backend* backend, marky_Selector* selector,
            marky_Scorer* scorer, size_t look_size);

    /* Deletes the provided Marky instance. Passing NULL is a (safe) no-op. */
    void marky_free(marky_Marky* marky);

    /* Adds the line (and its inter-word links) to the dataset.
     * Returns MARKY_FAILURE in the event of some error. */
    int marky_insert(marky_Marky* marky, const marky_words_t* line);

    /* Produces a malloc()ed list of strings from the search word, or from a
     * random word if the search word is unspecified. 'length_limit_words' and
     * 'length_limit_chars' each allow setting APPROXIMATE limits on the length
     * of the result, or no limit if zero.
     *
     * Produces a NULL line_out if the search word wasn't found (if applicable).
     * Returns MARKY_FAILURE in the event of some other error.
     * If MARKY_SUCCESS is returned and a non-NULL line_out is produced,
     * line_out must be freed by calling marky_words_free(). */
    int marky_produce(marky_Marky* marky,
            marky_words_t** line_out, const marky_words_t* search = NULL,
            size_t length_limit_words = 0, size_t length_limit_chars = 0);

    /* Tells the underlying backend to clean up any stale links it may have
     * lying around. This may be called periodically to free up resources.
     * Returns MARKY_FAILURE in the event of some error. */
    int marky_prune_backend(marky_Marky* marky);

    /* --- */

    /* Returns a new Backend instance. If any part of the instance creation
     * fails, returns a NULL pointer.
     *
     * If the returned instance is non-NULL, it must later be freed with
     * marky_backend_free(). See backend-*.h for more info about Backend types. */
    marky_Backend* marky_backend_new_map(void);
#ifdef BUILD_BACKEND_SQLITE
    marky_Backend* marky_backend_new_sqlite_direct(char* db_file_path);
    marky_Backend_Cacheable* marky_backend_new_sqlite_cacheable(char* db_file_path);
    marky_Backend* marky_backend_cache(marky_Backend_Cacheable* backend);
#endif

    /* Deletes the provided Backend instance. Passing NULL is a (safe) no-op. */
    void marky_backend_free(marky_Backend* backend);

    /* --- */

    /* Returns a new Selector instance. If any part of the instance creation
     * fails, returns a NULL pointer.
     *
     * If the returned instance is non-NULL, it must later be freed with
     * marky_selector_free(). See selector.h for more info about Selector types. */
    marky_Selector* marky_selector_new_best_always(void);
    marky_Selector* marky_selector_new_random(void);
    marky_Selector* marky_selector_new_best_weighted(uint8_t weight_factor = 128);

    /* Deletes the provided Selector instance. Passing NULL is a (safe) no-op. */
    void marky_selector_free(marky_Selector* selector);

    /* --- */

    /* Returns a new Scorer instance. If any part of the instance creation
     * fails, returns a NULL pointer.
     *
     * If the returned instance is non-NULL, it must later be freed with
     * marky_scorer_free(). See scorer.h for more info about Scorer types. */
    marky_Scorer* marky_scorer_new_no_adj(void);
    marky_Scorer* marky_scorer_new_word_adj(size_t score_decrement_words);
    marky_Scorer* marky_scorer_new_time_adj(size_t score_decrement_seconds);

    /* Deletes the provided Scorer instance. Passing NULL is a (safe) no-op. */
    void marky_scorer_free(marky_Scorer* scorer);

    /* --- */

    /* Returns a new words instance. If any part of the instance creation
     * fails, returns a NULL pointer.
     *
     * The returned instance will have with 'words_count' word entries, which
     * have all been initialized to NULL. The instance's word entries may then
     * be populated with malloc()ed C strings.
     *
     * If the returned instance is non-NULL, it must later be freed with
     * marky_words_free(). */
    marky_words_t* marky_words_new(size_t words_count);

    /* Deletes the provided line and all its entries, if any. Passing NULL is a
     * (safe) no-op. */
    void marky_words_free(marky_words_t* line);
}

#endif
