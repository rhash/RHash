/* rhash_torrent.h */
#ifndef RHASH_TORRENT_H
#define RHASH_TORRENT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RHASH_API
/* modifier for LibRHash functions */
# define RHASH_API
#endif

#ifndef LIBRHASH_RHASH_CTX_DEFINED
#define LIBRHASH_RHASH_CTX_DEFINED
/**
 * Hashing context.
 */
typedef struct rhash_context* rhash;
#endif /* LIBRHASH_RHASH_CTX_DEFINED */

/**
 * Binary string with length.
 */
typedef struct rhash_str
{
	char* str;
	size_t length;
} rhash_str;

/* possible torrent options */

/**
 * Torrent option: generate private BitTorrent.
 */
#define RHASH_TORRENT_OPT_PRIVATE 1
/**
 * Torrent option: calculate infohash without torrent file body.
 */
#define RHASH_TORRENT_OPT_INFOHASH_ONLY 2
/**
 * Torrent option: generate transmiision-compatible torrent file.
 */
#define RHASH_TORRENT_OPT_TRANSMISSION 4

/* torrent functions */

/**
 * Add a file info into the batch of files of given torrent.
 *
 * @param ctx rhash context
 * @param filepath file path
 * @param filesize file size
 * @return non-zero on success, zero on fail
 */
RHASH_API int  rhash_torrent_add_file(rhash ctx, const char* filepath, unsigned long long filesize);

/**
 * Set the torrent algorithm options.
 *
 * @param ctx rhash context
 * @param options the options to set
 */
RHASH_API void rhash_torrent_set_options(rhash ctx, unsigned options);

/**
 * Add an torrent announcement-URL for storing into torrent file.
 *
 * @param ctx rhash context
 * @param announce_url the announcement-URL
 * @return non-zero on success, zero on error
 */
RHASH_API int  rhash_torrent_add_announce(rhash ctx, const char* announce_url);

/**
 * Set optional name of the program generating the torrent
 * for storing into torrent file.
 *
 * @param ctx rhash context
 * @param name the program name
 * @return non-zero on success, zero on error
 */
RHASH_API int  rhash_torrent_set_program_name(rhash ctx, const char* name);

/**
 * Set length of a file piece.
 *
 * @param ctx rhash context
 * @param piece_length the piece length in bytes
 */
RHASH_API void rhash_torrent_set_piece_length(rhash ctx, size_t piece_length);

/**
 * Calculate, using uTorrent algorithm, the default torrent piece length
 * for a given torrent batch size.
 *
 * @param total_size the total size of files included into a torrent file
 * @return piece length for the torrent file
 */
RHASH_API size_t rhash_torrent_get_default_piece_length(unsigned long long total_size);

/**
 * Set a torrent batch size (the total size of files included into this torrent).
 *
 * @param ctx rhash context
 * @param total_size total size of files included into the torrent file
 */
RHASH_API void rhash_torrent_set_batch_size(rhash ctx, unsigned long long total_size);

/**
 * Get the content of the generated torrent file.
 *
 * @param ctx rhash context
 * @return binary string with the torrent file content on success, NULL on fail
 */
RHASH_API const rhash_str* rhash_torrent_generate_content(rhash ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* RHASH_TORRENT_H */
