/* hash_update.h - functions to update a hash file */
#ifndef HASH_UPDATE_H
#define HASH_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif

struct file_t;
struct update_ctx;
struct update_ctx* update_ctx_new(struct file_t* update_file);
int update_ctx_update(struct update_ctx* ctx, struct file_t* file);
int update_ctx_free(struct update_ctx* ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* HASH_UPDATE_H */
