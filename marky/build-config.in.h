#ifndef MARKY_BUILD_CONFIG_H
#define MARKY_BUILD_CONFIG_H

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

/* Defined if the build included SQLite support. */
#cmakedefine BUILD_BACKEND_SQLITE

#define MARKY_VERSION_MAJOR "@marky_VERSION_MAJOR@"
#define MARKY_VERSION_MINOR "@marky_VERSION_MINOR@"
#define MARKY_VERSION_PATCH "@marky_VERSION_PATCH@"

#endif
