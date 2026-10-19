#include "cylinders.h"
#include <limits.h>
#include <math.h>
#include <string.h>

typedef struct {
    exact_work field;
    billiard_exact_matrix frame;
    exact_point vertex[BILLIARD_SURFACE_MAX_COPIES][BILLIARD_POLYGON_MAX_VERTICES];
    exact_point seam[BILLIARD_SURFACE_MAX_COPIES][BILLIARD_POLYGON_MAX_VERTICES];
    exact_number unit, norm;
} direction_geometry;
typedef struct { size_t side; exact_number middle, lower, upper; } line_hit;
typedef struct { exact_number parameter; size_t copy, vertex, vertex_class; } boundary_vertex;

static unsigned divisor(unsigned first, unsigned second) {
    while (second) { unsigned remainder = first%second; first = second; second = remainder; }
    return first;
}
static unsigned magnitude32(int32_t number) { return number < 0 ? (unsigned)(-number) : (unsigned)number; }

billiard_cylinder_limits billiard_cylinder_default_limits(void) {
    return (billiard_cylinder_limits){BILLIARD_MAX_STRIPS,BILLIARD_MAX_STRIPS,BILLIARD_MAX_CONNECTIONS,{10,0,1},0};
}

static int include_grid_value(direction_geometry *geometry, exact_number value,
    exact_number *base, unsigned *numerator_gcd, int32_t *common_denominator) {
    exact_work *work = &geometry->field;
    if (!exact_order(work,value,exact_integer(0))) return !work->failed;
    if (!exact_order(work,*base,exact_integer(0))) {
        *base = exact_order(work,value,exact_integer(0)) > 0 ? value : exact_difference(work,exact_integer(0),value);
    }
    exact_number ratio = exact_quotient(work,value,*base);
    if (work->failed || ratio.radical) return 0;
    int32_t factor = ratio.denominator/(int32_t)divisor((unsigned)*common_denominator,(unsigned)ratio.denominator);
    int32_t new_denominator, old_numerator, new_numerator;
    if (__builtin_mul_overflow(*common_denominator,factor,&new_denominator) || new_denominator <= 0 ||
        __builtin_mul_overflow((int32_t)*numerator_gcd,factor,&old_numerator) ||
        __builtin_mul_overflow(ratio.rational,new_denominator/ratio.denominator,&new_numerator) ||
        old_numerator == INT32_MIN || new_numerator == INT32_MIN) { work->failed = 1; return 0; }
    *numerator_gcd = divisor(magnitude32(old_numerator),magnitude32(new_numerator));
    *common_denominator = new_denominator;
    return 1;
}

static enum billiard_cylinder_status prepare(const billiard_surface *surface,
    exact_point direction, direction_geometry *geometry) {
    memset(geometry,0,sizeof *geometry);
    geometry->unit = geometry->norm = exact_integer(0);
    exact_work *work = &geometry->field;
    geometry->frame = exact_identity();
    geometry->frame.entry[0][0] = geometry->frame.entry[1][1] = direction.x;
    geometry->frame.entry[0][1] = direction.y;
    geometry->frame.entry[1][0] = exact_difference(work,exact_integer(0),direction.y);
    geometry->norm = exact_dot(work,direction,direction);
    if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
    if (exact_order(work,geometry->norm,exact_integer(0)) <= 0) return BILLIARD_CYLINDER_INVALID;
    exact_number base = exact_integer(0);
    unsigned numerator_gcd = 0; int32_t common_denominator = 1;
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        for (size_t side = 0; side < surface->polygon.count; ++side) {
            exact_point point = billiard_surface_vertex(work,surface,copy,side);
            geometry->vertex[copy][side] = exact_transform(work,geometry->frame,point);
            geometry->seam[copy][side] = exact_transform(work,geometry->frame,surface->seam[copy][side].translation);
            if (!include_grid_value(geometry,geometry->vertex[copy][side].y,&base,&numerator_gcd,&common_denominator) ||
                !include_grid_value(geometry,geometry->seam[copy][side].y,&base,&numerator_gcd,&common_denominator))
                return work->failed ? BILLIARD_CYLINDER_ARITHMETIC_LIMIT : BILLIARD_CYLINDER_UNDETERMINED;
        }
    exact_number factor;
    if (!numerator_gcd || billiard_exact_make((int32_t)numerator_gcd,0,common_denominator,&factor)) return BILLIARD_CYLINDER_INVALID;
    geometry->unit = exact_product(work,base,factor);
    return work->failed ? BILLIARD_CYLINDER_ARITHMETIC_LIMIT : BILLIARD_CYLINDER_CERTIFIED;
}

