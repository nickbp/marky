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

#include <string.h>//strlen
#include <sqlite3.h>

#include "backend-sqlite.h"
#include "backend-map.h"
#include "config.h"
#include "string-pack.h"

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

#define STATE_TABLE "marky_state"
#define STATE_COL_KEY "key"
#define STATE_COL_VALUE "value"

#define STATE_KEY_TIME "time"
#define STATE_KEY_COUNT "count"

#define SNIPPET_TABLE "marky_snippet"
#define SNIPPETS_COL_SNIPPET_ID "snippet_id"
#define SNIPPETS_COL_WORDS "words"
#define SNIPPETS_COL_SCORE "score"
#define SNIPPETS_COL_TIME "time"
#define SNIPPETS_COL_COUNT "count"

#define NEXTS_TABLE "marky_nexts"
#define PREVS_TABLE "marky_prevs"
#define SNIPPETS_COL_SEARCH "search"

#define SNIPPET_WORDS_INDEX "marky_snippet_words_index"
#define PREVS_SEARCH_INDEX "marky_prevs_search_index"
#define NEXTS_SEARCH_INDEX "marky_nexts_search_index"

#define PRAGMA_ENABLE_FOREIGN_KEYS \
    "PRAGMA foreign_keys = ON"

#define UNSAFE_PRAGMA_OPTIMIZATIONS \
    "PRAGMA synchronous = OFF;" \
    "PRAGMA journal_mode = MEMORY"

/* Notes:
   state table: Don't worry about indexing: small table + not often accessed
   snippets table: Could be split into two tables with shared primary snippetid, but I doubt that'd help performance */
#define QUERY_CREATE_TABLES \
    "BEGIN TRANSACTION;" \
    "CREATE TABLE IF NOT EXISTS " STATE_TABLE " (" \
    STATE_COL_KEY " TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE, " \
    STATE_COL_VALUE " INTEGER NOT NULL); " \
\
    "CREATE TABLE IF NOT EXISTS " SNIPPET_TABLE " (" \
    SNIPPETS_COL_SNIPPET_ID " INTEGER NOT NULL PRIMARY KEY ON CONFLICT REPLACE, " \
    SNIPPETS_COL_WORDS " TEXT NOT NULL UNIQUE, " \
    SNIPPETS_COL_SCORE " INTEGER NOT NULL, " \
    SNIPPETS_COL_TIME " INTEGER NOT NULL, " \
    SNIPPETS_COL_COUNT " INTEGER NOT NULL); " \
    "CREATE UNIQUE INDEX IF NOT EXISTS " SNIPPET_WORDS_INDEX " ON " SNIPPET_TABLE " (" SNIPPETS_COL_WORDS "); " \
\
    "CREATE TABLE IF NOT EXISTS " PREVS_TABLE " (" \
    SNIPPETS_COL_SNIPPET_ID " INTEGER NOT NULL PRIMARY KEY ON CONFLICT REPLACE, " \
    SNIPPETS_COL_SEARCH " TEXT NOT NULL, " \
    "FOREIGN KEY (" SNIPPETS_COL_SNIPPET_ID ") REFERENCES " SNIPPET_TABLE "(" SNIPPETS_COL_SNIPPET_ID ") ON DELETE CASCADE); " \
    "CREATE INDEX IF NOT EXISTS " PREVS_SEARCH_INDEX " ON " PREVS_TABLE " (" SNIPPETS_COL_SEARCH "); " \
\
    "CREATE TABLE IF NOT EXISTS " NEXTS_TABLE " (" \
    SNIPPETS_COL_SNIPPET_ID " INTEGER NOT NULL PRIMARY KEY ON CONFLICT REPLACE, " \
    SNIPPETS_COL_SEARCH " TEXT NOT NULL, " \
    "FOREIGN KEY (" SNIPPETS_COL_SNIPPET_ID ") REFERENCES " SNIPPET_TABLE "(" SNIPPETS_COL_SNIPPET_ID ") ON DELETE CASCADE); " \
    "CREATE INDEX IF NOT EXISTS " NEXTS_SEARCH_INDEX " ON " NEXTS_TABLE " (" SNIPPETS_COL_SEARCH "); " \
\
    "COMMIT TRANSACTION"

#define QUERY_BEGIN_TRANSACTION "BEGIN TRANSACTION"
#define QUERY_END_TRANSACTION "COMMIT TRANSACTION"

#define QUERY_GET_STATE \
    "SELECT " STATE_COL_VALUE " FROM " STATE_TABLE " WHERE " STATE_COL_KEY "=?1"
