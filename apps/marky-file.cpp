/*
  marky - A Markov chain generator.
  Copyright (C) 2012  Nicholas Parker

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
	marky::word_t search;

	size_t count = 1, max_chars = 1000, max_words = 100;
	uint8_t score_weight = 128;
	size_t score_decrement = 0;
}

#define IS_STDIN(file) (strlen(file) == 1 && file[0] == '-')

static void syntax(char* appname) {
	ERROR_RAWDIR("");
	ERROR_RAWDIR("marky-file v%s (built %s)",
		  config::VERSION_STRING,
		  config::BUILD_DATE);
	ERROR_RAWDIR("Produces markov chains from plain text files or stdin.");
	ERROR_RAWDIR("");
	ERROR_RAWDIR("Usage: %s [options] <command>", appname);
	ERROR_RAWDIR("");
	ERROR_RAWDIR("Commands:");
#ifdef BUILD_BACKEND_SQLITE
	ERROR_RAWDIR("  -i/--import <file>  Adds data into --db-file from <file>, or '-' for stdin.");
	ERROR_RAWDIR("  -e/--export         Produces -n chains from previously imported --db-file.");
#endif
	ERROR_RAWDIR("  -p/--print <file>   Produces -n chains from <file>, or '-' for stdin.");
	ERROR_RAWDIR("  -h/--help           This help text.");
	ERROR_RAWDIR("");
	ERROR_RAWDIR("File Options:");
#ifdef BUILD_BACKEND_SQLITE
	ERROR_RAWDIR("  -d/--db-file <file>  The marky db to access. [default=%s]", db_path.c_str());
#endif
	ERROR_RAWDIR("  -l/--log <file>      Append any output to <file> instead of stdout.");
	ERROR_RAWDIR("");
	ERROR_RAWDIR("Output Options:");
	ERROR_RAWDIR("  -s/--search <str>  The search term to look for.");
	ERROR_RAW("  -n/--count <n>     The number of chains to produce. [default=%d]", count);
	ERROR_RAW("  --max-chars <n>    The character length limit of the produced chains. [default=%d]", max_chars);
	ERROR_RAW("  --max-words <n>    The word limit of the produced chains. [default=%d]", max_words);
	ERROR_RAWDIR("");
	ERROR_RAWDIR("Marky Settings:");
	ERROR_RAWDIR("  --score-weight <0-255>  How much preference to give to high scoring links.");
	ERROR_RAW("                          255: always pick highest, 0: ignore score. [default=%hhu]", score_weight);
	ERROR_RAWDIR("  --score-decrement <n>   How frequently to decrease link scores, in number of links.");
	ERROR_RAW("                          High=slow, low=quick, 0=none. [default=%lu]", score_decrement);
	ERROR_RAWDIR("");
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
			{"max-chars", required_argument, NULL, 'w'},
			{"max-words", required_argument, NULL, 'x'},
			{"search", required_argument, NULL, 's'},

			{"score-weight", required_argument, NULL, 'y'},
			{"score-decrement", required_argument, NULL, 'z'},

			{0,0,0,0}
		};

		int option_index = 0;
		c = getopt_long(argc, argv, "i:ep:hd:l:n:s:",
				long_options, &option_index);
		if (c == -1) {//unknown arg (doesnt match -x/--x format)
			if (optind >= argc) {
				//at end of successful parse
				break;
			}
			ERROR_RAW("Unknown argument: '%s'", argv[optind]);
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
					ERROR_RAW("Unable to open input file '%s': %s", optarg, e.what());
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
					ERROR_RAW("Unable to open input file %s: %s", optarg, e.what());
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
				ERROR_RAW("Unable to open log file %s: %s", optarg, e.what());
				return false;
			}
			break;


		case 'n':
			{
				char* err = NULL;
				long int tmp = strtol(optarg, &err, 10);
				if (*err != 0 || tmp <= 0) {
					ERROR_RAW("Invalid argument: -n/--count must be a positive integer: %s", optarg);
					return false;
				}
				count = (size_t)tmp;
			}
			break;
		case 'w':
			{
				char* err = NULL;
				long int tmp = strtol(optarg, &err, 10);
				if (*err != 0 || tmp < 0) {
					ERROR_RAW("Invalid argument: --max-chars must be a 0+ integer: %s", optarg);
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
					ERROR_RAW("Invalid argument: --max-words must be a 0+ integer: %s", optarg);
					return false;
				}
				max_words = (size_t)tmp;
			}
			break;
		case 's':
			search = optarg;
			break;
		case 'y':
			{
				char* err = NULL;
				long int tmp = strtol(optarg, &err, 10);
				if (*err != 0 || tmp < 0 || tmp > 255) {
					ERROR_RAW("Invalid argument: --score-weight must be an integer within 0-255: %s", optarg);
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
					ERROR_RAW("Invalid argument: --score-decrement must be a positive integer or zero: %s", optarg);
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

static void read_file(std::istream& in, marky::Marky& out,
		marky::backend_t pruneme, marky::scorer_t scorer, size_t prunefreq) {
	std::string line_s;
	marky::line_t insertme;
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
			pruneme->prune(scorer);
		}
	}
}

static void print_random(marky::Marky& in, std::ostream& out,
		size_t count, size_t max_words, size_t max_chars,
		marky::word_t search) {
	marky::line_t line;
	for (size_t i = 0; i < count; ++i) {
		if (!in.produce(line, search, max_words, max_chars)) {
			break;
		}

		marky::line_t::const_iterator iter = line.cbegin();
		for (;;) {
			out << *iter;
			if (++iter == line.cend()) {
				out << std::endl;
				break;
			} else {
				out << " ";
			}
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
	marky::scorer_t scorer = marky::scorers::link_adj(score_decrement);

	switch (run_cmd) {
#ifdef BUILD_BACKEND_SQLITE
	case CMD_IMPORT:
		{
			marky::cacheable_t sqlite = marky::Backend_SQLite::create_cacheable(db_path);
			if (!sqlite) {
				return EXIT_FAILURE;
			}
			marky::backend_t backend(new marky::Backend_Cache(sqlite));
			marky::Marky out(backend, selector, scorer);
			read_file(fin, out, backend, scorer, score_decrement);
		}
		return EXIT_SUCCESS;
	case CMD_EXPORT:
		{
			marky::cacheable_t sqlite = marky::Backend_SQLite::create_cacheable(db_path);
			if (!sqlite) {
				return EXIT_FAILURE;
			}
			marky::backend_t backend(new marky::Backend_Cache(sqlite));
			marky::Marky in(backend, selector, scorer);
			print_random(in, fout, count, max_words, max_chars, search);
		}
		return EXIT_SUCCESS;
#endif
	case CMD_PRINT:
		{
			marky::backend_t backend(new marky::Backend_Map);
			marky::Marky marky(backend, selector, scorer);
			read_file(fin, marky, backend, scorer, score_decrement);
			print_random(marky, fout, count, max_words, max_chars, search);
		}
		return EXIT_SUCCESS;
	case CMD_HELP:
		syntax(argv[0]);
		return EXIT_SUCCESS;
	default:
		ERROR_RAW("%s: no command specified", argv[0]);
		syntax(argv[0]);
		return EXIT_FAILURE;
	}
}
