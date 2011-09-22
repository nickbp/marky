#ifndef MARKY_SCORER_H
#define MARKY_SCORER_H

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

#include <functional>

#include "link.h"

namespace marky {
	/* Calculate the adjusted score for a link.
	   'link' is the PREVIOUS link and its info, 'info' is the CURRENT info */
	typedef std::function<score_t (const link_t& link, const info_t& info)> scorer_t;

	namespace scorers {
		/* No adjustment, scores just increment sequentially as links are encountered. */
		scorer_t no_adj();

		/* Adjusts scores to slowly decrease as other links are encountered. */
		scorer_t link_adj(size_t score_decrement_links);

		/* Adjusts scores to slowly decrease as time passes. */
		scorer_t time_adj(time_t score_decrement_time);
	}
}

#endif
