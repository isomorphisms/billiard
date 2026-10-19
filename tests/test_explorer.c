#include "explorer.h"
#include "scene.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void pixel_file(const char *path) {
    FILE *input = fopen(path,"rb"); assert(input);
    char header[64]; unsigned width, height, maximum;
    assert(fgets(header,sizeof header,input) && !strcmp(header,"P6\n"));
    assert(fgets(header,sizeof header,input) && sscanf(header,"%u %u",&width,&height) == 2);
    assert(fgets(header,sizeof header,input) && sscanf(header,"%u",&maximum) == 1 && maximum == 255);
    assert(width == 360 && height == 640);
    size_t ink = 0, top_ink = 0, geometry_ink = 0, unfolding_ink = 0, footer_ink = 0;
    for (unsigned vertical = 0; vertical < height; ++vertical)
        for (unsigned horizontal = 0; horizontal < width; ++horizontal) {
            unsigned char color[3]; assert(fread(color,1,3,input) == 3);
            if (color[0] != 255 || color[1] != 255 || color[2] != 255) {
                ++ink;
                if (vertical < 70) ++top_ink;
                if (vertical >= 90 && vertical < 300) ++geometry_ink;
                if (vertical >= 340 && vertical < 562) ++unfolding_ink;
                if (vertical >= 580) ++footer_ink;
            }
        }
    assert(fgetc(input) == EOF); fclose(input);
    assert(ink > 500 && ink < (size_t)width*height/2 && top_ink > 50 && geometry_ink > 50 && unfolding_ink > 20 && footer_ink > 50);
    printf("ICK math-to-Rough pixels %s: ink=%zu header=%zu geometry=%zu unfolding=%zu controls=%zu PASS\n",
        path,ink,top_ink,geometry_ink,unfolding_ink,footer_ink);
}

int main(int argc, char **argv) {
    if (argc > 1) { for (int file = 1; file < argc; ++file) pixel_file(argv[file]); return 0; }
    assert(sizeof(billiard_explorer_model) <= 512u*1024u);
    billiard_explorer_model *model = malloc(sizeof *model); assert(model);
    for (unsigned fixture = 0; fixture < BILLIARD_FIXTURE_COUNT; ++fixture)
        for (unsigned heading = 0; heading < 24; ++heading) {
            billiard_explorer_config config = billiard_explorer_defaults((enum billiard_fixture)fixture,BILLIARD_EXPLORER_COVER);
            config.heading = heading;
            assert(!billiard_explorer_build(&config,model));
            assert(model->geometry_valid && model->event_count <= config.event_limit && isfinite(model->total_length));
            assert(model->surface.genus == (fixture == BILLIARD_L_POLYGON ? 2u : 1u));
            if (model->periodic) assert(model->flow_status == BILLIARD_FLOW_PERIODIC);
        }
    billiard_explorer_config config = billiard_explorer_defaults(BILLIARD_SQUARE,BILLIARD_EXPLORER_POLYGON);
    assert(!billiard_explorer_build(&config,model));
    assert(model->polygon_vertex_stop && !model->periodic && model->event_count == 1);
    config.view = BILLIARD_EXPLORER_COVER;
    assert(!billiard_explorer_build(&config,model) && model->periodic && !model->polygon_vertex_stop);
    config = billiard_explorer_defaults(BILLIARD_L_POLYGON,BILLIARD_EXPLORER_COVER);
    assert(!billiard_explorer_build(&config,model));
    assert(model->flow_status == BILLIARD_FLOW_SINGULAR && !model->periodic);
    config.heading = 0; config.event_limit = 1;
    assert(!billiard_explorer_build(&config,model));
    assert(model->flow_status == BILLIARD_FLOW_CROSSING_LIMIT && !model->periodic);
    config.event_limit = BILLIARD_EXPLORER_MAX_EVENTS; config.shear_half_steps = 1;
    assert(!billiard_explorer_build(&config,model));
    billiard_explorer_layout layout; FILE *output = tmpfile(); assert(output);
    assert(!billiard_explorer_scene(output,360,640,model,0.7,&layout));
    assert(!billiard_explorer_chrome(output,360,640,&config,model,1,0.5));
    assert(layout.scale > 0 && layout.start_x > 0 && layout.start_x < 360 && layout.start_y > 0 && layout.start_y < 640);
    billiard_explorer_config saved = config;
    double outside_x = layout.origin_x+layout.scale*(1.5+0.5*1.5), outside_y = layout.origin_y-layout.scale*1.5;
    assert(billiard_explorer_drag(model,&layout,BILLIARD_EXPLORER_START,outside_x,outside_y,&config) == 0);
    assert(config.start_horizontal == saved.start_horizontal && config.start_vertical == saved.start_vertical);
    double inside_x = layout.origin_x+layout.scale*(0.25+0.5*0.75), inside_y = layout.origin_y-layout.scale*0.75;
    assert(billiard_explorer_drag(model,&layout,BILLIARD_EXPLORER_START,inside_x,inside_y,&config) == 1);
    assert(config.start_horizontal == 3 && config.start_vertical == 9);
    fclose(output);
    config = billiard_explorer_defaults(BILLIARD_RECTANGLE,BILLIARD_EXPLORER_COVER);
    assert(!billiard_explorer_build(&config,model));
    output = tmpfile(); assert(output);
    assert(!billiard_explorer_scene(output,360,640,model,0,&layout));
    assert(billiard_explorer_drag(model,&layout,BILLIARD_EXPLORER_WIDTH,layout.origin_x+7*layout.scale,layout.wall_y,&config) == 1);
    assert(config.width_ticks == 22);
    assert(!billiard_explorer_build(&config,model));
    assert(billiard_exact_equal(model->source_width,(exact_number){55,0,8})); fclose(output);
    config.view = BILLIARD_EXPLORER_POLYGON; config.shear_half_steps = 1;
    assert(billiard_explorer_build(&config,model) != 0);
    double previous = 10, distance = 0;
    assert(!billiard_distance_clock(10.05,1,2,&previous,&distance)); assert(fabs(distance-0.1) < 1e-12);
    assert(!billiard_distance_clock(500,0,2,&previous,&distance)); assert(fabs(distance-0.1) < 1e-12);
    assert(!billiard_distance_clock(500.05,1,2,&previous,&distance)); assert(fabs(distance-0.2) < 1e-12);
    assert(!billiard_distance_clock(900,1,2,&previous,&distance)); assert(fabs(distance-0.36) < 1e-12);
    assert(billiard_distance_clock(NAN,1,2,&previous,&distance) != 0);
    billiard_controls legacy = {1,.5,1.0/3.0,45,10}; billiard_layout old_layout;
    output = tmpfile(); assert(output);
    assert(!billiard_scene(output,360,640,&legacy,&old_layout)); fclose(output);
    free(model);
    puts("BIL-DEVICE-07 host controls: five fixtures x 24 headings, truthful stops/limits, inverse-affine drag, wall control and pause clock PASS");
    return 0;
}
