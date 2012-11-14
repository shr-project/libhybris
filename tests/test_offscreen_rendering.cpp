#define MESA_EGL_NO_X11_HEADERS
//#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include "fbdev_window.h"
#include "offscreen_window.h"

PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR=0;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR=0;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES=0;

static void checkGlError(const char* op) {
    bool doabort=false;
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        fprintf(stderr, "after %s() glError (0x%x)\n", op, error);
        doabort=true;
    }
    if(doabort) abort();

}

static const char gVertexShader[] = 
    "attribute vec4 vPosition;\n"
    "varying vec2 yuvTexCoords;\n"
    "void main() {\n"
    "  yuvTexCoords = vPosition.xy + vec2(0.5, 0.5);\n"
    "  gl_Position = vPosition;\n"
    "}\n";

static const char gFragmentShader[] = 
    "#extension GL_OES_EGL_image_external : require\n"
    "precision mediump float;\n"
    "uniform samplerExternalOES yuvTexSampler;\n"
    "varying vec2 yuvTexCoords;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(yuvTexSampler, yuvTexCoords);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    fprintf(stderr, "Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
            } else {
                fprintf(stderr, "Guessing at GL_INFO_LOG_LENGTH size\n");
                char* buf = (char*) malloc(0x1000);
                if (buf) {
                    glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    fprintf(stderr, "Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    fprintf(stderr, "Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLint gvPositionHandle;
GLint gYuvTexSamplerHandle;
const GLfloat gTriangleVertices[] = {
    -0.5f, 0.5f,
    -0.5f, -0.5f,
    0.5f, -0.5f,
    0.5f, 0.5f,
};


class EGLClient {
public:
    EGLClient(int pipe_read, int pipe_write)
        : pipe_read(pipe_read)
        , pipe_write(pipe_write)
        , frame(0) {
        EGLConfig ecfg;
        EGLint num_config;
        EGLint attr[] = {       // some attributes to set up our egl-interface
            EGL_BUFFER_SIZE, 32,
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,
            EGL_NONE
        };
        EGLint ctxattr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            printf("CLIENT: ERROR: Could not get default display\n");
            return;
        }

        printf("CLIENT: INFO: Successfully retrieved default display!\n");

        eglInitialize(display, 0, 0);
        eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);

        printf("CLIENT: INFO: Initialized display with default configuration\n");

        window = new OffscreenNativeWindow(pipe_read, pipe_write, 720, 1280);
        printf("CLIENT: INFO: Created native window %p\n", window);
        printf("CLIENT: creating window surface...\n");
        surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, *window, NULL);
        assert(surface != EGL_NO_SURFACE);
        printf("CLIENT: INFO: Created our main window surface %p\n", surface);
        context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
        assert(surface != EGL_NO_CONTEXT);
        printf("CLIENT: INFO: Created context for display\n");
    };
    void render() {
            assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);
            glViewport ( 0 , 0 , 1280, 720);
            printf("CLIENT: client frame %i\n", frame++);
            glClearColor ( 1.00 , (frame & 8) * 1.0f , ((float)(frame % 255))/255.0f, 1.);    // background color
            glClear(GL_COLOR_BUFFER_BIT);
            eglSwapBuffers(display, surface);
            printf("CLIENT: client swapped\n");
    }
    int pipe_write;
    int pipe_read;
    int frame;
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    OffscreenNativeWindow* window;
};

class EGLCompositor {
public:
    EGLCompositor(int pipe_read, int pipe_write)
        : pipe_read(pipe_read)
        , pipe_write(pipe_write)
        , frame(0) {
        EGLConfig ecfg;
        EGLint num_config;
        EGLint attr[] = {       // some attributes to set up our egl-interface
            EGL_BUFFER_SIZE, 32,
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,
            EGL_NONE
        };
        EGLint ctxattr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            printf("COMPOSITOR: ERROR: Could not get default display\n");
            return;
        }

        printf("COMPOSITOR: INFO: Successfully retrieved default display!\n");

