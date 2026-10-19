#include "billiard.h"

#include <float.h>
#include <math.h>
#include <stdint.h>

#define DEG_TO_RAD 0.017453292519943295769236907684886

static int valid_table(const billiard_table *table) {
    return table && isfinite(table->width) && isfinite(table->height) &&
           table->width > 0.0 && table->height > 0.0 &&
           table->start_x > 0.0 && table->start_x < table->width &&
           table->start_y > 0.0 && table->start_y < table->height &&
           isfinite(table->start_x) && isfinite(table->start_y) &&
           isfinite(table->launch_degrees);
}

int billiard_default(billiard_table *table, double width, double height) {
    if (!table || !isfinite(width) || !isfinite(height) ||
        width <= 0.0 || height <= 0.0) return -1;
    *table = (billiard_table){width, height, width * 0.5, height / 3.0, 45.0};
    return valid_table(table) ? 0 : -1;
}

int billiard_begin(const billiard_table *table, billiard_ray *ray) {
    if (!valid_table(table) || !ray) return -1;
    double angle = remainder(table->launch_degrees, 360.0) * DEG_TO_RAD;
    double dx = cos(angle);
    double dy = -sin(angle);
    if (!isfinite(dx) || !isfinite(dy)) return -1;
    /* Treat angles that are exact cardinal multiples as axial directions. */
    if (fabs(dx) < 8 * DBL_EPSILON) dx = 0.0;
    if (fabs(dy) < 8 * DBL_EPSILON) dy = 0.0;
    double magnitude = hypot(dx, dy);
    if (magnitude == 0.0 || !isfinite(magnitude)) return -1;
    *ray = (billiard_ray){table->start_x, table->start_y,
                          dx/magnitude, dy/magnitude, 0.0, 0};
    return 0;
}

static double next_wall_distance(double coordinate, double span, double direction) {
    if (direction > 0.0) return (span - coordinate) / direction;
    if (direction < 0.0) return -coordinate / direction;
    return INFINITY;
}

int billiard_next(const billiard_table *table, billiard_ray *ray,
                  billiard_segment *segment) {
    if (!valid_table(table) || !ray || !segment ||
        !isfinite(ray->x) || !isfinite(ray->y) ||
        !isfinite(ray->dx) || !isfinite(ray->dy) ||
        !isfinite(ray->distance) ||
        ray->x < 0.0 || ray->x > table->width ||
        ray->y < 0.0 || ray->y > table->height) return -1;

    double vertical = next_wall_distance(ray->x, table->width, ray->dx);
    double horizontal = next_wall_distance(ray->y, table->height, ray->dy);
    double distance = fmin(vertical, horizontal);
    if (!isfinite(distance) || distance <= 0.0) return -1;

    /* Only floating-point roundoff counts as simultaneous collision. */
    double uncertainty = 16.0 * DBL_EPSILON * fmax(1.0, distance);
    int strike_vertical = isfinite(vertical) &&
                          fabs(vertical - distance) <= uncertainty;
    int strike_horizontal = isfinite(horizontal) &&
                            fabs(horizontal - distance) <= uncertainty;
    if (!strike_vertical && !strike_horizontal) return -1;

    double end_x = strike_vertical ? (ray->dx > 0.0 ? table->width : 0.0)
                                   : ray->x + ray->dx * distance;
    double end_y = strike_horizontal ? (ray->dy > 0.0 ? table->height : 0.0)
                                     : ray->y + ray->dy * distance;
    if (!isfinite(end_x) || !isfinite(end_y) ||
        end_x < -uncertainty || end_x > table->width + uncertainty ||
        end_y < -uncertainty || end_y > table->height + uncertainty) return -1;
    /* Roundoff at a wall is harmless; the collision classification came first. */
    end_x = fmax(0.0, fmin(table->width, end_x));
    end_y = fmax(0.0, fmin(table->height, end_y));

    *segment = (billiard_segment){ray->x, ray->y, end_x, end_y,
                                  ray->distance, distance,
                                  (enum billiard_hit)((strike_vertical ? 1 : 0) |
                                                      (strike_horizontal ? 2 : 0))};
    ray->x = end_x;
    ray->y = end_y;
    if (strike_vertical) ray->dx = -ray->dx;
    if (strike_horizontal) ray->dy = -ray->dy;
    ray->distance += distance;
    ++ray->collisions;
    return isfinite(ray->distance) ? 0 : -1;
}

size_t billiard_trace(const billiard_table *table, double distance_budget,
                      billiard_segment *segments, size_t capacity) {
    billiard_ray ray;
    if (!segments || capacity == 0 || !isfinite(distance_budget) ||
        distance_budget < 0.0 || billiard_begin(table, &ray)) return SIZE_MAX;
    size_t written = 0;
    while (written < capacity && ray.distance < distance_budget) {
        billiard_ray proposed = ray;
        billiard_segment segment;
        if (billiard_next(table, &proposed, &segment)) return SIZE_MAX;
        double remaining = distance_budget - ray.distance;
        if (remaining < segment.length) {
            segment.x1 = ray.x + ray.dx * remaining;
            segment.y1 = ray.y + ray.dy * remaining;
            segment.length = remaining;
            segment.hit = BILLIARD_NO_HIT;
        } else {
            ray = proposed;
        }
        segments[written++] = segment;
        if (segment.hit == BILLIARD_NO_HIT) break;
    }
    return written;
}

double billiard_fold(double coordinate, double span) {
    if (!isfinite(coordinate) || !isfinite(span) || span <= 0.0) return NAN;
    double period = 2.0 * span;
    if (!isfinite(period)) return NAN;
    double residue = fmod(coordinate, period);
    if (residue < 0.0) residue += period;
    return residue <= span ? residue : period - residue;
}

double billiard_modulus_imaginary(const billiard_table *table) {
    if (!valid_table(table)) return NAN;
    return table->height / table->width;
}
