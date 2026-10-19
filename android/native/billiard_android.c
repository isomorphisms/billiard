#define _GNU_SOURCE
/* DEX-free NativeActivity adapter: billiard owns geometry, rough.framebuffer
 * owns rasterization; only input, timing and pixels are platform-specific. */
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "billiard.h"
#include "scene.h"
#include "pixel_sample.h"
#include "framebuffer.h"
#include "explorer.h"

#define TAG "Billiard"
#define LIMIT_PIXELS 450000u
#define PI 3.141592653589793238462643383279502884

enum touch_mode {TOUCH_NONE, TOUCH_WIDTH, TOUCH_START, TOUCH_ANGLE};

typedef struct {
    struct android_app *app;
    billiard_controls controls;
    billiard_layout layout;
    uint32_t *pixels;
    unsigned image_width, image_height;
    unsigned window_width, window_height;
    double speed;
    double previous_time;
    int focused, playing, dirty;
    int active_pointer;
    enum touch_mode drag;
    unsigned long frame, input;
    billiard_explorer_config explorer;
    billiard_explorer_model *model;
    billiard_explorer_layout explorer_layout;
    double explorer_distance, explorer_speed;
    int geometry_dirty;
} state;

static double clamp(double x, double a, double b) {
    return fmax(a,fmin(b,x));
}

static double now_seconds(void) {
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)) return 0.0;
    return (double)t.tv_sec + 1e-9 * (double)t.tv_nsec;
}

static void choose_image(unsigned w, unsigned h, unsigned *rw, unsigned *rh) {
    unsigned a = w < 360 ? w : 360;
    unsigned b = (unsigned)round((double)h * a / w);
    if(b < 320) b = 320;
    if(b > BILLIARD_IMAGE_LIMIT) b = BILLIARD_IMAGE_LIMIT;
    *rw=a;*rh=b;
}

static int prepare_image(state *s, unsigned w, unsigned h) {
    if(s->pixels && s->image_width==w && s->image_height==h) return 0;
    size_t count=(size_t)w*h;
    if(!count || count>LIMIT_PIXELS || count>SIZE_MAX/sizeof(uint32_t)) return -1;
    uint32_t *fresh=malloc(count*sizeof(uint32_t));
    if(!fresh) return -1;
    free(s->pixels);
    s->pixels=fresh;s->image_width=w;s->image_height=h;
    return 0;
}

static unsigned painted_count(const uint32_t *pixels,size_t count,uint64_t *hash) {
    unsigned painted=0;
    *hash=UINT64_C(14695981039346656037);
    for(size_t i=0;i<count;i++) {
        uint32_t color=pixels[i];
        if(color!=0xfffbfaf6u)painted++;
        *hash^=color;
        *hash*=UINT64_C(1099511628211);
    }
    return painted;
}

