#include "cylinders.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static exact_point integral(int32_t horizontal, int32_t vertical) {
    return (exact_point){exact_integer(horizontal),exact_integer(vertical)};
}
static billiard_surface fixture(enum billiard_fixture kind) {
    billiard_polygon polygon; billiard_surface surface;
    assert(billiard_polygon_fixture(kind,&polygon) == BILLIARD_POLYGON_OK);
    assert(billiard_surface_unfold(&polygon,&surface) == BILLIARD_SURFACE_OK);
    return surface;
}

/* Independent path clearance oracle: restart the straight-flow engine at the
 * recorded cone sector and stop at its first singularity/marked vertex. The
 * cylinder producer instead works with a transverse strip permutation. */
static void check_connection(const billiard_surface *surface, const billiard_cylinder_report *report,
    const billiard_connection *connection) {
    exact_work work = {0};
    exact_number budget = exact_sum(&work,connection->parameter,exact_integer(1));
    billiard_flow flow;
    assert(billiard_flow_begin_vertex(surface,connection->start_copy,connection->start_vertex,
        report->direction,budget,connection->strip_crossing_bound+4,&flow) == BILLIARD_FLOW_OK);
    int found = 0;
    for (unsigned step = 0; step < connection->strip_crossing_bound+8; ++step) {
        billiard_flow_event event;
        enum billiard_flow_status status = billiard_flow_next(surface,&flow,&event);
        if (status == BILLIARD_FLOW_SINGULAR || (connection->marked_point_convention &&
            (status == BILLIARD_FLOW_REGULAR_VERTEX || status == BILLIARD_FLOW_PERIODIC))) {
            assert(event.vertex_class == connection->end_class);
            assert(billiard_exact_equal(flow.parameter,connection->parameter));
            found = 1; break;
        }
        if (status != BILLIARD_FLOW_EDGE && status != BILLIARD_FLOW_REGULAR_VERTEX)
            fprintf(stderr,"connection clearance failed: %s, germ %zu parameter %.17g\n",
                billiard_flow_status_name(status),connection->start_germ,billiard_exact_display(connection->parameter));
        assert(status == BILLIARD_FLOW_EDGE || status == BILLIARD_FLOW_REGULAR_VERTEX);
    }
    assert(found && !work.failed);
}

