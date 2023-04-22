#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GL_GLES_PROTOTYPES 0
#include <GLES2/gl2.h>
#include <GLES2/gl2platform.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <linux/fb.h>
#include <math.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_MODES 32
#define MAX_CONFIGS 32

struct owlfb_sync_info {
    __u8 enabled;
    __u8 disp_id;
    __u16 reserved2;
};

#define OWL_IOW(num, dtype)    _IOW('O', num, dtype)
#define OWLFB_WAITFORVSYNC            OWL_IOW(57,long long)
#define OWLFB_VSYNC_EVENT_EN          OWL_IOW(67, struct owlfb_sync_info)

// To test eglGetProcAddress
PFNGLGETSTRINGPROC glGetString;
PFNGLCLEARPROC glClear;
PFNGLCLEARCOLORPROC glClearColor;

int main(int argc, char *argv[])
{
    int egl_maj, egl_min;
    EGLContext ctx;
    EGLSurface surface;
    EGLConfig configs[MAX_CONFIGS];
    EGLint config_count, i;
    EGLDisplay display;

    if (argc < 2) {
        fprintf(stderr, "You must provide the OpenGL ES major version as an argument.\nUsage:\n$ %s <major>\n", "eglconfig.elf");
        return -1;
    }

    EGLint api_bit = (argv[1][0] == '2') ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_ES_BIT;
    EGLint screen_attribs[] = {
        EGL_RENDERABLE_TYPE, api_bit,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    EGLint mode[MAX_MODES];
    EGLint screen;
    EGLint count, avail_configs;
    EGLint width = 0, height = 0;

    int fbdev_fd = open("/dev/fb0", O_RDWR, 0);
    if (fbdev_fd < 0) {
        printf("failed to open /dev/fb0.\n");
        return -1;
    }

    // Grab the display information
    struct fb_var_screeninfo vinfo = {};
    ioctl(fbdev_fd, FBIOGET_VSCREENINFO, &vinfo);

    // Setup for double-buffered rendering
    vinfo.yres_virtual = vinfo.yres * 2;
    ioctl(fbdev_fd, FBIOPUT_VSCREENINFO, &vinfo);
    ioctl(fbdev_fd, FBIOGET_VSCREENINFO, &vinfo);

    printf("Display: %d x %d\n", vinfo.xres, vinfo.yres);
    printf("Virtual Display: %d x %d\n", vinfo.xres_virtual, vinfo.yres_virtual);

    // Enable RG35XX vsync
    struct owlfb_sync_info sinfo;
    sinfo.enabled = 1;
    if (ioctl(fbdev_fd, OWLFB_VSYNC_EVENT_EN, &sinfo)) {
        printf("OWLFB_VSYNC_EVENT_EN failed\n");
    }

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(display, &egl_maj, &egl_min))
    {
        printf("eglgears: eglInitialize failed: 0x%04X\n", eglGetError());
        return -1;
    }

    printf("EGL version = %d.%d\n", egl_maj, egl_min);
    printf("EGL_VENDOR = %s\n", eglQueryString(display, EGL_VENDOR));

    eglBindAPI(EGL_OPENGL_ES_API);
    if (!eglChooseConfig(display, screen_attribs, configs, MAX_CONFIGS, &avail_configs))
    {
        printf("eglChooseConfig failed!\n");
        return 0;
    }

    printf("eglChooseConfig found %d configs!\n", avail_configs);
    ctx = eglCreateContext(display, configs[0], EGL_NO_CONTEXT, NULL);
    if (ctx == EGL_NO_CONTEXT)
    {
        printf("failed to create context: 0x%04X\n", eglGetError());
        return 0;
    }

    surface = eglCreateWindowSurface(display, configs[0], 0, NULL);
    if (surface == EGL_NO_SURFACE) {
        printf("failed to create window surface: 0x%04X\n", eglGetError());
        return 0;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
    printf("Using screen mode %d x %d.\n", width, height);
    if (!eglMakeCurrent(display, surface, surface, ctx))
    {
        printf("make current failed\n");
        return 0;
    }

    glGetString = (PFNGLGETSTRINGPROC)eglGetProcAddress("glGetString");
    glClearColor = (PFNGLCLEARCOLORPROC)eglGetProcAddress("glClearColor");
    glClear = (PFNGLCLEARPROC)eglGetProcAddress("glClear");

    if (glGetString == NULL || glClearColor == NULL || glClear == NULL) {
        printf("Failed to acquire one or more GL functions!\n");
        return 0;
    }

    eglSwapInterval(display, 1);
    printf("GL_RENDERER   = %s\n", (char *)glGetString(GL_RENDERER));
    printf("GL_VERSION    = %s\n", (char *)glGetString(GL_VERSION));
    printf("GL_VENDOR     = %s\n", (char *)glGetString(GL_VENDOR));
    printf("GL_EXTENSIONS: \n");
    const char *exts = glGetString(GL_EXTENSIONS);
    if (!exts) {
        printf(" ?\n");
    }
    else {
        do {
            const char *end = strchr(exts, ' ');
            if (end == NULL) {
                end = exts + strlen(exts);  // use end of string if no space is found
            }
            printf(" * %.*s\n", (int)(end - exts), exts);
            exts = (*end == ' ') ? end + 1 : end;  // move past space if found, otherwise stay at end of string
        } while (*exts != '\0');
    }

    long long _arg = 0;
    for (int i = 0; i < 600; i++) {
        // Obnoxious pattern to make vsync issues apparent
        glClearColor(((i % 3) == 0) * 1.f, ((i % 3) == 1) * 1.f, ((i % 3) == 2) * 1.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        /* necessary for proper VSYNC */
        ioctl(fbdev_fd, OWLFB_WAITFORVSYNC, &_arg);
        eglSwapBuffers(display, surface);
    }
}