//node the ON CONFLICT REPLACE above:
#define QUERY_SET_STATE \
    "INSERT INTO " STATE_TABLE " (" STATE_COL_KEY "," STATE_COL_VALUE ") VALUES (?1,?2)"

//TODO is this going to get one entry randomly, or sort things randomly and get one entry?
#define QUERY_GET_RANDOM \
    "SELECT " SNIPPETS_COL_WORDS " FROM " SNIPPET_TABLE " ORDER BY RANDOM() LIMIT 1"

#define QUERY_GET_PREVS \
    "SELECT " SNIPPETS_COL_WORDS ", " SNIPPETS_COL_TIME ", " SNIPPETS_COL_COUNT ", " \
    SNIPPETS_COL_SCORE " FROM " SNIPPET_TABLE " JOIN " PREVS_TABLE " ON " \
    SNIPPET_TABLE "." SNIPPETS_COL_SNIPPET_ID " = " PREVS_TABLE "." SNIPPETS_COL_SNIPPET_ID \
    " WHERE " PREVS_TABLE "." SNIPPETS_COL_SEARCH "=?1"
#define QUERY_GET_NEXTS \
    "SELECT " SNIPPETS_COL_WORDS ", " SNIPPETS_COL_TIME ", " SNIPPETS_COL_COUNT ", " \
    SNIPPETS_COL_SCORE " FROM " SNIPPET_TABLE " JOIN " NEXTS_TABLE " ON " \
    SNIPPET_TABLE "." SNIPPETS_COL_SNIPPET_ID " = " NEXTS_TABLE "." SNIPPETS_COL_SNIPPET_ID \
    " WHERE " NEXTS_TABLE "." SNIPPETS_COL_SEARCH "=?1"

#define QUERY_GET_SNIPPET \
    "SELECT " SNIPPETS_COL_TIME ", " SNIPPETS_COL_COUNT ", " SNIPPETS_COL_SCORE \
    " FROM " SNIPPET_TABLE " WHERE " SNIPPETS_COL_WORDS "=?1"
#define QUERY_GET_SNIPPETS_PREFIX \
    "SELECT " SNIPPETS_COL_WORDS ", " SNIPPETS_COL_TIME ", " SNIPPETS_COL_COUNT ", " \
    SNIPPETS_COL_SCORE " FROM " SNIPPET_TABLE " WHERE " SNIPPETS_COL_WORDS " IN "

#define QUERY_UPDATE_SNIPPET \
    "UPDATE " SNIPPET_TABLE " SET " SNIPPETS_COL_SCORE "=?1, " \
    SNIPPETS_COL_TIME "=?2, " SNIPPETS_COL_COUNT "=?3 WHERE "  \
    SNIPPETS_COL_WORDS "=?4"
//note the ON CONFLICT REPLACE above. rowid/snippet_id is created automatically:
#define QUERY_INSERT_SNIPPET \
    "INSERT INTO " SNIPPET_TABLE " (" \
    SNIPPETS_COL_WORDS "," SNIPPETS_COL_SCORE "," SNIPPETS_COL_TIME "," SNIPPETS_COL_COUNT \
    ") VALUES (?1,?2,?3,?4)"
//similar to INSERT_SNIPPET, except for when we may be updating an existing field.
//when a snippet is updated, the prevs/nexts are automatically deleted and need to be reinserted.
#define QUERY_UPSERT_SNIPPET \
    "INSERT OR REPLACE INTO " SNIPPET_TABLE " (" \
    SNIPPETS_COL_WORDS "," SNIPPETS_COL_SCORE "," SNIPPETS_COL_TIME "," SNIPPETS_COL_COUNT \
    ") VALUES (?1,?2,?3,?4)"
#define QUERY_INSERT_PREV \
    "INSERT INTO " PREVS_TABLE " (" \
    SNIPPETS_COL_SEARCH "," SNIPPETS_COL_SNIPPET_ID \
    ") VALUES (?1,?2)"
#define QUERY_INSERT_NEXT \
    "INSERT INTO " NEXTS_TABLE " (" \
    SNIPPETS_COL_SEARCH "," SNIPPETS_COL_SNIPPET_ID \
    ") VALUES (?1,?2)"

#define QUERY_GET_ALL \
    "SELECT " SNIPPETS_COL_WORDS ", " SNIPPETS_COL_TIME ", " \
    SNIPPETS_COL_COUNT ", " SNIPPETS_COL_SCORE \
    " FROM " SNIPPET_TABLE

