#include "polygon.h"

#include <limits.h>
#include <math.h>

typedef billiard_exact exact;
typedef billiard_exact_point point;
typedef struct { int failed; } calculation;

static uint32_t magnitude(int32_t value) {
    return value < 0 ? (uint32_t)(-value) : (uint32_t)value;
}

static uint32_t gcd(uint32_t first, uint32_t second) {
    while (second) {
        uint32_t remainder = first % second;
        first = second;
        second = remainder;
    }
    return first;
}

int billiard_exact_make(int32_t rational, int32_t radical, int32_t denominator,
                       exact *result) {
    if (!result || denominator <= 0 || rational == INT32_MIN || radical == INT32_MIN)
        return -1;
    uint32_t divisor = gcd(gcd(magnitude(rational), magnitude(radical)),
                           (uint32_t)denominator);
    *result = (exact){rational / (int32_t)divisor, radical / (int32_t)divisor,
                      denominator / (int32_t)divisor};
    return 0;
}

static exact integer(int32_t value) { return (exact){value, 0, 1}; }
static int canonical(exact number) {
    exact reduced;
    return !billiard_exact_make(number.rational, number.radical, number.denominator,
                               &reduced) && reduced.rational == number.rational &&
           reduced.radical == number.radical && reduced.denominator == number.denominator;
}
static int valid_point(point value) { return canonical(value.x) && canonical(value.y); }

static int32_t multiply_integer(calculation *work, int32_t first, int32_t second) {
    int32_t result;
    if (__builtin_mul_overflow(first, second, &result) || result == INT32_MIN) {
        work->failed = 1;
        return 0;
    }
    return result;
}

static int32_t add_integer(calculation *work, int32_t first, int32_t second) {
    int32_t result;
    if (__builtin_add_overflow(first, second, &result) || result == INT32_MIN) {
        work->failed = 1;
        return 0;
    }
    return result;
}

static exact reduced(calculation *work, int32_t rational, int32_t radical,
                     int32_t denominator) {
    exact result = integer(0);
    if (work->failed || billiard_exact_make(rational, radical, denominator, &result))
        work->failed = 1;
    return result;
}

static exact negative(exact number) {
    return (exact){-number.rational, -number.radical, number.denominator};
}

static exact add(calculation *work, exact first, exact second) {
    int32_t common = (int32_t)gcd((uint32_t)first.denominator,
                                 (uint32_t)second.denominator);
    int32_t left = second.denominator / common;
    int32_t right = first.denominator / common;
    int32_t rational_left = multiply_integer(work, first.rational, left);
    int32_t rational_right = multiply_integer(work, second.rational, right);
    int32_t radical_left = multiply_integer(work, first.radical, left);
    int32_t radical_right = multiply_integer(work, second.radical, right);
    int32_t rational = add_integer(work, rational_left, rational_right);
    int32_t radical = add_integer(work, radical_left, radical_right);
    int32_t denominator = multiply_integer(work, first.denominator, left);
    return reduced(work, rational, radical, denominator);
}

static exact subtract(calculation *work, exact first, exact second) {
    return add(work, first, negative(second));
}

static exact multiply(calculation *work, exact first, exact second) {
    /* Cancel common rational factors before forming products. */
    int32_t left_cancel = (int32_t)gcd(gcd(magnitude(first.rational),
        magnitude(first.radical)), (uint32_t)second.denominator);
    int32_t right_cancel = (int32_t)gcd(gcd(magnitude(second.rational),
        magnitude(second.radical)), (uint32_t)first.denominator);
    first.rational /= left_cancel; first.radical /= left_cancel;
    second.denominator /= left_cancel;
    second.rational /= right_cancel; second.radical /= right_cancel;
    first.denominator /= right_cancel;
    int32_t rational_product = multiply_integer(work, first.rational, second.rational);
    int32_t radical_product = multiply_integer(work, first.radical, second.radical);
    radical_product = multiply_integer(work, 3, radical_product);
    int32_t mixed_left = multiply_integer(work, first.rational, second.radical);
    int32_t mixed_right = multiply_integer(work, first.radical, second.rational);
    int32_t rational = add_integer(work, rational_product, radical_product);
    int32_t radical = add_integer(work, mixed_left, mixed_right);
    int32_t denominator = multiply_integer(work, first.denominator, second.denominator);
    return reduced(work, rational, radical, denominator);
}

