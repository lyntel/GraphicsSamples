//----------------------------------------------------------------------------------
// File:        gl4-kepler\MultiDrawIndirect/MultiDrawIndirect.cpp
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
#include "MultiDrawIndirect.h"
#include "NvUI/NvTweakBar.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NV/NvLogs.h"

#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvImageGL.h"
#include "NvGLUtils/NvModelGL.h"
#include "NvModel/NvModel.h"
#include "KHR/khrplatform.h"

#define GPU_TIMER_SCOPE() NvGPUTimerScope gpuTimer(&m_GPUTimer)
#define CPU_TIMER_SCOPE() NvCPUTimerScope cpuTimer(&m_CPUTimer)

#define VERTEX_BUFFER_COUNT 1

#define MAX_MODEL_INSTANCES 20
#define INSTANCE_SEPERATION 10.0f

#define DEFAULT_GRID_SIZE 100
#define MIN_GRID_SIZE 1
#define MAX_GRID_SIZE 200

#define DEFAULT_MOBILE_GRID_SIZE 100
#define MIN_MOBILE_GRID_SIZE 1
#define MAX_MOBILE_GRID_SIZE 200

#define POSITION_STREAM     0
#define NORMAL_STREAM       1
#define UV_STREAM           2

#define POSITION_OFFSET         0
#define NORMAL_OFFSET           3 * sizeof(float)
#define UV_OFFSET               6 * sizeof(float)

#define MODEL_TO_LOAD_1 "models/house.obj"
#define OFFSET(n) ((char *)NULL + (n))

enum {USE_MULTIDRAWINDIRECT, NO_MULTIDRAWINDIRECT};

static const float SKY_QUAD_COORDS[] = { -1.0f, -1.0f, -1.0f, 1.0f,
                                          1.0f, -1.0f, -1.0f, 1.0f,
                                         -1.0f,  1.0f, -1.0f, 1.0f,
                                          1.0f,  1.0f, -1.0f, 1.0f};

class BaseShader : public NvGLSLProgram
{
    public:
        BaseShader( const char *vertexProgramPath, 
                    const char *fragmentProgramPath) : m_PositionAHandle(-1)
        {
            setSourceFromFiles(vertexProgramPath, fragmentProgramPath);

            m_PositionAHandle = getAttribLocation("a_vPosition");
        }

        GLint   m_PositionAHandle;
};

class BaseProjectionShader : public BaseShader
{
    public:
        BaseProjectionShader(   const char *vertexProgramPath,
                                const char *fragmentProgramPath) :  BaseShader(vertexProgramPath, fragmentProgramPath),
                                                                    m_ProjectionMatUHandle(-1)
        {
            m_ProjectionMatUHandle = getUniformLocation("u_mProjectionMat");
        }

        GLint   m_ProjectionMatUHandle;
};

class BaseProjNormalShader : public BaseProjectionShader
{
    public:
        BaseProjNormalShader(const char *vertexProgramPath,
                             const char *fragmentProgramPath) : BaseProjectionShader(vertexProgramPath, fragmentProgramPath),
                                                                m_NormalAHandle(-1),
                                                                m_NormalMatUHandle(-1)
        {
            m_NormalAHandle    = getAttribLocation("a_vNormal");

            m_NormalMatUHandle = getUniformLocation("u_mNormalMat");
        }

        GLint   m_NormalAHandle;
        GLint   m_NormalMatUHandle;
};