#define QUERY_DELETE_SNIPPET \
    "DELETE FROM " SNIPPET_TABLE " WHERE " SNIPPETS_COL_WORDS "=?1"

namespace {
    inline bool exec(sqlite3* db, const char* cmd) {
        char* err = NULL;
        int ret = sqlite3_exec(db, cmd, NULL, NULL, &err);
        if (ret != SQLITE_OK) {
            ERROR("Error when executing '%s': %s", cmd, err);
            sqlite3_free(err);
            return false;
        }
        return true;
    }

    inline bool prepare(sqlite3* db, const char* cmd, sqlite3_stmt*& query) {
        /* include the null terminator in the length */
        int ret = sqlite3_prepare_v2(db, cmd, strlen(cmd)+1, &query, NULL);
        if (ret != SQLITE_OK) {
            ERROR("Error when preparing '%s': %d/%s",
                    cmd, ret, sqlite3_errmsg(db));
            return false;
        }
        return true;
    }

    inline bool bind_str(sqlite3_stmt* query, int index, const std::string& val) {
        int ret = sqlite3_bind_text(query, index, val.c_str(), val.size(), SQLITE_TRANSIENT);
        if (ret != SQLITE_OK) {
            ERROR("Error when binding %d/'%s': %d", index, val.c_str(), ret);
            return false;
        }
        return true;
    }

    inline bool bind_words(sqlite3_stmt* query, int index, const marky::words_t& words) {
        std::ostringstream oss;
        marky::pack(words, oss);
        return bind_str(query, index, oss.str());
    }

    inline bool bind_int64(sqlite3_stmt* query, int index, int64_t val) {
        int ret = sqlite3_bind_int64(query, index, val);
        if (ret != SQLITE_OK) {
            ERROR("Error when binding %d/'%ld': %d", index, val, ret);
            return false;
        }
        return true;
    }
}

/*static*/ marky::cacheable_t marky::Backend_SQLite::create_cacheable(const std::string& db_file_path) {
    Backend_SQLite* ret = new Backend_SQLite(db_file_path);
    if (!ret->init()) {
        delete ret;
        return cacheable_t();
    }
    return cacheable_t(ret);
}
/*static*/ marky::backend_t marky::Backend_SQLite::create_backend(const std::string& db_file_path) {
    Backend_SQLite* ret = new Backend_SQLite(db_file_path);
    if (!ret->init()) {
        delete ret;
        return backend_t();
    }
    return backend_t(ret);
}

namespace {
    void trace_callback(void*, const char* cmd) {
        DEBUG(cmd);
    }

    template <typename T>
    void set_state(sqlite3* db, sqlite3_stmt* stmt, const char* key, T val) {
        if (bind_str(stmt, 1, key) && bind_int64(stmt, 2, val)) {
            int step = sqlite3_step(stmt);
            if (step != SQLITE_DONE) {
                ERROR("Error when parsing response to '%s': %d\%s",
                        QUERY_SET_STATE,
                        step, sqlite3_errmsg(db));
            }
        }
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
    }

    /* If 'key' is found, val is updated and true is returned.
     * If 'key' is not found, val is left untouched and true is returned.
     * If there's an error, false is returned. */
    template <typename T>
    bool get_state(sqlite3* db, sqlite3_stmt* stmt, const char* key, T& val) {
        if (!bind_str(stmt, 1, key)) {
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            return false;
        }

        bool ok = true;
        int step = sqlite3_step(stmt);
        switch (step) {
            case SQLITE_ROW:/* row found, parse */
                val = sqlite3_column_int64(stmt, 0);
                break;
            case SQLITE_DONE:/* nothing found, do nothing */
                break;
            default:
                ok = false;
                ERROR("Error when parsing response to '%s': %d/%s",
                        QUERY_GET_STATE, step, sqlite3_errmsg(db));
                break;
        }

        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        return ok;
    }
}

marky::Backend_SQLite::Backend_SQLite(const std::string& db_file_path)
    : path(db_file_path), db(NULL), state_changed(false) {
}

marky::Backend_SQLite::~Backend_SQLite() {
    if (db != NULL) {
        sqlite3_finalize(stmt_set_state);
        sqlite3_finalize(stmt_get_state);
        sqlite3_finalize(stmt_get_random);
        sqlite3_finalize(stmt_get_prevs);
        sqlite3_finalize(stmt_get_nexts);
        sqlite3_finalize(stmt_update_snippet);
        sqlite3_finalize(stmt_upsert_snippet);
        sqlite3_finalize(stmt_insert_snippet);
        sqlite3_finalize(stmt_insert_next);
        sqlite3_finalize(stmt_insert_prev);
        sqlite3_finalize(stmt_get_all);
        sqlite3_finalize(stmt_delete_snippet);

        int ret = sqlite3_close(db);
        if (ret != SQLITE_OK) {
            /* possible memory loss? */
            ERROR("Failed to close sqlite db at %s!: %d/%s",
                    path.c_str(), ret, sqlite3_errmsg(db));
        }
    }
}

