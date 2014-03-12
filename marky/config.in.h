#ifndef MARKY_CONFIG_H
#define MARKY_CONFIG_H

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

#include <stdio.h>

#define _PRINT_PREFIX "%s (%s:%d) "
#define _PRINT_ARGS __FUNCTION__, __LINE__

//#define DEBUG_ENABLED

/* Some simple print helpers */
#ifdef DEBUG_ENABLED
#define DEBUG(...) config::_debug(__FUNCTION__, __VA_ARGS__)
#else
#define DEBUG(...)
#endif
#define LOG(...) config::_log(__FUNCTION__, __VA_ARGS__)
#define ERROR(...) config::_error(__FUNCTION__, __VA_ARGS__)

/* Skips "ERR" and func name in output. Used by help output. */
#define PRINT_HELP(...) config::_error(NULL, __VA_ARGS__)

#cmakedefine BUILD_BACKEND_SQLITE

namespace config {
    static const int
        VERSION_MAJOR = @marky_VERSION_MAJOR@,
        VERSION_MINOR = @marky_VERSION_MINOR@,
        VERSION_PATCH = @marky_VERSION_PATCH@;

    static const char VERSION_STRING[] = "@marky_VERSION_MAJOR@.@marky_VERSION_MINOR@.@marky_VERSION_PATCH@"
#ifdef BUILD_BACKEND_SQLITE
        "-sqlite"
#else
        "-nodb"
#endif
        ;
    static const char BUILD_DATE[] = __TIMESTAMP__;

    extern FILE *fout;
    extern FILE *ferr;

    /* DONT USE THESE DIRECTLY, use DEBUG()/LOG()/ERROR() instead.
     * The ones with a 'format' function support printf-style format before a list of args.
     * The ones without are for direct unformatted output (eg "_error("func", "printme");") */
#ifdef DEBUG_ENABLED
    void _debug(const char* func, const char* format, ...);
    void _debug(const char* func, ...);
#endif

    void _log(const char* func, const char* format, ...);
    void _log(const char* func, ...);
    void _error(const char* func, const char* format, ...);
    void _error(const char* func, ...);
}

#endif
