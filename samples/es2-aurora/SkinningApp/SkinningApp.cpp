//----------------------------------------------------------------------------------
// File:        es2-aurora\SkinningApp/SkinningApp.cpp
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
#include "SkinningApp.h"

#include "NvUI/NvTweakBar.h"

#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NV/NvLogs.h"

#include "CharacterModel.h"


// This sample demonstrates skinned mesh rendering using a very simple skeleton
// and a procedurally generated animation. It allows rendering of skinned meshes
// with one bone per vertex or two bones per vertex to illustrate the effect.



////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinningApp::draw()
//
//    Performs the actual rendering
//
////////////////////////////////////////////////////////////////////////////////
void SkinningApp::draw(void)
{
    // This function does the actual rendering of the skinned mesh
    GLfloat bones[4 * 4 * 9];

    // Clear the backbuffer
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    // enable our vertex and pixel shader
    m_skinningProgram->enable();

    // Compute and update the ModelViewProjection matrix
    nv::matrix4f temp;
    nv::perspective(m_MVP, 45.0f, (GLfloat)m_width / (GLfloat)m_height, 0.1f, 100.0f);
    m_MVP *= m_transformer->getModelViewMat();
    m_skinningProgram->setUniformMatrix4fv(m_ModelViewProjectionLocation, m_MVP._array, 1, false);

    // Compute and update the bone matrices
    computeBones(m_time, bones);
    m_time += m_timeScalar * getFrameDeltaTime();
    m_skinningProgram->setUniformMatrix4fv(m_BonesLocation, bones, 9, false);

    // Update other uniforms
    m_skinningProgram->setUniform3i(m_RenderModeLocation, (int32_t)m_singleBoneSkinning, (int32_t)m_renderMode, 0);
    m_skinningProgram->setUniform3f(m_LightDir0Location, 0.267f, 0.535f, 0.802f);
    m_skinningProgram->setUniform3f(m_LightDir1Location, -0.408f, 0.816f, -0.408f);

    // Render the mesh
    m_mesh.render(m_iPositionLocation, m_iNormalLocation, m_iWeightsLocation);

    // enable our vertex and pixel shader
    m_skinningProgram->disable();
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinningApp::computeBones()
//
//   This function creates a simple "Look out! the zombies are coming" animation  
//
////////////////////////////////////////////////////////////////////////////////
void SkinningApp::computeBones(float t, float* bones)
{
    // These are the bones in our simple skeleton
    // The indices dictate where the bones exist in the constant buffer
    const int32_t Back        = 0;
    const int32_t UpperLeg_L  = 1;
    const int32_t  LowerLeg_L = 2;
    const int32_t UpperLeg_R  = 3;
    const int32_t  LowerLeg_R = 4;
    const int32_t UpperArm_L  = 5;
    const int32_t  ForeArm_L  = 6;
    const int32_t UpperArm_R  = 7;
    const int32_t  ForeArm_R  = 8;


    // Locations of the heads of the bones in modelspace
    float position_UpperLeg_L[3] = {0.863f,   0.87f, 0.1f};
    float position_LowerLeg_L[3] = {1.621f,  -2.79f, 0.12f};
    float position_UpperLeg_R[3] = {-0.863f,  0.87f, 0.1f};
    float position_LowerLeg_R[3] = {-1.621f, -2.79f, 0.12f};

    float position_UpperArm_L[3] = { 1.70f, 5.81f, 0.3f};
    float position_LowerArm_L[3] = { 5.27f, 5.94f, 0.4f};
    float position_UpperArm_R[3] = {-1.70f, 5.81f, 0.3f};
    float position_LowerArm_R[3] = {-5.27f, 5.94f, 0.4f};


    nv::matrix4f M;
    nv::matrix4f temp;


    // Compute Back bone
    nv::rotationY(M, (float)sin(2.0f * t) * 20.0f * TO_RADIANS);
    copyMatrixToArray(M, &bones[Back * 4 * 4]);    

    // Compute left leg bones (Both UpperLeg_L and LowerLeg_L)
    // Since we're using column vectors, the operations are applied in reverse order as read.
    // That's because if we have M2 * M1 * M0 * v, first M0 transforms v, then M1 transforms (M0 * v)
    // then M2 transforms (M1 * (M0 * v)). So the first transformation applied is by the last/right-most matrix.
    // F: Translate back to model space
    nv::translation(M, position_UpperLeg_L[0], position_UpperLeg_L[1], position_UpperLeg_L[2]);
    // E: Rotate the UpperLeg_L bone
    M *= nv::rotationX(temp, (float)sin(2.0f * t) * 20.0f * TO_RADIANS);
    // D: Translate to head of the UpperLeg_L bone where the rotation occurs
    M *= nv::translation(temp, -position_UpperLeg_L[0], -position_UpperLeg_L[1], -position_UpperLeg_L[2]);
    copyMatrixToArray(M, &bones[UpperLeg_L * 4 * 4]);
    
    // C: Translate back to model space
    M *= nv::translation(temp, position_LowerLeg_L[0], position_LowerLeg_L[1], position_LowerLeg_L[2]);
    // B: Rotate the LowerLeg_L bone
    M *= nv::rotationX(temp, (1.0f + (float)sin(2.0f * t)) * 50.0f * TO_RADIANS);
    // A: Translate to head of the LowerLeg_L bone where the rotation occurs
    M *= nv::translation(temp, -position_LowerLeg_L[0], -position_LowerLeg_L[1], -position_LowerLeg_L[2]);
    copyMatrixToArray(M, &bones[LowerLeg_L * 4 * 4]);
    
    // So if you read the transformations backwards, this is what happens to the UpperLeg_L bone
    // D: Translate to head of the UpperLeg_L bone where the rotation occurs
    // E: Rotate the UpperLeg_L bone
    // F: Translate back to model space
    
    // This is what happens to the LowerLeg_L bone
    // A: Translate to head of the LowerLeg_L bone where the rotation occurs
    // B: Rotate the LowerLeg_L bone
    // C: Translate back to model space
    // D: Translate to head of the UpperLeg_L bone where the rotation occurs
    // E: Rotate the UpperLeg_L bone
    // F: Translate back to model space


        
    // Compute right leg bones (Both UpperLeg_R and LowerLeg_R)
    nv::translation(M, position_UpperLeg_R[0], position_UpperLeg_R[1], position_UpperLeg_R[2]);
    M *= nv::rotationX(temp, (float)sin(2.0f * t + 3.14f) * 20.0f * TO_RADIANS);
    M *= nv::translation(temp, -position_UpperLeg_R[0], -position_UpperLeg_R[1], -position_UpperLeg_R[2]);
    copyMatrixToArray(M, &bones[UpperLeg_R * 4 * 4]);
    
    M *= nv::translation(temp, position_LowerLeg_R[0], position_LowerLeg_R[1], position_LowerLeg_R[2]);
    M *= nv::rotationX(temp, (1.0f + (float)sin(2.0f * t + 3.14f)) * 50.0f * TO_RADIANS);
    M *= nv::translation(temp, -position_LowerLeg_R[0], -position_LowerLeg_R[1], -position_LowerLeg_R[2]);
    copyMatrixToArray(M, &bones[LowerLeg_R * 4 * 4]);


    // Compute left arm bones (Both UpperArm_L and ForeArm_L)
    nv::translation(M, position_UpperArm_L[0], position_UpperArm_L[1], position_UpperArm_L[2]);
    M *= nv::rotationZ(temp, (10.0f + (float)sin(4.0f * t) * 40.0f) * TO_RADIANS);
    M *= nv::translation(temp, -position_UpperArm_L[0], -position_UpperArm_L[1], -position_UpperArm_L[2]);
    copyMatrixToArray(M, &bones[UpperArm_L * 4 * 4]);

    M *= nv::translation(temp, position_LowerArm_L[0], position_LowerArm_L[1], position_LowerArm_L[2]);
    M *= nv::rotationX(temp, (-30.0f + -(1.0f + (float)sin(4.0f * t)) * 30.0f) * TO_RADIANS);
    M *= nv::translation(temp, -position_LowerArm_L[0], -position_LowerArm_L[1], -position_LowerArm_L[2]);
    copyMatrixToArray(M, &bones[ForeArm_L * 4 * 4]);


    // Compute right arm bones (Both UpperArm_R and ForeArm_R)
    nv::translation(M, position_UpperArm_R[0], position_UpperArm_R[1], position_UpperArm_R[2]);
    M *= nv::rotationZ(temp, (-(10.0f + (float)sin(4.0f * t) * 40.0f)) * TO_RADIANS);
    M *= nv::translation(temp, -position_UpperArm_R[0], -position_UpperArm_R[1], -position_UpperArm_R[2]);
    copyMatrixToArray(M, &bones[UpperArm_R * 4 * 4]);

    M *= nv::translation(temp, position_LowerArm_R[0], position_LowerArm_R[1], position_LowerArm_R[2]);
    M *= nv::rotationX(temp,  (-30.0f + -(1.0f + (float)sin(4.0f * t)) * 30.0f) * TO_RADIANS);
    M *= nv::translation(temp, -position_LowerArm_R[0], -position_LowerArm_R[1], -position_LowerArm_R[2]);
    copyMatrixToArray(M, &bones[ForeArm_R * 4 * 4]);    
}





////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinningApp::initRendering()
//
//    Sets up initial rendering state and creates the skinned character mesh
//
////////////////////////////////////////////////////////////////////////////////
void SkinningApp::initRendering(void) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    

    NvAssetLoaderAddSearchPath("es2-aurora/SkinningApp");

    m_mesh.m_useES2 = getGLContext()->getConfiguration().apiVer == NvGLAPIVersionES2();

    // Initialize the mesh
    int32_t vertexCount = sizeof(g_characterModelVertices) / (10*sizeof(float)); // 3x pos, 3x norm, (2+2)x bone
    int32_t indexCount = sizeof(g_characterModelIndices) / (sizeof(g_characterModelIndices[0]));

    // Convert the float data in the vertex array to half data
    // On Android, this may have already been converted during a previous run and
    // kept in-core.  We MUST skip this step in that case.
    if (!g_convertedToSkinnedVertex) {
        int32_t index = 0;
        for(int32_t i=0; i<vertexCount; i++)
        {
            SkinnedVertex& v = ((SkinnedVertex*)g_characterModelVertices)[i];
            v.m_position[0] = half(g_characterModelVertices[index++]);
            v.m_position[1] = half(g_characterModelVertices[index++]);
            v.m_position[2] = half(g_characterModelVertices[index++]);
            v.m_normal[0] = half(g_characterModelVertices[index++]);
            v.m_normal[1] = half(g_characterModelVertices[index++]);
            v.m_normal[2] = half(g_characterModelVertices[index++]);
            v.m_weights[0] = half(g_characterModelVertices[index++]);
            v.m_weights[1] = half(g_characterModelVertices[index++]);
            v.m_weights[2] = half(g_characterModelVertices[index++]);
            v.m_weights[3] = half(g_characterModelVertices[index++]);
        }
        g_convertedToSkinnedVertex = true;
    }

    // Stick the half float data into the mesh
    m_mesh.update(reinterpret_cast<const SkinnedVertex*>(g_characterModelVertices), vertexCount, g_characterModelIndices, indexCount);      


    //
    // Initialize the shaders
    //

    m_skinningProgram = NvGLSLProgram::createFromFiles("shaders/skinning.vert", "shaders/simple.frag");

    // Get the locations of the uniforms
    m_ModelViewProjectionLocation = m_skinningProgram->getUniformLocation("ModelViewProjection");
    m_BonesLocation = m_skinningProgram->getUniformLocation("Bones");
    m_RenderModeLocation = m_skinningProgram->getUniformLocation("RenderMode");
    m_LightDir0Location = m_skinningProgram->getUniformLocation("LightDir0");
    m_LightDir1Location = m_skinningProgram->getUniformLocation("LightDir1");

    // Get the locations of the attributes
    m_iPositionLocation = m_skinningProgram->getAttribLocation("iPosition");
    m_iNormalLocation = m_skinningProgram->getAttribLocation("iNormal");
    m_iWeightsLocation = m_skinningProgram->getAttribLocation("iWeights");

    // Initialize some view parameters
    m_transformer->setRotationVec(nv::vec3f(0.0f, NV_PI*0.25f, 0.0f));
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -25.0f));
    m_transformer->setMaxTranslationVel(50.0f); // seems to work decently.

    CHECK_GL_ERROR();
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinningApp::SkinningApp()
//
////////////////////////////////////////////////////////////////////////////////
SkinningApp::SkinningApp()
    : m_skinningProgram(NULL)
    , m_singleBoneSkinning(false)
    , m_pauseTime(false)
    , m_pausedByPerfHUD(false)
    , m_time(0.0f)
    , m_renderMode(0)        // 0: Render Color   1: Render Normals    2: Render Weights
    , m_ModelViewProjectionLocation(0)
    , m_BonesLocation(0)
    , m_RenderModeLocation(0)
    , m_LightDir0Location(0)
    , m_LightDir1Location(0)
    , m_iPositionLocation(0)
    , m_iNormalLocation(0)
    , m_iWeightsLocation(0)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}