static void check_report(const billiard_surface *surface, const billiard_cylinder_report *report) {
    assert(report->status == BILLIARD_CYLINDER_CERTIFIED);
    exact_work work = {0}; exact_number area = exact_integer(0);
    unsigned incoming[BILLIARD_MAX_STRIPS] = {0};
    for (size_t strip = 0; strip < report->strip_count; ++strip) {
        assert(report->strip[strip].next < report->strip_count);
        assert(report->strip[strip].cylinder < report->cylinder_count);
        ++incoming[report->strip[strip].next];
    }
    for (size_t strip = 0; strip < report->strip_count; ++strip) assert(incoming[strip] == 1);
    /* Trace an interior point of every strip independently. A shorter return
     * would refute the claimed primitive circumference even if area balances. */
    for (size_t index = 0; index < report->strip_count; ++index) {
        const billiard_direction_strip *strip = &report->strip[index];
        const billiard_cylinder *cylinder = &report->cylinder[strip->cylinder];
        exact_number left_sum = exact_sum(&work,strip->left_lower,strip->left_upper);
        exact_number right_sum = exact_sum(&work,strip->right_lower,strip->right_upper);
        exact_number horizontal = exact_sum(&work,left_sum,right_sum);
        horizontal = exact_quotient(&work,horizontal,exact_integer(4));
        exact_number middle = exact_sum(&work,exact_integer(strip->level),(exact_number){1,0,2});
        exact_number vertical = exact_product(&work,middle,report->transverse_unit);
        exact_number first = exact_product(&work,report->direction.x,horizontal);
        exact_number second = exact_product(&work,report->direction.y,vertical);
        exact_number chart_x = exact_difference(&work,first,second);
        chart_x = exact_quotient(&work,chart_x,report->direction_squared);
        first = exact_product(&work,report->direction.y,horizontal);
        second = exact_product(&work,report->direction.x,vertical);
        exact_number chart_y = exact_sum(&work,first,second);
        chart_y = exact_quotient(&work,chart_y,report->direction_squared);
        exact_point folded = billiard_surface_fold(&work,surface,strip->copy,(exact_point){chart_x,chart_y});
        exact_number budget = exact_sum(&work,cylinder->period_parameter,exact_integer(1));
        assert(!work.failed);
        billiard_flow flow;
        assert(billiard_flow_begin(surface,strip->copy,folded,report->direction,budget,2*BILLIARD_MAX_STRIPS+8,&flow) == BILLIARD_FLOW_OK);
        int returned = 0;
        for (unsigned step = 0; step < 2*BILLIARD_MAX_STRIPS+16; ++step) {
            billiard_flow_event event;
            enum billiard_flow_status status = billiard_flow_next(surface,&flow,&event);
            if (status == BILLIARD_FLOW_PERIODIC) {
                assert(billiard_exact_equal(flow.parameter,cylinder->period_parameter)); returned = 1; break;
            }
            assert(status == BILLIARD_FLOW_EDGE || status == BILLIARD_FLOW_REGULAR_VERTEX);
        }
        assert(returned);
    }
    for (size_t cylinder = 0; cylinder < report->cylinder_count; ++cylinder) {
        area = exact_sum(&work,area,report->cylinder[cylinder].area);
        assert(fabs(report->cylinder[cylinder].circumference*report->cylinder[cylinder].height-
            billiard_exact_display(report->cylinder[cylinder].area)) < 1e-10);
    }
    assert(!work.failed && billiard_exact_equal(area,surface->area));
    for (size_t connection = 0; connection < report->connection_count; ++connection) {
        check_connection(surface,report,&report->connection[connection]);
        for (size_t previous = 0; previous < connection; ++previous)
            assert(report->connection[previous].start_germ != report->connection[connection].start_germ ||
                !exact_point_equal(report->connection[previous].holonomy,report->connection[connection].holonomy));
    }
}

