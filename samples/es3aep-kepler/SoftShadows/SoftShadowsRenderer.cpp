//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/SoftShadowsRenderer.cpp
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
#include "SoftShadowsCommon.h"
#include "SoftShadowsRenderer.h"

#include <NvGLUtils/NvImageGL.h>
#include <NvGLUtils/NvShapesGL.h>

#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////
static const uint32_t LIGHT_RES = 1024;
static const float LIGHT_ZFAR = 32.0f;

////////////////////////////////////////////////////////////////////////////////
// File names
////////////////////////////////////////////////////////////////////////////////
static const char *ROCK_DIFFUSE_MAP_FILENAME = "textures/lichen6_rock.dds";
static const char *GROUND_DIFFUSE_MAP_FILENAME = "textures/lichen6.dds";
static const char *GROUND_NORMAL_MAP_FILENAME = "textures/lichen6_normal.dds";

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::MeshInstance::draw()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::MeshInstance::draw(SceneShader &shader)
{
    shader.setWorldMatrix(m_worldTransform);
    m_mesh->render(shader);
    CHECK_GL_ERROR();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::MeshInstance::accumStats()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::MeshInstance::accumStats(uint64_t &numIndices, uint64_t &numVertices)
{
    numIndices += m_mesh->getIndexCount();
    numVertices += m_mesh->getVertexCount();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::SoftShadowsRenderer()
////////////////////////////////////////////////////////////////////////////////
SoftShadowsRenderer::SoftShadowsRenderer(NvInputTransformer& camera)
    : m_meshInstances()
    , m_knightMesh(0)
    , m_podiumMesh(0)
    , m_viewProj()
    , m_lightViewProj()
    , m_projection()
    , m_camera(camera)
    , m_lightRadiusWorld(0.5f)
    , m_shadowMapFramebuffer(0)
    , m_shadowMapShader(0)
    , m_visTexShader(0)
    , m_depthPrepassShader(0)
    , m_pcssShader(0)
    , m_groundHeight(0)
    , m_groundRadius(0)
    , m_groundVertexBuffer(0)
    , m_groundIndexBuffer(0)
    , m_frameNumber(0)
    , m_backgroundColor(0.0f, 0.0f, 0.0f, 1.0f)
    , m_worldHeightOffset(0.2f)
    , m_worldWidthOffset(0.2f)
    , m_pcfSamplePattern(Poisson_25_25)
    , m_pcssSamplePattern(Poisson_25_25)
    , m_shadowTechnique(PCSS)
    , m_useTexture(true)
    , m_visualizeDepthTexture(false)
{
    m_camera.setMotionMode(NvCameraMotionType::DUAL_ORBITAL);

    std::fill(&m_samplers[0], &m_samplers[NumTextureUnits], 0);
    std::fill(&m_textures[0], &m_textures[NumTextureUnits], 0);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::~SoftShadowsRenderer()
////////////////////////////////////////////////////////////////////////////////
SoftShadowsRenderer::~SoftShadowsRenderer()
{
    destroyShaders();
    destroyMeshInstances();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::configurationCallback()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::configurationCallback(NvGLConfiguration& config)
{
    config.depthBits = 24; 
    config.stencilBits = 0; 
    // we need ES3.1AEP at least, but we'll take big GL if we can get it
    config.apiVer = NvGLAPIVersionGL4_4();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::initRendering()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::initRendering()
{
    GLint depthBits;
    glGetIntegerv(GL_DEPTH_BITS, &depthBits);
    LOGI("depth bits = %d\n", depthBits);

    // Setup the eye's view parameters
    initCamera(
        NvCameraXformType::MAIN,
        nv::vec3f(-0.644995f, 0.614183f, 0.660632f) * 1.5f, // position
        nv::vec3f(0.0f, 0.0f, 0.0f));                       // look at point

    // Setup the light's view parameters
    initCamera(
        NvCameraXformType::SECONDARY,
        nv::vec3f(3.57088f, 6.989f, 5.19698f) * 1.5f, // position
        nv::vec3f(0.0f, 0.0f, 0.0f));                 // look at point

    // Generate the samplers
    glGenSamplers(5, m_samplers);
    CHECK_GL_ERROR();
    for (GLuint unit = 0; unit < NumTextureUnits; ++unit)
    {
        glBindSampler(unit, m_samplers[unit]);
    }

    // Create resources
    createShadowMap();
    createGeometry();
    createTextures();
    createShaders();

    // Set states that don't change
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);

    glClearColor(
        m_backgroundColor.x,
        m_backgroundColor.y,
        m_backgroundColor.z,
        m_backgroundColor.w);
    glClearDepthf(1.0f);

    for (GLuint unit = 0; unit < NumTextureUnits; ++unit)
    {
        glBindSampler(unit, 0);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::updateFrame()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::updateFrame(float deltaTime)
{
    updateCamera();
    updateLightCamera();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::render()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::render(float deltaTime)
{
    // Frame counter
    m_frameNumber++;
    
    // Bind all our textures and samplers
    for (GLuint unit = 0; unit < NumTextureUnits; ++unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_textures[unit]);
        glBindSampler(unit, m_samplers[unit]);
    }
    
    //
    // STEP 1: render shadow map from the light's point of view
    //
    if (m_shadowTechnique != None || m_visualizeDepthTexture)
    {
        GLuint prevFBO = 0;
        // Enum has MANY names based on extension/version
        // but they all map to 0x8CA6
        glGetIntegerv(0x8CA6, (GLint*)&prevFBO);

        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFramebuffer);

        glViewport(0, 0, LIGHT_RES, LIGHT_RES);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(4.0f, 32.0f);

        m_shadowMapShader->enable();
        drawMeshes(*m_shadowMapShader);
        m_shadowMapShader->disable();

        glDisable(GL_POLYGON_OFFSET_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        glViewport(0, 0, m_screenWidth, m_screenHeight);
    }

    //
    // STEP 2: render scene from the eye's point of view
    //	
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_visualizeDepthTexture)
    {
        m_visTexShader->enable();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        NvDrawQuadGL(
            m_visTexShader->getPositionAttrHandle(),
            m_visTexShader->getTexCoordAttrHandle());

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        m_visTexShader->disable();
    }
    else
    {
        // To reduce overdraw, do a depth prepass to layout z
        m_depthPrepassShader->enable();

        drawMeshes(*m_depthPrepassShader);
        drawGround(*m_depthPrepassShader);

        m_depthPrepassShader->disable();
        
        // Do the shading pass
        EyeShader *shader = 0;
        switch (m_shadowTechnique)
        {            
        case None:        
            shader = m_pcssShader;
            m_pcssShader->enable();
            m_pcssShader->setShadowTechnique(static_cast<GLint>(m_shadowTechnique));
            break;

        case PCSS:
            shader = m_pcssShader;
            m_pcssShader->enable();
            m_pcssShader->setShadowTechnique(static_cast<GLint>(m_shadowTechnique));
            m_pcssShader->setSamplePattern(static_cast<GLint>(m_pcssSamplePattern));
            break;

        case PCF:
            shader = m_pcssShader;
            m_pcssShader->enable();
            m_pcssShader->setShadowTechnique(static_cast<GLint>(m_shadowTechnique));
            m_pcssShader->setSamplePattern(static_cast<GLint>(m_pcfSamplePattern));
            break;
        }
        if (shader != 0)
        {
            glDepthFunc(GL_EQUAL);

            drawMeshes(*shader);
            drawGround(*shader);

            shader->disable();
            glDepthFunc(GL_LEQUAL);
        }
        CHECK_GL_ERROR();
    }

    for (GLuint unit = 0; unit < NumTextureUnits; ++unit)
    {
        glBindSampler(unit, 0);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::reshape()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::reshape(int32_t width, int32_t height)
{
    // Update the cameras
    perspective(m_projection, NV_PI / 4.0f, (float)width / (float)height, 0.01f, 100.0f);
    m_camera.setScreenSize(width, height);

    glViewport(0, 0, (GLint)width, (GLint) height);
    m_screenWidth = width;
    m_screenHeight = height;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::setLightRadiusWorld()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::setLightRadiusWorld(float radius)
{
    m_lightRadiusWorld = radius;
    updateLightCamera();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::addMeshInstance()
////////////////////////////////////////////////////////////////////////////////
SoftShadowsRenderer::MeshInstance *SoftShadowsRenderer::addMeshInstance(
    RigidMesh *mesh,
    const nv::matrix4f &worldTransform,
    const wchar_t *name)
{
    m_meshInstances.push_back(new MeshInstance(mesh, name, worldTransform));
    return m_meshInstances.back();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::removeMeshInstance()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::removeMeshInstance(MeshInstance *instance)
{
    m_meshInstances.remove(instance);
    delete instance;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::getSceneStats()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::getSceneStats(uint64_t &numIndices, uint64_t &numVertices, uint32_t &lightResolution) const
{
    numIndices = 0;
    numVertices = 0;
    lightResolution = LIGHT_RES;

    for (MeshInstanceList::const_iterator it = m_meshInstances.begin(); it != m_meshInstances.end(); ++it)
    {
        MeshInstance *instance = *it;
        instance->accumStats(numIndices, numVertices);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::initCamera()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::initCamera(
    NvCameraXformType::Enum index,
    const nv::vec3f &eye,
    const nv::vec3f &at)
{
    // Construct the look matrix
    nv::matrix4f look;
    lookAt(look, eye, at, nv::vec3f(0.0f, 1.0f, 0.0f));

    // Decompose the look matrix to get the yaw and pitch.
    float pitch = atan2(-look._32, look._33);
    float yaw = atan2(look._31, length(nv::vec2f(-look._32, look._33)));

    // Initialize the camera view.
    m_camera.setRotationVec(nv::vec3f(pitch, yaw, 0.0f), index);
    m_camera.setTranslationVec(nv::vec3f(look._41, look._42, look._43), index);
    m_camera.update(0.0f);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::updateCamera()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::updateCamera()
{
    const nv::matrix4f &proj = m_projection;
    const nv::matrix4f &view = m_camera.getModelViewMat();

    nv::matrix4f offset;
    nv::translation(offset, m_worldWidthOffset, 0.0f, m_worldHeightOffset);

    nv::matrix4f offsetView = offset * view;

    m_viewProj = proj * offsetView;

    if (m_depthPrepassShader)
    {
        m_depthPrepassShader->enable();
        m_depthPrepassShader->setViewProjMatrix(m_viewProj);
        m_depthPrepassShader->disable();
        CHECK_GL_ERROR();
    }

    if (m_pcssShader)
    {
        m_pcssShader->enable();
        m_pcssShader->setViewProjMatrix(m_viewProj);
        m_pcssShader->disable();
        CHECK_GL_ERROR();
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::updateLightCamera()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::updateLightCamera()
{
    nv::matrix4f view = m_camera.getModelViewMat(NvCameraXformType::SECONDARY);
    nv::matrix4f inverseView = nv::inverse(view);
    nv::vec3f lightCenterWorld = transformCoord(inverseView, nv::vec3f(0.0f, 0.0f, 0.0f));

    static nv::vec3f s_rot;
    static nv::vec3f s_trans;
    static float s_scale;
    // If the light source is high enough above the ground plane
    if (lightCenterWorld.y > 1.0f)
    {
        s_rot = m_camera.getRotationVec(NvCameraXformType::SECONDARY);
        s_trans = m_camera.getTranslationVec(NvCameraXformType::SECONDARY);
        s_scale = m_camera.getScale(NvCameraXformType::SECONDARY);        
    }
    else
    {
        m_camera.setRotationVec(s_rot, NvCameraXformType::SECONDARY);
        m_camera.setTranslationVec(s_trans, NvCameraXformType::SECONDARY);
        m_camera.setScale(s_scale, NvCameraXformType::SECONDARY);
        m_camera.update(0.0f);
    }
    updateLightCamera(m_camera.getModelViewMat(NvCameraXformType::SECONDARY));
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::updateLightCamera()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::updateLightCamera(const nv::matrix4f &view)
{
    // Assuming that the bbox of mesh1 contains everything
    nv::vec3f center = m_knightMesh->getWorldCenter();
    nv::vec3f extents = m_knightMesh->getExtents();

    nv::vec3f box[2];
    box[0] = center - extents;
    box[1] = center + extents;

    nv::vec3f bbox[2];
    transformBoundingBox(bbox, box, view);

    float frustumWidth = std::max(fabs(bbox[0][0]), fabs(bbox[1][0])) * 2.0f;
    float frustumHeight = std::max(fabs(bbox[0][1]), fabs(bbox[1][1])) * 2.0f;
    float zNear = -bbox[1][2];
    float zFar = LIGHT_ZFAR;

    nv::matrix4f proj;
    perspectiveFrustum(proj, frustumWidth, frustumHeight, zNear, zFar);
    m_lightViewProj = proj * view;

    nv::matrix4f clip2Tex;
    clip2Tex.set_scale(nv::vec3f(0.5f, 0.5f, 0.5f));
    clip2Tex.set_translate(nv::vec3f(0.5f, 0.5f, 0.5f));

    nv::matrix4f viewProjClip2Tex = clip2Tex * m_lightViewProj;

    nv::matrix4f inverseView = nv::inverse(view);
    nv::vec3f lightCenterWorld = transformCoord(inverseView, nv::vec3f(0.0f, 0.0f, 0.0f));

    if (m_shadowMapShader)
    {
        m_shadowMapShader->enable();
        m_shadowMapShader->setViewProjMatrix(m_lightViewProj);
        m_shadowMapShader->disable();
        CHECK_GL_ERROR();
    }

    if (m_visTexShader)
    {
        m_visTexShader->enable();
        m_visTexShader->setLightZNear(zNear);
        m_visTexShader->setLightZFar(zFar);
        m_visTexShader->disable();
        CHECK_GL_ERROR();
    }

    if (m_pcssShader)
    {
        m_pcssShader->enable();
        m_pcssShader->setLightViewMatrix(view);
        m_pcssShader->setLightViewProjClip2TexMatrix(viewProjClip2Tex);
        m_pcssShader->setLightZNear(zNear);
        m_pcssShader->setLightZFar(zFar);
        m_pcssShader->setLightPosition(lightCenterWorld);
        m_pcssShader->disable();
        CHECK_GL_ERROR();
    }

    updateLightSize(frustumWidth, frustumHeight);
    CHECK_GL_ERROR();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::updateLightSize()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::updateLightSize(float frustumWidth, float frustumHeight)
{
    if (m_pcssShader)
    {
        nv::vec2f lightRadiusUV(
            m_lightRadiusWorld / frustumWidth, 
            m_lightRadiusWorld / frustumHeight);
        m_pcssShader->enable();
        m_pcssShader->setLightRadiusUv(lightRadiusUV);
        m_pcssShader->disable();
    }
}

void debugCallback(GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar *message,
            void *userParam)
{
    LOGE("%s\n", message);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::createShadowMap()
////////////////////////////////////////////////////////////////////////////////
bool SoftShadowsRenderer::createShadowMap()
{
    GLuint prevFBO = 0;
    // Enum has MANY names based on extension/version
    // but they all map to 0x8CA6
    glGetIntegerv(0x8CA6, (GLint*)&prevFBO);

    // Setup the shadowmap depth sampler
    glSamplerParameteri(m_samplers[ShadowDepthTextureUnit], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(m_samplers[ShadowDepthTextureUnit], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(m_samplers[ShadowDepthTextureUnit], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(m_samplers[ShadowDepthTextureUnit], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
//    glSamplerParameterfv(m_samplers[ShadowDepthTextureUnit], GL_TEXTURE_BORDER_COLOR, borderColor);
    CHECK_GL_ERROR();

    // Setup the shadowmap PCF sampler
    glSamplerParameteri(m_samplers[ShadowPcfTextureUnit], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(m_samplers[ShadowPcfTextureUnit], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(m_samplers[ShadowPcfTextureUnit], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(m_samplers[ShadowPcfTextureUnit], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glSamplerParameterfv(m_samplers[ShadowPcfTextureUnit], GL_TEXTURE_BORDER_COLOR, borderColor);
    glSamplerParameteri(m_samplers[ShadowPcfTextureUnit], GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glSamplerParameteri(m_samplers[ShadowPcfTextureUnit], GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    CHECK_GL_ERROR();

    // Generate the framebuffer
    glGenFramebuffers(1, &m_shadowMapFramebuffer);
    CHECK_GL_ERROR();
    if (m_shadowMapFramebuffer == 0)
        return false;

    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFramebuffer);

    // Generate the shadowmap texture
    glActiveTexture(GL_TEXTURE0 + ShadowDepthTextureUnit);
    glGenTextures(1, &m_textures[ShadowDepthTextureUnit]);
    if (&m_textures[ShadowDepthTextureUnit] == 0)
        return false;

    m_textures[ShadowPcfTextureUnit] = m_textures[ShadowDepthTextureUnit];
    glBindTexture(GL_TEXTURE_2D, m_textures[ShadowDepthTextureUnit]);
    glTexStorage2D(
        GL_TEXTURE_2D, 1,
        GL_DEPTH_COMPONENT32F,
        LIGHT_RES, LIGHT_RES);
    CHECK_GL_ERROR();
    
    // Add the shadowmap texture to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textures[ShadowDepthTextureUnit], 0);
    CHECK_GL_ERROR();
    GLuint buffer = GL_NONE;
    glDrawBuffers(1, &buffer);
    CHECK_GL_ERROR();
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;	

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::createGeometry()
////////////////////////////////////////////////////////////////////////////////
bool SoftShadowsRenderer::createGeometry()
{
    return createPlane(
        m_groundIndexBuffer,
        m_groundVertexBuffer,
        m_groundRadius,
        m_groundHeight);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::createTextures()
////////////////////////////////////////////////////////////////////////////////
bool SoftShadowsRenderer::createTextures()
{
    // Setup the samplers
    for (GLuint unit = GroundDiffuseTextureUnit; unit <= RockDiffuseTextureUnit; ++unit)
    {
        glSamplerParameteri(m_samplers[unit], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(m_samplers[unit], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(m_samplers[unit], GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(m_samplers[unit], GL_TEXTURE_WRAP_T, GL_REPEAT);
        CHECK_GL_ERROR();
    }

    // Load the ground diffuse map
    glActiveTexture(GL_TEXTURE0 + GroundDiffuseTextureUnit);
    m_textures[GroundDiffuseTextureUnit] = NvImageGL::UploadTextureFromDDSFile(GROUND_DIFFUSE_MAP_FILENAME);
    CHECK_GL_ERROR();
    if (m_textures[GroundDiffuseTextureUnit] == 0)
        return false;

    // Load the ground normal map
    glActiveTexture(GL_TEXTURE0 + GroundNormalTextureUnit);
    m_textures[GroundNormalTextureUnit] = NvImageGL::UploadTextureFromDDSFile(GROUND_NORMAL_MAP_FILENAME);
    CHECK_GL_ERROR();
    if (m_textures[GroundNormalTextureUnit] == 0)
        return false;

    // Load the ground specular map
    glActiveTexture(GL_TEXTURE0 + RockDiffuseTextureUnit);
    m_textures[RockDiffuseTextureUnit] = NvImageGL::UploadTextureFromDDSFile(ROCK_DIFFUSE_MAP_FILENAME);
    CHECK_GL_ERROR();
    if (m_textures[RockDiffuseTextureUnit] == 0)
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::createShaders()
////////////////////////////////////////////////////////////////////////////////
bool SoftShadowsRenderer::createShaders()
{
    destroyShaders();

    // Shadow map shader
    m_shadowMapShader = new ShadowMapShader();
    CHECK_GL_ERROR();
    if (!m_shadowMapShader)
        return false;

    m_shadowMapShader->enable();
    m_shadowMapShader->disable();
    CHECK_GL_ERROR();

    // Visualize depth texture shader
    m_visTexShader = new VisTexShader();
    CHECK_GL_ERROR();
    if (!m_visTexShader)
        return false;

    m_visTexShader->enable();
    m_visTexShader->setShadowMapTexture(ShadowDepthTextureUnit);
    m_visTexShader->disable();
    CHECK_GL_ERROR();

    // Depth prepass shader
    m_depthPrepassShader = new DepthPrepassShader();
    CHECK_GL_ERROR();
    if (!m_depthPrepassShader)
        return false;

    m_depthPrepassShader->enable();
    m_depthPrepassShader->disable();
    CHECK_GL_ERROR();

    // PCSS shader
    m_pcssShader = new PcssShader();
    CHECK_GL_ERROR();
    if (!m_pcssShader)
        return false;

    m_pcssShader->enable();
    m_pcssShader->setShadowMapTexture(ShadowDepthTextureUnit, ShadowPcfTextureUnit);
    m_pcssShader->setGroundDiffuseTexture(GroundDiffuseTextureUnit);
    m_pcssShader->setGroundNormalTexture(GroundNormalTextureUnit);
    m_pcssShader->setRockDiffuseTexture(RockDiffuseTextureUnit);
    m_pcssShader->setPodiumCenterLocation(m_podiumMesh->getCenter());
    m_pcssShader->disable();
    CHECK_GL_ERROR();

    // Update the light size
    updateLightCamera();
    CHECK_GL_ERROR();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::destroyShaders()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::destroyShaders()
{
    delete m_pcssShader;
    m_pcssShader = 0;

    delete m_depthPrepassShader;
    m_depthPrepassShader = 0;

    delete m_visTexShader;
    m_visTexShader = 0;

    delete m_shadowMapShader;
    m_shadowMapShader = 0;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::destroyMeshInstances()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::destroyMeshInstances()
{
    for (MeshInstanceList::iterator it = m_meshInstances.begin(); it != m_meshInstances.end(); ++it)
    {
        delete *it;
    }
    m_meshInstances.clear();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::drawMeshes()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::drawMeshes(SceneShader &shader)
{
    shader.setUseDiffuse(true);
    for (MeshInstanceList::iterator it = m_meshInstances.begin(); it != m_meshInstances.end(); ++it)
    {
        MeshInstance *instance = *it;
        if (instance == m_podiumMesh)
            shader.setUseTexture(m_useTexture ? 2 : 0);
        else
            shader.setUseTexture(0);

        instance->draw(shader);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::drawGround()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::drawGround(SceneShader &shader)
{
    // Set uniforms
    shader.setUseDiffuse(false);
    shader.setUseTexture(m_useTexture ? 1 : 0);

    // Bind the VBO for the vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_groundVertexBuffer);

    // Set up attribute for the position (3 floats)
    GLint positionLocation = shader.getPositionAttrHandle();
    if (positionLocation >= 0)
    {
        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (GLvoid *)0);
        glEnableVertexAttribArray(positionLocation);
    }

    // Set up attribute for the normal (3 floats)
    GLint normalLocation = shader.getNormalAttrHandle();
    if (normalLocation >= 0)
    {
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (GLvoid *)(3 * sizeof(float)));
        glEnableVertexAttribArray(normalLocation);
    }

    // Set up the indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_groundIndexBuffer);

    // Do the actual drawing
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    // Clear state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (positionLocation >= 0)
    {
        glDisableVertexAttribArray(positionLocation);
    }
    if (normalLocation >= 0)
    {
        glDisableVertexAttribArray(normalLocation);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
