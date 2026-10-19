#ifndef BILLIARD_AFFINE_H
#define BILLIARD_AFFINE_H
#include "periods.h"

#define BILLIARD_AFFINE_TOLERANCE 1e-12
typedef struct { double x, y; } billiard_numeric_point;
typedef struct {
    double entry[2][2], determinant, condition_estimate;
    int approximately_determinant_one;
} billiard_affine_matrix;
enum billiard_affine_status { BILLIARD_AFFINE_OK, BILLIARD_AFFINE_INVALID,
    BILLIARD_AFFINE_ILL_CONDITIONED, BILLIARD_AFFINE_NONFINITE, BILLIARD_AFFINE_ARITHMETIC_LIMIT };
typedef struct {
    billiard_affine_matrix matrix;
    size_t copy_count, vertex_count, period_count, zero_count;
    unsigned genus, zero_order[BILLIARD_SURFACE_MAX_CORNERS];
    billiard_numeric_point vertex[BILLIARD_SURFACE_MAX_COPIES][BILLIARD_POLYGON_MAX_VERTICES];
    billiard_numeric_point seam[BILLIARD_SURFACE_MAX_COPIES][BILLIARD_POLYGON_MAX_VERTICES];
    billiard_numeric_point period[BILLIARD_PERIOD_MAX_PERIODS];
    double area, maximum_seam_error;
    int tau_valid, rectangular_source_basis;
    int32_t tau_basis_coefficients[2][2];
    double tau_real, tau_imaginary;
} billiard_deformed_surface;

enum billiard_affine_status billiard_affine(double first, double second, double third, double fourth, billiard_affine_matrix *matrix);
enum billiard_affine_status billiard_affine_shear(double amount, billiard_affine_matrix *matrix);
enum billiard_affine_status billiard_affine_rotation(double radians, billiard_affine_matrix *matrix);
enum billiard_affine_status billiard_affine_diagonal(double time, billiard_affine_matrix *matrix);
enum billiard_affine_status billiard_affine_compose(billiard_affine_matrix after, billiard_affine_matrix before, billiard_affine_matrix *matrix);
enum billiard_affine_status billiard_affine_inverse(billiard_affine_matrix matrix, billiard_affine_matrix *inverse);
billiard_numeric_point billiard_affine_point(billiard_affine_matrix matrix, billiard_numeric_point point);
enum billiard_affine_status billiard_affine_apply(const billiard_surface *surface, const billiard_period_ledger *periods,
    billiard_affine_matrix matrix, billiard_deformed_surface *deformed);
int billiard_affine_write(FILE *output, const billiard_deformed_surface *deformed);
#endif
