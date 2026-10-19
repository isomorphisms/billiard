#ifndef BILLIARD_EXPLORER_H
#define BILLIARD_EXPLORER_H
#include "affine.h"
#include "cylinders.h"

#define BILLIARD_EXPLORER_MAX_EVENTS 160u
#define BILLIARD_EXPLORER_GHOST_EVENTS 8u
enum billiard_explorer_view { BILLIARD_EXPLORER_LEGACY, BILLIARD_EXPLORER_POLYGON, BILLIARD_EXPLORER_COVER };
enum billiard_explorer_handle { BILLIARD_EXPLORER_START, BILLIARD_EXPLORER_ANGLE, BILLIARD_EXPLORER_WIDTH };
typedef struct {
    enum billiard_fixture fixture;
    enum billiard_explorer_view view;
    unsigned heading, start_horizontal, start_vertical, width_ticks, event_limit;
    int shear_half_steps, show_cylinders;
} billiard_explorer_config;
typedef struct {
    billiard_explorer_config config;
    billiard_polygon polygon;
    billiard_surface surface;
    billiard_period_ledger periods;
    billiard_deformed_surface deformed;
    billiard_flow_event event[BILLIARD_EXPLORER_MAX_EVENTS];
    double distance_start[BILLIARD_EXPLORER_MAX_EVENTS], length[BILLIARD_EXPLORER_MAX_EVENTS], total_length;
    size_t event_count, cylinder_count;
    enum billiard_flow_status flow_status;
    enum billiard_cylinder_status cylinder_status;
    exact_number source_width, source_height;
    int geometry_valid, polygon_vertex_stop, periodic;
} billiard_explorer_model;
typedef struct {
    double origin_x, origin_y, scale;
    double start_x, start_y, handle_x, handle_y, wall_x, wall_y;
    unsigned width, height;
} billiard_explorer_layout;

billiard_explorer_config billiard_explorer_defaults(enum billiard_fixture fixture, enum billiard_explorer_view view);
int billiard_explorer_build(const billiard_explorer_config *config, billiard_explorer_model *model);
int billiard_explorer_scene(FILE *output, unsigned width, unsigned height, const billiard_explorer_model *model,
    double distance, billiard_explorer_layout *layout);
int billiard_explorer_chrome(FILE *output, unsigned width, unsigned height, const billiard_explorer_config *config,
    const billiard_explorer_model *model, int playing, double speed);
int billiard_explorer_drag(const billiard_explorer_model *model, const billiard_explorer_layout *layout,
    enum billiard_explorer_handle handle, double pixel_x, double pixel_y, billiard_explorer_config *config);
int billiard_distance_clock(double now, int playing, double speed, double *previous_time, double *distance);
#endif
