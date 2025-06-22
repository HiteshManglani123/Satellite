#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "image-processing.h"

void box_muller_transform(Box_Muller_Output *output);

int main(void)
{
    FILE *fp_image = fopen("images/some-random-stars.bmp", "r");

    if (fp_image == NULL) {
        fprintf(stderr, "Unable to find image\n");
        return -1;
    }

    Bmp_Header *bmp_header = malloc(sizeof(Bmp_Header));

    if (bmp_header == NULL) {
        fprintf(stderr, "Could not allocate space for bmp header");
        return -1;
    }

    // Transfer header of fp_image into bmp_header
    fread(bmp_header, 1, sizeof(Bmp_Header), fp_image);

    printf("Bit depth: %d\n", bmp_header->bit_depth);
    printf("width: %d\n", bmp_header->width);
    printf("heigth: %d\n", bmp_header->heigth);
    printf("offset: %d\n", bmp_header->data_offset);

    // go to image data, SEEK_SET = beginning of file
    fseek(fp_image, bmp_header->data_offset, SEEK_SET);

    int amount_of_bytes_to_read = bmp_header->data_size;
    unsigned char *image_buf = malloc(amount_of_bytes_to_read);

    if (image_buf == NULL) {
        fprintf(stderr, "Unable to allocate space for image buf");
        return 1;
    }

    if (fread(image_buf, 1, amount_of_bytes_to_read, fp_image) != amount_of_bytes_to_read) {
        fprintf(stderr, "unable to read %d bytes from image\n", amount_of_bytes_to_read);
        return 1;
    }

    // Gaussian noise
    srand(clock());
    Box_Muller_Output output;
    int width = (bmp_header->width * bmp_header->bit_depth) / 8;
    int heigth = bmp_header->heigth;
    double amount_of_noise_scale = 50;

    for (int i = 0; i < heigth; i++) {
        for (int j = 0; j < width; j++) {
            int index = (i * width) + j;
            box_muller_transform(&output);
            image_buf[index] += (output.z1 * amount_of_noise_scale);
        }
    }

    FILE *fo = fopen("images/test.bmp", "wb");
    if (fo == NULL) {
        fprintf(stderr, "Unable to open file for writing binary image\n");
        return -1;
    }

    if (fwrite(bmp_header, 1, sizeof(Bmp_Header), fo) != sizeof(Bmp_Header)) {
        fprintf(stderr, "Unable to write header file of %lu bytes\n", sizeof(Bmp_Header));
        return -1;
    }
    printf("Wrote header: %lu bytes\n", sizeof(Bmp_Header));
    fseek(fo, bmp_header->data_offset, SEEK_SET);
    if (fwrite(image_buf, 1, amount_of_bytes_to_read, fo) != amount_of_bytes_to_read) {
        fprintf(stderr, "Unable to write image data of %d bytes\n", amount_of_bytes_to_read);
        return 1;
    }
    printf("Wrote image data: %d bytes\n", amount_of_bytes_to_read);
    printf("File size: %d\n", bmp_header->file_size);
    printf("Data size: %d\n", bmp_header->data_size);
    fclose(fp_image);
    fclose(fo);
    return 0;
} 