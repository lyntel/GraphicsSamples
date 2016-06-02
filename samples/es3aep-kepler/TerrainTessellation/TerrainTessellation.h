//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation/TerrainTessellation.h
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

//define to make sure the header understands the compilation mode
#define CPP

// now just include the header all the shader use
#include "NV/NvMath.h"
#include "NV/NvShaderMappings.h"
#include "uniforms.h"

class NvGLSLProgram;
class NvSimpleFBO;

class TerrainTessellation : public NvSampleAppGL
{
public:
    TerrainTessellation();
    ~TerrainTessellation();
    
    void initRendering(void);
    void initUI(void);
    NvUIEventResponse handleReaction(const NvUIReaction& react);
    void draw(void);
    void reshape(int32_t width, int32_t height);

    void configurationCallback(NvGLConfiguration& config);

private:
    GLuint createShaderPipelineProgram(GLuint target, const char* src);
    void loadShaders();
    void initTerrainFbo();
    void updateTerrainTex();
    void computeFrustumPlanes(nv::matrix4f &viewMatrix, nv::matrix4f &projMatrix, nv::vec4f *plane);
    bool sphereInFrustum(nv::vec3f pos, float r, nv::vec4f *plane);
    void drawTerrainVBO();
    void drawTerrain();
    void drawQuad(float z);
    void drawSky();
    void updateQuality();

    // Terrain program stages
    GLuint mTerrainPipeline;

    GLuint mTerrainVertexProg;
    GLuint mTerrainTessControlProg;
    GLuint mTerrainTessEvalProg;
    GLuint mWireframeGeometryProg;
    GLuint mTerrainFragmentProg;
    GLuint mWireframeFragmentProg;

    GLuint mGPUQuery;
    GLuint mNumPrimitives;

    // declare a global struct that holds the uniform data
    TessellationParams mParams;

    NvGLSLProgram* mSkyProg;
    NvGLSLProgram *mGenerateTerrainProg;

    //uniform buffer object
    GLuint mUBO;
    //vertex buffer object
    GLuint mVBO;

    GLuint mRandTex;
    GLuint mRandTex3D;

    NvSimpleFBO *mTerrainFbo;

    nv::vec3f mLightDir;

    GLuint mTerrainVbo;
    GLuint mTerrainIbo;

    uint32_t mQuality;
    bool mCull;
    bool mLod;
    bool mWireframe;
    bool mAnimate;
    float mHeightScale;
    bool mReload;
    float mTime;

    NvUIValueText *mStatsText;
};
