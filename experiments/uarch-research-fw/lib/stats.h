// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "compiler.h"

int stats_pred_gt0(u64 val);

u64 stats_sum_u64(u64 *arr, u64 arr_len);
double stats_avg_u64(u64 *arr, u64 arr_len);
u64 stats_median_u64(u64 *arr, u64 arr_len);

u64 stats_min_u64(u64 *arr, u64 arr_len);
u64 stats_max_u64(u64 *arr, u64 arr_len);

u64 stats_count_u64(u64 *arr, u64 arr_len, int (*predicate)(u64 val));

// stats output
void stats_results_start(void);
void stats_results_end(void);
void stats_print_u64(char *name, u64 arr);
void stats_print_f64(char *name, f64 arr);
void stats_print_arr_u64(char *name, u64 *arr, u64 len);
void stats_print_arr_f64(char *name, f64 *arr, u64 len);
