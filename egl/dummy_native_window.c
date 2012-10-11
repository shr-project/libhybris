#include <stdio.h>

#include "dummy_native_window.h"
#include "support.h"

#include <hardware/gralloc.h>

#define NO_ERROR                0L
#define BAD_VALUE               -1

#define FRAMEBUFFER_WIDTH       720
#define FRAMEBUFFER_HEIGHT      1280
#define FRAMEBUFFER_FORMAT      5

void _window_common_incRef(struct android_native_base_t* base)
{
    printf("dummy_native_window_common_incRef\n");
}

void _window_common_decRef(struct android_native_base_t* base)
{
    printf("dummy_native_window_common_decRef\n");
}

int _window_setSwapInterval(struct ANativeWindow* window, int interval)
{
    printf("dummy_native_window_setSwapInterval: interval=%i\n", interval);
    return 0;
}

int _window_dequeueBuffer(struct ANativeWindow* window, void** buffer)
{
    printf("dummy_native_window_dequeueBuffer\n");
    return 0;
}

int _window_lockBuffer(struct ANativeWindow* window, void* buffer)
{
    printf("dummy_native_window_lockBuffer\n");
    return 0;
}

int _window_queueBuffer(struct ANativeWindow* window, void* buffer)
{
    printf("dummy_native_window_queueBuffer\n");
    return 0;
}

int _window_query(const struct ANativeWindow* window, int what, int* value)
{
    printf("window_query: what = %i\n", what);

    switch (what) {
        case NATIVE_WINDOW_WIDTH:
            *value = FRAMEBUFFER_WIDTH;
            return NO_ERROR;
        case NATIVE_WINDOW_HEIGHT:
            *value = FRAMEBUFFER_HEIGHT;
            return NO_ERROR;
        case NATIVE_WINDOW_FORMAT:
            *value = FRAMEBUFFER_FORMAT;
            return NO_ERROR;
        case NATIVE_WINDOW_CONCRETE_TYPE:
            *value = NATIVE_WINDOW_FRAMEBUFFER;
            return NO_ERROR;
        case NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER:
            *value = 0;
            return NO_ERROR;
        case NATIVE_WINDOW_DEFAULT_WIDTH:
            *value = FRAMEBUFFER_WIDTH;
            return NO_ERROR;
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
            *value = FRAMEBUFFER_HEIGHT;
            return NO_ERROR;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            *value = 0;
            return NO_ERROR;
    }

    *value = 0;

    return BAD_VALUE;
}

int _window_perform(struct ANativeWindow* window, int operation, ... )
{
    printf("window_perform: operation = %i\n", operation);

    return 0;
}

int _window_cancelBuffer(struct ANativeWindow* window, void* buffer)
{
    printf("window_cancelBuffer\n");

    return 0;
}

struct dummy_native_window *dummy_native_window_create(void)
{
    struct dummy_native_window *window;
    int i;

    window = (struct dummy_native_window*) malloc(sizeof(struct dummy_native_window));

    window->native.common.magic = ANDROID_NATIVE_WINDOW_MAGIC;
    window->native.common.version = sizeof(ANativeWindow);
    memset(window->native.common.reserved, 0, sizeof(window->native.common.reserved));
    window->native.common.decRef = _window_common_decRef;
    window->native.common.incRef = _window_common_incRef;

    window->native.flags = 0;
    window->native.minSwapInterval = 0;
    window->native.maxSwapInterval = 0;
    window->native.xdpi = 0;
    window->native.ydpi = 0;

    window->native.setSwapInterval = _window_setSwapInterval;
    window->native.lockBuffer = _window_lockBuffer;
    window->native.dequeueBuffer = _window_dequeueBuffer;
    window->native.queueBuffer = _window_queueBuffer;
    window->native.query = _window_query;
    window->native.perform = _window_perform;
    window->native.cancelBuffer = _window_cancelBuffer;

    window->num_buffers = NUM_FRAME_BUFFERS;
    window->num_free_buffers = NUM_FRAME_BUFFERS;
    window->buffer_head = window->num_buffers - 1;

    /* initialize our buffers with correct settings */
    for (i = 0; i < window->num_buffers; i++) {
        window->buffers[i].width = FRAMEBUFFER_WIDTH;
        window->buffers[i].height = FRAMEBUFFER_HEIGHT;
        window->buffers[i].format = FRAMEBUFFER_FORMAT;
        window->buffers[i].usage = GRALLOC_USAGE_HW_FB;
    }

    return window;
}

void dummy_native_window_free(struct dummy_native_window *window)
{
    if (window == 0)
        return;

    free(window);
}
