/* parse_cmdline.c - parsing of command line options */

#include "common_func.h" /* should be included before the C library files */
#include <assert.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> /* stat() */
#ifdef _WIN32
#include <windows.h> /* for SetFileApisToOEM(), CharToOem() */
#endif

#include "librhash/rhash.h"
#include "librhash/plug_openssl.h"
#include "win_utils.h"
#include "file_mask.h"
#include "crc_print.h"
#include "output.h"
#include "rhash_main.h"
#include "version.h"
#include "parse_cmdline.h"

#define VERSION_STRING PROGRAM_NAME " v" VERSION "\n"

typedef struct options_t options_t;
struct options_t conf_opt; /* config file parsed options */
struct options_t opt;      /* command line options */

/**
 * Print program help.
 */
static void print_help(void)
{
	assert(rhash_data.out != NULL);
	fprintf(rhash_data.out, "%s\n%s", VERSION_STRING,
		"Usage: " CMD_FILENAME " [<option> ...] <filename|-> [...]\n"
		"       " CMD_FILENAME " --printf=<format string> <filename|-> [...]\n\n"
		"Options:\n"
		"  -V, --version Print " PROGRAM_NAME " version and exit.\n"
		"  -h, --help    Print this help screen.\n"
		"  -C, --crc32   Calculate CRC32 hash sum.\n"
		"      --md4     Calculate MD4   hash sum.\n"
		"  -M, --md5     Calculate MD5   hash sum.\n"
		"  -H, --sha1    Calculate SHA1  hash sum.\n"
		"      --sha224, --sha256, --sha384, --sha512 Calculate SHA2 hash sum.\n"
		"  -T, --tth     Calculate TTH sum.\n"
		"      --btih    Calculate BitTorrent InfoHash.\n"
		"  -A, --aich    Calculate AICH hash.\n"
		"  -E, --ed2k    Calculate eDonkey hash sum.\n"
		"  -L, --ed2k-link  Calculate and print eDonkey link.\n"
		"      --tiger   Calculate Tiger hash sum.\n"
		"  -G, --gost    Calculate GOST R 34.11-94 hash.\n"
		"      --gost-cryptopro CryptoPro version of the GOST R 34.11-94 hash.\n"
		"      --ripemd160  Calculate RIPEMD-160 hash.\n"
		"      --has160  Calculate HAS-160 hash.\n"
		"      --edonr256, --edonr512  Calculate EDON-R 256/512 hash.\n"
		"      --snefru128, --snefru256  Calculate SNEFRU-128/256 hash.\n"
		"  -a, --all     Calculate all supported hashes.\n"
		"  -c, --check   Check hash files specified by command line.\n"
		"  -u, --update  Update hash files specified by command line.\n"
		"  -e, --embed-crc  Rename files by inserting crc32 sum into name.\n"
		"      --check-embedded  Verify files by crc32 sum embedded in their names.\n"
		"      --list-hashes  List the names of supported hashes, one per line.\n"
		"  -B, --benchmark  Benchmark selected algorithm.\n"
		"  -v, --verbose Be verbose.\n"
		"  -r, --recursive  Process directories recursively.\n"
		"      --skip-ok Don't print OK messages for successfully verified files.\n"
		"  -i, --ignore-case  Ignore case of filenames when updating hash files.\n"
		"      --percents   Show percents, while calculating or checking hashes.\n"
		"      --speed   Output per-file and total processing speed.\n"
		"      --maxdepth=<n> Descend at most <n> levels of directories.\n"
		"  -o, --output=<file> File to output calculation or checking results.\n"
		"  -l, --log=<file>    File to log errors and verbose information.\n"
		"      --sfv     Print hash sums, using SFV format (default).\n"
		"      --bsd     Print hash sums, using BSD-like format.\n"
		"      --simple  Print hash sums, using simple format.\n"
		"  -m, --magnet  Print hash sums  as magnet links.\n"
		"      --torrent Create torrent files.\n"
#ifdef _WIN32
		"      --ansi    Use Windows codepage for output (Windows only).\n"
#endif
		"      --template=<file> Load a printf-like template from the <file>\n"
		"  -p, --printf=<format string>  Format and print hash sums.\n"
		"                See the RHash manual for details.\n"
		);
	rsh_exit(0);
}

/**
 * Print the names of all supported hash algorithms to the console.
 */
static void list_hashes(void)
{
	int id;
	assert(rhash_data.out != NULL);
	for(id = 1; id < RHASH_ALL_HASHES; id <<= 1) {
		const char* hash_name = rhash_get_name(id);
		if(hash_name) fprintf(rhash_data.out, "%s\n", hash_name);
	}
	rsh_exit(0);
}

/**
 * Process --accept and --crc-accept options.
 */
