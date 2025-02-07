// SPDX-License-Identifier: GPL-3.0-only
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "stats.h"

static int stats_compare_u64(const void *a, const void *b)
{
    return *(u64 *)a > *(u64 *)b;
}

int stats_pred_gt0(u64 val)
{
    return val > 0;
}

u64 stats_sum_u64(u64 *arr, u64 arr_len)
{
    u64 sum = 0;
    for (u64 i = 0; i < arr_len; ++i)
    {
        sum += arr[i];
    }
    return sum;
}

double stats_avg_u64(u64 *arr, u64 arr_len)
{
    return stats_sum_u64(arr, arr_len) / ((double)arr_len);
}

u64 stats_median_u64(u64 *arr, u64 arr_len)
{
    if (!arr_len)
        err(1, "0 length array has no median");

    u64 *copy = calloc(arr_len, sizeof(*arr));
    if (!copy)
        err(1, "failed to temp arr");

    memcpy(copy, arr, arr_len * sizeof(*arr));
    qsort(copy, arr_len, sizeof(*arr), stats_compare_u64);

    u64 median = copy[arr_len / 2];

    free(copy);

    return median;
}

u64 stats_min_u64(u64 *arr, u64 arr_len)
{
    if (!arr_len)
        err(1, "0 length array has no min");

    u64 min = arr[0];
    for (u64 i = 1; i < arr_len; ++i)
    {
        if (arr[i] < min)
        {
            min = arr[i];
        }
    }

    return min;
}

u64 stats_max_u64(u64 *arr, u64 arr_len)
{
    if (!arr_len)
        err(1, "0 length array has no max");

    u64 max = arr[0];
    for (u64 i = 1; i < arr_len; ++i)
    {
        if (arr[i] > max)
        {
            max = arr[i];
        }
    }

    return max;
}

u64 stats_count_u64(u64 *arr, u64 arr_len, int (*predicate)(u64 val))
{
    u64 count = 0;
    for (u64 i = 0; i < arr_len; ++i)
    {
        if (predicate(arr[i]))
        {
            ++count;
        }
    }
    return count;
}

void stats_results_start()
{
    printf("### RESULTS START ###\n");
}

void stats_results_end()
{
    printf("### RESULTS END ###\n");
    fflush(stdout);
}

void stats_print_u64(char *name, u64 val)
{
    printf("%s = %lu\n", name, val);
}

void stats_print_f64(char *name, f64 val)
{
    printf("%s = %lf\n", name, val);
}

void stats_print_arr_u64(char *name, u64 *arr, u64 len)
{
    printf("%s = [", name);
    if (len > 0)
        printf("%lu", arr[0]);
    for (int i = 1; i < len; ++i)
    {
        printf(", %lu", arr[i]);
    }
    printf("]\n");
}

void stats_print_arr_f64(char *name, f64 *arr, u64 len)
{
    printf("%s = [", name);
    if (len > 0)
        printf("%lf", arr[0]);
    for (int i = 1; i < len; ++i)
    {
        printf(", %lf", arr[i]);
    }
    printf("]\n");
}
