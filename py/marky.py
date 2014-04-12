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

import ctypes, marky_ctypes

class Marky:
    def __init__(self, path = None):
        self.__interface = marky_ctypes.MARKY_INTERFACE()

        backend_c = self.__interface.i.marky_backend_new_map()
        selector_c = self.__interface.i.marky_selector_new_best_always()
        scorer_c = self.__interface.i.marky_scorer_new_no_adj()
        self.__marky_instance = self.__interface.i.marky_new(backend_c, selector_c, scorer_c, 1)
        self.__interface.i.marky_backend_free(backend_c)
        self.__interface.i.marky_selector_free(selector_c)
        self.__interface.i.marky_scorer_free(scorer_c)

    def __to_c_words(self, words):
        words_c = marky_ctypes.WORDS_STRUCT()
        words_c.words_count = len(words)
        words_c.words = (ctypes.c_char_p * len(words))()
        for i in xrange(0, len(words)):
            # note, this is memory OWNED BY PYTHON. NOT a new alloc
            words_c.words[i] = words[i]
        return words_c

    def insert_line(self, words):
        self.__interface.i.marky_insert(self.__marky_instance, self.__to_c_words(words))

    def produce_line(self, search = [], length_limit_words = 0, length_limit_chars = 0):
        if search:
            search_c = self.__to_c_words(search)
        else:
            search_c = None
        words_out_c = (ctypes.POINTER(marky_ctypes.WORDS_STRUCT))()
        self.__interface.i.marky_produce(self.__marky_instance, ctypes.byref(words_out_c), search_c,
                              ctypes.c_ulong(length_limit_words), ctypes.c_ulong(length_limit_chars))
        if not words_out_c:
            return None

        words_out = []
        for i in xrange(0, words_out_c.contents.words_count):
            words_out.append(words_out_c.contents.words[i])
        self.__interface.i.marky_words_free(words_out_c)
        return words_out
