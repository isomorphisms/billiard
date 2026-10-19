#ifndef BILLIARD_PIXEL_SAMPLE_H
#define BILLIARD_PIXEL_SAMPLE_H

#include <stdint.h>

#define BILLIARD_WINDOW_LIMIT 8192u
#define BILLIARD_IMAGE_LIMIT 1000u

/* paint validates both dimensions before indexing. These bounds also cover
 * choose_image: width <= 360 and height <= 1000. The largest numerator is
 * 8191 * 1000 = 8191000, so uint32_t gives exactly the original uint64_t
 * quotient, including its floor. No arithmetic runtime or approximation is
 * needed for this bounded pixel operation. */
_Static_assert((uint64_t)(BILLIARD_WINDOW_LIMIT - 1u) *
               BILLIARD_IMAGE_LIMIT <= UINT32_MAX, "pixel product must fit");

static inline uint32_t billiard_pixel_sample(uint32_t position,
                                            uint32_t image_size,
                                            uint32_t window_size) {
    return position * image_size / window_size;
}

#endif
