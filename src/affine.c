#include "affine.h"
#include <math.h>

enum billiard_affine_status billiard_affine(double first, double second, double third, double fourth, billiard_affine_matrix *matrix) {
    if (!matrix) return BILLIARD_AFFINE_INVALID;
    if (!isfinite(first) || !isfinite(second) || !isfinite(third) || !isfinite(fourth)) return BILLIARD_AFFINE_NONFINITE;
    double scale = fmax(fmax(fabs(first),fabs(second)),fmax(fabs(third),fabs(fourth)));
    if (!(scale > 0)) return BILLIARD_AFFINE_INVALID;
    double a = first/scale, b = second/scale, c = third/scale, d = fourth/scale;
    double normalized_determinant = a*d-b*c;
    if (normalized_determinant <= 0) return BILLIARD_AFFINE_INVALID;
    double condition = (a*a+b*b+c*c+d*d)/normalized_determinant;
    if (normalized_determinant <= BILLIARD_AFFINE_TOLERANCE || condition >= 1/BILLIARD_AFFINE_TOLERANCE)
        return BILLIARD_AFFINE_ILL_CONDITIONED;
    double determinant = normalized_determinant*scale*scale;
    if (!isfinite(determinant) || determinant <= 0) return BILLIARD_AFFINE_NONFINITE;
    *matrix = (billiard_affine_matrix){{{first,second},{third,fourth}},determinant,condition,
        fabs(determinant-1.0) <= BILLIARD_AFFINE_TOLERANCE*(1+fabs(determinant))};
    return BILLIARD_AFFINE_OK;
}

enum billiard_affine_status billiard_affine_shear(double amount, billiard_affine_matrix *matrix) {
    return billiard_affine(1,amount,0,1,matrix);
}
enum billiard_affine_status billiard_affine_rotation(double radians, billiard_affine_matrix *matrix) {
    if (!isfinite(radians)) return BILLIARD_AFFINE_NONFINITE;
    double cosine = cos(radians), sine = sin(radians);
    return billiard_affine(cosine,-sine,sine,cosine,matrix);
}
enum billiard_affine_status billiard_affine_diagonal(double time, billiard_affine_matrix *matrix) {
    if (!isfinite(time)) return BILLIARD_AFFINE_NONFINITE;
    return billiard_affine(exp(time),0,0,exp(-time),matrix);
}
enum billiard_affine_status billiard_affine_compose(billiard_affine_matrix after, billiard_affine_matrix before, billiard_affine_matrix *matrix) {
    billiard_affine_matrix checked;
    enum billiard_affine_status status = billiard_affine(after.entry[0][0],after.entry[0][1],after.entry[1][0],after.entry[1][1],&checked);
    if (status != BILLIARD_AFFINE_OK) return status;
    status = billiard_affine(before.entry[0][0],before.entry[0][1],before.entry[1][0],before.entry[1][1],&checked);
    if (status != BILLIARD_AFFINE_OK) return status;
    double first = after.entry[0][0]*before.entry[0][0]+after.entry[0][1]*before.entry[1][0];
    double second = after.entry[0][0]*before.entry[0][1]+after.entry[0][1]*before.entry[1][1];
    double third = after.entry[1][0]*before.entry[0][0]+after.entry[1][1]*before.entry[1][0];
    double fourth = after.entry[1][0]*before.entry[0][1]+after.entry[1][1]*before.entry[1][1];
    return billiard_affine(first,second,third,fourth,matrix);
}
enum billiard_affine_status billiard_affine_inverse(billiard_affine_matrix matrix, billiard_affine_matrix *inverse) {
    billiard_affine_matrix checked;
    enum billiard_affine_status status = billiard_affine(matrix.entry[0][0],matrix.entry[0][1],matrix.entry[1][0],matrix.entry[1][1],&checked);
    if (status != BILLIARD_AFFINE_OK) return status;
    return billiard_affine(checked.entry[1][1]/checked.determinant,-checked.entry[0][1]/checked.determinant,
        -checked.entry[1][0]/checked.determinant,checked.entry[0][0]/checked.determinant,inverse);
}
billiard_numeric_point billiard_affine_point(billiard_affine_matrix matrix, billiard_numeric_point point) {
    return (billiard_numeric_point){matrix.entry[0][0]*point.x+matrix.entry[0][1]*point.y,
        matrix.entry[1][0]*point.x+matrix.entry[1][1]*point.y};
}
static billiard_numeric_point numeric(exact_point point) {
    return (billiard_numeric_point){billiard_exact_display(point.x),billiard_exact_display(point.y)};
}
static int finite_point(billiard_numeric_point point) { return isfinite(point.x) && isfinite(point.y); }