static exact_number level_height(direction_geometry *geometry, int32_t level) {
    return exact_product(&geometry->field,geometry->unit,exact_integer(level));
}
static exact_number middle_height(direction_geometry *geometry, int32_t level) {
    exact_number half = {1,0,2};
    exact_number middle = exact_sum(&geometry->field,exact_integer(level),half);
    return exact_product(&geometry->field,geometry->unit,middle);
}
static exact_number intersection(direction_geometry *geometry, exact_point start,
    exact_point end, exact_number height) {
    exact_work *work = &geometry->field;
    exact_number rise = exact_difference(work,end.y,start.y);
    exact_number run = exact_difference(work,end.x,start.x);
    exact_number parameter = exact_difference(work,height,start.y);
    parameter = exact_quotient(work,parameter,rise);
    exact_number offset = exact_product(work,parameter,run);
    return exact_sum(work,start.x,offset);
}

static enum billiard_cylinder_status build_strips(const billiard_surface *surface,
    direction_geometry *geometry, billiard_cylinder_report *report) {
    exact_work *work = &geometry->field;
    for (size_t copy = 0; copy < surface->copy_count; ++copy) {
        int32_t lowest = INT32_MAX, highest = -INT32_MAX;
        for (size_t vertex = 0; vertex < surface->polygon.count; ++vertex) {
            exact_number index = exact_quotient(work,geometry->vertex[copy][vertex].y,geometry->unit);
            if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
            if (index.radical || index.denominator != 1) return BILLIARD_CYLINDER_UNDETERMINED;
            if (index.rational < lowest) lowest = index.rational;
            if (index.rational > highest) highest = index.rational;
        }
        if ((int64_t)highest-lowest > (int64_t)report->limits.max_strips) return BILLIARD_CYLINDER_LIMIT;
        for (int32_t level = lowest; level < highest; ++level) {
            exact_number lower = level_height(geometry,level), upper = level_height(geometry,level+1);
            exact_number middle = middle_height(geometry,level);
            line_hit hit[BILLIARD_POLYGON_MAX_VERTICES]; size_t hits = 0;
            for (size_t side = 0; side < surface->polygon.count; ++side) {
                exact_point start = geometry->vertex[copy][side], end = geometry->vertex[copy][(side+1)%surface->polygon.count];
                int below_start = exact_order(work,start.y,middle), below_end = exact_order(work,end.y,middle);
                if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
                if (below_start*below_end >= 0) continue;
                line_hit entry;
                entry.side = side;
                entry.middle = intersection(geometry,start,end,middle);
                entry.lower = intersection(geometry,start,end,lower);
                entry.upper = intersection(geometry,start,end,upper);
                size_t insertion = hits++;
                while (insertion && exact_order(work,entry.middle,hit[insertion-1].middle) < 0) {
                    hit[insertion] = hit[insertion-1]; --insertion;
                }
                hit[insertion] = entry;
            }
            if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
            if (hits%2) return BILLIARD_CYLINDER_INVALID;
            for (size_t pair = 0; pair < hits; pair += 2) {
                if (report->strip_count >= report->limits.max_strips) return BILLIARD_CYLINDER_LIMIT;
                if (exact_order(work,hit[pair].middle,hit[pair+1].middle) >= 0 ||
                    exact_order(work,hit[pair].lower,hit[pair+1].lower) > 0 ||
                    exact_order(work,hit[pair].upper,hit[pair+1].upper) > 0) return BILLIARD_CYLINDER_INVALID;
                report->strip[report->strip_count++] = (billiard_direction_strip){copy,hit[pair].side,hit[pair+1].side,
                    SIZE_MAX,SIZE_MAX,level,hit[pair].lower,hit[pair].upper,hit[pair+1].lower,hit[pair+1].upper};
            }
        }
    }
    if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
    unsigned incoming[BILLIARD_MAX_STRIPS] = {0};
    for (size_t index = 0; index < report->strip_count; ++index) {
        billiard_direction_strip *strip = &report->strip[index];
        size_t target_copy = surface->seam[strip->copy][strip->right_side].target_copy;
        exact_point translation = geometry->seam[strip->copy][strip->right_side];
        exact_number lower = level_height(geometry,strip->level);
        lower = exact_sum(work,lower,translation.y);
        exact_number target_lower = exact_sum(work,strip->right_lower,translation.x);
        exact_number target_upper = exact_sum(work,strip->right_upper,translation.x);
        for (size_t candidate = 0; candidate < report->strip_count; ++candidate) {
            billiard_direction_strip *next = &report->strip[candidate];
            if (next->copy != target_copy || next->left_side != strip->right_side) continue;
            exact_number next_lower = level_height(geometry,next->level);
            if (billiard_exact_equal(lower,next_lower) && billiard_exact_equal(target_lower,next->left_lower) &&
                billiard_exact_equal(target_upper,next->left_upper)) {
                if (strip->next != SIZE_MAX) return BILLIARD_CYLINDER_INVALID;
                strip->next = candidate;
            }
        }
        if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
        if (strip->next == SIZE_MAX) return BILLIARD_CYLINDER_INVALID;
        ++incoming[strip->next];
    }
    for (size_t strip = 0; strip < report->strip_count; ++strip)
        if (incoming[strip] != 1) return BILLIARD_CYLINDER_INVALID;
    return BILLIARD_CYLINDER_CERTIFIED;
}

