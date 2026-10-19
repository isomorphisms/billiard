#include "experiments.h"
#include <math.h>
#include <string.h>

enum billiard_claim_status billiard_claim_status(enum billiard_claim_kind kind) {
    switch (kind) {
    case BILLIARD_CLAIM_TOPOLOGY: case BILLIARD_CLAIM_PERIODS: return BILLIARD_FIXTURE_CERTIFIED;
    case BILLIARD_CLAIM_EMM_REFERENCE: return BILLIARD_EXTERNALLY_CITED;
    case BILLIARD_CLAIM_AFFINE_SAMPLE: case BILLIARD_CLAIM_FINITE_COUNTS: case BILLIARD_CLAIM_MISSING_FEATURE:
        return BILLIARD_FINITE_OBSERVED;
    default: return BILLIARD_UNKNOWN;
    }
}
int billiard_claim_admit(enum billiard_claim_kind kind, enum billiard_claim_status requested) {
    return kind >= BILLIARD_CLAIM_TOPOLOGY && kind < BILLIARD_CLAIM_COUNT && requested == billiard_claim_status(kind);
}

billiard_experiment_config billiard_experiment_defaults(const char *source_revision, const char *source_state) {
    billiard_experiment_config config = {0};
    config.fixture_mask = (1u<<BILLIARD_FIXTURE_COUNT)-1; config.steps = 4;
    config.direction_bound = 2; config.max_directions = 16;
    config.search = billiard_cylinder_default_limits(); config.search.length_bound = exact_integer(16);
    config.transformed_length_bound = exact_integer(4);
    config.shear_step = 0.5; config.diagonal_step = 0.2; config.rotation_step = 0.3;
    config.include_raw_certificates = 1; config.source_revision = source_revision; config.source_state = source_state;
    return config;
}

static int revision_valid(const char *revision) {
    if (!revision || strlen(revision) != 40) return 0;
    for (unsigned digit = 0; digit < 40; ++digit)
        if (!((revision[digit] >= '0' && revision[digit] <= '9') || (revision[digit] >= 'a' && revision[digit] <= 'f'))) return 0;
    return 1;
}
static int state_valid(const char *state) {
    return state && (!strcmp(state,"commit") || !strcmp(state,"working_tree") || !strcmp(state,"test_fixture"));
}
static unsigned gcd_unsigned(unsigned first, unsigned second) {
    while (second) { unsigned remainder = first%second; first = second; second = remainder; }
    return first;
}
static unsigned absolute32(int32_t number) { return number < 0 ? (unsigned)(-number) : (unsigned)number; }

static void write_claims(FILE *output) {
    static const char *const claims[BILLIARD_CLAIM_COUNT] = {"source_gluing_topology","source_period_chains","EMM_general_results_reference",
        "finite_affine_samples","finite_metric_counts","feature_not_observed_in_samples","individual_orbit_closure",
        "generic_density","ergodicity","affine_invariant_orbit_dimension","asymptotic_counting_constant"};
    static const char *const statuses[] = {"fixture_certified","externally_cited","finite_observed","unknown"};
    for (unsigned claim = 0; claim < BILLIARD_CLAIM_COUNT; ++claim)
        fprintf(output,"claim\t%s\t%s\n",claims[claim],statuses[billiard_claim_status((enum billiard_claim_kind)claim)]);
    fprintf(output,"external_reference\tEMM2015\thttps://doi.org/10.4007/annals.2015.182.2.7\tAnnals_182_673-721\tgeneral_orbit_closure_and_equidistribution_results_only\n");
    fprintf(output,"unanswered\tWhich individual orbit closure contains each genus-two sample?\n"
        "unanswered\tWhat do longer searches add to these finite counts?\n"
        "unanswered\tNo generic density, orbit dimension or asymptotic constant is inferred.\n");
}

static int exact_matrix(billiard_affine_matrix matrix, billiard_exact_matrix *result) {
    for (unsigned row = 0; row < 2; ++row)
        for (unsigned column = 0; column < 2; ++column)
            if (billiard_exact_from_binary_double(matrix.entry[row][column],&result->entry[row][column])) return 0;
    return 1;
}

