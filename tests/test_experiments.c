#include "experiments.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *test_revision = "0000000000000000000000000000000000000000";

static int contains(FILE *file, const char *needle) {
    char line[2048]; rewind(file);
    while (fgets(line,sizeof line,file)) if (strstr(line,needle)) return 1;
    return 0;
}

int main(void) {
    billiard_experiment_config config = billiard_experiment_defaults(test_revision,"test_fixture");
    FILE *first = tmpfile(), *second = tmpfile(); assert(first && second);
    billiard_experiment_summary left, right;
    assert(billiard_experiment_run(first,&config,&left) == BILLIARD_EXPERIMENT_OK);
    assert(billiard_experiment_run(second,&config,&right) == BILLIARD_EXPERIMENT_OK);
    assert(left.fixtures == BILLIARD_FIXTURE_COUNT && left.states == 4*BILLIARD_FIXTURE_COUNT);
    assert(left.direction_rows == right.direction_rows && left.observed_connections == right.observed_connections);
    assert(left.horizontal_family_observed && !left.truncated);
    rewind(first); rewind(second);
    for (;;) { int a = fgetc(first), b = fgetc(second); assert(a == b); if (a == EOF) break; }
    assert(contains(first,"individual_orbit_closure\tunknown"));
    assert(contains(first,"asymptotic_counting_constant\tunknown"));
    assert(contains(first,"EMM_general_results_reference\texternally_cited"));
    assert(contains(first,"dataset_complete\t1"));
    fclose(first); fclose(second);

    for (unsigned kind = BILLIARD_CLAIM_ORBIT_CLOSURE; kind < BILLIARD_CLAIM_COUNT; ++kind) {
        assert(billiard_claim_admit((enum billiard_claim_kind)kind,BILLIARD_UNKNOWN));
        assert(!billiard_claim_admit((enum billiard_claim_kind)kind,BILLIARD_FIXTURE_CERTIFIED));
        assert(!billiard_claim_admit((enum billiard_claim_kind)kind,BILLIARD_FINITE_OBSERVED));
    }
    config.fixture_mask = 1u<<BILLIARD_L_POLYGON; config.steps = 1; config.max_directions = 1;
    config.include_raw_certificates = 0;
    first = tmpfile(); assert(first);
    assert(billiard_experiment_run(first,&config,&left) == BILLIARD_EXPERIMENT_OK);
    assert(left.truncated && !left.horizontal_family_observed);
    assert(contains(first,"feature_horizontal_family\tNOT_OBSERVED_IN_SUPPLIED_SAMPLES"));
    assert(!contains(first,"IMPOSSIBLE")); fclose(first);
    config.max_directions = 16;
    first = tmpfile(); assert(first);
    assert(billiard_experiment_run(first,&config,&right) == BILLIARD_EXPERIMENT_OK);
    assert(right.horizontal_family_observed && !right.truncated);
    assert(contains(first,"feature_horizontal_family\tOBSERVED_IN_SUPPLIED_SAMPLES")); fclose(first);
    config.search.length_bound = exact_integer(1);
    first = tmpfile(); assert(first);
    assert(billiard_experiment_run(first,&config,&right) == BILLIARD_EXPERIMENT_OK);
    assert(contains(first,"source_cutoff_incomplete")); fclose(first);
    config = billiard_experiment_defaults(test_revision,"test_fixture");
    config.fixture_mask = 1u<<BILLIARD_SQUARE; config.shear_step = config.diagonal_step = 0;
    config.transformed_length_bound = exact_integer(2); config.include_raw_certificates = 0;
    first = tmpfile(); assert(first);
    assert(billiard_experiment_run(first,&config,&right) == BILLIARD_EXPERIMENT_OK);
    assert(right.uncertain_cutoffs > 0);
    assert(contains(first,"binary64_cutoffs_with_unknown_boundary")); fclose(first);
    config.steps = 0;
    first = tmpfile(); assert(first);
    assert(billiard_experiment_run(first,&config,&right) == BILLIARD_EXPERIMENT_INVALID); fclose(first);
    printf("BIL-ORBIT-06: deterministic %u-state corpus; four-valued claims, missing-feature and coverage controls PASS\n",4*BILLIARD_FIXTURE_COUNT);
    return 0;
}
