//----------------------------------------------------------------------------------
// File:        NvAppBase\android/NvEGLAppContext.h
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
#ifndef NV_EGL_APP_CONTEXT_H
#define NV_EGL_APP_CONTEXT_H

//BEGIN_INCLUDE(all)
#include <NvSimpleTypes.h>


#include <jni.h>
#include <errno.h>
#include "NvAppBase/NvAppBase.h"
#include "NvAppBase/gl/NvAppContextGL.h"
#include "../../NvEGLUtil/NvEGLUtil.h"

#define EGL_STATUS_LOG(str) \
    LOGD("Success: %s (%s:%d)", str, __FUNCTION__, __LINE__)

#define EGL_ERROR_LOG(str) \
        LOGE("Failure: %s, error = 0x%08x (%s:%d)", str, eglGetError(), __FUNCTION__, __LINE__)

class NvEGLAppContext: public NvAppContextGL {
public:
	NvEGLAppContext(NvEGLWinUtil* win) :
        NvAppContextGL(NvPlatformInfo(
            NvPlatformCategory::PLAT_MOBILE, 
            NvPlatformOS::OS_ANDROID))
        , mWin(win) {

		win->getConfiguration(mConfig);
    }

    bool bindContext() {
        return mWin->bind();
    }

    bool unbindContext() {
        return mWin->unbind();
    }

    bool prepareThread() {
        return mWin->prepareThread();
    }

    bool swap() {
        return mWin->swap();
    }

    bool setSwapInterval(int32_t) {
        return false;
    }

    int32_t width() {
		return mUseFBOPair ? m_fboWidth : mWin->getWidth();
    }

    int32_t height() {
		return mUseFBOPair ? m_fboHeight : mWin->getHeight();
    }

	bool isExtensionSupported(const char* ext);

	GLproc getGLProcAddress(const char* procname) {
		return mWin->getProcAddress(procname);
	}

    virtual bool requestResetContext() {
        return mWin->requestResetContext();
    }

    virtual void* getCurrentPlatformContext() { 
        return (void*)mWin->getContext(); 
    }

    virtual void* getCurrentPlatformDisplay() { 
        return (void*)mWin->getDisplay(); 
    }


protected:
	NvEGLWinUtil* mWin;
};

inline bool NvEGLAppContext::isExtensionSupported(const char* ext)
{
	// Extension names should not have spaces.
	const GLubyte *where = (GLubyte *)strchr(ext, ' ');
	if (where || *ext == '\0') {
		return false;
	}

	const GLubyte *extensions = glGetString(GL_EXTENSIONS);
	if (!extensions) {
		// Is an OpenGL context not bound??
		return false;
	}
	// It takes a bit of care to be fool-proof about parsing the
	// OpenGL extensions string.  Don't be fooled by sub-strings,
	// etc.
	const GLubyte *start = extensions;
	for (;;) {
		where = (const GLubyte *)strstr((const char *)start, ext);
		if (!where) {
			break;
		}
		const GLubyte *terminator = where + strlen(ext);
		if (where == start || *(where - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0') {
				return true;
			}
		}
		start = terminator;
	}
	return false;
}

#endif
