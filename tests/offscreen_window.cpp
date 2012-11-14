#include "offscreen_window.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/matroxfb.h> // for FBIO_WAITFORVSYNC
#include <sys/mman.h> //mmap, munmap
#include <errno.h>
#include <unistd.h>
void printUsage(int usage);

template<typename T> inline void PUSH(char*& to, T what) {
    *reinterpret_cast<T*>(to) = what;
    to+=sizeof(T);
}

template<typename T> inline void PULL(char*& from, T* what) {
    *what = *reinterpret_cast<T*>(from);
    from+=sizeof(T);
}

int OffscreenNativeWindowBuffer::serialize(char* to) {
    char* start = to;
    PUSH(to, handle);
    PUSH(to, width);
    PUSH(to, height);
    PUSH(to, stride);
    PUSH(to, format);
    PUSH(to, usage);

    printf("serialized: handle %i width %i Height %i stride %i format %i\n",
            handle, width, height, stride, format);
    return to - start;
}

int OffscreenNativeWindowBuffer::deserialize(char* from) {
    char* start = from;
    PULL(from, &handle);
    PULL(from, &width);
    PULL(from, &height);
    PULL(from, &stride);
    PULL(from, &format);
    PULL(from, &usage);

    printf("deserialized: handle %i width %i Height %i stride %i format %i\n",
            handle, width, height, stride, format);
    return from - start;
}

OffscreenNativeWindowBuffer::OffscreenNativeWindowBuffer(unsigned int width,
                            unsigned int height,
                            unsigned int format,
                            unsigned int usage)
                            : vaddr(0)
{
    // Base members
    ANativeWindowBuffer::width = width;
    ANativeWindowBuffer::height = height;
    ANativeWindowBuffer::format = format;
    ANativeWindowBuffer::usage = usage;

    int err;
    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t**)&m_gralloc);
    err = gralloc_open((hw_module_t*)m_gralloc, &m_alloc);
    TRACE("got alloc %p err:%s\n", m_alloc, strerror(-err));
    printUsage(usage);
    err = m_alloc->alloc(m_alloc,
            width, height, format, usage,
            &handle,
            &stride);
    TRACE("got alloc %p err:%s\n", m_alloc, strerror(-err));
    TRACE("allocated OffscreenBuffer handle %i width %i height %i stride %i\n", handle, width, height, stride);
};

OffscreenNativeWindowBuffer::OffscreenNativeWindowBuffer(char* buf) 
                :vaddr(0)
{
    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t**)&m_gralloc);
    int err = gralloc_open((hw_module_t*)m_gralloc, &m_alloc);
    TRACE("got alloc %p err:%s\n", m_alloc, strerror(-err));
    deserialize(buf);
    err = m_gralloc->registerBuffer(m_gralloc, handle);
    TRACE("got register %p err:%s\n", m_alloc, strerror(-err));
};

void OffscreenNativeWindowBuffer::lock(int lockUsage) {
    int err = m_gralloc->lock(m_gralloc,
            handle, 
            lockUsage| usage,
            0,0, width, height,
            &vaddr
            );
    TRACE("%i lock %s vaddr %p\n", handle, strerror(-err), vaddr);
}

void OffscreenNativeWindowBuffer::unlock() {
    int err = m_gralloc->unlock(m_gralloc, handle);
    TRACE("%i unlock %s\n", handle, strerror(-err));
}

OffscreenNativeWindow::OffscreenNativeWindow(int pipe_read, int pipe_write, unsigned int aWidth, unsigned int aHeight, unsigned int aFormat)
    : m_width(aWidth)
    , m_height(aHeight)
    , m_defaultWidth(aWidth)
    , m_defaultHeight(aHeight)
    , m_format(aFormat)
    , m_pipeRead(pipe_read)
    , m_pipeWrite(pipe_write)
{
    m_usage=GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_2D | GRALLOC_USAGE_SW_READ_RARELY;

    for(unsigned int i = 0; i < NUM_BUFFERS; i++) {
        m_buffers[i] = 0;
    }
    m_frontbuffer = 0;
    m_tailbuffer = 0;
}

OffscreenNativeWindow::~OffscreenNativeWindow() {
    TRACE("%s\n",__PRETTY_FUNCTION__);
}

