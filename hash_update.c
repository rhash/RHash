/* hash_update.c - functions to update a hash file */

#include "hash_update.h"
#include "calc_sums.h"
#include "file_mask.h"
#include "file_set.h"
#include "hash_print.h"
#include "output.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "win_utils.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
# include <dirent.h>
#endif

typedef struct update_ctx
{
	FILE* fd;
	char* cut_dir_path;
	file_t file;
	file_set* crc_entries;
	unsigned bit_flags;
} update_ctx;

enum HashFileExtraBits
{
	HashFileErrorOcurred = 0x100
};

/* define some internal functions, implemented later in this file */
static int fix_sfv_header(file_t* file);
static int open_and_prepare_hash_file(struct update_ctx* ctx);

/**
 * Construct updated hash file context.
 * In a case of fail, the error will be logged.
 *
 * @param update_file the hash file to update
 * @return constructed update context on success, NULL on fail
 */
struct update_ctx* update_ctx_new(file_t* update_file)
{
	struct update_ctx* ctx;
	file_set* crc_entries = file_set_new();
	int update_flags = load_updated_hash_file(update_file, crc_entries);
	if (update_flags < 0) {
		file_set_free(crc_entries);
		return NULL;
	}
	file_set_sort(crc_entries);
	opt.mode &= ~MODE_CHECK;

	ctx = (update_ctx*)rsh_malloc(sizeof(update_ctx));
	memset(ctx, 0, sizeof(*ctx));
	file_clone(&(ctx->file), update_file);
	ctx->crc_entries = crc_entries;
	ctx->bit_flags = (unsigned)update_flags;
	return ctx;
}

/**
 * Add message digests of the specified file to the updated hash file,
 * if the path of the first file is not yet present in the second.
 * In a case of fail, the error will be logged.
 *
 * @param ctx the update context for updated hash file
 * @param file the file to add
 * @return 0 on success, -1 on fail, -2 on fatal error
 */
int update_ctx_update(struct update_ctx* ctx, file_t* hash_file)
{
	int res;
	if ((ctx->bit_flags & HashFileErrorOcurred) != 0)
		return -1;

	/* skip files already present in the hash file */
	if (file_set_exist(ctx->crc_entries, file_get_print_path(hash_file, FPathUtf8)))
		return 0;

	if (!ctx->fd && open_and_prepare_hash_file(ctx) < 0) {
		log_error_file_t(&ctx->file);
		ctx->bit_flags |= HashFileErrorOcurred;
		return -2;
	}

	/* print message digests to the hash file */
	res = calculate_and_print_sums(ctx->fd, &ctx->file, hash_file);
	if (res < 0)
		ctx->bit_flags |= HashFileErrorOcurred;
	return res;
}

/**
 * Destroy update context.
 *
 * @param ctx the update context to cleanup
 * @return 0 on success, -1 on fail
 */
int update_ctx_free(struct update_ctx* ctx)
{
	int res = 0;
	if (!ctx)
		return 0;
	free(ctx->cut_dir_path);
	file_set_free(ctx->crc_entries);
	if (ctx->fd) {
		if (fclose(ctx->fd) < 0) {
			log_error_file_t(&ctx->file);
			res = -1;
		} else if (!!(ctx->bit_flags & HashFileErrorOcurred)) {
			res = -1;
		} else if (!rhash_data.stop_flags) {
			if (rhash_data.is_sfv)
				res = fix_sfv_header(&ctx->file); /* finalize the hash file */
			if (res == 0)
				log_msg_file_t(_("Updated: %s\n"), &ctx->file);
		}
	}
	file_cleanup(&ctx->file);
	free(ctx);
	return res;
}

/**
 * Open the updated hash file for appending. Add SFV header, if required.
 *
 * @param ctx the update context for updated hash file
 * @return 0 on success, -1 on fail with error code stored in errno
 */
static int open_and_prepare_hash_file(struct update_ctx* ctx)
{
	int open_mode = (ctx->bit_flags & HashFileExist ? FOpenRW : FOpenWrite) | FOpenBin;
	assert(!ctx->fd);
	/* open the hash file for reading/writing or create it */
	ctx->fd = file_fopen(&ctx->file, open_mode);
	if (!ctx->fd)
		return -1;
	if (!(ctx->bit_flags & HashFileIsEmpty)) {
		int ch;
		/* read the last character of the file to check if it is EOL */
		if (fseek(ctx->fd, -1, SEEK_END) != 0)
			return -1;
		ch = fgetc(ctx->fd);
		if (ch < 0 && ferror(ctx->fd))
			return -1;
		/* writing doesn't work without seeking */
		if (fseek(ctx->fd, 0, SEEK_END) != 0)
			return -1;
		/* write EOL, if it isn't present */
		if (ch != '\n' && ch != '\r') {
			if (rsh_fprintf(ctx->fd, "\n") < 0)
				return -1;
		}
	} else {
		/* skip BOM, if present */
		if ((ctx->bit_flags & HashFileHasBom) && fseek(ctx->fd, 0, SEEK_END) != 0)
			return -1;
		/* SFV banner will be printed only in SFV mode and only for empty hash files */
		if (rhash_data.is_sfv)
			return print_sfv_banner(ctx->fd);
	}
	return 0;
}

/**
 * Move all SFV header lines (i.e. all lines starting with a semicolon)
 * from the end of updated file to its head.
 * In a case of fail, the error will be logged.
 *
 * @param file the hash file requiring fixing of its SFV header
 * @return 0 on success, -1 on error
 */
static int fix_sfv_header(file_t* file)
{
	FILE* in;
	FILE* out;
	char buf[2048];
	file_t new_file;
	int result = 0;
	int is_comment;
	int print_comments;
	/* open the hash file for reading */
	in = file_fopen(file, FOpenRead);
	if (!in) {
		log_error_file_t(file);
		return -1;
	}
	/* open a temporary file for writing */
	file_modify_path(&new_file, file, ".new", FModifyAppendSuffix);
	out = file_fopen(&new_file, FOpenWrite);
	if (!out) {
		log_error_file_t(&new_file);
		file_cleanup(&new_file);
		fclose(in);
		return -1;
	}
	/* The first pass, prints commented lines to the destination file,
	 * and the second pass, prints all other lines */
	for (print_comments = 1;; print_comments = 0) {
		while (fgets(buf, 2048, in)) {
			char* line = buf;
			/* skip BOM, unless it is on the first line */
			if (STARTS_WITH_UTF8_BOM(line)) {
				is_comment = (line[3] == ';');
				if (ftell(out) != 0)
					line += 3;
			} else
				is_comment = (line[0] == ';');
			if (is_comment == print_comments) {
				if (fputs(line, out) < 0)
					break;
			}
		}
		if (!print_comments || ferror(out) || ferror(in) || fseek(in, 0, SEEK_SET) != 0)
			break;
	}
	if (ferror(in)) {
		log_error_file_t(file);
		result = -1;
	}
	else if (ferror(out)) {
		log_error_file_t(&new_file);
		result = -1;
	}
	fclose(in);
	if (fclose(out) < 0 && result == 0) {
		log_error_file_t(&new_file);
		result = -1;
	}
	/* overwrite the hash file with the new one */
	if (result == 0 && file_rename(&new_file, file) < 0) {
		/* TRANSLATORS: printed when a file rename failed */
		log_error(_("can't move %s to %s: %s\n"),
			file_get_print_path(&new_file, FPathPrimaryEncoding | FPathNotNull),
			file_get_print_path(file, FPathPrimaryEncoding | FPathNotNull), strerror(errno));
		result = -1;
	}
	file_cleanup(&new_file);
	return result;
}
