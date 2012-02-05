/*
  marky - A Markov chain generator.
  Copyright (C) 2011-2012  Nicholas Parker

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

#define STATE_TABLE "marky_state"
#define STATE_COL_KEY "key"
#define STATE_COL_VALUE "value"

#define STATE_KEY_TIME "time"
#define STATE_KEY_LINK "link"

#define LINKS_TABLE "marky_links"
#define LINKS_COL_PREV "prev"
#define LINKS_COL_NEXT "next"
#define LINKS_COL_SCORE "score"
#define LINKS_COL_TIME "time"
#define LINKS_COL_LINK "link"

#define LINKS_INDEX_PREV "prev_index"
#define LINKS_INDEX_NEXT "next_index"

#define UNSAFE_PRAGMA_OPTIMIZATIONS \
	"PRAGMA synchronous = OFF;" \
	"PRAGMA journal_mode = MEMORY"

/* Notes:
   state table: Don't worry about indexing: small table + not often accessed
   links table: Could be split into two tables with shared primary linkid, but I doubt that'd help performance */
#define QUERY_CREATE_TABLES \
	"BEGIN TRANSACTION;" \
	"CREATE TABLE IF NOT EXISTS " STATE_TABLE " (" \
	STATE_COL_KEY " TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE, " \
	STATE_COL_VALUE " INTEGER NOT NULL); " \
\
	"CREATE TABLE IF NOT EXISTS " LINKS_TABLE " (" \
	LINKS_COL_PREV " TEXT, " \
	LINKS_COL_NEXT " TEXT, " \
	LINKS_COL_SCORE " INTEGER NOT NULL, " \
	LINKS_COL_TIME " INTEGER NOT NULL, " \
	LINKS_COL_LINK " INTEGER NOT NULL, " \
	"PRIMARY KEY (" LINKS_COL_PREV ", " LINKS_COL_NEXT ") ON CONFLICT REPLACE); " \
\
	"CREATE INDEX IF NOT EXISTS " LINKS_INDEX_PREV " ON " LINKS_TABLE " (" LINKS_COL_PREV ");" \
	"CREATE INDEX IF NOT EXISTS " LINKS_INDEX_NEXT " ON " LINKS_TABLE " (" LINKS_COL_NEXT ");" \
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
	"SELECT " LINKS_COL_PREV ", " LINKS_COL_NEXT ", " \
	LINKS_COL_TIME ", " LINKS_COL_LINK ", " LINKS_COL_SCORE \
	" FROM " LINKS_TABLE " ORDER BY RANDOM() LIMIT 1"

#define QUERY_GET_PREVS \
	"SELECT " LINKS_COL_NEXT ", " LINKS_COL_TIME ", " LINKS_COL_LINK ", " \
	LINKS_COL_SCORE	" FROM " LINKS_TABLE " WHERE " LINKS_COL_PREV "=?1"
#define QUERY_GET_NEXTS \
	"SELECT " LINKS_COL_PREV ", " LINKS_COL_TIME ", " LINKS_COL_LINK ", " \
	LINKS_COL_SCORE " FROM " LINKS_TABLE " WHERE " LINKS_COL_NEXT "=?1"

#define QUERY_GET_LINK \
	"SELECT " LINKS_COL_TIME ", " LINKS_COL_LINK ", " LINKS_COL_SCORE \
	" FROM " LINKS_TABLE " WHERE " LINKS_COL_PREV "=?1 AND " LINKS_COL_NEXT "=?2"

#define QUERY_UPDATE_LINK \
	"UPDATE " LINKS_TABLE " SET " LINKS_COL_SCORE "=?1, " \
	LINKS_COL_TIME "=?2, " LINKS_COL_LINK "=?3 WHERE " \
	LINKS_COL_PREV "=?4 AND " LINKS_COL_NEXT "=?5"
//note the ON CONFLICT REPLACE above:
#define QUERY_INSERT_LINK \
	"INSERT INTO " LINKS_TABLE " (" \
	LINKS_COL_PREV "," LINKS_COL_NEXT "," LINKS_COL_SCORE "," \
	LINKS_COL_TIME "," LINKS_COL_LINK ") VALUES (?1,?2,?3,?4,?5)"

#define QUERY_GET_ALL \
	"SELECT " LINKS_COL_PREV ", " LINKS_COL_NEXT ", " \
	LINKS_COL_TIME ", " LINKS_COL_LINK ", " LINKS_COL_SCORE \
	" FROM " LINKS_TABLE