        eglInitialize(display, 0, 0);
        eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);

        printf("COMPOSITOR: INFO: Initialized display with default configuration\n");

        window = new FbDevNativeWindow();
        printf("COMPOSITOR: INFO: Created native window %p\n", window);
        printf("COMPOSITOR: creating window surface...\n");
        surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, *window, NULL);
        assert(surface != EGL_NO_SURFACE);
        printf("COMPOSITOR: INFO: Created our main window surface %p\n", surface);
        context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
        assert(surface != EGL_NO_CONTEXT);
        printf("COMPOSITOR: INFO: Created context for display\n");
        assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);
        printf("COMPOSITOR: INFO: Made context and surface current for display\n");
        glGenTextures(1, &texture);
        
        gProgram = createProgram(gVertexShader, gFragmentShader);
        if (!gProgram) {
            printf("COMPOSITOR: no program\n");
            abort();
        }
        gvPositionHandle = glGetAttribLocation(gProgram, "COMPOSITOR: vPosition");
        checkGlError("COMPOSITOR: glGetAttribLocation");
        fprintf(stderr, "COMPOSITOR: glGetAttribLocation(\"vPosition\") = %d\n",
                gvPositionHandle);
        gYuvTexSamplerHandle = glGetUniformLocation(gProgram, "COMPOSITOR: yuvTexSampler");
        checkGlError("COMPOSITOR: glGetUniformLocation");
        fprintf(stderr, "COMPOSITOR: glGetUniformLocation(\"yuvTexSampler\") = %d\n",
                gYuvTexSamplerHandle);

        glViewport(0, 0, 1280, 720);
        checkGlError("COMPOSITOR: glViewport");

    };
    void render() {
            int err;
            assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);
            char temp[64];
            int count = read(pipe_read, temp, 64);
            OffscreenNativeWindowBuffer buffer(temp);
            write(pipe_write, &count, sizeof(count));
            
            EGLint attrs[] = {
                EGL_IMAGE_PRESERVED_KHR,    EGL_TRUE,
                EGL_NONE,
            };
            buffer.lock(GRALLOC_USAGE_HW_TEXTURE);
            EGLImageKHR image = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (ANativeWindowBuffer*)&buffer, attrs); // is this cast correct? segfaults!
            if (image == EGL_NO_IMAGE_KHR) {
                EGLint error = eglGetError();
                printf("\n\nCOMPOSITOR: error creating EGLImage: %#x\n\n", error);
                abort();
            }
            printf("COMPOSITOR: got egl image %p\n", image);

            glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
            if(err = glGetError()) printf("COMPOSITOR: %i gl error %x\n", __LINE__, err);
            glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)image);
            if(err = glGetError()) printf("COMPOSITOR: %i gl error %x\n", __LINE__, err);



            glViewport ( 0 , 0 , 1280, 720);
            printf("COMPOSITOR: compositor frame %i\n", frame++);
            float c = (frame % 64) / 64.0f;
            glClearColor ( c , c , c, 1.);    // background color

            checkGlError("COMPOSITOR: glClearColor");
            glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            checkGlError("COMPOSITOR: glClear");
            glUseProgram(gProgram);
            checkGlError("COMPOSITOR: glUseProgram");
            glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
            checkGlError("COMPOSITOR: glVertexAttribPointer");
            glEnableVertexAttribArray(gvPositionHandle);
            checkGlError("COMPOSITOR: glEnableVertexAttribArray");
            glUniform1i(gYuvTexSamplerHandle, 0);
            checkGlError("COMPOSITOR: glUniform1i");
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
            checkGlError("COMPOSITOR: glBindTexture");
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            checkGlError("COMPOSITOR: glDrawArrays");

            eglSwapBuffers(display, surface);
            eglDestroyImageKHR(display, image);
            buffer.unlock();
            printf("COMPOSITOR: compositor swapped\n");
    }
    int frame;
    int pipe_read;
    int pipe_write;
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    FbDevNativeWindow* window;
    GLuint texture;
    EGLImageKHR image;
};

#define READ_END 0
#define WRITE_END 1

int main(int argc, char **argv)
{
    int pipe1[2];
    pipe(pipe1);
    int pipe2[2];
    pipe(pipe2);
    int pid = fork();
    if(pid) {
        printf("COMPOSITOR STARTUP\n");
        EGLCompositor compositor(pipe1[READ_END], pipe2[WRITE_END]);
        eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
        eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
        glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
        while(1) {
            printf("COMPOSITOR render loop >>> ##########################################################33333#########################################\n");
            compositor.render();
        }
    } else {
        printf("CLIENT STARTUP\n");
        EGLClient client(pipe2[READ_END], pipe1[WRITE_END]);
        while(1) {
            printf("CLIENT RENDER LOOP >>> +=====++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=====+++++++++++++++++++++++++++++++++++\n");
            client.render();
        }
    }
}
