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

#include "link.h"
#include "scorer.h"
#include "selector.h"

namespace marky {
	/* Base interface for storing/retrieving links. */
	class IBackend {
	public:
		virtual ~IBackend() { }

		/* Gets a random link, or an empty pointer if none was available.
		 * Return false in the event of a backend error. */
		virtual bool get_random(scorer_t scorer, link_t& random) = 0;

		/* For a given 'word', uses 'selector' and 'scorer' to select and return
		 * an associated link, or an empty pointer if no link was found. Returns
		 * false in the event of some error. */

		/* Get a link whose 'prev' is 'word', or an empty pointer if none was
		 * found. Return false in the event of a backend error. */
		virtual bool get_prev(selector_t selector, scorer_t scorer,
				const word_t& word, link_t& prev) = 0;
		/* Get a link whose 'next' is 'word', or an empty pointer if none was
		 * found. Return false in the event of a backend error. */
		virtual bool get_next(selector_t selector, scorer_t scorer,
				const word_t& word, link_t& next) = 0;

		/* For a given pair of words, updates their link, creating one if
		 * necessary. Returns false in the event of a backend error. */
		virtual bool increment_link(scorer_t scorer,
				const word_t& first, const word_t& second) = 0;

		/* Prunes/updates links according to the rules of the scorer. Any links
		 * with an adjusted score of 0 are removed. This is assumed to only be
		 * called periodically, so it's not assumed to be especially fast. */
		virtual bool prune(scorer_t scorer) = 0;
	};
	typedef std::shared_ptr<IBackend> backend_t;

	/* Backends which support being passed to Backend_Cache should implement
	 * this interface. */
	class ICacheable : public IBackend {
	public:
		virtual ~ICacheable() { }

		/* Get the current state for the backend. */
		virtual state_t state() = 0;

		/* Get all links whose 'prev' is 'word', or an empty list if none are
		 * found. Return false in the event of a backend error. */
		virtual bool get_prevs(const word_t& word, links_t& out) = 0;

		/* Get all links whose 'next' is 'word', or an empty list if none are
		 * found. Return false in the event of a backend error. */
		virtual bool get_nexts(const word_t& word, links_t& out) = 0;

		/* Get a link whose 'prev' is 'first' and 'next' is 'second', or an
		 * empty pointer if none are found. Return false in the event of a
		 * backend error */
		virtual bool get_link(const word_t& first, const word_t& second,
				link_t& out) = 0;

		/* Update the backend with the provided link data and state. */
		virtual bool flush(const links_t& links, const state_t& state) = 0;
	};
	typedef std::shared_ptr<ICacheable> cacheable_t;
}

#endif