static void crc_accept(options_t *o, char* accept_string, unsigned type)
{
	file_mask_array** ptr = (type ? &o->crc_accept : &o->files_accept);
	if(!*ptr) *ptr = file_mask_new();
	file_mask_add_list(*ptr, accept_string);
}

/**
 * Process an --openssl option.
 */
static void openssl_flags(options_t *o, char* openssl_hashes, unsigned type)
{
#ifdef USE_OPENSSL
	char *cur, *next;
	(void)type;
	o->openssl_mask = 0x80000000;

	/* set the openssl_mask */
	for(cur = openssl_hashes; cur && *cur; cur = next) {
		print_hash_info *info = hash_info_table;
		unsigned bit;
		size_t length;
		next = strchr(cur, ',');
		length = (next != NULL ? (size_t)(next - cur) : strlen(cur));

		for(bit = 1; bit <= RHASH_ALL_HASHES; bit = bit << 1, info++) {
			if( (bit && OPENSSL_SUPPORTED_HASHES_MASK) &&
				memcmp(cur, info->short_name, length) == 0 &&
				info->short_name[length] == 0) {
					o->openssl_mask |= bit;
					break;
			}
		}
		if(bit > RHASH_ALL_HASHES) {
			cur[length] = 0;
			log_msg(PROGRAM_NAME " warning: openssl option doesn't support '%s' hash\n", cur);
		}
	}
#else
	(void)type;
	(void)openssl_hashes;
	(void)o;
	log_msg(PROGRAM_NAME " warning: compiled without openssl support\n");
#endif
}

/**
 * Process --video option.
 */
static void accept_video(options_t *o)
{
	crc_accept(o, ".avi,.ogm,.mkv,.mp4,.mpeg,.mpg,.asf,.rm,.wmv,.vob", 0);
}

/**
 * Say nya! Keep secret! =)
 */
static void nya(void)
{
	assert(rhash_data.out != NULL);
	fprintf(rhash_data.out, "\n  /\\___/\\   _\n = ' T ' = //\n"
		"  |     |\\//\n  | | | | )\n  \"\"  \"\" \"\n");
	rsh_exit(0);
}

/**
 * Process on --maxdepth option.
 *
 * @param o pointer to the processed option
 * @param number string containing the max-depth number
 * @param param unused parameter
 */
static void set_max_depth(options_t *o, char* number, unsigned param)
{
	(void)param;
	if(strspn(number, "0123456789") < strlen(number)) {
		log_msg(PROGRAM_NAME ": maxdepth parameter is not a number: %s\n", number);
		rsh_exit(2);
	}
	o->find_max_depth = atoi(number);
}

/**
 * Set bittorent file piece-length
 *
 * @param o pointer to the processed option
 * @param number string containing the bt-piece-length number
 * @param param unused parameter
 */
static void set_bt_piece_length(options_t *o, char* number, unsigned param)
{
	(void)param;
	if(strspn(number, "0123456789") < strlen(number)) {
		log_msg(PROGRAM_NAME ": bt-piece-length parameter is not a number: %s\n", number);
		rsh_exit(2);
	}
	o->bt_piece_length = (size_t)atoi(number);
}

/**
 * Set the path separator to use when printing paths
 *
 * @param o pointer to the processed option
 * @param sep file separator, can be only '/' or '\'
 * @param param unused parameter
 */
static void set_path_separator(options_t *o, char* sep, unsigned param)
{
	(void)param;
	if((*sep == '/' || *sep == '\\') && sep[1] == 0) {
		o->path_separator = *sep;
#if defined(_WIN32)
		/* MSYS enviropment changes '/' in command line to HOME, see http://www.mingw.org/wiki/FAQ */
	} else if(getenv("MSYSTEM") || getenv("TERM")) {
		log_msg("warning: wrong path-separator, use '//' instead of '/' on MSYS\n");
		o->path_separator = '/';
#endif
	} else {
		log_msg(PROGRAM_NAME ": error: path-separator is not '/' or '\\': %s\n", sep);
		rsh_exit(2);
	}
}

/**
 * Information about a command line option.
 */
typedef struct cmdline_opt_t
{
	char  type;           /* how to porcess option, see below */
	char  short1, short2; /* short option names */
	char* long_name;      /* long option name */
	void* ptr;            /* accessorial pointer, e.g. to an opt field */
	unsigned param;       /* optional integer parameter */
} cmdline_opt_t;

/* The 'type' character of cmdline_opt_t determines how option will be processed:
 * u - set a bit flag in a uint32_t field of the opt structure
 * s - store the second parameter into a char* field
 * P - store file path (without a codepage conversion)
 * F - call a function to process the following string parameter
 * f - call a function for complex option processing
 * p - print a constant string and exit
 * Note, that only option of types 's', 'P', 'F' require a second parameter.
 */

