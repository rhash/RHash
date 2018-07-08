/* parse_cmdline.h */
#ifndef PARSE_CMD_LINE_H
#define PARSE_CMD_LINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The official name of the program */
#define PROGRAM_NAME "RHash"
#define CMD_FILENAME "rhash"

#ifdef _WIN32
typedef wchar_t opt_tchar;
#else
typedef char opt_tchar;
#endif

/**
 * Options bit flags and constants.
 */
enum {
	OPT_ED2K_LINK = 0x10000000,

	/* program modes */
	MODE_CHECK     = 0x1,
	MODE_CHECK_EMBEDDED = 0x2,
	MODE_UPDATE    = 0x4,
	MODE_BENCHMARK = 0x8,
	MODE_TORRENT   = 0x10,

	/* misc options */
	OPT_EMBED_CRC  = 0x20,
	OPT_RECURSIVE  = 0x40,
	OPT_FOLLOW     = 0x80,
	OPT_SKIP_OK    = 0x100,
	OPT_IGNORE_CASE = 0x200,
	OPT_VERBOSE    = 0x400,
	OPT_PERCENTS   = 0x800,
	OPT_SPEED      = 0x1000,
	OPT_BT_PRIVATE = 0x2000,
	OPT_UPPERCASE  = 0x4000,
	OPT_LOWERCASE  = 0x8000,
	OPT_GOST_REVERSE = 0x10000,
	OPT_BENCH_RAW  = 0x20000,

#ifdef _WIN32
	OPT_UTF8 = 0x10000000,
	OPT_ANSI = 0x20000000,
	OPT_OEM  = 0x40000000,
	OPT_ENCODING = OPT_UTF8|OPT_ANSI|OPT_OEM,
#endif

	FMT_BSD     = 1,
	FMT_SFV     = 2,
	FMT_SIMPLE  = 4,
	FMT_MAGNET  = 8,
	OPT_FORMAT_MASK = FMT_BSD|FMT_SFV|FMT_SIMPLE|FMT_MAGNET
};

struct vector_t;

/**
 * Parsed program options.
 */
struct options_t
{
	unsigned flags;      /* program options */
	unsigned sum_flags;  /* flags to specify what sums will be calculated */
	unsigned fmt;        /* flags to specify output format to use */
	unsigned mode;       /* flags to specify program mode */
	unsigned openssl_mask;  /* bit-mask for enabled OpenSSL hash functions */
	const char* config_file; /* config file path */
	char* printf_str;        /* printf-like format */
	opt_tchar* template_file; /* printf-like template file path */
	opt_tchar* output;       /* file to output calculation or checking results to */
	opt_tchar* log;          /* file to log percents and other info to */
	char* embed_crc_delimiter;
	char  path_separator;
	int   find_max_depth;
	struct vector_t *files_accept; /* suffixes of files to process */
	struct vector_t *files_exclude; /* suffixes of files to exclude from processing */
	struct vector_t *crc_accept;   /* suffixes of crc files to verify or update */
	struct vector_t * bt_announce; /* BitTorrent announce URL */
	size_t bt_piece_length; /* BitTorrent piece length */
	opt_tchar*  bt_batch_file;   /* path to save a batch torrent to */

	char** argv;
	int has_files; /* flag: command line contain files */
	struct file_search_data* search_data; /* files obtained from the command line */
	struct vector_t *mem; /* allocated memory blocks that must be freed on exit */
};
extern struct options_t opt;

void read_options(int argc, char *argv[]);
void options_destroy(struct options_t*);

#ifdef _WIN32
int detect_encoding(wchar_t** wargv, int nArg);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* PARSE_CMD_LINE_H */