marky::State marky::Backend_SQLite::create_state() {
    /* init state to defaults, then (try to) update with db state: */
    State state(time(NULL), 0);
    get_state(db, stmt_get_state, STATE_KEY_TIME, state.time);
    get_state(db, stmt_get_state, STATE_KEY_COUNT, state.count);
    return state;
}

bool marky::Backend_SQLite::store_state(const State& state, scorer_t /*scorer*/) {
    if (db != NULL && state_changed) {
        /* update db state */
        set_state(db, stmt_set_state, STATE_KEY_TIME, state.time);
        set_state(db, stmt_set_state, STATE_KEY_COUNT, state.count);
    }
    return true;
}

bool marky::Backend_SQLite::init() {
    if (db != NULL) {
        ERROR("Already inited sqlite db at %s!", path.c_str());
        return false;
    }

    int ret = sqlite3_open(path.c_str(), &db);/* rw + create */
    if (ret != SQLITE_OK) {
        ERROR("Failed to open sqlite db at %s: %d/%s",
                path.c_str(), ret, sqlite3_errmsg(db));
        return false;
    }

#ifdef DEBUG_ENABLED
    sqlite3_trace(db, trace_callback, NULL);
#endif

    if (!exec(db, PRAGMA_ENABLE_FOREIGN_KEYS)) {
        ERROR("Unable to enable SQLite Foreign Key support. "
                "Please rebuild SQLite with SQLITE_OMIT_FORIGN_KEY and SQLITE_OMIT_TRIGGER turned OFF!");
        return false;
    }
    if (!exec(db, UNSAFE_PRAGMA_OPTIMIZATIONS)) {
        LOG("Failed to enable unsafe SQLite speed optimizations. Continuing anyway...");
    }
    if (!exec(db, QUERY_CREATE_TABLES)) {
        return false;
    }

    if (!prepare(db, QUERY_SET_STATE, stmt_set_state) ||
            !prepare(db, QUERY_GET_STATE, stmt_get_state) ||
            !prepare(db, QUERY_GET_RANDOM, stmt_get_random) ||
            !prepare(db, QUERY_GET_PREVS, stmt_get_prevs) ||
            !prepare(db, QUERY_GET_NEXTS, stmt_get_nexts) ||
            !prepare(db, QUERY_UPDATE_SNIPPET, stmt_update_snippet) ||
            !prepare(db, QUERY_UPSERT_SNIPPET, stmt_upsert_snippet) ||
            !prepare(db, QUERY_INSERT_SNIPPET, stmt_insert_snippet) ||
            !prepare(db, QUERY_INSERT_NEXT, stmt_insert_next) ||
            !prepare(db, QUERY_INSERT_PREV, stmt_insert_prev) ||
            !prepare(db, QUERY_GET_ALL, stmt_get_all) ||
            !prepare(db, QUERY_DELETE_SNIPPET, stmt_delete_snippet)) {
        ERROR("Unable to prepare SQLite statements.");
        return false;
    }
    return true;
}

// IBACKEND STUFF (when used directly, PROBABLY SLOW)

bool marky::Backend_SQLite::get_random(const State& /*state*/, scorer_t /*scorer*/,
        word_t& random) {
    bool ok = true;
    int step = sqlite3_step(stmt_get_random);
    random = IBackend::LINE_END;
    switch (step) {
    case SQLITE_ROW:/* row found, parse */
        {
            words_t words;
            unpack((const char*)sqlite3_column_text(stmt_get_random, 0), words);
            for (words_t::const_iterator iter = words.begin();
                 iter != words.end(); ++iter) {
                if (*iter != IBackend::LINE_END) {
                    random = *iter;
                }
            }
            break;
        }
    case SQLITE_DONE:/* nothing found, do nothing */
        break;
    default:
        ok = false;
        ERROR("Error when parsing response to '%s': %d/%s",
                QUERY_GET_RANDOM, step, sqlite3_errmsg(db));
        break;
    }

    sqlite3_clear_bindings(stmt_get_random);
    sqlite3_reset(stmt_get_random);
    return ok;
}