OffscreenNativeWindowBuffer* OffscreenNativeWindow::getFrontBuffer() {
    return m_buffers[m_frontbuffer];
}

// overloads from BaseNativeWindow
int OffscreenNativeWindow::setSwapInterval(int interval) {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    //FIXME todo
    return 0;
}

int OffscreenNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer){
    TRACE("%s ===================================\n",__PRETTY_FUNCTION__);
    if(m_buffers[m_tailbuffer] == 0) {
        m_buffers[m_tailbuffer] = new OffscreenNativeWindowBuffer(width(), height(), m_format, m_usage);
    }
    *buffer = m_buffers[m_tailbuffer];
    TRACE("dequeueing buffer %i %p\n",m_tailbuffer, m_buffers[m_tailbuffer]);
    m_tailbuffer++;
    if(m_tailbuffer == NUM_BUFFERS)
        m_tailbuffer = 0;
    return NO_ERROR;
}

int OffscreenNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer){
    TRACE("%s ===================\n",__PRETTY_FUNCTION__);
    // FIXME PVR on GNEX does not like this lock... why? different versions of android drivers
    // maybe behave differently...

    /*
    OffscreenNativeWindowBuffer *buf = static_cast<OffscreenNativeWindowBuffer*>(buffer);
    int usage = buf->usage | GRALLOC_USAGE_SW_READ_RARELY;
    printf("lock usage: ");
    printUsage(usage);
    int err = m_gralloc->lock(m_gralloc,
            buf->handle, 
            usage,
            0,0, m_width, m_height,
            &buf->vaddr
            );
    TRACE("lock %s vaddr %p\n", strerror(-err), buf->vaddr);
    return err;
    */
    return NO_ERROR;
}

int OffscreenNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer){
    OffscreenNativeWindowBuffer* buf = static_cast<OffscreenNativeWindowBuffer*>(buffer);
    m_frontbuffer++;
    if(m_frontbuffer == NUM_BUFFERS)
        m_frontbuffer = 0;

    char tempbuf[64];
    int count = buf->serialize(tempbuf);
    write(m_pipeWrite, tempbuf, count);
    int temp;
    read(m_pipeRead, &temp, sizeof(temp)); //synchronization
    
    int res = 0;
    //FIXME actually do something / proper synchronization / egl fence?
    TRACE("%s %s =+++++=====================================\n",__PRETTY_FUNCTION__,strerror(-res));
    return NO_ERROR;
}

int OffscreenNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer){
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

unsigned int OffscreenNativeWindow::width() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_width);
    return m_width;
}

unsigned int OffscreenNativeWindow::height() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_height);
    return m_height;
}

unsigned int OffscreenNativeWindow::format() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_format);
    return m_format;
}

unsigned int OffscreenNativeWindow::defaultWidth() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_defaultWidth);
    return m_defaultWidth;
}

unsigned int OffscreenNativeWindow::defaultHeight() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_defaultHeight);
    return m_defaultHeight;
}

unsigned int OffscreenNativeWindow::queueLength() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 1;
}

unsigned int OffscreenNativeWindow::type() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return NATIVE_WINDOW_SURFACE_TEXTURE_CLIENT;
}

unsigned int OffscreenNativeWindow::transformHint() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

int OffscreenNativeWindow::setBuffersFormat(int format) {
    TRACE("%s format %i\n",__PRETTY_FUNCTION__, format);
    return NO_ERROR;
}

int OffscreenNativeWindow::setBuffersDimensions(int width, int height) {
    TRACE("%s size %ix%i\n",__PRETTY_FUNCTION__, width, height);
    return NO_ERROR;
}

int OffscreenNativeWindow::setUsage(int usage) {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    usage |= GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_RARELY;
    printUsage(usage);
    m_usage = usage;
    return NO_ERROR;
}

