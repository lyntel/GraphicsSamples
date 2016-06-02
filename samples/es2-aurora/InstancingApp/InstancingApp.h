//----------------------------------------------------------------------------------
// File:        es2-aurora\InstancingApp/InstancingApp.h
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
#include "NV/NvMath.h"
#include "NV/NvPlatformGL.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvGamepad/NvGamepad.h"
#include "NvGLUtils/NvGLSLProgram.h"

class NvStopWatch;
class NvFramerateCounter;
class NvModelGL;
class NvTweakBar;

class InstancingApp : public NvSampleAppGL
{
public:
    InstancingApp();
    ~InstancingApp();
    
    void initRendering(void);
    void initUI(void);
    void draw(void);
    void reshape(int32_t width, int32_t height);

    void configurationCallback(NvGLConfiguration& config);

private:
    void loadModelFromFile(const char *pFileName, int32_t modelNum );
    void initShaders();
    bool configTexture(GLuint texID, int32_t index);
    void initSceneColorPalette( int32_t sceneIndex );
    void initPerInstanceRotations( int32_t sceneIndex, float* pHwInstanceData );
    void initPerInstancePositions( int32_t sceneIndex, float* pHwInstanceData );
    void initModelCopies( int32_t sceneIndex, float* pV, int32_t* pI, NvModelGL* pMdl );
    void initGLObjects( int32_t sceneIndex, float* pV, int32_t* pI, NvModelGL* pMdl );
    bool initSceneInstancingData( int32_t sceneIndex );
    void drawModelLit();

#ifdef ANDROID
    const static int32_t GRID_SIZE = 30;
#else
    const static int32_t GRID_SIZE = 65;
#endif
    const static uint32_t INVALID_ID = 0xffffffff;
    const static int32_t MAX_INSTANCES = 100;
    const static int32_t MAX_OBJECTS = (GRID_SIZE*GRID_SIZE*GRID_SIZE);

    enum { BOXES_SCENE = 0, GRASS_SCENE = 1, NUMSCENES = 2 };
    enum { NO_INSTANCING, SHADER_INSTANCING, HARDWARE_INSTANCING };

    uint32_t m_vboID[NUMSCENES];
    uint32_t m_iboID[NUMSCENES];
    float m_instanceOffsets[NUMSCENES][3*MAX_OBJECTS];  // contains the object positions (3 floats) for 2 scenes
    float m_instanceRotation[NUMSCENES][3*MAX_OBJECTS]; // contains object rotations for 2 scene .z = color index
    float m_instanceColor[NUMSCENES][3*6]; // 6 colors per scene
    bool  m_hwInstancing;
    float m_time;
    bool  m_bUseNVExtension;

    uint32_t  m_instancingOptions;
    uint32_t  m_sceneIndex;
    uint32_t  m_instanceCount;
    
    NvGLSLProgram* m_shaders[NUMSCENES*2];
    uint32_t m_textureIDs[NUMSCENES*2];

    // Shader uniform handles
    // there are two scene in the sample 'cubes' and 'grass'
    // if no hardware instancing is used/supported
    // index 0 is used by the gl program and it's uniforms for 'cubes'
    // index 1 is used by the gl program and it's uniforms for 'grass'
    // if hardware instancing is used
    // index 2 is used by the gl program and it's uniforms for 'cubes'
    // index 3 is used by the gl program and it's uniforms for 'grass'
    GLuint m_positionHandle[NUMSCENES*2];
    GLuint m_instanceOffsetHandle[NUMSCENES*2];   // for hw instancing this handle points to vertex buffer attributes
    GLuint m_instanceRotationHandle[NUMSCENES*2]; // for hw instancing this handle points to vertex buffer attributes
    GLuint m_instanceColorsHandle[NUMSCENES*2];
    GLuint m_normalHandle[NUMSCENES*2];
    GLuint m_texCoordHandle[NUMSCENES*2];
    GLuint m_modelViewMatrixHandle[NUMSCENES*2];
    GLuint m_timeHandle[NUMSCENES*2];
    GLuint m_projectionMatrixHandle[NUMSCENES*2];
    GLuint m_texHandle[NUMSCENES*2];

    NvModelGL* m_pModel[NUMSCENES];

    //matrices
    nv::matrix4f m_projectionMatrix;
    nv::matrix4f m_viewMatrix;
    nv::matrix4f m_normalMatrix;
    nv::matrix4f m_ortho2DProjectionMatrixScreen;
};