bool marky::Backend_SQLite::get_prev(const State& state, selector_t selector,
        scorer_t scorer, const words_t& search_words, word_t& prev) {
#ifdef READ_DEBUG_ENABLED
    DEBUG("get_prev(%s)", str(search_words).c_str());
#endif
    if (!bind_words(stmt_get_prevs, 1, search_words)) {
        sqlite3_clear_bindings(stmt_get_prevs);
        sqlite3_reset(stmt_get_prevs);
        return false;
    }

    bool ok = true;
    snippets_ptr_t snippets(new snippet_ptr_set_t);
    for (;;) {
        int step = sqlite3_step(stmt_get_prevs);
        bool done = false;
        switch (step) {
            case SQLITE_DONE:
                done = true;
                break;
            case SQLITE_ROW:
                {
                    words_t words;
                    unpack((const char*)sqlite3_column_text(stmt_get_prevs, 0), words);
                    snippet_t snippet(new Snippet(words,
                                    sqlite3_column_int64(stmt_get_prevs, 1),
                                    sqlite3_column_int64(stmt_get_prevs, 2),
                                    sqlite3_column_int64(stmt_get_prevs, 3)));
                    snippets->insert(snippet);
                    break;
                }
            default:
                ok = false;
                ERROR("Error when parsing response to '%s': %d/%s",
                        QUERY_GET_PREVS, step, sqlite3_errmsg(db));
                break;
        }
        if (!ok || done) {
            break;
        }
    }
    sqlite3_clear_bindings(stmt_get_prevs);
    sqlite3_reset(stmt_get_prevs);

    if (snippets->empty()) {
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
        const words_t& prev_snippet = selector(*snippets, scorer, state)->words;
#ifdef READ_DEBUG_ENABLED
        for (snippet_ptr_set_t::const_iterator siter = snippets->begin();
             siter != snippets->end(); ++siter) {
            DEBUG("  prevs%s = snippet(%s, %lu)", str(search_words).c_str(),
                    str((*siter)->words).c_str(), (*siter)->score(scorer, state));
        }
        DEBUG("    prev_snippet -> %s", str(prev_snippet).c_str());
#endif
        prev = prev_snippet.front();
    }
    return ok;
}

bool marky::Backend_SQLite::get_next(const State& state, selector_t selector,
        scorer_t scorer, const words_t& search_words, word_t& next) {
#ifdef READ_DEBUG_ENABLED
    DEBUG("get_next(%s)", str(search_words).c_str());
#endif
    if (!bind_words(stmt_get_nexts, 1, search_words)) {
        sqlite3_clear_bindings(stmt_get_nexts);
        sqlite3_reset(stmt_get_nexts);
        return false;
    }

    bool ok = true;
    snippets_ptr_t snippets(new snippet_ptr_set_t);
    for (;;) {
        int step = sqlite3_step(stmt_get_nexts);
        bool done = false;
        switch (step) {
            case SQLITE_DONE:
                done = true;
                break;
            case SQLITE_ROW:
                {
                    words_t words;
                    unpack((const char*)sqlite3_column_text(stmt_get_nexts, 0), words);
                    snippet_t snippet(new Snippet(words,
                                    sqlite3_column_int64(stmt_get_nexts, 1),
                                    sqlite3_column_int64(stmt_get_nexts, 2),
                                    sqlite3_column_int64(stmt_get_nexts, 3)));
                    snippets->insert(snippet);
                    break;
                }
            default:
                ok = false;
                ERROR("Error when parsing response to '%s': %d/%s",
                        QUERY_GET_NEXTS, step, sqlite3_errmsg(db));
                break;
        }
        if (!ok || done) {
            break;
        }
    }
    sqlite3_clear_bindings(stmt_get_nexts);
    sqlite3_reset(stmt_get_nexts);

    if (snippets->empty()) {
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
        const words_t& next_snippet = selector(*snippets, scorer, state)->words;
#ifdef READ_DEBUG_ENABLED
        for (snippet_ptr_set_t::const_iterator siter = snippets->begin();
             siter != snippets->end(); ++siter) {
            DEBUG("  nexts%s = snippet(%s, %lu)", str(search_words).c_str(),
                    str((*siter)->words).c_str(), (*siter)->score(scorer, state));
        }
        DEBUG("    next_snippet -> %s", str(next_snippet).c_str());
#endif
        next = next_snippet.back();
    }
    return ok;
}

