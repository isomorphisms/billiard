#include "flow.h"
#include <math.h>

static enum billiard_flow_status polygon_result(enum billiard_polygon_status status) {
    return status == BILLIARD_POLYGON_ARITHMETIC_LIMIT ? BILLIARD_FLOW_ARITHMETIC_LIMIT : BILLIARD_FLOW_INVALID;
}

static enum billiard_flow_status begin(const billiard_surface *surface, size_t copy,
    exact_point position, size_t vertex, exact_point direction, exact_number budget,
    uint32_t crossings, billiard_flow *flow) {
    if (!flow || billiard_surface_validate(surface) != BILLIARD_SURFACE_OK ||
        copy >= surface->copy_count || !exact_point_valid(direction) || !billiard_exact_valid(budget))
        return BILLIARD_FLOW_INVALID;
    exact_work work = {0};
    if (exact_order(&work,budget,exact_integer(0)) < 0 || work.failed) return BILLIARD_FLOW_INVALID;
    exact_point folded_direction = exact_transform(&work,exact_transpose(surface->copy[copy].placement),direction);
    if (work.failed) return BILLIARD_FLOW_ARITHMETIC_LIMIT;
    billiard_polygon_ray ray;
    enum billiard_polygon_status status = vertex == SIZE_MAX ?
        billiard_polygon_begin(&surface->polygon,position,folded_direction,UINT32_MAX,&ray) :
        billiard_polygon_depart(&surface->polygon,vertex,folded_direction,UINT32_MAX,&ray);
    if (status != BILLIARD_POLYGON_OK) return polygon_result(status);
    billiard_flow proposed = {0};
    proposed.initial_copy = proposed.copy = copy;
    proposed.initial_vertex_class = vertex == SIZE_MAX ? SIZE_MAX : surface->vertex_class[copy][vertex];
    proposed.initial_position = ray.position;
    proposed.initial_chart_position = exact_transform(&work,surface->copy[copy].placement,ray.position);
    proposed.direction = direction;
    proposed.chart_translation = (exact_point){exact_integer(0),exact_integer(0)};
    proposed.folded_ray = ray;
    proposed.parameter = exact_integer(0); proposed.parameter_left = budget;
    proposed.crossings_left = crossings;
    if (work.failed) return BILLIARD_FLOW_ARITHMETIC_LIMIT;
    *flow = proposed;
    return BILLIARD_FLOW_OK;
}

enum billiard_flow_status billiard_flow_begin(const billiard_surface *surface,
    size_t copy, exact_point position, exact_point direction, exact_number budget,
    uint32_t crossings, billiard_flow *flow) {
    return begin(surface,copy,position,SIZE_MAX,direction,budget,crossings,flow);
}
enum billiard_flow_status billiard_flow_begin_vertex(const billiard_surface *surface,
    size_t copy, size_t vertex, exact_point direction, exact_number budget,
    uint32_t crossings, billiard_flow *flow) {
    exact_point ignored = {exact_integer(0),exact_integer(0)};
    if (!surface || vertex >= surface->polygon.count) return BILLIARD_FLOW_INVALID;
    return begin(surface,copy,ignored,vertex,direction,budget,crossings,flow);
}

static int return_within(exact_work *work, const billiard_flow *flow,
    exact_number flight_parameter, exact_number *return_parameter) {
    if (flow->copy != flow->initial_copy || flow->initial_vertex_class != SIZE_MAX) return 0;
    exact_point offset = exact_point_difference(work,flow->initial_position,flow->folded_ray.position);
    exact_number collinearity = exact_cross(work,offset,flow->folded_ray.direction);
    if (exact_order(work,collinearity,exact_integer(0))) return 0;
    exact_number candidate;
    if (exact_order(work,flow->folded_ray.direction.x,exact_integer(0)))
        candidate = exact_quotient(work,offset.x,flow->folded_ray.direction.x);
    else candidate = exact_quotient(work,offset.y,flow->folded_ray.direction.y);
    if (exact_order(work,candidate,exact_integer(0)) <= 0 ||
        exact_order(work,candidate,flight_parameter) > 0) return 0;
    *return_parameter = candidate;
    return 1;
}

