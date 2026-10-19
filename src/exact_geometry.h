#ifndef BILLIARD_EXACT_GEOMETRY_H
#define BILLIARD_EXACT_GEOMETRY_H
#include "polygon.h"

typedef billiard_exact exact_number;
typedef billiard_exact_point exact_point;
typedef struct { exact_number entry[2][2]; } billiard_exact_matrix;
typedef struct { int failed; } exact_work;

static inline exact_number exact_integer(int32_t value) { return (exact_number){value,0,1}; }
static inline exact_number exact_sum(exact_work *work, exact_number first, exact_number second) {
    exact_number result = exact_integer(0);
    if (billiard_exact_add(first,second,&result)) work->failed = 1;
    return result;
}
static inline exact_number exact_difference(exact_work *work, exact_number first, exact_number second) {
    exact_number result = exact_integer(0);
    if (billiard_exact_subtract(first,second,&result)) work->failed = 1;
    return result;
}
static inline exact_number exact_product(exact_work *work, exact_number first, exact_number second) {
    exact_number result = exact_integer(0);
    if (billiard_exact_multiply(first,second,&result)) work->failed = 1;
    return result;
}
static inline exact_number exact_quotient(exact_work *work, exact_number first, exact_number second) {
    exact_number result = exact_integer(0);
    if (billiard_exact_divide(first,second,&result)) work->failed = 1;
    return result;
}
static inline int exact_order(exact_work *work, exact_number first, exact_number second) {
    int ordering = 0;
    if (billiard_exact_compare(first,second,&ordering)) work->failed = 1;
    return ordering;
}
static inline exact_point exact_point_sum(exact_work *work, exact_point first, exact_point second) {
    exact_number horizontal = exact_sum(work,first.x,second.x);
    exact_number vertical = exact_sum(work,first.y,second.y);
    return (exact_point){horizontal,vertical};
}
static inline exact_point exact_point_difference(exact_work *work, exact_point first, exact_point second) {
    exact_number horizontal = exact_difference(work,first.x,second.x);
    exact_number vertical = exact_difference(work,first.y,second.y);
    return (exact_point){horizontal,vertical};
}
static inline exact_point exact_point_scale(exact_work *work, exact_point vector, exact_number scale) {
    exact_number horizontal = exact_product(work,vector.x,scale);
    exact_number vertical = exact_product(work,vector.y,scale);
    return (exact_point){horizontal,vertical};
}
static inline exact_number exact_dot(exact_work *work, exact_point first, exact_point second) {
    exact_number horizontal = exact_product(work,first.x,second.x);
    exact_number vertical = exact_product(work,first.y,second.y);
    return exact_sum(work,horizontal,vertical);
}
static inline exact_number exact_cross(exact_work *work, exact_point first, exact_point second) {
    exact_number first_product = exact_product(work,first.x,second.y);
    exact_number second_product = exact_product(work,first.y,second.x);
    return exact_difference(work,first_product,second_product);
}
static inline int exact_point_equal(exact_point first, exact_point second) {
    return billiard_exact_equal(first.x,second.x) && billiard_exact_equal(first.y,second.y);
}
static inline int exact_point_valid(exact_point point) {
    return billiard_exact_valid(point.x) && billiard_exact_valid(point.y);
}
static inline billiard_exact_matrix exact_identity(void) {
    billiard_exact_matrix matrix = {{{{1,0,1},{0,0,1}},{{0,0,1},{1,0,1}}}};
    return matrix;
}
static inline exact_point exact_transform(exact_work *work, billiard_exact_matrix matrix, exact_point vector) {
    exact_point first_row = {matrix.entry[0][0],matrix.entry[0][1]};
    exact_point second_row = {matrix.entry[1][0],matrix.entry[1][1]};
    exact_number horizontal = exact_dot(work,first_row,vector);
    exact_number vertical = exact_dot(work,second_row,vector);
    return (exact_point){horizontal,vertical};
}
static inline billiard_exact_matrix exact_matrix_product(exact_work *work,
    billiard_exact_matrix first, billiard_exact_matrix second) {
    billiard_exact_matrix result;
    for (unsigned row = 0; row < 2; ++row)
        for (unsigned column = 0; column < 2; ++column) {
            exact_number left = exact_product(work,first.entry[row][0],second.entry[0][column]);
            exact_number right = exact_product(work,first.entry[row][1],second.entry[1][column]);
            result.entry[row][column] = exact_sum(work,left,right);
        }
    return result;
}
static inline int exact_matrix_equal(billiard_exact_matrix first, billiard_exact_matrix second) {
    for (unsigned row = 0; row < 2; ++row)
        for (unsigned column = 0; column < 2; ++column)
            if (!billiard_exact_equal(first.entry[row][column],second.entry[row][column])) return 0;
    return 1;
}
static inline billiard_exact_matrix exact_transpose(billiard_exact_matrix matrix) {
    exact_number saved = matrix.entry[0][1];
    matrix.entry[0][1] = matrix.entry[1][0]; matrix.entry[1][0] = saved;
    return matrix;
}
#endif
