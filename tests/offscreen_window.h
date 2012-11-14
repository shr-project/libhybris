#ifndef Offscreen_WINDOW_H
#define Offscreen_WINDOW_H
#include "nativewindowbase.h"
#include <linux/fb.h>
#include <hardware/gralloc.h>
#define NUM_BUFFERS 3

class OffscreenNativeWindowBuffer : public BaseNativeWindowBuffer
{
    friend class OffscreenNativeWindow;
public:
    OffscreenNativeWindowBuffer(char* buf);
    void lock(int usage);
    void unlock();
protected:
    OffscreenNativeWindowBuffer(unsigned int width,
                            unsigned int height,
                            unsigned int format,
                            unsigned int usage);
    virtual int serialize(char* to);
    virtual int deserialize(char* from);
    alloc_device_t* m_alloc;
    const gralloc_module_t* m_gralloc;
    void* vaddr;
};

class OffscreenNativeWindow : public BaseNativeWindow
{
public:
    OffscreenNativeWindow(int pipe_read, int pipe_write, unsigned int width, unsigned int height, unsigned int format = 5);
    ~OffscreenNativeWindow();
    OffscreenNativeWindowBuffer* getFrontBuffer();
protected:
    // overloads from BaseNativeWindow
    virtual int setSwapInterval(int interval);
    virtual int dequeueBuffer(BaseNativeWindowBuffer **buffer);
    virtual int lockBuffer(BaseNativeWindowBuffer* buffer);
    virtual int queueBuffer(BaseNativeWindowBuffer* buffer);
    virtual int cancelBuffer(BaseNativeWindowBuffer* buffer);
    virtual unsigned int type() const;
    virtual unsigned int width() const;
    virtual unsigned int height() const;
    virtual unsigned int format() const;
    virtual unsigned int defaultWidth() const;
    virtual unsigned int defaultHeight() const;
    virtual unsigned int queueLength() const;
    virtual unsigned int transformHint() const;
    // perform calls
    virtual int setUsage(int usage);
    virtual int setBuffersFormat(int format);
    virtual int setBuffersDimensions(int width, int height);
private:
    unsigned int m_frontbuffer;
    unsigned int m_tailbuffer;
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_format;
    unsigned int m_defaultWidth;
    unsigned int m_defaultHeight;
    unsigned int m_usage;
    OffscreenNativeWindowBuffer* m_buffers[NUM_BUFFERS];

    int m_pipeWrite;
    int m_pipeRead;
};

#endif
