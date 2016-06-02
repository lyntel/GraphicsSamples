//----------------------------------------------------------------------------------
// File:        es2-aurora\Bloom/Bloom.h
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
#include "NvAppBase/gl/NvSampleAppGL.h"

#include "KHR/khrplatform.h"
#include "NvGamepad/NvGamepad.h"
#include "NV/NvMath.h"

class NvStopWatch;
class NvGLSLProgram;
class NvFramerateCounter;

class Bloom : public NvSampleAppGL
{
public:
    Bloom();
    ~Bloom();
    
    void initRendering(void);
    void initUI(void);
    void draw(void);
    void reshape(int32_t width, int32_t height);

    void configurationCallback(NvGLConfiguration& config);

private:
    typedef struct
    {
        GLuint fbo;                // name of an fbo.
        GLuint color_texture;      // name of a texture attached.
        GLuint depth_renderbuffer; // name of depth renderbuffer texture attached
        GLuint depth_texture;      // name of depth texture attached.
        GLuint width;              // Width of the buffer
        GLuint height;             // Height of the buffer
    } FBOType;

    typedef struct
    {
        nv::matrix4f mCameraMVP;
        nv::matrix4f mShadowMVP;
        nv::vec4f mWorldLightDir;
    } DynamicsType;

    void renderShadowGate();
    void renderGate();
    void renderScene();
    bool createGateVBOs();
    bool createOSDVBOs();
    GLuint initFBO(GLuint width, GLuint height, GLuint internalformat, GLuint format, GLuint type, 
        GLuint mipmapped, GLuint depthformat, FBOType& fbo);
    GLuint initDepthTexFBO(GLuint width, GLuint height, GLuint internalformat, GLuint format, GLuint type, 
        GLuint mipmapped, FBOType& fbo);
    void updateDynamics();

    // gate geometry
    struct
    {
        GLuint vertex_vb;
        GLuint vertex_ib;
        int32_t numvertices;
        int32_t numindices;
    } mGateModel;

    GLuint mQuadVBO;

    // per-frame dynamic variables /////////////////////////////
    DynamicsType mDynamics;

    GLuint mGateDiffuseTex;
    GLuint mGateGlowTex;

    nv::vec3f mLightPosition;

    // service shaders
    NvGLSLProgram* mQuadProgram;

    // scene related shaders
    NvGLSLProgram* mGateProgram;
    NvGLSLProgram* mGateShadowProgram;
    NvGLSLProgram* mBlurProgram;
    NvGLSLProgram* mDownfilterProgram;
    NvGLSLProgram* mCombineProgram;

    // buffers ///////////////////////////////////////////////////////////////////
    struct
    {
        FBOType scene;
        FBOType halfres;
        FBOType blurred;
        FBOType shadowmap;
    } mFBOs;

    float mAnimationTime;

    bool mDrawBloom;
    bool mDrawGlow;
    bool mDrawShadow;
    float mBlurWidth;
    float mBloomIntensity;
    bool mAutoSpin;
};
