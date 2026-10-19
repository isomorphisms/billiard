#include "periods.h"
#include <string.h>

enum billiard_period_status billiard_periods(const billiard_surface *surface, billiard_period_ledger *ledger) {
    if (!ledger) return BILLIARD_PERIOD_INVALID;
    enum billiard_surface_status surface_status = billiard_surface_validate(surface);
    if (surface_status != BILLIARD_SURFACE_OK)
        return surface_status == BILLIARD_SURFACE_ARITHMETIC_LIMIT ? BILLIARD_PERIOD_ARITHMETIC_LIMIT : BILLIARD_PERIOD_INVALID;
    billiard_period_ledger proposed = {0};
    proposed.vertex_count = surface->vertex_class_count; proposed.genus = surface->genus;
    proposed.base_vertex = 0;
    for (size_t vertex = 0; vertex < proposed.vertex_count; ++vertex) {
        proposed.parent_vertex[vertex] = proposed.parent_edge[vertex] = SIZE_MAX;
        if (surface->cone[vertex].zero_order) {
            if (!proposed.zero_count) proposed.base_vertex = vertex;
            proposed.zero_order[proposed.zero_count++] = surface->cone[vertex].zero_order;
        }
    }
    exact_work work = {0};
    for (size_t copy = 0; copy < surface->copy_count; ++copy)
        for (size_t side = 0; side < surface->polygon.count; ++side) {
            size_t opposite = surface->seam[copy][side].target_copy;
            if (copy > opposite) continue;
            if (proposed.edge_count >= BILLIARD_PERIOD_MAX_EDGES) return BILLIARD_PERIOD_LIMIT;
            size_t end = (side+1)%surface->polygon.count;
            exact_point start_point = billiard_surface_vertex(&work,surface,copy,side);
            exact_point end_point = billiard_surface_vertex(&work,surface,copy,end);
            exact_point holonomy = exact_point_difference(&work,end_point,start_point);
            proposed.edge[proposed.edge_count++] = (billiard_period_edge){copy,side,opposite,
                surface->vertex_class[copy][side],surface->vertex_class[copy][end],holonomy,0,0};
        }
    if (work.failed) return BILLIARD_PERIOD_ARITHMETIC_LIMIT;
    if (proposed.edge_count != surface->edge_count) return BILLIARD_PERIOD_INVALID;
    size_t queue[BILLIARD_SURFACE_MAX_CORNERS], begin = 0, end = 1;
    queue[0] = proposed.base_vertex; proposed.parent_vertex[proposed.base_vertex] = proposed.base_vertex;
    proposed.vertex_potential[proposed.base_vertex] = (exact_point){exact_integer(0),exact_integer(0)};
    while (begin < end) {
        size_t vertex = queue[begin++];
        for (size_t edge = 0; edge < proposed.edge_count; ++edge) {
            billiard_period_edge *record = &proposed.edge[edge];
            size_t neighbor; int sign;
            if (record->start_vertex == vertex) { neighbor = record->end_vertex; sign = 1; }
            else if (record->end_vertex == vertex) { neighbor = record->start_vertex; sign = -1; }
            else continue;
            if (proposed.parent_vertex[neighbor] != SIZE_MAX) continue;
            proposed.parent_vertex[neighbor] = vertex; proposed.parent_edge[neighbor] = edge;
            proposed.parent_sign[neighbor] = sign; record->primal_tree = 1;
            exact_point displacement = exact_point_scale(&work,record->holonomy,exact_integer(sign));
            proposed.vertex_potential[neighbor] = exact_point_sum(&work,proposed.vertex_potential[vertex],displacement);
            queue[end++] = neighbor;
        }
    }
    if (end != proposed.vertex_count) return BILLIARD_PERIOD_INVALID;
    unsigned char dual_visited[BILLIARD_SURFACE_MAX_COPIES] = {0};
    begin = 0; end = 1; queue[0] = 0; dual_visited[0] = 1;
    while (begin < end) {
        size_t face = queue[begin++];
        for (size_t edge = 0; edge < proposed.edge_count; ++edge) {
            billiard_period_edge *record = &proposed.edge[edge];
            if (record->primal_tree) continue;
            size_t neighbor;
            if (record->copy == face) neighbor = record->opposite_copy;
            else if (record->opposite_copy == face) neighbor = record->copy;
            else continue;
            if (dual_visited[neighbor]) continue;
            dual_visited[neighbor] = 1; record->dual_tree = 1; queue[end++] = neighbor;
        }
    }
    if (end != surface->copy_count) return BILLIARD_PERIOD_INVALID;
    for (size_t edge = 0; edge < proposed.edge_count; ++edge) {
        const billiard_period_edge *record = &proposed.edge[edge];
        if (record->primal_tree || record->dual_tree) continue;
        if (proposed.period_count >= BILLIARD_PERIOD_MAX_PERIODS) return BILLIARD_PERIOD_LIMIT;
        exact_point period = exact_point_sum(&work,proposed.vertex_potential[record->start_vertex],record->holonomy);
        period = exact_point_difference(&work,period,proposed.vertex_potential[record->end_vertex]);
        proposed.period[proposed.period_count++] = (billiard_period){BILLIARD_PERIOD_ABSOLUTE,edge,period};
        ++proposed.absolute_count;
    }
    if (proposed.absolute_count != 2*surface->genus) return BILLIARD_PERIOD_INVALID;
    for (size_t vertex = 0; vertex < proposed.vertex_count; ++vertex) {
        if (!surface->cone[vertex].zero_order || vertex == proposed.base_vertex) continue;
        if (proposed.period_count >= BILLIARD_PERIOD_MAX_PERIODS) return BILLIARD_PERIOD_LIMIT;
        proposed.period[proposed.period_count++] = (billiard_period){BILLIARD_PERIOD_RELATIVE,vertex,proposed.vertex_potential[vertex]};
        ++proposed.relative_count;
    }
    if (work.failed) return BILLIARD_PERIOD_ARITHMETIC_LIMIT;
    *ledger = proposed;
    return BILLIARD_PERIOD_OK;
}

