/* file_mask.h */
#ifndef FILE_MASK_H
#define FILE_MASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common_func.h"

/* an array to store rules for file acceptance */
typedef struct vector_t file_mask_array;

#define file_mask_new()   rsh_vector_new_simple()
#define file_mask_free(v) rsh_vector_free(v)

file_mask_array* file_mask_new_from_list(const char* comma_separated_list);
void file_mask_add_list(file_mask_array* vect, const char* comma_separated_list);
int file_mask_match(file_mask_array* vect, const char* name);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* FILE_MASK_H */
