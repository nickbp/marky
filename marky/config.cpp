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

#include <stdarg.h>

#include "config.h"
#include "build-config.h"

namespace config {
    char VERSION_STRING[] = MARKY_VERSION_MAJOR "." MARKY_VERSION_MINOR "." MARKY_VERSION_PATCH
#ifdef BUILD_BACKEND_SQLITE
        "-sqlite"
#else
        "-nodb"
#endif
        ;

    bool has_sqlite() {
        return
#ifdef BUILD_BACKEND_SQLITE
            true
#else
            false
#endif
            ;
    }

    FILE *fout = stdout, *ferr = stderr;

#ifdef DEBUG_ENABLED
    void _debug(const char* func, const char* format, ...) {
        va_list args;
        va_start(args, format);
        if (func != NULL) {
            fprintf(fout, "DEBUG %s ", func);
        }
        vfprintf(fout, format, args);
        va_end(args);
        fprintf(fout, "\n");
    }
    void _debug(const char* func, ...) {
        va_list args;
        va_start(args, func);
        if (func != NULL) {
            fprintf(fout, "DEBUG %s ", func);
        }
        vfprintf(fout, "%s\n", args);//only one arg, the string itself
        va_end(args);
    }
#endif

    void _log(const char* func, const char* format, ...) {
        va_list args;
        va_start(args, format);
        if (func != NULL) {
            fprintf(fout, "LOG %s() ", func);
        }
        vfprintf(fout, format, args);
        va_end(args);
        fprintf(fout, "\n");
    }
    void _log(const char* func, ...) {
        va_list args;
        va_start(args, func);
        if (func != NULL) {
            fprintf(fout, "LOG %s() ", func);
        }
        vfprintf(fout, "%s\n", args);//only one arg, the string itself
        va_end(args);
    }

    void _error(const char* func, const char* format, ...) {
        va_list args;
        va_start(args, format);
        if (func != NULL) {
            fprintf(ferr, "ERROR %s() ", func);
        }
        vfprintf(ferr, format, args);
        va_end(args);
        fprintf(ferr, "\n");
    }
    void _error(const char* func, ...) {
        va_list args;
        va_start(args, func);
        if (func != NULL) {
            fprintf(ferr, "ERROR %s() ", func);
        }
        vfprintf(ferr, "%s\n", args);//only one arg, the string itself
        va_end(args);
    }
}
