//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CascadedShadowMapping/CascadedShadowMappingRenderer.cpp
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
#include "CascadedShadowMappingRenderer.h"
#include "FloorModel.h"
#include "KnightNvModel.h"
#include "LightVsOnlyProgram.h"
#include "LightProgram.h"
#include "CameraProgram.h"

#include <NV/NvLogs.h>
#include <NvAssetLoader/NvAssetLoader.h>
#include <NvGLUtils/NvGLSLProgram.h>
#include <NvAppBase/gl/NvSampleAppGL.h>

#include <cstdlib>
#include <ctime>

#define LIGHT_TEXTURE_SIZE (4096)
#define LIGHT_TEXUTRE_MIPMAP_LEVELS (11)

CascadedShadowMappingRenderer::CascadedShadowMappingRenderer(NvSampleAppGL& app, NvInputTransformer& camera)
    : m_app(app)
    , m_camera(camera)
    , m_fov(NV_PI * 0.5f)
    , m_width(0)
    , m_height(0)
    , m_projMatrix()
    , m_nearPlane(0.1f)
    , m_farPlane(100.0f)
    , m_frustumSegmentCount(4)
    , m_frustumSplitCorrection(0.8f)
    , m_normalizedFarPlanes()
    , m_lightDir()
    , m_lightViewMatrix()
    , m_lightProjMatrix()
    , m_cameraProgram(nullptr)
    , m_lightGsCullProgram(nullptr)
    , m_lightGsMulticastCullProgram(nullptr)
    , m_lightFgsMulticastCullProgram(nullptr)
    , m_lightStandardProgram(nullptr)
    , m_lightVsOnlyMulticastProgram(nullptr)
    , m_lightFBO(0)
    , m_lightTex(0)
    , m_method(ShadowMapMethod::GsNoCull)
{
    srand(static_cast<unsigned int>(time(nullptr)));
    m_camera.setMotionMode(NvCameraMotionType::FIRST_PERSON);
}

CascadedShadowMappingRenderer::~CascadedShadowMappingRenderer()
{

}

void CascadedShadowMappingRenderer::initRendering()
{
    initCamera(
        NvCameraXformType::MAIN,
        nv::vec3f(-15.0f, 5.0f, 15.0f), // position
        nv::vec3f(0.0f, 0.0f, 0.0f));   // look at point

    // Setup the light parameters.
    initLight(
        nv::vec3f(100.0f, 100.0f, 100.0f),
        nv::vec3f(0.0f, 0.0f, 0.0f),
        nv::vec3f(0.0f, 1.0f, 0.0f));

    // Load shaders.
    NvAssetLoaderAddSearchPath("gl4-maxwell/CascadedShadowMapping");

    m_cameraProgram = new CameraProgram();
    m_cameraProgram->init("shaders/Camera.vert", "shaders/Camera.frag");

    m_lightStandardProgram = new LightProgram();
    m_lightStandardProgram->init("shaders/Light.vert", "shaders/LightStandard.geom");

    m_lightGsCullProgram = new LightProgram();
    m_lightGsCullProgram->init("shaders/Light.vert", "shaders/LightGsCull.geom");

    m_lightGsMulticastCullProgram = new LightProgram();
    m_lightGsMulticastCullProgram->init("shaders/Light.vert", "shaders/LightMulticast.geom");

    m_lightFgsMulticastCullProgram = new LightProgram();
    m_lightFgsMulticastCullProgram->init("shaders/Light.vert", "shaders/LightFgsMulticast.geom");

    m_lightVsOnlyMulticastProgram = new LightVsOnlyProgram();
    m_lightVsOnlyMulticastProgram->init("shaders/LightVsOnly.vert");

    // Setup geometry.
    initGeometry(10);

    // Setup resources for shadow pass.
    glGenFramebuffers(1, &m_lightFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_lightFBO);

    glGenTextures(1, &m_lightTex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_lightTex);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, LIGHT_TEXUTRE_MIPMAP_LEVELS, GL_DEPTH_COMPONENT32F, LIGHT_TEXTURE_SIZE, LIGHT_TEXTURE_SIZE, MAX_CAMERA_FRUSTUM_SPLIT_COUNT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_lightTex, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_app.getMainFBO());

    glGetFloatv(GL_MAX_VIEWPORT_DIMS, m_viewportDims._array);
}

