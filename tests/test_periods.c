#include "affine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void near(double observed, double expected) { assert(fabs(observed-expected) <= 1e-10*(1+fabs(expected))); }

/* Independent cellular-chain check: incidence boundary and period are summed
 * from the explicit edge chain, rather than the producer's tree potentials. */
static void check_chains(const billiard_surface *surface, const billiard_period_ledger *ledger) {
    for (size_t period = 0; period < ledger->period_count; ++period) {
        int32_t coefficient[BILLIARD_PERIOD_MAX_EDGES], boundary[BILLIARD_SURFACE_MAX_CORNERS] = {0};
        assert(billiard_period_chain(ledger,period,coefficient) == BILLIARD_PERIOD_OK);
        exact_work work = {0}; exact_point sum = {exact_integer(0),exact_integer(0)};
        for (size_t edge = 0; edge < ledger->edge_count; ++edge) {
            const billiard_period_edge *record = &ledger->edge[edge];
            boundary[record->start_vertex] -= coefficient[edge]; boundary[record->end_vertex] += coefficient[edge];
            exact_point displacement = exact_point_scale(&work,record->holonomy,exact_integer(coefficient[edge]));
            sum = exact_point_sum(&work,sum,displacement);
        }
        assert(!work.failed && exact_point_equal(sum,ledger->period[period].value));
        for (size_t vertex = 0; vertex < ledger->vertex_count; ++vertex) {
            int expected = 0;
            if (ledger->period[period].kind == BILLIARD_PERIOD_RELATIVE)
                expected = (vertex == ledger->period[period].witness)-(vertex == ledger->base_vertex);
            assert(boundary[vertex] == expected);
        }
    }
    assert(ledger->absolute_count == 2*surface->genus);
    assert(ledger->relative_count == (ledger->zero_count ? ledger->zero_count-1 : 0));
}

static void check_transformed_chains(const billiard_period_ledger *ledger, const billiard_deformed_surface *deformed) {
    for (size_t period = 0; period < ledger->period_count; ++period) {
        int32_t coefficient[BILLIARD_PERIOD_MAX_EDGES];
        assert(billiard_period_chain(ledger,period,coefficient) == BILLIARD_PERIOD_OK);
        long double horizontal = 0, vertical = 0;
        for (size_t edge = 0; edge < ledger->edge_count; ++edge) {
            const billiard_period_edge *record = &ledger->edge[edge];
            billiard_numeric_point start = deformed->vertex[record->copy][record->side];
            billiard_numeric_point end = deformed->vertex[record->copy][(record->side+1)%deformed->vertex_count];
            horizontal += coefficient[edge]*((long double)end.x-start.x);
            vertical += coefficient[edge]*((long double)end.y-start.y);
        }
        near(deformed->period[period].x,(double)horizontal); near(deformed->period[period].y,(double)vertical);
    }
}

static void check_surface(const billiard_surface *surface, int rectangle, double rectangle_ratio) {
    billiard_period_ledger ledger;
    assert(billiard_periods(surface,&ledger) == BILLIARD_PERIOD_OK);
    assert(billiard_periods_validate(surface,&ledger) == BILLIARD_PERIOD_OK);
    check_chains(surface,&ledger);
    billiard_affine_matrix actions[5];
    assert(billiard_affine(1,0,0,1,&actions[0]) == BILLIARD_AFFINE_OK);
    assert(billiard_affine_shear(0.5,&actions[1]) == BILLIARD_AFFINE_OK);
    assert(billiard_affine_diagonal(log(2.0),&actions[2]) == BILLIARD_AFFINE_OK);
    assert(billiard_affine_rotation(0.3,&actions[3]) == BILLIARD_AFFINE_OK);
    assert(billiard_affine(2,0,0,3,&actions[4]) == BILLIARD_AFFINE_OK);
    for (unsigned action = 0; action < 5; ++action) {
        billiard_deformed_surface deformed;
        assert(billiard_affine_apply(surface,&ledger,actions[action],&deformed) == BILLIARD_AFFINE_OK);
        near(deformed.area,billiard_exact_display(surface->area)*(action == 4 ? 6 : 1));
        assert(deformed.genus == surface->genus && deformed.zero_count == ledger.zero_count);
        unsigned zero_sum = 0;
        for (size_t zero = 0; zero < deformed.zero_count; ++zero) zero_sum += deformed.zero_order[zero];
        assert(zero_sum == 2*surface->genus-2);
        assert(deformed.tau_valid == (surface->genus == 1));
        check_transformed_chains(&ledger,&deformed);
        if (rectangle) {
            assert(deformed.rectangular_source_basis);
            if (action == 0) { near(deformed.tau_real,0); near(deformed.tau_imaginary,rectangle_ratio); }
            if (action == 1) { near(deformed.tau_real,0.5*rectangle_ratio); near(deformed.tau_imaginary,rectangle_ratio); }
            if (action == 2) near(deformed.tau_imaginary,rectangle_ratio/4);
            if (action == 3) { near(deformed.tau_real,0); near(deformed.tau_imaginary,rectangle_ratio); }
        }
        assert(actions[action].approximately_determinant_one == (action != 4));
    }
    billiard_period_ledger changed = ledger;
    changed.period[0].value.x = exact_integer(123);
    assert(billiard_periods_validate(surface,&changed) == BILLIARD_PERIOD_INVALID);
    billiard_deformed_surface rejected;
    assert(billiard_affine_apply(surface,&changed,actions[0],&rejected) == BILLIARD_AFFINE_INVALID);
    printf("periods genus %u: absolute=%zu relative=%zu zeroes=%zu; independent chains/affine geometry PASS\n",
        surface->genus,ledger.absolute_count,ledger.relative_count,ledger.zero_count);
}