/* 1 included, -1 excluded, 0 unresolved. A failed exact predicate is not
 * silently replaced by a rounded certificate. Numerical matrices explicitly
 * use a boundary uncertainty band and remain finite observations. */
static int length_classification(billiard_affine_matrix matrix, const billiard_exact_matrix *exact_action,
    int exact_available, exact_point holonomy, exact_number length_bound) {
    if (exact_available) {
        exact_work work = {0};
        exact_point transformed = exact_transform(&work,*exact_action,holonomy);
        exact_number squared = exact_dot(&work,transformed,transformed);
        exact_number cutoff = exact_product(&work,length_bound,length_bound);
        int order = exact_order(&work,squared,cutoff);
        return work.failed ? 0 : (order <= 0 ? 1 : -1);
    }
    billiard_numeric_point point = {billiard_exact_display(holonomy.x),billiard_exact_display(holonomy.y)};
    point = billiard_affine_point(matrix,point);
    double length = hypot(point.x,point.y), bound = billiard_exact_display(length_bound);
    if (!isfinite(length) || !isfinite(bound) || bound < 0) return 0;
    double uncertainty = 16*BILLIARD_AFFINE_TOLERANCE*(1+length+bound);
    if (fabs(length-bound) <= uncertainty) return 0;
    return length < bound ? 1 : -1;
}

static enum billiard_experiment_status write_directions(FILE *output, const billiard_surface *surface,
    billiard_affine_matrix matrix, const billiard_experiment_config *config, unsigned fixture, unsigned step,
    billiard_experiment_summary *summary) {
    billiard_exact_matrix exact_action;
    int exact_available = exact_matrix(matrix,&exact_action);
    billiard_affine_matrix inverse;
    if (billiard_affine_inverse(matrix,&inverse) != BILLIARD_AFFINE_OK) return BILLIARD_EXPERIMENT_LIMIT;
    double inverse_bound = hypot(hypot(inverse.entry[0][0],inverse.entry[0][1]),hypot(inverse.entry[1][0],inverse.entry[1][1]));
    double required_source_length = inverse_bound*billiard_exact_display(config->transformed_length_bound);
    int sufficient_envelope = billiard_exact_display(config->search.length_bound) >=
        required_source_length*(1+16*BILLIARD_AFFINE_TOLERANCE);
    fprintf(output,"metric_coverage\t%u\t%u\t%s\t%.17g\t%s\n",fixture,step,
        sufficient_envelope ? "source_cutoff_sufficient_numerically" : "source_cutoff_incomplete",
        required_source_length,exact_available ? "bounded_exact_binary_matrix_cutoffs" : "binary64_cutoffs_with_unknown_boundary");
    unsigned examined = 0;
    for (int32_t horizontal = 0; horizontal <= (int32_t)config->direction_bound; ++horizontal)
        for (int32_t vertical = -(int32_t)config->direction_bound; vertical <= (int32_t)config->direction_bound; ++vertical) {
            if ((!horizontal && vertical <= 0) || gcd_unsigned((unsigned)horizontal,absolute32(vertical)) != 1) continue;
            if (examined >= config->max_directions) {
                summary->truncated = 1;
                fprintf(output,"direction_limit\t%u\t%u\t%u\tUNKNOWN_REMAINDER\n",fixture,step,examined);
                return BILLIARD_EXPERIMENT_OK;
            }
            ++examined; ++summary->direction_rows;
            exact_point direction = {exact_integer(horizontal),exact_integer(vertical)};
            billiard_cylinder_report decomposition;
            enum billiard_cylinder_status status = billiard_cylinders(surface,direction,&config->search,&decomposition);
            if (status == BILLIARD_CYLINDER_INVALID) return BILLIARD_EXPERIMENT_INVALID;
            size_t connections = 0, families = 0; unsigned uncertain = 0;
            if (status == BILLIARD_CYLINDER_CERTIFIED) {
                for (size_t connection = 0; connection < decomposition.connection_count; ++connection) {
                    int classification = length_classification(matrix,&exact_action,exact_available,
                        decomposition.connection[connection].holonomy,config->transformed_length_bound);
                    if (classification > 0) ++connections;
                    if (!classification) ++uncertain;
                }
                for (size_t cylinder = 0; cylinder < decomposition.cylinder_count; ++cylinder) {
                    int classification = length_classification(matrix,&exact_action,exact_available,
                        decomposition.cylinder[cylinder].holonomy,config->transformed_length_bound);
                    if (classification > 0) ++families;
                    if (!classification) ++uncertain;
                }
                if (horizontal == 1 && vertical == 0 && decomposition.cylinder_count) summary->horizontal_family_observed = 1;
                summary->observed_connections += connections; summary->observed_closed_families += families;
            } else ++summary->unresolved_directions;
            summary->uncertain_cutoffs += uncertain;
            billiard_numeric_point transported = billiard_affine_point(matrix,(billiard_numeric_point){horizontal,vertical});
            fprintf(output,"count\t%u\t%u\t%d\t%d\t%.17g\t%.17g\t%d\t%zu\t%zu\t%u\t%s\n",fixture,step,horizontal,vertical,
                transported.x,transported.y,status,connections,families,uncertain,sufficient_envelope ? "selected_directions_only" : "selected_directions_and_source_cutoff_only");
            if (config->include_raw_certificates && step == 0) {
                fprintf(output,"raw_direction_certificate\t%u\t%d\t%d\n",fixture,horizontal,vertical);
                if (billiard_cylinders_write(output,&decomposition)) return BILLIARD_EXPERIMENT_IO_ERROR;
            }
        }
    return BILLIARD_EXPERIMENT_OK;
}

