//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/TextureArrayTerrain.h
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

#include "NV/NvMath.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvGamepad/NvGamepad.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "TerrainSim.h"

class NvStopWatch;
class NvFramerateCounter;

class TerrainGenerator;

class TextureArrayTerrain : public NvSampleAppGL
{
public:
    TextureArrayTerrain();
    ~TextureArrayTerrain();
    
    void initUI(void);
    void initRendering(void);
    void shutdownRendering(void);
    void draw(void);
    void reshape(int32_t width, int32_t height);

    void configurationCallback(NvGLConfiguration& config);

private:
    class BaseShader : public NvGLSLProgram
    {
    public:
        BaseShader(const char *vertexProgramPath, const char *fragmentProgramPath);
        virtual ~BaseShader() {}
        void initShaderParameters();

        GLint m_positionAttrHandle;
        GLint m_projectionMatrixHandle;

    private:
        virtual void derivedInitShaderParameters()    {}
    };

    class SkyShader : public BaseShader
    {
    public:
        SkyShader();

        virtual void derivedInitShaderParameters();
        GLint m_inverseViewMatrixHandle;
        GLint m_skyTexCubeHandle;
    };

    class TerrainShader : public BaseShader
    {
    public:
        TerrainShader( const char *vertexProgramPath,
            const char *fragmentProgramPath );

        virtual void derivedInitShaderParameters();
        GLint m_normalAndHeightAttrHandle;
        GLint m_terrainTexHandle;
        GLint m_modelViewMatrixHandle;
        GLint m_interpOffsetHandle;
    };
    
    void createShaders();
    void destroyShaders();
    void updateBufferData();
    void initTerrainSurfaces(int32_t w, int32_t h, int32_t numTiles);
    void deleteTerrainSurfaces();
    void drawSkyBox(const nv::matrix4f& invViewMatrix);
    void drawTerrainSurfaces();
    void renderTerrainSurfaces();

    bool m_showOptions;

    nv::matrix4f m_projectionMatrix;
    nv::matrix4f m_inverseProjMatrix;
    nv::matrix4f m_viewMatrix;
    nv::matrix4f m_normalMatrix;

    TerrainShader*        m_pTerrainShader;
    SkyShader*            m_pSkyShader;

    nv::vec4f m_lightPositionEye;
    nv::vec3f m_lightPosition;

    TerrainSim::Params m_simParams;

    bool m_pausedByPerfHUD;

    TerrainGenerator **m_ppTerrain;

    GLuint m_SkyTexture;
    GLuint m_TerrainTexture;

    const static int32_t NUM_TILES = 9;

    bool m_paused;
};