class SceneShader : public BaseProjNormalShader
{
    public:
        SceneShader(const char* pVert, const char* pFrag) : BaseProjNormalShader(pVert, pFrag),
                            m_TexcoordAHandle(-1),
                            m_InstanceAHandle(-1),
                            m_ModelViewMatUHandle(-1),
                            m_LightPositionUHandle(-1),
                            m_LightAmbientUHandle(-1),
                            m_LightDiffuseUHandle(-1),
                            m_LightSpecularUHandle(-1),
                            m_LightShininessUHandle(-1),
                            m_DiffuseTexUHandle(-1),
                            m_PositionUHandle(-1)
        {
            m_TexcoordAHandle       = getAttribLocation("a_vTexCoord");
            m_InstanceAHandle       = getAttribLocation("a_vInstance");
            m_ModelViewMatUHandle   = getUniformLocation("u_mModelViewMat");
            m_LightPositionUHandle  = getUniformLocation("u_vLightPosition");
            m_LightAmbientUHandle   = getUniformLocation("u_fLightAmbient");
            m_LightDiffuseUHandle   = getUniformLocation("u_fLightDiffuse");
            m_LightSpecularUHandle  = getUniformLocation("u_fLightSpecular");
            m_LightShininessUHandle = getUniformLocation("u_fLightShininess");
            m_DiffuseTexUHandle     = getUniformLocation("u_tDiffuseTex");
            m_PositionUHandle     = getUniformLocation("u_vPosition");
        }

        GLint   m_TexcoordAHandle;
        GLint   m_InstanceAHandle;
        GLint   m_ModelViewMatUHandle;
        GLint   m_LightPositionUHandle;
        GLint   m_LightAmbientUHandle;
        GLint   m_LightDiffuseUHandle;
        GLint   m_LightSpecularUHandle;
        GLint   m_LightShininessUHandle;
        GLint   m_DiffuseTexUHandle;
        GLint   m_PositionUHandle;
};

class SkyboxShader : public BaseProjectionShader
{
    public:
        SkyboxShader(void) :    BaseProjectionShader(   "shaders/skyboxcolor.vert",
                                                        "shaders/skyboxcolor.frag"),
                                                        m_InverseViewMatUHandle(-1),
                                                        m_SkyboxCubemapTexUHandle(-1)
        {
            m_InverseViewMatUHandle    = getUniformLocation("u_mInverseViewMat");
            m_SkyboxCubemapTexUHandle  = getUniformLocation("u_tSkyboxCubemapTex");
        }

        GLint   m_InverseViewMatUHandle;
        GLint   m_SkyboxCubemapTexUHandle;
};

MultiDrawIndirect::MultiDrawIndirect() : NvSampleAppGL()
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();

    m_SkyBoxTextureID = m_WindmillTextureID = 0;

    m_SceneShader = NULL;
    m_SceneShaderMDI = NULL;

    m_SkyboxShader = NULL;

    m_Model = NULL;

    m_transformer->setMotionMode(NvCameraMotionType::FIRST_PERSON);

    m_ProjectionMatrixChanged = true;

    m_VertexArrayObject = 0;

    m_IndirectDrawBuffer = 0;

    m_DrawInstanceMode = USE_MULTIDRAWINDIRECT;

    m_statsFrames = 0;

    m_MaxGridSize = m_MaxModelInstances = 0;

    m_Model = NULL;
}

MultiDrawIndirect::~MultiDrawIndirect()
{
    LOGI("MultiDrawIndirect: destroyed\n");
}

void MultiDrawIndirect::GetGridPoints(  unsigned int        j,
                                        std::vector<float>& Offsets)
{
    bool Loop;
    unsigned int Points, PointsDrawnUp, PointsDrawnAcross;

    Points = 1 + j;
    PointsDrawnUp = PointsDrawnAcross = 0;

    Loop = true;

    while (Loop)
    {
        Offsets.push_back(PointsDrawnAcross * INSTANCE_SEPERATION);
        Offsets.push_back(j * INSTANCE_SEPERATION);

        PointsDrawnAcross++;

        if (PointsDrawnAcross == Points)
        {
            Loop = false;

            continue;
        }

        Offsets.push_back(j * INSTANCE_SEPERATION);
        Offsets.push_back(PointsDrawnUp * INSTANCE_SEPERATION);

        PointsDrawnUp++;
    }
}

void MultiDrawIndirect::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

void MultiDrawIndirect::FreeGLBindings(void) const
{
    glBindBuffer(GL_ARRAY_BUFFER,         0); 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D,          0);
    glBindTexture(GL_TEXTURE_CUBE_MAP,    0);
    CHECK_GL_ERROR();
}

