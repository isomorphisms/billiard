#ifndef BILLIARD_PERIODS_H
#define BILLIARD_PERIODS_H
#include "surface.h"

#define BILLIARD_PERIOD_MAX_EDGES (BILLIARD_SURFACE_MAX_CORNERS/2u)
#define BILLIARD_PERIOD_MAX_PERIODS (BILLIARD_PERIOD_MAX_EDGES+2u)
enum billiard_period_status { BILLIARD_PERIOD_OK, BILLIARD_PERIOD_INVALID,
    BILLIARD_PERIOD_ARITHMETIC_LIMIT, BILLIARD_PERIOD_LIMIT };
enum billiard_period_kind { BILLIARD_PERIOD_ABSOLUTE, BILLIARD_PERIOD_RELATIVE };
typedef struct {
    size_t copy, side, opposite_copy, start_vertex, end_vertex;
    exact_point holonomy;
    int primal_tree, dual_tree;
} billiard_period_edge;
typedef struct { enum billiard_period_kind kind; size_t witness; exact_point value; } billiard_period;
typedef struct {
    size_t edge_count, vertex_count, base_vertex, period_count, absolute_count, relative_count, zero_count;
    unsigned genus;
    billiard_period_edge edge[BILLIARD_PERIOD_MAX_EDGES];
    size_t parent_vertex[BILLIARD_SURFACE_MAX_CORNERS], parent_edge[BILLIARD_SURFACE_MAX_CORNERS];
    int parent_sign[BILLIARD_SURFACE_MAX_CORNERS];
    exact_point vertex_potential[BILLIARD_SURFACE_MAX_CORNERS];
    unsigned zero_order[BILLIARD_SURFACE_MAX_CORNERS];
    billiard_period period[BILLIARD_PERIOD_MAX_PERIODS];
} billiard_period_ledger;

enum billiard_period_status billiard_periods(const billiard_surface *surface, billiard_period_ledger *ledger);
enum billiard_period_status billiard_periods_validate(const billiard_surface *surface, const billiard_period_ledger *ledger);
enum billiard_period_status billiard_period_chain(const billiard_period_ledger *ledger, size_t period,
    int32_t coefficients[BILLIARD_PERIOD_MAX_EDGES]);
int billiard_periods_write(FILE *output, const billiard_period_ledger *ledger);
#endif
