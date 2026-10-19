#include "surface.h"
#include <string.h>

static unsigned common_divisor(unsigned first, unsigned second) {
    while (second) { unsigned remainder = first % second; first = second; second = remainder; }
    return first;
}

static unsigned required_copies(const billiard_polygon *polygon) {
    unsigned denominator_lcm = 1;
    for (size_t vertex = 0; vertex < polygon->count; ++vertex) {
        billiard_angle angle = polygon->angle[vertex];
        unsigned denominator = angle.denominator / common_divisor(angle.numerator,angle.denominator);
        denominator_lcm = denominator_lcm / common_divisor(denominator_lcm,denominator) * denominator;
        if (denominator_lcm > BILLIARD_SURFACE_MAX_COPIES/2) return 0;
    }
    return 2*denominator_lcm;
}

static billiard_exact_matrix side_reflection(exact_work *work,
    const billiard_polygon *polygon, size_t side) {
    exact_point edge = exact_point_difference(work,polygon->vertex[(side+1)%polygon->count],polygon->vertex[side]);
    exact_number norm = exact_dot(work,edge,edge);
    exact_number coordinates[2] = {edge.x,edge.y};
    billiard_exact_matrix reflection;
    for (unsigned row = 0; row < 2; ++row)
        for (unsigned column = 0; column < 2; ++column) {
            exact_number entry = exact_product(work,coordinates[row],coordinates[column]);
            entry = exact_product(work,entry,exact_integer(2));
            entry = exact_quotient(work,entry,norm);
            reflection.entry[row][column] = exact_difference(work,entry,exact_integer(row == column));
        }
    return reflection;
}

exact_point billiard_surface_vertex(exact_work *work, const billiard_surface *surface,
                                   size_t copy, size_t vertex) {
    if (!surface || copy >= surface->copy_count || vertex >= surface->polygon.count) {
        work->failed = 1; return (exact_point){exact_integer(0),exact_integer(0)};
    }
    return exact_transform(work,surface->copy[copy].placement,surface->polygon.vertex[vertex]);
}

exact_point billiard_surface_fold(exact_work *work, const billiard_surface *surface,
                                 size_t copy, exact_point point) {
    if (!surface || copy >= surface->copy_count || !exact_point_valid(point)) {
        work->failed = 1; return (exact_point){exact_integer(0),exact_integer(0)};
    }
    return exact_transform(work,exact_transpose(surface->copy[copy].placement),point);
}

static size_t representative(size_t *parent, size_t item) {
    while (parent[item] != item) { parent[item] = parent[parent[item]]; item = parent[item]; }
    return item;
}
static void identify(size_t *parent, size_t first, size_t second) {
    first = representative(parent,first); second = representative(parent,second);
    if (first != second) parent[second] = first;
}

static enum billiard_surface_status topology(billiard_surface *surface) {
    size_t sides = surface->polygon.count;
    size_t corners = surface->copy_count * sides;
    size_t parent[BILLIARD_SURFACE_MAX_CORNERS], class_number[BILLIARD_SURFACE_MAX_CORNERS];
    size_t connected[BILLIARD_SURFACE_MAX_COPIES];
    for (size_t corner = 0; corner < corners; ++corner) { parent[corner] = corner; class_number[corner] = SIZE_MAX; }
    for (size_t copy = 0; copy < surface->copy_count; ++copy) connected[copy] = copy;
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        for (size_t side = 0; side < sides; ++side) {
            billiard_surface_seam seam = surface->seam[copy][side];
            identify(connected,copy,seam.target_copy);
            identify(parent,copy*sides+side,seam.target_copy*sides+side);
            identify(parent,copy*sides+(side+1)%sides,seam.target_copy*sides+(side+1)%sides);
        }
    surface->components = 0;
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        if (representative(connected,copy) == copy) ++surface->components;
    surface->vertex_class_count = 0;
    memset(surface->cone,0,sizeof surface->cone);
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        for (size_t vertex = 0; vertex < sides; ++vertex) {
            size_t root = representative(parent,copy*sides+vertex);
            if (class_number[root] == SIZE_MAX) {
                class_number[root] = surface->vertex_class_count++;
                surface->cone[class_number[root]].representative = copy*sides+vertex;
            }
            size_t number = class_number[root];
            surface->vertex_class[copy][vertex] = number;
            billiard_angle angle = surface->polygon.angle[vertex];
            if ((12u*angle.numerator)%angle.denominator) return BILLIARD_SURFACE_INVALID;
            surface->cone[number].angle_twelfths += 12u*angle.numerator/angle.denominator;
        }
    surface->edge_count = (unsigned)(corners/2);
    surface->euler_characteristic = (int)surface->vertex_class_count - (int)surface->edge_count + (int)surface->copy_count;
    int twice_genus = 2*(int)surface->components - surface->euler_characteristic;
    if (twice_genus < 0 || twice_genus%2) return BILLIARD_SURFACE_INVALID;
    surface->genus = (unsigned)(twice_genus/2);
    unsigned zero_sum = 0;
    for (size_t number = 0; number < surface->vertex_class_count; ++number) {
        unsigned angle = surface->cone[number].angle_twelfths;
        if (angle < 24 || angle%24) return BILLIARD_SURFACE_INVALID;
        surface->cone[number].zero_order = angle/24-1;
        zero_sum += angle/24-1;
    }
    if (2*surface->genus < 2*surface->components || zero_sum != 2*surface->genus-2*surface->components)
        return BILLIARD_SURFACE_INVALID;
    exact_work work = {0};
    exact_number twice_area = exact_integer(0);
    for (size_t side = 0; side < sides; ++side) {
        exact_number piece = exact_cross(&work,surface->polygon.vertex[side],surface->polygon.vertex[(side+1)%sides]);
        twice_area = exact_sum(&work,twice_area,piece);
    }
    exact_number multiplier;
    if (billiard_exact_make((int32_t)surface->copy_count,0,2,&multiplier)) return BILLIARD_SURFACE_ARITHMETIC_LIMIT;
    surface->area = exact_product(&work,twice_area,multiplier);
    return work.failed ? BILLIARD_SURFACE_ARITHMETIC_LIMIT : BILLIARD_SURFACE_OK;
}