static int add_tree_path(const billiard_period_ledger *ledger, size_t vertex, int sign, int32_t *coefficients) {
    for (size_t steps = 0; vertex != ledger->base_vertex; ++steps) {
        if (steps >= ledger->vertex_count || vertex >= ledger->vertex_count ||
            ledger->parent_edge[vertex] >= ledger->edge_count ||
            (ledger->parent_sign[vertex] != 1 && ledger->parent_sign[vertex] != -1)) return -1;
        coefficients[ledger->parent_edge[vertex]] += sign*ledger->parent_sign[vertex];
        vertex = ledger->parent_vertex[vertex];
    }
    return 0;
}

enum billiard_period_status billiard_period_chain(const billiard_period_ledger *ledger, size_t index, int32_t coefficients[BILLIARD_PERIOD_MAX_EDGES]) {
    if (!ledger || !coefficients || index >= ledger->period_count || ledger->period_count > BILLIARD_PERIOD_MAX_PERIODS ||
        ledger->edge_count > BILLIARD_PERIOD_MAX_EDGES || ledger->vertex_count > BILLIARD_SURFACE_MAX_CORNERS ||
        ledger->base_vertex >= ledger->vertex_count) return BILLIARD_PERIOD_INVALID;
    memset(coefficients,0,ledger->edge_count*sizeof *coefficients);
    billiard_period period = ledger->period[index];
    if (period.kind == BILLIARD_PERIOD_RELATIVE) {
        if (period.witness >= ledger->vertex_count || add_tree_path(ledger,period.witness,1,coefficients))
            return BILLIARD_PERIOD_INVALID;
    } else if (period.kind == BILLIARD_PERIOD_ABSOLUTE) {
        if (period.witness >= ledger->edge_count) return BILLIARD_PERIOD_INVALID;
        billiard_period_edge edge = ledger->edge[period.witness];
        coefficients[period.witness] = 1;
        if (add_tree_path(ledger,edge.start_vertex,1,coefficients) || add_tree_path(ledger,edge.end_vertex,-1,coefficients))
            return BILLIARD_PERIOD_INVALID;
    } else return BILLIARD_PERIOD_INVALID;
    return BILLIARD_PERIOD_OK;
}

