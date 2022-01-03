/* parse_cmdline.c - parsing of command line options */

#include "parse_cmdline.h"
#include "file_mask.h"
#include "find_file.h"
#include "hash_print.h"
#include "output.h"
#include "rhash_main.h"
#include "win_utils.h"
#include "librhash/rhash.h"
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <windows.h> /* for CommandLineToArgvW(), GetCommandLineW(), ... */
#endif

typedef struct options_t options_t;
struct options_t conf_opt; /* config file parsed options */
struct options_t opt;      /* command line options */

static const char* get_full_program_version(void)
{
	static char version_buffer[64];
	sprintf(version_buffer, "%s v%s\n", PROGRAM_NAME, get_version_string());
	assert(strlen(version_buffer) < sizeof(version_buffer));
	return version_buffer;
}

static void print_version(void)
{
	rsh_fprintf(rhash_data.out, "%s", get_full_program_version());
	rsh_exit(0);
}

static void print_help_line(const char* option, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	rsh_fprintf(rhash_data.out, "%s", option);
	rsh_vfprintf(rhash_data.out, format, args);
	va_end(args);
}

/**
 * Print program help.
 */
static void print_help(void)
{
	const char* checksum_format;
	const char* digest_format;
	assert(rhash_data.out != NULL);

	/* print program version and usage */
	rsh_fprintf(rhash_data.out, _("%s\n"
		"Usage: %s [OPTION...] [FILE | -]...\n"
		"       %s --printf=<format string> [FILE | -]...\n\n"), get_full_program_version(), CMD_FILENAME, CMD_FILENAME);

	rsh_fprintf(rhash_data.out, _("Options:\n"));
	print_help_line("  -V, --version    ", _("Print program version and exit.\n"));
	print_help_line("  -h, --help       ", _("Print this help screen.\n"));
	/* TRANSLATORS: help screen line template for CRC32 and CRC32C */
	checksum_format = _("Calculate %s checksum.\n");
	/* TRANSLATORS: help screen line template for MD5, SHA1, e.t.c.\n" */
	digest_format = _("Calculate %s message digest.\n");
	print_help_line("  -C, --crc32      ", checksum_format, "CRC32");
	print_help_line("      --crc32c     ", checksum_format, "CRC32C");
	print_help_line("      --md4        ", digest_format, "MD4");
	print_help_line("  -M, --md5        ", digest_format, "MD5");
	print_help_line("  -H, --sha1       ", digest_format, "SHA1");
	print_help_line("      --sha224, --sha256, --sha384, --sha512 ", digest_format, "SHA2");
	print_help_line("      --sha3-224, --sha3-256, --sha3-384, --sha3-512 ", digest_format, "SHA3");
	print_help_line("  -T, --tth        ", digest_format, "TTH");
	print_help_line("      --btih       ", digest_format, "BitTorrent InfoHash");
	print_help_line("  -A, --aich       ", digest_format, "AICH");
	print_help_line("  -E, --ed2k       ", digest_format, "eDonkey");
	print_help_line("  -L, --ed2k-link  ", _("Calculate and print eDonkey link.\n"));
	print_help_line("      --tiger      ", digest_format, "Tiger");
	print_help_line("  -G, --gost12-256 ", digest_format, _("GOST R 34.11-2012, 256 bit"));
	print_help_line("      --gost12-512 ", digest_format, _("GOST R 34.11-2012, 512 bit"));
	/* TRANSLATORS: This hash function name should be translated to Russian only */
	print_help_line("      --gost94     ", digest_format, _("GOST R 34.11-94"));
	/* TRANSLATORS: This hash function name should be translated to Russian only */
	print_help_line("      --gost94-cryptopro ", digest_format, _("GOST R 34.11-94 CryptoPro"));
	print_help_line("      --ripemd160  ", digest_format, "RIPEMD-160");
	print_help_line("      --has160     ", digest_format, "HAS-160");
	print_help_line("      --blake2s,   --blake2b   ", digest_format, "BLAKE2S/BLAKE2B");
	print_help_line("      --edonr256,  --edonr512  ", digest_format, "EDON-R 256/512");
	print_help_line("      --snefru128, --snefru256 ", digest_format, "SNEFRU-128/256");
	print_help_line("  -a, --all        ", _("Calculate all supported hash functions.\n"));
	print_help_line("  -c, --check      ", _("Check hash files specified by command line.\n"));
	print_help_line("  -u, --update=<file> ", _("Update the specified hash file.\n"));
	print_help_line("  -e, --embed-crc  ", _("Rename files by inserting crc32 sum into name.\n"));
	print_help_line("  -k, --check-embedded  ", _("Verify files by crc32 sum embedded in their names.\n"));
	print_help_line("      --list-hashes  ", _("List the names of supported hash functions, one per line.\n"));
	print_help_line("  -B, --benchmark  ", _("Benchmark selected algorithm.\n"));
	print_help_line("  -v, --verbose    ", _("Be verbose.\n"));
	print_help_line("      --brief      ", _("Use brief form of hash file verification report.\n"));
	print_help_line("  -r, --recursive  ", _("Process directories recursively.\n"));
	print_help_line("      --file-list=<file> ", _("Process a list of files.\n"));
	print_help_line("  -m, --message=<text> ", _("Process the text message.\n"));
	print_help_line("      --skip-ok    ", _("Don't print OK messages for successfully verified files.\n"));
	print_help_line("      --ignore-missing ", _("Ignore missing files, while verifying a hash file.\n"));
	print_help_line("  -i, --ignore-case  ", _("Ignore case of filenames when updating hash files.\n"));
	print_help_line("  -P, --percents   ", _("Show percents, while calculating or verifying message digests.\n"));
	print_help_line("      --speed      ", _("Output per-file and total processing speed.\n"));
	print_help_line("      --maxdepth=<n> ", _("Descend at most <n> levels of directories.\n"));
	if (rhash_is_openssl_supported())
		print_help_line("      --openssl=<list> ", _("Specify hash functions to be calculated using OpenSSL.\n"));
	print_help_line("  -o, --output=<file> ", _("File to output calculation or checking results.\n"));
	print_help_line("  -l, --log=<file>    ", _("File to log errors and verbose information.\n"));
	print_help_line("      --sfv        ", _("Print message digests, using SFV format (default).\n"));
	print_help_line("      --bsd        ", _("Print message digests, using BSD-like format.\n"));
	print_help_line("      --simple     ", _("Print message digests, using simple format.\n"));
	print_help_line("      --hex        ", _("Print message digests in hexadecimal format.\n"));
	print_help_line("      --base32     ", _("Print message digests in Base32 format.\n"));
	print_help_line("  -b, --base64     ", _("Print message digests in Base64 format.\n"));

	print_help_line("  -g, --magnet     ", _("Print message digests as magnet links.\n"));
	print_help_line("      --torrent    ", _("Create torrent files.\n"));
#ifdef _WIN32
	print_help_line("      --utf8       ", _("Use UTF-8 encoding for output (Windows only).\n"));
	print_help_line("      --win        ", _("Use Windows codepage for output (Windows only).\n"));
	print_help_line("      --dos        ", _("Use DOS codepage for output (Windows only).\n"));
#endif
	print_help_line("      --template=<file> ", _("Load a printf-like template from the <file>\n"));
	print_help_line("  -p, --printf=<format string>  ", _("Format and print message digests.\n"));
	print_help_line("                   ", _("See the RHash manual for details.\n"));
	rsh_exit(0);
}

