#ifndef BILLIARD_EXPERIMENTS_H
#define BILLIARD_EXPERIMENTS_H
#include "affine.h"
#include "cylinders.h"

enum billiard_claim_status { BILLIARD_FIXTURE_CERTIFIED, BILLIARD_EXTERNALLY_CITED,
    BILLIARD_FINITE_OBSERVED, BILLIARD_UNKNOWN };
enum billiard_claim_kind { BILLIARD_CLAIM_TOPOLOGY, BILLIARD_CLAIM_PERIODS,
    BILLIARD_CLAIM_EMM_REFERENCE, BILLIARD_CLAIM_AFFINE_SAMPLE, BILLIARD_CLAIM_FINITE_COUNTS,
    BILLIARD_CLAIM_MISSING_FEATURE, BILLIARD_CLAIM_ORBIT_CLOSURE, BILLIARD_CLAIM_DENSITY,
    BILLIARD_CLAIM_ERGODICITY, BILLIARD_CLAIM_ORBIT_DIMENSION, BILLIARD_CLAIM_ASYMPTOTIC_CONSTANT,
    BILLIARD_CLAIM_COUNT };
typedef struct {
    unsigned fixture_mask, steps, direction_bound, max_directions;
    billiard_cylinder_limits search;
    exact_number transformed_length_bound;
    double shear_step, diagonal_step, rotation_step;
    int include_raw_certificates;
    const char *source_revision, *source_state;
} billiard_experiment_config;
typedef struct {
    unsigned fixtures, states, direction_rows, unresolved_directions, uncertain_cutoffs;
    size_t observed_connections, observed_closed_families;
    int horizontal_family_observed, truncated;
} billiard_experiment_summary;
enum billiard_experiment_status { BILLIARD_EXPERIMENT_OK, BILLIARD_EXPERIMENT_INVALID,
    BILLIARD_EXPERIMENT_LIMIT, BILLIARD_EXPERIMENT_IO_ERROR };

enum billiard_claim_status billiard_claim_status(enum billiard_claim_kind kind);
int billiard_claim_admit(enum billiard_claim_kind kind, enum billiard_claim_status requested_status);
billiard_experiment_config billiard_experiment_defaults(const char *source_revision, const char *source_state);
enum billiard_experiment_status billiard_experiment_run(FILE *output, const billiard_experiment_config *config,
    billiard_experiment_summary *summary);
#endif
