#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "image-processing.h"

// there is strict typing for BMP headers, this is to make sure there is no padding
#pragma pack(1)

/*
    Input: 2 random numbers from a uniform distribution
    Output 2 numbers from normal (gaussian) distribution
*/
void box_muller_transform(Box_Muller_Output *output)
{
    // generate 2 random numbers
    double u1 = (double) rand() / RAND_MAX;
    double u2 = (double) rand() / RAND_MAX;

    output->z1 = (sqrt(-2 * log(u1)))*(cos(2*M_PI*u2));
    output->z2 = (sqrt(-2 * log(u1)))*(sin(2*M_PI*u2));
}
