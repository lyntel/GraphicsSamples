//----------------------------------------------------------------------------------
// File:        es2-aurora\InstancingApp/InstancingApp.cpp
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
#include "InstancingApp.h"
#include "NvUI/NvTweakBar.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvImage/NvImage.h"
#include "NvGLUtils/NvImageGL.h"
#include "NvGLUtils/NvModelGL.h"
#include "NvModel/NvModel.h"
#include "NV/NvLogs.h"

typedef void (KHRONOS_APIENTRY *PFNDrawElementsInstanced)(GLenum mode, GLsizei count,GLenum type, const void* indices, GLsizei primcount);
typedef void (KHRONOS_APIENTRY *PFNVertexAttribDivisor)(GLuint index, GLuint divisor);

PFNDrawElementsInstanced glDrawElementsInstancedInternal = NULL;
PFNVertexAttribDivisor glVertexAttribDivisorInternal = NULL;

#define PI 3.1415926f
const float toRadians = PI / 180.0f;
#define OFFSET(n) ((char *)NULL + (n))

InstancingApp::InstancingApp()
    : m_sceneIndex(0)
    , m_instancingOptions(HARDWARE_INSTANCING)
    , m_instanceCount(MAX_OBJECTS)
{
    m_time = 0.0f;

    for( int32_t i = 0; i < NUMSCENES*2; ++i ) {
        m_shaders[ i ] = NULL;
        m_textureIDs[ i ] = INVALID_ID;
    }

    for( int32_t i = 0; i < NUMSCENES; ++i ) {
        m_pModel[i] = 0;
        m_vboID[ i ] = INVALID_ID;
        m_iboID[ i ] = INVALID_ID;
    }

    m_transformer->setRotationVec(nv::vec3f(PI*0.25f, 0.0f, 0.0f));
    m_transformer->setMaxTranslationVel(20.0f); // seems to work decently.

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

InstancingApp::~InstancingApp()
{
    LOGI("InstancingApp: destroyed\n");
}

void InstancingApp::configurationCallback(NvGLConfiguration& config)
{ 
    // The GBuffer FBO needs depth, but the onscreen buffer does not
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionES3();
}

void InstancingApp::initRendering(void) {
    m_transformer->setTranslationVec(isMobilePlatform() 
        ? nv::vec3f(0.0f, 0.0f, -40.0f) : nv::vec3f(-20.0f, 0.0f, -100.0f));

    if( requireMinAPIVersion(NvGLAPIVersionES3(), false) ) {
        m_hwInstancing = true;
        glDrawElementsInstancedInternal = (PFNDrawElementsInstanced)getGLContext()->getGLProcAddress("glDrawElementsInstanced");
        glVertexAttribDivisorInternal = (PFNVertexAttribDivisor)getGLContext()->getGLProcAddress("glVertexAttribDivisor");
    } else {
        // We need at least _one_ of these two extensions
        if (!requireExtension("GL_ARB_instanced_arrays", false) &&
            !requireExtension("GL_NV_draw_instanced", false)) {
            m_hwInstancing              = false;
            m_instancingOptions         = SHADER_INSTANCING;
        } else {
            m_hwInstancing              = true;
            if (requireExtension("GL_ARB_instanced_arrays", false) ){
                glDrawElementsInstancedInternal = (PFNDrawElementsInstanced)getGLContext()->getGLProcAddress("glDrawElementsInstancedARB");
                glVertexAttribDivisorInternal = (PFNVertexAttribDivisor)getGLContext()->getGLProcAddress("glVertexAttribDivisorARB");
            }
            else
            {
                glDrawElementsInstancedInternal = (PFNDrawElementsInstanced)getGLContext()->getGLProcAddress("glDrawElementsInstancedNV");
                glVertexAttribDivisorInternal = (PFNVertexAttribDivisor)getGLContext()->getGLProcAddress("glVertexAttribDivisorNV");
            }
        }
    }

    if( m_hwInstancing == false )
    {
        m_instancingOptions = SHADER_INSTANCING;
    }

    NvAssetLoaderAddSearchPath("es2-aurora/InstancingApp");

    LOGI("Hardware Instancing %s\n", m_hwInstancing ? "Available" : "Not available" );

	if (getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL) {
		NvGLSLProgram::setGlobalShaderHeader("#version 130\n");
	}
	else {
		NvGLSLProgram::setGlobalShaderHeader("#version 300 es\n");
	}

	//init the shaders
    m_shaders[0] = NvGLSLProgram::createFromFiles("shaders/boxes.vert", "shaders/boxes.frag");
    m_shaders[1] = NvGLSLProgram::createFromFiles("shaders/grass.vert", "shaders/grass.frag");
    m_shaders[2] = NvGLSLProgram::createFromFiles("shaders/boxes_instanced.vert", "shaders/boxes.frag");
    m_shaders[3] = NvGLSLProgram::createFromFiles("shaders/grass_instanced.vert", "shaders/grass.frag");

	NvGLSLProgram::setGlobalShaderHeader(NULL);

    initShaders();

    CHECK_GL_ERROR();

    //load g_pModel
    loadModelFromFile("models/cube.obj", 0);
    loadModelFromFile("models/grass.obj", 1);

    CHECK_GL_ERROR();

    GLuint texID;

    NvImage::VerticalFlip(false);

	CHECK_GL_ERROR();
	texID = NvImageGL::UploadTextureFromDDSFile("images/rock.dds");
    if( texID > 0) {
        configTexture( texID, 0 );
        configTexture( texID, 2 );
    }
	CHECK_GL_ERROR();

    texID = NvImageGL::UploadTextureFromDDSFile( "images/grass.dds" );
    if( texID > 0) {
        configTexture( texID, 1 );
        configTexture( texID, 3 );
    }
	CHECK_GL_ERROR();

    texID = NvImageGL::UploadTextureFromDDSFile( "images/rock.dds" );
    if( texID > 0)
    configTexture( texID, 2 );
	CHECK_GL_ERROR();

    texID = NvImageGL::UploadTextureFromDDSFile( "images/grass.dds" );
    if( texID > 0)
    configTexture( texID, 3 );
	CHECK_GL_ERROR();

    NvImage::VerticalFlip(false);

    glClearColor(0.0, 0.0, 0.0, 1.0);

    CHECK_GL_ERROR();
}

void InstancingApp::loadModelFromFile(const char *pFileName, int32_t modelNum )
{
    int32_t length;
    char *modelData = NvAssetLoaderRead(pFileName, length);

    m_pModel[modelNum] = NvModelGL::CreateFromObj((uint8_t*)modelData, 10.0f, true);

    NvAssetLoaderFree(modelData);

    initSceneInstancingData( modelNum );
}

void InstancingApp::initShaders()
{
    for( int32_t i = 0; i < 4; ++i )
    {
        m_positionHandle[i] = m_shaders[i]->getAttribLocation("vPosition");
        m_normalHandle[i] = m_shaders[i]->getAttribLocation("vNormal");
        m_texCoordHandle[i] = m_shaders[i]->getAttribLocation("vTexCoord");

        //Matrices
        m_modelViewMatrixHandle[i] = m_shaders[i]->getUniformLocation("ModelViewMatrix");
        m_projectionMatrixHandle[i] = m_shaders[i]->getUniformLocation("ProjectionMatrix");

        // time
        m_timeHandle[i] = m_shaders[i]->getUniformLocation("g_fTime");

        //tex
        m_texHandle[i] = m_shaders[i]->getUniformLocation("tex");

        // instance data
        m_instanceColorsHandle[i] = m_shaders[i]->getUniformLocation("InstanceColors");
        if( i < 2 )
        {
            m_instanceOffsetHandle[i] = m_shaders[i]->getUniformLocation("InstanceOffsets");
            m_instanceRotationHandle[i] = m_shaders[i]->getUniformLocation("InstanceRotations");
        }
        else
        {
            m_instanceOffsetHandle[i] = m_shaders[i]->getAttribLocation("vInstanceOffsets");
            m_instanceRotationHandle[i] = m_shaders[i]->getAttribLocation("vInstanceRotations");
        }

        //assign some values which never change
        m_shaders[i]->enable();

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(m_texHandle[i], 0);

        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i]);

        glActiveTexture(GL_TEXTURE0);

        m_shaders[i]->disable();
    }
}

