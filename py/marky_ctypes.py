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

MARKY_LIB_NAME = "marky"
# Assume the user has built Marky in ../bin:
MARKY_LIB_PATH = "../bin/marky/lib" + MARKY_LIB_NAME + ".so"

import ctypes, ctypes.util

class WORDS_STRUCT(ctypes.Structure):
    _fields_ = [("words", ctypes.POINTER(ctypes.c_char_p)),
                ("words_count", ctypes.c_ulong)]
class BACKEND_STRUCT(ctypes.Structure):
    pass
class BACKEND_CACHEABLE_STRUCT(ctypes.Structure):
    pass
class SELECTOR_STRUCT(ctypes.Structure):
    pass
class SCORER_STRUCT(ctypes.Structure):
    pass
class MARKY_STRUCT(ctypes.Structure):
    pass

class MARKY_INTERFACE:
    def __init__(self, lib_path = None):
        if not lib_path:
            lib_path = MARKY_LIB_PATH
        try:
            self.i = ctypes.cdll.LoadLibrary(lib_path)
        except OSError: # file not found
            markyname = ctypes.util.find_library(MARKY_LIB_NAME)
            if not markyname:
                raise Exception("Can't find marky! Tried: path=%s, name=%s" % (MARKY_LIB_PATH, MARKY_LIB_NAME))
            self.i = ctypes.cdll.LoadLibrary(markyname)

        self.i.marky_new.restype = ctypes.POINTER(MARKY_STRUCT)
        self.i.marky_new.argtypes = \
            [ctypes.POINTER(BACKEND_STRUCT), ctypes.POINTER(SELECTOR_STRUCT),
             ctypes.POINTER(SCORER_STRUCT), ctypes.c_ulong]

        self.i.marky_free.restype = None
        self.i.marky_free.argtypes = [ctypes.POINTER(MARKY_STRUCT)]

        self.i.marky_insert.restype = ctypes.c_int
        self.i.marky_insert.argtypes = [ctypes.POINTER(MARKY_STRUCT), ctypes.POINTER(WORDS_STRUCT)]

        self.i.marky_produce.restype = ctypes.c_int
        self.i.marky_produce.argtypes = \
            [ctypes.POINTER(MARKY_STRUCT),
             ctypes.POINTER(ctypes.POINTER(WORDS_STRUCT)), ctypes.POINTER(WORDS_STRUCT),
             ctypes.c_ulong, ctypes.c_ulong]

        self.i.marky_prune_backend.restype = ctypes.c_int
        self.i.marky_prune_backend.argtypes = [ctypes.POINTER(MARKY_STRUCT)]

        # ---

        self.i.marky_backend_new_map.restype = ctypes.POINTER(BACKEND_STRUCT)
        self.i.marky_backend_new_map.argtypes = []

        self.i.marky_backend_new_cache.restype = ctypes.POINTER(BACKEND_STRUCT)
        self.i.marky_backend_new_cache.argtypes = [ctypes.POINTER(BACKEND_CACHEABLE_STRUCT)]

        self.i.marky_backend_new_sqlite_direct.restype = ctypes.POINTER(BACKEND_STRUCT)
        self.i.marky_backend_new_sqlite_direct.argtypes = [ctypes.c_char_p]

        self.i.marky_backend_new_sqlite_cacheable.restype = ctypes.POINTER(BACKEND_CACHEABLE_STRUCT)
        self.i.marky_backend_new_sqlite_cacheable.argtypes = [ctypes.c_char_p]

        self.i.marky_has_sqlite.restype = ctypes.c_int
        self.i.marky_has_sqlite.argtypes = []

        self.i.marky_backend_free.restype = None
        self.i.marky_backend_free.argtypes = [ctypes.POINTER(BACKEND_STRUCT)]

        self.i.marky_backend_cacheable_free.restype = None
        self.i.marky_backend_cacheable_free.argtypes = [ctypes.POINTER(BACKEND_STRUCT)]

        # ---

        self.i.marky_selector_new_best_always.restype = ctypes.POINTER(SELECTOR_STRUCT)
        self.i.marky_selector_new_best_always.argtypes = []

        self.i.marky_selector_new_random.restype = ctypes.POINTER(SELECTOR_STRUCT)
        self.i.marky_selector_new_random.argtypes = []

        self.i.marky_selector_new_best_weighted.restype = ctypes.POINTER(SELECTOR_STRUCT)
        self.i.marky_selector_new_best_weighted.argtypes = [ctypes.c_ubyte]

        self.i.marky_selector_free.restype = None
        self.i.marky_selector_free.argtypes = [ctypes.POINTER(SELECTOR_STRUCT)]

        # ---

        self.i.marky_scorer_new_no_adj.restype = ctypes.POINTER(SCORER_STRUCT)
        self.i.marky_scorer_new_no_adj.argtypes = []

        self.i.marky_scorer_new_word_adj.restype = ctypes.POINTER(SCORER_STRUCT)
        self.i.marky_scorer_new_word_adj.argtypes = [ctypes.c_ulong]

        self.i.marky_scorer_new_time_adj.restype = ctypes.POINTER(SCORER_STRUCT)
        self.i.marky_scorer_new_time_adj.argtypes = [ctypes.c_ulong]

        self.i.marky_scorer_free.restype = None
        self.i.marky_scorer_free.argtypes = [ctypes.POINTER(SCORER_STRUCT)]

        # ---

        self.i.marky_words_new.restype = ctypes.POINTER(WORDS_STRUCT)
        self.i.marky_words_new.argtypes = [ctypes.c_ulong]

        self.i.marky_words_free.restype = None
        self.i.marky_words_free.argtypes = [ctypes.POINTER(WORDS_STRUCT)]
