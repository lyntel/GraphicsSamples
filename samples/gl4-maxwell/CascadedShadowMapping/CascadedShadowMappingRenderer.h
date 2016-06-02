//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CascadedShadowMapping/CascadedShadowMappingRenderer.h
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
#pragma once

#include <NvAppBase/NvInputTransformer.h>
#include <NvGLUtils/NvModelGL.h>
#include "CascadedShadowMappingCommon.h"

class CameraProgram;
class LightVsOnlyProgram;
class LightProgram;
class NvSampleAppGL;

struct ModelData {
    NvModelGL*   model;
    nv::matrix4f modelMatrix;
    nv::vec3f    materialColor;
};

class CascadedShadowMappingRenderer
{
public:
    CascadedShadowMappingRenderer(NvSampleAppGL& app, NvInputTransformer& camera);
    ~CascadedShadowMappingRenderer();

    void initRendering();
    void render(float deltaTime);
    void reshape(int32_t width, int32_t height);
    void shutdownRendering();

    void setShadowMapMethod(ShadowMapMethod::Enum method);
    void setFrustumSegmentCount(unsigned int frustumSegmentCount);
private:
    NvSampleAppGL&      m_app;
    // Camera params.
    NvInputTransformer& m_camera;
    nv::vec2f           m_viewportDims;
    float               m_fov;
    int32_t             m_width;
    int32_t             m_height;
    nv::matrix4f        m_projMatrix;
    float               m_nearPlane;
    float               m_farPlane;
    unsigned int        m_frustumSegmentCount; // Number 1-4
    float               m_frustumSplitCorrection;
    nv::vec4f           m_farPlanes;
    nv::vec4f           m_normalizedFarPlanes; // Normalized to [0, 1] depth range.

    // Light params.
    nv::vec3f    m_lightDir;
    nv::matrix4f m_lightViewMatrix;
    nv::matrix4f m_lightProjMatrix;
    nv::matrix4f m_lightSegmentVPSBMatrices[MAX_CAMERA_FRUSTUM_SPLIT_COUNT];
    nv::vec4f    m_lightViewports[MAX_CAMERA_FRUSTUM_SPLIT_COUNT];

    CameraProgram*         m_cameraProgram; // for rendering the final scene from the camera's point of view
    LightProgram*	       m_lightStandardProgram; // starndard gs
    LightProgram*          m_lightGsCullProgram; // standard gs with culling
    LightProgram*          m_lightGsMulticastCullProgram; // normal gs with multicast and culling
    LightProgram*          m_lightFgsMulticastCullProgram; // fast gs with multicast and culling
    LightVsOnlyProgram*    m_lightVsOnlyMulticastProgram; // vertex shader only with multicast
    GLuint                 m_lightFBO;
    GLuint                 m_lightTex;

    std::vector<NvModelGL*> m_models;
    std::vector<ModelData>  m_modelInstances;
    ShadowMapMethod::Enum   m_method;

private:
    CascadedShadowMappingRenderer(const CascadedShadowMappingRenderer&);
    CascadedShadowMappingRenderer& operator=(const CascadedShadowMappingRenderer&);

    void initCamera(
        NvCameraXformType::Enum index,
        const nv::vec3f& position,
        const nv::vec3f& focusPoint);

    void initLight(
        const nv::vec3f& position,
        const nv::vec3f& center,
        const nv::vec3f& up);

    void initGeometry(const unsigned int& knightCount);

    void frustumBoundingBoxLightViewSpace(float nearPlane, float farPlane, nv::vec4f& min, nv::vec4f& max);
    void updateFrustumSegmentFarPlanes();
    void updateLightProjAndViewports();

    void renderLight();
    void renderCamera();
};