#define QUERY_DELETE_LINK \
	"DELETE FROM " LINKS_TABLE " WHERE " \
	LINKS_COL_PREV "=?1 AND " LINKS_COL_NEXT "=?2"


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
		int ret = sqlite3_bind_text(query, index, val.c_str(), val.size(), SQLITE_STATIC);
		if (ret != SQLITE_OK) {
			ERROR("Error when binding %d/'%s': %d", index, val.c_str(), ret);
			return false;
		}
		return true;
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
		DEBUG_RAWDIR(cmd);
	}

	template <typename T>
	void set_state(sqlite3* db, const char* key, T val) {
		sqlite3_stmt* response = NULL;
		if (prepare(db, QUERY_SET_STATE, response) &&
				bind_str(response, 1, key) &&
				bind_int64(response, 2, val)) {
			int step = sqlite3_step(response);
			if (step != SQLITE_DONE) {
				ERROR("Error when parsing response to '%s': %d\%s",
						QUERY_SET_STATE,
						step, sqlite3_errmsg(db));
			}
		}
		sqlite3_finalize(response);
	}

	/* If 'key' is found, val is updated and true is returned.
	 * If 'key' is not found, val is left untouched and true is returned.
	 * If there's an error, false is returned. */
	template <typename T>
	bool get_state(sqlite3* db, const char* key, T& val) {
		sqlite3_stmt* response = NULL;
		if (!prepare(db, QUERY_GET_STATE, response) ||
				!bind_str(response, 1, key)) {
			sqlite3_finalize(response);
			return false;
		}

		bool ok = true;
		int step = sqlite3_step(response);
		switch (step) {
		case SQLITE_ROW:/* row found, parse */
			val = sqlite3_column_int64(response, 0);
			break;
		case SQLITE_DONE:/* nothing found, do nothing */
			break;
		default:
			ok = false;
			ERROR("Error when parsing response to '%s': %d/%s",
					QUERY_GET_STATE, step, sqlite3_errmsg(db));
		}

		sqlite3_finalize(response);
		return ok;
	}
}

marky::Backend_SQLite::Backend_SQLite(const std::string& db_file_path)
	: path(db_file_path), db(NULL), state_changed(false) { }

marky::Backend_SQLite::~Backend_SQLite() {
	if (db != NULL) {
		if ((bool)state_ && state_changed) {
			/* update db state */
			set_state(db, STATE_KEY_TIME, state_->time);
			set_state(db, STATE_KEY_LINK, state_->link);
		}

		int ret = sqlite3_close(db);
		if (ret != SQLITE_OK) {
			/* possible memory loss? */
			ERROR("Failed to close sqlite db at %s!: %d/%s",
					path.c_str(), ret, sqlite3_errmsg(db));
		}
	}
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

	if (config::debug_enabled) {
		sqlite3_trace(db, trace_callback, NULL);
	}

	if (!exec(db, UNSAFE_PRAGMA_OPTIMIZATIONS)) {
		return false;
	}
	if (!exec(db, QUERY_CREATE_TABLES)) {
		return false;
	}

	/* init state to defaults, then update with db state: */
	state_.reset(new _state_t(time(NULL), 0));
	return get_state(db, STATE_KEY_TIME, state_->time) &&
		get_state(db, STATE_KEY_LINK, state_->link);
}

// IBACKEND STUFF (when used directly, PROBABLY SLOW)

bool marky::Backend_SQLite::get_random(scorer_t /*scorer*/, link_t& random) {
	sqlite3_stmt* response = NULL;
	if (!prepare(db, QUERY_GET_RANDOM, response)) {
		sqlite3_finalize(response);
		return false;
	}

	bool ok = true;
	int step = sqlite3_step(response);
	switch (step) {
	case SQLITE_ROW:/* row found, parse */
		/* this might produce a record with a zero score after scorer adjustment, oh well. */
		random.reset(new Link((const char*)sqlite3_column_text(response, 0),
						(const char*)sqlite3_column_text(response, 1),
						sqlite3_column_int64(response, 2),
						sqlite3_column_int64(response, 3),
						sqlite3_column_int64(response, 4)));
		break;
	case SQLITE_DONE:/* nothing found, do nothing */
		random.reset();
		break;
	default:
		ok = false;
		ERROR("Error when parsing response to '%s': %d/%s",
				QUERY_GET_RANDOM, step, sqlite3_errmsg(db));
	}

	sqlite3_finalize(response);
	return ok;
}

