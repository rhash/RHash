/* rhash_torrent.c - functions to make a torrent file.
 *
 * Copyright (c) 2013, Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE  INCLUDING ALL IMPLIED WARRANTIES OF  MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT,  OR CONSEQUENTIAL DAMAGES  OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT,  NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF  OR IN CONNECTION  WITH THE USE  OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* modifier for Windows DLL */
#if (defined(_WIN32) || defined(__CYGWIN__) ) && defined(RHASH_EXPORTS)
# define RHASH_API __declspec(dllexport)
#endif

#include "rhash_torrent.h"
#include "algorithms.h"
#include "torrent.h"
#include <assert.h>

/* obtain torrent context from rhash context */
#define BT_CTX(rctx) ((torrent_ctx*)(((rhash_context_ext*)rctx)->bt_ctx))

RHASH_API int rhash_torrent_add_file(rhash ctx, const char* filepath, unsigned long long filesize)
{
	if (!BT_CTX(ctx)) return 0;
	return bt_add_file(BT_CTX(ctx), filepath, filesize);
}

RHASH_API void rhash_torrent_set_options(rhash ctx, unsigned options)
{
	if (!BT_CTX(ctx)) return;
	bt_set_options(BT_CTX(ctx), options);
}

RHASH_API int rhash_torrent_add_announce(rhash ctx, const char* announce_url)
{
	if (!BT_CTX(ctx)) return 0;
	return bt_add_announce(BT_CTX(ctx), announce_url);
}

RHASH_API int rhash_torrent_set_program_name(rhash ctx, const char* name)
{
	if (!BT_CTX(ctx)) return 0;
	return bt_set_program_name(BT_CTX(ctx), name);
}

RHASH_API void rhash_torrent_set_piece_length(rhash ctx, size_t piece_length)
{
	if (!BT_CTX(ctx)) return;
	bt_set_piece_length(BT_CTX(ctx), piece_length);
}

RHASH_API size_t rhash_torrent_get_default_piece_length(unsigned long long total_size)
{
	return bt_default_piece_length(total_size);
}

RHASH_API const rhash_str* rhash_torrent_generate_content(rhash ctx)
{
	torrent_ctx* tc = BT_CTX(ctx);
	if (!tc || tc->error || !tc->content.str) return 0;
	return (rhash_str*)(&tc->content);
}
