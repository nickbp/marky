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

# -- C Library Connection

def connect_lib(lib_path = None):
    """ Initializes the connection to an underlying Marky C Library.
    This should only need to be called if the default path is insufficient.
    lib_path is the path to the library itself (eg /path/to/libmarky.so, or c:\path\to\marky.dll)."""
    global __marky_c_interface
    if not '__marky_c_interface' in globals():
        try:
            __marky_c_interface = marky_ctypes.MARKY_INTERFACE(lib_path)
        except Exception as e:
            raise Exception("""Unable to initialize connection to Marky's C Interface.
Use marky.connect_lib(path) to provide a custom path to the Marky library.
Original exception: %s""" % e)
    return __marky_c_interface.i

connect_lib()

# -- Backends

def backend_map():
    """ Creates a Map/Dict based Backend, which loses all state when the program exits. """
    return __Backend(connect_lib().marky_backend_new_map())

def backend_sqlite(db_path):
    """ Creates an SQLite-based Backend, which stores its data to an SQLite file.

    Raises:
    - Exception if Marky didn't have SQLite support enabled.
    - IOError if the provided db_path couldn't be opened."""
    if connect_lib().marky_has_sqlite() != 0:
        raise Exception("This instance of Marky doesn't have SQLite support enabled. Rebuild with BUILD_BACKEND_SQLITE enabled.")
    sqlite_cacheable = connect_lib().marky_backend_new_sqlite_cacheable(db_path)
    if sqlite_cacheable == None:
        raise IOError("Unable to open SQLite DB: %s" % db_path)
    # cache takes ownership of cacheable:
    return __Backend(connect_lib().marky_backend_new_cache(sqlite_cacheable))

# -- Selectors

def selector_best_always():
    """ Returns a Selector which always selects the best snippet by score, with zero randomness (unless two scores are equal).
    Equivalent to selector_best_weighted with a very high weight_factor. """
    return __Selector(connect_lib().marky_selector_new_best_always())

def selector_best_random():
    """ Returns a Selector which always selects snippets randomly, regardless of score, with extreme randomness.
    Equivalent to selector_best_weighted with a very low weight_factor. """
    return __Selector(connect_lib().marky_selector_new_best_random())

def selector_best_weighted(weight_factor):
    """ Returns a Selector which selects snippets with a custom degree of randomness.

    'weight_factor' modifies how the weighing is exaggerated.
    factor > 128:
      More weight to higher-scoring snippets (less random), 255 = best_always()
    factor < 128:
      More weight to lesser-scoring snippets (more random), 0 = random() """
    return __Selector(connect_lib().marky_selector_new_best_weighted(weight_factor))

# -- Scorers

def scorer_no_adj():
    """ Returns a Scorer which performs no adjustment to scores. Scores just increment sequentially as words are encountered.
    This is useful when parsing data where all data should be weighted equally, like the content of a book. """
    return __Scorer(connect_lib().marky_scorer_new_no_adj())

def scorer_word_adj(score_decrement_words):
    """ Returns a Scorer which adjusts scores downward to slowly decrease as additional words are encountered.
    This is useful for parsing log archives or forums, where more recent data should have higher weighting than less recent data.

    score_decrement_words is the number of words which equate to a point decrease.
    For example, a value of 100 means that a given snippet loses one point after 100 other words have appeared. If decrement is 0, the Scorer will be equivalent to no_adj(). """
    return __Scorer(connect_lib().marky_scorer_new_word_adj(score_decrement_words))

def scorer_time_adj(score_decrement_seconds):
    """ Returns a Scorer which adjusts scores to slowly decrease as time passes.
    This is useful for parsing live/ongoing streams of data, where more recent data should have higher weighting than less recent data.

    score_decrement_seconds is the number of seconds which equate to a point decrease.
    For example, a value of 100 means that a given snippet loses one point after 100 seconds have transpired. If the decrement is 0, the Scorer will be equivalent to no_adj()."""
    return __Scorer(connect_lib().marky_scorer_new_time_adj(score_decrement_seconds))

