/*
  marky - A Markov chain generator.
  Copyright (C) 2012-2014  Nicholas Parker

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

#include <getopt.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <iostream>

#include <marky/config.h>
#include <marky/build-config.h>
#include <marky/marky.h>

#include <marky/backend-map.h>
#ifdef BUILD_BACKEND_SQLITE
#include <marky/backend-cache.h>
#include <marky/backend-sqlite.h>
#endif

namespace {
    enum CMD { CMD_UNKNOWN, CMD_IMPORT, CMD_EXPORT, CMD_PRINT, CMD_HELP };
    CMD run_cmd = CMD_UNKNOWN;

    std::ifstream file_in;
    std::ofstream file_out;
    std::string db_path("marky.db");
    marky::words_t search;

    size_t count = 1, max_chars = 1000, max_words = 100;
    size_t look_size = 1;
    uint8_t score_weight = 128;
    size_t score_decrement = 0;
}

#define IS_STDIN(file) (strlen(file) == 1 && file[0] == '-')

static void syntax(char* appname) {
    PRINT_HELP("");
    PRINT_HELP("marky-file v%s (built %s)",
          config::VERSION_STRING,
          config::BUILD_DATE);
    PRINT_HELP("Produces markov chains from plain text files or stdin.");
    PRINT_HELP("");
    PRINT_HELP("Usage: %s [options] <command>", appname);
    PRINT_HELP("");
    PRINT_HELP("Commands:");
#ifdef BUILD_BACKEND_SQLITE
    PRINT_HELP("  -i/--import <file>  Adds data into --db-file from <file>, or '-' for stdin.");
    PRINT_HELP("  -e/--export         Produces -n chains from previously imported --db-file.");
#endif
    PRINT_HELP("  -p/--print <file>   Produces -n chains from <file>, or '-' for stdin.");
    PRINT_HELP("  -h/--help           This help text.");
    PRINT_HELP("");
    PRINT_HELP("File Options:");
#ifdef BUILD_BACKEND_SQLITE
    PRINT_HELP("  -d/--db-file <file>  The marky db to access. [default=%s]", db_path.c_str());
#endif
    PRINT_HELP("  -l/--log <file>      Append any output to <file> instead of stdout.");
    PRINT_HELP("");
    PRINT_HELP("Output Options:");
    PRINT_HELP("  -n/--count <n>     The number of chains to produce. [default=%d]", count);
    PRINT_HELP("  -s/--search <str>  A search term to include in the outputted chain(s).");
    PRINT_HELP("  --max-chars <n>    The character length limit of the produced chains. [default=%d]", max_chars);
    PRINT_HELP("  --max-words <n>    The word count limit of the produced chains. [default=%d]", max_words);
    PRINT_HELP("");
    PRINT_HELP("Marky Settings:");
    PRINT_HELP("  -w/--window <1-10>      Max look-ahead/look-behind length, in number of words.");
    PRINT_HELP("                          Bigger: more exact search, Lower: less exact. [default=%lu]", look_size);
    PRINT_HELP("  --score-weight <0-255>  How much preference to give to high scoring links.");
    PRINT_HELP("                          255: always pick highest, 0: ignore score. [default=%hhu]", score_weight);
    PRINT_HELP("  --score-decrement <n>   How frequently to decrease link scores, in number of links.");
    PRINT_HELP("                          High=slow, low=quick, 0=none. [default=%lu]", score_decrement);
    PRINT_HELP("");
}

static bool parse_config(int argc, char* argv[]) {
    if (argc == 1) {
        syntax(argv[0]);
        return false;
    }
    int c = 0;
    while (1) {
        static struct option long_options[] = {
#ifdef BUILD_BACKEND_SQLITE
            {"input", required_argument, NULL, 'i'},
            {"export", no_argument, NULL, 'e'},
#endif
            {"print", required_argument, NULL, 'p'},
            {"help", no_argument, NULL, 'h'},

#ifdef BUILD_BACKEND_SQLITE
            {"db-file", required_argument, NULL, 'd'},
#endif
            {"log", required_argument, NULL, 'l'},

            {"count", required_argument, NULL, 'n'},
            {"search", required_argument, NULL, 's'},
            {"max-chars", required_argument, NULL, 'q'},
            {"max-words", required_argument, NULL, 'x'},

            {"window", required_argument, NULL, 'w'},
            {"score-weight", required_argument, NULL, 'y'},
            {"score-decrement", required_argument, NULL, 'z'},

            {0,0,0,0}
        };

        int option_index = 0;
        c = getopt_long(argc, argv, "i:ep:hd:l:n:s:w:",
                long_options, &option_index);
        if (c == -1) {//unknown arg (doesnt match -x/--x format)
            if (optind >= argc) {
                //at end of successful parse
                break;
            }
            ERROR("Unknown argument: '%s'", argv[optind]);
            syntax(argv[0]);
            return false;
        }

        switch (c) {
        case 'i':
            run_cmd = CMD_IMPORT;
            if (!IS_STDIN(optarg)) {
                file_in.exceptions(std::ifstream::badbit);
                try {
                    file_in.open(optarg);
                } catch (const std::exception& e) {
                    ERROR("Unable to open input file '%s': %s", optarg, e.what());
                    return false;
                }
            }
            break;
        case 'e':
            run_cmd = CMD_EXPORT;
            break;
        case 'p':
            run_cmd = CMD_PRINT;
            if (!IS_STDIN(optarg)) {
                file_in.exceptions(std::ifstream::badbit);
                try {
                    file_in.open(optarg);
                } catch (const std::exception& e) {
                    ERROR("Unable to open input file %s: %s", optarg, e.what());
                    return false;
                }
            }
            break;
        case 'h':
            run_cmd = CMD_HELP;
            break;


        case 'd':
            db_path = optarg;
            break;
        case 'l':
            file_out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            try {
                file_out.open(optarg);
            } catch (const std::exception& e) {
                ERROR("Unable to open log file %s: %s", optarg, e.what());
                return false;
            }
            break;


        case 'n':
            {
                char* err = NULL;
                long int tmp = strtol(optarg, &err, 10);
                if (*err != 0 || tmp <= 0) {
                    ERROR("Invalid argument: -n/--count must be a positive integer: %s", optarg);
                    return false;
                }
                count = (size_t)tmp;
            }
            break;
        case 's':
            {
                char* saveptr = NULL;
                char* word = strtok_r(optarg, " ", &saveptr);
                for (;;) {
                    if (word == NULL) {
                        break;
                    }
                    search.push_back(marky::word_t(word));
                    word = strtok_r(NULL, " ", &saveptr);
                }
            }
            break;
        case 'q':
            {
                char* err = NULL;
                long int tmp = strtol(optarg, &err, 10);
                if (*err != 0 || tmp < 0) {
                    ERROR("Invalid argument: --max-chars must be a 0+ integer: %s", optarg);
                    return false;
                }
                max_chars = (size_t)tmp;
            }
            break;
        case 'x':
            {
                char* err = NULL;
                long int tmp = strtol(optarg, &err, 10);
                if (*err != 0 || tmp < 0) {
                    ERROR("Invalid argument: --max-words must be a 0+ integer: %s", optarg);
                    return false;
                }
                max_words = (size_t)tmp;
            }
            break;
        case 'w':
            {
                char* err = NULL;
                long int tmp = strtol(optarg, &err, 10);
                if (*err != 0 || tmp < 1 || tmp > 10) {
                    ERROR("Invalid argument: --window must be an integer within 0-10: %s", optarg);
                    return false;
                }
                look_size = (uint8_t)tmp;
            }
            break;
        case 'y':
            {
                char* err = NULL;
                long int tmp = strtol(optarg, &err, 10);
                if (*err != 0 || tmp < 0 || tmp > 255) {
                    ERROR("Invalid argument: --score-weight must be an integer within 0-255: %s", optarg);
                    return false;
                }
                score_weight = (uint8_t)tmp;
            }
            break;
        case 'z':
            {
                char* err = NULL;
                long int tmp = strtol(optarg, &err, 10);
                if (*err != 0 || tmp < 0) {
                    ERROR("Invalid argument: --score-decrement must be a positive integer or zero: %s", optarg);
                    return false;
                }
                score_decrement = (size_t)tmp;
            }
            break;
        default:
            syntax(argv[0]);
            return false;
        }
    }

    return true;
}

static void read_file(std::istream& in, marky::Marky& out, size_t prunefreq) {
    std::string line_s;
    marky::words_t insertme;
    size_t count = 0;
    while (in.good()) {
        try {
            std::getline(in, line_s);
        } catch (const std::exception& e) {
            ERROR("Error.. %s", e.what());
            break;
        }
        if (line_s.empty()) {
            continue;
        }
        std::istringstream iss(line_s);

        /* for each word in line_s, append to insertme */
        do {
            insertme.push_back(marky::word_t());
        } while (iss >> insertme.back());
        insertme.pop_back();/* remove the empty word we just added */
        if (insertme.empty()) {
            continue;
        }

        /* send insertme to marky */
        if (!out.insert(insertme)) {
            break;
        }
        insertme.clear();

        /* arbitrary: take the word limit and use it against line count: */
        if (++count == prunefreq) {
            count = 0;
            if (!out.prune_backend()) {
                break;
            }
        }
    }
}