void MultiDrawIndirect::shutdownRendering(void)
{
    // If we still have a bound context, then it makes sense to delete GL resources

    if (getPlatformContext()->isContextBound())
    {
        CleanRendering();
    }
}

void MultiDrawIndirect::CleanRendering(void)
{
    if (m_SceneShader)
    {
        delete m_SceneShader;

        m_SceneShader = NULL;
    }

    if (m_SceneShaderMDI)
    {
        delete m_SceneShaderMDI;

        m_SceneShaderMDI = NULL;
    }

    if (m_SkyboxShader)
    {
        delete m_SkyboxShader;

        m_SkyboxShader = NULL;
    }

    if (m_WindmillTextureID != 0)
    {
        glDeleteTextures(1, &m_WindmillTextureID);
        CHECK_GL_ERROR();

        m_WindmillTextureID = 0;
    }

    if (m_SkyBoxTextureID != 0)
    {
        glDeleteTextures(1, &m_SkyBoxTextureID);
        CHECK_GL_ERROR();

        m_SkyBoxTextureID = 0;
     }

    if (m_VertexArrayObject != 0)
    {
        glDeleteVertexArrays(1, &m_VertexArrayObject);
        CHECK_GL_ERROR();

        m_VertexArrayObject = 0;
    }

    if (m_IndirectDrawBuffer != 0)
    {
        glDeleteBuffers(1, &m_IndirectDrawBuffer);
        CHECK_GL_ERROR();

        m_IndirectDrawBuffer = 0;
    }

    if (m_Model)
    {
        delete m_Model;

        m_Model = NULL;
    }
}

void MultiDrawIndirect::initRendering(void)
{
    bool MultidrawAvailable;

    m_GPUTimer.init();
    m_CPUTimer.init();

    MultidrawAvailable = requireExtension("GL_ARB_multi_draw_indirect", false);

    if (MultidrawAvailable == false)
    {
        errorExit("The current system does not appear to support the extension GL_ARB_multi_draw_indirect, which is required by the sample.\n\nThis is likely because the systems GPU or driver does not support the extension. Please see the samples source code for details");

        return;
    }

    if (isMobilePlatform())
    {
        m_GridSize = DEFAULT_MOBILE_GRID_SIZE;
        m_MaxGridSize = MAX_MOBILE_GRID_SIZE;
        m_MaxModelInstances = MAX_MODEL_INSTANCES;
    }
    else
    {
        m_GridSize = DEFAULT_GRID_SIZE;
        m_MaxGridSize = MAX_GRID_SIZE;
        m_MaxModelInstances = MAX_MODEL_INSTANCES;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    NvAssetLoaderAddSearchPath("gl4-kepler/MultiDrawIndirect");

    CleanRendering();

    Startup();
}

void MultiDrawIndirect::DrawScreenAlignedQuad(const unsigned int attrHandle) const
{
    glVertexAttribPointer(attrHandle, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), SKY_QUAD_COORDS);

    glEnableVertexAttribArray(attrHandle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(attrHandle);

    CHECK_GL_ERROR();
}

void MultiDrawIndirect::DrawSkyboxColorDepth(void) const
{
    // Skybox computations use the camera matrix (inverse of the view matrix)
    const nv::matrix4f invVM = nv::inverse(m_CurrentViewMatrix);

    m_SkyboxShader->enable();

    if (m_ProjectionMatrixChanged == true)
    {
        glUniformMatrix4fv(m_SkyboxShader->m_ProjectionMatUHandle, 1, GL_FALSE, m_InverseProjMatrix._array);
    }

    glUniformMatrix4fv(m_SkyboxShader->m_InverseViewMatUHandle, 1, GL_FALSE, invVM._array);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyBoxTextureID);
    glUniform1i(m_SkyboxShader->m_SkyboxCubemapTexUHandle, 0);

    DrawScreenAlignedQuad(m_SkyboxShader->m_PositionAHandle);

    m_SkyboxShader->disable();
}

