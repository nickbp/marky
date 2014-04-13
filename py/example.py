#!/usr/bin/python

'''
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
'''

# Example usage of Marky's python interface.
# Backends/Selectors/Scorers can of course be customized.

import marky

frontend = marky.Marky(marky.backend_map(), marky.selector_best_always(), marky.scorer_no_adj(), 1)
frontend.insert_line(__doc__.split())
print frontend.produce_line()
