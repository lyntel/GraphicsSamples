//----------------------------------------------------------------------------------
// File:        es3aep-kepler\FeedbackParticlesApp/TextureUtils.cpp
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
#include "TextureUtils.h"
#include <NV/NvPlatformGL.h>
#include <NvGLUtils/NvGLSLProgram.h>
#include <NvGLUtils/NvShapesGL.h>
#include <NV/NvLogs.h>


//------------------------------------------------------------------------------
//
uint32_t TexturesUtils::FBM3DTexture(uint32_t extent)
{
    NvGLSLProgram* FBMProgram = NvGLSLProgram::createFromFiles("fbm.vert","fbm.frag");

    GLuint prevFBO = 0;
    // Enum has MANY names based on extension/version
    // but they all map to 0x8CA6
    glGetIntegerv(0x8CA6, (GLint*)&prevFBO);

    uint32_t out;
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_3D);
    glGenTextures(1,&out);
    glBindTexture(GL_TEXTURE_3D,out);
    glTexImage3D(GL_TEXTURE_3D,0,GL_RGBA16F,extent,extent,extent,0,GL_RGBA,GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glBindTexture(GL_TEXTURE_3D,0);

    uint32_t framebuffer;
    glGenFramebuffers(1,&framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);

    GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1,buffers);
    
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0,0,extent,extent);
    for(uint32_t n = 0; n < extent; n++)
    {
        const float layer = n/(float)extent;

        glFramebufferTexture3D( GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_3D,
                                out,0,n);

        FBMProgram->enable();
        FBMProgram->setUniform1f("u_Layer",layer);

        NvDrawQuadGL(0);
    }
    glPopAttrib();
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glDeleteFramebuffers(1,&framebuffer);

    delete FBMProgram;

    return out;
}


//------------------------------------------------------------------------------
//
uint32_t TexturesUtils::random1DTexture(uint32_t extent)
{
    float* pRaw = new float[extent*4];

    for (uint32_t n = 0;n < extent*4;n++)
        pRaw[n] = rand()/(float)RAND_MAX;

    uint32_t out;
    glGenTextures(1,&out);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,out);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,extent,1,0,GL_RGBA,GL_FLOAT,pRaw);

    delete [] pRaw;

    return out;
}


//------------------------------------------------------------------------------
//
uint32_t TexturesUtils::spotTexture(uint32_t width,uint32_t height)
{
    uint8_t* pRaw = new uint8_t[width*height];

    const float fMaxDist = (float)sqrt(width*height/2.f);
    for (uint32_t y = 0;y < height;y++)
    {
        for (uint32_t x = 0;x < width;x++)
        {
            float length = (float)sqrt((x - width/2.f)*(x - width/2.f) 
                                     + (y - height/2.f)*(y - height/2.f));

            float fSpot  = (float)powf((fMaxDist - length)/fMaxDist,8.f);
            fSpot        = fSpot > 1.f ? 1.f : fSpot;

            pRaw[x + y*width] = fSpot > 0 ? (uint8_t)(fSpot*255.f) : 0;
        }
    }
    
    uint32_t out;
    glGenTextures(1,&out);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,out);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(   GL_TEXTURE_2D,0,GL_ALPHA,width,height,0,
                    GL_ALPHA,GL_UNSIGNED_BYTE,pRaw);
    glGenerateMipmap(GL_TEXTURE_2D);

    delete [] pRaw;

    return out;
}

