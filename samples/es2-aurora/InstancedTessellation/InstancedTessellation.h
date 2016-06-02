//----------------------------------------------------------------------------------
// File:        es2-aurora\InstancedTessellation/InstancedTessellation.h
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

#include <NvSimpleTypes.h>
#include "KHR/khrplatform.h"
#include "NV/NvMath.h"
#include "NvGamepad/NvGamepad.h"
#include "NvGLUtils/NvGLSLProgram.h"



class NvStopWatch;
class NvFramerateCounter;
class NvModelGL;
class NvTweakBar;

#define MAX_TESS_LEVEL 6

enum Models {
    eBuddha,
    eArmadillo,
    eDragon,
    eNumModels
};

enum ETessellationMode {
    eNPatch,
    eFirstTessellationMode = eNPatch,
    eDeformation,
    eNumTessellationModes
};

struct PerInstanceData
{
    nv::vec3f m_p0;      // texcoord 0 - triangle vertex 0
    nv::vec3f m_p1;      // texcoord 1 - triangle vertex 1
    nv::vec3f m_p2;      // texcoord 2 - triangle vertex 2
    nv::vec3f m_n0;      // texcoord 3 - triangle vertex normal 0
    nv::vec3f m_n1;      // texcoord 4 - triangle vertex normal 1
    nv::vec3f m_n2;      // texcoord 5 - triangle vertex normal 2
    //nv::vec4f m_t0t1;    // texcoord 6 - triangle vertex texture coord 0-1
    //nv::vec2f m_t2;      // texcoord 7 - triangle vertex texture coord 2
};

#define INVALID_ID 0xffffffff

// USER DEFINED FUNCITONS

class BaseShader : public NvGLSLProgram
{
public:
    BaseShader()
    {
    }
    virtual ~BaseShader()
    {
    }

    void initShader(const char *pVertexProgramPath, const char *pFragmentProgramPath)
    {
        setSourceFromFiles(pVertexProgramPath, pFragmentProgramPath);
        initShaderParameters();
    }

    virtual void initShaderParameters()
    {
        // vertex attributes
        m_b01Handle                = getAttribLocation("a_v2B01");

        // transformation matrices
        m_lightPositionHandle1   = getUniformLocation("g_v4LightPosition1");
        m_modelViewMatrixHandle  = getUniformLocation("g_m4x4ModelViewMatrix");
        m_projectionMatrixHandle = getUniformLocation("g_m4x4ProjectionMatrix");
        m_normalMatrixHandle     = getUniformLocation("g_m4x4NormalMatrix");
        m_colorHandle            = getUniformLocation("g_v4Color");
    }

    GLint m_b01Handle;
    GLint m_modelViewMatrixHandle;
    GLint m_projectionMatrixHandle;
    GLint m_normalMatrixHandle;
    GLint m_colorHandle;
    GLint m_lightPositionHandle1; //specifies direction... directional(w=0.0f) point(w=1.0f);
};

class TessShader : public BaseShader
{
public:
    TessShader() {
        initShader("shaders/npatch.vert", "shaders/lighting.frag");
    }

    virtual void initShaderParameters()
    {
        BaseShader::initShaderParameters();

        m_lightAmbientHandle1    = getUniformLocation("g_fLightAmbient1");
        m_lightDiffuseHandle1    = getUniformLocation("g_fLightDiffuse1");

        m_posHandle                = getUniformLocation("g_v3Pos" );
        m_normHandle                = getUniformLocation("g_v3Norm");
        //m_texHandle                 = getUniformLocation(m_programId, "g_v2Tex" );
    }

    GLint m_posHandle;
    GLint m_normHandle;
    //GLint m_texHandle;
    GLint m_lightDiffuseHandle1;
    GLint m_lightAmbientHandle1;
};

struct DeformShader : public TessShader
{
public:
    DeformShader() {
        initShader("shaders/deform.vert", "shaders/lighting.frag");
    }

    virtual void initShaderParameters()
    {
        TessShader::initShaderParameters();

        m_timeHandle    = getUniformLocation("g_fTime");
    }

