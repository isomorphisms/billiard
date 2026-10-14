#include "billiard.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

static void near(double got, double wanted, double tolerance) {
    if (!(fabs(got - wanted) <= tolerance)) {
        fprintf(stderr, "got %.17g; wanted %.17g\n", got, wanted);
        assert(0);
    }
}

int main(void) {
    billiard_table table;
    billiard_ray ray;
    billiard_segment segment;
    assert(billiard_default(&table, 480.0, 300.0) == 0);
    near(table.start_x, 240.0, 0.0);
    near(table.start_y, 100.0, 0.0);
    near(table.launch_degrees, 45.0, 0.0);
    assert(billiard_begin(&table, &ray) == 0);
    near(ray.dx, sqrt(0.5), 1e-15);
    near(ray.dy, -sqrt(0.5), 1e-15);
    assert(billiard_next(&table, &ray, &segment) == 0);
    assert(segment.hit == BILLIARD_HORIZONTAL_WALL);
    near(segment.y1, 0.0, 0.0);
    near(ray.dy, sqrt(0.5), 1e-15);

    /* An exact upper-right corner collision reverses the whole incoming ray. */
    assert(billiard_default(&table, 4.0, 6.0) == 0);
    assert(billiard_begin(&table, &ray) == 0);
    double vx = ray.dx, vy = ray.dy;
    assert(billiard_next(&table, &ray, &segment) == 0);
    assert(segment.hit == BILLIARD_CORNER);
    near(ray.x, 4.0, 0.0);
    near(ray.y, 0.0, 0.0);
    near(ray.dx, -vx, 0.0);
    near(ray.dy, -vy, 0.0);
    assert(billiard_next(&table, &ray, &segment) == 0);
    assert(segment.hit == BILLIARD_VERTICAL_WALL);
    near(ray.x, 0.0, 0.0);
    near(ray.y, 4.0, 1e-12);

    /* Axial rays hit one wall, never a phantom infinite wall. */
    assert(billiard_default(&table, 4.0, 6.0) == 0);
    table.launch_degrees = 90.0;
    assert(billiard_begin(&table, &ray) == 0);
    assert(ray.dx == 0.0);
    assert(billiard_next(&table, &ray, &segment) == 0);
    assert(segment.hit == BILLIARD_HORIZONTAL_WALL);
    table.launch_degrees = 0.0;
    assert(billiard_begin(&table, &ray) == 0);
    assert(ray.dy == 0.0);
    assert(billiard_next(&table, &ray, &segment) == 0);
    assert(segment.hit == BILLIARD_VERTICAL_WALL);

    /* A near-corner collision is NOT a reverse-path event. */
    assert(billiard_default(&table, 4.0 + 1e-7, 6.0) == 0);
    assert(billiard_begin(&table, &ray) == 0);
    assert(billiard_next(&table, &ray, &segment) == 0);
    assert(segment.hit != BILLIARD_CORNER);

    /* Changing width changes modulus and collisions, not launch convention. */
    assert(billiard_default(&table, 8.0, 6.0) == 0);
    near(billiard_modulus_imaginary(&table), 0.75, 0.0);
    table.launch_degrees = 30.0;
    assert(billiard_begin(&table, &ray) == 0);
    near(ray.dx, cos(30.0 * acos(-1.0) / 180.0), 1e-15);
    table.start_x = 3.0;
    table.start_y = 1.0;
    assert(billiard_begin(&table, &ray) == 0);
    near(ray.x, 3.0, 0.0);
    near(ray.y, 1.0, 0.0);

    /* Folding an unfolded straight line agrees with 700 consecutive collisions. */
    assert(billiard_default(&table, 29.0, 17.0) == 0);
    table.launch_degrees = 37.0;
    assert(billiard_begin(&table, &ray) == 0);
    const double initial_x = ray.x, initial_y = ray.y;
    const double dir_x = ray.dx, dir_y = ray.dy;
    for (int i = 0; i < 700; ++i) {
        assert(billiard_next(&table, &ray, &segment) == 0);
        near(billiard_fold(initial_x + dir_x * ray.distance, table.width),
             ray.x, 5e-9);
        near(billiard_fold(initial_y + dir_y * ray.distance, table.height),
             ray.y, 5e-9);
    }

    billiard_segment data[512];
    assert(billiard_default(&table, 17.0, 9.0) == 0);
    size_t count = billiard_trace(&table, 137.125, data, 512);
    assert(count > 1 && count < 512);
    near(data[count - 1].distance_start + data[count - 1].length, 137.125, 1e-12);
    assert(data[count - 1].hit == BILLIARD_NO_HIT);
    assert(billiard_trace(&table, 0.0, data, 512) == 0);
    assert(billiard_trace(&table, -1.0, data, 512) == SIZE_MAX);
    assert(billiard_trace(&table, NAN, data, 512) == SIZE_MAX);
    assert(billiard_default(&table, 0.0, 8.0) != 0);
    puts("billiard rectangle/corner/unfold/modulus: PASS");
    return 0;
}
