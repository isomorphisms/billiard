#include "explorer.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

static double clamp(double value, double lower, double upper) { return fmax(lower,fmin(upper,value)); }
static billiard_numeric_point numeric(exact_point point) {
    return (billiard_numeric_point){billiard_exact_display(point.x),billiard_exact_display(point.y)};
}

billiard_explorer_config billiard_explorer_defaults(enum billiard_fixture fixture, enum billiard_explorer_view view) {
    billiard_explorer_config config = {fixture,view,3,12,12,16,BILLIARD_EXPLORER_MAX_EVENTS,0,1};
    if (fixture == BILLIARD_RECTANGLE) config.start_vertical = 16;
    if (fixture == BILLIARD_RIGHT_TRIANGLE) config.start_horizontal = config.start_vertical = 8;
    if (fixture == BILLIARD_EQUILATERAL) config.start_vertical = 8;
    if (fixture == BILLIARD_L_POLYGON) config.start_horizontal = config.start_vertical = 6;
    if (view == BILLIARD_EXPLORER_LEGACY) config.show_cylinders = 0;
    return config;
}

int billiard_explorer_build(const billiard_explorer_config *config, billiard_explorer_model *model) {
    if (!config || !model || config->fixture < 0 || config->fixture >= BILLIARD_FIXTURE_COUNT ||
        config->view < BILLIARD_EXPLORER_POLYGON || config->view > BILLIARD_EXPLORER_COVER || config->heading >= 24 ||
        !config->start_horizontal || config->start_horizontal >= 24 || !config->start_vertical || config->start_vertical >= 24 ||
        config->width_ticks < 8 || config->width_ticks > 24 || !config->event_limit || config->event_limit > BILLIARD_EXPLORER_MAX_EVENTS ||
        config->shear_half_steps < -1 || config->shear_half_steps > 2 ||
        (config->view != BILLIARD_EXPLORER_COVER && config->shear_half_steps) ||
        (config->show_cylinders != 0 && config->show_cylinders != 1)) return -1;
    billiard_explorer_model proposed = {0}; proposed.config = *config;
    proposed.flow_status = BILLIARD_FLOW_INVALID; proposed.cylinder_status = BILLIARD_CYLINDER_UNDETERMINED;
    if (billiard_polygon_fixture(config->fixture,&proposed.polygon) != BILLIARD_POLYGON_OK) return -1;
    if (config->fixture == BILLIARD_RECTANGLE) {
        exact_number width;
        if (billiard_exact_make((int32_t)(5*config->width_ticks),0,16,&width)) return -1;
        proposed.polygon.vertex[1].x = proposed.polygon.vertex[2].x = width;
    }
    exact_work work = {0}; proposed.source_width = proposed.source_height = exact_integer(0);
    for (size_t vertex = 0; vertex < proposed.polygon.count; ++vertex) {
        if (exact_order(&work,proposed.polygon.vertex[vertex].x,proposed.source_width) > 0) proposed.source_width = proposed.polygon.vertex[vertex].x;
        if (exact_order(&work,proposed.polygon.vertex[vertex].y,proposed.source_height) > 0) proposed.source_height = proposed.polygon.vertex[vertex].y;
    }
    if (work.failed || billiard_surface_unfold(&proposed.polygon,&proposed.surface) != BILLIARD_SURFACE_OK ||
        billiard_periods(&proposed.surface,&proposed.periods) != BILLIARD_PERIOD_OK) return -1;
    billiard_affine_matrix shear;
    if (billiard_affine_shear(0.5*config->shear_half_steps,&shear) != BILLIARD_AFFINE_OK ||
        billiard_affine_apply(&proposed.surface,&proposed.periods,shear,&proposed.deformed) != BILLIARD_AFFINE_OK) return -1;
    proposed.geometry_valid = 1;
    exact_number horizontal_fraction, vertical_fraction;
    if (billiard_exact_make((int32_t)config->start_horizontal,0,24,&horizontal_fraction) ||
        billiard_exact_make((int32_t)config->start_vertical,0,24,&vertical_fraction)) return -1;
    exact_point position = {exact_product(&work,proposed.source_width,horizontal_fraction),
        exact_product(&work,proposed.source_height,vertical_fraction)}, direction;
    if (work.failed || billiard_polygon_direction(config->heading,&direction)) return -1;
    billiard_flow flow;
    enum billiard_flow_status status = billiard_flow_begin(&proposed.surface,0,position,direction,exact_integer(100000),config->event_limit,&flow);
    if (status != BILLIARD_FLOW_OK) { proposed.flow_status = status; *model = proposed; return -1; }
    for (unsigned step = 0; step < config->event_limit; ++step) {
        billiard_flow_event event;
        status = billiard_flow_next(&proposed.surface,&flow,&event);
        if (status != BILLIARD_FLOW_EDGE && status != BILLIARD_FLOW_REGULAR_VERTEX && status != BILLIARD_FLOW_SINGULAR &&
            status != BILLIARD_FLOW_PERIODIC && status != BILLIARD_FLOW_DISTANCE_LIMIT) break;
        exact_point displacement = exact_point_difference(&work,event.developed_end,event.developed_start);
        billiard_numeric_point transformed = billiard_affine_point(shear,numeric(displacement));
        double length = hypot(transformed.x,transformed.y);
        if (work.failed || !isfinite(length) || length <= 0 || !isfinite(proposed.total_length+length)) {
            status = BILLIARD_FLOW_ARITHMETIC_LIMIT; break;
        }
        size_t index = proposed.event_count++;
        proposed.event[index] = event; proposed.distance_start[index] = proposed.total_length;
        proposed.length[index] = length; proposed.total_length += length;
        if (config->view == BILLIARD_EXPLORER_POLYGON && event.vertex_class != SIZE_MAX) {
            proposed.polygon_vertex_stop = 1; break;
        }
        if (status == BILLIARD_FLOW_PERIODIC) { proposed.periodic = 1; break; }
        if (status == BILLIARD_FLOW_SINGULAR || status == BILLIARD_FLOW_DISTANCE_LIMIT) break;
    }
    if (!proposed.polygon_vertex_stop && !proposed.periodic && (status == BILLIARD_FLOW_EDGE || status == BILLIARD_FLOW_REGULAR_VERTEX))
        status = BILLIARD_FLOW_CROSSING_LIMIT;
    proposed.flow_status = status;
    if (config->show_cylinders) {
        billiard_cylinder_report report;
        billiard_cylinder_limits limits = billiard_cylinder_default_limits();
        proposed.cylinder_status = billiard_cylinders(&proposed.surface,direction,&limits,&report);
        if (proposed.cylinder_status == BILLIARD_CYLINDER_CERTIFIED) proposed.cylinder_count = report.cylinder_count;
    }
    *model = proposed;
    return 0;
}