# -- Marky

class Marky:
    """ The main Marky Frontend. Given a Backend, Selector, and Scorer, the Frontend handles inserting and retrieving Markov Chains. """

    def __init__(self, backend, selector, scorer, look_size):
        """ Creates a Marky instance using the provided components and a look size.
        The choice of components will determine how Marky scores and stores any input."""
        self.__marky_instance = connect_lib().marky_new(backend.backend_c(), selector.selector_c(), scorer.scorer_c(), look_size)

    def __del__(self):
        connect_lib().marky_free(self.__marky_instance)

    def insert_line(self, words):
        """ Adds the List of words (and their inter-word snippets) to the dataset.
        Raises an Exception in the event of some error. """
        res = connect_lib().marky_insert(self.__marky_instance, self.__to_c_words(words))
        if res != 0:
            raise Exception("Failed to insert line: %s", words)

    def produce_line(self, search = [], length_limit_words = 100, length_limit_chars = 1000):
        """Produces a word List from the search word(s), or from a random word if the search words are unspecified.

        'length_limit_words' and 'length_limit_chars' each allow setting APPROXIMATE limits on the length of the result. If either limit is set to zero, that limit is disabled. One of the two limits MUST always be non-zero, to avoid infinite loops.

        Produces an empty line if the search words (if any) wasn't found, or if no data was available. Raises Exception in the event of an error. """
        if search:
            search_c = self.__to_c_words(search)
        else:
            search_c = None
        words_out_c = (ctypes.POINTER(marky_ctypes.WORDS_STRUCT))()
        res = connect_lib().marky_produce(self.__marky_instance, ctypes.byref(words_out_c), search_c,
                                               ctypes.c_ulong(length_limit_words), ctypes.c_ulong(length_limit_chars))
        if res != 0:
            connect_lib().marky_words_free(words_out_c)
            raise Exception("Failed to produce words.")
        if not words_out_c:
            return None

        words_out = []
        for i in xrange(0, words_out_c.contents.words_count):
            words_out.append(words_out_c.contents.words[i])
        connect_lib().marky_words_free(words_out_c)
        return words_out

    def prune_backend(self):
        """ Tells the underlying backend to clean up any stale snippets it may have lying around.
        This may be called periodically to free up resources. """
        res = connect_lib().marky_prune_backend(self.__marky_instance)
        if res != 0:
            raise Exception("Failed to prune backend.")

    def __to_c_words(self, words):
        words_c = marky_ctypes.WORDS_STRUCT()
        words_c.words_count = len(words)
        words_c.words = (ctypes.c_char_p * len(words))()
        for i in xrange(0, len(words)):
            # note, this is memory OWNED BY PYTHON. NOT a new alloc
            words_c.words[i] = words[i].encode('utf-8')
        return words_c

# -- Internal types

class __Backend:
    """ A container for a Marky Backend.
    Do not instantiate directly, instead create Backend instances using backend_*()."""

    def __init__(self, backend_c):
        self.__backend_c = backend_c

    def __del__(self):
        connect_lib().marky_backend_free(self.__backend_c)

    def backend_c(self):
        return self.__backend_c

class __Selector:
    """ A container for a Marky Selector.
    Do not instantiate directly, instead create Selector instances using selector_*()."""

    def __init__(self, selector_c):
        self.__selector_c = selector_c

    def __del__(self):
        connect_lib().marky_selector_free(self.__selector_c)

    def selector_c(self):
        return self.__selector_c

class __Scorer:
    """ A container for a Marky Scorer.
    Do not instantiate directly, instead create Scorer instances using scorer_*()."""

    def __init__(self, scorer_c):
        self.__scorer_c = scorer_c

    def __del__(self):
        connect_lib().marky_scorer_free(self.__scorer_c)

    def scorer_c(self):
        return self.__scorer_c