void CascadedShadowMappingRenderer::render(float deltaTime)
{
    renderLight();
    renderCamera();
}

void CascadedShadowMappingRenderer::reshape(int32_t width, int32_t height)
{
    m_width = width;
    m_height = height;
    nv::perspective(m_projMatrix, m_fov, static_cast<float>(m_width) / m_height, m_nearPlane, m_farPlane);
    updateFrustumSegmentFarPlanes();
}

void CascadedShadowMappingRenderer::shutdownRendering()
{
    if (m_cameraProgram) {
        delete m_cameraProgram;
        m_cameraProgram = nullptr;
    }

    if (m_lightGsMulticastCullProgram) {
        delete m_lightGsMulticastCullProgram;
        m_lightGsMulticastCullProgram = nullptr;
    }

    if (m_lightFgsMulticastCullProgram) {
        delete m_lightFgsMulticastCullProgram;
        m_lightFgsMulticastCullProgram = nullptr;
    }

    if (m_lightStandardProgram) {
        delete m_lightStandardProgram;
        m_lightStandardProgram = nullptr;
    }

    if (m_lightGsCullProgram) {
        delete m_lightGsCullProgram;
        m_lightGsCullProgram = nullptr;
    }
    
    if (m_lightVsOnlyMulticastProgram) {
        delete m_lightVsOnlyMulticastProgram;
        m_lightVsOnlyMulticastProgram = nullptr;
    }

    m_modelInstances.clear();
    for (unsigned int i = 0; i < m_models.size(); ++i) {
        delete m_models[i];
        m_models[i] = nullptr;
    }
    m_models.clear();
}

void CascadedShadowMappingRenderer::setShadowMapMethod(ShadowMapMethod::Enum method)
{
    m_method = method;
}

void CascadedShadowMappingRenderer::setFrustumSegmentCount(unsigned int frustumSegmentCount)
{
    m_frustumSegmentCount = frustumSegmentCount;
    updateFrustumSegmentFarPlanes();
}

void CascadedShadowMappingRenderer::initCamera(
    NvCameraXformType::Enum index,
    const nv::vec3f& eye,
    const nv::vec3f& at)
{
    // Construct the look matrix
    nv::matrix4f look;
    lookAt(look, eye, at, nv::vec3f(0.0f, 1.0f, 0.0f));

    // Decompose the look matrix to get the yaw and pitch.
    const float pitch = atan2(-look._32, look._33);
    const float yaw = atan2(look._31, length(nv::vec2f(-look._32, look._33)));

    // Get translation vector proper for first person camera.
    nv::vec3f translationVec(look._41, look._42, look._43);
    translationVec.rotate(-pitch, 1.0f, 0.0f, 0.0f);
    translationVec.rotate(-yaw, 0.0f, 1.0f, 0.0f);

    // Initialize the camera view.
    m_camera.setRotationVec(nv::vec3f(pitch, yaw, 0.0f), index);
    m_camera.setTranslationVec(translationVec, index);
    m_camera.update(0.0f);
}

void CascadedShadowMappingRenderer::initLight(
    const nv::vec3f& position,
    const nv::vec3f& center,
    const nv::vec3f& up)
{
    m_lightDir = position;
    // Sets the light to focus on the origin.
    nv::lookAt(m_lightViewMatrix, position, center, up);
}