int billiard_distance_clock(double now, int playing, double speed, double *previous, double *distance) {
    if (!previous || !distance || !isfinite(now) || !isfinite(*previous) || !isfinite(*distance) ||
        !isfinite(speed) || speed < 0 || *distance < 0) return -1;
    double elapsed = clamp(now-*previous,0,0.08);
    double proposed = *distance+(playing ? speed*elapsed : 0);
    if (!isfinite(proposed)) return -1;
    *previous = now; *distance = proposed;
    return 0;
}

int billiard_explorer_drag(const billiard_explorer_model *model, const billiard_explorer_layout *layout,
    enum billiard_explorer_handle handle, double pixel_x, double pixel_y, billiard_explorer_config *config) {
    if (!model || !layout || !config || !model->geometry_valid || !isfinite(pixel_x) || !isfinite(pixel_y) ||
        !isfinite(layout->scale) || layout->scale <= 0) return -1;
    double transformed_x = (pixel_x-layout->origin_x)/layout->scale;
    double source_y = (layout->origin_y-pixel_y)/layout->scale;
    double source_x = transformed_x-0.5*config->shear_half_steps*source_y;
    billiard_explorer_config proposed = *config;
    if (handle == BILLIARD_EXPLORER_START) {
        proposed.start_horizontal = (unsigned)lround(clamp(24*source_x/billiard_exact_display(model->source_width),1,23));
        proposed.start_vertical = (unsigned)lround(clamp(24*source_y/billiard_exact_display(model->source_height),1,23));
        exact_work work = {0}; exact_number horizontal, vertical;
        if (billiard_exact_make((int32_t)proposed.start_horizontal,0,24,&horizontal) ||
            billiard_exact_make((int32_t)proposed.start_vertical,0,24,&vertical)) return -1;
        exact_point point = {exact_product(&work,model->source_width,horizontal),exact_product(&work,model->source_height,vertical)};
        int location;
        if (work.failed || billiard_polygon_locate(&model->polygon,point,&location) != BILLIARD_POLYGON_OK) return -1;
        if (location != 1) return 0;
    } else if (handle == BILLIARD_EXPLORER_ANGLE) {
        double start_x = billiard_exact_display(model->source_width)*config->start_horizontal/24;
        double start_y = billiard_exact_display(model->source_height)*config->start_vertical/24;
        if (hypot(source_x-start_x,source_y-start_y) < 1e-8) return 0;
        long heading = lround(atan2(source_y-start_y,source_x-start_x)*12/acos(-1.0));
        proposed.heading = (unsigned)((heading%24+24)%24);
    } else if (handle == BILLIARD_EXPLORER_WIDTH && config->fixture == BILLIARD_RECTANGLE) {
        proposed.width_ticks = (unsigned)lround(clamp(source_x*16/5,8,24));
    } else return -1;
    *config = proposed;
    return 1;
}

