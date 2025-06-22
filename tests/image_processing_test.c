#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "../src/image-processing.h"

static int tests_run = 0;
static int tests_passed = 0;
#define TEST_ASSERT(expr) do { tests_run++; if (expr) { tests_passed++; } else { \
    printf("\033[0;31mTest failed: %s at %s:%d\033[0m\n", #expr, __FILE__, __LINE__); \
    return 1; } } while(0)

// Forward declaration if not in header
void box_muller_transform(Box_Muller_Output *output);

int main() {
    Box_Muller_Output output;
    int nan_count = 0, inf_count = 0;
    int N = 10000;
    double sum_z1 = 0, sum_z2 = 0, sumsq_z1 = 0, sumsq_z2 = 0;

    for (int i = 0; i < N; ++i) {
        box_muller_transform(&output);
        if (isnan(output.z1) || isnan(output.z2)) nan_count++;
        if (isinf(output.z1) || isinf(output.z2)) inf_count++;
        sum_z1 += output.z1;
        sum_z2 += output.z2;
        sumsq_z1 += output.z1 * output.z1;
        sumsq_z2 += output.z2 * output.z2;
    }

    double mean_z1 = sum_z1 / N;
    double mean_z2 = sum_z2 / N;
    double stddev_z1 = sqrt(sumsq_z1 / N - mean_z1 * mean_z1);
    double stddev_z2 = sqrt(sumsq_z2 / N - mean_z2 * mean_z2);

    TEST_ASSERT(nan_count == 0 && inf_count == 0);
    TEST_ASSERT(fabs(mean_z1) < 0.1 && fabs(mean_z2) < 0.1); // mean should be close to 0
    TEST_ASSERT(fabs(stddev_z1 - 1.0) < 0.1 && fabs(stddev_z2 - 1.0) < 0.1); // stddev should be close to 1

    if (tests_run == tests_passed) {
        printf("\033[0;32m%d/%d tests passed\033[0m\n", tests_passed, tests_run);
    } else {
        printf("\033[0;31m%d/%d tests passed\033[0m\n", tests_passed, tests_run);
    }
    return 0;
} 