bool marky::Backend_SQLite::update_snippets(const State& state, scorer_t scorer,
        const words_to_counts::map_t& line_windows) {
#ifdef WRITE_DEBUG_ENABLED
    DEBUG("update_score -> %lu windows", line_windows.size());
#endif

    words_to_snippet_t found_snippets;
    if (!get_snippets(line_windows, found_snippets)) {
        return false;
    }

    snippet_ptr_set_t snippets_to_update;
    snippet_ptr_set_t snippets_to_insert;
    for (words_to_counts::map_t::const_iterator window_iter = line_windows.begin();
         window_iter != line_windows.end(); ++window_iter) {
        words_to_snippet_t::const_iterator found_snippet =
            found_snippets.find(window_iter->first);
        if (found_snippet != found_snippets.end()) {
            /* entry found, re-score/update */
            found_snippet->second->increment(scorer, state, window_iter->second);
            snippets_to_update.insert(found_snippet->second);
        } else {
            /* nothing found, insert new */
            snippet_t new_snippet(new Snippet(window_iter->first,
                            state.time, state.count, window_iter->second));
            snippets_to_insert.insert(new_snippet);
        }
    }

    bool ok = true;
    if (!snippets_to_update.empty() || !snippets_to_insert.empty()) {
        state_changed = true;
        if (!snippets_to_update.empty() &&
                !update_snippets_impl(state, scorer, snippets_to_update)) {
            ok = false;
        }
        if (!snippets_to_insert.empty() &&
                !insert_snippets_impl(snippets_to_insert, true)) {
            ok = false;
        }
    }
    return ok;
}

bool marky::Backend_SQLite::prune(const State& state, scorer_t scorer) {
    bool ok = true;
    snippet_ptr_set_t delme;

    for (;;) {
        int step = sqlite3_step(stmt_get_all);
        bool done = false;
        switch (step) {
            case SQLITE_DONE:
                done = true;
                break;
            case SQLITE_ROW:
                {
                    words_t words;
                    unpack((const char*)sqlite3_column_text(stmt_get_all, 0), words);
                    snippet_t snippet(new Snippet(words,
                                    sqlite3_column_int64(stmt_get_all, 1),
                                    sqlite3_column_int64(stmt_get_all, 2),
                                    sqlite3_column_int64(stmt_get_all, 3)));
                    if (snippet->score(scorer, state) == 0) {
                        /* zero score; prune */
                        delme.insert(snippet);
                    }
                    break;
                }
            default:
                ok = false;
                ERROR("Error when parsing response to '%s': %d/%s",
                        QUERY_GET_ALL, step, sqlite3_errmsg(db));
                break;
        }
        if (!ok || done) {
            break;
        }
    }
    sqlite3_clear_bindings(stmt_get_all);
    sqlite3_reset(stmt_get_all);

    if (!ok) {
        return false;
    }
    DEBUG("%lu to prune", delme.size());
    if (delme.empty()) {
        return true;/* nothing to prune! */
    }

    if (!exec(db, QUERY_BEGIN_TRANSACTION)) {
        ok = false;
    }

    /* don't return false if !ok; really want to close the transaction */

    /* delete snippets in delme */
    for (snippet_ptr_set_t::const_iterator iter = delme.begin();
         iter != delme.end(); ++iter) {
        if (!bind_words(stmt_delete_snippet, 1, (*iter)->words)) {
            ok = false;
        }
        int step = sqlite3_step(stmt_delete_snippet);
        if (step != SQLITE_DONE) {
            ERROR("Error when parsing response to '%s': %d\%s",
                    QUERY_DELETE_SNIPPET, step, sqlite3_errmsg(db));
            ok = false;
        }
        if (!ok) {
            break;
        }
    }
    sqlite3_clear_bindings(stmt_delete_snippet);
    sqlite3_reset(stmt_delete_snippet);

    if (!exec(db, QUERY_END_TRANSACTION)) {
        ok = false;
    }

    return ok;
}

// ICACHEABLE STUFF (when wrapped in cache)

bool marky::Backend_SQLite::get_prevs(const words_t& words, snippet_ptr_set_t& out) {
    if (!bind_words(stmt_get_prevs, 1, words)) {
        sqlite3_clear_bindings(stmt_get_prevs);
        sqlite3_reset(stmt_get_prevs);
        return false;
    }

    bool ok = true;
    out.clear();
    for (;;) {
        int step = sqlite3_step(stmt_get_prevs);
        bool done = false;
        switch (step) {
            case SQLITE_DONE:
                done = true;
                break;
            case SQLITE_ROW:
                {
                    words_t words;
                    unpack((const char*)sqlite3_column_text(stmt_get_prevs, 0), words);
                    snippet_t snippet(new Snippet(words,
                                    sqlite3_column_int64(stmt_get_prevs, 1),
                                    sqlite3_column_int64(stmt_get_prevs, 2),
                                    sqlite3_column_int64(stmt_get_prevs, 3)));
                    out.insert(snippet);
                    break;
                }
            default:
                ok = false;
                ERROR("Error when parsing response to '%s': %d/%s",
                        QUERY_GET_PREVS, step, sqlite3_errmsg(db));
                break;
        }
        if (!ok || done) {
            break;
        }
    }
    sqlite3_clear_bindings(stmt_get_prevs);
    sqlite3_reset(stmt_get_prevs);

    return ok;
}