/* Small native stroke labels. All output remains Rough's P/A/W/D/M/L stream. */
static void label(FILE *output, double x, double y, double scale, double alpha, const char *text) {
    static const uint8_t glyph[36][7] = {
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14},{14,17,17,15,1,1,14}
    };
    fprintf(output,"P 48 68 80\nA %.3f\nW %.3f\nD 0 0\n",alpha,0.9*scale);
    for (; *text; ++text, x += 6*scale) {
        unsigned char character = (unsigned char)*text;
        if (character >= 'a' && character <= 'z') character -= 'a'-'A';
        uint8_t punctuation[7] = {0}; const uint8_t *rows = punctuation;
        if (character >= 'A' && character <= 'Z') rows = glyph[character-'A'];
        else if (character >= '0' && character <= '9') rows = glyph[26+character-'0'];
        else if (character == '-') punctuation[3] = 14;
        else if (character == '+') { punctuation[2] = punctuation[4] = 4; punctuation[3] = 14; }
        else if (character == '.') punctuation[6] = 4;
        else if (character == ':') punctuation[2] = punctuation[5] = 4;
        else if (character == '/') for (unsigned row = 0; row < 7; ++row) punctuation[row] = (uint8_t)(1u<<(row*4/6));
        else if (character != ' ') { punctuation[0] = 14; punctuation[1] = 17; punctuation[2] = 2; punctuation[3] = 4; punctuation[5] = 4; }
        for (unsigned row = 0; row < 7; ++row)
            for (unsigned column = 0; column < 5; ++column) {
                if (!(rows[row]&(1u<<(4-column)))) continue;
                unsigned start = column;
                while (column+1 < 5 && (rows[row]&(1u<<(3-column)))) ++column;
                fprintf(output,"M %.3f %.3f\nL %.3f %.3f\n",x+start*scale,y+row*scale,x+column*scale,y+row*scale);
            }
    }
}

