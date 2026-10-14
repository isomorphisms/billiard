#include "scene.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    billiard_controls scene={1.0,.5,1.0/3.0,45.0,800.0};
    if(argc > 1) scene.width_factor=strtod(argv[1],NULL);
    if(argc > 2) scene.launch_degrees=strtod(argv[2],NULL);
    if(argc > 3) scene.distance=strtod(argv[3],NULL);
    if(argc > 4) return 2;
    billiard_layout layout;
    if(billiard_scene(stdout,360,640,&scene,&layout)) return 1;
    return 0;
}
