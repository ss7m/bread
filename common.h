#ifndef FLT_COMMON_H
#define FLT_COMMON_H

#include <stdlib.h>
#include <stdio.h>

#define BARF(fmt, ...) do { \
        fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
        fprintf(stderr, "at line %d\n", __LINE__); \
        fprintf(stderr, "in function %s\n", __func__); \
        fprintf(stderr, "in file %s\n", __FILE__); \
        exit(EXIT_FAILURE); \
} while (0)

#endif
