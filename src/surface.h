#ifndef BILLIARD_SURFACE_H
#define BILLIARD_SURFACE_H
#include <stdio.h>
#include "exact_geometry.h"

#define BILLIARD_SURFACE_MAX_COPIES 24u
#define BILLIARD_SURFACE_MAX_CORNERS (BILLIARD_SURFACE_MAX_COPIES * BILLIARD_POLYGON_MAX_VERTICES)

enum billiard_surface_status { BILLIARD_SURFACE_OK, BILLIARD_SURFACE_INVALID,
    BILLIARD_SURFACE_ARITHMETIC_LIMIT, BILLIARD_SURFACE_COPY_LIMIT };
typedef struct { billiard_exact_matrix placement; int orientation; } billiard_surface_copy;
typedef struct { size_t target_copy, target_side; exact_point translation; } billiard_surface_seam;
typedef struct { unsigned angle_twelfths, zero_order; size_t representative; } billiard_surface_cone;
typedef struct {
    billiard_polygon polygon;
    size_t copy_count;
    billiard_surface_copy copy[BILLIARD_SURFACE_MAX_COPIES];
    billiard_surface_seam seam[BILLIARD_SURFACE_MAX_COPIES][BILLIARD_POLYGON_MAX_VERTICES];
    size_t vertex_class[BILLIARD_SURFACE_MAX_COPIES][BILLIARD_POLYGON_MAX_VERTICES];
    size_t vertex_class_count;
    billiard_surface_cone cone[BILLIARD_SURFACE_MAX_CORNERS];
    unsigned components, genus, edge_count;
    int euler_characteristic;
    exact_number area;
} billiard_surface;

/* Standard reflected-copy translation cover, with 2*lcm(angle denominators)
 * copies. It is neither the original polygon nor its orientation double. */
enum billiard_surface_status billiard_surface_unfold(const billiard_polygon *polygon,
                                                    billiard_surface *surface);
enum billiard_surface_status billiard_surface_validate(const billiard_surface *surface);
int billiard_surface_write(FILE *output, const billiard_surface *surface);
exact_point billiard_surface_vertex(exact_work *work, const billiard_surface *surface,
                                   size_t copy, size_t original_vertex);
exact_point billiard_surface_fold(exact_work *work, const billiard_surface *surface,
                                 size_t copy, exact_point point);
#endif
