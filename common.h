#ifndef BRD_COMMON_H
#define BRD_COMMON_H

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/* 
 * global variables for the parser
 */
extern int skip_newlines;
extern int line_number;

#ifdef DEBUG
#define BARF(fmt, ...) do { \
        fprintf(stderr, "Error: " fmt "\n", ##__VA_ARGS__); \
        fprintf(stderr, "\tat line %d\n", __LINE__); \
        fprintf(stderr, "\tin function %s\n", __func__); \
        fprintf(stderr, "\tin file %s\n", __FILE__); \
        exit(EXIT_FAILURE); \
} while (0)
#else
#define BARF(fmt, ...) do { \
        fprintf(stderr, "Error: " fmt "\n", ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
} while (0)
#endif

#endif