void CascadedShadowMappingRenderer::initGeometry(const unsigned int& knightCount)
{
    const float floorSize = 50.0f;
    const float halfFloorSize = floorSize / 2.0f;
    // Add floor to models.
    FloorModel* floor = new FloorModel();
    NvModelGL* floorGL = NvModelGL::CreateFromModel(floor);
    nv::matrix4f floorModelMatrix;
    floorModelMatrix.set_scale(halfFloorSize);
    nv::vec3f floorMaterialColor(1.0f, 1.0f, 1.0f);
    ModelData floorData = { floorGL, floorModelMatrix, floorMaterialColor };
    m_models.push_back(floorGL);
    m_modelInstances.push_back(floorData);

    // Add knights.
    KnightNvModel* knight = new KnightNvModel();
	NvModelGL* knightGL = NvModelGL::CreateFromModel(knight);
    m_models.push_back(knightGL);
    for (unsigned int row = 0; row < knightCount; ++row) {
        for (unsigned int col = 0; col < knightCount; ++col) {
            nv::matrix4f knightModelMatrix;
            nv::matrix4f scale, translate;
            scale.set_scale(5.0f);
            nv::translation(translate, -halfFloorSize + row * 5.0f + 2.5f, 1.75767496f, -halfFloorSize + col * 5.0f + 2.5f);
            knightModelMatrix = translate * scale;
            nv::vec3f knightMaterialColor(1.0f, 1.0f, 1.0f);
            ModelData knightData = { knightGL, knightModelMatrix, knightMaterialColor };
            m_modelInstances.push_back(knightData);
        }
    }

}

void CascadedShadowMappingRenderer::updateFrustumSegmentFarPlanes()
{
    for (unsigned int i = 1; i <= m_frustumSegmentCount; ++i)
    {
        const float distFactor = static_cast<float>(i) / m_frustumSegmentCount;
        const float stdTerm = m_nearPlane * pow(m_farPlane / m_nearPlane, distFactor);
        const float corrTerm = m_nearPlane + distFactor * (m_farPlane - m_nearPlane);
        const float viewDepth = m_frustumSplitCorrection * stdTerm + (1.0f - m_frustumSplitCorrection) * corrTerm;
        m_farPlanes[i - 1] = viewDepth;
        const nv::vec4f projectedDepth = m_projMatrix * nv::vec4f(0.0f, 0.0f, - viewDepth, 1.0f);
        // Normalized to [0, 1] depth range.
        m_normalizedFarPlanes[i - 1] = (projectedDepth.z / projectedDepth.w) * 0.5f + 0.5f;
    }
}

void CascadedShadowMappingRenderer::frustumBoundingBoxLightViewSpace(float nearPlane, float farPlane, nv::vec4f& min, nv::vec4f& max)
{
    nv::vec4f frustumMin(std::numeric_limits<float>::max());
    nv::vec4f frustumMax(std::numeric_limits<float>::lowest());

    const float nearHeight = 2.0f * tan(m_fov * 0.5f) * nearPlane;
    const float nearWidth = nearHeight * static_cast<float>(m_width) / m_height;
    const float farHeight = 2.0f * tan(m_fov * 0.5f) * farPlane;
    const float farWidth = farHeight * static_cast<float>(m_width) / m_height;
    const nv::vec4f cameraPos = nv::inverse(m_camera.getTranslationMat()) * nv::vec4f(0.0f, 0.0f, 0.0f, 1.0f);
    const nv::matrix4f invRot = nv::inverse(m_camera.getRotationMat());
    const nv::vec4f viewDir = invRot * nv::vec4f(0.0f, 0.0f, -1.0f, 0.0f);
    const nv::vec4f upDir = invRot * nv::vec4f(0.0f, 1.0f, 0.0f, 0.0f);
    const nv::vec4f rightDir = invRot * nv::vec4f(1.0f, 0.0f, 0.0f, 0.0f);
    const nv::vec4f nc = cameraPos + viewDir * nearPlane; // near center
    const nv::vec4f fc = cameraPos + viewDir * farPlane; // far center

    // Vertices in a world space.
    nv::vec4f vertices[8] = {
        nc - upDir * nearHeight * 0.5f - rightDir * nearWidth * 0.5f, // nbl (near, bottom, left)
        nc - upDir * nearHeight * 0.5f + rightDir * nearWidth * 0.5f, // nbr
        nc + upDir * nearHeight * 0.5f + rightDir * nearWidth * 0.5f, // ntr
        nc + upDir * nearHeight * 0.5f - rightDir * nearWidth * 0.5f, // ntl
        fc - upDir * farHeight  * 0.5f - rightDir * farWidth * 0.5f, // fbl (far, bottom, left)
        fc - upDir * farHeight  * 0.5f + rightDir * farWidth * 0.5f, // fbr
        fc + upDir * farHeight  * 0.5f + rightDir * farWidth * 0.5f, // ftr
        fc + upDir * farHeight  * 0.5f - rightDir * farWidth * 0.5f, // ftl
    };

    for (unsigned int vertId = 0; vertId < 8; ++vertId) {
        // Light view space.
        vertices[vertId] = m_lightViewMatrix * vertices[vertId];
        // Update bounding box.
        frustumMin = nv::min(frustumMin, vertices[vertId]);
        frustumMax = nv::max(frustumMax, vertices[vertId]);
    }

    min = frustumMin;
    max = frustumMax;
}

