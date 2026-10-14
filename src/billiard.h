#ifndef BILLIARD_H
#define BILLIARD_H

#include <stddef.h>

/* Geometry is in mathematical units, with origin at the top-left and y down. */
enum billiard_hit {
    BILLIARD_NO_HIT = 0,
    BILLIARD_VERTICAL_WALL = 1,
    BILLIARD_HORIZONTAL_WALL = 2,
    BILLIARD_CORNER = 3
};

typedef struct {
    double width;
    double height;
    double start_x;
    double start_y;
    /* Degrees counterclockwise from rightward on a y-up mathematical plane. */
    double launch_degrees;
} billiard_table;

typedef struct {
    double x, y;
    double dx, dy; /* unit direction in y-down screen coordinates */
    double distance;
    unsigned long collisions;
} billiard_ray;

typedef struct {
    double x0, y0, x1, y1;
    double distance_start;
    double length;
    enum billiard_hit hit;
} billiard_segment;

/* width, height > 0. First point is (width/2, height/3), shot up-right at 45°. */
int billiard_default(billiard_table *table, double width, double height);
int billiard_begin(const billiard_table *table, billiard_ray *ray);

/* One exact straight segment; a corner flips both components. Return 0 on success. */
int billiard_next(const billiard_table *table, billiard_ray *ray, billiard_segment *segment);

/* Trace up to a geometric distance budget, possibly ending mid-segment.
   Returns number written, or SIZE_MAX for invalid arguments / geometry. */
size_t billiard_trace(const billiard_table *table, double distance_budget,
                      billiard_segment *segments, size_t capacity);

/* Fold an unbounded straight path into a mirrored [0,span] interval. */
double billiard_fold(double unfolded_coordinate, double span);

/* The marked rectangular torus has modulus i*height/width (no real part). */
double billiard_modulus_imaginary(const billiard_table *table);

#endif
