#ifndef MARKY_BACKEND_SQLITE_H
#define MARKY_BACKEND_SQLITE_H

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

struct sqlite3;
struct sqlite3_stmt;

namespace marky {
    /* A backend which uses a sqlite3 database for storing persistent state. */
    class Backend_SQLite : public ICacheable {
    public:
        /* Returns a SQLite backend, or an empty ptr if there was an error
         * when creating it. */
        static cacheable_t create_cacheable(const std::string& db_file_path);
        static backend_t create_backend(const std::string& db_file_path);

        virtual ~Backend_SQLite();

        /* for IBackend: */
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

        /* for ICacheable: */
        bool get_prevs(const words_t& words, snippet_ptr_set_t& out);
        bool get_nexts(const words_t& words, snippet_ptr_set_t& out);
        bool get_snippets(const words_to_counts::map_t& windows,
                words_to_snippet_t& out);

        bool flush(const State& state, scorer_t scorer,
                const snippet_ptr_set_t& links);

    private:
        Backend_SQLite(const std::string& db_file_path);
        bool init();

        bool update_snippets_impl(const State& state, scorer_t scorer,
                snippet_ptr_set_t& snippets);
        bool insert_snippets_impl(const snippet_ptr_set_t& snippets,
                bool allow_updates);
        bool get_snippets_impl(const words_to_counts::map_t& windows,
                words_to_snippet_t& out);

        sqlite3_stmt *stmt_set_state, *stmt_get_state;
        sqlite3_stmt *stmt_get_random, *stmt_get_prevs, *stmt_get_nexts;
        sqlite3_stmt *stmt_update_snippet, *stmt_upsert_snippet, *stmt_insert_snippet;
        sqlite3_stmt *stmt_insert_next, *stmt_insert_prev;
        sqlite3_stmt *stmt_get_all;
        sqlite3_stmt *stmt_delete_snippet;

        const std::string path;
        sqlite3* db;
        bool state_changed;
    };
}

#endif