void CascadedShadowMappingRenderer::updateLightProjAndViewports()
{
    // Find a bounding box of whole camera frustum in light view space.
    nv::vec4f frustumMin(std::numeric_limits<float>::max());
    nv::vec4f frustumMax(std::numeric_limits<float>::lowest());
    frustumBoundingBoxLightViewSpace(m_nearPlane, m_farPlane, frustumMin, frustumMax);

    // Update light projection matrix to only cover the area viewable by the camera
    nv::ortho3D(m_lightProjMatrix, frustumMin.x, frustumMax.x, frustumMin.y, frustumMax.y, 0.0f, frustumMin.z);

    // Find a bounding box of segment in light view space.
    float nearSegmentPlane = 0.0f;
    for (unsigned int i = 0; i < m_frustumSegmentCount; ++i) {
        nv::vec4f segmentMin(std::numeric_limits<float>::max());
        nv::vec4f segmentMax(std::numeric_limits<float>::lowest());
        frustumBoundingBoxLightViewSpace(nearSegmentPlane, m_farPlanes[i], segmentMin, segmentMax);

        // Update viewports.
        nv::vec2f frustumSize(frustumMax.x - frustumMin.x, frustumMax.y - frustumMin.y);
        const float segmentSizeX = segmentMax.x - segmentMin.x;
        const float segmentSizeY = segmentMax.y - segmentMin.y;
        const float segmentSize = segmentSizeX < segmentSizeY ? segmentSizeY : segmentSizeX;
        const nv::vec2f offsetBottomLeft(segmentMin.x - frustumMin.x, segmentMin.y - frustumMin.y);
        const nv::vec2f offsetSegmentSizeRatio(offsetBottomLeft.x / segmentSize, offsetBottomLeft.y / segmentSize);
        const nv::vec2f frustumSegmentSizeRatio(frustumSize.x / segmentSize, frustumSize.y / segmentSize);

        nv::vec2f pixelOffsetTopLeft(offsetSegmentSizeRatio * LIGHT_TEXTURE_SIZE);
        nv::vec2f pixelFrustumSize(frustumSegmentSizeRatio * LIGHT_TEXTURE_SIZE);

        // Scale factor that helps if frustum size is supposed to be bigger
        // than maximum viewport size.
        nv::vec2f scaleFactor(
            m_viewportDims[0] < pixelFrustumSize.x ? m_viewportDims[0] / pixelFrustumSize.x : 1.0f,
            m_viewportDims[1] < pixelFrustumSize.y ? m_viewportDims[1] / pixelFrustumSize.y : 1.0f);

        pixelOffsetTopLeft *= scaleFactor;
        pixelFrustumSize *= scaleFactor;

        m_lightViewports[i] = nv::vec4f(-pixelOffsetTopLeft.x, -pixelOffsetTopLeft.y, pixelFrustumSize.x, pixelFrustumSize.y);
        glViewportIndexedfv(i, m_lightViewports[i]._array);

        // Update light view-projection matrices per segment.
        nv::matrix4f lightProj;
        nv::ortho3D(lightProj, segmentMin.x, segmentMin.x + segmentSize, segmentMin.y, segmentMin.y + segmentSize, 0.0f, frustumMin.z);
        nv::matrix4f lightScale;
        lightScale.set_scale(nv::vec3f(0.5f * scaleFactor.x, 0.5f * scaleFactor.y, 0.5f));
        nv::matrix4f lightBias;
        lightBias.set_translate(nv::vec3f(0.5f * scaleFactor.x, 0.5f * scaleFactor.y, 0.5f));
        m_lightSegmentVPSBMatrices[i] = lightBias * lightScale * lightProj * m_lightViewMatrix;

        nearSegmentPlane = m_normalizedFarPlanes[i];
    }

    // Set remaining viewports to some kind of standard state.
    for (unsigned int i = m_frustumSegmentCount; i < MAX_CAMERA_FRUSTUM_SPLIT_COUNT; ++i) {
        glViewportIndexedf(i, 0, 0, LIGHT_TEXTURE_SIZE, LIGHT_TEXTURE_SIZE);
    }
}

