// Build Me!
// arm-linux-gnueabihf-g++ -shared eglwrap.cpp -o libEGL.so -O3 -lIMGegl -lsrv_um -L.

#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <typeinfo>
#include <alloca.h>
#include <string.h>
#define EGL_EGL_PROTOTYPES 1
#include <EGL/egl.h>

extern "C" {
    extern void* PVRSRVLoadLibrary(const char *pszLibraryName);
    extern int PVRSRVUnloadLibrary(void *hExtDrv);
    extern int PVRSRVGetLibFuncAddr(void *hExtDrv, const char *pszFunctionName, void **ppvFuncAddr);
};

template<class F, F f> struct wrapper_impl;
template<class R, class... Args, R(*f)(Args...)>
struct wrapper_impl<R(*)(Args...), f> {
    static R wrap(Args... args) {
        // stuff
        return f(args...);
    }
};

template<class F, F f>
constexpr auto wrapper = wrapper_impl<F, f>::wrap;

#define DO_IMG_WRAP(func) \
    extern "C" { \
        using IMG##func##_proto = decltype(func); \
        extern IMG##func##_proto IMG##func; \
    }

DO_IMG_WRAP(eglBindAPI);
DO_IMG_WRAP(eglBindTexImage);
DO_IMG_WRAP(eglChooseConfig);
DO_IMG_WRAP(eglCopyBuffers);
DO_IMG_WRAP(eglCreateContext);
DO_IMG_WRAP(eglCreatePbufferFromClientBuffer);
DO_IMG_WRAP(eglCreatePbufferSurface);
DO_IMG_WRAP(eglCreatePixmapSurface);
DO_IMG_WRAP(eglCreateWindowSurface);
DO_IMG_WRAP(eglDestroyContext);
DO_IMG_WRAP(eglDestroySurface);
DO_IMG_WRAP(eglGetConfigAttrib);
DO_IMG_WRAP(eglGetConfigs);
DO_IMG_WRAP(eglGetCurrentContext);
DO_IMG_WRAP(eglGetCurrentDisplay);
DO_IMG_WRAP(eglGetCurrentSurface);
DO_IMG_WRAP(eglGetDisplay);
DO_IMG_WRAP(eglGetError);
DO_IMG_WRAP(eglGetProcAddress);
DO_IMG_WRAP(eglInitialize);
DO_IMG_WRAP(eglMakeCurrent);
DO_IMG_WRAP(eglQueryAPI);
DO_IMG_WRAP(eglQueryContext);
DO_IMG_WRAP(eglQueryString);
DO_IMG_WRAP(eglQuerySurface);
DO_IMG_WRAP(eglReleaseTexImage);
DO_IMG_WRAP(eglReleaseThread);
DO_IMG_WRAP(eglSurfaceAttrib);
DO_IMG_WRAP(eglSwapBuffers);
DO_IMG_WRAP(eglSwapInterval);
DO_IMG_WRAP(eglTerminate);
DO_IMG_WRAP(eglWaitClient);
DO_IMG_WRAP(eglWaitGL);
DO_IMG_WRAP(eglWaitNative);

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    return IMGeglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

EGLBoolean eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
    return IMGeglCopyBuffers(dpy, surface, target);
}

/*
    Seems like you absolutely need to set up EGL_CONTEXT_MAJOR_VERSION with PVR, or
    the context creation fails for some reason. Let's look it up.
*/
EGLContext eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *_attrib_list)
{
    EGLint renderable;
    EGLint attrib_list[] = {
        EGL_CONTEXT_MAJOR_VERSION, EGL_FALSE,
        EGL_NONE
    };
    
    eglGetConfigAttrib(dpy, config, EGL_RENDERABLE_TYPE, &renderable);
    attrib_list[1] = 
        (renderable == EGL_OPENGL_ES2_BIT) ? 2 :
        (renderable == EGL_OPENGL_ES_BIT) ? 1 :
        -1;

    if (attrib_list[1] == -1) {
        fprintf(stderr, "EGL_WRAPPER: Invalid EGL_RENDERABLE_TYPE 0x%04X on chosen config!\n", renderable);
        return EGL_NO_CONTEXT;
    }

    return IMGeglCreateContext(dpy, config, share_context, attrib_list);
}

EGLSurface eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    return IMGeglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLSurface eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
    return IMGeglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLSurface eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
    return IMGeglCreateWindowSurface(dpy, config, win, attrib_list);
}

EGLBoolean eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
    return IMGeglDestroyContext(dpy, ctx);
}

