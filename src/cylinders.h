#ifndef BILLIARD_CYLINDERS_H
#define BILLIARD_CYLINDERS_H
#include "flow.h"

#define BILLIARD_MAX_STRIPS 256u
#define BILLIARD_MAX_CONNECTIONS 512u
#define BILLIARD_MAX_DIRECTIONS 512u
enum billiard_cylinder_status { BILLIARD_CYLINDER_CERTIFIED, BILLIARD_CYLINDER_LIMIT,
    BILLIARD_CYLINDER_UNDETERMINED, BILLIARD_CYLINDER_ARITHMETIC_LIMIT, BILLIARD_CYLINDER_INVALID };
typedef struct {
    unsigned max_strips, max_crossings, max_connections;
    exact_number length_bound;
    int mark_regular_vertices;
} billiard_cylinder_limits;
typedef struct {
    size_t copy, left_side, right_side, next, cylinder;
    int32_t level;
    exact_number left_lower, left_upper, right_lower, right_upper;
} billiard_direction_strip;
typedef struct {
    exact_number period_parameter, circumference_squared, area;
    exact_point holonomy;
    size_t first_strip;
    unsigned strip_count;
    double circumference, height;
    int within_length_bound;
} billiard_cylinder;
typedef struct {
    size_t start_copy, start_vertex, start_class, end_class, start_germ;
    exact_number parameter, length_squared;
    exact_point holonomy;
    unsigned strip_crossing_bound;
    int marked_point_convention;
} billiard_connection;
typedef struct {
    enum billiard_cylinder_status status;
    billiard_cylinder_limits limits;
    exact_point direction;
    exact_number transverse_unit, direction_squared, area_covered;
    size_t strip_count, cylinder_count, connection_count;
    unsigned counted_cylinders, boundary_segments_over_length;
    billiard_direction_strip strip[BILLIARD_MAX_STRIPS];
    billiard_cylinder cylinder[BILLIARD_MAX_STRIPS];
    billiard_connection connection[BILLIARD_MAX_CONNECTIONS];
} billiard_cylinder_report;

billiard_cylinder_limits billiard_cylinder_default_limits(void);
/* Certifies directions whose chart vertex heights and seam translations lie
 * in a bounded discrete transverse lattice. Other directions remain UNKNOWN.
 * Connections are oriented by the caller's direction, retaining distinct
 * outgoing cone sectors even when holonomy and endpoints coincide. */
enum billiard_cylinder_status billiard_cylinders(const billiard_surface *surface,
    exact_point direction, const billiard_cylinder_limits *limits,
    billiard_cylinder_report *report);
int billiard_cylinders_write(FILE *output, const billiard_cylinder_report *report);
typedef struct {
    int32_t horizontal, vertical;
    enum billiard_cylinder_status status;
    size_t connections;
    unsigned closed_families;
} billiard_direction_count;
typedef struct {
    unsigned direction_bound, directions_examined, unresolved_directions;
    size_t connections, closed_families;
    int truncated, all_lengths_covered;
    billiard_cylinder_limits limits;
    billiard_direction_count direction[BILLIARD_MAX_DIRECTIONS];
} billiard_counting_report;
/* Enumerate coprime integer directions (p,q), p>0 or p=0,q>0. Reversals and
 * scalar multiples are excluded. A full length-bound certificate additionally
 * requires integral vertex/seam holonomy, bound>=length and no unresolved row. */
enum billiard_cylinder_status billiard_direction_sweep(const billiard_surface *surface,
    unsigned direction_bound, unsigned max_directions, const billiard_cylinder_limits *limits,
    FILE *raw_evidence, billiard_counting_report *report);
#endif