static void paint(state *s) {
    if(!s->app || !s->app->window) return;
    /* Exact topology/flow work must not hold an Android window buffer. */
    if(s->explorer.view!=BILLIARD_EXPLORER_LEGACY) {
        if(!s->model)s->model=calloc(1,sizeof *s->model);
        if(!s->model)return;
        if(s->geometry_dirty) {
            int built=billiard_explorer_build(&s->explorer,s->model);
            if(built && !s->model->geometry_valid)
                __android_log_print(ANDROID_LOG_ERROR,TAG,"explorer geometry rejected");
            s->geometry_dirty=0;
        }
    }
    ANativeWindow_Buffer win;
    if(ANativeWindow_lock(s->app->window,&win,NULL)<0)return;
    if(!win.bits || win.width<160 || win.height<320 ||
       (unsigned)win.width>BILLIARD_WINDOW_LIMIT ||
       (unsigned)win.height>BILLIARD_WINDOW_LIMIT ||
       win.stride<win.width || win.format!=WINDOW_FORMAT_RGBA_8888) {
        ANativeWindow_unlockAndPost(s->app->window); return;
    }
    unsigned ww=(unsigned)win.width,wh=(unsigned)win.height,rw,rh;
    choose_image(ww,wh,&rw,&rh);
    if(prepare_image(s,rw,rh)) {
        ANativeWindow_unlockAndPost(s->app->window);return;
    }
    s->window_width=ww;s->window_height=wh;
    for(size_t i=0;i<(size_t)rw*rh;i++)s->pixels[i]=0xfffbfaf6u;

    char *operations=NULL;
    size_t bytes=0;
    FILE *writer=open_memstream(&operations,&bytes);
    if(!writer) {ANativeWindow_unlockAndPost(s->app->window);return;}
    int valid;
    if(s->explorer.view==BILLIARD_EXPLORER_LEGACY) {
        valid=billiard_scene(writer,rw,rh,&s->controls,&s->layout);
    } else {
        valid=billiard_explorer_scene(writer,rw,rh,s->model,s->explorer_distance,&s->explorer_layout);
    }
    if(!valid)valid=billiard_explorer_chrome(writer,rw,rh,&s->explorer,s->model,s->playing,
        s->explorer.view==BILLIARD_EXPLORER_LEGACY?s->speed:s->explorer_speed);
    if(fclose(writer) || valid) {
        free(operations);ANativeWindow_unlockAndPost(s->app->window);return;
    }
    FILE *reader=fmemopen(operations,bytes,"r");
    if(!reader) {
        free(operations);ANativeWindow_unlockAndPost(s->app->window);return;
    }
    rough_framebuffer target={s->pixels,rw,rh,rw};
    int render=rough_render_stream(reader,&target);
    fclose(reader);free(operations);
    if(render) {
        __android_log_print(ANDROID_LOG_ERROR,TAG,"raster returned %d",render);
        ANativeWindow_unlockAndPost(s->app->window);return;
    }
    /* Convert straight-alpha AARRGGBB pixels to window RGBA_8888 bytes. */
    for(unsigned y=0;y<wh;y++) {
        unsigned sy=billiard_pixel_sample(y,rh,wh);
        uint8_t *row=(uint8_t*)win.bits+(size_t)y*win.stride*4;
        for(unsigned x=0;x<ww;x++) {
            unsigned sx=billiard_pixel_sample(x,rw,ww);
            uint32_t c=s->pixels[(size_t)sy*rw+sx];
            row[x*4]=(uint8_t)(c>>16);
            row[x*4+1]=(uint8_t)(c>>8);
            row[x*4+2]=(uint8_t)c;
            row[x*4+3]=(uint8_t)(c>>24);
        }
    }
    uint64_t hash;
    unsigned painted=painted_count(s->pixels,(size_t)rw*rh,&hash);
    ANativeWindow_unlockAndPost(s->app->window);
    ++s->frame;
    __android_log_print(ANDROID_LOG_INFO,TAG,
        "FRAME seq=%lu input=%lu width_factor=%.4f angle=%.4f travel=%.4f speed=%.2f painted=%u hash=%016llx",
        s->frame,s->input,s->controls.width_factor,s->controls.launch_degrees,
        s->controls.distance,s->speed,painted,(unsigned long long)hash);
    if(s->explorer.view!=BILLIARD_EXPLORER_LEGACY && s->model)
        __android_log_print(ANDROID_LOG_INFO,TAG,
            "MATH frame=%lu fixture=%d view=%d heading=%u start=%u,%u width=%u shear=%d distance=%.6f flow=%d events=%zu periodic=%d polygon_vertex=%d cylinder_status=%d cylinders=%zu",
            s->frame,s->explorer.fixture,s->explorer.view,s->explorer.heading,
            s->explorer.start_horizontal,s->explorer.start_vertical,s->explorer.width_ticks,
            s->explorer.shear_half_steps,s->explorer_distance,s->model->flow_status,
            s->model->event_count,s->model->periodic,s->model->polygon_vertex_stop,
            s->model->cylinder_status,s->model->cylinder_count);
}