bool marky::Backend_SQLite::get_nexts(const words_t& words, snippet_ptr_set_t& out) {
    if (!bind_words(stmt_get_nexts, 1, words)) {
        sqlite3_clear_bindings(stmt_get_nexts);
        sqlite3_reset(stmt_get_nexts);
        return false;
    }

    bool ok = true;
    out.clear();
    for (;;) {
        int step = sqlite3_step(stmt_get_nexts);
        bool done = false;
        switch (step) {
            case SQLITE_DONE:
                done = true;
                break;
            case SQLITE_ROW:
                {
                    words_t words;
                    unpack((const char*)sqlite3_column_text(stmt_get_nexts, 0), words);
                    snippet_t snippet(new Snippet(words,
                                    sqlite3_column_int64(stmt_get_nexts, 1),
                                    sqlite3_column_int64(stmt_get_nexts, 2),
                                    sqlite3_column_int64(stmt_get_nexts, 3)));
                    out.insert(snippet);
                    break;
                }
            default:
                ok = false;
                ERROR("Error when parsing response to '%s': %d/%s",
                        QUERY_GET_NEXTS, step, sqlite3_errmsg(db));
                break;
        }
        if (!ok || done) {
            break;
        }
    }
    sqlite3_clear_bindings(stmt_get_nexts);
    sqlite3_reset(stmt_get_nexts);

    return ok;
}

bool marky::Backend_SQLite::get_snippets(const words_to_counts::map_t& windows,
        words_to_snippet_t& out) {
    out.clear();

    sqlite3_stmt* get_response = NULL;
    /* dynamically build a query with the correct number of params */
    std::ostringstream query;
    query << QUERY_GET_SNIPPETS_PREFIX << '(';
    for (size_t i = 1; i <= windows.size(); ) {
        query << '?' << i;
        if (++i <= windows.size()) {
            query << ',';
        }
    }
    query << ')';// result: WHERE x IN (?1,?2,?3,...)
    if (!prepare(db, query.str().c_str(), get_response)) {
        sqlite3_finalize(get_response);
        return false;
    }
    /* bind the query params */
    size_t cur_bind_id = 1;
    for (words_to_counts::map_t::const_iterator iter = windows.begin();
         iter != windows.end(); ++iter) {
        if (!bind_words(get_response, cur_bind_id++, iter->first)) {
            sqlite3_finalize(get_response);
            return false;
        }
    }

    bool ok = true;
    for (;;) {
        int step = sqlite3_step(get_response);
        bool done = false;
        switch (step) {
            case SQLITE_DONE:
                done = true;
                break;
            case SQLITE_ROW:
                {
                    words_t words;
                    unpack((const char*)sqlite3_column_text(get_response, 0), words);
                    out[words].reset(new Snippet(words,
                                    sqlite3_column_int64(get_response, 1),
                                    sqlite3_column_int64(get_response, 2),
                                    sqlite3_column_int64(get_response, 3)));
                    break;
                }
            default:
                ok = false;
                ERROR("Error when parsing response to '%s' for %lu entries: %d/%s",
                        query.str().c_str(), windows.size(), step, sqlite3_errmsg(db));
                break;
        }
        if (!ok || done) {
            break;
        }
    }
    sqlite3_finalize(get_response);
    return ok;
}

/*#include <sys/time.h>*/

bool marky::Backend_SQLite::flush(const State& state, scorer_t scorer, const snippet_ptr_set_t& snippets) {
    /* update the sqlite state */
    state_changed = true;

    if (snippets.empty()) {
        /* nothing left to do! */
        return true;
    }

    bool ok = true;

    /*struct timeval ta, tb;
      gettimeofday(&ta, NULL);*/

    if (!exec(db, QUERY_BEGIN_TRANSACTION)) {
        ok = false;
    }

    /* if !ok keep going: really want to close the transaction */

    /* insert or update snippets, depending on current presence */
    if (!insert_snippets_impl(snippets, true)) {
        ok = false;
    }

    if (!exec(db, QUERY_END_TRANSACTION)) {
        ok = false;
    }

    /*
    gettimeofday(&tb, NULL);
    int64_t us = tb.tv_usec - ta.tv_usec;
    us += (tb.tv_sec-ta.tv_sec)*1000000;
    ERROR("flush of %lu took %ld ms -> %.02f row/s", snippets.size(), us/1000, snippets.size()/sec);
    */
    return ok;
}