/**
 * Print the names of all supported hash algorithms to the console.
 */
static void list_hashes(void)
{
	unsigned id;
	for (id = 1; id < RHASH_ALL_HASHES; id <<= 1) {
		const char* hash_name = rhash_get_name(id);
		if (hash_name) rsh_fprintf(rhash_data.out, "%s\n", hash_name);
	}
	rsh_exit(0);
}

enum file_suffix_type {
	MASK_ACCEPT,
	MASK_EXCLUDE,
	MASK_CRC_ACCEPT
};

/**
 * Add a special file.
 *
 * @param o pointer to the options structure to update
 * @param path the path of the file
 * @param type the type of the option
 */
static void add_special_file(options_t* o, tstr_t path, unsigned file_mode)
{
	if (o->search_data) {
		file_search_add_file(o->search_data, path, file_mode);
		opt.has_files = 1;
	}
}

/**
 * Process --accept, --exclude and --crc-accept options.
 *
 * @param o pointer to the options structure to update
 * @param accept_string comma delimited string to parse
 * @param type the type of the option
 */
static void add_file_suffix(options_t* o, char* accept_string, unsigned type)
{
	file_mask_array** ptr = (type == MASK_ACCEPT ? &o->files_accept :
		type == MASK_EXCLUDE ? &o->files_exclude : &o->crc_accept);
	if (!*ptr) *ptr = file_mask_new();
	file_mask_add_list(*ptr, accept_string);
}

/**
 * Process --bt_announce option.
 *
 * @param o pointer to the options structure
 * @param announce_url the url to parse
 * @param unused a tottaly unused parameter
 */
static void bt_announce(options_t* o, char* announce_url, unsigned unused)
{
	(void)unused;
	/* skip empty string */
	if (!announce_url || !announce_url[0]) return;
	if (!o->bt_announce) o->bt_announce = rsh_vector_new_simple();
	rsh_vector_add_ptr(o->bt_announce, rsh_strdup(announce_url));
}

/**
 * Process an --openssl option.
 *
 * @param o pointer to the options structure to update
 * @param openssl_hashes comma delimited string with names of hash functions
 * @param type ignored
 */
static void openssl_flags(options_t* o, char* openssl_hashes, unsigned type)
{
	(void)type;
	if (rhash_is_openssl_supported())
	{
		rhash_uptr_t openssl_supported_hashes = rhash_get_openssl_supported_mask();
		char* cur;
		char* next;
		o->openssl_mask = 0x80000000; /* turn off using default mask */

		/* set the openssl_mask */
		for (cur = openssl_hashes; cur && *cur; cur = next) {
			print_hash_info* info = hash_info_table;
			unsigned bit;
			size_t length;
			next = strchr(cur, ',');
			length = (next != NULL ? (size_t)(next++ - cur) : strlen(cur));

			for (bit = 1; bit <= RHASH_ALL_HASHES; bit = bit << 1, info++) {
				if ( (bit & openssl_supported_hashes) &&
					memcmp(cur, info->short_name, length) == 0 &&
					info->short_name[length] == 0) {
						o->openssl_mask |= bit;
						break;
				}
			}
			if (bit > RHASH_ALL_HASHES) {
				cur[length] = '\0'; /* terminate wrong hash function name */
				log_warning(_("openssl option doesn't support '%s' hash function\n"), cur);
			}
		}
	}
	else
		log_warning(_("compiled without openssl support\n"));
}

/**
 * Process --video option.
 *
 * @param o pointer to the options structure to update
 */
static void accept_video(options_t* o)
{
	add_file_suffix(o, ".avi,.ogm,.mkv,.mp4,.mpeg,.mpg,.asf,.rm,.wmv,.vob", MASK_ACCEPT);
}

/**
 * Say nya! Keep secret! =)
 */
static void nya(void)
{
	rsh_fprintf(rhash_data.out, "  /\\__/\\\n (^ _ ^.) %s\n  (_uu__)\n",
		/* TRANSLATORS: Keep it secret ;) */
		_("Purrr..."));
	rsh_exit(0);
}

/**
 * Process on --maxdepth option.
 *
 * @param o pointer to the processed option
 * @param number the string containing the max-depth number
 * @param param unused parameter
 */
static void set_max_depth(options_t* o, char* number, unsigned param)
{
	(void)param;
	if (strspn(number, "0123456789") < strlen(number)) {
		log_error(_("maxdepth parameter is not a number: %s\n"), number);
		rsh_exit(2);
	}
	o->find_max_depth = atoi(number);
}

/**
 * Set the length of a BitTorrent file piece.
 *
 * @param o pointer to the processed option
 * @param number string containing the piece length number
 * @param param unused parameter
 */
