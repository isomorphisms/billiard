#include "scene.h"
#include "billiard.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#define PI 3.141592653589793238462643383279502884

static double clamp(double value, double low, double high) {
    return fmax(low, fmin(high, value));
}

/* Stable drawing decoration; it never moves a collision wall. */
static void sketch_wall(FILE *out, double ax, double ay,
                        double bx, double by, double seed) {
    double dx = bx - ax, dy = by - ay;
    double length = hypot(dx, dy);
    if (!(length > 0.0)) return;
    double nx = -dy / length, ny = dx / length;
    unsigned parts = (unsigned)fmax(2.0, ceil(length / 13.0));
    fprintf(out, "M %.5f %.5f\n", ax, ay);
    for (unsigned i = 1; i < parts; ++i) {
        double t = (double)i / parts;
        double jitter = sin(2.35*i + seed) * 0.85 * sin(t * PI);
        fprintf(out, "L %.5f %.5f\n", ax + dx*t + nx*jitter,
                ay + dy*t + ny*jitter);
    }
    fprintf(out, "L %.5f %.5f\n", bx, by);
}

static void rectangle(FILE *out, double x, double y, double w, double h) {
    fprintf(out,"M %.5f %.5f\nL %.5f %.5f\nL %.5f %.5f\n"
                "L %.5f %.5f\nL %.5f %.5f\n",
            x,y, x+w,y, x+w,y+h, x,y+h, x,y);
}

int billiard_scene(FILE *out, unsigned screen_width, unsigned screen_height,
                   const billiard_controls *controls, billiard_layout *layout) {
    if (!out || !controls || !layout || screen_width < 160 ||
        screen_height < 320 || screen_width > 4096 || screen_height > 4096 ||
        !isfinite(controls->width_factor) || !isfinite(controls->start_u) ||
        !isfinite(controls->start_v) || !isfinite(controls->launch_degrees) ||
        !isfinite(controls->distance) || controls->distance < 0.0) return -1;

    double sw = screen_width, sh = screen_height;
    double left = sw * .075, top = sh * .115;
    double width = fmin(sw * .83, sw * .53 * clamp(controls->width_factor, .5, 1.4));
    double height = fmin(sh * .32, sw * .45);
    double u = clamp(controls->start_u, .02, .98);
    double v = clamp(controls->start_v, .02, .98);
    double angle = controls->launch_degrees;
    double distance = fmin(controls->distance, 20000.0);
    billiard_table table = {width,height,width*u,height*v,angle};
    billiard_ray initial;
    if (billiard_begin(&table, &initial)) return -1;
    *layout=(billiard_layout){left,top,width,height,
                             left+table.start_x,top+table.start_y,
                             left+table.start_x+initial.dx*29.0,
                             top+table.start_y+initial.dy*29.0};

    /* Rough visible walls, exact mathematical mirror positions. */
    fprintf(out,"P 50 64 73\nA .82\nW 1.35\nD 0 0\n");
    sketch_wall(out,left,top,left+width,top,1.0);
    sketch_wall(out,left+width,top,left+width,top+height,2.0);
    sketch_wall(out,left+width,top+height,left,top+height,3.0);
    sketch_wall(out,left,top+height,left,top,4.0);
    fprintf(out,"A .37\nW .72\n");
    sketch_wall(out,left,top+.8,left+width,top+.8,5.0);
    sketch_wall(out,left+width-.8,top,left+width-.8,top+height,6.0);

    billiard_segment segments[512];
    size_t count = billiard_trace(&table,distance,segments,512);
    if(count == SIZE_MAX) return -1;
    fprintf(out,"P 30 105 172\nA .20\nW 1.15\nD 1.05 6.0\n"
                "M %.5f %.5f\n",layout->start_x,layout->start_y);
    for(size_t i=0;i<count;i++)
        fprintf(out,"L %.5f %.5f\n",left+segments[i].x1,top+segments[i].y1);

    /* The reflected grid is far more transparent than the physical path. */
    double ghost_scale = .31;
    double gw = width*ghost_scale, gh = height*ghost_scale;
    double gx = sw*.10 + 2.0*gw;
    double gy = top+height + sh*.085 + 1.0*gh;
    fprintf(out,"P 110 120 135\nA .075\nW .7\nD .95 6.5\n");
    for(int ix=-2;ix<=3;ix++)
        for(int iy=-1;iy<=3;iy++)
            rectangle(out,gx+ix*gw,gy+iy*gh,gw,gh);
    fprintf(out,"P 35 118 172\nA .15\nW 1.0\nD 1 6\n"
                "M %.5f %.5f\nL %.5f %.5f\n",
            gx+table.start_x*ghost_scale,gy+table.start_y*ghost_scale,
            gx+(table.start_x+initial.dx*distance)*ghost_scale,
            gy+(table.start_y+initial.dy*distance)*ghost_scale);

    /* Moving endpoint, launch handle, start dot and draggable right wall dot. */
    double tip_x=count?left+segments[count-1].x1:layout->start_x;
    double tip_y=count?top+segments[count-1].y1:layout->start_y;
    fprintf(out,"P 25 95 127\nA .67\nW 1.0\nD 0 0\n"
                "M %.5f %.5f\nL %.5f %.5f\n"
                "P 225 130 45\nA .93\nW 4.0\n"
                "M %.5f %.5f\nL %.5f %.5f\n"
                "P 30 120 145\nA .82\nW 3.5\n"
                "M %.5f %.5f\nL %.5f %.5f\n"
                "P 60 90 120\nW 5\nA .72\n"
                "M %.5f %.5f\nL %.5f %.5f\n",
            layout->start_x,layout->start_y,layout->handle_x,layout->handle_y,
            tip_x,tip_y,tip_x,tip_y,
            layout->start_x,layout->start_y,
            layout->start_x,layout->start_y,
            left+width,top+height*.5,
            left+width,top+height*.5);
    return ferror(out) ? -1 : 0;
}