bool marky::Backend_SQLite::get_prev(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& prev) {
	sqlite3_stmt* response = NULL;
	if (!prepare(db, QUERY_GET_PREVS, response) ||
			!bind_str(response, 1, word)) {
		sqlite3_finalize(response);
		return false;
	}

	bool ok = true;
	links_t links(new _links_t);
	for (;;) {
		int step = sqlite3_step(response);
		/* use if instead of switch to easily break the for loop */
		if (step == SQLITE_DONE) {
			break;
		} else if (step == SQLITE_ROW) {
			link_t link(new Link(word, (const char*)sqlite3_column_text(response, 0),
							sqlite3_column_int64(response, 1),
							sqlite3_column_int64(response, 2),
							sqlite3_column_int64(response, 3)));
			links->push_back(link);
		} else {
			ok = false;
			ERROR("Error when parsing response to '%s': %d/%s",
					QUERY_GET_PREVS, step, sqlite3_errmsg(db));
			break;
		}
	}
	sqlite3_finalize(response);

	if (links->empty()) {
		prev.reset();
	} else {
		prev = selector(links, scorer, state_);
	}
	return ok;
}
bool marky::Backend_SQLite::get_next(selector_t selector, scorer_t scorer,
		const word_t& word, link_t& next) {
	sqlite3_stmt* response = NULL;
	if (!prepare(db, QUERY_GET_NEXTS, response) ||
			!bind_str(response, 1, word)) {
		sqlite3_finalize(response);
		return false;
	}

	bool ok = true;
	links_t links(new _links_t);
	for (;;) {
		int step = sqlite3_step(response);
		/* use if instead of switch to easily break the for loop */
		if (step == SQLITE_DONE) {
			break;
		} else if (step == SQLITE_ROW) {
			link_t link(new Link((const char*)sqlite3_column_text(response, 0), word,
							sqlite3_column_int64(response, 1),
							sqlite3_column_int64(response, 2),
							sqlite3_column_int64(response, 3)));
			links->push_back(link);
		} else {
			ok = false;
			ERROR("Error when parsing response to '%s': %d/%s",
					QUERY_GET_NEXTS, step, sqlite3_errmsg(db));
			break;
		}
	}
	sqlite3_finalize(response);

	if (links->empty()) {
		next.reset();
	} else {
		next = selector(links, scorer, state_);
	}
	return ok;
}

bool marky::Backend_SQLite::increment_link(scorer_t scorer,
		const word_t& first, const word_t& second) {
	sqlite3_stmt* get_response = NULL;
	if (!prepare(db, QUERY_GET_LINK, get_response) ||
			!bind_str(get_response, 1, first) ||
			!bind_str(get_response, 2, second)) {
		sqlite3_finalize(get_response);
		return false;
	}

	state_changed = true;

	/* update time BEFORE link is added */
	time_t now = time(NULL);
	state_->time = now;

	bool ok = true;
	int get_step = sqlite3_step(get_response);
	switch (get_step) {
	case SQLITE_ROW:/* entry found, score/update */
		{
			Link link(first, second,
					sqlite3_column_int64(get_response, 0),
					sqlite3_column_int64(get_response, 1),
					sqlite3_column_int64(get_response, 2));

			sqlite3_stmt* update_response = NULL;
			if (!prepare(db, QUERY_UPDATE_LINK, update_response) ||
					!bind_int64(update_response, 1, link.increment(scorer, state_)) ||
					!bind_int64(update_response, 2, state_->time) ||
					!bind_int64(update_response, 3, state_->link) ||
					!bind_str(update_response, 4, first) ||
					!bind_str(update_response, 5, second)) {
				ok = false;
			} else {
				int update_step = sqlite3_step(update_response);
				if (update_step != SQLITE_DONE) {
					ERROR("Error when parsing response to '%s': %d/%s",
							QUERY_UPDATE_LINK, update_step, sqlite3_errmsg(db));
				}
			}
			sqlite3_finalize(update_response);
		}
		break;
	case SQLITE_DONE:/* nothing found, insert new */
		{
			Link link(first, second, state_->time, state_->link);
			sqlite3_stmt* insert_response = NULL;
			if (!prepare(db, QUERY_INSERT_LINK, insert_response) ||
					!bind_str(insert_response, 1, first) ||
					!bind_str(insert_response, 2, second) ||
					!bind_int64(insert_response, 3, link.score(scorer, state_)) ||
					!bind_int64(insert_response, 4, state_->time) ||
					!bind_int64(insert_response, 5, state_->link)) {
				ok = false;
			} else {
				int insert_step = sqlite3_step(insert_response);
				if (insert_step != SQLITE_DONE) {
					ERROR("Error when parsing response to '%s': %d/%s",
							QUERY_INSERT_LINK, insert_step, sqlite3_errmsg(db));
				}
			}
			sqlite3_finalize(insert_response);
		}
		break;
	default:
		ok = false;
		ERROR("Error when parsing response to '%s': %d/%s",
				QUERY_GET_LINK, get_step, sqlite3_errmsg(db));
	}
	sqlite3_finalize(get_response);

	/* increment link count AFTER link is added (first link gets id 0) */
	++state_->link;

	return ok;
}