static int distinguished(const billiard_surface *surface, const billiard_cylinder_report *report,
    size_t copy, size_t vertex) {
    return report->limits.mark_regular_vertices || surface->cone[surface->vertex_class[copy][vertex]].zero_order > 0;
}

static int boundary_has_vertex(const billiard_surface *surface, direction_geometry *geometry,
    const billiard_cylinder_report *report, size_t first, int upper) {
    exact_work *work = &geometry->field;
    size_t current = first;
    do {
        const billiard_direction_strip *strip = &report->strip[current];
        exact_number height = level_height(geometry,strip->level+upper);
        exact_number left = upper ? strip->left_upper : strip->left_lower;
        exact_number right = upper ? strip->right_upper : strip->right_lower;
        for (size_t vertex = 0; vertex < surface->polygon.count; ++vertex) {
            exact_point point = geometry->vertex[strip->copy][vertex];
            if (distinguished(surface,report,strip->copy,vertex) && billiard_exact_equal(point.y,height) &&
                exact_order(work,point.x,left) >= 0 && exact_order(work,point.x,right) <= 0) return 1;
        }
        current = strip->next;
    } while (current != first);
    return 0;
}

static size_t root(size_t *parent, size_t index) {
    while (parent[index] != index) { parent[index] = parent[parent[index]]; index = parent[index]; }
    return index;
}

static int strictly_between(exact_work *work, exact_number point, exact_number first, exact_number second) {
    return exact_order(work,point,first) > 0 && exact_order(work,point,second) < 0;
}

static size_t adjacent_strip(const billiard_surface *surface, direction_geometry *geometry,
    const billiard_cylinder_report *report, size_t index, int upper) {
    const billiard_direction_strip *strip = &report->strip[index];
    exact_work *work = &geometry->field;
    exact_number left = upper ? strip->left_upper : strip->left_lower;
    exact_number right = upper ? strip->right_upper : strip->right_lower;
    if (exact_order(work,left,right) >= 0) return SIZE_MAX;
    exact_number height = level_height(geometry,strip->level+upper), horizontal = left;
    for (unsigned choice = 2; choice < BILLIARD_POLYGON_MAX_VERTICES+4; ++choice) {
        exact_number width = exact_difference(work,right,left), fraction;
        if (billiard_exact_make(1,0,(int32_t)choice,&fraction)) { work->failed = 1; return SIZE_MAX; }
        width = exact_product(work,width,fraction); horizontal = exact_sum(work,left,width);
        int touches_vertex = 0;
        for (size_t vertex = 0; vertex < surface->polygon.count; ++vertex)
            if (billiard_exact_equal(geometry->vertex[strip->copy][vertex].x,horizontal) &&
                billiard_exact_equal(geometry->vertex[strip->copy][vertex].y,height)) touches_vertex = 1;
        if (!touches_vertex) break;
    }
    size_t copy = strip->copy;
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
        for (size_t candidate = 0; candidate < report->strip_count; ++candidate) {
            const billiard_direction_strip *next = &report->strip[candidate];
            if (next->copy != copy) continue;
            exact_number boundary = level_height(geometry,next->level+(upper ? 0 : 1));
            exact_number next_left = upper ? next->left_lower : next->left_upper;
            exact_number next_right = upper ? next->right_lower : next->right_upper;
            if (billiard_exact_equal(boundary,height) && strictly_between(work,horizontal,next_left,next_right)) return candidate;
        }
        if (attempt) break;
        int crossed = 0;
        for (size_t side = 0; side < surface->polygon.count; ++side) {
            exact_point first = geometry->vertex[copy][side], second = geometry->vertex[copy][(side+1)%surface->polygon.count];
            if (!billiard_exact_equal(first.y,height) || !billiard_exact_equal(second.y,height)) continue;
            if (!(strictly_between(work,horizontal,first.x,second.x) || strictly_between(work,horizontal,second.x,first.x))) continue;
            exact_point translation = geometry->seam[copy][side];
            horizontal = exact_sum(work,horizontal,translation.x); height = exact_sum(work,height,translation.y);
            copy = surface->seam[copy][side].target_copy; crossed = 1; break;
        }
        if (!crossed) break;
    }
    return SIZE_MAX;
}