bool InstancingApp::configTexture(GLuint texID, int32_t index)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_textureIDs[index] = texID;

    return true;
}

void InstancingApp::initSceneColorPalette( int32_t sceneIndex )
{
   for( int32_t c = 0; c < 6; ++c )
   {
      m_instanceColor[sceneIndex][ c*3 + 0 ] = m_instanceColor[sceneIndex][ c*3 + 1 ] = m_instanceColor[sceneIndex][ c*3 + 2 ] =
            ( float( rand() ) / ( 2.0f * float( RAND_MAX ) ) ) + 0.5f;
   }
}

void InstancingApp::initPerInstanceRotations( int32_t sceneIndex, float* pHwInstanceData )
{
    // init random rotations for the instances of the objects in the scene
    for( int32_t i = 0; i < MAX_OBJECTS; ++i )
    {
        float fAngle = float( rand() ) / float( RAND_MAX ) * 2.0f * float( PI );
        pHwInstanceData[ i * 6 + 3 ] = m_instanceRotation[ sceneIndex ][ i*3 + 0 ] = cos( fAngle );
        pHwInstanceData[ i * 6 + 4 ] = m_instanceRotation[ sceneIndex ][ i*3 + 1 ] = sin( fAngle );
        if( sceneIndex == BOXES_SCENE )
            pHwInstanceData[ i * 6 + 5 ] = m_instanceRotation[ sceneIndex ][ i*3 + 2 ] = float( i % 6 );
        else
            pHwInstanceData[ i * 6 + 5 ] = m_instanceRotation[ sceneIndex ][ i*3 + 2 ] = ( float( rand() ) / float( RAND_MAX ) ) * 5;
    }
}

