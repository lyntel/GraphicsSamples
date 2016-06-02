//----------------------------------------------------------------------------------
// File:        gl4-kepler\DeferredShadingMSAA/DeferredShadingMSAA.h
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
#ifndef DeferredShadingMSAA_H
#define DeferredShadingMSAA_H

#define ENABLE_GPU_TIMERS 1

#include "NvAppBase/gl/NvSampleAppGL.h"
#include "NvGLUtils/NvTimers.h"
#include "NV/NvMath.h"
#include "NvGLUtils/NvModelExtGL.h"

class NvModelGL;
class NvStopWatch;
class NvFramerateCounter;
class NvGLSLProgram;


struct UIReactionIDs 
{
    enum Enum
    {
        UIApproach              = 0x10000001,
        UIBRDF                  = 0x10000002,
        UIAdaptiveShading       = 0x10000003,
        UIMarkComplexPixels     = 0x10000004,
        UISeparateComplexPass   = 0x10000005,
        UIPerSampleShading      = 0x10000006,
        UIExtMeshLoader         = 0x10000007
    };
};

class DeferredShadingMSAA : public NvSampleAppGL
{
public:
    DeferredShadingMSAA();
    ~DeferredShadingMSAA();
    
    void initUI(void);
    void initRendering(void);
    void draw(void);
    void reshape(int32_t width, int32_t height);

    void configurationCallback(NvGLConfiguration& config);
    bool getRequestedWindowSize(int32_t& width, int32_t& height);
    virtual NvUIEventResponse handleReaction(const NvUIReaction& react);

    enum 
    {
        SPONZA_MODEL,
        MODELS_COUNT
    };

    enum
    {
        TIMER_GEOMETRY,
        TIMER_COMPOSITING,
        TIMER_COUNT
    };

    enum ComplexDetectApproach
    {
        APPROACH_NOMSAA,
        APPROACH_COVERAGEMASK,
        APPROACH_DISCONTINUITY,
        APPROACH_COUNT
    };

    enum BRDF
    {
        BRDF_LAMBERT,
        BRDF_ORENNAYAR,
        BRDF_COUNT
    };

    enum RenderTargetMode
    {
        RTMODE_NONE,
        RTMODE_NOMSAA,
        RTMODE_PERPIXEL,
        RTMODE_PERSAMPLE,
        RTMODE_COUNT
    };

protected:
    bool UpdateRenderTargetMode();

    void InitRenderTargets_NoMSAA();
    void InitRenderTargets_PerPixelShading();
    void InitRenderTargets_PerSampleShading();

    void DestroyRenderTargets();

    // Draw methods
    void RenderToGBuffer();
    void RenderToGBuffer_NoMSAA();
    void ResolveMSAAGBuffer();
    void FillStencilBuffer();
    void PerformSinglePassLighting();
    void PerformDualPassLighting_PerPixel();
    void PerformDualPassLighting_PerSample();
    void PerformSinglePassLighting_NoMSAA();

    NvModelGL* LoadModel(const char *model_filename);
    Nv::NvModelExtGL* LoadModelExt(const char *model_filename);
    void DrawModel(NvGLSLProgram* shader);
    void BuildShaders();
    void DestroyShaders();
    void ReloadShaders();
    void RenderFullscreenQuad(NvGLSLProgram* shader);

    NvModelGL *m_models[MODELS_COUNT];
    Nv::NvModelExtGL *m_modelsExt[MODELS_COUNT];
    uint32_t m_modelID;
    GLuint m_queryId;

    int m_imageWidth;
    int m_imageHeight;

    int m_iMSAACount;
    
    uint32_t m_approach;
    uint32_t m_brdf;
    RenderTargetMode m_currentRTMode;

    bool m_bAdaptiveShading;
    bool m_bMarkComplexPixels;
    bool m_bSeparateComplexPass;
    bool m_bUsePerSamplePixelShader;
    bool m_bUseExtendedModelLoader;

#if ENABLE_GPU_TIMERS
    NvGPUTimer m_timers[TIMER_COUNT];
#endif

    const float *m_backgroundColor;

    // GBuffer resources
    NvGLSLProgram* m_shaderFillGBuffer;
    NvGLSLProgram* m_shaderFillGBuffer_NoMSAA;

    // Mask Generation
    NvGLSLProgram* m_shaderMaskGeneration;

    // Lighting resources
    NvGLSLProgram* m_shaderLighting;
    NvGLSLProgram* m_shaderLightingPerSample;
    NvGLSLProgram* m_shaderLighting_NoMSAA;
    NvGLSLProgram* m_shaderLightingMarkComplex;

    // Textures
    GLuint m_texGBuffer[3];
    GLuint m_texGBufferFboId;

    GLuint m_texGBufResolved2;
    GLuint m_texGBufResolved2FboId;

    GLuint m_texDepthStencilBuffer;

    GLuint m_texOffscreenMSAA;
    GLuint m_texOffscreenMSAAFboId;

    // Id of the frame buffer object currently being used as the target for lighting pass(es).
    // This is not a separately allocated buffer, but simply the Id of an existing target
    // to use, based on the current MSAA/rendering mode
    GLuint m_lightingAccumulationBufferFboId;

    nv::matrix4f m_proj;
    nv::matrix4f m_MVP;
    nv::matrix4f m_normalMat;
    nv::matrix4f m_fullscreenMVP;

    NvUIText* m_statsText;

    // UI Tweak Variables
    NvTweakVarBase* m_pUIApproachVar;
    NvTweakVarBase* m_pUIBRDFVar;
    NvTweakVarBase* m_pUIAdaptiveShadingVar;
    NvTweakVarBase* m_pUIMarkComplexPixelsVar;
    NvTweakVarBase* m_pUISeparateComplexPassVar;
    NvTweakVarBase* m_pUIPerSampleShadingVar;
    NvTweakVarBase* m_pUIExtendedModelLoaderVar;
};

#endif