static exact divide(calculation *work, exact first, exact second) {
    if (second.radical == 0) {
        if (!second.rational) { work->failed = 1; return integer(0); }
        exact reciprocal = {second.denominator, 0, second.rational};
        if (reciprocal.denominator < 0) {
            reciprocal.denominator = -reciprocal.denominator;
            reciprocal.rational = -reciprocal.rational;
        }
        return multiply(work, first, reciprocal);
    }
    int32_t rational_square = multiply_integer(work, second.rational, second.rational);
    int32_t radical_square = multiply_integer(work, second.radical, second.radical);
    radical_square = multiply_integer(work, 3, radical_square);
    int32_t norm = add_integer(work, rational_square, -radical_square);
    if (work->failed || !norm) { work->failed = 1; return integer(0); }
    exact conjugate = {second.rational, -second.radical, 1};
    exact reciprocal = multiply(work, conjugate, integer(second.denominator));
    reciprocal = divide(work, reciprocal, integer(norm));
    return multiply(work, first, reciprocal);
}

static int sign(exact number) {
    int32_t rational = number.rational, radical = number.radical;
    if (!radical) return (rational > 0) - (rational < 0);
    if (!rational) return radical > 0 ? 1 : -1;
    if ((rational > 0) == (radical > 0)) return rational > 0 ? 1 : -1;
    /* Each coefficient has magnitude <= INT32_MAX. Even 3*b*b fits uint64_t.
     * Squaring here is exact and needs no wide integer division. */
    uint64_t rational_square = (uint64_t)magnitude(rational) * magnitude(rational);
    uint64_t radical_square = UINT64_C(3) * magnitude(radical) * magnitude(radical);
    int comparison = rational_square > radical_square ? 1 : -1;
    return rational > 0 ? comparison : -comparison;
}

static point difference(calculation *work, point first, point second) {
    exact x = subtract(work, first.x, second.x);
    exact y = subtract(work, first.y, second.y);
    return (point){x, y};
}
static point scale(calculation *work, point vector, exact factor) {
    exact x = multiply(work, vector.x, factor);
    exact y = multiply(work, vector.y, factor);
    return (point){x, y};
}
static point translate(calculation *work, point origin, point displacement) {
    exact x = add(work, origin.x, displacement.x);
    exact y = add(work, origin.y, displacement.y);
    return (point){x, y};
}
static exact cross(calculation *work, point first, point second) {
    exact left = multiply(work, first.x, second.y);
    exact right = multiply(work, first.y, second.x);
    return subtract(work, left, right);
}
static exact dot(calculation *work, point first, point second) {
    exact left = multiply(work, first.x, second.x);
    exact right = multiply(work, first.y, second.y);
    return add(work, left, right);
}

double billiard_exact_display(exact number) {
    if (!canonical(number)) return NAN;
    return ((double)number.rational + (double)number.radical * sqrt(3.0)) /
           number.denominator;
}

int billiard_exact_valid(exact number) { return canonical(number); }
int billiard_exact_equal(exact first, exact second) {
    return canonical(first) && canonical(second) &&
        first.rational == second.rational && first.radical == second.radical &&
        first.denominator == second.denominator;
}

#define FIELD_OPERATION(name, operation) \
int name(exact first, exact second, exact *result) { \
    if (!result || !canonical(first) || !canonical(second)) return -1; \
    calculation work = {0}; \
    exact proposed = operation(&work, first, second); \
    if (work.failed) return -1; \
    *result = proposed; \
    return 0; \
}
FIELD_OPERATION(billiard_exact_add, add)
FIELD_OPERATION(billiard_exact_subtract, subtract)
FIELD_OPERATION(billiard_exact_multiply, multiply)
FIELD_OPERATION(billiard_exact_divide, divide)
#undef FIELD_OPERATION