static enum billiard_affine_status torus_basis(const billiard_surface *surface,
    const billiard_period_ledger *periods, billiard_deformed_surface *deformed) {
    if (surface->genus != 1) return BILLIARD_AFFINE_OK;
    if (periods->absolute_count != 2) return BILLIARD_AFFINE_INVALID;
    exact_point first = periods->period[0].value, second = periods->period[1].value;
    exact_work work = {0};
    exact_number determinant = exact_cross(&work,first,second);
    int orientation = exact_order(&work,determinant,exact_integer(0));
    exact_number positive_area = orientation > 0 ? determinant : exact_difference(&work,exact_integer(0),determinant);
    if (work.failed) return BILLIARD_AFFINE_ARITHMETIC_LIMIT;
    if (!orientation || !billiard_exact_equal(positive_area,surface->area)) return BILLIARD_AFFINE_INVALID;
    deformed->tau_basis_coefficients[0][0] = 1;
    deformed->tau_basis_coefficients[1][1] = 1;
    const billiard_polygon *polygon = &surface->polygon;
    exact_point zero = {exact_integer(0),exact_integer(0)};
    int rectangle = polygon->count == 4 && exact_point_equal(polygon->vertex[0],zero) &&
        billiard_exact_equal(polygon->vertex[1].y,exact_integer(0)) &&
        billiard_exact_equal(polygon->vertex[3].x,exact_integer(0)) &&
        billiard_exact_equal(polygon->vertex[2].x,polygon->vertex[1].x) &&
        billiard_exact_equal(polygon->vertex[2].y,polygon->vertex[3].y);
    if (rectangle) {
        exact_point basis[2] = {{exact_product(&work,polygon->vertex[1].x,exact_integer(2)),exact_integer(0)},
            {exact_integer(0),exact_product(&work,polygon->vertex[3].y,exact_integer(2))}};
        for (unsigned index = 0; index < 2; ++index) {
            exact_number numerator = exact_cross(&work,basis[index],second);
            exact_number left = exact_quotient(&work,numerator,determinant);
            numerator = exact_cross(&work,first,basis[index]);
            exact_number right = exact_quotient(&work,numerator,determinant);
            if (work.failed) return BILLIARD_AFFINE_ARITHMETIC_LIMIT;
            if (left.radical || left.denominator != 1 || right.radical || right.denominator != 1) return BILLIARD_AFFINE_INVALID;
            deformed->tau_basis_coefficients[index][0] = left.rational;
            deformed->tau_basis_coefficients[index][1] = right.rational;
        }
        int64_t basis_determinant = (int64_t)deformed->tau_basis_coefficients[0][0]*deformed->tau_basis_coefficients[1][1]-
            (int64_t)deformed->tau_basis_coefficients[0][1]*deformed->tau_basis_coefficients[1][0];
        if (basis_determinant != 1 && basis_determinant != -1) return BILLIARD_AFFINE_INVALID;
        first = basis[0]; second = basis[1]; deformed->rectangular_source_basis = 1;
    } else if (orientation < 0) {
        second = exact_point_scale(&work,second,exact_integer(-1));
        deformed->tau_basis_coefficients[1][1] = -1;
    }
    billiard_numeric_point transformed_first = billiard_affine_point(deformed->matrix,numeric(first));
    billiard_numeric_point transformed_second = billiard_affine_point(deformed->matrix,numeric(second));
    double squared_norm = transformed_first.x*transformed_first.x+transformed_first.y*transformed_first.y;
    deformed->tau_real = (transformed_first.x*transformed_second.x+transformed_first.y*transformed_second.y)/squared_norm;
    deformed->tau_imaginary = (transformed_first.x*transformed_second.y-transformed_first.y*transformed_second.x)/squared_norm;
    if (work.failed) return BILLIARD_AFFINE_ARITHMETIC_LIMIT;
    if (!isfinite(deformed->tau_real) || !isfinite(deformed->tau_imaginary) || deformed->tau_imaginary <= 0)
        return BILLIARD_AFFINE_NONFINITE;
    deformed->tau_valid = 1;
    return BILLIARD_AFFINE_OK;
}

