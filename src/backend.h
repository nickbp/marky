#ifndef MARKY_BACKEND_H
#define MARKY_BACKEND_H

/*
  marky - A Markov chain generator.
  Copyright (C) 2011  Nicholas Parker

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

#include "link.h"
#include "scorer.h"
#include "selector.h"

namespace marky {
	/* Base interface for storing/retrieving links. */
	class IBackend {
	public:
		virtual ~IBackend() { }

		/* Gets a random link and returns true, or returns false on failure.
		 * NOTE: The link's score is unadjusted. */
		virtual bool get_random(link_t& random) = 0;

		/* For a given 'word', uses 'selector' and 'scorer' to select and return
		 * an associated link, or an empty pointer if no link was found. Returns
		 * false in the event of some error. */
		virtual bool get_prev(link_t& prev,
				selector_t selector, scorer_t scorer,
				const word_t& word) = 0;
		virtual bool get_next(link_t& next,
				selector_t selector, scorer_t scorer,
				const word_t& word) = 0;

		/* For a given pair of words, updates their link, creating one if
		 * necessary. Returns false in the event of some error. */
		virtual bool increment_link(scorer_t scorer,
				const word_t& first, const word_t& second) = 0;

		/* Prunes/updates links according to the rules of the scorer. Any links
		 * with an adjusted score of 0 are removed. */
		virtual bool prune(scorer_t scorer) = 0;
	};
	typedef std::shared_ptr<IBackend> backend_t;

	/* A simple one-off map which loses state upon destruction. */
	class Backend_Map : public IBackend {
	public:
		Backend_Map();

		bool get_random(link_t& random);

		bool get_prev(link_t& prev,
				selector_t selector, scorer_t scorer,
				const word_t& word);
		bool get_next(link_t& next,
				selector_t selector, scorer_t scorer,
				const word_t& word);

		bool increment_link(scorer_t scorer,
				const word_t& first, const word_t& second);

		bool prune(scorer_t scorer);

	private:
		state_t state;
		link_t last_link;
		typedef std::unordered_map<word_t, links_t> word_to_links_t;
		word_to_links_t prevs, nexts;
		typedef std::unordered_map<std::pair<word_t,word_t>, link_t> words_to_link_t;
		words_to_link_t words;
	};

	//TODO other backends: sqlite3, kyotocabinet?
}

#endif
