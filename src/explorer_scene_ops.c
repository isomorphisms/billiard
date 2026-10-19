#include "explorer.h"
#include <errno.h>
#include <stdlib.h>

static int argument(const char *text, unsigned maximum, unsigned *result) {
    char *end; errno = 0; unsigned long value = strtoul(text,&end,10);
    if (!*text || *text == '-' || errno || *end || value > maximum) return -1;
    *result = (unsigned)value; return 0;
}
int main(int argc, char **argv) {
    unsigned fixture = BILLIARD_L_POLYGON, heading = 0, shear = 0;
    if (argc > 4 || (argc > 1 && argument(argv[1],BILLIARD_FIXTURE_COUNT-1,&fixture)) ||
        (argc > 2 && argument(argv[2],23,&heading)) || (argc > 3 && argument(argv[3],2,&shear))) return 2;
    billiard_explorer_model *model = calloc(1,sizeof *model); if (!model) return 1;
    billiard_explorer_config config = billiard_explorer_defaults((enum billiard_fixture)fixture,BILLIARD_EXPLORER_COVER);
    config.heading = heading; config.shear_half_steps = (int)shear;
    int status = billiard_explorer_build(&config,model);
    billiard_explorer_layout layout;
    if (!status) status = billiard_explorer_scene(stdout,360,640,model,model->total_length*0.72,&layout);
    if (!status) status = billiard_explorer_chrome(stdout,360,640,&config,model,1,0.5);
    free(model); return status ? 1 : 0;
}
