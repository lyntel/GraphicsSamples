//----------------------------------------------------------------------------------
// File:        NvAppBase\linux/MainLinux.cpp
// SDK Version: v3.00 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
#include <algorithm>
#include <ctype.h>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include <time.h>

#include "glfw/NvGLFWContext.h"

#include "NvAppBase/NvAppBase.h"
#include "NvAppBase/gl/NvSampleAppGL.h"
#include "NV/NvStopWatch.h"
#include "NvGamepad/linux/NvGamepadLinux.h"
#include "NvAssetLoader/NvAssetLoader.h"

class NvLinuxStopWatch: public NvStopWatch
{
public:
    //! Constructor, default
    NvLinuxStopWatch() :
        start_time(), diff_time( 0.0)
    { };

    // Destructor
    ~NvLinuxStopWatch()
    { };

public:
    //! Start time measurement
    void start() {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        m_running = true;
    }

    //! Stop time measurement
    void stop() {
        diff_time = getDiffTime();
        m_running = false;
    }

    //! Reset time counters to zero
    void reset()
    {
        diff_time = 0;
        if( m_running )
            start();
    }

    const float getTime() const {
        if(m_running) {
            return getDiffTime();
        } else {
            // time difference in milli-seconds
            return  diff_time;
        }
    }

private:

    // helper functions
      
    //! Get difference between start time and current time
    float getDiffTime() const {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return  (float) (( now.tv_sec - start_time.tv_sec)
                    + (0.000000001 * (now.tv_nsec - start_time.tv_nsec)) );
    }

    // member variables

    //! Start of measurement
    struct timespec  start_time;

    //! Time difference between the last start and stop
    float  diff_time;
};


static NvAppBase *sApp = NULL;

// this needs to be global so inputcallbacksglfw can access...
NvInputCallbacks* sCallbacks = NULL;
extern void setInputCallbacksGLFW(GLFWwindow *window);

static bool sWindowIsFocused = true;
static bool sHasResized = true;
static int32_t sForcedRenderCount = 0;
static bool sRenderWithoutFocus = false;

class NvLinuxPlatformContext : public NvPlatformContext {
public:
    NvLinuxPlatformContext() : 
        mWindow(NULL), 
        mGamepad(new NvGamepadLinux),
        mRenderOnDemand(false),
        mRenderRequested(false) { }
    ~NvLinuxPlatformContext() { delete mGamepad; }

    void setWindow(GLFWwindow* window) {
        mWindow = window;
    }

    virtual bool isAppRunning();
    virtual void requestExit() { glfwSetWindowShouldClose(mWindow, 1); }
    virtual bool pollEvents(NvInputCallbacks* callbacks);
    virtual bool isContextLost() { return false; }
    virtual bool isContextBound() { return glfwGetCurrentContext() != NULL; }
    virtual bool shouldRender();
    virtual bool hasWindowResized();
    virtual NvGamepad* getGamepad() { return mGamepad; }
    virtual void setAppTitle(const char* title) { if (mWindow) glfwSetWindowTitle(mWindow, title); }
    virtual const std::vector<std::string>& getCommandLine() { return m_commandLine; }

    virtual NvRedrawMode::Enum getRedrawMode() { return mRenderOnDemand ? NvRedrawMode::ON_DEMAND : NvRedrawMode::UNBOUNDED; }
    virtual void setRedrawMode(NvRedrawMode::Enum mode) {
        mRenderOnDemand = (mode == NvRedrawMode::ON_DEMAND);
        if (mRenderOnDemand)
            requestRedraw(); 
    }
    virtual void requestRedraw() { mRenderRequested = true; }

    std::vector<std::string> m_commandLine;
protected:
    GLFWwindow* mWindow;
    NvGamepadLinux* mGamepad;
    bool mRenderOnDemand;
    bool mRenderRequested;
};

bool NvLinuxPlatformContext::isAppRunning() {
    return !glfwWindowShouldClose(mWindow);
}

bool NvLinuxPlatformContext::pollEvents(NvInputCallbacks* callbacks) {
    sCallbacks = callbacks;
    glfwPollEvents();

    if (mGamepad) {
        uint32_t changed = mGamepad->pollGamepads();
        if (callbacks && changed)
            callbacks->gamepadChanged(changed);
    }

    sCallbacks = NULL;
    
    return true;
}

