//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/SoftShadowsRenderer.h
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
#ifndef SOFT_SHADOWS_RENDERER_H
#define SOFT_SHADOWS_RENDERER_H

#include "RigidMesh.h"

#include "DepthPrepassShader.h"
#include "PcssShader.h"
#include "ShadowMapShader.h"
#include "VisTexShader.h"

#include <list>
#include <memory>
#include <string>

class NvSimpleMesh;
class CDXUTSDKMesh;
class CBaseCamera;

class SoftShadowsRenderer
{
public:
    SoftShadowsRenderer(NvInputTransformer& camera);
    ~SoftShadowsRenderer();

    class MeshInstance
    {
    public:

        MeshInstance(RigidMesh *mesh, const wchar_t *name, const nv::matrix4f &worldTransform)
            : m_mesh(mesh)
            , m_name(name)
            , m_worldTransform(worldTransform)
        {
        }

        ~MeshInstance() {}

        const wchar_t *getName() const { return m_name.c_str(); }

        void setWorldTransform(const nv::matrix4f &worldTransform) { m_worldTransform = worldTransform; }
        const nv::matrix4f &getWorldTransform() const { return m_worldTransform; }

        nv::vec3f getCenter() const { return m_mesh->getCenter(); }
        nv::vec3f getExtents() const { return m_mesh->getExtents(); }

        nv::vec3f getWorldCenter() const { return nv::vec3f(m_worldTransform * nv::vec4f(getCenter(), 1.0f)); }

        void draw(SceneShader &shader);

        void accumStats(uint64_t &numIndices, uint64_t &numVertices);

    protected:

        RigidMesh *m_mesh;
        std::wstring m_name;
        nv::matrix4f m_worldTransform;		
    };

    // App callback handling
    void configurationCallback(NvGLConfiguration& config);
    void initRendering();
    void updateFrame(float deltaTime);
    void render(float deltaTime);
    void reshape(int32_t width, int32_t height);

    // Light radius access
    void setLightRadiusWorld(float radius);
    float getLightRadiusWorld() const { return m_lightRadiusWorld; }

    // World width offset access
    void changeWorldWidthOffset(float deltaOffset) { m_worldWidthOffset += deltaOffset; }
    float getWorldWidthOffset() const { return m_worldWidthOffset; }

    // World height offset access
    void changeWorldHeightOffset(float deltaOffset) { m_worldHeightOffset += deltaOffset; }
    float getWorldHeightOffset() const { return m_worldHeightOffset; }

    // Shadow technique access
    enum ShadowTechnique
    {
        None = 0,
        PCSS,
        PCF
    };

    void setShadowTechnique(ShadowTechnique technique) { m_shadowTechnique = technique; }
    ShadowTechnique getShadowTechnique() const { return m_shadowTechnique; }

    // Sample pattern access
    enum SamplePattern
    {
        Poisson_25_25 = 0,
        Poisson_32_64,
        Poisson_100_100,
        Poisson_64_128,
        Regular_49_225
    };

    void setPcssSamplePattern(SamplePattern pattern) { m_pcssSamplePattern = pattern; }
    SamplePattern getPcssSamplePattern() const { return m_pcssSamplePattern; }

    void setPcfSamplePattern(SamplePattern pattern) { m_pcfSamplePattern = pattern; }
    SamplePattern getPcfSamplePattern() const { return m_pcfSamplePattern; }
        
    // Use texture
    void useTexture(bool enable) { m_useTexture = enable; }
    bool useTexture() const { return m_useTexture; }

    // Display texture
    void visualizeDepthTexture(bool enable) { m_visualizeDepthTexture = enable; }
    bool visualizeDepthTexture() const { return m_visualizeDepthTexture; }

    // Scene access
    MeshInstance* addMeshInstance(RigidMesh *mesh, const nv::matrix4f &worldTransform, const wchar_t *name);
    void removeMeshInstance(MeshInstance *instance);

    // Scene elements of interest
    void setGroundPlane(float height, float radius)
    {
        m_groundHeight = height;
        m_groundRadius = radius;
    }

    void setKnightMesh(MeshInstance *mesh) { m_knightMesh = mesh; }
    MeshInstance *getKnightMesh() { return m_knightMesh; }

    void setPodiumMesh(MeshInstance *mesh) { m_podiumMesh = mesh; }
    MeshInstance *getPodiumMesh() { return m_podiumMesh; }

    // Stats
    void getSceneStats(uint64_t &numIndices, uint64_t &numVertices, uint32_t &lightResolution) const;

private:

    class SimpleMeshInstance;
    class SdkMeshInstance;

    // Non-copyable
    SoftShadowsRenderer(const SoftShadowsRenderer &);
    SoftShadowsRenderer &operator=(const SoftShadowsRenderer&);

    void initCamera(
        NvCameraXformType::Enum index,
        const nv::vec3f &position,
        const nv::vec3f &focusPoint);

    void updateCamera();
    void updateLightCamera();
    void updateLightCamera(const nv::matrix4f &view);
    void updateLightSize(float frustumWidth, float frustumHeight);

    bool createShadowMap();
    bool createGeometry();
    bool createTextures();
    bool createShaders();
    void relinkShaders();

    void destroyShaders();
    void destroyMeshInstances();

    void drawMeshes(SceneShader &shader);
    void drawGround(SceneShader &shader);

    // ---
    // Scene elements
    typedef std::list<MeshInstance *> MeshInstanceList;
    MeshInstanceList m_meshInstances;
    MeshInstance *m_knightMesh;
    MeshInstance *m_podiumMesh;

    // ---
    // Cameras
    nv::matrix4f m_viewProj;
    nv::matrix4f m_lightViewProj;
    nv::matrix4f m_projection;
    NvInputTransformer& m_camera;      // model viewer camera

    // ---
    // Light parameters
    float m_lightRadiusWorld;

    // ---
    // The shadow map framebuffer
    GLuint m_shadowMapFramebuffer;

    // ---
    // Shaders
    ShadowMapShader    *m_shadowMapShader;
    VisTexShader       *m_visTexShader;
    DepthPrepassShader *m_depthPrepassShader;
    PcssShader         *m_pcssShader;
    
    // ---
    // Ground plane
    float m_groundHeight;
    float m_groundRadius;
    GLuint m_groundVertexBuffer;
    GLuint m_groundIndexBuffer;

    unsigned int m_frameNumber;

    nv::vec4f m_backgroundColor;

    float m_worldHeightOffset;
    float m_worldWidthOffset;

    ShadowTechnique m_shadowTechnique;
    SamplePattern m_pcssSamplePattern;
    SamplePattern m_pcfSamplePattern;
    
    bool m_useTexture;
    bool m_visualizeDepthTexture;

    int32_t m_screenWidth;
    int32_t m_screenHeight;

    static const GLint ShadowDepthTextureUnit = 0;
    static const GLint ShadowPcfTextureUnit = 1;
    static const GLint GroundDiffuseTextureUnit = 2;
    static const GLint GroundNormalTextureUnit = 3;
    static const GLint RockDiffuseTextureUnit = 4;
    static const GLint NumTextureUnits = 5;

    // ---
    // Textures and samplers
    GLuint m_samplers[NumTextureUnits];
    GLuint m_textures[NumTextureUnits];
};

#endif
