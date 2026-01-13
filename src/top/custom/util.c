// +FHDR========================================================================
//  License:
//      
//  =============================================================================
//  File Name:      util.c
//                  Rocky (luoqi754@gmail.com)
//  Organization:
//  Description:
//      Utility functions for the C model.
// -FHDR========================================================================
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "util.h"

// Global log file pointer
static FILE *g_log_file = NULL;

void log_init(const char *log_path) {
    if (g_log_file) {
        fclose(g_log_file);
    }
    g_log_file = fopen(log_path, "a");
    if (!g_log_file) {
        fprintf(stderr, "Warning: Failed to open log file: %s\n", log_path);
    }
}

void log_close(void) {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

FILE* log_get_file(void) {
    return g_log_file;
}

uint64_t ExpMax(const uint64_t *exps,
                unsigned        exp_width,
                unsigned        data_size)
{
    assert(exp_width >= 1 && exp_width <= 64);
    const uint64_t mask = bitmask_u64(exp_width);

    uint64_t o_ExpMax = exps[0] & mask;
    for (unsigned i = 1; i < data_size; ++i) {
        uint64_t v = exps[i] & mask;
        if (v > o_ExpMax) o_ExpMax = v;
    }
    return o_ExpMax;
}