static void set_bt_piece_length(options_t* o, char* number, unsigned param)
{
	(void)param;
	if (strspn(number, "0123456789") < strlen(number)) {
		log_error(_("bt-piece-length parameter is not a number: %s\n"), number);
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
static void set_path_separator(options_t* o, char* sep, unsigned param)
{
	(void)param;
	if ((*sep == '/' || *sep == '\\') && sep[1] == 0) {
		o->path_separator = *sep;
#if defined(_WIN32)
		/* MSYS environment changes '/' in command line to HOME, see http://www.mingw.org/wiki/FAQ */
	} else if (getenv("MSYSTEM") || getenv("TERM")) {
		log_warning(_("wrong path-separator, use '//' instead of '/' on MSYS\n"));
		o->path_separator = '/';
#endif
	} else {
		log_error(_("path-separator is neither '/' nor '\\': %s\n"), sep);
		rsh_exit(2);
	}
}

/**
 * Function pointer to store an option handler.
 */
typedef void(*opt_handler_t)(void);

/**
 * Information about a command line option.
 */
typedef struct cmdline_opt_t
{
	unsigned short type;  /* how to process the option, see option_type_t below */
	char  short1, short2; /* short option names */
	char* long_name;      /* long option name */
	opt_handler_t handler; /* option handler */
	void* ptr;            /* auxiliary pointer, e.g. to an opt field */
	unsigned param;       /* optional integer parameter */
} cmdline_opt_t;

enum option_type_t
{
	F_NEED_PARAM = 16, /* flag: option needs a parameter */
	F_OUTPUT_OPT = 32, /* flag: option changes program output */
	F_UFLG = 1, /* set a bit flag in a uint32_t field */
	F_UENC = F_UFLG | F_OUTPUT_OPT, /* an encoding changing option */
	F_CSTR = 2 | F_NEED_PARAM, /* store parameter as a C string */
	F_TSTR = 3 | F_NEED_PARAM, /* store parameter as a tstr_t */
	F_TOUT = 4 | F_NEED_PARAM | F_OUTPUT_OPT,
	F_VFNC = 5, /* just call a function */
	F_PFNC = 6 | F_NEED_PARAM, /* process option parameter by calling a handler */
	F_TFNC = 7 | F_NEED_PARAM, /* process option parameter by calling a handler */
	F_UFNC = 8 | F_NEED_PARAM, /* pass UTF-8 encoded parameter to the handler */
	F_PRNT = 9, /* print a constant C-string and exit */
};

#define is_param_required(option_type) ((option_type) & F_NEED_PARAM)
#define is_output_modifier(option_type) ((option_type) & F_OUTPUT_OPT)

/* supported program options */
cmdline_opt_t cmdline_opt[] =
{
	/* program modes */
	{ F_UFLG, 'c',   0, "check",     0, &opt.mode, MODE_CHECK },
	{ F_UFLG, 'k',   0, "check-embedded", 0, &opt.mode, MODE_CHECK_EMBEDDED },
	{ F_TSTR, 'u',   0, "update",    0, &opt.update_file, 0 },
	{ F_UFLG, 'B',   0, "benchmark", 0, &opt.mode, MODE_BENCHMARK },
	{ F_UFLG,   0,   0, "torrent",   0, &opt.mode, MODE_TORRENT },
	{ F_VFNC,   0,   0, "list-hashes", (opt_handler_t)list_hashes, 0, 0 },
	{ F_VFNC, 'h',   0, "help",        (opt_handler_t)print_help, 0, 0 },
	{ F_VFNC, 'V',   0, "version",     (opt_handler_t)print_version, 0, 0 },

	/* hash functions options */
	{ F_UFLG, 'a',   0, "all",      0, &opt.sum_flags, RHASH_ALL_HASHES },
	{ F_UFLG, 'C',   0, "crc32",    0, &opt.sum_flags, RHASH_CRC32 },
	{ F_UFLG,   0,   0, "crc32c",   0, &opt.sum_flags, RHASH_CRC32C },
	{ F_UFLG,   0,   0, "md4",      0, &opt.sum_flags, RHASH_MD4 },
	{ F_UFLG, 'M',   0, "md5",      0, &opt.sum_flags, RHASH_MD5 },
	{ F_UFLG, 'H',   0, "sha1",     0, &opt.sum_flags, RHASH_SHA1 },
	{ F_UFLG,   0,   0, "sha224",   0, &opt.sum_flags, RHASH_SHA224 },
	{ F_UFLG,   0,   0, "sha256",   0, &opt.sum_flags, RHASH_SHA256 },
	{ F_UFLG,   0,   0, "sha384",   0, &opt.sum_flags, RHASH_SHA384 },
	{ F_UFLG,   0,   0, "sha512",   0, &opt.sum_flags, RHASH_SHA512 },
	{ F_UFLG,   0,   0, "sha3-224", 0, &opt.sum_flags, RHASH_SHA3_224 },
	{ F_UFLG,   0,   0, "sha3-256", 0, &opt.sum_flags, RHASH_SHA3_256 },
	{ F_UFLG,   0,   0, "sha3-384", 0, &opt.sum_flags, RHASH_SHA3_384 },
	{ F_UFLG,   0,   0, "sha3-512", 0, &opt.sum_flags, RHASH_SHA3_512 },
	{ F_UFLG,   0,   0, "tiger",    0, &opt.sum_flags, RHASH_TIGER },
	{ F_UFLG, 'T',   0, "tth",      0, &opt.sum_flags, RHASH_TTH },
	{ F_UFLG,   0,   0, "btih",     0, &opt.sum_flags, RHASH_BTIH },
	{ F_UFLG, 'E',   0, "ed2k",     0, &opt.sum_flags, RHASH_ED2K },
	{ F_UFLG, 'A',   0, "aich",     0, &opt.sum_flags, RHASH_AICH },
	{ F_UFLG, 'G',   0, "gost12-256", 0, &opt.sum_flags, RHASH_GOST12_256 },
	{ F_UFLG,   0,   0, "gost12-512", 0, &opt.sum_flags, RHASH_GOST12_512 },
	{ F_UFLG,   0,   0, "gost94",   0, &opt.sum_flags, RHASH_GOST94 },
	{ F_UFLG,   0,   0, "gost94-cryptopro", 0, &opt.sum_flags, RHASH_GOST94_CRYPTOPRO },
	/* legacy: the following two gost options are left for compatibility */
	{ F_UFLG,   0,   0, "gost",   0, &opt.sum_flags, RHASH_GOST94 },
	{ F_UFLG,   0,   0, "gost-cryptopro", 0, &opt.sum_flags, RHASH_GOST94_CRYPTOPRO },
	{ F_UFLG, 'W',   0, "whirlpool", 0, &opt.sum_flags, RHASH_WHIRLPOOL },
	{ F_UFLG,   0,   0, "ripemd160", 0, &opt.sum_flags, RHASH_RIPEMD160 },
	{ F_UFLG,   0,   0, "has160",    0, &opt.sum_flags, RHASH_HAS160 },
	{ F_UFLG,   0,   0, "snefru128", 0, &opt.sum_flags, RHASH_SNEFRU128 },
	{ F_UFLG,   0,   0, "snefru256", 0, &opt.sum_flags, RHASH_SNEFRU256 },
	{ F_UFLG,   0,   0, "edonr256",  0, &opt.sum_flags, RHASH_EDONR256 },
	{ F_UFLG,   0,   0, "edonr512",  0, &opt.sum_flags, RHASH_EDONR512 },
	{ F_UFLG,   0,   0, "blake2s",   0, &opt.sum_flags, RHASH_BLAKE2S },
	{ F_UFLG,   0,   0, "blake2b",   0, &opt.sum_flags, RHASH_BLAKE2B },
	{ F_UFLG, 'L',   0, "ed2k-link", 0, &opt.sum_flags, OPT_ED2K_LINK },

	/* output formats */
	{ F_UFLG,   0,   0, "sfv",       0, &opt.fmt, FMT_SFV },
	{ F_UFLG,   0,   0, "bsd",       0, &opt.fmt, FMT_BSD },
	{ F_UFLG,   0,   0, "simple",    0, &opt.fmt, FMT_SIMPLE },
	{ F_UFLG, 'g',   0, "magnet",    0, &opt.fmt, FMT_MAGNET },
	{ F_UFLG,   0,   0, "uppercase", 0, &opt.flags, OPT_UPPERCASE },
	{ F_UFLG,   0,   0, "lowercase", 0, &opt.flags, OPT_LOWERCASE },
	{ F_TSTR,   0,   0, "template",  0, &opt.template_file, 0 },
	{ F_CSTR, 'p',   0, "printf",    0, &opt.printf_str, 0 },

	/* other options */
	{ F_UFLG, 'r', 'R', "recursive",     0, &opt.flags, OPT_RECURSIVE },
	{ F_TFNC, 'm',   0, "message",       (opt_handler_t)add_special_file, 0, FileIsData },
	{ F_TFNC,   0,   0, "file-list",     (opt_handler_t)add_special_file, 0, FileIsList },
	{ F_UFLG,   0,   0, "follow",        0, &opt.flags, OPT_FOLLOW },
	{ F_UFLG, 'v',   0, "verbose",       0, &opt.flags, OPT_VERBOSE },
	{ F_UFLG,   0,   0, "brief",         0, &opt.flags, OPT_BRIEF },
	{ F_UFLG,   0,   0, "gost-reverse",  0, &opt.flags, OPT_GOST_REVERSE },
	{ F_UFLG,   0,   0, "skip-ok",       0, &opt.flags, OPT_SKIP_OK },
	{ F_UFLG,   0,   0, "ignore-missing",   0, &opt.flags, OPT_IGNORE_MISSING },
	{ F_UFLG, 'i',   0, "ignore-case",   0, &opt.flags, OPT_IGNORE_CASE },
	{ F_UENC, 'P',   0, "percents",      0, &opt.flags, OPT_PERCENTS },
	{ F_UFLG,   0,   0, "speed",         0, &opt.flags, OPT_SPEED },
	{ F_UFLG, 'e',   0, "embed-crc",     0, &opt.flags, OPT_EMBED_CRC },
	{ F_CSTR,   0,   0, "embed-crc-delimiter", 0, &opt.embed_crc_delimiter, 0 },
	{ F_PFNC,   0,   0, "path-separator", (opt_handler_t)set_path_separator, 0, 0 },
	{ F_TOUT, 'o',   0, "output",        0, &opt.output, 0 },
	{ F_TOUT, 'l',   0, "log",           0, &opt.log,    0 },
	{ F_PFNC, 'q',   0, "accept",        (opt_handler_t)add_file_suffix, 0, MASK_ACCEPT },
	{ F_PFNC, 't',   0, "crc-accept",    (opt_handler_t)add_file_suffix, 0, MASK_CRC_ACCEPT },
	{ F_PFNC,   0,   0, "exclude",       (opt_handler_t)add_file_suffix, 0, MASK_EXCLUDE },
	{ F_VFNC,   0,   0, "video",         (opt_handler_t)accept_video, 0, 0 },
	{ F_VFNC,   0,   0, "nya",           (opt_handler_t)nya, 0, 0 },
	{ F_PFNC,   0,   0, "maxdepth",      (opt_handler_t)set_max_depth, 0, 0 },
	{ F_UFLG,   0,   0, "bt-private",    0, &opt.flags, OPT_BT_PRIVATE },
	{ F_UFLG,   0,   0, "bt-transmission", 0, &opt.flags, OPT_BT_TRANSMISSION },
	{ F_PFNC,   0,   0, "bt-piece-length", (opt_handler_t)set_bt_piece_length, 0, 0 },
	{ F_UFNC,   0,   0, "bt-announce",   (opt_handler_t)bt_announce, 0, 0 },
	{ F_TSTR,   0,   0, "bt-batch",      0, &opt.bt_batch_file, 0 },
	{ F_UFLG,   0,   0, "benchmark-raw", 0, &opt.flags, OPT_BENCH_RAW },
	{ F_UFLG,   0,   0, "no-detect-by-ext", 0, &opt.flags, OPT_NO_DETECT_BY_EXT },
	{ F_UFLG,   0,   0, "no-path-escaping", 0, &opt.flags, OPT_NO_PATH_ESCAPING },
	{ F_UFLG,   0,   0, "hex",           0, &opt.flags, OPT_HEX },
	{ F_UFLG,   0,   0, "base32",        0, &opt.flags, OPT_BASE32 },
	{ F_UFLG, 'b',   0, "base64",        0, &opt.flags, OPT_BASE64 },
	{ F_PFNC,   0,   0, "openssl",       (opt_handler_t)openssl_flags, 0, 0 },

#ifdef _WIN32 /* code pages (windows only) */
	{ F_UENC,   0,   0, "utf8", 0, &opt.flags, OPT_UTF8 },
	{ F_UENC,   0,   0, "win",  0, &opt.flags, OPT_ENC_WIN },
	{ F_UENC,   0,   0, "dos",  0, &opt.flags, OPT_ENC_DOS },
	/* legacy: the following two options are left for compatibility */
	{ F_UENC,   0,   0, "ansi", 0, &opt.flags, OPT_ENC_WIN },
	{ F_UENC,   0,   0, "oem",  0, &opt.flags, OPT_ENC_DOS },
#endif
	{ 0,0,0,0,0,0,0 }
};
cmdline_opt_t cmdline_file = { F_TFNC, 0, 0, "FILE", (opt_handler_t)add_special_file, 0, 0 };

/**
 * Log a message and exit the program.
 *
 * @param msg the message to log
 */
static void die(const char* msg)
{
	log_error(msg);
	rsh_exit(2);
}

/**
 * Log an error about unknown option and exit the program.
 *
 * @param option_name the name of the unknown option encountered
 */
static void fail_on_unknow_option(const char* option_name)
{
	log_error(_("unknown option: %s\n"), (option_name ? option_name : "?"));
	rsh_exit(2);
}

/* structure to store command line option information */
typedef struct parsed_option_t
{
	cmdline_opt_t* o;
	const char* name; /* the parsed option name */
	char buf[4];
	void* parameter;  /* option argument, if required */
} parsed_option_t;

/**
 * Process given command line option
 *
 * @param opts the structure to store results of option processing
 * @param option option to process
 */
static void apply_option(options_t* opts, parsed_option_t* option)
{
	cmdline_opt_t* o = option->o;
	unsigned short option_type = o->type;
	char* value = NULL;

	/* check if option requires a parameter */
	if (is_param_required(option_type)) {
		if (!option->parameter) {
			log_error(_("argument is required for option %s\n"), option->name);
			rsh_exit(2);
		}

#ifdef _WIN32
		if (option_type == F_TOUT || option_type == F_TFNC || option_type == F_TSTR) {
			/* leave the value in UTF-16 */
			value = (char*)rsh_wcsdup((wchar_t*)option->parameter);
		}
		else if (option_type == F_UFNC) {
			/* convert from UTF-16 to UTF-8 */
			value = convert_wcs_to_str((wchar_t*)option->parameter, ConvertToUtf8 | ConvertExact);
		} else {
			/* convert from UTF-16 */
			value = convert_wcs_to_str((wchar_t*)option->parameter, ConvertToPrimaryEncoding);
		}
		rsh_vector_add_ptr(opt.mem, value);
#else
		value = (char*)option->parameter;
#endif
	}

	/* process option, choosing the method by type */
	switch (option_type) {
	case F_UFLG:
	case F_UENC:
		*(unsigned*)((char*)opts + ((char*)o->ptr - (char*)&opt)) |= o->param;
		break;
	case F_CSTR:
	case F_TSTR:
	case F_TOUT:
		/* save the option parameter */
		*(char**)((char*)opts + ((char*)o->ptr - (char*)&opt)) = value;
		break;
	case F_PFNC:
	case F_TFNC:
	case F_UFNC:
		/* call option parameter handler */
		( (void(*)(options_t*, char*, unsigned))o->handler )(opts, value, o->param);
		break;
	case F_VFNC:
		( (void(*)(options_t*))o->handler )(opts); /* call option handler */
		break;
	case F_PRNT:
		log_msg("%s", (char*)o->ptr);
		rsh_exit(0);
		break;
	default:
		assert(0); /* impossible option type */
	}
}

#ifdef _WIN32
# define rsh_tgetenv(name) _wgetenv(name)
#else
# define rsh_tgetenv(name) getenv(name)
#endif
#define COUNTOF(array) (sizeof(array) / sizeof(*array))
enum ConfigLookupFlags
{
	ConfFlagNeedSplit = 8,
	ConfFlagNoVars = 16
};

/**
 * Check if a config file, specified by path subparts, is a regular file.
 * On success the resulting path is stored as rhash_data.config_file.
 *
 * @param path_parts subparts of the path
 * @param flags check flags
 * @return 1 if the file is regular, 0 otherwise
 */
static int try_config(ctpath_t path_parts[], unsigned flags)
{
	const size_t parts_count = flags & 3;
	tpath_t allocated = NULL;
	ctpath_t path = NULL;
	size_t i;
	for (i = 0; i < parts_count; i++) {
		ctpath_t sub_path = path_parts[i];
		if (sub_path[0] == RSH_T('$') && !(flags & ConfFlagNoVars)) {
			sub_path = rsh_tgetenv(sub_path + 1);
			if (!sub_path || !sub_path[0]) {
				free(allocated);
				return 0;
			}
#ifndef _WIN32
			/* check if the variable should be splitted */
			if (flags == (2 | ConfFlagNeedSplit) && i == 0) {
				tpath_t next;
				ctpath_t parts[2];
				parts[1] = path_parts[1];
				sub_path = allocated = rsh_strdup(sub_path);
				do {
					next = strchr(sub_path, ':');
					if (next)
						*(next++) = '\0';
					if (sub_path[0]) {
						parts[0] = sub_path;
						if (try_config(parts, COUNTOF(parts) | ConfFlagNoVars)) {
							free(allocated);
							return 1;
						}
					}
					sub_path = next;
				} while(sub_path);
				free(allocated);
				return 0;
			}
#endif
		}
		if (path) {
			tpath_t old_allocated = allocated;
			path = allocated = make_tpath(path, sub_path);
			free(old_allocated);
		} else {
			path = sub_path;
		}
	}
	assert(!rhash_data.config_file.real_path);
	{
		unsigned init_flags = FileInitRunFstat | (!allocated ? FileInitReusePath : 0);
		int res = file_init(&rhash_data.config_file, path, init_flags);
		free(allocated);
		if (res == 0 && FILE_ISREG(&rhash_data.config_file))
			return 1;
		file_cleanup(&rhash_data.config_file);
		return 0;
	}
}

/**
 * Search for config file.
 *
 * @return 1 if config file is found, 0 otherwise
 */
static int find_conf_file(void)
{
#ifndef SYSCONFDIR
# define SYSCONFDIR "/etc"
#endif

#ifndef _WIN32
	/* Linux/Unix part */
	static ctpath_t xdg_conf_home[2] = { "$XDG_CONFIG_HOME", "rhash/rhashrc" };
	static ctpath_t xdg_conf_default[2] = { "$HOME", ".config/rhash/rhashrc" };
	static ctpath_t xdg_conf_dirs[2] = { "$XDG_CONFIG_DIRS", "rhash/rhashrc" };
	static ctpath_t home_conf[2] = { "$HOME", ".rhashrc" };
	static ctpath_t sysconf_dir[1] = { SYSCONFDIR "/rhashrc" };

	return (try_config(xdg_conf_home, COUNTOF(xdg_conf_home)) ||
			try_config(xdg_conf_default, COUNTOF(xdg_conf_default)) ||
			try_config(xdg_conf_dirs, COUNTOF(xdg_conf_dirs) | ConfFlagNeedSplit) ||
			try_config(home_conf, COUNTOF(home_conf)) ||
			try_config(sysconf_dir, COUNTOF(sysconf_dir)));

#else /* _WIN32 */

	static ctpath_t app_data[2] = { L"$APPDATA", L"RHash\\rhashrc" };
	static ctpath_t home_conf[3] = { L"$HOMEDRIVE", L"$HOMEPATH", L"rhashrc" };

	if (try_config(app_data, COUNTOF(app_data)) || try_config(home_conf, COUNTOF(home_conf))) {
		return 1;
	} else {
		tpath_t prog_dir[2];
		prog_dir[0] = get_program_dir();
		prog_dir[1] = L"rhashrc";
		return try_config((ctpath_t*)prog_dir, COUNTOF(prog_dir));
	}

#endif /* _WIN32 */
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
	unsigned line_number = 0;
	int res;

	/* initialize conf_opt */
	memset(&conf_opt, 0, sizeof(opt));
	conf_opt.find_max_depth = -1;

	if (!find_conf_file()) return 0;
	assert(!!rhash_data.config_file.real_path);
	assert(FILE_ISREG(&rhash_data.config_file));

	fd = file_fopen(&rhash_data.config_file, FOpenRead);
	if (!fd) return -1;

	while (fgets(buf, LINE_BUF_SIZE, fd)) {
		size_t index;
		cmdline_opt_t* t;
		char* line = str_trim(buf);
		char* name;
		char* value;

		line_number++;
		if (*line == 0 || IS_COMMENT(*line))
			continue;

		/* search for '=' */
		index = strcspn(line, "=");
		if (line[index] == 0) {
			log_warning(_("%s:%u: can't parse line \"%s\"\n"),
				file_get_print_path(&rhash_data.config_file, FPathUtf8 | FPathNotNull),
				line_number, line);
			continue;
		}
		line[index] = 0;
		name = str_trim(line);

		for (t = cmdline_opt; t->type; t++) {
			if (strcmp(name, t->long_name) == 0) {
				break;
			}
		}

		if (!t->type) {
			log_warning(_("%s:%u: unknown option \"%s\"\n"),
				file_get_print_path(&rhash_data.config_file, FPathUtf8 | FPathNotNull),
				line_number, line);
			continue;
		}

		value = str_trim(line + index + 1);

		/* process a long option */
		if (is_param_required(t->type)) {
			rsh_vector_add_ptr(opt.mem, (value = rsh_strdup(value)));;
		} else {
			/* possible boolean values for a config file variable */
			static const char* strings[] = { "on", "yes", "true", 0 };
			const char** cmp;
			for (cmp = strings; *cmp && strcmp(value, *cmp); cmp++);
			if (*cmp == 0) continue;
		}

		option.name = name;
		option.parameter = value;
		option.o = t;
		apply_option(&conf_opt, &option);
	}
	res = fclose(fd);

#ifdef _WIN32
	if ( (opt.flags & OPT_ENCODING) == 0 )
		opt.flags |= (conf_opt.flags & OPT_ENCODING);
#endif
	return (res == 0 ? 0 : -1);
}

/**
 * Find long option info, by it's name and retrieve its parameter if required.
 * Error is reported for unknown options.
 *
 * @param option structure to receive the parsed option info
 * @param parg pointer to a command line argument
 */
static void parse_long_option(parsed_option_t* option, rsh_tchar*** parg)
{
	size_t length;
	rsh_tchar* eq_sign;
	cmdline_opt_t* t;
	char* name;

#ifdef _WIN32
	rsh_tchar* wname = **parg; /* "--<option name>" */
	int fail = 0;
	assert((**parg)[0] == L'-' && (**parg)[1] == L'-');

	/* search for the '=' sign */
	length = ((eq_sign = wcschr(wname, L'=')) ? (size_t)(eq_sign - wname) : wcslen(wname));
	option->name = name = (char*)rsh_malloc(length + 1);
	rsh_vector_add_ptr(opt.mem, name);
	if (length < 30) {
		size_t i = 0;
		for (; i < length; i++) {
			if (((unsigned)wname[i]) <= 128) name[i] = (char)wname[i];
			else {
				fail = 1;
				break;
			}
		}
		name[i] = '\0';

		name += 2; /* skip  "--" */
		length -= 2;
	} else fail = 1;

	if (fail)
		fail_on_unknow_option(convert_wcs_to_str(**parg, ConvertToUtf8));
#else
	option->name = **parg;
	name =  **parg + 2; /* skip "--" */
	length = ((eq_sign = strchr(name, '=')) ? (size_t)(eq_sign - name) : strlen(name));
	name[length] = '\0';
#endif
	/* search for the option by its name */
	for (t = cmdline_opt; t->type && (strncmp(name, t->long_name, length) != 0 ||
		strlen(t->long_name) != length); t++) {
	}
	if (!t->type) {
		fail_on_unknow_option(option->name); /* report error and exit */
	}

	option->o = t; /* store the option found */
	if (is_param_required(t->type)) {
		/* store parameter without a code page conversion */
		option->parameter = (eq_sign ? eq_sign + 1 : *(++(*parg)));
	}
}

/**
 * Parsed program command line.
 */
struct parsed_cmd_line_t
{
	blocks_vector_t options; /* array of parsed options */
	int  argc;
	char** argv;
#ifdef _WIN32
	rsh_tchar** warg; /* program arguments in Unicode */
#endif
};

/**
 * Allocate parsed option.
 *
 * @param cmd_line the command line to store the parsed option into
 * @return allocated parsed option
 */
static parsed_option_t* new_option(struct parsed_cmd_line_t* cmd_line)
{
	rsh_blocks_vector_add_empty(&cmd_line->options, 16, sizeof(parsed_option_t));
	return rsh_blocks_vector_get_item(&cmd_line->options, cmd_line->options.size - 1, 16, parsed_option_t);
}

/**
 * Parse command line arguments.
 *
 * @param cmd_line structure to store parsed options data
 */
static void parse_cmdline_options(struct parsed_cmd_line_t* cmd_line)
{
	int argc;
	int b_opt_end = 0;
	rsh_tchar** parg;
	rsh_tchar** end_arg;
	parsed_option_t* next_opt;

#ifdef _WIN32
	parg = cmd_line->warg = CommandLineToArgvW(GetCommandLineW(), &argc);
	if ( NULL == parg || argc < 1) {
		die(_("CommandLineToArgvW failed\n"));
	}
#else
	argc = cmd_line->argc;
	parg = cmd_line->argv;
#endif

	/* allocate array for files */
	end_arg = parg + argc;

	/* loop by program arguments */
	for (parg++; parg < end_arg; parg++) {
		/* if argument is not an option */
		if ((*parg)[0] != RSH_T('-') || (*parg)[1] == 0 || b_opt_end) {
			/* it's a file, note that '-' is interpreted as stdin */
			next_opt = new_option(cmd_line);
			next_opt->name = "";
			next_opt->o = &cmdline_file;
			next_opt->parameter = *parg;
		} else if ((*parg)[1] == L'-' && (*parg)[2] == 0) {
			b_opt_end = 1; /* string "--" means end of options */
			continue;
		} else if ((*parg)[1] == RSH_T('-')) {
			next_opt = new_option(cmd_line);
			parse_long_option(next_opt, &parg);

			/* process encoding and -o/-l options early */
			if (is_output_modifier(next_opt->o->type)) {
				apply_option(&opt, next_opt);
			}
		} else if ((*parg)[1] != 0) {
			/* found '-'<some string> */
			rsh_tchar* ptr;

			/* parse short options. A string of several characters is interpreted
			 * as separate short options */
			for (ptr = *parg + 1; *ptr; ptr++) {
				cmdline_opt_t* t;
				char ch = (char)*ptr;

#ifdef _WIN32
				if (((unsigned)*ptr) >= 128) {
					ptr[1] = 0;
					fail_on_unknow_option(convert_wcs_to_str(ptr, ConvertToUtf8));
				}
#endif
				next_opt = new_option(cmd_line);
				next_opt->buf[0] = '-', next_opt->buf[1] = ch, next_opt->buf[2] = '\0';
				next_opt->name = next_opt->buf;
				next_opt->parameter = NULL;

				/* search for the short option */
				for (t = cmdline_opt; t->type && ch != t->short1 && ch != t->short2; t++);
				if (!t->type) fail_on_unknow_option(next_opt->buf);
				next_opt->o = t;
				if (is_param_required(t->type)) {
					next_opt->parameter = (ptr[1] ? ptr + 1 : *(++parg));
					if (!next_opt->parameter) {
						/* note: need to check for parameter here, for early -o/-l options processing */
						log_error(_("argument is required for option %s\n"), next_opt->name);
						rsh_exit(2);
					}
				}

				/* process encoding and -o/-l options early */
				if (is_output_modifier(t->type)) {
					apply_option(&opt, next_opt);
				}
				if (next_opt->parameter) break;  /* a parameter ends the short options string */
			}
		}

	} /* for */
}

/**
 * Apply all parsed command line options: set binary flags, store strings,
 * and do complex options handling by calling callbacks.
 *
 * @param cmd_line the parsed options information
 */
static void apply_cmdline_options(struct parsed_cmd_line_t* cmd_line)
{
	size_t count = cmd_line->options.size;
	size_t i;
	for (i = 0; i < count; i++) {
		parsed_option_t* o = (parsed_option_t*)rsh_blocks_vector_get_ptr(
			&cmd_line->options, i, 16, sizeof(parsed_option_t));

		/* process the option, if it was not applied early */
		if (!is_output_modifier(o->o->type)) {
			apply_option(&opt, o);
		}
	}

	/* if no formatting options were specified at the command line */
	if (!opt.printf_str && !opt.template_file && !opt.sum_flags && !opt.fmt) {
		/* copy the format from config */
		opt.printf_str = conf_opt.printf_str;
		opt.template_file = conf_opt.template_file;
	}
	opt.fmt |= (opt.printf_str ? FMT_PRINTF : 0) | (opt.template_file ? FMT_FILE_TEMPLATE : 0);

	if (!(opt.fmt & FMT_PRINTF_MASK)) {
		if (!opt.fmt) opt.fmt = conf_opt.fmt;
		if (!opt.sum_flags) opt.sum_flags = conf_opt.sum_flags;
	}

	if (!opt.mode && !opt.update_file) {
		opt.mode = conf_opt.mode;
		opt.update_file = conf_opt.update_file;
	}
	if (opt.update_file)
		opt.mode |= MODE_UPDATE;

	if (!(opt.flags & OPT_FMT_MODIFIERS))
		opt.flags |= conf_opt.flags & OPT_FMT_MODIFIERS;
	opt.flags |= conf_opt.flags & ~OPT_FMT_MODIFIERS; /* copy the rest of options */

	if (opt.files_accept == 0)  {
		opt.files_accept = conf_opt.files_accept;
		conf_opt.files_accept = 0;
	}
	if (opt.files_exclude == 0)  {
		opt.files_exclude = conf_opt.files_exclude;
		conf_opt.files_exclude = 0;
	}
	if (opt.crc_accept == 0) {
		opt.crc_accept = conf_opt.crc_accept;
		conf_opt.crc_accept = 0;
	}
	if (opt.bt_announce == 0) {
		opt.bt_announce = conf_opt.bt_announce;
		conf_opt.bt_announce = 0;
	}

	if (opt.embed_crc_delimiter == 0) opt.embed_crc_delimiter = conf_opt.embed_crc_delimiter;
	if (!opt.path_separator) opt.path_separator = conf_opt.path_separator;
	if (opt.flags & OPT_EMBED_CRC) opt.sum_flags |= RHASH_CRC32;
	if (opt.openssl_mask == 0) opt.openssl_mask = conf_opt.openssl_mask;
	if (opt.find_max_depth < 0) opt.find_max_depth = conf_opt.find_max_depth;
	if (!(opt.flags & OPT_RECURSIVE)) opt.find_max_depth = 0;
	opt.search_data->max_depth = opt.find_max_depth;

	/* set defaults */
	if (opt.embed_crc_delimiter == 0) opt.embed_crc_delimiter = " ";
}

/**
 * Try to detect hash functions options from program name.
 *
 * @param progName the program name
 */
static void set_default_sums_flags(const char* progName)
{
	const char* p;
	char* buf;

	/* do nothing if hash functions are already selected by command line or config */
	if (opt.sum_flags != 0)
		return;

	/* remove directory name from the path */
	p = strrchr(progName, '/');
	if (p) progName = p + 1;
#ifdef _WIN32
	p = strrchr(progName, '\\');
	if (p) progName = p + 1;
#endif

	/* convert the progName to lowercase */
	buf = str_tolower(progName);

	if (strstr(buf, "sfv") && opt.fmt == 0) opt.fmt = FMT_SFV;
	if (strstr(buf, "bsd") && opt.fmt == 0) opt.fmt = FMT_BSD;
	if (strstr(buf, "magnet") && opt.fmt == 0) opt.fmt = FMT_MAGNET;

	if (strstr(buf, "crc32c")) opt.sum_flags |= RHASH_CRC32C;
	else if (strstr(buf, "crc32")) opt.sum_flags |= RHASH_CRC32;
	if (strstr(buf, "md4"))   opt.sum_flags |= RHASH_MD4;
	if (strstr(buf, "md5"))   opt.sum_flags |= RHASH_MD5;
	if (strstr(buf, "sha1"))  opt.sum_flags |= RHASH_SHA1;
	if (strstr(buf, "sha256")) opt.sum_flags |= RHASH_SHA256;
	if (strstr(buf, "sha512")) opt.sum_flags |= RHASH_SHA512;
	if (strstr(buf, "sha224")) opt.sum_flags |= RHASH_SHA224;
	if (strstr(buf, "sha384")) opt.sum_flags |= RHASH_SHA384;
	if (strstr(buf, "sha3-256")) opt.sum_flags |= RHASH_SHA3_256;
	if (strstr(buf, "sha3-512")) opt.sum_flags |= RHASH_SHA3_512;
	if (strstr(buf, "sha3-224")) opt.sum_flags |= RHASH_SHA3_224;
	if (strstr(buf, "sha3-384")) opt.sum_flags |= RHASH_SHA3_384;
	if (strstr(buf, "tiger")) opt.sum_flags |= RHASH_TIGER;
	if (strstr(buf, "tth"))   opt.sum_flags |= RHASH_TTH;
	if (strstr(buf, "btih"))  opt.sum_flags |= RHASH_BTIH;
	if (strstr(buf, "aich"))  opt.sum_flags |= RHASH_AICH;
	if (strstr(buf, "gost12-256"))  opt.sum_flags |= RHASH_GOST12_256;
	if (strstr(buf, "gost12-512"))  opt.sum_flags |= RHASH_GOST12_512;
	if (strstr(buf, "gost94-cryptopro"))  opt.sum_flags |= RHASH_GOST94_CRYPTOPRO;
	else if (strstr(buf, "gost94"))  opt.sum_flags |= RHASH_GOST94;
	if (strstr(buf, "has160"))  opt.sum_flags |= RHASH_HAS160;
	if (strstr(buf, "ripemd160") || strstr(buf, "rmd160"))  opt.sum_flags |= RHASH_RIPEMD160;
	if (strstr(buf, "whirlpool")) opt.sum_flags |= RHASH_WHIRLPOOL;
	if (strstr(buf, "edonr256"))  opt.sum_flags |= RHASH_EDONR256;
	if (strstr(buf, "edonr512"))  opt.sum_flags |= RHASH_EDONR512;
	if (strstr(buf, "blake2s"))   opt.sum_flags |= RHASH_BLAKE2S;
	if (strstr(buf, "blake2b"))   opt.sum_flags |= RHASH_BLAKE2B;
	if (strstr(buf, "snefru256")) opt.sum_flags |= RHASH_SNEFRU128;
	if (strstr(buf, "snefru128")) opt.sum_flags |= RHASH_SNEFRU256;
	if (strstr(buf, "ed2k-link")) opt.sum_flags |= OPT_ED2K_LINK;
	else if (strstr(buf, "ed2k")) opt.sum_flags |= RHASH_ED2K;

	if (!opt.sum_flags && opt.fmt == FMT_MAGNET)
		opt.sum_flags = RHASH_TTH | RHASH_ED2K | RHASH_AICH;

	free(buf);
}

/**
 * Destroy a parsed options object.
 *
 * @param o pointer to the options object to destroy
 */
void options_destroy(struct options_t* o)
{
	file_mask_free(o->files_accept);
	file_mask_free(o->files_exclude);
	file_mask_free(o->crc_accept);
	rsh_vector_free(o->bt_announce);
	rsh_vector_free(o->mem);
	file_search_data_free(o->search_data);
}

enum {
	ChkMode,
	ChkFmt
};

/**
 * Ensure that the specified bit_mask has only one bit set.
 * Report error and exit the program on fail.
 *
 * @param what what to check
 * @param bit_mask the bit_mask to check
 */
static void check_compatibility(int what, unsigned bit_mask)
{
	if ((bit_mask & (bit_mask - 1)) == 0)
		return;
	die(what == ChkMode ?
		_("incompatible program modes\n") :
		_("incompatible formatting options\n"));
}

/**
 * Check that options do not conflict with each other.
 * Also make some final options processing steps.
 */
static void make_final_options_checks(void)
{
	if ((opt.flags & OPT_VERBOSE) && !!rhash_data.config_file.real_path) {
		/* note that the first log_msg call shall be made after setup_output() */
		log_msg_file_t(_("Config file: %s\n"), &rhash_data.config_file);
	}

	if (opt.bt_batch_file)
		opt.mode |= MODE_TORRENT;
	else if (!opt.mode)
		opt.mode = MODE_DEFAULT;
	if (IS_MODE(MODE_TORRENT))
		opt.sum_flags |= RHASH_BTIH;

	/* check options compatibility for program mode and output format */
	if (opt.mode & ~(MODE_CHECK | MODE_UPDATE))
		check_compatibility(ChkMode, opt.mode);
	check_compatibility(ChkFmt, opt.fmt);
	check_compatibility(ChkFmt, (opt.flags & OPT_FMT_MODIFIERS) | (opt.fmt & FMT_PRINTF_MASK));

	if (!opt.crc_accept)
		opt.crc_accept = file_mask_new_from_list(".sfv");
	if (opt.openssl_mask)
		rhash_set_openssl_mask(opt.openssl_mask);
}

static struct parsed_cmd_line_t cmd_line;

static void cmd_line_destroy(void)
{
	rsh_blocks_vector_destroy(&cmd_line.options);
#ifdef _WIN32
	LocalFree(cmd_line.warg);
#endif
}

/**
 * Parse command line options.
 *
 * @param argv program arguments
 */
void read_options(int argc, char* argv[])
{
	opt.mem = rsh_vector_new_simple();
	opt.search_data = file_search_data_new();
	opt.find_max_depth = -1;

	/* initialize cmd_line */
	memset(&cmd_line, 0, sizeof(cmd_line));
	rsh_blocks_vector_init(&cmd_line.options);
	cmd_line.argv = argv;
	cmd_line.argc = argc;
	rsh_install_exit_handler(cmd_line_destroy);

	/* parse command line and apply encoding options */
	parse_cmdline_options(&cmd_line);
	read_config();
	errno = 0;

	/* setup the program output */
	IF_WINDOWS(setup_console());
	setup_output();

	apply_cmdline_options(&cmd_line); /* process the rest of command line options */
	options_destroy(&conf_opt);

	/* options were processed, so we don't need them any more */
	rsh_remove_exit_handler();
	cmd_line_destroy();

	make_final_options_checks();
	set_default_sums_flags(argv[0]); /* detect default hash functions from program name */
}