enum billiard_affine_status billiard_affine_apply(const billiard_surface *surface,
    const billiard_period_ledger *periods, billiard_affine_matrix matrix, billiard_deformed_surface *deformed) {
    if (!deformed) return BILLIARD_AFFINE_INVALID;
    enum billiard_period_status period_status = billiard_periods_validate(surface,periods);
    if (period_status != BILLIARD_PERIOD_OK)
        return period_status == BILLIARD_PERIOD_ARITHMETIC_LIMIT ? BILLIARD_AFFINE_ARITHMETIC_LIMIT : BILLIARD_AFFINE_INVALID;
    billiard_deformed_surface proposed = {0};
    enum billiard_affine_status status = billiard_affine(matrix.entry[0][0],matrix.entry[0][1],matrix.entry[1][0],matrix.entry[1][1],&proposed.matrix);
    if (status != BILLIARD_AFFINE_OK) return status;
    proposed.copy_count = surface->copy_count; proposed.vertex_count = surface->polygon.count;
    proposed.period_count = periods->period_count; proposed.genus = surface->genus; proposed.zero_count = periods->zero_count;
    for (size_t zero = 0; zero < periods->zero_count; ++zero) proposed.zero_order[zero] = periods->zero_order[zero];
    proposed.area = billiard_exact_display(surface->area)*proposed.matrix.determinant;
    if (!isfinite(proposed.area) || proposed.area <= 0) return BILLIARD_AFFINE_NONFINITE;
    exact_work work = {0};
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        for (size_t vertex = 0; vertex < surface->polygon.count; ++vertex) {
            exact_point point = billiard_surface_vertex(&work,surface,copy,vertex);
            proposed.vertex[copy][vertex] = billiard_affine_point(proposed.matrix,numeric(point));
            proposed.seam[copy][vertex] = billiard_affine_point(proposed.matrix,numeric(surface->seam[copy][vertex].translation));
            if (!finite_point(proposed.vertex[copy][vertex]) || !finite_point(proposed.seam[copy][vertex])) return BILLIARD_AFFINE_NONFINITE;
        }
    for (size_t period = 0; period < periods->period_count; ++period) {
        proposed.period[period] = billiard_affine_point(proposed.matrix,numeric(periods->period[period].value));
        if (!finite_point(proposed.period[period])) return BILLIARD_AFFINE_NONFINITE;
    }
    double independently_summed_area = 0;
    for (size_t copy = 0; copy < surface->copy_count; ++copy) {
        double twice_area = 0;
        for (size_t side = 0; side < surface->polygon.count; ++side) {
            size_t next = (side+1)%surface->polygon.count;
            billiard_numeric_point point = proposed.vertex[copy][side], after = proposed.vertex[copy][next];
            twice_area += point.x*after.y-point.y*after.x;
            size_t target = surface->seam[copy][side].target_copy;
            for (unsigned endpoint = 0; endpoint < 2; ++endpoint) {
                size_t vertex = (side+endpoint)%surface->polygon.count;
                billiard_numeric_point from = proposed.vertex[copy][vertex], to = proposed.vertex[target][vertex];
                billiard_numeric_point translation = proposed.seam[copy][side];
                double error = hypot(from.x+translation.x-to.x,from.y+translation.y-to.y);
                double scale = 1+hypot(from.x,from.y)+hypot(to.x,to.y)+hypot(translation.x,translation.y);
                if (!isfinite(error) || error > 8*BILLIARD_AFFINE_TOLERANCE*scale) return BILLIARD_AFFINE_ILL_CONDITIONED;
                if (error > proposed.maximum_seam_error) proposed.maximum_seam_error = error;
            }
        }
        independently_summed_area += fabs(twice_area)*0.5;
    }
    if (work.failed) return BILLIARD_AFFINE_ARITHMETIC_LIMIT;
    if (!isfinite(independently_summed_area) || fabs(independently_summed_area-proposed.area) >
        16*BILLIARD_AFFINE_TOLERANCE*(1+proposed.area)) return BILLIARD_AFFINE_ILL_CONDITIONED;
    status = torus_basis(surface,periods,&proposed);
    if (status == BILLIARD_AFFINE_OK) *deformed = proposed;
    return status;
}