int billiard_exact_compare(exact first, exact second, int *ordering) {
    if (!ordering || !canonical(first) || !canonical(second)) return -1;
    calculation work = {0};
    exact delta = subtract(&work, first, second);
    if (work.failed) return -1;
    *ordering = sign(delta);
    return 0;
}

enum billiard_polygon_status billiard_polygon_fixture(enum billiard_fixture kind,
                                                     billiard_polygon *polygon) {
    if (!polygon || kind < 0 || kind >= BILLIARD_FIXTURE_COUNT) return BILLIARD_POLYGON_INVALID;
    const int32_t vertices[BILLIARD_FIXTURE_COUNT][6][2] = {
        {{0,0},{1,0},{1,1},{0,1}}, {{0,0},{5,0},{5,3},{0,3}},
        {{0,0},{2,0},{0,2}}, {{0,0},{2,0},{1,0}},
        {{0,0},{2,0},{2,1},{1,1},{1,2},{0,2}}
    };
    billiard_polygon proposed = {0};
    proposed.count = kind == BILLIARD_RIGHT_TRIANGLE || kind == BILLIARD_EQUILATERAL ? 3 :
        (kind == BILLIARD_L_POLYGON ? 6 : 4);
    for (size_t vertex = 0; vertex < proposed.count; ++vertex) {
        proposed.vertex[vertex] = (point){integer(vertices[kind][vertex][0]), integer(vertices[kind][vertex][1])};
        proposed.angle[vertex] = (billiard_angle){1,2};
    }
    if (kind == BILLIARD_RIGHT_TRIANGLE)
        proposed.angle[1] = proposed.angle[2] = (billiard_angle){1,4};
    if (kind == BILLIARD_EQUILATERAL) {
        proposed.vertex[2].y = (exact){0,1,1};
        for (size_t vertex = 0; vertex < 3; ++vertex) proposed.angle[vertex] = (billiard_angle){1,3};
    }
    if (kind == BILLIARD_L_POLYGON) proposed.angle[3] = (billiard_angle){3,2};
    enum billiard_polygon_status status = billiard_polygon_validate(&proposed);
    if (status == BILLIARD_POLYGON_OK) *polygon = proposed;
    return status;
}

static int on_segment(calculation *work, point candidate, point start, point end) {
    point before = difference(work, candidate, start);
    point after = difference(work, candidate, end);
    return sign(cross(work, before, after)) == 0 && sign(dot(work, before, after)) <= 0;
}

static int intersect(calculation *work, point a, point b, point c, point d) {
    point first = difference(work, b, a), second = difference(work, d, c);
    point ca = difference(work, c, a), da = difference(work, d, a);
    point ac = difference(work, a, c), bc = difference(work, b, c);
    int side_c = sign(cross(work, first, ca)), side_d = sign(cross(work, first, da));
    int side_a = sign(cross(work, second, ac)), side_b = sign(cross(work, second, bc));
    if (side_c * side_d < 0 && side_a * side_b < 0) return 1;
    return (!side_c && on_segment(work, c, a, b)) ||
           (!side_d && on_segment(work, d, a, b)) ||
           (!side_a && on_segment(work, a, c, d)) ||
           (!side_b && on_segment(work, b, c, d));
}

static point direction_for(unsigned heading) {
    const point first_quadrant[6] = {
        {{1,0,1},{0,0,1}}, {{2,1,1},{1,0,1}}, {{0,1,1},{1,0,1}},
        {{1,0,1},{1,0,1}}, {{1,0,1},{0,1,1}}, {{1,0,1},{2,1,1}}
    };
    point direction = first_quadrant[heading % 6];
    for (unsigned turn = 0; turn < heading / 6; ++turn)
        direction = (point){negative(direction.y), direction.x};
    return direction;
}

static int heading_for(calculation *work, point edge, point first_edge) {
    /* Conjugate multiplication removes the arbitrary initial rotation without
     * a square root or normalization of either edge length. */
    exact along = dot(work, edge, first_edge);
    exact across = cross(work, first_edge, edge);
    point relative = {along, across};
    for (unsigned heading = 0; heading < 24; ++heading) {
        point reference = direction_for(heading);
        if (!sign(cross(work, relative, reference)) && sign(dot(work, relative, reference)) > 0)
            return (int)heading;
        if (work->failed) return -1;
    }
    return -1;
}