bool marky::Backend_SQLite::prune(scorer_t scorer) {
	bool ok = true;
	_links_t delme;
	{
		sqlite3_stmt* response = NULL;
		if (!prepare(db, QUERY_GET_ALL, response)) {
			return false;
		}

		for (;;) {
			int step = sqlite3_step(response);
			/* use if instead of switch to easily break the for loop */
			if (step == SQLITE_DONE) {
				break;
			} else if (step == SQLITE_ROW) {
				link_t link(new Link((const char*)sqlite3_column_text(response, 0),
								(const char*)sqlite3_column_text(response, 1),
								sqlite3_column_int64(response, 2),
								sqlite3_column_int64(response, 3),
								sqlite3_column_int64(response, 4)));
				if (link->score(scorer, state_) == 0) {
					/* zero score; prune */
					delme.push_back(link);
				}
			} else {
				ok = false;
				ERROR("Error when parsing response to '%s': %d/%s",
						QUERY_GET_ALL, step, sqlite3_errmsg(db));
				break;
			}
		}
		sqlite3_finalize(response);
	}

	if (!ok) {
		return false;
	}
	DEBUG_DIR("%lu to prune", delme.size());
	if (delme.empty()) {
		return true;/* nothing to prune! */
	}

	if (!exec(db, QUERY_BEGIN_TRANSACTION)) {
		ok = false;
	}

	/* don't return false if !ok; really want to close the transaction */

	{
		/* delete links in delme */
		sqlite3_stmt* response = NULL;
		if (!prepare(db, QUERY_DELETE_LINK, response)) {
			ok = false;
		} else {
			const _links_t::const_iterator end = delme.end();
			for (_links_t::const_iterator iter = delme.begin();
				 iter != end; ++iter) {
				if (!bind_str(response, 1, (*iter)->prev) ||
						!bind_str(response, 2, (*iter)->next)) {
					ok = false;
				}
				int step = sqlite3_step(response);
				if (step != SQLITE_DONE) {
					ERROR("Error when parsing response to '%s': %d\%s",
							QUERY_DELETE_LINK, step, sqlite3_errmsg(db));
					ok = false;
				}
				sqlite3_reset(response);
				if (!ok) {
					break;
				}
			}
		}
		sqlite3_finalize(response);
	}

	if (!exec(db, QUERY_END_TRANSACTION)) {
		ok = false;
	}

	return ok;
}

// ICACHEABLE STUFF (when wrapped in cache)

marky::state_t marky::Backend_SQLite::state() {
	if (!(bool)state_) {
		ERROR_DIR("get_state() called against uninitialized Backend_SQLite!");
		return state_t();
	}
	state_t state_cpy(new _state_t(*state_));
	return state_cpy;
}

bool marky::Backend_SQLite::get_prevs(const word_t& word, links_t& out) {
	sqlite3_stmt* response = NULL;
	if (!prepare(db, QUERY_GET_PREVS, response) ||
			!bind_str(response, 1, word)) {
		sqlite3_finalize(response);
		return false;
	}

	bool ok = true;
	for (;;) {
		int step = sqlite3_step(response);
		/* use if instead of switch to easily break the for loop */
		if (step == SQLITE_DONE) {
			break;
		} else if (step == SQLITE_ROW) {
			link_t link(new Link(word, (const char*)sqlite3_column_text(response, 0),
							sqlite3_column_int64(response, 1),
							sqlite3_column_int64(response, 2),
							sqlite3_column_int64(response, 3)));
			out->push_back(link);
		} else {
			ok = false;
			ERROR("Error when parsing response to '%s': %d/%s",
					QUERY_GET_PREVS, step, sqlite3_errmsg(db));
			break;
		}
	}
	sqlite3_finalize(response);

	return ok;
}

