#ifndef DUMMY_NATIVE_WINDOW_H_
#define DUMMY_NATIVE_WINDOW_H_

#include <system/window.h>
#include <hardware/gralloc.h>

#define NUM_FRAME_BUFFERS  2

struct dummy_native_window {
    ANativeWindow native;

    ANativeWindowBuffer_t* buffers[NUM_FRAME_BUFFERS];
    uint8_t num_buffers;
    uint8_t num_free_buffers;
    uint8_t buffer_head;
    uint8_t current_buffer_index;

    struct alloc_device_t *gr_dev;
};

#endif