enum billiard_surface_status billiard_surface_unfold(const billiard_polygon *polygon,
                                                    billiard_surface *surface) {
    if (!surface) return BILLIARD_SURFACE_INVALID;
    enum billiard_polygon_status polygon_status = billiard_polygon_validate(polygon);
    if (polygon_status == BILLIARD_POLYGON_ARITHMETIC_LIMIT) return BILLIARD_SURFACE_ARITHMETIC_LIMIT;
    if (polygon_status != BILLIARD_POLYGON_OK) return BILLIARD_SURFACE_INVALID;
    unsigned expected = required_copies(polygon);
    if (!expected) return BILLIARD_SURFACE_COPY_LIMIT;
    billiard_surface proposed = {0};
    proposed.polygon = *polygon;
    proposed.copy_count = 1;
    proposed.copy[0] = (billiard_surface_copy){exact_identity(),1};
    exact_work work = {0};
    billiard_exact_matrix reflections[BILLIARD_POLYGON_MAX_VERTICES];
    for (size_t side = 0; side < polygon->count; ++side) reflections[side] = side_reflection(&work,polygon,side);
    if (work.failed) return BILLIARD_SURFACE_ARITHMETIC_LIMIT;
    for (size_t copy = 0; copy < proposed.copy_count; ++copy)
        for (size_t side = 0; side < polygon->count; ++side) {
            billiard_exact_matrix placement = exact_matrix_product(&work,proposed.copy[copy].placement,reflections[side]);
            if (work.failed) return BILLIARD_SURFACE_ARITHMETIC_LIMIT;
            size_t target;
            for (target = 0; target < proposed.copy_count; ++target)
                if (exact_matrix_equal(placement,proposed.copy[target].placement)) break;
            if (target == proposed.copy_count) {
                if (target >= expected || target >= BILLIARD_SURFACE_MAX_COPIES) return BILLIARD_SURFACE_COPY_LIMIT;
                proposed.copy[proposed.copy_count++] = (billiard_surface_copy){placement,-proposed.copy[copy].orientation};
            }
            exact_point before = billiard_surface_vertex(&work,&proposed,copy,side);
            exact_point after = billiard_surface_vertex(&work,&proposed,target,side);
            exact_point translation = exact_point_difference(&work,after,before);
            if (work.failed) return BILLIARD_SURFACE_ARITHMETIC_LIMIT;
            proposed.seam[copy][side] = (billiard_surface_seam){target,side,translation};
        }
    if (proposed.copy_count != expected) return BILLIARD_SURFACE_INVALID;
    enum billiard_surface_status status = topology(&proposed);
    if (status != BILLIARD_SURFACE_OK) return status;
    status = billiard_surface_validate(&proposed);
    if (status == BILLIARD_SURFACE_OK) *surface = proposed;
    return status;
}

