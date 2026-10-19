#ifndef BILLIARD_POLYGON_H
#define BILLIARD_POLYGON_H

#include <stddef.h>
#include <stdint.h>
#include "billiard.h"

#define BILLIARD_POLYGON_MAX_VERTICES 32u

/* Exactly (rational + radical * sqrt(3)) / denominator. Canonical values
 * have positive denominator, no INT32_MIN coefficient, and common gcd 1.
 * Arithmetic exhaustion is reported, never converted to a floating tie. */
typedef struct { int32_t rational, radical, denominator; } billiard_exact;
typedef struct { billiard_exact x, y; } billiard_exact_point;
typedef struct { uint16_t numerator, denominator; } billiard_angle;

typedef struct {
    size_t count;
    billiard_exact_point vertex[BILLIARD_POLYGON_MAX_VERTICES];
    billiard_angle angle[BILLIARD_POLYGON_MAX_VERTICES]; /* interior angle / pi */
} billiard_polygon;

enum billiard_polygon_status {
    BILLIARD_POLYGON_OK = 0,
    BILLIARD_POLYGON_EDGE = 1,
    BILLIARD_POLYGON_VERTEX_STOP = 2,
    BILLIARD_POLYGON_STEP_LIMIT = 3,
    BILLIARD_POLYGON_ARITHMETIC_LIMIT = 4,
    BILLIARD_POLYGON_UNSUPPORTED_ANGLE = 5,
    BILLIARD_POLYGON_INVALID = 6,
    BILLIARD_POLYGON_STOPPED = 7
};

typedef struct {
    billiard_exact_point position, direction;
    size_t previous_side;
    uint32_t steps_remaining;
    double distance; /* approximate display length; never used in predicates */
    int stopped;
} billiard_polygon_ray;

typedef struct {
    enum billiard_polygon_status kind;
    size_t side, vertex; /* vertex == SIZE_MAX at a regular edge */
    billiard_exact_point start, end, incoming, outgoing;
    billiard_exact parameter; /* end = start + parameter * incoming */
    double length, distance_start; /* display estimates only */
} billiard_polygon_event;

int billiard_exact_make(int32_t rational, int32_t radical, int32_t denominator,
                       billiard_exact *result);
double billiard_exact_display(billiard_exact number);

/* Certification currently recognizes exact directions in multiples of pi/12
 * relative to the first side, using algebra in Q(sqrt(3)). The caller must
 * supply angle claims; validation proves them from the exact vertices.
 * Other rational-angle families return UNSUPPORTED_ANGLE, not INVALID. */
enum billiard_polygon_status billiard_polygon_validate(const billiard_polygon *polygon);
enum billiard_polygon_status billiard_polygon_side_normal(const billiard_polygon *polygon,
    size_t side, billiard_exact_point *outward_normal);

/* location: -1 outside, 0 boundary, +1 interior. Requires validated geometry;
 * exact arithmetic failure leaves location unchanged. */
enum billiard_polygon_status billiard_polygon_locate(const billiard_polygon *polygon,
    billiard_exact_point point, int *location);

/* Construction adapter only: the legacy rectangle engine and UI convention
 * are unchanged. Unrepresentable binary floating inputs return a limit. */
enum billiard_polygon_status billiard_polygon_rectangle(const billiard_table *table,
                                                       billiard_polygon *polygon);
enum billiard_polygon_status billiard_polygon_begin(const billiard_polygon *polygon,
    billiard_exact_point position, billiard_exact_point direction, uint32_t step_limit,
    billiard_polygon_ray *ray);
enum billiard_polygon_status billiard_polygon_next(const billiard_polygon *polygon,
    billiard_polygon_ray *ray, billiard_polygon_event *event);

#endif