void MultiDrawIndirect::DrawModelColorDepth()
{
    const nv::matrix4f mv = m_CurrentViewMatrix;

    //NOTE: NormalMatrix now contains a 3x3 inverse component of the
    //      ModelMatrix; hence, we need to transpose this before sending it to
    //      the shader

    const nv::matrix4f n = nv::transpose(nv::inverse(mv));

    SceneShader* pShader = (m_DrawInstanceMode == NO_MULTIDRAWINDIRECT) ? m_SceneShader : m_SceneShaderMDI;

    pShader->enable();

    glUniformMatrix4fv(pShader->m_ProjectionMatUHandle, 1, GL_FALSE, m_ProjectionMatrix._array);
    glUniformMatrix4fv(pShader->m_NormalMatUHandle, 1, GL_FALSE, n._array);
    glUniformMatrix4fv(pShader->m_ModelViewMatUHandle, 1, GL_FALSE, mv._array);

    if (m_DrawInstanceMode == NO_MULTIDRAWINDIRECT)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_WindmillTextureID);
        glUniform1i(pShader->m_DiffuseTexUHandle, 0);

        DrawAsSingleCalls();
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_WindmillTextureID);
        glUniform1i(pShader->m_DiffuseTexUHandle, 0);

        SetupMultiDrawParameters();
        DrawMulti();
    }

    pShader->disable();
    CHECK_GL_ERROR();
}

void MultiDrawIndirect::RenderC()
{
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        CPU_TIMER_SCOPE();
        GPU_TIMER_SCOPE();
    DrawSkyboxColorDepth();

        DrawModelColorDepth();
    }

    glDepthMask(false);

    m_ProjectionMatrixChanged = false;

    glDepthMask(true);
}

void MultiDrawIndirect::draw(void)
{
    m_CurrentViewMatrix = m_transformer->getModelViewMat();

    FreeGLBindings();
    RenderC();
    FreeGLBindings();

    m_statsFrames++;

    if (m_statsFrames > 20) 
    {
        char buffer[1024];

        sprintf(buffer, 
            "GPU Timing: %6.2fms\n"
            "CPU Timing: %6.2fms",
            m_GPUTimer.getScaledCycles() / m_GPUTimer.getStartStopCycles(),
            m_CPUTimer.getScaledCycles() * 1000.0f / m_statsFrames);

        m_timingStats->SetString(buffer);

        m_GPUTimer.reset();
        m_CPUTimer.reset();
        m_statsFrames = 0;
    }
}

void MultiDrawIndirect::initUI()
{
    if (mTweakBar)
    {
        mTweakBar->addPadding();

        if (isMobilePlatform())
        {
            mTweakBar->addValue("Grid Size (n x n) :", m_GridSize, MIN_MOBILE_GRID_SIZE, MAX_MOBILE_GRID_SIZE, 1);
        }
        else
        {
            mTweakBar->addValue("Grid Size (n x n) :", m_GridSize, MIN_GRID_SIZE, MAX_GRID_SIZE, 1);
        }

        NvTweakEnum<uint32_t> callModes[] =
        {
            {"Use Multidraw", USE_MULTIDRAWINDIRECT},
            {"Use Individual Draw Calls", NO_MULTIDRAWINDIRECT},
        };

        mTweakBar->addEnum("Instancing Mode:", m_DrawInstanceMode, callModes, TWEAKENUM_ARRAYSIZE(callModes));
    }

     // UI elements for displaying triangle statistics
    if (mFPSText) 
    {
        NvUIRect tr;
        mFPSText->GetScreenRect(tr); // base off of fps element.
        m_timingStats = new NvUIText("Multi\nLine\nString", NvUIFontFamily::SANS, (mFPSText->GetFontSize()*2)/3, NvUITextAlign::RIGHT);
        m_timingStats->SetColor(NV_PACKED_COLOR(0x30,0xD0,0xD0,0xB0));
        m_timingStats->SetShadow();
        mUIWindow->Add(m_timingStats, tr.left, tr.top+tr.height+8);
    }
}
 
void MultiDrawIndirect::reshape(int32_t width, int32_t height)
{
    m_ProjectionMatrixChanged = true;

    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    nv::perspective(m_ProjectionMatrix, NV_PI / 3.0f,
                    static_cast<float>(NvSampleApp::m_width) /
                    static_cast<float>(NvSampleApp::m_height),
                    1.0f, 3000.0f);

    m_InverseProjMatrix = nv::inverse(m_ProjectionMatrix);

    CHECK_GL_ERROR();
}

