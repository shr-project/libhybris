#define MESA_EGL_NO_X11_HEADERS
#include <EGL/egl.h>
#include <assert.h>
#include <stdio.h>

#include "dummy_native_window.h"

int main(int argc, char **argv)
{
    EGLDisplay display;
    EGLConfig ecfg;
    EGLint num_config;
    EGLint attr[] = {       // some attributes to set up our egl-interface
        EGL_BUFFER_SIZE, 32,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLSurface surface;
    EGLint ctxattr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext context;

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        printf("ERROR: Could not get default display\n");
        return -1;
    }

    printf("INFO: Successfully retrieved default display!\n");

    eglInitialize(display, 0, 0);
    eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);

    printf("INFO: Initialized display with default configuration\n");

    // ANativeWindow *window = android_create_native_window();
    printf("INFO: Created dummy native window\n");

    struct dummy_native_window *window;

    window = dummy_native_window_create();

    surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, (EGLNativeWindowType) &window->native, NULL);
    assert(surface != EGL_NO_SURFACE);

    printf("INFO: Created our main window surface\n");

    context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
    assert(surface != EGL_NO_CONTEXT);

    printf("INFO: Created context for display\n");

    assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);

    printf("INFO: Made context and surface current for display\n");

    while (1) { }

    printf("stop\n");

#if 0
(*egldestroycontext)((EGLDisplay) display, context);
    printf("destroyed context\n");

    (*egldestroysurface)((EGLDisplay) display, surface);
    printf("destroyed surface\n");
    (*eglterminate)((EGLDisplay) display);
    printf("terminated\n");
    android_dlclose(baz);
#endif
}