enum billiard_polygon_status billiard_polygon_validate(const billiard_polygon *polygon) {
    if (!polygon || polygon->count < 3 || polygon->count > BILLIARD_POLYGON_MAX_VERTICES)
        return BILLIARD_POLYGON_INVALID;
    calculation work = {0};
    point edges[BILLIARD_POLYGON_MAX_VERTICES];
    int headings[BILLIARD_POLYGON_MAX_VERTICES];
    exact twice_area = integer(0);
    for (size_t vertex = 0; vertex < polygon->count; ++vertex)
        if (!valid_point(polygon->vertex[vertex])) return BILLIARD_POLYGON_INVALID;
    for (size_t side = 0; side < polygon->count; ++side) {
        point start = polygon->vertex[side], end = polygon->vertex[(side+1)%polygon->count];
        edges[side] = difference(&work, end, start);
        exact area_piece = cross(&work, start, end);
        twice_area = add(&work, twice_area, area_piece);
        if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
        if (!sign(edges[side].x) && !sign(edges[side].y)) return BILLIARD_POLYGON_INVALID;
    }
    if (sign(twice_area) <= 0) return BILLIARD_POLYGON_INVALID;
    for (size_t first = 0; first < polygon->count; ++first)
        for (size_t second = first+1; second < polygon->count; ++second) {
            if (second == first+1 || (first == 0 && second+1 == polygon->count)) continue;
            int crossing = intersect(&work, polygon->vertex[first],
                polygon->vertex[(first+1)%polygon->count], polygon->vertex[second],
                polygon->vertex[(second+1)%polygon->count]);
            if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
            if (crossing) return BILLIARD_POLYGON_INVALID;
        }
    for (size_t side = 0; side < polygon->count; ++side) {
        headings[side] = heading_for(&work, edges[side], edges[0]);
        if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
        if (headings[side] < 0) return BILLIARD_POLYGON_UNSUPPORTED_ANGLE;
    }
    unsigned sum = 0;
    for (size_t vertex = 0; vertex < polygon->count; ++vertex) {
        int turn = headings[vertex] - headings[(vertex+polygon->count-1)%polygon->count];
        if (turn > 12) turn -= 24;
        if (turn < -12) turn += 24;
        if (turn == 12 || turn == -12) return BILLIARD_POLYGON_INVALID;
        unsigned interior = (unsigned)(12-turn);
        billiard_angle claim = polygon->angle[vertex];
        if (!claim.numerator || !claim.denominator ||
            12u*claim.numerator != interior*claim.denominator)
            return BILLIARD_POLYGON_UNSUPPORTED_ANGLE;
        sum += interior;
    }
    return sum == 12u*(polygon->count-2) ? BILLIARD_POLYGON_OK : BILLIARD_POLYGON_INVALID;
}

enum billiard_polygon_status billiard_polygon_side_normal(const billiard_polygon *polygon,
    size_t side, point *outward_normal) {
    if (!outward_normal) return BILLIARD_POLYGON_INVALID;
    enum billiard_polygon_status status = billiard_polygon_validate(polygon);
    if (status != BILLIARD_POLYGON_OK) return status;
    if (side >= polygon->count) return BILLIARD_POLYGON_INVALID;
    calculation work = {0};
    point edge = difference(&work, polygon->vertex[(side+1)%polygon->count], polygon->vertex[side]);
    if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
    *outward_normal = (point){edge.y,negative(edge.x)};
    return BILLIARD_POLYGON_OK;
}