bool marky::Backend_SQLite::update_snippets_impl(const State& state,
        scorer_t scorer, snippet_ptr_set_t& snippets) {
    bool ok = true;
    for (snippet_ptr_set_t::const_iterator iter = snippets.begin();
         iter != snippets.end(); ++iter) {
        Snippet& snippet = **iter;

        /* update existing scores; use increment() since these snippets were not just created */
        if (!bind_int64(stmt_update_snippet, 1, snippet.score(scorer, state)) ||
                !bind_int64(stmt_update_snippet, 2, state.time) ||
                !bind_int64(stmt_update_snippet, 3, state.count) ||
                !bind_words(stmt_update_snippet, 4, snippet.words)) {
            ok = false;
        }

        int update_step = sqlite3_step(stmt_update_snippet);
        if (update_step != SQLITE_DONE) {
            ERROR("Error when parsing response to '%s': %d/%s",
                    QUERY_UPDATE_SNIPPET, update_step, sqlite3_errmsg(db));
        }

        sqlite3_clear_bindings(stmt_update_snippet);
        sqlite3_reset(stmt_update_snippet);
        if (!ok) {
            break;
        }
    }
    return ok;
}

bool marky::Backend_SQLite::insert_snippets_impl(const snippet_ptr_set_t& snippets,
        bool allow_updates) {
    bool ok = true;
    sqlite3_stmt* snippet_update_stmt = (allow_updates) ? stmt_upsert_snippet : stmt_insert_snippet;

    for (snippet_ptr_set_t::const_iterator iter = snippets.begin();
         iter != snippets.end(); ++iter) {
        const Snippet& snippet = **iter;
        //ERROR("INSERT: %s", snippet.str().c_str());

        /* snippets table */
        if (!bind_words(snippet_update_stmt, 1, snippet.words) ||
                !bind_int64(snippet_update_stmt, 2, snippet.cur_score()) ||
                !bind_int64(snippet_update_stmt, 3, snippet.cur_state().time) ||
                !bind_int64(snippet_update_stmt, 4, snippet.cur_state().count)) {
            ok = false;
        } else {
            int insert_step = sqlite3_step(snippet_update_stmt);
            if (insert_step != SQLITE_DONE) {
                ok = false;
                ERROR("Error when flushing entry with '%s': %d/%s [%s]",
                        QUERY_INSERT_SNIPPET, insert_step, sqlite3_errmsg(db),
                        snippet.str().c_str());
            }
        }
        sqlite3_clear_bindings(snippet_update_stmt);
        sqlite3_reset(snippet_update_stmt);
        sqlite3_int64 snippet_id = sqlite3_last_insert_rowid(db);

        /* nexts table */
        words_t words_subset = snippet.words;
        words_subset.pop_back();// all except back
        if (!bind_words(stmt_insert_next, 1, words_subset) ||
                !bind_int64(stmt_insert_next, 2, snippet_id)) {
            ok = false;
        } else {
            int insert_step = sqlite3_step(stmt_insert_next);
            if (insert_step != SQLITE_DONE) {
                ok = false;
                ERROR("Error when parsing response to '%s': %d/%s",
                        QUERY_INSERT_NEXT, insert_step, sqlite3_errmsg(db));
            }
        }
        sqlite3_clear_bindings(stmt_insert_next);
        sqlite3_reset(stmt_insert_next);

        /* prevs table */
        words_subset.push_back(snippet.words.back());
        words_subset.pop_front();// all except front (from all except back)
        if (!bind_words(stmt_insert_prev, 1, words_subset) ||
                !bind_int64(stmt_insert_prev, 2, snippet_id)) {
            ok = false;
        } else {
            int insert_step = sqlite3_step(stmt_insert_prev);
            if (insert_step != SQLITE_DONE) {
                ok = false;
                ERROR("Error when parsing response to '%s': %d/%s",
                        QUERY_INSERT_PREV, insert_step, sqlite3_errmsg(db));
            }
        }
        sqlite3_clear_bindings(stmt_insert_prev);
        sqlite3_reset(stmt_insert_prev);

        if (!ok) {
            break;
        }
    }

    return ok;
}