enum billiard_experiment_status billiard_experiment_run(FILE *output, const billiard_experiment_config *config,
    billiard_experiment_summary *summary) {
    if (!output || !config || !summary || !revision_valid(config->source_revision) || !state_valid(config->source_state) ||
        !config->fixture_mask || config->fixture_mask >= (1u<<BILLIARD_FIXTURE_COUNT) || !config->steps || config->steps > 16 ||
        !config->direction_bound || config->direction_bound > 16 || !config->max_directions || config->max_directions > BILLIARD_MAX_DIRECTIONS ||
        !isfinite(config->shear_step) || !isfinite(config->diagonal_step) || !isfinite(config->rotation_step) ||
        (config->include_raw_certificates != 0 && config->include_raw_certificates != 1) ||
        !billiard_exact_valid(config->transformed_length_bound) || !billiard_exact_valid(config->search.length_bound) ||
        !config->search.max_strips || config->search.max_strips > BILLIARD_MAX_STRIPS || !config->search.max_crossings ||
        config->search.max_crossings > BILLIARD_MAX_STRIPS || !config->search.max_connections || config->search.max_connections > BILLIARD_MAX_CONNECTIONS ||
        (config->search.mark_regular_vertices != 0 && config->search.mark_regular_vertices != 1)) return BILLIARD_EXPERIMENT_INVALID;
    exact_work validation = {0};
    if (exact_order(&validation,config->transformed_length_bound,exact_integer(0)) < 0 ||
        exact_order(&validation,config->search.length_bound,exact_integer(0)) < 0 || validation.failed)
        return BILLIARD_EXPERIMENT_INVALID;
    billiard_experiment_summary result = {0};
    fprintf(output,"dataset\tBIL-ORBIT-06-v1\nsource_revision\t%s\nsource_state\t%s\ncompiler_source\tc61e448251744a2f40ad743ebef1a027bdcd2f9d\n"
        "compiler_reference\t6294f1d9e7536e5ffcde09d1528c918d63abfef5\nmodel\treflected_translation_covers\n"
        "budgets\tsteps=%u\tdirection_box=%u\tdirection_limit=%u\tstrips=%u\tcycle_crossings=%u\tconnections=%u\n"
        "length_bounds_exact\t%d\t%d\t%d\t%d\t%d\t%d\nnormalization\tprimitive_integer_directions_positive_half_plane\n"
        "status_codes\t0=certified_selected_direction\t1=limit_unknown\t2=undetermined\t3=arithmetic_limit_unknown\t4=invalid\n",
        config->source_revision,config->source_state,config->steps,config->direction_bound,config->max_directions,
        config->search.max_strips,config->search.max_crossings,config->search.max_connections,
        config->search.length_bound.rational,config->search.length_bound.radical,config->search.length_bound.denominator,
        config->transformed_length_bound.rational,config->transformed_length_bound.radical,config->transformed_length_bound.denominator);
    write_claims(output);
    static const char *const fixture_names[] = {"unit_square","five_by_three_rectangle","right_isosceles_triangle","equilateral_triangle","three_square_L"};
    for (unsigned fixture = 0; fixture < BILLIARD_FIXTURE_COUNT; ++fixture) {
        if (!(config->fixture_mask&(1u<<fixture))) continue;
        billiard_polygon polygon; billiard_surface surface; billiard_period_ledger periods;
        if (billiard_polygon_fixture((enum billiard_fixture)fixture,&polygon) != BILLIARD_POLYGON_OK ||
            billiard_surface_unfold(&polygon,&surface) != BILLIARD_SURFACE_OK || billiard_periods(&surface,&periods) != BILLIARD_PERIOD_OK)
            return BILLIARD_EXPERIMENT_INVALID;
        ++result.fixtures;
        fprintf(output,"fixture\t%u\t%s\tcopies=%zu\tgenus=%u\tpositive_zeros=%zu\n",fixture,fixture_names[fixture],surface.copy_count,surface.genus,periods.zero_count);
        if (config->include_raw_certificates && (billiard_surface_write(output,&surface) || billiard_periods_write(output,&periods)))
            return BILLIARD_EXPERIMENT_IO_ERROR;
        billiard_affine_matrix current;
        if (billiard_affine(1,0,0,1,&current) != BILLIARD_AFFINE_OK) return BILLIARD_EXPERIMENT_INVALID;
        for (unsigned step = 0; step < config->steps; ++step) {
            const char *operation = "identity"; double amount = 0;
            if (step) {
                billiard_affine_matrix generator, composed; enum billiard_affine_status status;
                if (step%3 == 1) { operation = "shear"; amount = config->shear_step; status = billiard_affine_shear(amount,&generator); }
                else if (step%3 == 2) { operation = "diagonal"; amount = config->diagonal_step; status = billiard_affine_diagonal(amount,&generator); }
                else { operation = "rotation"; amount = config->rotation_step; status = billiard_affine_rotation(amount,&generator); }
                if (status != BILLIARD_AFFINE_OK || billiard_affine_compose(generator,current,&composed) != BILLIARD_AFFINE_OK) {
                    fprintf(output,"affine_limit\t%u\t%u\tUNKNOWN_REMAINDER\n",fixture,step); return BILLIARD_EXPERIMENT_LIMIT;
                }
                current = composed;
            }
            billiard_deformed_surface deformed;
            if (billiard_affine_apply(&surface,&periods,current,&deformed) != BILLIARD_AFFINE_OK) return BILLIARD_EXPERIMENT_LIMIT;
            fprintf(output,"orbit_step\t%u\t%u\t%s\t%.17g\n",fixture,step,operation,amount);
            if (billiard_affine_write(output,&deformed)) return BILLIARD_EXPERIMENT_IO_ERROR;
            ++result.states;
            enum billiard_experiment_status status = write_directions(output,&surface,current,config,fixture,step,&result);
            if (status != BILLIARD_EXPERIMENT_OK) return status;
        }
    }
    fprintf(output,"feature_horizontal_family\t%s\n",result.horizontal_family_observed ? "OBSERVED_IN_SUPPLIED_SAMPLES" : "NOT_OBSERVED_IN_SUPPLIED_SAMPLES");
    fprintf(output,"summary\tfixtures=%u\tstates=%u\tdirection_rows=%u\tunresolved=%u\tuncertain_cutoffs=%u\tconnections=%zu\tclosed_families=%zu\ttruncated=%d\n",
        result.fixtures,result.states,result.direction_rows,result.unresolved_directions,result.uncertain_cutoffs,result.observed_connections,result.observed_closed_families,result.truncated);
    fprintf(output,"dataset_complete\t1\n");
    if (ferror(output)) return BILLIARD_EXPERIMENT_IO_ERROR;
    *summary = result;
    return BILLIARD_EXPERIMENT_OK;
}
