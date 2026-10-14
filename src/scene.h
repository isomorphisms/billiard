#ifndef BILLIARD_SCENE_H
#define BILLIARD_SCENE_H

#include <stdio.h>

typedef struct {
    double width_factor; /* [0.5,1.4] of initial table width */
    double start_u;      /* horizontal fraction */
    double start_v;      /* vertical fraction from top */
    double launch_degrees;
    double distance;     /* geometric length traversed */
} billiard_controls;

typedef struct {
    double left, top, width, height;
    double start_x, start_y;
    double handle_x, handle_y;
} billiard_layout;

/* Deterministic operations P/A/W/D/M/L for rough.framebuffer.
   Collision geometry stays sharp even when the drawn walls are rough. */
int billiard_scene(FILE *out, unsigned screen_width, unsigned screen_height,
                   const billiard_controls *controls, billiard_layout *layout);

#endif
