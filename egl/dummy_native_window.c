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

int _window_dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer_t** buffer)
{
    struct dummy_native_window *self;
    int index;

    self = container_of(window, struct dummy_native_window, native);

    index = self->buffer_head;
    if (self->buffer_head >= self->num_buffers)
        self->buffer_head = 0;

    /* FIXME maybe we should wait here until a bufer is free instead of failing here */
    if (!self->num_free_buffers) {
        *buffer = NULL;
        return -1;
    }

    self->num_free_buffers--;
    self->current_buffer_index = index;

    *buffer = self->buffers[index];

    printf("leaving _window_dequeueBuffer\n");

    return 0;
}

int _window_lockBuffer(struct ANativeWindow* window, void* buffer)
{
    printf("dummy_native_window_lockBuffer\n");
    return 0;
}

int _window_queueBuffer(struct ANativeWindow* window, void* buffer)
{
    printf("_window_queueBuffer\n");
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
    hw_module_t const* module;
    int i, err;

    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module) < 0) {
        printf("ERROR: failed to initialize gralloc module\n");
        return NULL;
    }

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
    window->current_buffer_index = window->buffer_head;

    printf("before gralloc init\n");
    /* initial gralloc for buffer allocation */
    err = gralloc_open(module, &window->gr_dev);
    if (err != 0) {
        free(window);
        printf("ERROR: couldn't open gralloc HAL (%s)\n", strerror(-err));
        return NULL;
    }
    printf("after gralloc init\n");

    /* initialize our buffers with correct settings */
    for (i = 0; i < window->num_buffers; i++) {
        window->buffers[i] = (ANativeWindowBuffer_t*) malloc(sizeof(ANativeWindowBuffer_t));
        window->buffers[i]->width = FRAMEBUFFER_WIDTH;
        window->buffers[i]->height = FRAMEBUFFER_HEIGHT;
        window->buffers[i]->format = FRAMEBUFFER_FORMAT;
        window->buffers[i]->usage = GRALLOC_USAGE_HW_TEXTURE;

        err = window->gr_dev->alloc(window->gr_dev,
                                    FRAMEBUFFER_WIDTH,
                                    FRAMEBUFFER_HEIGHT,
                                    FRAMEBUFFER_FORMAT,
                                    GRALLOC_USAGE_HW_TEXTURE,
                                    &window->buffers[i]->handle,
                                    &window->buffers[i]->stride);
        if (err != 0) {
            printf("ERROR: buffer %d allocation failed w=%d, h=%d, err=%s\n",
                   i, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, strerror(-err));
            window->num_buffers = i;
            window->num_free_buffers = i;
            window->buffer_head = window->num_buffers - 1;
            break;
        }

        printf("INFO: allocated buffer with id %i\n", i);
    }

    return window;
}

void dummy_native_window_free(struct dummy_native_window *window)
{
    int i;

    if (window == 0)
        return;

    if (window->gr_dev != 0) {
        for (i = 0; window->num_buffers; i++) {
            if (window->buffers[i] != NULL) {
                window->gr_dev->free(window->gr_dev, window->buffers[i]->handle);
                free(window->buffers[i]);
            }
        }

        gralloc_close(window->gr_dev);
    }

    free(window);
}
