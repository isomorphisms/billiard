#ifndef BILLIARD_FLOW_H
#define BILLIARD_FLOW_H
#include "surface.h"

enum billiard_flow_status { BILLIARD_FLOW_OK, BILLIARD_FLOW_EDGE,
    BILLIARD_FLOW_REGULAR_VERTEX, BILLIARD_FLOW_SINGULAR,
    BILLIARD_FLOW_PERIODIC, BILLIARD_FLOW_DISTANCE_LIMIT,
    BILLIARD_FLOW_CROSSING_LIMIT, BILLIARD_FLOW_STOPPED,
    BILLIARD_FLOW_ARITHMETIC_LIMIT, BILLIARD_FLOW_INVALID };

typedef struct {
    size_t initial_copy, copy, initial_vertex_class;
    exact_point initial_position, initial_chart_position, direction;
    exact_point chart_translation;
    billiard_polygon_ray folded_ray;
    exact_number parameter, parameter_left;
    uint32_t crossings_left, crossings;
    enum billiard_flow_status terminal;
} billiard_flow;

typedef struct {
    enum billiard_flow_status kind;
    size_t from_copy, to_copy, side, vertex_class;
    exact_point folded_start, folded_end, developed_start, developed_end;
    exact_point global_direction;
    exact_number parameter, parameter_start;
    double length; /* display estimate only */
} billiard_flow_event;

enum billiard_flow_status billiard_flow_begin(const billiard_surface *surface,
    size_t copy, exact_point folded_position, exact_point global_direction,
    exact_number parameter_budget, uint32_t crossing_budget, billiard_flow *flow);
enum billiard_flow_status billiard_flow_begin_vertex(const billiard_surface *surface,
    size_t copy, size_t vertex, exact_point global_direction,
    exact_number parameter_budget, uint32_t crossing_budget, billiard_flow *flow);
enum billiard_flow_status billiard_flow_next(const billiard_surface *surface,
    billiard_flow *flow, billiard_flow_event *event);
const char *billiard_flow_status_name(enum billiard_flow_status status);
int billiard_flow_write_event(FILE *output, const billiard_flow_event *event);
/* PERIODIC is a first-return witness for this position and direction. A limit
 * is unresolved, and never means aperiodic, minimal, dense or ergodic. */
#endif