static double proximity(double x,double y,double u,double v) {
    return hypot(x-u,y-v);
}

static void enter_mathematics(state *s) {
    if(s->explorer.view!=BILLIARD_EXPLORER_LEGACY)return;
    s->explorer.width_ticks=(unsigned)lround(clamp(16*s->controls.width_factor,8,24));
    s->explorer.start_horizontal=(unsigned)lround(clamp(24*s->controls.start_u,1,23));
    s->explorer.start_vertical=(unsigned)lround(clamp(24*(1-s->controls.start_v),1,23));
    long heading=lround(s->controls.launch_degrees/15);
    s->explorer.heading=(unsigned)((heading%24+24)%24);
    s->explorer.view=BILLIARD_EXPLORER_COVER;
}

static void changed_geometry(state *s) {
    s->geometry_dirty=1;s->dirty=1;s->explorer_distance=0;s->previous_time=now_seconds();
}

static int32_t touch(struct android_app *app,AInputEvent *event) {
    state *s=(state*)app->userData;
    if(AInputEvent_getType(event)!=AINPUT_EVENT_TYPE_MOTION ||
       !s->window_width || !s->window_height)return 0;
    int action=AMotionEvent_getAction(event);
    int kind=action&AMOTION_EVENT_ACTION_MASK;
    size_t count=AMotionEvent_getPointerCount(event);
    if(!count)return 0;
    size_t index=(size_t)((action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>
                          AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    if(kind==AMOTION_EVENT_ACTION_MOVE) {
        index=count;
        for(size_t i=0;i<count;i++)
            if(AMotionEvent_getPointerId(event,i)==s->active_pointer) {
                index=i;break;
            }
        if(index==count || s->active_pointer<0)return 0;
    } else if(kind!=AMOTION_EVENT_ACTION_DOWN &&
              kind!=AMOTION_EVENT_ACTION_POINTER_DOWN &&
              kind!=AMOTION_EVENT_ACTION_UP &&
              kind!=AMOTION_EVENT_ACTION_POINTER_UP &&
              kind!=AMOTION_EVENT_ACTION_CANCEL)return 0;
    if(kind==AMOTION_EVENT_ACTION_CANCEL){
        s->active_pointer=-1;s->drag=TOUCH_NONE;return 1;
    }
    if(index>=count)return 0;
    int id=AMotionEvent_getPointerId(event,index);
    if(kind==AMOTION_EVENT_ACTION_DOWN ||
       kind==AMOTION_EVENT_ACTION_POINTER_DOWN) {
        if(s->active_pointer>=0)return 1;
        s->active_pointer=id;
    } else if(id!=s->active_pointer)return 1;
    double px=AMotionEvent_getX(event,index)*s->image_width/s->window_width;
    double py=AMotionEvent_getY(event,index)*s->image_height/s->window_height;
    if(!isfinite(px)||!isfinite(py))return 0;

    if(kind==AMOTION_EVENT_ACTION_DOWN ||
       kind==AMOTION_EVENT_ACTION_POINTER_DOWN) {
        if(py<38) {
            if(px<s->image_width*.45) s->playing=!s->playing;
            else if(s->explorer.view==BILLIARD_EXPLORER_LEGACY)
                s->speed=s->speed<50.0?50.0:(s->speed<110.0?110.0:20.0);
            else s->explorer_speed=s->explorer_speed<0.5?0.5:(s->explorer_speed<1.1?1.1:0.2);
            s->previous_time=now_seconds();
            s->drag=TOUCH_NONE;
        } else if(py>=38 && py<66) {
            unsigned selected=(unsigned)clamp(floor(px*BILLIARD_FIXTURE_COUNT/s->image_width),0,BILLIARD_FIXTURE_COUNT-1);
            enum billiard_fixture fixture=(enum billiard_fixture)selected;
            s->explorer=billiard_explorer_defaults(fixture,fixture==BILLIARD_RECTANGLE?BILLIARD_EXPLORER_LEGACY:BILLIARD_EXPLORER_COVER);
            if(fixture==BILLIARD_RECTANGLE) {
                s->controls=(billiard_controls){1.0,.5,1.0/3.0,45.0,0.0};s->explorer.show_cylinders=0;
            }
            s->drag=TOUCH_NONE;changed_geometry(s);
        } else if(py>s->image_height-57) {
            unsigned button=(unsigned)clamp(floor(px*3/s->image_width),0,2);
            if(button==0) {
                if(s->explorer.fixture==BILLIARD_RECTANGLE) {
                    if(s->explorer.view==BILLIARD_EXPLORER_LEGACY) {enter_mathematics(s);s->explorer.view=BILLIARD_EXPLORER_POLYGON;}
                    else if(s->explorer.view==BILLIARD_EXPLORER_POLYGON)s->explorer.view=BILLIARD_EXPLORER_COVER;
                    else {
                        s->explorer.view=BILLIARD_EXPLORER_LEGACY;s->explorer.show_cylinders=0;
                        s->controls.width_factor=s->explorer.width_ticks/16.0;
                        s->controls.start_u=s->explorer.start_horizontal/24.0;
                        s->controls.start_v=1-s->explorer.start_vertical/24.0;
                        s->controls.launch_degrees=15*s->explorer.heading;s->controls.distance=0;
                    }
                } else s->explorer.view=s->explorer.view==BILLIARD_EXPLORER_POLYGON?BILLIARD_EXPLORER_COVER:BILLIARD_EXPLORER_POLYGON;
                if(s->explorer.view!=BILLIARD_EXPLORER_COVER)s->explorer.shear_half_steps=0;
            } else if(button==1) {
                enter_mathematics(s);s->explorer.view=BILLIARD_EXPLORER_COVER;
                s->explorer.shear_half_steps=s->explorer.shear_half_steps==2?-1:s->explorer.shear_half_steps+1;
            } else {
                enter_mathematics(s);s->explorer.show_cylinders=!s->explorer.show_cylinders;
            }
            s->drag=TOUCH_NONE;changed_geometry(s);
        } else if(s->explorer.view!=BILLIARD_EXPLORER_LEGACY && s->model) {
            if(proximity(px,py,s->explorer_layout.handle_x,s->explorer_layout.handle_y)<20)s->drag=TOUCH_ANGLE;
            else if(proximity(px,py,s->explorer_layout.start_x,s->explorer_layout.start_y)<20)s->drag=TOUCH_START;
            else if(s->explorer.fixture==BILLIARD_RECTANGLE && proximity(px,py,s->explorer_layout.wall_x,s->explorer_layout.wall_y)<30)s->drag=TOUCH_WIDTH;
            else s->drag=TOUCH_NONE;
        } else if(proximity(px,py,s->layout.handle_x,s->layout.handle_y)<20.0)
            s->drag=TOUCH_ANGLE;
        else if(proximity(px,py,s->layout.start_x,s->layout.start_y)<20.0)
            s->drag=TOUCH_START;
        else if(fabs(px-(s->layout.left+s->layout.width))<30.0 &&
                py>s->layout.top-20.0 &&
                py<s->layout.top+s->layout.height+20.0)
            s->drag=TOUCH_WIDTH;
        else s->drag=TOUCH_NONE;
    } else if(kind==AMOTION_EVENT_ACTION_MOVE) {
        if(s->explorer.view!=BILLIARD_EXPLORER_LEGACY && s->model && s->drag!=TOUCH_NONE) {
            enum billiard_explorer_handle handle=s->drag==TOUCH_WIDTH?BILLIARD_EXPLORER_WIDTH:
                (s->drag==TOUCH_START?BILLIARD_EXPLORER_START:BILLIARD_EXPLORER_ANGLE);
            if(billiard_explorer_drag(s->model,&s->explorer_layout,handle,px,py,&s->explorer)>0)changed_geometry(s);
        } else if(s->drag==TOUCH_WIDTH)
            s->controls.width_factor=clamp((px-s->layout.left)/
                                             (s->image_width*.53),.5,1.4);
        else if(s->drag==TOUCH_START) {
            s->controls.start_u=clamp((px-s->layout.left)/
                                      s->layout.width,.02,.98);
            s->controls.start_v=clamp((py-s->layout.top)/
                                      s->layout.height,.02,.98);
        } else if(s->drag==TOUCH_ANGLE)
            s->controls.launch_degrees=atan2(s->layout.start_y-py,
                                              px-s->layout.start_x)*180.0/PI;
    } else {
        s->active_pointer=-1;s->drag=TOUCH_NONE;
    }
    s->dirty=1;++s->input;
    return 1;
}

static void command(struct android_app *app,int32_t event) {
    state *s=(state*)app->userData;
    switch(event) {
    case APP_CMD_INIT_WINDOW:
        if(app->window && ANativeWindow_setBuffersGeometry(app->window,0,0,
             WINDOW_FORMAT_RGBA_8888)==0)s->dirty=1;
        break;
    case APP_CMD_GAINED_FOCUS:
        s->focused=1;s->previous_time=now_seconds();s->dirty=1;break;
    case APP_CMD_LOST_FOCUS:
        s->focused=0;s->previous_time=now_seconds();break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONTENT_RECT_CHANGED:
    case APP_CMD_CONFIG_CHANGED:
        s->dirty=1;break;
    case APP_CMD_TERM_WINDOW:
        s->dirty=0;s->active_pointer=-1;s->drag=TOUCH_NONE;break;
    default:break;
    }
}

void android_main(struct android_app *app) {
    state s={0};
    s.app=app;
    s.controls=(billiard_controls){1.0,.5,1.0/3.0,45.0,0.0};
    s.speed=50.0;s.playing=1;s.active_pointer=-1;
    s.explorer=billiard_explorer_defaults(BILLIARD_RECTANGLE,BILLIARD_EXPLORER_LEGACY);
    s.explorer.show_cylinders=0;s.explorer_speed=0.5;s.geometry_dirty=1;
    s.previous_time=now_seconds();
    app->userData=&s;
    app->onAppCmd=command;
    app->onInputEvent=touch;
    for(;;) {
        if(app->destroyRequested)break;
        int events=0;
        struct android_poll_source *source=NULL;
        int timeout=s.focused && app->window && s.playing?33:(s.dirty?0:-1);
        int result=ALooper_pollOnce(timeout,NULL,&events,(void**)&source);
        if(result>=0 && source)source->process(app,source);
        while(!app->destroyRequested &&
              ALooper_pollOnce(0,NULL,&events,(void**)&source)>=0) {
            if(source)source->process(app,source);
        }
        if(app->destroyRequested)break;
        double current=now_seconds();
        int advancing=s.playing && s.focused && app->window;
        double *distance=s.explorer.view==BILLIARD_EXPLORER_LEGACY?&s.controls.distance:&s.explorer_distance;
        double speed=s.explorer.view==BILLIARD_EXPLORER_LEGACY?s.speed:s.explorer_speed;
        if(billiard_distance_clock(current,advancing,speed,&s.previous_time,distance))s.playing=0;
        if(advancing) {
            if(s.explorer.view==BILLIARD_EXPLORER_LEGACY) {
                if(s.controls.distance>=20000.0){s.controls.distance=20000.0;s.playing=0;}
            } else if(s.model && !s.geometry_dirty && !s.model->periodic && s.explorer_distance>=s.model->total_length) {
                s.explorer_distance=s.model->total_length;s.playing=0;
            }
            s.dirty=1;
        }
        if(s.dirty && s.focused && app->window) {
            s.dirty=0;paint(&s);
        }
    }
    free(s.pixels);
    free(s.model);
}