static billiard_numeric_point project(billiard_affine_matrix matrix, const billiard_explorer_layout *layout, billiard_numeric_point point) {
    point = billiard_affine_point(matrix,point);
    return (billiard_numeric_point){layout->origin_x+layout->scale*point.x,layout->origin_y-layout->scale*point.y};
}
static void line(FILE *output, billiard_numeric_point first, billiard_numeric_point second) {
    fprintf(output,"M %.5f %.5f\nL %.5f %.5f\n",first.x,first.y,second.x,second.y);
}
static void rough_wall(FILE *output, billiard_numeric_point first, billiard_numeric_point second, double seed) {
    double dx = second.x-first.x, dy = second.y-first.y, length = hypot(dx,dy);
    if (!(length > 0)) return;
    unsigned parts = (unsigned)fmax(2,ceil(length/13));
    fprintf(output,"M %.5f %.5f\n",first.x,first.y);
    for (unsigned part = 1; part < parts; ++part) {
        double parameter = (double)part/parts, jitter = 0.75*sin(2.35*part+seed)*sin(parameter*acos(-1.0));
        fprintf(output,"L %.5f %.5f\n",first.x+dx*parameter-dy/length*jitter,first.y+dy*parameter+dx/length*jitter);
    }
    fprintf(output,"L %.5f %.5f\n",second.x,second.y);
}

static void bounds(billiard_numeric_point point, double *left, double *right, double *bottom, double *top) {
    *left = fmin(*left,point.x); *right = fmax(*right,point.x); *bottom = fmin(*bottom,point.y); *top = fmax(*top,point.y);
}

static int unfolded_scene(FILE *output, unsigned width, unsigned height, const billiard_explorer_model *model) {
    if (!model->event_count) return 0;
    size_t count = model->event_count < BILLIARD_EXPLORER_GHOST_EVENTS ? model->event_count : BILLIARD_EXPLORER_GHOST_EVENTS;
    billiard_numeric_point polygon[BILLIARD_EXPLORER_GHOST_EVENTS][BILLIARD_POLYGON_MAX_VERTICES];
    double left = INFINITY, right = -INFINITY, bottom = INFINITY, top = -INFINITY;
    exact_work work = {0};
    for (size_t event = 0; event < count; ++event) {
        const billiard_flow_event *record = &model->event[event];
        exact_point chart_start = exact_transform(&work,model->surface.copy[record->from_copy].placement,record->folded_start);
        billiard_numeric_point chart = numeric(chart_start), developed = numeric(record->developed_start);
        double shift_x = chart.x-developed.x, shift_y = chart.y-developed.y;
        for (size_t vertex = 0; vertex < model->polygon.count; ++vertex) {
            exact_point point = billiard_surface_vertex(&work,&model->surface,record->from_copy,vertex);
            billiard_numeric_point placed = numeric(point); placed.x -= shift_x; placed.y -= shift_y;
            polygon[event][vertex] = billiard_affine_point(model->deformed.matrix,placed);
            bounds(polygon[event][vertex],&left,&right,&bottom,&top);
        }
    }
    if (work.failed || !(right > left) || !(top > bottom)) return -1;
    double upper = height*0.56, lower = height-79.0;
    double scale = fmin((width-34.0)/(right-left),(lower-upper)/(top-bottom));
    if (!isfinite(scale) || scale <= 0) return -1;
    billiard_explorer_layout layout = {0}; layout.scale = scale;
    layout.origin_x = width*0.5-(left+right)*0.5*scale; layout.origin_y = (upper+lower)*0.5+(bottom+top)*0.5*scale;
    billiard_affine_matrix identity; if (billiard_affine(1,0,0,1,&identity) != BILLIARD_AFFINE_OK) return -1;
    static const unsigned palette[6][3] = {{60,130,160},{160,105,65},{110,100,165},{75,145,100},{160,80,110},{95,115,140}};
    for (size_t event = 0; event < count; ++event) {
        size_t copy = model->event[event].from_copy;
        for (size_t side = 0; side < model->polygon.count; ++side) {
            size_t target = model->surface.seam[copy][side].target_copy;
            size_t pair = (copy < target ? copy : target)*model->polygon.count+side;
            unsigned color = (unsigned)(pair%6);
            fprintf(output,"P %u %u %u\nA .18\nW .8\nD 0 0\n",palette[color][0],palette[color][1],palette[color][2]);
            billiard_numeric_point first = project(identity,&layout,polygon[event][side]);
            billiard_numeric_point second = project(identity,&layout,polygon[event][(side+1)%model->polygon.count]);
            line(output,first,second);
            char edge_label[24]; snprintf(edge_label,sizeof edge_label,"%zu:%zu",copy < target ? copy : target,side);
            label(output,(first.x+second.x)*0.5,(first.y+second.y)*0.5,0.55,0.28,edge_label);
        }
    }
    billiard_numeric_point first = numeric(model->event[0].developed_start), last = numeric(model->event[count-1].developed_end);
    first = billiard_affine_point(model->deformed.matrix,first); last = billiard_affine_point(model->deformed.matrix,last);
    first = project(identity,&layout,first); last = project(identity,&layout,last);
    fprintf(output,"P 30 112 170\nA .34\nW 1.1\nD 1 6\n"); line(output,first,last);
    char caption[64]; snprintf(caption,sizeof caption,"UNFOLD %zu EVENTS / PAIRED SIDES",count);
    label(output,18,height*0.535,0.85,0.60,caption);
    return 0;
}