enum billiard_surface_status billiard_surface_validate(const billiard_surface *surface) {
    if (!surface || surface->copy_count < 2 || surface->copy_count > BILLIARD_SURFACE_MAX_COPIES)
        return BILLIARD_SURFACE_INVALID;
    enum billiard_polygon_status polygon_status = billiard_polygon_validate(&surface->polygon);
    if (polygon_status == BILLIARD_POLYGON_ARITHMETIC_LIMIT) return BILLIARD_SURFACE_ARITHMETIC_LIMIT;
    if (polygon_status != BILLIARD_POLYGON_OK || surface->copy_count != required_copies(&surface->polygon))
        return BILLIARD_SURFACE_INVALID;
    if (!exact_matrix_equal(surface->copy[0].placement,exact_identity())) return BILLIARD_SURFACE_INVALID;
    exact_work work = {0};
    size_t sides = surface->polygon.count;
    for (size_t copy = 0; copy < surface->copy_count; ++copy) {
        billiard_exact_matrix placement = surface->copy[copy].placement;
        billiard_exact_matrix orthogonality = exact_matrix_product(&work,placement,exact_transpose(placement));
        exact_number positive = exact_product(&work,placement.entry[0][0],placement.entry[1][1]);
        exact_number negative = exact_product(&work,placement.entry[0][1],placement.entry[1][0]);
        exact_number determinant = exact_difference(&work,positive,negative);
        if (work.failed) return BILLIARD_SURFACE_ARITHMETIC_LIMIT;
        if (!exact_matrix_equal(orthogonality,exact_identity()) ||
            (surface->copy[copy].orientation != 1 && surface->copy[copy].orientation != -1) ||
            !billiard_exact_equal(determinant,exact_integer(surface->copy[copy].orientation))) return BILLIARD_SURFACE_INVALID;
        for (size_t previous = 0; previous < copy; ++previous)
            if (exact_matrix_equal(placement,surface->copy[previous].placement)) return BILLIARD_SURFACE_INVALID;
        for (size_t side = 0; side < sides; ++side) {
            billiard_surface_seam seam = surface->seam[copy][side];
            if (seam.target_copy >= surface->copy_count || seam.target_side != side ||
                surface->copy[seam.target_copy].orientation != -surface->copy[copy].orientation ||
                !exact_point_valid(seam.translation)) return BILLIARD_SURFACE_INVALID;
            billiard_surface_seam reverse = surface->seam[seam.target_copy][side];
            if (reverse.target_copy != copy || reverse.target_side != side) return BILLIARD_SURFACE_INVALID;
            billiard_exact_matrix reflection = side_reflection(&work,&surface->polygon,side);
            billiard_exact_matrix expected = exact_matrix_product(&work,placement,reflection);
            if (!exact_matrix_equal(expected,surface->copy[seam.target_copy].placement)) return BILLIARD_SURFACE_INVALID;
            for (unsigned endpoint = 0; endpoint < 2; ++endpoint) {
                size_t vertex = (side+endpoint)%sides;
                exact_point from = billiard_surface_vertex(&work,surface,copy,vertex);
                exact_point to = billiard_surface_vertex(&work,surface,seam.target_copy,vertex);
                exact_point translated = exact_point_sum(&work,from,seam.translation);
                if (work.failed) return BILLIARD_SURFACE_ARITHMETIC_LIMIT;
                if (!exact_point_equal(translated,to)) return BILLIARD_SURFACE_INVALID;
            }
        }
    }
    if (work.failed) return BILLIARD_SURFACE_ARITHMETIC_LIMIT;
    billiard_surface reconstructed = *surface;
    enum billiard_surface_status status = topology(&reconstructed);
    if (status != BILLIARD_SURFACE_OK) return status;
    if (surface->components != 1 || reconstructed.components != surface->components ||
        reconstructed.genus != surface->genus || reconstructed.edge_count != surface->edge_count ||
        reconstructed.euler_characteristic != surface->euler_characteristic ||
        reconstructed.vertex_class_count != surface->vertex_class_count ||
        !billiard_exact_equal(reconstructed.area,surface->area)) return BILLIARD_SURFACE_INVALID;
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        for (size_t vertex = 0; vertex < sides; ++vertex)
            if (reconstructed.vertex_class[copy][vertex] != surface->vertex_class[copy][vertex]) return BILLIARD_SURFACE_INVALID;
    for (size_t number = 0; number < surface->vertex_class_count; ++number)
        if (surface->cone[number].zero_order != reconstructed.cone[number].zero_order ||
            surface->cone[number].angle_twelfths != reconstructed.cone[number].angle_twelfths ||
            surface->cone[number].representative != reconstructed.cone[number].representative) return BILLIARD_SURFACE_INVALID;
    return BILLIARD_SURFACE_OK;
}

int billiard_surface_write(FILE *output, const billiard_surface *surface) {
    if (!output || billiard_surface_validate(surface) != BILLIARD_SURFACE_OK) return -1;
    fprintf(output,"model\treflected_translation_cover\ncopies\t%zu\nedges\t%u\nvertex_classes\t%zu\ncomponents\t%u\neuler\t%d\ngenus\t%u\narea_exact\t%d\t%d\t%d\n",
        surface->copy_count,surface->edge_count,surface->vertex_class_count,surface->components,
        surface->euler_characteristic,surface->genus,surface->area.rational,surface->area.radical,surface->area.denominator);
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        for (size_t side = 0; side < surface->polygon.count; ++side) {
            billiard_surface_seam seam = surface->seam[copy][side];
            fprintf(output,"seam\t%zu\t%zu\t%zu\t%zu\t%d\t%d\t%d\t%d\t%d\t%d\n",copy,side,seam.target_copy,seam.target_side,
                seam.translation.x.rational,seam.translation.x.radical,seam.translation.x.denominator,
                seam.translation.y.rational,seam.translation.y.radical,seam.translation.y.denominator);
        }
    for (size_t number = 0; number < surface->vertex_class_count; ++number)
        fprintf(output,"cone\t%zu\t%zu\t%u\t%u\n",number,surface->cone[number].representative,
            surface->cone[number].angle_twelfths,surface->cone[number].zero_order);
    return ferror(output) ? -1 : 0;
}