NvAppBase* NvAppFactory() 
{
    return new MultiDrawIndirect();
}

void MultiDrawIndirect::SetConstants()
{
    std::vector<NvModelGL*>::iterator   ii;
    NvModel*                            pData;
    int32_t                             SizeOfCompiledVertex;
    unsigned int                        NumberOfCompiledVertices;
    unsigned int                        NumberOfIndices;

    pData = m_Model->getModel();

    NumberOfCompiledVertices = 0;
    NumberOfIndices = 0;

    SizeOfCompiledVertex = pData->getCompiledVertexSize();

    NumberOfCompiledVertices += pData->getCompiledVertexCount();
	NvModelPrimType::Enum prim;
    NumberOfIndices += pData->getCompiledIndexCount(prim);

    m_IndexSize = sizeof(uint32_t);
    m_VertexSize = sizeof(float) * SizeOfCompiledVertex;

    m_SizeofIndexBuffer = NumberOfIndices * m_IndexSize;

    // Vertices
    m_SizeofVertexBuffer = NumberOfCompiledVertices * m_VertexSize * m_MaxModelInstances;

    m_OffsetofInstanceBuffer = m_SizeofVertexBuffer;

    m_SizeofVertexBuffer += 2 * sizeof(GLfloat) * m_MaxGridSize * m_MaxGridSize;
}