/* supported program options */
cmdline_opt_t cmdline_opt[] = {
	/* program modes */
	{ 'u', 'c',   0, "check",  &opt.mode, MODE_CHECK },
	{ 'u',   0,   0, "check-embedded",  &opt.mode, MODE_CHECK_EMBEDDED },
	{ 'u', 'u',   0, "update", &opt.mode, MODE_UPDATE },
	{ 'u', 'B',   0, "benchmark", &opt.mode, MODE_BENCHMARK },
	{ 'u',   0,   0, "torrent", &opt.mode, MODE_TORRENT },
	{ 'f',   0,   0, "list-hashes", list_hashes, 0 },
	{ 'f', 'h',   0, "help",   print_help, 0 },
	{ 'p', 'V',   0, "version", VERSION_STRING, 0 },
	/* hash sums options */
	{ 'u', 'a',   0, "all",    &opt.sum_flags, RHASH_ALL_HASHES },
	{ 'u', 'C',   0, "crc32",  &opt.sum_flags, RHASH_CRC32 },
	{ 'u',   0,   0, "md4",    &opt.sum_flags, RHASH_MD4 },
	{ 'u', 'M',   0, "md5",    &opt.sum_flags, RHASH_MD5 },
	{ 'u', 'H',   0, "sha1",   &opt.sum_flags, RHASH_SHA1 },
	{ 'u',   0,   0, "sha224", &opt.sum_flags, RHASH_SHA224 },
	{ 'u',   0,   0, "sha256", &opt.sum_flags, RHASH_SHA256 },
	{ 'u',   0,   0, "sha384", &opt.sum_flags, RHASH_SHA384 },
	{ 'u',   0,   0, "sha512", &opt.sum_flags, RHASH_SHA512 },
	{ 'u',   0,   0, "tiger",  &opt.sum_flags, RHASH_TIGER },
	{ 'u', 'T',   0, "tth",    &opt.sum_flags, RHASH_TTH },
	{ 'u',   0,   0, "btih",   &opt.sum_flags, RHASH_BTIH },
	{ 'u', 'E',   0, "ed2k",   &opt.sum_flags, RHASH_ED2K },
	{ 'u', 'A',   0, "aich",   &opt.sum_flags, RHASH_AICH },
	{ 'u', 'G',   0, "gost",   &opt.sum_flags, RHASH_GOST },
	{ 'u',   0,   0, "gost-cryptopro", &opt.sum_flags, RHASH_GOST_CRYPTOPRO },
	{ 'u', 'W',   0, "whirlpool", &opt.sum_flags, RHASH_WHIRLPOOL },
	{ 'u',   0,   0, "ripemd160", &opt.sum_flags, RHASH_RIPEMD160 },
	{ 'u',   0,   0, "has160",    &opt.sum_flags, RHASH_HAS160 },
	{ 'u',   0,   0, "snefru128", &opt.sum_flags, RHASH_SNEFRU128 },
	{ 'u',   0,   0, "snefru256", &opt.sum_flags, RHASH_SNEFRU256 },
	{ 'u',   0,   0, "edonr256",  &opt.sum_flags, RHASH_EDONR256 },
	{ 'u',   0,   0, "edonr512",  &opt.sum_flags, RHASH_EDONR512 },
	{ 'u', 'L',   0, "ed2k-link", &opt.sum_flags, OPT_ED2K_LINK },
	/* output formats */
	{ 'u',   0,   0, "sfv",     &opt.fmt, FMT_SFV },
	{ 'u',   0,   0, "bsd",     &opt.fmt, FMT_BSD },
	{ 'u',   0,   0, "simple",  &opt.fmt, FMT_SIMPLE },
	{ 'u', 'm',   0, "magnet",  &opt.fmt, FMT_MAGNET },
	{ 'u',   0,   0, "uppercase", &opt.flags, OPT_UPPERCASE },
	{ 'u',   0,   0, "lowercase", &opt.flags, OPT_LOWERCASE },
	{ 's',   0,   0, "template",  &opt.template_file, 0 },
	{ 's', 'p',   0, "printf",  &opt.printf, 0 },
	/* other options */
	{ 'u', 'r', 'R', "recursive", &opt.flags, OPT_RECURSIVE },
	{ 'u', 'v',   0, "verbose", &opt.flags, OPT_VERBOSE },
	{ 'u',   0,   0, "gost-reverse", &opt.flags, OPT_GOST_REVERSE },
	{ 'u',   0,   0, "skip-ok", &opt.flags, OPT_SKIP_OK },
	{ 'u', 'i',   0, "ignore-case", &opt.flags, OPT_IGNORE_CASE },
	{ 'u',   0,   0, "percents", &opt.flags, OPT_PERCENTS },
	{ 'u',   0,   0, "speed",  &opt.flags, OPT_SPEED },
	{ 'u', 'e',   0, "embed-crc",  &opt.flags, OPT_EMBED_CRC },
	{ 's',   0,   0, "embed-crc-delimiter", &opt.embed_crc_delimiter, 0 },
	{ 'F',   0,   0, "path-separator", set_path_separator, 0 },
	{ 'P', 'o',   0, "output", &opt.output, 0 },
	{ 'P', 'l',   0, "log",    &opt.log,    0 },
	{ 'F', 'q',   0, "accept", crc_accept, 0 },
	{ 'F', 't',   0, "crc-accept", crc_accept, 1 },
	{ 'f',   0,   0, "video",  accept_video, 0 },
	{ 'f',   0,   0, "nya",  nya, 0 },
	{ 'F',   0,   0, "maxdepth", set_max_depth, 0 },
	{ 'u',   0,   0, "bt-private", &opt.flags, OPT_BT_PRIVATE },
	{ 'F',   0,   0, "bt-piece-length", set_bt_piece_length, 0 },
	{ 's',   0,   0, "bt-announce", &opt.bt_announce, 0 },
	{ 'u',   0,   0, "benchmark-raw", &opt.flags, OPT_BENCH_RAW },
	{ 'F',   0,   0, "openssl", openssl_flags, 0 },
#ifdef _WIN32 /* code pages */
	{ 'u',   0,   0, "utf8", &opt.flags, OPT_UTF8 },
	{ 'u',   0,   0, "ansi", &opt.flags, OPT_ANSI },
	{ 'u',   0,   0, "oem",  &opt.flags, OPT_OEM },
#endif
	{ 0,0,0,0,0,0 }
};

