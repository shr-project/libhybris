bin_PROGRAMS = \
	test_egl \
	test_glesv2 \
	test_offscreen_rendering

test_egl_SOURCES = test_egl.c
test_egl_CFLAGS = \
	-I$(top_srcdir)/include
if !WANT_MESA
test_egl_CFLAGS += -DMESA_EGL_NO_X11_HEADERS
endif
test_egl_LDADD = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/egl/libEGL.la

test_glesv2_SOURCES = test_glesv2.c
test_glesv2_CFLAGS = \
	-I$(top_srcdir)/include
if !WANT_MESA
test_glesv2_CFLAGS += -DMESA_EGL_NO_X11_HEADERS
endif
test_glesv2_LDADD = \
	-lm \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/egl/libEGL.la \
	$(top_builddir)/glesv2/libGLESv2.la

test_offscreen_rendering_SOURCES = \
	test_offscreen_rendering.cpp \
	fbdev_window.cpp \
	nativewindowbase.cpp \
	offscreen_window.cpp \
	fdpass.c \
	native_handle.c
test_offscreen_rendering_CXXFLAGS = \
	-I$(top_srcdir)/include
if !WANT_MESA
test_offscreen_rendering_CXXFLAGS += -DMESA_EGL_NO_X11_HEADERS
endif
test_offscreen_rendering_CFLAGS = \
	-I$(top_srcdir)/include
if !WANT_MESA
test_offscreen_rendering_CFLAGS += -DMESA_EGL_NO_X11_HEADERS
endif
test_offscreen_rendering_LDADD = \
	-lm \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/hardware/libhardware.la \
	$(top_builddir)/egl/libEGL.la \
	$(top_builddir)/glesv2/libGLESv2.la