bool marky::Backend_SQLite::get_nexts(const word_t& word, links_t& out) {
	sqlite3_stmt* response = NULL;
	if (!prepare(db, QUERY_GET_NEXTS, response) ||
			!bind_str(response, 1, word)) {
		sqlite3_finalize(response);
		return false;
	}

	bool ok = true;
	for (;;) {
		int step = sqlite3_step(response);
		/* use if instead of switch to easily break the for loop */
		if (step == SQLITE_DONE) {
			break;
		} else if (step == SQLITE_ROW) {
			link_t link(new Link((const char*)sqlite3_column_text(response, 0), word,
							sqlite3_column_int64(response, 1),
							sqlite3_column_int64(response, 2),
							sqlite3_column_int64(response, 3)));
			out->push_back(link);
		} else {
			ok = false;
			ERROR("Error when parsing response to '%s': %d/%s",
					QUERY_GET_NEXTS, step, sqlite3_errmsg(db));
			break;
		}
	}
	sqlite3_finalize(response);

	return ok;
}

bool marky::Backend_SQLite::get_link(const word_t& first, const word_t& second,
		link_t& out) {
	sqlite3_stmt* get_response = NULL;
	if (!prepare(db, QUERY_GET_LINK, get_response) ||
			!bind_str(get_response, 1, first) ||
			!bind_str(get_response, 2, second)) {
		sqlite3_finalize(get_response);
		return false;
	}

	bool ok = true;
	int get_step = sqlite3_step(get_response);
	switch (get_step) {
	case SQLITE_ROW:/* entry found, produce it */
		out.reset(new Link(first, second,
						sqlite3_column_int64(get_response, 0),
						sqlite3_column_int64(get_response, 1),
						sqlite3_column_int64(get_response, 2)));
		break;
	case SQLITE_DONE:/* nothing found, produce null */
		out.reset();
		break;
	default:
		ok = false;
		ERROR("Error when parsing response to '%s': %d/%s",
				QUERY_GET_LINK, get_step, sqlite3_errmsg(db));
	}
	sqlite3_finalize(get_response);
	return ok;
}

/*#include <sys/time.h>*/

bool marky::Backend_SQLite::flush(const links_t& links, const state_t& state) {
	/* update the sqlite state */
	state_changed = true;
	state_ = state;

	if (links->empty()) {
		/* nothing left to do! */
		return true;
	}

	bool ok = true;

	/*struct timeval ta, tb;
	  gettimeofday(&ta, NULL);*/

	if (!exec(db, QUERY_BEGIN_TRANSACTION)) {
		ok = false;
	}

	/* don't return false if !ok; really want to close the transaction */

	{
		/* insert links */
		sqlite3_stmt* response = NULL;
		if (!prepare(db, QUERY_INSERT_LINK, response)) {
			ok = false;
		} else {
			const _links_t::const_iterator end = links->end();
			for (_links_t::const_iterator iter = links->begin();
				 iter != end; ++iter) {
				const Link& link = **iter;
				if (!bind_str(response, 1, link.prev) ||
						!bind_str(response, 2, link.next) ||
						!bind_int64(response, 3, link.cur_score()) ||
						!bind_int64(response, 4, link.cur_state().time) ||
						!bind_int64(response, 5, link.cur_state().link)) {
					ok = false;
				}

				int step = sqlite3_step(response);
				if (step != SQLITE_DONE) {
					ok = false;
					ERROR("Error when flushing entry with '%s': %d/%s",
							QUERY_INSERT_LINK, step, sqlite3_errmsg(db));
				}
				sqlite3_clear_bindings(response);
				sqlite3_reset(response);
				if (!ok) {
					break;
				}
			}
		}
		sqlite3_finalize(response);
	}

	if (!exec(db, QUERY_END_TRANSACTION)) {
		ok = false;
	}

	/*
	gettimeofday(&tb, NULL);
	int64_t us = tb.tv_usec - ta.tv_usec;
	us += (tb.tv_sec-ta.tv_sec)*1000000;
	ERROR("flush of %lu took %ld ms -> %.02f row/s", links->size(), us/1000, links->size()/sec);
	*/
	return ok;
}