EGLBoolean eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
    return IMGeglDestroySurface(dpy, surface);
}

EGLBoolean eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    EGLBoolean ret = IMGeglGetConfigAttrib(dpy, config, attribute, value);
    // printf("eglGetConfigAttrib(0x%04X) => %d\n", attribute, *value);
    return ret;
}

EGLBoolean eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    return IMGeglGetConfigs(dpy, configs, config_size, num_config);
}

EGLDisplay eglGetCurrentDisplay (void)
{
    return IMGeglGetCurrentDisplay();
}

EGLSurface eglGetCurrentSurface (EGLint readdraw)
{
    return IMGeglGetCurrentSurface(readdraw);
}

EGLDisplay eglGetDisplay (EGLNativeDisplayType display_id)
{
    return IMGeglGetDisplay(display_id);
}

EGLint eglGetError (void)
{
    return IMGeglGetError();
}

/*
    eglGetProcAddress here seems to not pull the GLESv* symbols from PVR, so
    we're going to emulate the behavior by pulling the required libraries and then
    look them up manually.
*/
#define MAX_CONFIGS 32
__eglMustCastToProperFunctionPointerType eglGetProcAddress (const char *procname)
{
    __eglMustCastToProperFunctionPointerType ret = IMGeglGetProcAddress(procname);
    if (!ret) {
        EGLint render = EGL_NONE, cfg, no = 0;
        EGLConfig configs[MAX_CONFIGS];

        EGLDisplay dpy = eglGetCurrentDisplay();
        EGLSurface surf = eglGetCurrentSurface(EGL_DRAW);
        eglGetConfigs(dpy, configs, MAX_CONFIGS, &cfg);
        eglQuerySurface(dpy, surf, EGL_CONFIG_ID, &no);

        if (dpy != EGL_NO_DISPLAY && surf != EGL_NO_SURFACE) {
            eglGetConfigAttrib(dpy, configs[no], EGL_RENDERABLE_TYPE, &render);

            const char *soname = (render == EGL_OPENGL_ES2_BIT) ? "libGLESv2.so" : "libGLESv1_CM.so";
            void *lib = PVRSRVLoadLibrary(soname);
            if (!lib) {
                fprintf(stderr, "EGL_WRAPPER: Failed to load '%s'.\n", soname);
                return ret;
            }

            PVRSRVGetLibFuncAddr(lib, procname, (void**)&ret);
        } else {
            fprintf(stderr, "No EGL context available, not looking for '%s'\n", procname);
        }
    };

    return ret;
}

EGLBoolean eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    return IMGeglInitialize(dpy, major, minor);
}

EGLBoolean eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    return IMGeglMakeCurrent(dpy, draw, read, ctx);
}

EGLBoolean eglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    return IMGeglQueryContext(dpy, ctx, attribute, value);
}

const char * eglQueryString (EGLDisplay dpy, EGLint name)
{
    return IMGeglQueryString(dpy, name);
}

EGLBoolean eglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    return IMGeglQuerySurface(dpy, surface, attribute, value);
}

EGLBoolean eglSwapBuffers (EGLDisplay dpy, EGLSurface surface)
{
    return IMGeglSwapBuffers(dpy, surface);
}

EGLBoolean eglTerminate (EGLDisplay dpy)
{
    return IMGeglTerminate(dpy);
}

EGLBoolean eglWaitGL (void)
{
    return IMGeglWaitGL();
}

EGLBoolean eglWaitNative (EGLint engine)
{
    return IMGeglWaitNative(engine);
}

EGLBoolean eglBindTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    return IMGeglBindTexImage(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    return IMGeglReleaseTexImage(dpy, surface, buffer);
}

EGLBoolean eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    return IMGeglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLBoolean eglSwapInterval (EGLDisplay dpy, EGLint interval)
{
    return IMGeglSwapInterval(dpy, interval);
}

EGLBoolean eglBindAPI (EGLenum api)
{
    return IMGeglBindAPI(api);
}

EGLenum eglQueryAPI (void)
{
    return IMGeglQueryAPI();
}

EGLSurface eglCreatePbufferFromClientBuffer (EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
    return IMGeglCreatePbufferFromClientBuffer(dpy, buftype, buffer, config, attrib_list);
}
EGLBoolean eglReleaseThread (void)
{
    return IMGeglReleaseThread();
}

EGLBoolean eglWaitClient (void)
{
    return IMGeglWaitClient();
}

EGLContext eglGetCurrentContext (void)
{
    return IMGeglGetCurrentContext();
}