static enum billiard_flow_status depart_regular(const billiard_surface *surface,
    size_t vertex_class, exact_point direction, size_t *copy_out, billiard_polygon_ray *ray_out) {
    exact_work work = {0};
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        for (size_t vertex = 0; vertex < surface->polygon.count; ++vertex) {
            if (surface->vertex_class[copy][vertex] != vertex_class) continue;
            exact_point folded = exact_transform(&work,exact_transpose(surface->copy[copy].placement),direction);
            if (work.failed) return BILLIARD_FLOW_ARITHMETIC_LIMIT;
            billiard_polygon_ray ray;
            enum billiard_polygon_status status = billiard_polygon_depart(&surface->polygon,vertex,folded,UINT32_MAX,&ray);
            if (status == BILLIARD_POLYGON_ARITHMETIC_LIMIT) return BILLIARD_FLOW_ARITHMETIC_LIMIT;
            if (status == BILLIARD_POLYGON_OK) { *copy_out = copy; *ray_out = ray; return BILLIARD_FLOW_OK; }
        }
    return BILLIARD_FLOW_INVALID;
}

enum billiard_flow_status billiard_flow_next(const billiard_surface *surface,
    billiard_flow *flow, billiard_flow_event *event) {
    if (!flow || !event || billiard_surface_validate(surface) != BILLIARD_SURFACE_OK ||
        flow->copy >= surface->copy_count || flow->initial_copy >= surface->copy_count ||
        !exact_point_valid(flow->direction) || !billiard_exact_valid(flow->parameter) ||
        !billiard_exact_valid(flow->parameter_left)) return BILLIARD_FLOW_INVALID;
    if (flow->terminal != BILLIARD_FLOW_OK) return BILLIARD_FLOW_STOPPED;
    exact_work work = {0};
    if (exact_order(&work,flow->parameter_left,exact_integer(0)) <= 0)
        return work.failed ? BILLIARD_FLOW_ARITHMETIC_LIMIT : BILLIARD_FLOW_DISTANCE_LIMIT;
    exact_point direction = exact_transform(&work,surface->copy[flow->copy].placement,flow->folded_ray.direction);
    exact_point developed_start = exact_point_scale(&work,flow->direction,flow->parameter);
    developed_start = exact_point_sum(&work,flow->initial_chart_position,developed_start);
    exact_point chart_start = exact_transform(&work,surface->copy[flow->copy].placement,flow->folded_ray.position);
    exact_point expected_chart = exact_point_sum(&work,developed_start,flow->chart_translation);
    if (work.failed) return BILLIARD_FLOW_ARITHMETIC_LIMIT;
    if (!exact_point_equal(direction,flow->direction) || !exact_point_equal(expected_chart,chart_start))
        return BILLIARD_FLOW_INVALID;
    billiard_polygon_ray advanced = flow->folded_ray;
    billiard_polygon_event collision;
    enum billiard_polygon_status polygon_status = billiard_polygon_next(&surface->polygon,&advanced,&collision);
    if (polygon_status != BILLIARD_POLYGON_EDGE && polygon_status != BILLIARD_POLYGON_VERTEX_STOP)
        return polygon_result(polygon_status);
    exact_number flight_parameter = collision.parameter, return_parameter;
    int returns = return_within(&work,flow,flight_parameter,&return_parameter);
    enum billiard_flow_status kind = BILLIARD_FLOW_EDGE;
    if (returns && exact_order(&work,return_parameter,flow->parameter_left) <= 0) {
        flight_parameter = return_parameter; kind = BILLIARD_FLOW_PERIODIC;
    } else if (exact_order(&work,flow->parameter_left,flight_parameter) < 0) {
        flight_parameter = flow->parameter_left; kind = BILLIARD_FLOW_DISTANCE_LIMIT;
    } else if (!flow->crossings_left) return BILLIARD_FLOW_CROSSING_LIMIT;
    if (work.failed) return BILLIARD_FLOW_ARITHMETIC_LIMIT;
    billiard_flow proposed = *flow;
    int boundary_reached = kind == BILLIARD_FLOW_EDGE;
    exact_point folded_start = flow->folded_ray.position;
    exact_point folded_displacement = exact_point_scale(&work,flow->folded_ray.direction,flight_parameter);
    exact_point folded_end = exact_point_sum(&work,folded_start,folded_displacement);
    exact_point displacement = exact_point_scale(&work,flow->direction,flight_parameter);
    exact_point developed_end = exact_point_sum(&work,developed_start,displacement);
    proposed.parameter = exact_sum(&work,flow->parameter,flight_parameter);
    proposed.parameter_left = exact_difference(&work,flow->parameter_left,flight_parameter);
    size_t vertex_class = SIZE_MAX;
    if (kind == BILLIARD_FLOW_EDGE) {
        ++proposed.crossings; --proposed.crossings_left;
        if (polygon_status == BILLIARD_POLYGON_VERTEX_STOP) {
            vertex_class = surface->vertex_class[flow->copy][collision.vertex];
            if (surface->cone[vertex_class].zero_order) {
                kind = BILLIARD_FLOW_SINGULAR; proposed.folded_ray = advanced;
            } else if (vertex_class == flow->initial_vertex_class) {
                kind = BILLIARD_FLOW_PERIODIC; proposed.folded_ray = advanced;
            } else {
                kind = BILLIARD_FLOW_REGULAR_VERTEX;
                enum billiard_flow_status status = depart_regular(surface,vertex_class,flow->direction,&proposed.copy,&proposed.folded_ray);
                if (status != BILLIARD_FLOW_OK) return status;
            }
        } else {
            proposed.copy = surface->seam[flow->copy][collision.side].target_copy;
            proposed.folded_ray = advanced;
            proposed.folded_ray.steps_remaining = UINT32_MAX;
        }
        exact_point old_chart = exact_transform(&work,surface->copy[flow->copy].placement,folded_end);
        exact_point new_chart = exact_transform(&work,surface->copy[proposed.copy].placement,proposed.folded_ray.position);
        exact_point translation = exact_point_difference(&work,new_chart,old_chart);
        proposed.chart_translation = exact_point_sum(&work,flow->chart_translation,translation);
        if (kind == BILLIARD_FLOW_EDGE || kind == BILLIARD_FLOW_REGULAR_VERTEX) {
            exact_point after_direction = exact_transform(&work,surface->copy[proposed.copy].placement,proposed.folded_ray.direction);
            if (!exact_point_equal(after_direction,flow->direction)) return BILLIARD_FLOW_INVALID;
        }
    } else {
        proposed.folded_ray.position = folded_end;
        proposed.folded_ray.previous_side = SIZE_MAX;
        proposed.folded_ray.launch_vertex = SIZE_MAX;
    }
    if (kind == BILLIARD_FLOW_SINGULAR || kind == BILLIARD_FLOW_PERIODIC || kind == BILLIARD_FLOW_DISTANCE_LIMIT)
        proposed.terminal = kind;
    if (work.failed) return BILLIARD_FLOW_ARITHMETIC_LIMIT;
    double length = hypot(billiard_exact_display(displacement.x),billiard_exact_display(displacement.y));
    if (!isfinite(length)) return BILLIARD_FLOW_ARITHMETIC_LIMIT;
    *event = (billiard_flow_event){kind,flow->copy,proposed.copy,boundary_reached ? collision.side : SIZE_MAX,vertex_class,
        folded_start,folded_end,developed_start,developed_end,flow->direction,flight_parameter,flow->parameter,length};
    *flow = proposed;
    return kind;
}