void InstancingApp::initPerInstancePositions( int32_t sceneIndex, float* pHwInstanceData )
{
    // init positional offsets for all instances
    if( sceneIndex == BOXES_SCENE ) // boxes scene
    {
        for( int32_t z = 0; z < GRID_SIZE; ++z )
        {
            for( int32_t y = 0; y < GRID_SIZE; ++y )
            {
                for( int32_t x = 0; x < GRID_SIZE; ++x )
                {
                    int32_t i = 3 * ( z * GRID_SIZE * GRID_SIZE + y * GRID_SIZE + x );
                    
                    m_instanceOffsets[sceneIndex][ i + 0 ] = - 10.0f + float( x ) * 1.1f 
                        + (isMobilePlatform() ? -5.0f : 0.0f);
                    m_instanceOffsets[sceneIndex][ i + 1 ] = - 10.0f + float( y ) * 1.1f;
                    m_instanceOffsets[sceneIndex][ i + 2 ] = - 10.0f + float( z ) * 1.1f;
                    int32_t j = 6 * ( z * GRID_SIZE * GRID_SIZE + y * GRID_SIZE + x );
                    pHwInstanceData[ j + 0 ] = m_instanceOffsets[sceneIndex][ i + 0 ];
                    pHwInstanceData[ j + 1 ] = m_instanceOffsets[sceneIndex][ i + 1 ];
                    pHwInstanceData[ j + 2 ] = m_instanceOffsets[sceneIndex][ i + 2 ];
                }
            }
        }
    }
    else // grass scene
    {
      const static int32_t MAX_GRASS_SIZE = int32_t( sqrt( float(MAX_OBJECTS) ) );

        for( int32_t y = 0; y < MAX_GRASS_SIZE; ++y )
        {
            for( int32_t x = 0; x < MAX_GRASS_SIZE; ++x )
            {
                int32_t i = ( y * MAX_GRASS_SIZE + x );
                if( i < MAX_OBJECTS )
                {
                    i *= 3;
                    m_instanceOffsets[sceneIndex][ i + 0 ] = -10.0f + float( x ) * 0.25f + ( ( float( rand() ) / float( RAND_MAX ) ) - 0.5f ) * 0.08f 
                        - (isMobilePlatform() ? 10.0f : 20.0f);
                    m_instanceOffsets[sceneIndex][ i + 1 ] = isMobilePlatform() ? 10.0f : 40.0f;
                    m_instanceOffsets[sceneIndex][ i + 2 ] = -10.0f + float( y ) * 0.25f + ( ( float( rand() ) / float( RAND_MAX ) ) - 0.5f ) * 0.08f;
                    int32_t j = 6 * ( y * MAX_GRASS_SIZE + x );
                    pHwInstanceData[ j + 0 ] = m_instanceOffsets[sceneIndex][ i + 0 ];
                    pHwInstanceData[ j + 1 ] = m_instanceOffsets[sceneIndex][ i + 1 ];
                    pHwInstanceData[ j + 2 ] = m_instanceOffsets[sceneIndex][ i + 2 ];
                }
            }
        }
    }
}

