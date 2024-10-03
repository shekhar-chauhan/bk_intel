#include "igt.h"

/*
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "xe/xe_ioctl.h"
#include "xe/xe_query.h"
#include "intel_pat.h"
#include "intel_blt.h"
#include "intel_mocs.h"
 
#include <unistd.h>
#include <termios.h>
*/

IGT_TEST_DESCRIPTION("test");

 
typedef struct {
        int drm_fd;
        igt_display_t display; //struct for disp state (how disp is configured)
        igt_output_t *output; // struct for disp o/p (eg monitor)
        enum pipe pipe; // gpu display pipe
        uint32_t devid; //devid for drm driv
 
 
        struct igt_fb primary_fb[2]; //array of fbs for primary plane, 2 buffs for double buff
        igt_render_copyfunc_t rendercopy; // copy of render copy func for copying data to gpu
        struct intel_batchbuffer *batch; // holds gpu commands
 
        struct intel_buf *igtbo[2];
        struct buf_ops *bops;
 
        struct intel_bb *ibb; //intel batch buffer
 
        igt_render_copyfunc_t render_copy;
} data_t;
 
igt_main
{
        static data_t data = {}; //initializes everything to zero, null etc.
 
        igt_fixture {
                data.drm_fd = drm_open_driver_master(DRIVER_ANY);
                igt_display_require(&data.display, data.drm_fd); //ensures there is a disp, and then initializes the disp struct
                data.devid = intel_get_drm_devid(data.drm_fd); //devid for gpu
                kmstest_set_vt_graphics_mode(); // switches console to gfx mode to allow visual changes
        }
 
        igt_subtest_f("testi") { //subtest declrn, and _f stands for formatted string
                drmModeModeInfo *drm_mode; //holds info about current display mode(reso, refreshrate)
                igt_plane_t *plane; // display plane (cursor, primary etc)
                struct igt_fb tiledfb; // fb obj in tiled format
 
                data.output = igt_get_single_output_for_pipe(&data.display, PIPE_A); // gets output for a pipe connected to disp
                igt_require(data.output); // if no disp or pipe, test fails
                drm_mode = igt_output_get_mode(data.output); //gets mode info (res, refreshrate etc)
 
                igt_output_set_pipe(data.output, PIPE_A); //use pipe A

                /*
                creates a fb filled with a pattern.
                data.drm_fd                            - fd for drm device (gpu) opened via drm_open_driver_master
                drm_mode->hdisplay, drm_mode->vdisplay - horz and vert reso obtained from drmModeModeInfo
                                                         this creates a fb with horz x vert
                DRM_FORMAT_XYUV8888                    - pixel format, YUV 4:4:4, y is luminance, u - blue diff chrome, v - red diff chroma
                                                         packed in 8 bits per channel.
                I915_FORMAT_MOD_X_TILED                - arranges pixel data in memory in small "tiles" rather than linearly
                                                         this optimizes memory access patterns for gpus, improving perf.
                &tiledfb                               - instance of struct igt_fb, holds all the info - format, reso, buff etc.
                
                igt_fb.c
                igt_create_pattern_fb() calls igt_create_fb() which handles actual creation of fb.
                igt_create_fb() - allocates buffer on gpu. params - drm_fd, w, h, pixel_format, modifier(tiling), fb
                after creating the fb, it is filled with pattern : igt_paint_test_pattern(cairo, width, height);
                cairo is a 2d gfx library to draw the pattern.
                all of this is created and then stored in &tiledfb for use.
                */
                igt_create_pattern_fb(data.drm_fd, drm_mode->hdisplay, drm_mode->vdisplay,
                                        DRM_FORMAT_XYUV8888,
                                        I915_FORMAT_MOD_X_TILED,
                                        &tiledfb);
 
                plane = igt_output_get_plane_type(data.output, DRM_PLANE_TYPE_PRIMARY); //retrieves the primary plane
                igt_plane_set_fb(plane, &tiledfb); //assigns fb to primary plane
                igt_display_commit_atomic(&data.display, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL); // atomic commit so all changes across reso, fb, plane are applied at once
 
                igt_info("press return\n"); // on console
                getchar();
        }

        igt_subtest_f("test juha") { 
                // igt_create_color_fb with different color 
                // and then set for cursor plane different framebuffer
                drmModeModeInfo *drm_mode;
                igt_plane_t *plane;
                struct igt_fb tiled_fb1, tiled_fb2;
                igt_pipe_crc_t *pipe_crc = data->pipe_crc;
                igt_crc_t *crc1, *crc2;

                data.output = igt_get_single_output_for_pipe(&data.display, PIPE_A);
                igt_require(data.output);
                drm_mode = igt_output_get_mode(data.output);
                igt_output_set_pipe(data.output, PIPE_A);
                igt_create_color_pattern_fb(data.drm_fd, drm_mode->hdisplay, drm_mode->vdisplay,
                                                DRM_FORMAT_XYUV8888,
                                                I915_FORMAT_MOD_X_TILED,
                                                &tiled_fb1);
                
                plane = igt_output_get_plane_type(data.output, DRM_PLANE_TYPE_PRIMARY);
                igt_plane_set_fb(plane, &tiled_fb1);
                igt_display_commit_atomic(&data.display, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);

                igt_pipe_crc_get_current(data->drm_fd, pipe_crc, &crc1);

                sleep(5); //wait until screen is visible again

                igt_create_color_pattern_fb(data.drm_fd, drm_mode->hdisplay, drm_mode->vdisplay,
                                                DRM_FORMAT_ABGR1555,
                                                I915_FORMAT_MOD_X_TILED,
                                                &tiled_fb2);
                igt_plane_set_fb(plane, &tiled_fb2);
                igt_display_commit_atomic(&data.display, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
                igt_pipe_crc_get_current(data->drm_fd, pipe_crc, &crc2);

                igt_assert_crc_equal(&crc1, &crc2);
                
                igt_info("press return\n");
                getchar();
        }
 
        igt_fixture {
                kmstest_set_vt_text_mode(); //set it back to text mode
                igt_display_fini(&data.display); //cleans the disp struct and frees mem resources
        }
}