const char *billiard_flow_status_name(enum billiard_flow_status status) {
    static const char *const names[] = {"ready","edge","regular_vertex","singular_stop",
        "periodic_first_return","distance_limit_unknown","crossing_limit_unknown","stopped",
        "arithmetic_limit_unknown","invalid"};
    return status >= BILLIARD_FLOW_OK && status <= BILLIARD_FLOW_INVALID ? names[status] : "invalid";
}

int billiard_flow_write_event(FILE *output, const billiard_flow_event *event) {
    if (!output || !event) return -1;
    fprintf(output,"flow\t%s\t%zu\t%zu\t%zu\t%zu\t%d\t%d\t%d\t%d\t%d\t%d",
        billiard_flow_status_name(event->kind),event->from_copy,event->to_copy,event->side,event->vertex_class,
        event->parameter_start.rational,event->parameter_start.radical,event->parameter_start.denominator,
        event->parameter.rational,event->parameter.radical,event->parameter.denominator);
    const exact_point points[] = {event->folded_start,event->folded_end,event->developed_start,event->developed_end,event->global_direction};
    for (unsigned point = 0; point < sizeof points/sizeof *points; ++point)
        fprintf(output,"\t%d\t%d\t%d\t%d\t%d\t%d",points[point].x.rational,points[point].x.radical,points[point].x.denominator,
            points[point].y.rational,points[point].y.radical,points[point].y.denominator);
    fprintf(output,"\t%.17g\n",event->length);
    return ferror(output) ? -1 : 0;
}
