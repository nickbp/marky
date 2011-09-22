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

#include <memory>

#include "link.h"
#include "scorer.h"

namespace marky {
	/* Base interface for storing/retrieving links. */
	class IBackend {
	public:
		virtual ~IBackend() { }

		/* Gets the current state of the backend.
		 * This should only be considered valid at the start of processing. */
		virtual bool get_info(info_t& info) = 0;

		/* Updates the state of the backend before shutting down. */
		virtual bool update_info(const info_t& info) = 0;


		/* Gets a random link and returns true, or returns false on failure.
		 * NOTE: The link's score is unadjusted. */
		virtual bool get_random(link_t& random) = 0;

		/* For a given 'word', retrieves all available preceding/following
		 * links, or returns false on failure. NOTE: The links' scores are
		 * unadjusted. */
		virtual bool get_by_word(links_t& prev, links_t& next,
				const word_t& word) = 0;

		/* For a given pair of words, retrieves the link for them, or returns
		 * false if the requested link couldn't be found. */
		virtual bool get_by_link(link_t& link,
				const word_t& first, const word_t& second) = 0;

		/* Sets/updates the link in question, or returns false on failure. */
		virtual bool update_link(const link_t& link) = 0;

		/* Prunes links according to the rules of the scorer. Any links with an
		 * adjusted score of 0 are removed. */
		virtual bool prune(scorer_t scorer) = 0;
	};
	typedef std::shared_ptr<IBackend> backend_t;

	/* A simple one-off map which loses state upon destruction. */
	class Backend_Map : public IBackend {
	public:
		bool get_info(info_t& info);
		bool update_info(const info_t& info);

		bool get_random(link_t& random);
		bool get_by_word(links_t& prev, links_t& next, const word_t& word);
		bool get_by_link(link_t& link, const word_t& first, const word_t& second);
		bool update_link(const link_t& link);

		bool prune(scorer_t scorer);
	};

	//TODO other backends: sqlite3, kyotocabinet?
}

#endif
