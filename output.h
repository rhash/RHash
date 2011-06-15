/* output.h */
#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct file_info;
struct timeval;

struct percents_output_info_t {
  /* methods to output percents */
  int  (*init)(struct file_info *info);
  void (*update)(struct file_info *info, uint64_t offset);
  void (*finish)(struct file_info *info, int process_res);
  const char* name;
};

/* pointer to the selected percents output method */
extern struct percents_output_info_t *percents_output;
#define init_percents(info)   percents_output->init(info)
#define update_percents(info, offset) percents_output->update(info, offset)
#define finish_percents(info, process_res) percents_output->finish(info, process_res)

/* pointers to functions to print percents and file info */
#if 0
extern int  (*init_percents)(struct file_info *info);
extern void (*update_percents)(struct file_info *info, uint64_t offset);
extern void (*finish_percents)(struct file_info *info, int process_res);
#endif

/* method to initialize pointers to output methods */
void setup_output(void);

void log_msg(const char* format, ...);
void log_file_error(const char* filepath);
void print_check_stats(void);

void print_time_stats(double time, uint64_t size, int total);
void print_file_time_stats(struct file_info* info);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* OUTPUT_H */