void MultiDrawIndirect::SetupMultipleModelData()
{
    NvModel*                            pData;
    float*                              pVertexData;
    uint32_t*                           pIndexData;
    unsigned int                        j;

    pIndexData = (uint32_t *) glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, 0,
                                                m_SizeofIndexBuffer,
                                                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    pVertexData = (float *) glMapBufferRange(   GL_ARRAY_BUFFER, 0,
                                                m_SizeofVertexBuffer,
                                                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    
    pData = m_Model->getModel();

    for (unsigned int k = 0; k < m_MaxModelInstances; k++)
    {
        float *pPositionData, Scale;

        memcpy( pVertexData,
                pData->getCompiledVertices(),
                pData->getCompiledVertexCount() * m_VertexSize);

        pPositionData = pVertexData;

        Scale = 1.0f + (rand() / (float) RAND_MAX) * 3.0f;;

        for (int z = 0; z < pData->getCompiledVertexCount(); z++)
        {
            pPositionData[0] = pPositionData[0];
            pPositionData[1] = pPositionData[1] * Scale;
            pPositionData[2] = pPositionData[2];

            pPositionData += pData->getCompiledVertexSize();
        }

        pVertexData += pData->getCompiledVertexCount() * pData->getCompiledVertexSize();
    }

	NvModelPrimType::Enum prim;
	memcpy(pIndexData,
            pData->getCompiledIndices(prim),
            pData->getCompiledIndexCount(prim) * m_IndexSize);

    pIndexData += pData->getCompiledIndexCount(prim);

    float* pInstData = (float*)pVertexData;

    for (j = 0; j < m_MaxGridSize; j++)
    {
        GetGridPoints(j, m_Offsets);
    }

    for (j = 0; j < m_MaxGridSize * m_MaxGridSize; j++)
    {
        *(pInstData++) = m_Offsets[2 * j];
        *(pInstData++) = m_Offsets[(2 * j) + 1];
    }

    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

void MultiDrawIndirect::SetupMultiDrawIndirectData( GLint                       PositionHandle,
                                                    GLint                       NormalHandle,
                                                    GLint                       TexcoordHandle,
                                                    GLint                       InstanceHandle)
{
    GLuint                              BufferID;
    NvModel*                            pData;
    int32_t                             PositionSize;
    int32_t                             NormalSize;
    int32_t                             TexCoordSize;

    pData = m_Model->getModel();

    PositionSize = pData->getPositionSize();
    NormalSize = pData->getNormalSize();
    TexCoordSize = pData->getTexCoordSize();

    SetConstants();

    glBindVertexArray(m_VertexArrayObject);

    glGenBuffers(1, &BufferID);
    glBindBuffer(GL_ARRAY_BUFFER, BufferID);

    glBufferData(   GL_ARRAY_BUFFER,
                    m_SizeofVertexBuffer,
                    NULL,
                    GL_STATIC_DRAW);
    glGenBuffers(1, &BufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferID);

    glBufferData(   GL_ELEMENT_ARRAY_BUFFER,
                    m_SizeofIndexBuffer,
                    NULL,
                    GL_STATIC_DRAW);

    SetupMultipleModelData();

    glVertexAttribPointer(  PositionHandle,
                            PositionSize,
                            GL_FLOAT,
                            GL_FALSE,
                            m_VertexSize,
                            0);
    glEnableVertexAttribArray(PositionHandle);

    glVertexAttribPointer(  NormalHandle,
                            NormalSize,
                            GL_FLOAT,
                            GL_FALSE,
                            m_VertexSize,
                            OFFSET(pData->getCompiledNormalOffset() * sizeof(float)));
    glEnableVertexAttribArray(NormalHandle);

    glVertexAttribPointer(  TexcoordHandle,
                            TexCoordSize,
                            GL_FLOAT,
                            GL_FALSE,
                            m_VertexSize,
                            OFFSET(pData->getCompiledTexCoordOffset() * sizeof(float)));
    glEnableVertexAttribArray(TexcoordHandle);
    
    if (InstanceHandle >= 0) {
        glVertexAttribPointer(  InstanceHandle,
                                2,
                                GL_FLOAT,
                                GL_FALSE,
                                0,
                               OFFSET(m_OffsetofInstanceBuffer));
        glEnableVertexAttribArray(InstanceHandle);
        glVertexAttribDivisor(InstanceHandle,1);
    }

    glBindVertexArray(0);
    CHECK_GL_ERROR();
}

void MultiDrawIndirect::CreateMultiDrawParameters()
{
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_IndirectDrawBuffer);

    glBufferStorage(GL_DRAW_INDIRECT_BUFFER, m_MaxGridSize * m_MaxGridSize * sizeof(DrawElementsIndirectCommand), 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT);

    m_MultidrawCommands = (DrawElementsIndirectCommand *)   glMapBufferRange(   GL_DRAW_INDIRECT_BUFFER, 0,
                                                                                m_MaxGridSize * m_MaxGridSize * sizeof(DrawElementsIndirectCommand),
                                                                                GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    CHECK_GL_ERROR();
}

void MultiDrawIndirect::SetupMultiDrawParameters()
{
	NvModelPrimType::Enum prim;
	unsigned int                        j;
    NvModel*                            pData;
    unsigned int                        VertexOffset;
    unsigned int                        IndexOffset;

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_IndirectDrawBuffer);
    CHECK_GL_ERROR();

    VertexOffset = 0;
    IndexOffset = 0;

    CHECK_GL_ERROR();

    pData = m_Model->getModel();

    for (j = 0; j < m_GridSize * m_GridSize; j++)
    {
		VertexOffset = (j % MAX_MODEL_INSTANCES) * pData->getCompiledVertexCount();

        m_MultidrawCommands[j].count = pData->getCompiledIndexCount(prim);
        m_MultidrawCommands[j].instanceCount = 1;
        m_MultidrawCommands[j].firstIndex = IndexOffset;
        m_MultidrawCommands[j].baseVertex = VertexOffset;
        m_MultidrawCommands[j].baseInstance = j;
    }

    IndexOffset += pData->getCompiledIndexCount(prim);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    CHECK_GL_ERROR();
}

void MultiDrawIndirect::DrawMulti()
{
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_IndirectDrawBuffer);
    glBindVertexArray(m_VertexArrayObject);

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, m_GridSize * m_GridSize, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    CHECK_GL_ERROR();
}