int billiard_explorer_scene(FILE *output, unsigned width, unsigned height, const billiard_explorer_model *model,
    double distance, billiard_explorer_layout *layout) {
    if (!output || !model || !layout || !model->geometry_valid || model->event_count > BILLIARD_EXPLORER_MAX_EVENTS ||
        model->polygon.count > BILLIARD_POLYGON_MAX_VERTICES || width < 160 || height < 320 || width > 4096 || height > 4096 ||
        !isfinite(distance) || distance < 0) return -1;
    double left = INFINITY, right = -INFINITY, bottom = INFINITY, top = -INFINITY;
    for (size_t vertex = 0; vertex < model->polygon.count; ++vertex)
        bounds(model->deformed.vertex[0][vertex],&left,&right,&bottom,&top);
    if (model->config.fixture == BILLIARD_RECTANGLE) {
        const billiard_numeric_point domain[] = {{0,0},{7.5,0},{7.5,3},{0,3}};
        left = bottom = INFINITY; right = top = -INFINITY;
        for (unsigned vertex = 0; vertex < 4; ++vertex)
            bounds(billiard_affine_point(model->deformed.matrix,domain[vertex]),&left,&right,&bottom,&top);
    }
    double upper = 91, lower = height*0.44;
    double scale = fmin((width-40.0)/(right-left),(lower-upper)/(top-bottom));
    if (!(scale > 0) || !isfinite(scale)) return -1;
    *layout = (billiard_explorer_layout){0}; layout->scale = scale; layout->width = width; layout->height = height;
    layout->origin_x = width*0.5-(left+right)*0.5*scale;
    layout->origin_y = (upper+lower)*0.5+(bottom+top)*0.5*scale;
    billiard_affine_matrix matrix = model->deformed.matrix;
    fprintf(output,"P 52 68 78\nA .8\nW 1.3\nD 0 0\n");
    for (size_t side = 0; side < model->polygon.count; ++side) {
        billiard_numeric_point first = project(matrix,layout,numeric(model->polygon.vertex[side]));
        billiard_numeric_point second = project(matrix,layout,numeric(model->polygon.vertex[(side+1)%model->polygon.count]));
        rough_wall(output,first,second,(double)side+1);
    }
    billiard_numeric_point initial = {billiard_exact_display(model->source_width)*model->config.start_horizontal/24,
        billiard_exact_display(model->source_height)*model->config.start_vertical/24};
    billiard_numeric_point start = project(matrix,layout,initial);
    layout->start_x = start.x; layout->start_y = start.y;
    fprintf(output,"P 28 107 171\nA .32\nW 1.15\nD 1.05 6.0\nM %.5f %.5f\n",start.x,start.y);
    double visible = model->periodic && distance >= model->total_length ? model->total_length : fmin(distance,model->total_length);
    for (size_t event = 0; event < model->event_count; ++event) {
        if (visible <= model->distance_start[event]) break;
        double part = clamp((visible-model->distance_start[event])/model->length[event],0,1);
        billiard_numeric_point first = numeric(model->event[event].folded_start), end = numeric(model->event[event].folded_end);
        end.x = first.x+part*(end.x-first.x); end.y = first.y+part*(end.y-first.y);
        end = project(matrix,layout,end); fprintf(output,"L %.5f %.5f\n",end.x,end.y);
        if (part < 1) break;
    }
    double phase = model->periodic && model->total_length > 0 ? fmod(distance,model->total_length) : fmin(distance,model->total_length);
    billiard_numeric_point tip = start;
    for (size_t event = 0; event < model->event_count; ++event) {
        if (phase > model->distance_start[event]+model->length[event] && event+1 < model->event_count) continue;
        double part = clamp((phase-model->distance_start[event])/model->length[event],0,1);
        billiard_numeric_point first = numeric(model->event[event].folded_start), end = numeric(model->event[event].folded_end);
        tip = project(matrix,layout,(billiard_numeric_point){first.x+part*(end.x-first.x),first.y+part*(end.y-first.y)}); break;
    }
    exact_point direction; if (billiard_polygon_direction(model->config.heading,&direction)) return -1;
    billiard_numeric_point arrow = billiard_affine_point(matrix,numeric(direction)); double arrow_norm = hypot(arrow.x,arrow.y);
    layout->handle_x = start.x+29*arrow.x/arrow_norm; layout->handle_y = start.y-29*arrow.y/arrow_norm;
    fprintf(output,"P 80 110 130\nA .75\nW 1\nD 0 0\n"); line(output,start,(billiard_numeric_point){layout->handle_x,layout->handle_y});
    fprintf(output,"P 220 120 40\nA .95\nW 4\n"); line(output,tip,tip);
    fprintf(output,"P 35 120 145\nA .9\nW 3.5\n"); line(output,start,start);
    layout->wall_x = layout->wall_y = -1000;
    if (model->config.fixture == BILLIARD_RECTANGLE) {
        billiard_numeric_point wall = project(matrix,layout,(billiard_numeric_point){billiard_exact_display(model->source_width),1.5});
        layout->wall_x = wall.x; layout->wall_y = wall.y;
        fprintf(output,"P 65 90 120\nW 5\n"); line(output,wall,wall);
    }
    const char *status = model->polygon_vertex_stop ? "VERTEX STOP" : (model->periodic ? "PERIODIC" :
        (model->flow_status == BILLIARD_FLOW_SINGULAR ? "CONE STOP" : (model->flow_status == BILLIARD_FLOW_INVALID ? "START OUTSIDE" : "UNKNOWN LIMIT")));
    char topology[64]; snprintf(topology,sizeof topology,"COVER G%u Z%zu %s",model->surface.genus,model->periods.zero_count,status);
    label(output,20,lower+13,1,0.9,topology);
    if (model->deformed.tau_valid) snprintf(topology,sizeof topology,"TORUS %.2f + %.2f I",model->deformed.tau_real,model->deformed.tau_imaginary);
    else snprintf(topology,sizeof topology,"PERIODS %zu / ZERO ORDER %u",model->periods.period_count,
        model->periods.zero_count ? model->periods.zero_order[0] : 0);
    label(output,20,lower+25,0.85,0.65,topology);
    if (model->config.show_cylinders) {
        if (model->cylinder_status == BILLIARD_CYLINDER_CERTIFIED) snprintf(topology,sizeof topology,"COVER CYL %zu",model->cylinder_count);
        else snprintf(topology,sizeof topology,"CYL UNKNOWN");
        label(output,width-112,lower+25,0.85,0.65,topology);
    }
    label(output,20,76,1,0.8,model->config.shear_half_steps ? "AFFINE COVER / FOLDED PROJECTION" :
        (model->config.view == BILLIARD_EXPLORER_COVER ? "COVER FLOW" : "POLYGON / VERTEX STOP"));
    if (unfolded_scene(output,width,height,model)) label(output,20,height*0.56,1,0.7,"UNFOLD DISPLAY LIMIT");
    return ferror(output) ? -1 : 0;
}