enum billiard_polygon_status billiard_polygon_locate(const billiard_polygon *polygon,
                                                    point position, int *location) {
    if (!polygon || !location || polygon->count < 3 ||
        polygon->count > BILLIARD_POLYGON_MAX_VERTICES || !valid_point(position))
        return BILLIARD_POLYGON_INVALID;
    calculation work = {0};
    int winding = 0;
    for (size_t side = 0; side < polygon->count; ++side) {
        point start = polygon->vertex[side], end = polygon->vertex[(side+1)%polygon->count];
        if (!valid_point(start) || !valid_point(end)) return BILLIARD_POLYGON_INVALID;
        int boundary = on_segment(&work, position, start, end);
        if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
        if (boundary) { *location = 0; return BILLIARD_POLYGON_OK; }
        point relative = difference(&work, position, start);
        point edge = difference(&work, end, start);
        int orientation = sign(cross(&work, edge, relative));
        int start_y = sign(subtract(&work, start.y, position.y));
        int end_y = sign(subtract(&work, end.y, position.y));
        if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
        if (start_y <= 0 && end_y > 0 && orientation > 0) ++winding;
        if (start_y > 0 && end_y <= 0 && orientation < 0) --winding;
    }
    *location = winding ? 1 : -1;
    return BILLIARD_POLYGON_OK;
}

static int from_binary_double(double value, exact *number) {
    if (!isfinite(value)) return -1;
    int32_t denominator = 1;
    while (value != trunc(value)) {
        if (denominator > INT32_MAX/2 || fabs(value) > INT32_MAX/2) return -1;
        value *= 2.0;
        denominator *= 2;
    }
    if (fabs(value) > INT32_MAX) return -1;
    return billiard_exact_make((int32_t)value, 0, denominator, number);
}

enum billiard_polygon_status billiard_polygon_rectangle(const billiard_table *table,
                                                       billiard_polygon *polygon) {
    if (!table || !polygon || !isfinite(table->width) || !isfinite(table->height) ||
        table->width <= 0 || table->height <= 0) return BILLIARD_POLYGON_INVALID;
    exact width, height, zero = integer(0);
    if (from_binary_double(table->width, &width) || from_binary_double(table->height, &height))
        return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
    billiard_polygon result = {0};
    result.count = 4;
    result.vertex[0] = (point){zero, zero}; result.vertex[1] = (point){width, zero};
    result.vertex[2] = (point){width, height}; result.vertex[3] = (point){zero, height};
    for (size_t vertex = 0; vertex < 4; ++vertex) result.angle[vertex] = (billiard_angle){1,2};
    enum billiard_polygon_status status = billiard_polygon_validate(&result);
    if (status == BILLIARD_POLYGON_OK) *polygon = result;
    return status;
}

enum billiard_polygon_status billiard_polygon_begin(const billiard_polygon *polygon,
    point position, point direction, uint32_t step_limit, billiard_polygon_ray *ray) {
    if (!ray || !valid_point(direction) || (!sign(direction.x) && !sign(direction.y)))
        return BILLIARD_POLYGON_INVALID;
    enum billiard_polygon_status status = billiard_polygon_validate(polygon);
    if (status != BILLIARD_POLYGON_OK) return status;
    int location;
    status = billiard_polygon_locate(polygon, position, &location);
    if (status != BILLIARD_POLYGON_OK) return status;
    if (location != 1) return BILLIARD_POLYGON_INVALID;
    *ray = (billiard_polygon_ray){position, direction, SIZE_MAX, step_limit, 0.0, 0};
    return BILLIARD_POLYGON_OK;
}

