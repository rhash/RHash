/* output.h */
#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct file_info;
struct file_t;

/**
 * A 'method' to output percents.
 */
struct percents_output_info_t
{
	int  (*init)(struct file_info* info);
	void (*update)(struct file_info* info, uint64_t offset);
	int  (*finish)(struct file_info* info, int process_res);
	const char* name;
};

/* pointer to the selected percents output method */
extern struct percents_output_info_t* percents_output;
#define init_percents(info)   percents_output->init(info)
#define update_percents(info, offset) percents_output->update(info, offset)
#define finish_percents(info, process_res) percents_output->finish(info, process_res)

/* initialization of percents output method */
void setup_output(void);
void setup_percents(void);

enum FileOutputFlags {
	OutDefaultFlags = 0x0,
	OutForceUtf8 = 0x1,
	OutBaseName = 0x4,
	OutCountSymbols = 0x8
};
int fprintf_file_t(FILE* out, const char* format, struct file_t* file, unsigned output_flags);
int fprint_urlencoded(FILE* out, const char* str, int upper_case);

void log_msg(const char* format, ...);
void log_msg_file_t(const char* format, struct file_t* file);
#define log_warning log_error
#define log_warning_msg_file_t log_error_msg_file_t
void log_error(const char* format, ...);
void log_error_file_t(struct file_t* file);
void log_error_msg_file_t(const char* format, struct file_t* file);

void report_interrupted(void);
int print_verifying_header(struct file_t* file);
int print_verifying_footer(void);
int print_check_stats(void);

void print_time_stats(double time, uint64_t size, int total);
void print_file_time_stats(struct file_info* info);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* OUTPUT_H */
