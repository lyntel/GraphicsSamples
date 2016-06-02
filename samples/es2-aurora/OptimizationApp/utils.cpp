//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/utils.cpp
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
#include "utils.h"
#include <map>
#include <sstream>
#include <NV/NvLogs.h>
#include <NvAssetLoader/NvAssetLoader.h>
#include "AppExtensions.h"

NvModelGL* loadModelFromFile(const char *pFileName, float rescale )
{
    int32_t length;
    char *modelData = NvAssetLoaderRead(pFileName, length);

    NvModelGL* pModel = NvModelGL::CreateFromObj(
		(uint8_t *)modelData, rescale, true, false);

    NvAssetLoaderFree(modelData);

    return pModel;
}

 GLuint createFBO(GLuint a_depthColorTex, GLuint a_depthRenderBuffer, int32_t a_width, int32_t a_height)
 {
    GLuint prevFBO = 0;
    // Enum has MANY names based on extension/version
    // but they all map to 0x8CA6
    glGetIntegerv(0x8CA6, (GLint*)&prevFBO);

    GLuint m_fbo;

   glBindTexture(GL_TEXTURE_2D, a_depthColorTex);                       CHECK_GL_ERROR();
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   CHECK_GL_ERROR();
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   CHECK_GL_ERROR();
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_GL_ERROR();
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_GL_ERROR();
     glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, a_width, a_height, 0, GL_LUMINANCE, gFloatTypeEnum, NULL); CHECK_GL_ERROR();

   glBindRenderbuffer(GL_RENDERBUFFER, a_depthRenderBuffer);                          CHECK_GL_ERROR();
     glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, a_width, a_height);    CHECK_GL_ERROR();

   glGenFramebuffers(1, &m_fbo); CHECK_GL_ERROR();
   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); CHECK_GL_ERROR();
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, a_depthColorTex, 0);      CHECK_GL_ERROR();
     glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, a_depthRenderBuffer); CHECK_GL_ERROR();

   GLint st = glCheckFramebufferStatus(GL_FRAMEBUFFER); CHECK_GL_ERROR();
    
     if (st != GL_FRAMEBUFFER_COMPLETE)
     {
         LOGE("incomplete framebuffer status %d (%d, %s)", st , __LINE__, __FILE__);
         return 0;
     }

   glBindTexture(GL_TEXTURE_2D, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
   glBindFramebuffer(GL_FRAMEBUFFER, prevFBO); 

   return m_fbo;
}


nv::matrix4f ortho3D(float const & left, float const & right, 
    float const & bottom, float const & top, 
    float const & zNear, float const & zFar)
{
    nv::matrix4f Result;
    Result.make_identity();

    Result.element(0,0) = 2.0f / (right - left);
    Result.element(1,1) = 2.0f / (top - bottom);
    Result.element(2,2) = - 2.0f / (zFar - zNear);
    Result.element(3,0) = - (right + left) / (right - left);
    Result.element(3,1) = - (top + bottom) / (top - bottom);
    Result.element(3,2) = - (zFar + zNear) / (zFar - zNear);

    return Result;
}


nv::vec3f operator*(const nv::matrix4f& m, const nv::vec3f& v)
{
  nv::vec4f res = m*nv::vec4f(v.x, v.y, v.z, 1);
  return nv::vec3f(res.x, res.y, res.z);
}