/**
 * Return non-zero if given option has a required parameter
 *
 * @param type the type of option to check
 */
static int is_param_required(char type)
{
	return (type == 's' || type == 'P' || type == 'F');
}

/**
 * Log a message and exit the program.
 *
 * @param msg the message to log
 */
static void die(const char* msg)
{
	log_msg(msg);
	rsh_exit(2);
}

/**
 * Log an error about unknown option and exit the program.
 *
 * @param option_name the name of the unknown option encountered
 */
static void fail_on_unknow_option(const char* option_name)
{
	log_msg(PROGRAM_NAME ": unknown option: %s", (option_name ? option_name : "?"));
	rsh_exit(2);
}

/* structure to store command line option information */
typedef struct parsed_option_t
{
	cmdline_opt_t *o;
	const char* name; /* the parsed option name */
	char buf[4];
	void* parameter;  /* option argument, if required */
} parsed_option_t;

/**
 * Process given command line option
 *
 * @param opts the sructure to store results of option processing
 * @param option option to process
 */
static void apply_option(options_t *opts, parsed_option_t* option)
{
	cmdline_opt_t* o = option->o;
	char type = o->type;
	char* value;

	if(type == 'u') {
		*(unsigned*)((char*)opts + ((char*)o->ptr - (char*)&opt)) |= o->param;
	}
	else if(type == 's' || type == 'F' || type == 'P')
	{
		/* option requires a parameter */
		if(!option->parameter) {
			log_msg(PROGRAM_NAME ": argument is required for option %s\n", option->name);
			rsh_exit(2);
		}

#ifdef _WIN32
		/* convert from UTF-16 if not a filepath */
		if(type != 'P') {
			value = w2c((wchar_t*)option->parameter);
			rsh_vector_add_ptr(opt.mem, value);
		} else 
#endif
		{
			value = (char*)option->parameter;
		}

		if(type == 'F') {
			/* call option parameter handler */
			( ( void(*)(options_t *, char*, unsigned) )o->ptr )(opts, value, o->param);
		} else {
			/* save the option parameter */
			*(char**)((char*)opts + ((char*)o->ptr - (char*)&opt)) = value;
		}
	} else if(o->type == 'f') {
		( ( void(*)(options_t *) )o->ptr )(opts); /* call option handler */
	} else if(o->type == 'p') {
		log_msg("%s", (char*)o->ptr);
		rsh_exit(0);
	}
}

/**
 * Search for config file.
 * 
 * @return the relative path to config file
 */
