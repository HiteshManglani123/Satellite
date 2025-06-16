#ifndef IMAGE_PROCESSING_H

#define IMAGE_PROCESSING_H
#include <stdint.h>

typedef struct {
    int16_t signature; // must be 4D42 hex
    uint32_t file_size; // unreliable
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t data_offset;

    uint32_t info_header_size; // must be 40?

    int32_t width; // could be negative
    int32_t heigth; // could be negative

    uint16_t planes; // must be 1
    uint16_t bit_depth; // should be (1, 4, ,8 or 24)
    uint32_t compression_type; // 0=none, 1=RLE-8, 2=RLE-4)
    uint32_t data_size; // includes padding
    int32_t horizontal_resolution; // per meter? - unreliable
    int32_t vertical_resolution; // per meter? - unreliable
    int32_t number_of_colors; // could be zero
    int32_t number_of_important_colors; // could be zero
} Bmp_Header;

#endif