static void print_random(marky::Marky& in, std::ostream& out,
        size_t count, size_t max_words, size_t max_chars,
        const marky::words_t& search) {
    marky::words_t line;
    for (size_t i = 0; i < count; ++i) {
        if (!in.produce(line, search, max_words, max_chars)) {
            break;
        }

        marky::words_t::const_iterator line_iter = line.cbegin();
        if (line_iter == line.cend()) {
            if (search.empty()) {
                out << "Nothing found, no data?" << std::endl;
            } else {
                out << "Nothing found for '";
                for (marky::words_t::const_iterator search_iter = search.begin();
                     search_iter != search.end(); ) {
                    out << *search_iter;
                    if (++search_iter != search.end()) {
                        out << ' ';
                    }
                }
                out << "'." << std::endl;
            }
        } else {
            for (; line_iter != line.cend();) {
                out << *line_iter;
                if (++line_iter != line.cend()) {
                    out << ' ';
                }
            }
            out << std::endl;
        }
        line.clear();
    }
}

int main(int argc, char* argv[]) {
    if (!parse_config(argc, argv)) {
        return EXIT_FAILURE;
    }

    std::istream& fin = (file_in.is_open()) ? file_in : std::cin;
    std::ostream& fout = (file_out.is_open()) ? file_out : std::cout;

    marky::selector_t selector = marky::selectors::best_weighted(score_weight);
    marky::scorer_t scorer = marky::scorers::word_adj(score_decrement);

    switch (run_cmd) {
#ifdef BUILD_BACKEND_SQLITE
    case CMD_IMPORT:
        {
            marky::cacheable_t sqlite =
                marky::Backend_SQLite::create_cacheable(db_path);
            if (!sqlite) {
                return EXIT_FAILURE;
            }
            marky::backend_t backend(new marky::Backend_Cache(sqlite));
            marky::Marky out(backend, selector, scorer, look_size);
            read_file(fin, out, score_decrement);
        }
        return EXIT_SUCCESS;
    case CMD_EXPORT:
        {
            marky::cacheable_t sqlite =
                marky::Backend_SQLite::create_cacheable(db_path);
            if (!sqlite) {
                return EXIT_FAILURE;
            }
            marky::backend_t backend(new marky::Backend_Cache(sqlite));
            marky::Marky in(backend, selector, scorer, look_size);
            print_random(in, fout, count, max_words, max_chars, search);
        }
        return EXIT_SUCCESS;
#endif
    case CMD_PRINT:
        {
            marky::backend_t backend(new marky::Backend_Map);
            marky::Marky marky(backend, selector, scorer, look_size);
            read_file(fin, marky, score_decrement);
            print_random(marky, fout, count, max_words, max_chars, search);
        }
        return EXIT_SUCCESS;
    case CMD_HELP:
        syntax(argv[0]);
        return EXIT_SUCCESS;
    default:
        ERROR("%s: no command specified", argv[0]);
        syntax(argv[0]);
        return EXIT_FAILURE;
    }
}