static const char* find_conf_file(void)
{
# define CONF_FILE_NAME "rhashrc"
	struct rsh_stat_struct st;
	char *dir1, *path;

#ifndef _WIN32 /* Linux/Unix part */
	/* first check for $HOME/.rhashrc file */
	if( (dir1 = getenv("HOME")) ) {
		path = make_path(dir1, ".rhashrc");
		if(rsh_stat(path, &st) >= 0) {
			rsh_vector_add_ptr(opt.mem, path);
			return (conf_opt.config_file = path);
		}
		free(path);
	}
	/* then check for global config */
	if(rsh_stat( (path = "/etc/" CONF_FILE_NAME), &st) >= 0) {
		return (conf_opt.config_file = path);
	}

#else /* _WIN32 */

	/* first check for the %APPDATA%\RHash\rhashrc config */
	if( (dir1 = getenv("APPDATA")) ) {
		dir1 = make_path(dir1, "RHash");
		rsh_vector_add_ptr(opt.mem, path = make_path(dir1, CONF_FILE_NAME));
		free(dir1);
		if(rsh_stat(path, &st) >= 0) {
			return (conf_opt.config_file = path);
		}
	}

	/* then check for %HOMEDRIVE%%HOMEPATH%\rhashrc */
	/* note that %USERPROFILE% is generally not a user home dir */
	if( (dir1 = getenv("HOMEDRIVE")) && (path = getenv("HOMEPATH"))) {
		dir1 = make_path(dir1, path);
		rsh_vector_add_ptr(opt.mem, path = make_path(dir1, CONF_FILE_NAME));
		free(dir1);
		if(rsh_stat(path, &st) >= 0) {
			return (conf_opt.config_file = path);
		}
	}
#endif /* _WIN32 */

	return (conf_opt.config_file = NULL); /* config file not found */
}

/**
 * Parse config file of the program.
 *
 * @return 0 on success, -1 on fail
 */
static int read_config(void)
{
#define LINE_BUF_SIZE 2048
	char buf[LINE_BUF_SIZE];
	FILE* fd;
	parsed_option_t option;
	int res;

	/* initialize conf_opt and opt structures */
	memset(&conf_opt, 0, sizeof(opt));
	conf_opt.find_max_depth = -1;

	if(!find_conf_file()) return 0;

	fd = fopen(conf_opt.config_file, "r");
	if(!fd) return -1;

	while(fgets(buf, LINE_BUF_SIZE, fd)) {
		size_t index;
		cmdline_opt_t* t;
		char* line = str_trim(buf);
		char  *name, *value;

		if(*line == 0 || IS_COMMENT(*line)) continue;

		/* search for '=' */
		index = strcspn(line, "=");
		if(line[index] == 0) {
			log_msg("%s: warning: can't parse line \"%s\"\n", conf_opt.config_file, line);
			continue;
		}
		line[index] = 0;
		name = str_trim(line);

		for(t = cmdline_opt; t->type; t++) {
			if(strcmp(name, t->long_name) == 0) {
				break;
			}
		}

		if(!t->type) {
			log_msg("%s: warning: unknown option \"%s\"\n", conf_opt.config_file, line);
			continue;
		}

		value = str_trim(line + index + 1);

		/* process a long option */
		if(is_param_required(t->type)) {
			rsh_vector_add_ptr(opt.mem, (value = rsh_strdup(value)));;
		} else {
			static const char* strings[] = {"1", "on", "yes", 0};
			const char** cmp;
			for(cmp = strings; *cmp && strcmp(value, *cmp); cmp++);
			if(*cmp == 0) continue;
		}

		option.name = name;
		option.parameter = value;
		option.o = t;
		apply_option(&conf_opt, &option);
	}
	res = fclose(fd);

#ifdef _WIN32
	if( (opt.flags & OPT_ENCODING) == 0 ) opt.flags |= (conf_opt.flags & OPT_ENCODING);
	if( (opt.flags & OPT_ENCODING) == 0 ) opt.flags |= OPT_UTF8;
#endif
	return (res == 0 ? 0 : -1);
}

#ifdef _WIN32
typedef wchar_t rhash_tchar;
#define RSH_T(str) L##str
#else
typedef char rhash_tchar;
#define RSH_T(str) str
#endif

/**
 * Find long option info, by it's name and retrive its parameter if required.
 * Error is reported for unknow options.
 *
 * @param option structure to recieve the parsed option info
 * @param parg pointer to a command line argument
 */