enum billiard_polygon_status billiard_polygon_next(const billiard_polygon *polygon,
    billiard_polygon_ray *ray, billiard_polygon_event *event) {
    if (!ray || !event || !valid_point(ray->position) || !valid_point(ray->direction) ||
        !isfinite(ray->distance) || ray->distance < 0 ||
        (!sign(ray->direction.x) && !sign(ray->direction.y))) return BILLIARD_POLYGON_INVALID;
    enum billiard_polygon_status status = billiard_polygon_validate(polygon);
    if (status != BILLIARD_POLYGON_OK) return status;
    if (ray->stopped) return BILLIARD_POLYGON_STOPPED;
    if (!ray->steps_remaining) return BILLIARD_POLYGON_STEP_LIMIT;
    calculation work = {0};
    int location;
    status = billiard_polygon_locate(polygon, ray->position, &location);
    if (status != BILLIARD_POLYGON_OK) return status;
    if (location < 0) return BILLIARD_POLYGON_INVALID;
    if (location == 0) {
        if (ray->previous_side >= polygon->count) return BILLIARD_POLYGON_INVALID;
        point start = polygon->vertex[ray->previous_side];
        point end = polygon->vertex[(ray->previous_side+1)%polygon->count];
        int boundary = on_segment(&work, ray->position, start, end);
        point edge = difference(&work, end, start);
        int inward = sign(cross(&work, edge, ray->direction));
        if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
        if (!boundary || inward <= 0) return BILLIARD_POLYGON_INVALID;
        point before = difference(&work, ray->position, start);
        point after = difference(&work, ray->position, end);
        if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
        if ((!sign(before.x) && !sign(before.y)) || (!sign(after.x) && !sign(after.y)))
            return BILLIARD_POLYGON_INVALID;
    }
    size_t chosen_side = SIZE_MAX, chosen_vertex = SIZE_MAX;
    exact nearest = integer(0);
    for (size_t side = 0; side < polygon->count; ++side) {
        point start = polygon->vertex[side];
        point edge = difference(&work, polygon->vertex[(side+1)%polygon->count], start);
        point offset = difference(&work, start, ray->position);
        exact determinant = cross(&work, ray->direction, edge);
        if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
        if (!sign(determinant)) continue;
        exact ray_numerator = cross(&work, offset, edge);
        exact edge_numerator = cross(&work, offset, ray->direction);
        exact parameter = divide(&work, ray_numerator, determinant);
        exact fraction = divide(&work, edge_numerator, determinant);
        int fraction_end = sign(subtract(&work, fraction, integer(1)));
        if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
        if (sign(parameter) <= 0 || sign(fraction) < 0 || fraction_end > 0) continue;
        size_t vertex = !sign(fraction) ? side : (!fraction_end ? (side+1)%polygon->count : SIZE_MAX);
        int order = chosen_side == SIZE_MAX ? -1 : sign(subtract(&work, parameter, nearest));
        if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
        if (order < 0) { nearest = parameter; chosen_side = side; chosen_vertex = vertex; }
        else if (!order && (vertex == SIZE_MAX || vertex != chosen_vertex))
            return BILLIARD_POLYGON_INVALID; /* impossible in a simple polygon */
    }
    if (chosen_side == SIZE_MAX) return BILLIARD_POLYGON_INVALID;
    point displacement = scale(&work, ray->direction, nearest);
    point end = translate(&work, ray->position, displacement);
    point outgoing = ray->direction;
    if (chosen_vertex == SIZE_MAX) {
        point edge = difference(&work, polygon->vertex[(chosen_side+1)%polygon->count],
                                polygon->vertex[chosen_side]);
        exact projection = dot(&work, ray->direction, edge);
        exact squared_length = dot(&work, edge, edge);
        exact factor = divide(&work, projection, squared_length);
        factor = multiply(&work, integer(2), factor);
        point twice_projection = scale(&work, edge, factor);
        outgoing = difference(&work, twice_projection, ray->direction);
    }
    if (work.failed) return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
    double length = hypot(billiard_exact_display(displacement.x),
                          billiard_exact_display(displacement.y));
    if (!isfinite(length) || length <= 0 || !isfinite(ray->distance+length))
        return BILLIARD_POLYGON_ARITHMETIC_LIMIT;
    status = chosen_vertex == SIZE_MAX ? BILLIARD_POLYGON_EDGE : BILLIARD_POLYGON_VERTEX_STOP;
    point event_outgoing = chosen_vertex == SIZE_MAX ? outgoing : (point){integer(0),integer(0)};
    *event = (billiard_polygon_event){status, chosen_side, chosen_vertex, ray->position,
        end, ray->direction, event_outgoing, nearest, length, ray->distance};
    ray->position = end;
    ray->direction = outgoing;
    ray->previous_side = chosen_side;
    ray->distance += length;
    ray->steps_remaining--;
    ray->stopped = status == BILLIARD_POLYGON_VERTEX_STOP;
    return status;
}