bool NvLinuxPlatformContext::shouldRender() {
    if (sWindowIsFocused || (sForcedRenderCount > 0) || sRenderWithoutFocus) {
        if (sForcedRenderCount > 0)
            sForcedRenderCount--;

        if (!mRenderOnDemand || mRenderRequested) {
            mRenderRequested = false;
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool NvLinuxPlatformContext::hasWindowResized() {
    if (sHasResized) {
        sHasResized = false;
        return true;
    }
    return false;
}

// window size callback
static void reshape( GLFWwindow* window, int32_t width, int32_t height )
{
    //GLfloat aspect = (GLfloat) height / (GLfloat) width;
    //glViewport( 0, 0, (GLint) width, (GLint) height );
    sHasResized = true;
    sForcedRenderCount += 2;
}

static void focus(GLFWwindow*,int32_t focused)
{
    sWindowIsFocused = focused != 0;
    sApp->focusChanged(sWindowIsFocused);
    sForcedRenderCount += 2;
}


// program initialization
static void initGL(int32_t argc, char *argv[])
{
    //glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
}

void glfwError(int,const char* err) {
    fprintf( stderr, "GLFW error = %s\n", err);
}

extern void NvInitSharedFoundation(void);
extern void NvReleaseSharedFoundation(void);

// program entry
int32_t main(int32_t argc, char *argv[])
{
    GLFWwindow* window;
    int32_t width, height;

    NvInitSharedFoundation();

    NvAssetLoaderInit(NULL);

    sWindowIsFocused = true;
    sForcedRenderCount = 0;

    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }

    glfwSetErrorCallback(glfwError);

    NvLinuxPlatformContext* platform = new NvLinuxPlatformContext;

    // add command line arguments
    for (int i = 1; i < argc; i++) {
        platform->m_commandLine.push_back(argv[i]);
    }

    for (std::vector<std::string>::const_iterator it = platform->m_commandLine.begin(); 
        it != platform->m_commandLine.end(); ++it)
    {
        std::string key(*it);
        std::transform(key.begin(), key.end(), key.begin(), tolower);
        if (0 == key.compare("-forcerender")) {
            fprintf( stderr, "Force rendering when not focused (enabled)\n" );
            sRenderWithoutFocus = true;
            break;
        }
    }

    sApp = NvAppFactory();

    NvGLConfiguration config(NvGLAPIVersionGL4(), 8, 8, 8, 8, 16, 0);
    // HACK to allow testing until we rework config
    ((NvSampleAppGL*)sApp)->configurationCallback(config);

    // Does not seem to work...
/*
    if (config.api == GLAppContext::Configuration::API_ES)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, config.majVer);
    //glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    */

    NvGLFWContext* context = new NvGLFWContext(config, NvGLPlatformCategory::PLAT_DESKTOP, 
        NvGLPlatformOS::OS_LINUX);

    window = glfwCreateWindow( 1280, 720, "Linux SDK Application", NULL, NULL );
    if (!window)
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    platform->setWindow(window);

    context->setWindow(window);
    sApp->setGLContext(context);

    // Set callback functions
    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetWindowFocusCallback(window, focus);
    setInputCallbacksGLFW(window);

    context->bindContext();
    glfwSwapInterval( 1 );

    glfwGetFramebufferSize(window, &width, &height);

    int32_t major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    int32_t minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    config.apiVer = NvGLAPIVersion(NvGLAPI::GL, major, minor);

    glGetIntegerv(GL_RED_BITS, (GLint*)&config.redBits);
    glGetIntegerv(GL_GREEN_BITS, (GLint*)&config.greenBits);
    glGetIntegerv(GL_BLUE_BITS, (GLint*)&config.blueBits);
    glGetIntegerv(GL_ALPHA_BITS, (GLint*)&config.alphaBits);
    glGetIntegerv(GL_DEPTH_BITS, (GLint*)&config.depthBits);
    glGetIntegerv(GL_STENCIL_BITS, (GLint*)&config.stencilBits);

    context->setConfiguration(config);

#if 1
    // get extensions (need for ES2.0)
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
      /* Problem: glewInit failed, something is seriously wrong. */
      fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      exit(-1);
    }
    fprintf(stdout, "Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

    // Parse command-line options
    initGL(argc, argv);

    reshape(window, width, height);

    sApp->mainLoop();

    // Shut down the app before shutting down GL
    delete sApp;

    // Terminate GLFW
    glfwTerminate();

    NvReleaseSharedFoundation();

    NvAssetLoaderShutdown();

    // Exit program
    exit( EXIT_SUCCESS );
}

// Timer and timing functions
NvStopWatch* NvAppBase::createStopWatch() {
    return new NvLinuxStopWatch;
}

bool NvAppBase::showDialog(const char* title, const char *text, bool exitApp) {
    fprintf(stdout, "%s: %s\n", title, text);
    if (exitApp)
        exit(-1);
    return true;
}
bool NvAppBase::writeScreenShot(int32_t, int32_t, const uint8_t*, const std::string&) {
    return false;
}

bool NvAppBase::writeLogFile(const std::string&, bool, const char*, ...) {
    return false;
}

void NvAppBase::forceLinkHack() {
}

void NVPlatformLog(const char* fmt, ...) {
    fprintf(stderr, __VA_ARGS__); 
    fprintf(stderr, "\n");
}