void InstancingApp::initModelCopies( int32_t sceneIndex, float* pV, int32_t* pI, NvModelGL* pMdl )
{
    NvModel *pBaseMdl = pMdl->getModel();

	NvModelPrimType::Enum prim;
	int32_t    vtxSize = pBaseMdl->getCompiledVertexSize();
	int32_t    vtxCount = pBaseMdl->getCompiledVertexCount();
	int32_t    idxCount = pBaseMdl->getCompiledIndexCount(prim);
    int32_t    tcOff         = pBaseMdl->getCompiledTexCoordOffset();
    const float* pModel  = pBaseMdl->getCompiledVertices();

    // write MAX_INSTANCES copies of the scene object to be instanced to
    // a big vertex buffer and index buffer - this data is used in the
    // shader instancing rendering path
    for( int32_t i = 0; i < MAX_INSTANCES; ++i )
    {
        int32_t voff = vtxCount * vtxSize * i;
        int32_t ioff = idxCount * i;

        for( int32_t v = 0; v < vtxCount; ++v )
        {
            // copy original vertex
            memcpy( (void*) &pV[ voff + v * vtxSize ],
                      (void*) &pModel[ v * vtxSize ],
                      vtxSize * sizeof( float) );

            // patch instance id into .z component of the 3d texture coordinates
            pV[ voff + v * vtxSize +  tcOff  + 2 ] = float( i );
        }

        // copy indices and modify them accordingly
        for( int32_t idx = 0; idx < idxCount; ++idx )
        {
			pI[ioff + idx] = *(pBaseMdl->getCompiledIndices(prim) + idx);
            pI[ ioff + idx ] += vtxCount * i;
        }
    }
}

