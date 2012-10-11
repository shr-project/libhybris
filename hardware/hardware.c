#include <dlfcn.h>
#include <stddef.h>

static void *_libhardware = NULL;

static int (*_hw_get_module)(const char *id, const struct hw_module_t **module) = NULL;

#define HARDWARE_DLSYM(fptr, sym) do { if (_libhardware == NULL) { _init_lib_hardware(); }; if (*(fptr) == NULL) { *(fptr) = (void *) android_dlsym(_libhardware, sym); } } while (0) 

static void _init_lib_hardware()
{
    _libhardware = (void *) android_dlopen("/system/lib/libhardware.so", RTLD_LAZY);
}

int hw_get_module(const char *id, const struct hw_module_t **module)
{
    HARDWARE_DLSYM(&_hw_get_module, "hw_get_module");
    return (*hw_get_module)(id, module);
}

