#include "experiments.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifndef BILLIARD_SOURCE_REVISION
#define BILLIARD_SOURCE_REVISION "unrecorded"
#endif
#ifndef BILLIARD_SOURCE_STATE
#define BILLIARD_SOURCE_STATE "unrecorded"
#endif

static int number(const char *text, unsigned maximum, unsigned *result) {
    if (!text || !*text || *text == '-') return -1;
    char *end; errno = 0; unsigned long value = strtoul(text,&end,10);
    if (errno || *end || value > maximum) return -1;
    *result = (unsigned)value; return 0;
}
static void usage(FILE *output) {
    fprintf(output,"billiard-experiments [--fixture 0..4] [--steps 1..16] [--directions 1..16]\n"
        "  [--max-directions 1..512] [--length 0..1000] [--source-length 0..1000] [--summary]\n"
        "Writes a bounded TSV corpus. Limits and unresolved results remain explicit.\n");
}
int main(int argc, char **argv) {
    billiard_experiment_config config = billiard_experiment_defaults(BILLIARD_SOURCE_REVISION,BILLIARD_SOURCE_STATE);
    for (int argument = 1; argument < argc; ++argument) {
        const char *option = argv[argument]; unsigned value;
        if (!strcmp(option,"--help")) { usage(stdout); return 0; }
        if (!strcmp(option,"--summary")) { config.include_raw_certificates = 0; continue; }
        if (++argument >= argc || number(argv[argument],1000,&value)) { usage(stderr); return 2; }
        if (!strcmp(option,"--fixture") && value < BILLIARD_FIXTURE_COUNT) config.fixture_mask = 1u<<value;
        else if (!strcmp(option,"--steps") && value >= 1 && value <= 16) config.steps = value;
        else if (!strcmp(option,"--directions") && value >= 1 && value <= 16) config.direction_bound = value;
        else if (!strcmp(option,"--max-directions") && value >= 1 && value <= BILLIARD_MAX_DIRECTIONS) config.max_directions = value;
        else if (!strcmp(option,"--length")) config.transformed_length_bound = exact_integer((int32_t)value);
        else if (!strcmp(option,"--source-length")) config.search.length_bound = exact_integer((int32_t)value);
        else { usage(stderr); return 2; }
    }
    billiard_experiment_summary summary;
    enum billiard_experiment_status status = billiard_experiment_run(stdout,&config,&summary);
    if (status != BILLIARD_EXPERIMENT_OK) {
        fprintf(stderr,"BIL-ORBIT-06 stopped: status %d; an incomplete output is not an accepted dataset.\n",status);
        return status == BILLIARD_EXPERIMENT_INVALID ? 2 : 3;
    }
    return 0;
}