int billiard_affine_write(FILE *output, const billiard_deformed_surface *deformed) {
    if (!output || !deformed || deformed->copy_count > BILLIARD_SURFACE_MAX_COPIES ||
        deformed->vertex_count > BILLIARD_POLYGON_MAX_VERTICES || deformed->period_count > BILLIARD_PERIOD_MAX_PERIODS ||
        deformed->zero_count > BILLIARD_SURFACE_MAX_CORNERS) return -1;
    fprintf(output,"affine_precision\tIEEE754_binary64\nrelative_gate\t%.17g\nmatrix\t%.17g\t%.17g\t%.17g\t%.17g\ndeterminant\t%.17g\ncondition\t%.17g\napproximately_SL2\t%d\narea\t%.17g\ngenus\t%u\nmaximum_seam_error\t%.17g\n",
        BILLIARD_AFFINE_TOLERANCE,deformed->matrix.entry[0][0],deformed->matrix.entry[0][1],deformed->matrix.entry[1][0],deformed->matrix.entry[1][1],
        deformed->matrix.determinant,deformed->matrix.condition_estimate,deformed->matrix.approximately_determinant_one,deformed->area,deformed->genus,deformed->maximum_seam_error);
    for (size_t zero = 0; zero < deformed->zero_count; ++zero) fprintf(output,"affine_zero_order\t%zu\t%u\n",zero,deformed->zero_order[zero]);
    for (size_t copy = 0; copy < deformed->copy_count; ++copy)
        for (size_t vertex = 0; vertex < deformed->vertex_count; ++vertex)
            fprintf(output,"affine_vertex_and_seam\t%zu\t%zu\t%.17g\t%.17g\t%.17g\t%.17g\n",copy,vertex,
                deformed->vertex[copy][vertex].x,deformed->vertex[copy][vertex].y,deformed->seam[copy][vertex].x,deformed->seam[copy][vertex].y);
    for (size_t period = 0; period < deformed->period_count; ++period)
        fprintf(output,"affine_period\t%zu\t%.17g\t%.17g\n",period,deformed->period[period].x,deformed->period[period].y);
    if (deformed->tau_valid) {
        fprintf(output,"torus_basis_tau\t%.17g\t%.17g\n",deformed->tau_real,deformed->tau_imaginary);
        fprintf(output,"torus_basis_coefficients\t%d\t%d\t%d\t%d\t%s\n",deformed->tau_basis_coefficients[0][0],
            deformed->tau_basis_coefficients[0][1],deformed->tau_basis_coefficients[1][0],deformed->tau_basis_coefficients[1][1],
            deformed->rectangular_source_basis ? "rectangular_source_lattice" : "cellular_homology_basis");
    }
    else fprintf(output,"global_tau\tNOT_DEFINED_FOR_HIGHER_GENUS\n");
    return ferror(output) ? -1 : 0;
}
