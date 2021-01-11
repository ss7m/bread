#ifndef FLT_COMMON_H
#define FLT_COMMON_H

#include <stdlib.h>
#include <stdio.h>

#define BARF(fmt, ...) do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); exit(EXIT_FAILURE); } while (0)

#endif