int main(void) {
    for (unsigned kind = 0; kind < BILLIARD_FIXTURE_COUNT; ++kind) {
        billiard_polygon polygon; billiard_surface surface;
        assert(billiard_polygon_fixture((enum billiard_fixture)kind,&polygon) == BILLIARD_POLYGON_OK);
        assert(billiard_surface_unfold(&polygon,&surface) == BILLIARD_SURFACE_OK);
        check_surface(&surface,kind == BILLIARD_SQUARE || kind == BILLIARD_RECTANGLE,kind == BILLIARD_RECTANGLE ? 0.6 : 1);
    }
    billiard_polygon staircase = {0}; staircase.count = 8;
    const int32_t corners[8][2] = {{0,0},{3,0},{3,1},{2,1},{2,2},{1,2},{1,3},{0,3}};
    for (size_t vertex = 0; vertex < 8; ++vertex) {
        staircase.vertex[vertex] = (exact_point){exact_integer(corners[vertex][0]),exact_integer(corners[vertex][1])};
        staircase.angle[vertex] = (billiard_angle){vertex == 3 || vertex == 5 ? 3 : 1,2};
    }
    billiard_surface surface;
    assert(billiard_surface_unfold(&staircase,&surface) == BILLIARD_SURFACE_OK);
    assert(surface.genus == 3);
    check_surface(&surface,0,0);
    billiard_affine_matrix shear, inverse, product;
    assert(billiard_affine_shear(0.5,&shear) == BILLIARD_AFFINE_OK);
    assert(billiard_affine_inverse(shear,&inverse) == BILLIARD_AFFINE_OK);
    assert(billiard_affine_compose(inverse,shear,&product) == BILLIARD_AFFINE_OK);
    near(product.entry[0][0],1); near(product.entry[0][1],0); near(product.entry[1][0],0); near(product.entry[1][1],1);
    billiard_numeric_point vertical = billiard_affine_point(shear,(billiard_numeric_point){0,1});
    near(hypot(vertical.x,vertical.y),sqrt(1.25)); assert(hypot(vertical.x,vertical.y) > 1);
    assert(billiard_affine(1,0,0,0,&product) == BILLIARD_AFFINE_INVALID);
    assert(billiard_affine(1,0,0,-1,&product) == BILLIARD_AFFINE_INVALID);
    billiard_affine_matrix reversal = {{{1,0},{0,-1}},-1,2,0};
    assert(billiard_affine_compose(reversal,reversal,&product) == BILLIARD_AFFINE_INVALID);
    assert(billiard_affine(1e-8,0,0,1e8,&product) == BILLIARD_AFFINE_ILL_CONDITIONED);
    assert(billiard_affine(NAN,0,0,1,&product) == BILLIARD_AFFINE_NONFINITE);
    assert(billiard_affine_shear(INFINITY,&product) == BILLIARD_AFFINE_NONFINITE);
    assert(billiard_affine_diagonal(1000,&product) == BILLIARD_AFFINE_NONFINITE);
    puts("BIL-MODULI-05 exact period witnesses, relative periods, torus tau, genus/stratum invariants and GL+ controls: PASS");
    return 0;
}