static enum billiard_cylinder_status assemble_cylinders(const billiard_surface *surface,
    direction_geometry *geometry, billiard_cylinder_report *report,
    size_t *cycle_of, size_t *seeds, size_t *cycle_count) {
    exact_work *work = &geometry->field;
    size_t parent[BILLIARD_MAX_STRIPS], final_number[BILLIARD_MAX_STRIPS];
    exact_number period[BILLIARD_MAX_STRIPS], area[BILLIARD_MAX_STRIPS];
    unsigned cycle_size[BILLIARD_MAX_STRIPS];
    for (size_t strip = 0; strip < report->strip_count; ++strip) cycle_of[strip] = SIZE_MAX;
    *cycle_count = 0;
    for (size_t first = 0; first < report->strip_count; ++first) {
        if (cycle_of[first] != SIZE_MAX) continue;
        size_t cycle = (*cycle_count)++, current = first;
        seeds[cycle] = first; parent[cycle] = cycle; final_number[cycle] = SIZE_MAX;
        exact_number lower_width = exact_integer(0), upper_width = exact_integer(0);
        cycle_size[cycle] = 0;
        do {
            if (cycle_of[current] != SIZE_MAX) return BILLIARD_CYLINDER_INVALID;
            if (++cycle_size[cycle] > report->limits.max_crossings) return BILLIARD_CYLINDER_LIMIT;
            cycle_of[current] = cycle;
            const billiard_direction_strip *strip = &report->strip[current];
            exact_number lower = exact_difference(work,strip->right_lower,strip->left_lower);
            exact_number upper = exact_difference(work,strip->right_upper,strip->left_upper);
            lower_width = exact_sum(work,lower_width,lower); upper_width = exact_sum(work,upper_width,upper);
            current = strip->next;
        } while (current != first);
        if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
        if (!billiard_exact_equal(lower_width,upper_width) || exact_order(work,lower_width,exact_integer(0)) <= 0)
            return BILLIARD_CYLINDER_INVALID;
        period[cycle] = exact_quotient(work,lower_width,geometry->norm);
        area[cycle] = exact_product(work,lower_width,geometry->unit);
        area[cycle] = exact_quotient(work,area[cycle],geometry->norm);
    }
    for (size_t cycle = 0; cycle < *cycle_count; ++cycle)
        for (int upper = 0; upper < 2; ++upper) {
            if (boundary_has_vertex(surface,geometry,report,seeds[cycle],upper)) continue;
            size_t current = seeds[cycle];
            do {
                const billiard_direction_strip *strip = &report->strip[current];
                exact_number left = upper ? strip->left_upper : strip->left_lower;
                exact_number right = upper ? strip->right_upper : strip->right_lower;
                if (exact_order(work,left,right) < 0) {
                    size_t neighbor = adjacent_strip(surface,geometry,report,current,upper);
                    if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
                    if (neighbor == SIZE_MAX || !billiard_exact_equal(period[cycle],period[cycle_of[neighbor]]))
                        return BILLIARD_CYLINDER_INVALID;
                    size_t first_root = root(parent,cycle), second_root = root(parent,cycle_of[neighbor]);
                    if (first_root != second_root) parent[second_root] = first_root;
                }
                current = strip->next;
            } while (current != seeds[cycle]);
        }
    report->area_covered = exact_integer(0);
    for (size_t cycle = 0; cycle < *cycle_count; ++cycle) {
        size_t representative = root(parent,cycle);
        if (final_number[representative] == SIZE_MAX) {
            size_t number = report->cylinder_count++;
            final_number[representative] = number;
            billiard_cylinder *cylinder = &report->cylinder[number];
            cylinder->period_parameter = period[cycle]; cylinder->area = exact_integer(0);
            cylinder->first_strip = seeds[cycle];
            cylinder->holonomy = exact_point_scale(work,report->direction,period[cycle]);
            cylinder->circumference_squared = exact_dot(work,cylinder->holonomy,cylinder->holonomy);
        }
        billiard_cylinder *cylinder = &report->cylinder[final_number[representative]];
        cylinder->area = exact_sum(work,cylinder->area,area[cycle]);
        cylinder->strip_count += cycle_size[cycle];
        report->area_covered = exact_sum(work,report->area_covered,area[cycle]);
    }
    exact_number length_squared = exact_product(work,report->limits.length_bound,report->limits.length_bound);
    for (size_t cylinder = 0; cylinder < report->cylinder_count; ++cylinder) {
        billiard_cylinder *record = &report->cylinder[cylinder];
        record->within_length_bound = exact_order(work,record->circumference_squared,length_squared) <= 0;
        report->counted_cylinders += (unsigned)record->within_length_bound;
        record->circumference = sqrt(billiard_exact_display(record->circumference_squared));
        record->height = billiard_exact_display(record->area)/record->circumference;
        if (!isfinite(record->circumference) || record->circumference <= 0 || !isfinite(record->height) || record->height <= 0)
            return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
    }
    for (size_t strip = 0; strip < report->strip_count; ++strip)
        report->strip[strip].cylinder = final_number[root(parent,cycle_of[strip])];
    if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
    return billiard_exact_equal(report->area_covered,surface->area) ? BILLIARD_CYLINDER_CERTIFIED : BILLIARD_CYLINDER_INVALID;
}