static void parse_long_option(parsed_option_t* option, rhash_tchar ***parg)
{
	size_t length;
	rhash_tchar* eq_sign;
	cmdline_opt_t *t;
	char* name;

#ifdef _WIN32
	rhash_tchar* wname = **parg; /* skip "--" */
	int fail = 0;
	assert((**parg)[0] == L'-' && (**parg)[1] == L'-');

	/* search for the '=' sign */
	length = ((eq_sign = wcschr(wname, L'=')) ? (size_t)(eq_sign - wname) : wcslen(wname));
	option->name = name = (char*)rsh_malloc(length + 1);
	rsh_vector_add_ptr(opt.mem, name);
	if(length < 30) {
		size_t i = 0;
		for(; i < length; i++) {
			if(((unsigned)wname[i]) <= 128) name[i] = (char)wname[i];
			else {
				fail = 1;
				break;
			}
		}
		name[i] = '\0';
		
		name += 2; /* skip  "--" */
		length -= 2;
	} else fail = 1;

	if(fail) fail_on_unknow_option(w2c(**parg));
#else
	option->name = **parg;
	name =  **parg + 2; /* skip "--" */
	length = ((eq_sign = strchr(name, '=')) ? (size_t)(eq_sign - name) : strlen(name));
	name[length] = '\0';
#endif
	/* search for the option by its name */
	for(t = cmdline_opt; t->type && (strncmp(name, t->long_name, length) != 0 ||
		strlen(t->long_name) != length); t++) {
	}
	if(!t->type) {
		fail_on_unknow_option(option->name); /* report error and exit */
	}

	option->o = t; /* store the option found */
	if(is_param_required(t->type)) {
		/* store parameter without a code page conversion */
		option->parameter = (eq_sign ? eq_sign + 1 : *(++(*parg)));
	}
}

struct parsed_cmd_line_t
{
	blocks_vector_t options; /* array of parsed options */
	char **argv;
	int n_files;
	rhash_tchar** files;
#ifdef _WIN32
	rhash_tchar** warg;
#endif
};

/**
 * Parse command line arguments.
 *
 * @param cmd_line structure to store parsed options data
 */
static void parse_cmdline_options(struct parsed_cmd_line_t* cmd_line)
{
	int argc;
	int n_files = 0, b_opt_end = 0;
	rhash_tchar** files;
	rhash_tchar **parg, **end_arg;
	parsed_option_t *next_opt;

#ifdef _WIN32
	parg = cmd_line->warg = CommandLineToArgvW(GetCommandLineW(), &argc);
	if( NULL == parg || argc < 1) {
		die("CommandLineToArgvW failed\n");
	}
#else
	parg = cmd_line->argv;
	for(argc = 0; parg[argc]; argc++); /* compute argc for files[] size */
#endif

	/* allocate array for files */
	files = (rhash_tchar**)rsh_malloc(argc * sizeof(rhash_tchar*));
	end_arg = parg + argc;

	/* loop by program arguments */
	for(parg++; parg < end_arg; parg++)
	{
		/* if argument is not an option */
		if((*parg)[0] != RSH_T('-') || (*parg)[1] == 0 || b_opt_end) {
			/* file encountered, note that '-' is interpreted as stdin */
			files[n_files++] = *parg;
			continue;
		}
		assert((*parg)[0] == RSH_T('-') && (*parg)[1] != 0);
		
		if((*parg)[1] == L'-' && (*parg)[2] == 0) {
			b_opt_end = 1; /* string "--" means end of options */
			continue;
		}

		/* check for "--" */
		if((*parg)[1] == RSH_T('-')) {
			cmdline_opt_t *t;

			/* allocate parsed_option */
			rsh_blocks_vector_add_empty(&cmd_line->options, 16, sizeof(parsed_option_t));
			next_opt = rsh_blocks_vector_get_item(&cmd_line->options, cmd_line->options.size - 1, 16, parsed_option_t);

			/* find the long option */
			parse_long_option(next_opt, &parg);
			t = next_opt->o;

			/* process encoding and -o/-l options early */
			if(
#ifdef _WIN32
			(t->type == 'u' && t->ptr == &opt.flags && (t->param & OPT_ENCODING) != 0) ||
#endif
				(t->type == 'P' && (t->ptr == &opt.output || t->ptr == &opt.log))) {
				apply_option(&opt, next_opt);
			}
		} else if((*parg)[1] != 0) {
			/* found '-'<some string> */
			rhash_tchar* ptr;
			
			/* parse short options. A string of several characters is interpreted
			 * as separate short options */
			for(ptr = *parg + 1; *ptr; ptr++) {
				cmdline_opt_t *t;
				char ch = (char)*ptr;

#ifdef _WIN32
				if(((unsigned)*ptr) >= 128) {
					ptr[1] = 0;
					fail_on_unknow_option(w2c(ptr));
				}
#endif
				/* allocate parsed_option */
				rsh_blocks_vector_add_empty(&cmd_line->options, 16, sizeof(parsed_option_t));
				next_opt = rsh_blocks_vector_get_item(&cmd_line->options, cmd_line->options.size - 1, 16, parsed_option_t);

				next_opt->buf[0] = '-', next_opt->buf[1] = ch, next_opt->buf[2] = '\0';
				next_opt->name = next_opt->buf;
				next_opt->parameter = NULL;
				
				/* search for the short option */
				for(t = cmdline_opt; t->type && ch != t->short1 && ch != t->short2; t++);
				if(!t->type) fail_on_unknow_option(next_opt->buf);
				next_opt->o = t;
				if(is_param_required(t->type)) {
					next_opt->parameter = (ptr[1] ? ptr + 1 : *(++parg));
					if(!next_opt->parameter) {
						/* note: need to check for parameter here, for early -o/-l options processing */
						log_msg(PROGRAM_NAME ": argument is required for option %s\n", next_opt->name);
						rsh_exit(2);
					}
				}

				/* process encoding and -o/-l options early */
				if(
#ifdef _WIN32
				(t->type == 'u' && t->ptr == &opt.flags && (t->param & OPT_ENCODING) != 0) ||
#endif
					(t->type == 'P' && (t->ptr == &opt.output || t->ptr == &opt.log))) {
					apply_option(&opt, next_opt);
				}
				if(next_opt->parameter) break;  /* a parameter ends the short options string */
			}
		}

	} /* for */

	cmd_line->n_files = n_files;
	cmd_line->files = files;
}