void InstancingApp::initGLObjects( int32_t sceneIndex, float* pV, int32_t* pI, NvModelGL* pMdl )
{
	NvModelPrimType::Enum prim;
	NvModel *pBaseMdl = pMdl->getModel();
    int32_t        floatCount     = pBaseMdl->getCompiledVertexCount() * pBaseMdl->getCompiledVertexSize() * MAX_INSTANCES;
	int32_t        intCount = pBaseMdl->getCompiledIndexCount(prim) * MAX_INSTANCES;

    // create vbo from the data
    glGenBuffers(1, &m_vboID[sceneIndex]);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboID[sceneIndex]);
    glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float) + 6 * MAX_OBJECTS * sizeof(float), pV, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // create ibo from the data
    glGenBuffers(1, &m_iboID[sceneIndex]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboID[sceneIndex]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, intCount * sizeof(uint32_t), pI, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool InstancingApp::initSceneInstancingData( int32_t sceneIndex )
{
	NvModelPrimType::Enum prim;
	NvModelGL* pMdl = m_pModel[sceneIndex];
    NvModel *pBaseMdl = pMdl->getModel();
    int32_t        floatCount     = pBaseMdl->getCompiledVertexCount() * pBaseMdl->getCompiledVertexSize() * MAX_INSTANCES;
    int32_t        intCount       = pBaseMdl->getCompiledIndexCount(prim) * MAX_INSTANCES;

    // array big enough to hold n instances and data for hw instancing
    // after n copies of the vertex data of the object there is room
    // for all position and rotation data for all instances
    float* pV            = new float[ floatCount + 6 * MAX_OBJECTS ];
    GLint* pI            = new GLint[ intCount ];

    // early out if no data
    if( pV == 0 || pI == 0 )
    {
        if( pV )
            delete[] pV;
        if( pI )
            delete[] pI;

        return false;
    }

    // ptr to the per object instancing data in the vbo
    float* phwInstanceData = pV + floatCount;

    // init data for this scene
    initSceneColorPalette( sceneIndex );
    initPerInstanceRotations( sceneIndex, phwInstanceData );
    initPerInstancePositions( sceneIndex, phwInstanceData );
    initModelCopies( sceneIndex, pV, pI, pMdl );
    initGLObjects( sceneIndex, pV, pI, pMdl );

    // free temp data
    delete[] pV;
    delete[] pI;

    return true;
}

static bool testbool = false;

void InstancingApp::initUI() {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        // render method
        NvTweakEnum<uint32_t> renderModes[] =
        {
            {"None", NO_INSTANCING},
            {"Shader", SHADER_INSTANCING},
            {"Hardware", HARDWARE_INSTANCING}
        };

        if (!requireExtension("GL_ARB_instanced_arrays", false) &&
            !requireExtension("GL_NV_draw_instanced", false)) 
           mTweakBar->addEnum("Instancing Mode:", m_instancingOptions, renderModes, TWEAKENUM_ARRAYSIZE(renderModes)-1);
        else
           mTweakBar->addEnum("Instancing Mode:", m_instancingOptions, renderModes, TWEAKENUM_ARRAYSIZE(renderModes));

        // scene
        mTweakBar->addPadding();
        NvTweakEnum<uint32_t> renderScenes[] =
        {
            {"Boxes", BOXES_SCENE},
            {"Grass", GRASS_SCENE},
        };
        mTweakBar->addEnum("Render Mode:", m_sceneIndex, renderScenes, TWEAKENUM_ARRAYSIZE(renderScenes));

        // instance count
        mTweakBar->addPadding();
        mTweakBar->addValue("Instance Count:", m_instanceCount, MAX_INSTANCES, MAX_OBJECTS, MAX_INSTANCES);

        // test a checkbox
        //mTweakBar->addPadding();
        //mTweakBar->addValue("Test Check:", testbool);
    }
}

void InstancingApp::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );

    //setting the perspective projection matrix
    m_projectionMatrix = nv::perspective(m_projectionMatrix, 60.0f * toRadians,    (float) m_width / (float) m_height, 1.0f, 150.0f);
    m_ortho2DProjectionMatrixScreen = nv::ortho2D(m_ortho2DProjectionMatrixScreen, 0.0f, (float)m_width, 0.0f, (float)m_height);

    CHECK_GL_ERROR();
}