int billiard_explorer_chrome(FILE *output, unsigned width, unsigned height, const billiard_explorer_config *config,
    const billiard_explorer_model *model, int playing, double speed) {
    if (!output || !config || width < 160 || height < 320 || config->fixture < 0 || config->fixture >= BILLIARD_FIXTURE_COUNT ||
        config->view < BILLIARD_EXPLORER_LEGACY || config->view > BILLIARD_EXPLORER_COVER) return -1;
    fprintf(output,"P 55 85 100\nA .8\nW 2\nD 0 0\n");
    if (playing) { line(output,(billiard_numeric_point){16,16},(billiard_numeric_point){16,30}); line(output,(billiard_numeric_point){23,16},(billiard_numeric_point){23,30}); }
    else { line(output,(billiard_numeric_point){16,16},(billiard_numeric_point){28,23}); line(output,(billiard_numeric_point){28,23},(billiard_numeric_point){16,30}); }
    label(output,35,20,0.9,0.8,playing ? "PAUSE" : "PLAY");
    char text[48]; snprintf(text,sizeof text,"SPEED %.1f",speed); label(output,width-100,20,0.9,0.8,text);
    static const char *const tabs[] = {"SQR","RECT","RIGHT","EQUI","L"};
    for (unsigned tab = 0; tab < BILLIARD_FIXTURE_COUNT; ++tab) {
        label(output,(double)width*tab/BILLIARD_FIXTURE_COUNT+5,46,0.9,tab == (unsigned)config->fixture ? 0.95 : 0.48,tabs[tab]);
        if (tab == (unsigned)config->fixture) {
            fprintf(output,"P 40 110 145\nA .8\nW 1.5\nD 0 0\n");
            line(output,(billiard_numeric_point){(double)width*tab/BILLIARD_FIXTURE_COUNT+4,60},
                (billiard_numeric_point){(double)width*(tab+1)/BILLIARD_FIXTURE_COUNT-4,60});
        }
    }
    if (config->view == BILLIARD_EXPLORER_LEGACY) label(output,18,height-67,0.9,0.8,"RECTANGLE / CORNER REVERSE");
    const char *views[] = {"VIEW LEGACY","VIEW POLY","VIEW COVER"};
    label(output,9,height-43,0.85,0.85,config->view <= BILLIARD_EXPLORER_COVER ? views[config->view] : "VIEW UNKNOWN");
    snprintf(text,sizeof text,"SHEAR %.1f",0.5*config->shear_half_steps); label(output,width/3.0+6,height-43,0.85,0.8,text);
    label(output,2*width/3.0+6,height-43,0.85,0.8,config->show_cylinders ? "CYL ON" : "CYL OFF");
    label(output,9,height-21,0.7,0.60,config->view == BILLIARD_EXPLORER_LEGACY ? "DRAG START / ANGLE / RIGHT WALL" : "DRAG START GRID 1/24 / ANGLE 15 DEG");
    (void)model;
    return ferror(output) ? -1 : 0;
}
