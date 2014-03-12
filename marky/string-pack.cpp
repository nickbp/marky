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

#include "string-pack.h"

/*
  normal delimited: ["hello","world"] <-> "hello,world"
  escaped delimited: ["hello,","world"] <-> "hello\,,world"
*/
#define PACK_DELIM ','
#define PACK_ESCAPE '\\'

void marky::pack(const words_t& words, std::ostringstream& oss) {
    for (words_t::const_iterator words_iter = words.begin();
         words_iter != words.end(); ) {
        const word_t& word = *words_iter;
        if (word.find(PACK_DELIM) == std::string::npos) {
            /* shortcut: word passthrough */
            oss << word;
        } else {
            for (size_t i = 0; i < word.size(); ++i) {
                if (word[i] == PACK_DELIM) {
                    /* escaped delim */
                    oss << PACK_ESCAPE << PACK_DELIM;
                } else {
                    oss << word[i];
                }
            }
        }
        if (++words_iter != words.end()) {
            oss << PACK_DELIM;
        }
    }
}

void marky::unpack(const std::string& str, words_t& words) {
    if (str.find(PACK_DELIM) == std::string::npos) {
        /* shortcut: single word passthrough */
        words.push_back(str);
        return;
    }

    std::ostringstream cur_word;
    for (size_t cur_pos = 0; cur_pos < str.size(); ++cur_pos) {
        switch (str[cur_pos]) {
        case PACK_ESCAPE:
            if (cur_pos + 1 < str.size()) {
                /* escaped: treat next char as verbatim and skip past it */
                cur_word << str[cur_pos + 1];
                ++cur_pos;
            } else {
                /* bad input. just pass through the escape i guess. */
                cur_word << PACK_ESCAPE;
            }
            break;
        case PACK_DELIM:
            /* word swap */
            words.push_back(cur_word.str());
            cur_word.clear();
            cur_word.str("");
            break;
        default:
            /* save char keep going */
            cur_word << str[cur_pos];
            break;
        }
    }
    /* remaining word, or preserving empty string/LINE_END */
    words.push_back(cur_word.str());
}
