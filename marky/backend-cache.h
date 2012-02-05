#ifndef MARKY_BACKEND_CACHE_H
#define MARKY_BACKEND_CACHE_H

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

#include <unordered_map>

#include "backend.h"

namespace marky {
	/* A cache wrapper around an ICacheable. */
	class Backend_Cache : public IBackend {
	public:
		Backend_Cache(cacheable_t backend);
		virtual ~Backend_Cache();

		bool get_random(scorer_t scorer, link_t& random);

		bool get_prev(selector_t selector, scorer_t scorer,
				const word_t& word, link_t& prev);
		bool get_next(selector_t selector, scorer_t scorer,
				const word_t& word, link_t& next);

		bool increment_link(scorer_t scorer,
				const word_t& first, const word_t& second);

		bool prune(scorer_t scorer);

	private:
		template <typename T>
		struct pair_hash {
		public:
			inline size_t operator()(const std::pair<T,T>& p) const {
				return hash(p.first) ^ hash(p.second);
			}
		private:
			const std::hash<T> hash;
		};

		typedef std::unordered_map<word_t, links_t> word_to_links_t;
		typedef std::unordered_map<std::pair<word_t, word_t>,
			link_t, pair_hash<word_t> > words_to_link_t;

		bool pick_word(const word_to_links_t::iterator& got_iter,
				word_to_links_t& changed_map,
				selector_t selector, scorer_t scorer,
				const word_t& word, link_t& out);

		cacheable_t wrapme;
		state_t state;

		/* prev OR next -> links/NULL */
		word_to_links_t got_prevs, got_nexts,/* unmodified words we've gotten from wrapme */
			changed_prevs, changed_nexts;/* modified words from increment_link */
		/* prev AND next -> link */
		words_to_link_t got_words,/* pool of all unmodified words */
			changed_words;/* pool of all modified words */
	};
}

#endif