/**
 * Apply all parsed command line options: set binary flags, store strings,
 * and do complex options handling by calling callbacks.
 *
 * @param cmd_line the parsed options information
 */
static void apply_cmdline_options(struct parsed_cmd_line_t *cmd_line)
{
	int count = cmd_line->options.size;
	int i;
	for(i = 0; i < count; i++) {
		parsed_option_t* o = (parsed_option_t*)rsh_blocks_vector_get_ptr(
			&cmd_line->options, i, 16, sizeof(parsed_option_t));

		apply_option(&opt, o); /* process the option */
	}

	/* copy formating options from config if not specified at command line */
	if(!opt.printf && !opt.template_file && !opt.sum_flags && !opt.fmt) {
		opt.printf = conf_opt.printf;
		opt.template_file = conf_opt.template_file;
	}

	if(!opt.printf && !opt.template_file) {
		if(!opt.fmt) opt.fmt = conf_opt.fmt;
		if(!opt.sum_flags) opt.sum_flags = conf_opt.sum_flags;
	}

	if(!opt.mode)  opt.mode = conf_opt.mode;
	opt.flags |= conf_opt.flags; /* copy all non-sum options */

	if(opt.files_accept == 0)  opt.files_accept = conf_opt.files_accept;
	if(opt.crc_accept == 0)    opt.crc_accept = conf_opt.crc_accept;
	if(opt.embed_crc_delimiter == 0) opt.embed_crc_delimiter = conf_opt.embed_crc_delimiter;
	if(!opt.path_separator) opt.path_separator = conf_opt.path_separator;
	if(opt.find_max_depth < 0) opt.find_max_depth = conf_opt.find_max_depth;
	if(opt.flags & OPT_EMBED_CRC) opt.sum_flags |= RHASH_CRC32;
	if(opt.openssl_mask == 0) opt.openssl_mask = conf_opt.openssl_mask;

	/* set defaults */
	if(opt.embed_crc_delimiter == 0) opt.embed_crc_delimiter = " ";
}

/**
 * Try to detect hash sums options from program name.
 *
 * @param progName the program name
 */
static void set_default_sums_flags(const char* progName)
{
	char *buf;
	int res = 0;

	/* remove directory name from path */
	const char* p = strrchr(progName, '/');
	if(p) progName = p+1;
#ifdef _WIN32
	p = strrchr(progName, '\\');
	if(p) progName = p+1;
#endif

	/* convert progName to lowercase */
	buf = str_tolower(progName);

	if(strstr(buf, "crc32")) res |= RHASH_CRC32;
	if(strstr(buf, "md4"))   res |= RHASH_MD4;
	if(strstr(buf, "md5"))   res |= RHASH_MD5;
	if(strstr(buf, "sha1"))  res |= RHASH_SHA1;
	if(strstr(buf, "sha256")) res |= RHASH_SHA256;
	if(strstr(buf, "sha512")) res |= RHASH_SHA512;
	if(strstr(buf, "sha224")) res |= RHASH_SHA224;
	if(strstr(buf, "sha384")) res |= RHASH_SHA384;
	if(strstr(buf, "tiger")) res |= RHASH_TIGER;
	if(strstr(buf, "tth"))   res |= RHASH_TTH;
	if(strstr(buf, "btih"))  res |= RHASH_BTIH;
	if(strstr(buf, "aich"))  res |= RHASH_AICH;
	if(strstr(buf, "gost"))  res |= RHASH_GOST;
	if(strstr(buf, "gost-cryptopro"))  res |= RHASH_GOST_CRYPTOPRO;
	if(strstr(buf, "has160"))  res |= RHASH_HAS160;
	if(strstr(buf, "ripemd160"))  res |= RHASH_RIPEMD160;
	if(strstr(buf, "whirlpool"))  res |= RHASH_WHIRLPOOL;
	if(strstr(buf, "edonr256"))   res |= RHASH_EDONR256;
	if(strstr(buf, "edonr512"))   res |= RHASH_EDONR512;
	if(strstr(buf, "snefru256"))  res |= RHASH_SNEFRU128;
	if(strstr(buf, "snefru128"))  res |= RHASH_SNEFRU256;
	if(strstr(buf, "ed2k-link") || strstr(buf, "ed2k-hash")) res |= OPT_ED2K_LINK;
	else if(strstr(buf, "ed2k")) res |= RHASH_ED2K;

	if(strstr(buf, "sfv") && opt.fmt == 0) opt.fmt = FMT_SFV;
	if(strstr(buf, "magnet") && opt.fmt == 0) opt.fmt = FMT_MAGNET;

	free(buf);

	/* change program flags only if opt.sum_flags was not set */
	if(!opt.sum_flags) {
		opt.sum_flags = (res ? res : (opt.fmt == FMT_MAGNET ? RHASH_TTH | RHASH_ED2K | RHASH_AICH : RHASH_CRC32));
	}
}