////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinningApp::~SkinningApp()
//
////////////////////////////////////////////////////////////////////////////////
SkinningApp::~SkinningApp()
{
    LOGI("SkinningApp: destroyed\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinningApp::initUI()
//
//    Sets up the user interface
//
////////////////////////////////////////////////////////////////////////////////
void SkinningApp::initUI() {
    if (mTweakBar) {
        NvTweakVarBase *var;
        // expose the rendering modes
        NvTweakEnum<uint32_t> renderModes[] = {
            {"Color", 0},
            {"Normals", 1},
            {"Weights", 2}
        };
        var = mTweakBar->addEnum("Render Mode", m_renderMode, renderModes, TWEAKENUM_ARRAYSIZE(renderModes));
        addTweakKeyBind(var, NvKey::K_M);
        addTweakButtonBind(var, NvGamepad::BUTTON_Y);

        mTweakBar->addPadding();
        var = mTweakBar->addValue("Single Bone Skinning", m_singleBoneSkinning);
        addTweakKeyBind(var, NvKey::K_B);
        addTweakButtonBind(var, NvGamepad::BUTTON_X);

        mTweakBar->addPadding();
        m_timeScalar = 1.0f;
        var = mTweakBar->addValue("Animation Speed", m_timeScalar, 0, 5.0, 0.1f);
        addTweakKeyBind(var, NvKey::K_RBRACKET, NvKey::K_LBRACKET);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER, NvGamepad::BUTTON_LEFT_SHOULDER);
    }

    // Change the filtering for the framerate
    mFramerate->setMaxReportRate(.2f);
    mFramerate->setReportFrames(20);

    // Disable wait for vsync
    getGLContext()->setSwapInterval(0);  
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinningApp::reshape()
//
//    Called when the viewport dimensions are changed
//
////////////////////////////////////////////////////////////////////////////////
void SkinningApp::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );

    CHECK_GL_ERROR();
}




////////////////////////////////////////////////////////////////////////////////
//
//  Miscellaneous stuff
//
////////////////////////////////////////////////////////////////////////////////
void SkinningApp::copyMatrixToArray(nv::matrix4f& M, float* dest)
{
    for(int32_t i=0; i<16; i++)
    {
        dest[i] = M._array[i];
    }
}



void SkinningApp::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionES2();
}



NvAppBase* NvAppFactory() 
{
    return new SkinningApp();
}