void MultiDrawIndirect::DrawAsSingleCalls()
{
	NvModelPrimType::Enum prim;
	unsigned int                        j;
    NvModel*                            pData;
    unsigned int                        VertexOffset;
    unsigned int                        IndexOffset;

    VertexOffset = 0;
    IndexOffset = 0;

    glBindVertexArray(m_VertexArrayObject);

    pData = m_Model->getModel();

    for (j = 0; j < m_GridSize * m_GridSize; j++)
    {
        VertexOffset = (j % MAX_MODEL_INSTANCES) * pData->getCompiledVertexCount();

        glUniform2f(m_SceneShader->m_PositionUHandle, m_Offsets[2 * j], m_Offsets[(2 * j) + 1]);

        glDrawElementsBaseVertex(  GL_TRIANGLES,
                                    pData->getCompiledIndexCount(prim),
                                    GL_UNSIGNED_INT,
                                    (GLvoid *) (IndexOffset * sizeof(IndexOffset)),
                                    VertexOffset);
    }

    IndexOffset += pData->getCompiledIndexCount(prim);

    glBindVertexArray(0);
    CHECK_GL_ERROR();
}

void MultiDrawIndirect::Startup()
{
    std::vector<NvModelGL*>::iterator   ii;
    int32_t                             length;
    char*                               modelData;

    m_SceneShader = new SceneShader("shaders/scenecolor.vert", "shaders/scenecolorvanilla.frag");
    m_SceneShaderMDI = new SceneShader("shaders/scenecolormdi.vert", "shaders/scenecolorvanilla.frag");

    m_SkyboxShader = new SkyboxShader;

    // Set the initial view 
    m_transformer->setRotationVec(nv::vec3f(0.0f, 2.35f, 0.0f));
    m_transformer->setTranslationVec(nv::vec3f(0.0f, -50.0f, 0.0f));

    modelData = NvAssetLoaderRead(MODEL_TO_LOAD_1, length);
	m_Model = NvModelGL::CreateFromObj(
		(uint8_t *)modelData, -1.0f, true, false);
    NvAssetLoaderFree(modelData);

    m_WindmillTextureID = NvImageGL::UploadTextureFromDDSFile("textures/windmill_diffuse1.dds");
    glBindTexture(GL_TEXTURE_2D, m_WindmillTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_SkyBoxTextureID = NvImageGL::UploadTextureFromDDSFile("textures/sky_cube.dds");
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyBoxTextureID);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    nv::vec4f lightPositionEye(1.0f, 1.0f, 1.0f, 0.0f);

    glGenBuffers(1, &m_IndirectDrawBuffer);
    glGenVertexArrays(1, &m_VertexArrayObject);
    CreateMultiDrawParameters();
    SetupMultiDrawParameters();

    m_SceneShader->enable();

    glUniform4fv(m_SceneShader->m_LightPositionUHandle, 1, lightPositionEye._array);
    glUniform1f(m_SceneShader->m_LightAmbientUHandle, 0.1f);
    glUniform1f(m_SceneShader->m_LightDiffuseUHandle, 0.7f);
    glUniform1f(m_SceneShader->m_LightSpecularUHandle, 1.0f);
    glUniform1f(m_SceneShader->m_LightShininessUHandle, 64.0f);

    m_SceneShader->disable();
    m_SceneShaderMDI->enable();

    glUniform4fv(m_SceneShaderMDI->m_LightPositionUHandle, 1, lightPositionEye._array);
    glUniform1f(m_SceneShaderMDI->m_LightAmbientUHandle, 0.1f);
    glUniform1f(m_SceneShaderMDI->m_LightDiffuseUHandle, 0.7f);
    glUniform1f(m_SceneShaderMDI->m_LightSpecularUHandle, 1.0f);
    glUniform1f(m_SceneShaderMDI->m_LightShininessUHandle, 64.0f);

    SetupMultiDrawIndirectData( m_SceneShaderMDI->m_PositionAHandle,
                                m_SceneShaderMDI->m_NormalAHandle,
                                m_SceneShaderMDI->m_TexcoordAHandle,
                                m_SceneShaderMDI->m_InstanceAHandle);

    m_SceneShaderMDI->disable();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void MultiDrawIndirect::Shutdown()
{
    CleanRendering();
}