void printUsage(int usage) {
    if(usage & GRALLOC_USAGE_SW_READ_NEVER) {
        TRACE("GRALLOC_USAGE_SW_READ_NEVER | ");
    }
    /* buffer is rarely read in software */
    if(usage & GRALLOC_USAGE_SW_READ_RARELY) {
        TRACE("GRALLOC_USAGE_SW_READ_RARELY | ");
    }
    /* buffer is often read in software */
    if(usage & GRALLOC_USAGE_SW_READ_OFTEN) {
        TRACE("GRALLOC_USAGE_SW_READ_OFTEN | ");
    }
    /* mask for the software read values */
    if(usage & GRALLOC_USAGE_SW_READ_MASK) {
        TRACE("GRALLOC_USAGE_SW_READ_MASK | ");
    }
    
    /* buffer is never written in software */
    if(usage & GRALLOC_USAGE_SW_WRITE_NEVER) {
        TRACE("GRALLOC_USAGE_SW_WRITE_NEVER | ");
    }
    /* buffer is never written in software */
    if(usage & GRALLOC_USAGE_SW_WRITE_RARELY) {
        TRACE("GRALLOC_USAGE_SW_WRITE_RARELY | ");
    }
    /* buffer is never written in software */
    if(usage & GRALLOC_USAGE_SW_WRITE_OFTEN) {
        TRACE("GRALLOC_USAGE_SW_WRITE_OFTEN | ");
    }
    /* mask for the software write values */
    if(usage & GRALLOC_USAGE_SW_WRITE_MASK) {
        TRACE("GRALLOC_USAGE_SW_WRITE_MASK | ");
    }

    /* buffer will be used as an OpenGL ES texture */
    if(usage & GRALLOC_USAGE_HW_TEXTURE) {
        TRACE("GRALLOC_USAGE_HW_TEXTURE | ");
    }
    /* buffer will be used as an OpenGL ES render target */
    if(usage & GRALLOC_USAGE_HW_RENDER) {
        TRACE("GRALLOC_USAGE_HW_RENDER | ");
    }
    /* buffer will be used by the 2D hardware blitter */
    if(usage & GRALLOC_USAGE_HW_2D) {
        TRACE("GRALLOC_USAGE_HW_2D | ");
    }
    /* buffer will be used by the HWComposer HAL module */
    if(usage & GRALLOC_USAGE_HW_COMPOSER) {
        TRACE("GRALLOC_USAGE_HW_COMPOSER | ");
    }
    /* buffer will be used with the framebuffer device */
    if(usage & GRALLOC_USAGE_HW_FB) {
        TRACE("GRALLOC_USAGE_HW_FB | ");
    }
    /* buffer will be used with the HW video encoder */
    if(usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
        TRACE("GRALLOC_USAGE_HW_VIDEO_ENCODER | ");
    }
    /* mask for the software usage bit-mask */
    if(usage & GRALLOC_USAGE_HW_MASK) {
        TRACE("GRALLOC_USAGE_HW_MASK | ");
    }

    /* buffer should be displayed full-screen on an external display when
     * possible
     */
    if(usage & GRALLOC_USAGE_EXTERNAL_DISP) {
        TRACE("GRALLOC_USAGE_EXTERNAL_DISP | ");
    }

    /* Must have a hardware-protected path to external display sink for
     * this buffer.  If a hardware-protected path is not available, then
     * either don't composite only this buffer (preferred) to the
     * external sink, or (less desirable) do not route the entire
     * composition to the external sink.
     */
    if(usage & GRALLOC_USAGE_PROTECTED) {
        TRACE("GRALLOC_USAGE_PROTECTED | ");
    }

    /* implementation-specific private usage flags */
    if(usage & GRALLOC_USAGE_PRIVATE_0) {
        TRACE("GRALLOC_USAGE_PRIVATE_0 | ");
    }
    if(usage & GRALLOC_USAGE_PRIVATE_1) {
        TRACE("GRALLOC_USAGE_PRIVATE_1 | ");
    }
    if(usage & GRALLOC_USAGE_PRIVATE_2) {
        TRACE("GRALLOC_USAGE_PRIVATE_2 |");
    }
    if(usage & GRALLOC_USAGE_PRIVATE_3) {
        TRACE("GRALLOC_USAGE_PRIVATE_3 |");
    }
    if(usage & GRALLOC_USAGE_PRIVATE_MASK) {
        TRACE("GRALLOC_USAGE_PRIVATE_MASK |");
    }
    TRACE("\n");
}