int main(void) {
    billiard_cylinder_limits limits = billiard_cylinder_default_limits();
    for (unsigned kind = 0; kind < BILLIARD_FIXTURE_COUNT; ++kind) {
        billiard_surface surface = fixture((enum billiard_fixture)kind);
        billiard_cylinder_report report;
        enum billiard_cylinder_status status = billiard_cylinders(&surface,integral(1,0),&limits,&report);
        printf("horizontal fixture %u status %d strips %zu cylinders %zu connections %zu\n",
            kind,status,report.strip_count,report.cylinder_count,report.connection_count); fflush(stdout);
        assert(status == BILLIARD_CYLINDER_CERTIFIED);
        check_report(&surface,&report);
        assert(report.cylinder_count == (kind == BILLIARD_L_POLYGON ? 2u : 1u));
        assert(report.connection_count == (kind == BILLIARD_L_POLYGON ? 3u : 0u));
    }
    billiard_surface square = fixture(BILLIARD_SQUARE);
    const int32_t directions[][2] = {{1,0},{1,1},{2,1},{-1,2}};
    for (unsigned direction = 0; direction < sizeof directions/sizeof *directions; ++direction) {
        billiard_cylinder_report report;
        int32_t horizontal = directions[direction][0], vertical = directions[direction][1];
        assert(billiard_cylinders(&square,integral(horizontal,vertical),&limits,&report) == BILLIARD_CYLINDER_CERTIFIED);
        check_report(&square,&report);
        assert(report.cylinder_count == 1 && report.connection_count == 0);
        assert(billiard_exact_equal(report.cylinder[0].period_parameter,exact_integer(2)));
        assert(billiard_exact_equal(report.cylinder[0].circumference_squared,exact_integer(4*(horizontal*horizontal+vertical*vertical))));
    }
    billiard_surface elbow = fixture(BILLIARD_L_POLYGON);
    billiard_cylinder_report diagonal;
    assert(billiard_cylinders(&elbow,integral(1,1),&limits,&diagonal) == BILLIARD_CYLINDER_CERTIFIED);
    check_report(&elbow,&diagonal);
    printf("L diagonal: cylinders %zu connections %zu\n",diagonal.cylinder_count,diagonal.connection_count); fflush(stdout);
    assert(diagonal.cylinder_count == 1 && diagonal.connection_count == 3);
    assert(billiard_exact_equal(diagonal.cylinder[0].period_parameter,exact_integer(6)));
    size_t previous_count = 0;
    for (int32_t bound = 1; bound <= 4; ++bound) {
        billiard_cylinder_report report;
        limits.length_bound = exact_integer(bound);
        assert(billiard_cylinders(&elbow,integral(1,0),&limits,&report) == BILLIARD_CYLINDER_CERTIFIED);
        assert(report.connection_count >= previous_count); previous_count = report.connection_count;
        if (bound == 1) assert(report.connection_count == 0 && report.counted_cylinders == 0);
        if (bound == 2) assert(report.connection_count == 3 && report.counted_cylinders == 1);
        if (bound == 4) assert(report.counted_cylinders == 2);
    }
    limits = billiard_cylinder_default_limits();
    billiard_cylinder_report report;
    limits.max_strips = 1;
    assert(billiard_cylinders(&elbow,integral(1,0),&limits,&report) == BILLIARD_CYLINDER_LIMIT);
    limits = billiard_cylinder_default_limits(); limits.max_crossings = 1;
    assert(billiard_cylinders(&square,integral(1,0),&limits,&report) == BILLIARD_CYLINDER_LIMIT);
    limits = billiard_cylinder_default_limits(); limits.max_connections = 1;
    assert(billiard_cylinders(&elbow,integral(1,0),&limits,&report) == BILLIARD_CYLINDER_LIMIT);
    limits = billiard_cylinder_default_limits();
    exact_point irrational = {exact_integer(1),(exact_number){0,1,1}};
    assert(billiard_cylinders(&square,irrational,&limits,&report) == BILLIARD_CYLINDER_UNDETERMINED);
    exact_number almost; assert(!billiard_exact_make(1001,0,1000,&almost));
    assert(billiard_cylinders(&square,(exact_point){exact_integer(1),almost},&limits,&report) == BILLIARD_CYLINDER_LIMIT);
    billiard_counting_report counts;
    limits.length_bound = exact_integer(4);
    assert(billiard_direction_sweep(&square,4,BILLIARD_MAX_DIRECTIONS,&limits,NULL,&counts) == BILLIARD_CYLINDER_CERTIFIED);
    assert(counts.all_lengths_covered && counts.connections == 0 && counts.closed_families == 4);
    assert(billiard_direction_sweep(&square,4,1,&limits,NULL,&counts) == BILLIARD_CYLINDER_LIMIT);
    assert(counts.truncated && !counts.all_lengths_covered);
    limits.mark_regular_vertices = 1;
    assert(billiard_cylinders(&square,integral(1,0),&limits,&report) == BILLIARD_CYLINDER_CERTIFIED);
    assert(report.connection_count > 0);
    check_report(&square,&report);
    for (size_t connection = 0; connection < report.connection_count; ++connection) assert(report.connection[connection].marked_point_convention);
    square.seam[0][0].target_copy = SIZE_MAX;
    assert(billiard_cylinders(&square,integral(1,0),&limits,&report) == BILLIARD_CYLINDER_INVALID);
    puts("BIL-CYL-04 independent flow clearance, exact area, primitive tori, L regimes, monotone counts and UNKNOWN bounds: PASS");
    return 0;
}