void CascadedShadowMappingRenderer::renderLight()
{
    LightProgram *shader = 0;
    switch(m_method) {
    case ShadowMapMethod::GsNoCull:
        shader = m_lightStandardProgram;
        break;
    case ShadowMapMethod::GsCull:
        shader = m_lightGsCullProgram;
        break;
    case ShadowMapMethod::MulticastCull:
        shader = m_lightGsMulticastCullProgram;
        break;
    case ShadowMapMethod::FgsMulticastCull:
        shader = m_lightFgsMulticastCullProgram;
        break;
    case ShadowMapMethod::VsOnlyMulticast:
        shader = m_lightVsOnlyMulticastProgram;
        break;
    default:
        return;
    }

    shader->enable();

    glBindFramebuffer(GL_FRAMEBUFFER, m_lightFBO);
    updateLightProjAndViewports();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    // Clear render target.
    const GLfloat one = 1.0f;
    glClearBufferfv(GL_DEPTH, 0, &one);

    shader->setUniform1i("frustumSegmentCount", m_frustumSegmentCount);

    // VS only method uses static viewport mask
    if (m_method == ShadowMapMethod::VsOnlyMulticast) {
        int mask = 0;
        for (unsigned int i = 0; i < m_frustumSegmentCount; ++i)
            mask += 1 << i;
        shader->setUniform1i("viewportMask", mask);
    }

    // Send viewport parameters for culling
    if (m_method == ShadowMapMethod::GsCull || m_method == ShadowMapMethod::MulticastCull || m_method == ShadowMapMethod::FgsMulticastCull) {
        shader->setUniform2f("shadowMapSize", LIGHT_TEXTURE_SIZE, LIGHT_TEXTURE_SIZE);
        shader->setUniform4fv("viewports", m_lightViewports[0]._array, m_frustumSegmentCount);
    }

    for (unsigned int i = 0; i < m_modelInstances.size(); ++i)
    {
        const nv::matrix4f mvpMatrix = m_lightProjMatrix * m_lightViewMatrix * m_modelInstances[i].modelMatrix;
        shader->setMvpMatrix(mvpMatrix);
        m_modelInstances[i].model->drawElements(shader->getPositionAttrHandle());
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_app.getMainFBO());
    glUseProgram(0);
}

void CascadedShadowMappingRenderer::renderCamera()
{
    m_cameraProgram->enable();
    glViewport(0, 0, m_width, m_height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    // Clear render target.
    const GLfloat zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const GLfloat one = 1.0f;
    glClearBufferfv(GL_COLOR, 0, zeros);
    glClearBufferfv(GL_DEPTH, 0, &one);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_lightTex);
    m_cameraProgram->setViewMatrix(m_camera.getModelViewMat());
    m_cameraProgram->setProjMatrix(m_projMatrix);
    m_cameraProgram->setLightDir(m_lightDir);
    m_cameraProgram->setLightVPSBMatrices(m_lightSegmentVPSBMatrices, MAX_CAMERA_FRUSTUM_SPLIT_COUNT);
    m_cameraProgram->setNormalizedFarPlanes(m_normalizedFarPlanes);
    m_cameraProgram->setAmbient(nv::vec3f(0.05f, 0.05f, 0.05f));
    m_cameraProgram->setSpecularAlbedo(nv::vec3f(1.0f, 1.0f, 1.0f));
    m_cameraProgram->setSpecularPower(16.0f);

    for (unsigned int i = 0; i < m_modelInstances.size(); ++i)
    {
        m_cameraProgram->setModelMatrix(m_modelInstances[i].modelMatrix);
        m_cameraProgram->setDiffuseAlbedo(m_modelInstances[i].materialColor);
        m_modelInstances[i].model->drawElements(
            m_cameraProgram->getPositionAttrHandle(),
            m_cameraProgram->getNormalAttrHandle()
            );
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    m_cameraProgram->disable();
}