enum billiard_period_status billiard_periods_validate(const billiard_surface *surface, const billiard_period_ledger *ledger) {
    if (!ledger) return BILLIARD_PERIOD_INVALID;
    billiard_period_ledger rebuilt;
    enum billiard_period_status status = billiard_periods(surface,&rebuilt);
    if (status != BILLIARD_PERIOD_OK) return status;
    if (ledger->edge_count != rebuilt.edge_count || ledger->vertex_count != rebuilt.vertex_count ||
        ledger->base_vertex != rebuilt.base_vertex || ledger->period_count != rebuilt.period_count ||
        ledger->absolute_count != rebuilt.absolute_count || ledger->relative_count != rebuilt.relative_count ||
        ledger->zero_count != rebuilt.zero_count || ledger->genus != rebuilt.genus) return BILLIARD_PERIOD_INVALID;
    for (size_t vertex = 0; vertex < rebuilt.vertex_count; ++vertex)
        if (ledger->parent_vertex[vertex] != rebuilt.parent_vertex[vertex] || ledger->parent_edge[vertex] != rebuilt.parent_edge[vertex] ||
            ledger->parent_sign[vertex] != rebuilt.parent_sign[vertex] ||
            !exact_point_equal(ledger->vertex_potential[vertex],rebuilt.vertex_potential[vertex])) return BILLIARD_PERIOD_INVALID;
    for (size_t edge = 0; edge < rebuilt.edge_count; ++edge) {
        billiard_period_edge first = ledger->edge[edge], second = rebuilt.edge[edge];
        if (first.copy != second.copy || first.side != second.side || first.opposite_copy != second.opposite_copy ||
            first.start_vertex != second.start_vertex || first.end_vertex != second.end_vertex || first.primal_tree != second.primal_tree ||
            first.dual_tree != second.dual_tree || !exact_point_equal(first.holonomy,second.holonomy)) return BILLIARD_PERIOD_INVALID;
    }
    for (size_t zero = 0; zero < rebuilt.zero_count; ++zero)
        if (ledger->zero_order[zero] != rebuilt.zero_order[zero]) return BILLIARD_PERIOD_INVALID;
    for (size_t period = 0; period < rebuilt.period_count; ++period)
        if (ledger->period[period].kind != rebuilt.period[period].kind || ledger->period[period].witness != rebuilt.period[period].witness ||
            !exact_point_equal(ledger->period[period].value,rebuilt.period[period].value)) return BILLIARD_PERIOD_INVALID;
    return BILLIARD_PERIOD_OK;
}

int billiard_periods_write(FILE *output, const billiard_period_ledger *ledger) {
    if (!output || !ledger || ledger->period_count > BILLIARD_PERIOD_MAX_PERIODS || ledger->edge_count > BILLIARD_PERIOD_MAX_EDGES ||
        ledger->vertex_count > BILLIARD_SURFACE_MAX_CORNERS || ledger->zero_count > BILLIARD_SURFACE_MAX_CORNERS) return -1;
    fprintf(output,"period_basis\tprimal_tree_and_disjoint_dual_tree\ngenus\t%u\npositive_zeros\t%zu\nabsolute_periods\t%zu\nrelative_periods\t%zu\nbase_vertex\t%zu\n",
        ledger->genus,ledger->zero_count,ledger->absolute_count,ledger->relative_count,ledger->base_vertex);
    for (size_t zero = 0; zero < ledger->zero_count; ++zero) fprintf(output,"zero_order\t%zu\t%u\n",zero,ledger->zero_order[zero]);
    for (size_t edge = 0; edge < ledger->edge_count; ++edge) {
        const billiard_period_edge *record = &ledger->edge[edge];
        fprintf(output,"period_edge\t%zu\t%zu\t%zu\t%zu\t%zu\t%zu\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",edge,
            record->copy,record->side,record->opposite_copy,record->start_vertex,record->end_vertex,record->primal_tree,record->dual_tree,
            record->holonomy.x.rational,record->holonomy.x.radical,record->holonomy.x.denominator,
            record->holonomy.y.rational,record->holonomy.y.radical,record->holonomy.y.denominator);
    }
    for (size_t period = 0; period < ledger->period_count; ++period) {
        billiard_period record = ledger->period[period];
        int32_t coefficients[BILLIARD_PERIOD_MAX_EDGES];
        if (billiard_period_chain(ledger,period,coefficients) != BILLIARD_PERIOD_OK) return -1;
        fprintf(output,"period\t%zu\t%s\t%zu\t%d\t%d\t%d\t%d\t%d\t%d\n",period,
            record.kind == BILLIARD_PERIOD_ABSOLUTE ? "absolute" : "relative",record.witness,
            record.value.x.rational,record.value.x.radical,record.value.x.denominator,
            record.value.y.rational,record.value.y.radical,record.value.y.denominator);
        for (size_t edge = 0; edge < ledger->edge_count; ++edge)
            if (coefficients[edge]) fprintf(output,"period_chain\t%zu\t%zu\t%d\n",period,edge,coefficients[edge]);
    }
    return ferror(output) ? -1 : 0;
}