static size_t ray_germ(const billiard_surface *surface, direction_geometry *geometry,
    exact_point global_direction, size_t copy, size_t vertex) {
    exact_work *work = &geometry->field;
    size_t germ = copy*surface->polygon.count+vertex;
    exact_point direction = exact_transform(work,exact_transpose(surface->copy[copy].placement),global_direction);
    size_t neighbor_vertex[2] = {(vertex+1)%surface->polygon.count,(vertex+surface->polygon.count-1)%surface->polygon.count};
    size_t incident_side[2] = {vertex,(vertex+surface->polygon.count-1)%surface->polygon.count};
    for (unsigned side = 0; side < 2; ++side) {
        exact_point edge = exact_point_difference(work,surface->polygon.vertex[neighbor_vertex[side]],surface->polygon.vertex[vertex]);
        exact_number crossing = exact_cross(work,direction,edge), projection = exact_dot(work,direction,edge);
        if (!exact_order(work,crossing,exact_integer(0)) && exact_order(work,projection,exact_integer(0)) > 0) {
            size_t adjacent = surface->seam[copy][incident_side[side]].target_copy*surface->polygon.count+vertex;
            if (adjacent < germ) germ = adjacent;
        }
    }
    return germ;
}

static enum billiard_cylinder_status boundary_observations(const billiard_surface *surface,
    direction_geometry *geometry, const billiard_cylinder_report *report, size_t seed, int upper,
    boundary_vertex *observations, size_t *count, exact_number *period, unsigned *crossings) {
    exact_work *work = &geometry->field;
    size_t current = seed; *count = 0; *crossings = 0;
    exact_number elapsed = exact_integer(0);
    do {
        const billiard_direction_strip *strip = &report->strip[current];
        exact_number height = level_height(geometry,strip->level+upper);
        exact_number left = upper ? strip->left_upper : strip->left_lower;
        exact_number right = upper ? strip->right_upper : strip->right_lower;
        boundary_vertex found[BILLIARD_POLYGON_MAX_VERTICES]; size_t found_count = 0;
        for (size_t vertex = 0; vertex < surface->polygon.count; ++vertex) {
            exact_point point = geometry->vertex[strip->copy][vertex];
            if (!distinguished(surface,report,strip->copy,vertex) || !billiard_exact_equal(point.y,height) ||
                exact_order(work,point.x,left) < 0 || exact_order(work,point.x,right) >= 0) continue;
            exact_number offset = exact_difference(work,point.x,left);
            offset = exact_quotient(work,offset,geometry->norm);
            boundary_vertex entry = {exact_sum(work,elapsed,offset),strip->copy,vertex,surface->vertex_class[strip->copy][vertex]};
            size_t insertion = found_count++;
            while (insertion && exact_order(work,entry.parameter,found[insertion-1].parameter) < 0) {
                found[insertion] = found[insertion-1]; --insertion;
            }
            found[insertion] = entry;
        }
        for (size_t vertex = 0; vertex < found_count; ++vertex) {
            boundary_vertex entry = found[vertex];
            if (*count && billiard_exact_equal(observations[*count-1].parameter,entry.parameter)) {
                if (observations[*count-1].vertex_class != entry.vertex_class) return BILLIARD_CYLINDER_INVALID;
                if (entry.copy*surface->polygon.count+entry.vertex < observations[*count-1].copy*surface->polygon.count+observations[*count-1].vertex)
                    observations[*count-1] = entry;
            } else {
                if (*count >= BILLIARD_MAX_CONNECTIONS) return BILLIARD_CYLINDER_LIMIT;
                observations[(*count)++] = entry;
            }
        }
        exact_number width = exact_difference(work,right,left);
        width = exact_quotient(work,width,geometry->norm);
        elapsed = exact_sum(work,elapsed,width);
        ++*crossings;
        current = strip->next;
    } while (current != seed);
    if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
    if (*count && billiard_exact_equal(observations[*count-1].parameter,elapsed)) {
        if (!billiard_exact_equal(observations[0].parameter,exact_integer(0)) ||
            observations[0].vertex_class != observations[*count-1].vertex_class) return BILLIARD_CYLINDER_INVALID;
        --*count;
    }
    *period = elapsed;
    return BILLIARD_CYLINDER_CERTIFIED;
}