/**
 * Destroy a parsed options object.
 *
 * @param o pointer to the options object to destroy.
 */
void options_destroy(struct options_t* o)
{
	file_mask_free(o->files_accept);
	file_mask_free(o->crc_accept);
	rsh_vector_free(o->cmd_vec);
	rsh_vector_free(o->mem);
}

/**
 * Check that options do not conflict with each other.
 * Also do some final options processing steps.
 */
static void make_final_options_checks(void)
{
	unsigned ff; /* formating flags */

	if((opt.flags & OPT_VERBOSE) && conf_opt.config_file) {
		/* note that the first log_msg call shall be made after setup_output() */
		log_msg("Config file: %s\n", (conf_opt.config_file ? conf_opt.config_file : "None"));
	}

	/* check that no more than one program mode specified */
	if(opt.mode & (opt.mode - 1)) {
		die(PROGRAM_NAME ": incompatible program modes\n");
	}

	ff = (opt.printf ? 1 : 0) | (opt.template_file ? 2 : 0) | (opt.fmt ? 4 : 0);
	if((opt.fmt & (opt.fmt - 1)) || (ff & (ff - 1))) {
		die(PROGRAM_NAME ": too many formating options\n");
	}

	if(opt.mode & MODE_TORRENT) opt.sum_flags |= RHASH_BTIH;

	if(!opt.crc_accept) opt.crc_accept = file_mask_new_from_list(".sfv");

	if(opt.openssl_mask) rhash_transmit(RMSG_SET_OPENSSL_MASK, 0, opt.openssl_mask, 0);
}

/**
 * Parse command line options.
 *
 * @param argv program arguments
 */
void read_options(char *argv[])
{
	struct parsed_cmd_line_t cmd_line;
#ifdef _WIN32
	int i;
	vector_t *expanded_cnames;
#endif

	memset(&opt, 0, sizeof(opt));
	opt.mem = rsh_vector_new_simple();
	opt.find_max_depth = -1;

	/* initialize cmd_line */
	memset(&cmd_line, 0, sizeof(cmd_line));
	rsh_blocks_vector_init(&cmd_line.options);
	cmd_line.argv = argv;

	/* parse command line and apply encoding options */
	parse_cmdline_options(&cmd_line);
	read_config();

	/* note: encoding and -o/-l options are already applied */
	IF_WINDOWS(setup_console());
	setup_output(); /* setup program output */

	apply_cmdline_options(&cmd_line);

	/* options were processed, so we don't need them anymore */
	rsh_blocks_vector_destroy(&cmd_line.options);
	
#ifdef _WIN32
	expanded_cnames = rsh_vector_new_simple();

	/* convert paths to internal encoding and expand wildcards. */
	for(i = 0; i < cmd_line.n_files; i++) {
		wchar_t* path = cmd_line.files[i];
		wchar_t* p = wcschr(path, L'\0') - 1;

		/* strip trailing '\','/' symbols (if not preceeded by ':') */
		for(; p > path && IS_PATH_SEPARATOR_W(*p) && p[-1] != L':'; p--) *p = 0;
		expand_wildcards(expanded_cnames, path);
	}
	/* the following NULL marks the end of files */
	if(cmd_line.n_files) rsh_vector_add_ptr(expanded_cnames, NULL);

	opt.cmd_vec = expanded_cnames;
	opt.files = (char**)expanded_cnames->array;
	free(cmd_line.files);
	LocalFree(cmd_line.warg);
#else
	opt.files = cmd_line.files;
	rsh_vector_add_ptr(opt.mem, opt.files);
#endif

	make_final_options_checks();

	set_default_sums_flags(argv[0]); /* detect default hashes from program name */
}