    GLint m_timeHandle;
};

struct InstancedTessShader : BaseShader
{
    InstancedTessShader() {
        initShader("shaders/npatch_instanced.vert", "shaders/lighting.frag");
    }

    virtual void initShaderParameters()
    {
        BaseShader::initShaderParameters();

        m_posHandle0                = getAttribLocation("a_v3Pos0" );
        m_normHandle0             = getAttribLocation("a_v3Norm0");
        //m_tex0Handle             = getAttribLocation("a_v2Tex0");

        m_posHandle1                = getAttribLocation("a_v3Pos1" );
        m_normHandle1             = getAttribLocation("a_v3Norm1");
        //m_tex1Handle             = getAttribLocation("a_v2Tex1");

        m_posHandle2                = getAttribLocation("a_v3Pos2" );
        m_normHandle2             = getAttribLocation("a_v3Norm2");
        //m_tex2Handke             = getAttribLocation("a_v2Tex2");

        m_lightAmbientHandle1     = getUniformLocation("g_fLightAmbient1");
        m_lightDiffuseHandle1     = getUniformLocation("g_fLightDiffuse1");
    }

    GLint m_posHandle0,  m_posHandle1, m_posHandle2;
    GLint m_normHandle0, m_normHandle1, m_normHandle2;
    //GLint m_tex0Handle,  m_tex1Handle,  m_tex2Handle;
    GLint m_lightDiffuseHandle1;
    GLint m_lightAmbientHandle1;
};

struct InstancedDeformShader : InstancedTessShader
{
    InstancedDeformShader() {
        initShader("shaders/deform_instanced.vert", "shaders/lighting.frag");
    }

    virtual void initShaderParameters()
    {
        InstancedTessShader::initShaderParameters();

        m_timeHandle    = getUniformLocation("g_fTime");
    }

    GLint m_timeHandle;
};


class InstancedTessellation : public NvSampleAppGL
{
public:
    InstancedTessellation();
    ~InstancedTessellation();
    
    void initRendering(void);
    void initUI(void);
    void draw(void);
    void reshape(int32_t width, int32_t height);

    bool handleGamepadChanged(uint32_t changedPadFlags);

    void configurationCallback(NvGLConfiguration& config);

private:
   void drawModelLitInstancingON();
   void drawModelLitInstancingOFF();
   void drawModelLit();
   void initShaders();
   void initGlobalVariables();
   void loadModelFromData  (int32_t modelIndex, char *pFileData);
   bool initGeneralTessellationInstancingData();
   bool initPerModelTessellationInstancingData( NvModelGL* mdl, int32_t modelIndex );

    uint32_t m_wireframe;
    uint32_t m_instancing;

    nv::matrix4f m_projectionMatrixHandle;
    nv::matrix4f m_viewMatrix;
    nv::matrix4f m_normalMatrixHandle;

    TessShader* m_npatchShader;
    InstancedTessShader* m_instancedNPatchShader;
    DeformShader* m_deformationShader;
    InstancedDeformShader* m_instancedDeformationShader;
    nv::vec4f m_lightPositionEye;
    nv::vec3f m_lightPosition;
    float m_lightType; //1.0f = point light / 0.0f = directional light

    float m_floorPlane[4];
    NvModelGL *m_pModel[ eNumModels ];
   nv::matrix4f m_ModelMatrix[ eNumModels ];
    PerInstanceData* m_pTriangleData[ eNumModels ];
    nv::vec3f m_modelColor;
    uint32_t   m_tessFactor;
   float      m_time;

    //light attributes
    float m_lightAmbient; //ambient component
    float m_lightDiffuse; //diffuse component

    GLuint m_perTriangleDataVboID[ eNumModels ];
    GLuint m_tessVboID[ MAX_TESS_LEVEL ];
    GLuint m_tessIboID[ MAX_TESS_LEVEL ];
    GLuint m_wireframeTessIboID[ MAX_TESS_LEVEL ];

    bool   m_hwInstancing;
    bool   m_pausedByPerfHUD;
    uint32_t    m_modelIndex;
    uint32_t    m_tessMode;
};