static enum billiard_cylinder_status collect_connections(const billiard_surface *surface,
    direction_geometry *geometry, billiard_cylinder_report *report, const size_t *seeds, size_t cycle_count) {
    exact_work *work = &geometry->field;
    exact_number bound_squared = exact_product(work,report->limits.length_bound,report->limits.length_bound);
    for (size_t cycle = 0; cycle < cycle_count; ++cycle)
        for (int upper = 0; upper < 2; ++upper) {
            boundary_vertex observations[BILLIARD_MAX_CONNECTIONS]; size_t count; exact_number period; unsigned crossings;
            enum billiard_cylinder_status status = boundary_observations(surface,geometry,report,seeds[cycle],upper,
                observations,&count,&period,&crossings);
            if (status != BILLIARD_CYLINDER_CERTIFIED) return status;
            for (size_t first = 0; first < count; ++first) {
                size_t next = (first+1)%count;
                exact_number parameter = exact_difference(work,observations[next].parameter,observations[first].parameter);
                if (!next) parameter = exact_sum(work,parameter,period);
                if (exact_order(work,parameter,exact_integer(0)) <= 0) return BILLIARD_CYLINDER_INVALID;
                exact_point holonomy = exact_point_scale(work,report->direction,parameter);
                exact_number squared = exact_dot(work,holonomy,holonomy);
                size_t germ = ray_germ(surface,geometry,report->direction,observations[first].copy,observations[first].vertex);
                if (work->failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
                if (exact_order(work,squared,bound_squared) > 0) { ++report->boundary_segments_over_length; continue; }
                int duplicate = 0;
                for (size_t old = 0; old < report->connection_count; ++old)
                    if (report->connection[old].start_germ == germ && exact_point_equal(report->connection[old].holonomy,holonomy)) duplicate = 1;
                if (duplicate) continue;
                if (report->connection_count >= report->limits.max_connections) return BILLIARD_CYLINDER_LIMIT;
                report->connection[report->connection_count++] = (billiard_connection){observations[first].copy,observations[first].vertex,
                    observations[first].vertex_class,observations[next].vertex_class,germ,parameter,squared,holonomy,2*crossings+2,
                    report->limits.mark_regular_vertices};
            }
        }
    return work->failed ? BILLIARD_CYLINDER_ARITHMETIC_LIMIT : BILLIARD_CYLINDER_CERTIFIED;
}

enum billiard_cylinder_status billiard_cylinders(const billiard_surface *surface,
    exact_point direction, const billiard_cylinder_limits *limits, billiard_cylinder_report *report) {
    if (!report || !limits || !exact_point_valid(direction) || !billiard_exact_valid(limits->length_bound) ||
        !limits->max_strips || limits->max_strips > BILLIARD_MAX_STRIPS || !limits->max_crossings ||
        limits->max_crossings > BILLIARD_MAX_STRIPS || !limits->max_connections || limits->max_connections > BILLIARD_MAX_CONNECTIONS ||
        (limits->mark_regular_vertices != 0 && limits->mark_regular_vertices != 1)) return BILLIARD_CYLINDER_INVALID;
    enum billiard_surface_status surface_status = billiard_surface_validate(surface);
    if (surface_status != BILLIARD_SURFACE_OK)
        return surface_status == BILLIARD_SURFACE_ARITHMETIC_LIMIT ? BILLIARD_CYLINDER_ARITHMETIC_LIMIT : BILLIARD_CYLINDER_INVALID;
    exact_work validation = {0};
    if (exact_order(&validation,limits->length_bound,exact_integer(0)) < 0 || validation.failed) return BILLIARD_CYLINDER_INVALID;
    billiard_cylinder_report proposed = {0}; proposed.direction = direction; proposed.limits = *limits;
    proposed.area_covered = exact_integer(0);
    direction_geometry geometry;
    enum billiard_cylinder_status status = prepare(surface,direction,&geometry);
    proposed.transverse_unit = geometry.unit; proposed.direction_squared = geometry.norm;
    if (status == BILLIARD_CYLINDER_CERTIFIED) status = build_strips(surface,&geometry,&proposed);
    size_t cycle_of[BILLIARD_MAX_STRIPS], seeds[BILLIARD_MAX_STRIPS], cycles = 0;
    if (status == BILLIARD_CYLINDER_CERTIFIED) status = assemble_cylinders(surface,&geometry,&proposed,cycle_of,seeds,&cycles);
    if (status == BILLIARD_CYLINDER_CERTIFIED) status = collect_connections(surface,&geometry,&proposed,seeds,cycles);
    proposed.status = status; *report = proposed;
    return status;
}

int billiard_cylinders_write(FILE *output, const billiard_cylinder_report *report) {
    if (!output || !report || report->status < BILLIARD_CYLINDER_CERTIFIED || report->status > BILLIARD_CYLINDER_INVALID) return -1;
    static const char *const names[] = {"certified_selected_direction","limit_unknown","undetermined","arithmetic_limit_unknown","invalid"};
    fprintf(output,"decomposition\t%s\nlimits\t%u\t%u\t%u\nconvention\t%s\nstrips\t%zu\ncylinders\t%zu\nconnections\t%zu\n",
        names[report->status],report->limits.max_strips,report->limits.max_crossings,report->limits.max_connections,
        report->limits.mark_regular_vertices ? "regular_vertices_marked" : "genuine_cones_only",report->strip_count,report->cylinder_count,report->connection_count);
    const exact_number parameters[] = {report->direction.x,report->direction.y,report->limits.length_bound,
        report->transverse_unit,report->direction_squared,report->area_covered};
    fprintf(output,"direction_bound_grid_norm_area");
    for (unsigned number = 0; number < sizeof parameters/sizeof *parameters; ++number)
        fprintf(output,"\t%d\t%d\t%d",parameters[number].rational,parameters[number].radical,parameters[number].denominator);
    fputc('\n',output);
    for (size_t strip = 0; strip < report->strip_count; ++strip) {
        const billiard_direction_strip *record = &report->strip[strip];
        fprintf(output,"strip\t%zu\t%zu\t%d\t%zu\t%zu\t%zu\t%zu\n",strip,record->copy,record->level,
            record->left_side,record->right_side,record->next,record->cylinder);
        const exact_number bounds[] = {record->left_lower,record->left_upper,record->right_lower,record->right_upper};
        fprintf(output,"strip_bounds\t%zu",strip);
        for (unsigned bound = 0; bound < 4; ++bound)
            fprintf(output,"\t%d\t%d\t%d",bounds[bound].rational,bounds[bound].radical,bounds[bound].denominator);
        fputc('\n',output);
    }
    for (size_t cylinder = 0; cylinder < report->cylinder_count; ++cylinder) {
        const billiard_cylinder *record = &report->cylinder[cylinder];
        fprintf(output,"cylinder\t%zu\t%d\t%d\t%d\t%d\t%d\t%d\t%.17g\t%.17g\t%d\n",cylinder,
            record->period_parameter.rational,record->period_parameter.radical,record->period_parameter.denominator,
            record->area.rational,record->area.radical,record->area.denominator,record->circumference,record->height,record->within_length_bound);
    }
    for (size_t connection = 0; connection < report->connection_count; ++connection) {
        const billiard_connection *record = &report->connection[connection];
        fprintf(output,"connection\t%zu\t%zu\t%zu\t%zu\t%zu\t%zu\t%d\t%d\t%d\t%u\n",connection,
            record->start_copy,record->start_vertex,record->start_class,record->end_class,record->start_germ,
            record->parameter.rational,record->parameter.radical,record->parameter.denominator,record->strip_crossing_bound);
    }
    return ferror(output) ? -1 : 0;
}

enum billiard_cylinder_status billiard_direction_sweep(const billiard_surface *surface,
    unsigned direction_bound, unsigned max_directions, const billiard_cylinder_limits *limits,
    FILE *raw_evidence, billiard_counting_report *report) {
    if (!report || !limits || !direction_bound || direction_bound > 16 || !max_directions ||
        max_directions > BILLIARD_MAX_DIRECTIONS || billiard_surface_validate(surface) != BILLIARD_SURFACE_OK)
        return BILLIARD_CYLINDER_INVALID;
    billiard_counting_report proposed = {0};
    proposed.direction_bound = direction_bound; proposed.limits = *limits;
    int integral_holonomy = 1;
    exact_work work = {0};
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        for (size_t vertex = 0; vertex < surface->polygon.count; ++vertex) {
            exact_point point = billiard_surface_vertex(&work,surface,copy,vertex);
            exact_point seam = surface->seam[copy][vertex].translation;
            if (point.x.radical || point.x.denominator != 1 || point.y.radical || point.y.denominator != 1 ||
                seam.x.radical || seam.x.denominator != 1 || seam.y.radical || seam.y.denominator != 1) integral_holonomy = 0;
        }
    if (work.failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
    for (int32_t horizontal = 0; horizontal <= (int32_t)direction_bound; ++horizontal)
        for (int32_t vertical = -(int32_t)direction_bound; vertical <= (int32_t)direction_bound; ++vertical) {
            if ((!horizontal && vertical <= 0) || divisor((unsigned)horizontal,magnitude32(vertical)) != 1) continue;
            if (proposed.directions_examined >= max_directions) { proposed.truncated = 1; goto finished; }
            billiard_cylinder_report decomposition;
            exact_point direction = {exact_integer(horizontal),exact_integer(vertical)};
            enum billiard_cylinder_status status = billiard_cylinders(surface,direction,limits,&decomposition);
            if (status == BILLIARD_CYLINDER_INVALID) return status;
            billiard_direction_count *row = &proposed.direction[proposed.directions_examined++];
            *row = (billiard_direction_count){horizontal,vertical,status,0,0};
            if (status == BILLIARD_CYLINDER_CERTIFIED) {
                row->connections = decomposition.connection_count;
                row->closed_families = decomposition.counted_cylinders;
                proposed.connections += row->connections; proposed.closed_families += row->closed_families;
            } else ++proposed.unresolved_directions;
            if (raw_evidence) {
                fprintf(raw_evidence,"direction\t%d\t%d\n",horizontal,vertical);
                if (billiard_cylinders_write(raw_evidence,&decomposition)) return BILLIARD_CYLINDER_INVALID;
            }
        }
finished:
    proposed.all_lengths_covered = integral_holonomy && !proposed.truncated && !proposed.unresolved_directions &&
        exact_order(&work,limits->length_bound,exact_integer((int32_t)direction_bound)) <= 0;
    if (work.failed) return BILLIARD_CYLINDER_ARITHMETIC_LIMIT;
    *report = proposed;
    return proposed.truncated || proposed.unresolved_directions ? BILLIARD_CYLINDER_LIMIT : BILLIARD_CYLINDER_CERTIFIED;
}