void InstancingApp::drawModelLit(void)
{
    int32_t i = m_sceneIndex;
    int32_t si = i; // shader index

   m_time += getFrameDeltaTime();

   // if hardware instancing is on => use second set of shaders
   // that draw instancing data in an attributes
    if( m_instancingOptions == HARDWARE_INSTANCING )
        si = i + 2;

    m_shaders[si]->enable();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIDs[si]);

   // send time
   glUniform1f(m_timeHandle[si], m_time );

    //send matrices
    glUniformMatrix4fv(m_modelViewMatrixHandle[si], 1, GL_FALSE, m_viewMatrix._array);
    glUniformMatrix4fv(m_projectionMatrixHandle[si], 1, GL_FALSE,
                       m_projectionMatrix._array);
    glUniform3fv(m_instanceColorsHandle[si], 6, &(m_instanceColor[i][0]) );

    glBindBuffer(GL_ARRAY_BUFFER, m_vboID[i]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboID[i]);

    glVertexAttribPointer(m_positionHandle[si], 3, GL_FLOAT, GL_FALSE, m_pModel[i]->getModel()->getCompiledVertexSize() * sizeof(float), OFFSET( m_pModel[i]->getModel()->getCompiledPositionOffset() * sizeof(float) )) ;
    glVertexAttribPointer(m_texCoordHandle[si], 3, GL_FLOAT, GL_FALSE, m_pModel[i]->getModel()->getCompiledVertexSize() * sizeof(float), OFFSET(m_pModel[i]->getModel()->getCompiledTexCoordOffset() * sizeof(float)));
    glEnableVertexAttribArray(m_positionHandle[si]);
    glEnableVertexAttribArray(m_texCoordHandle[si]);
    if( m_instancingOptions == SHADER_INSTANCING )
    {
        int32_t offset = 0;

        for( int32_t toDraw = m_instanceCount; toDraw > 0; toDraw -= MAX_INSTANCES )
        {
			NvModelPrimType::Enum prim;
			int32_t draw_count = toDraw < MAX_INSTANCES ? toDraw : MAX_INSTANCES;
            glUniform3fv( m_instanceOffsetHandle[si], draw_count, &(m_instanceOffsets[i][ offset * 3  ]) );
            glUniform3fv( m_instanceRotationHandle[si], draw_count, &(m_instanceRotation[i][ offset * 3  ]) );
            glDrawElements(GL_TRIANGLES, m_pModel[i]->getModel()->getCompiledIndexCount(prim) * draw_count, GL_UNSIGNED_INT, 0);
            offset += draw_count;
        }
    }
    else if( m_instancingOptions == HARDWARE_INSTANCING )
    {
		NvModelPrimType::Enum prim;
		int32_t    floatCount = m_pModel[i]->getModel()->getCompiledVertexCount() * m_pModel[i]->getModel()->getCompiledVertexSize() * MAX_INSTANCES;

        glEnableVertexAttribArray(m_instanceOffsetHandle[si]);
        glEnableVertexAttribArray(m_instanceRotationHandle[si]);

        glVertexAttribPointer(m_instanceOffsetHandle[si], 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), OFFSET( floatCount * sizeof(float) )) ;
        glVertexAttribPointer(m_instanceRotationHandle[si], 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), OFFSET( ( floatCount + 3 ) * sizeof(float) ) );

        glVertexAttribDivisorInternal( m_instanceOffsetHandle[si], 1 );
        glVertexAttribDivisorInternal( m_instanceRotationHandle[si], 1  );

        glDrawElementsInstancedInternal(GL_TRIANGLES, m_pModel[i]->getModel()->getCompiledIndexCount(prim), GL_UNSIGNED_INT, 0, m_instanceCount );

        glVertexAttribDivisorInternal( m_instanceOffsetHandle[si], 0 );
        glVertexAttribDivisorInternal( m_instanceRotationHandle[si], 0  );

        glDisableVertexAttribArray(m_instanceOffsetHandle[si]);
        glDisableVertexAttribArray(m_instanceRotationHandle[si]);
    }
    else if( m_instancingOptions == NO_INSTANCING )
    {
        for( uint32_t j = 0; j < m_instanceCount; ++j )
        {
			NvModelPrimType::Enum prim;
			glUniform3fv(m_instanceOffsetHandle[si], 1, &(m_instanceOffsets[i][j * 3]));
            glUniform3fv( m_instanceRotationHandle[si], 1, &(m_instanceRotation[i][ j * 3  ]) );
            glDrawElements(GL_TRIANGLES, m_pModel[i]->getModel()->getCompiledIndexCount(prim), GL_UNSIGNED_INT, 0);
        }
    }
    glDisableVertexAttribArray(m_positionHandle[si]);
    glDisableVertexAttribArray(m_texCoordHandle[si]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

   m_shaders[si]->disable();
}

void InstancingApp::draw(void)
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);

    m_viewMatrix = m_transformer->getModelViewMat();

    m_normalMatrix = nv::inverse(m_viewMatrix);
    m_normalMatrix = nv::transpose(m_normalMatrix);

    drawModelLit();
}

NvAppBase* NvAppFactory() {
    return new InstancingApp();
}
