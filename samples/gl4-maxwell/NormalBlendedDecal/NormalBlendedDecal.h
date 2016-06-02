//----------------------------------------------------------------------------------
// File:        gl4-maxwell\NormalBlendedDecal/NormalBlendedDecal.h
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
#ifndef NormalBlendedDecal_H
#define NormalBlendedDecal_H

#include "NvAppBase/gl/NvSampleAppGL.h"
#include "NV/NvMath.h"

class NvModelGL;
class NvStopWatch;
class NvFramerateCounter;
class NvGLSLProgram;

#define NUM_DECAL_BLENDS 2

class NormalBlendedDecal : public NvSampleAppGL
{
public:
    NormalBlendedDecal();
    ~NormalBlendedDecal();
    
    virtual NvUIEventResponse handleReaction(const NvUIReaction &react);
    virtual void initUI(void);
    virtual void initRendering(void);
    virtual void draw(void);
    virtual void reshape(int32_t width, int32_t height);

    virtual void configurationCallback(NvGLConfiguration& config);

    enum eLock {
        LOCK_NONE,
        LOCK_MEMORY_BARRIER,
        LOCK_PIXEL_SHADER_INTERLOCK,
    };

    enum eModel {
        BUNNY_MODEL,
        BOX_MODEL,
        MODELS_COUNT
    };

    enum eView {
        VIEW_DEFAULT,
        VIEW_NORMALS,
        VIEW_DECAL_BOXES
    };

    enum GBUFFER_TEXTURES {
        COLOR_TEXTURE,
        NORMAL_TEXTURE,
        WORLDPOS_TEXTURE,
        DEPTH_TEXTURE,
        NUM_GBUFFER_TEXTURES
    };

protected:
    bool buildShaders();
    void reloadShaders();
    void destroyShaders();
    void destroyBuffers();

    NvModelGL* loadModel(const char *model_filename);
    void draw2d( GLuint texture, NvGLSLProgram* program );

    // Which locking method to use
    eLock mLock;

    // View option
    eView mViewOption;

    // Number of decal objects
    unsigned int mNumDecals;
    nv::vec2f* mDecalPos;

    // Square root of the number of scene models
    unsigned int mSqrtNumModels;

    // Model ID to show
    uint32_t mModelID;

    // Loaded models
    NvModelGL *mModels[MODELS_COUNT];

    // Model for decal box
    NvModelGL *mCube;

    // Distance between models
    float mModelDistance;

    // Distance between decal boxes
    float mDecalDistance;

    // Decal size
    float mDecalSize;

    // Decal normal textures
    GLuint mDecalNormals[NUM_DECAL_BLENDS];

    // Blend weight between two normals
    float mBlendWeight;

    // FBO for GBuffer
    GLuint mGbufferFBO;

    // Textures for GBuffer textures
    GLuint mGbufferTextures[NUM_GBUFFER_TEXTURES];

    // FBO for Decal rendering into GBuffer
    GLuint mGbufferDecalFBO;

    NvGLSLProgram* mGbufferProgram;     // glprogram for gbuffer render pass
    NvGLSLProgram* mDecalProgram;       // glprogram for decal render pass
    NvGLSLProgram* mCombinerProgram;    // glprogram for pass combiner
    NvGLSLProgram* mBlitProgram;        // glprogram for blitting
};

#endif
