//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/TextureArrayTerrain.cpp
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
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NV/NvLogs.h"
#include "TerrainGenerator.h"
#include "TextureArrayTerrain.h"
#include "NvGLUtils/NvImageGL.h"
#include "NvUI/NvTweakBar.h"

#define PI 3.14159265f
const float TO_RADIANS = PI / 180.0f;

#define GL_TEXTURE_2D_ARRAY_EXT        0x8C1A

GLuint NvImageLoadTextureArrayFromDDSData(const char* ddsFile[], int32_t count);

static void (KHRONOS_APIENTRY *glTexImage3DOES) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
static void (KHRONOS_APIENTRY *glTexSubImage3DOES) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels);
static void (KHRONOS_APIENTRY *glCompressedTexImage3DOES) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);
static void (KHRONOS_APIENTRY *glCompressedTexSubImage3DOES) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data);

static void checkGlError(const char* op, const char* loc = "") {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x) @%s\n", op, error, loc);
    }
}

TextureArrayTerrain::BaseShader::BaseShader(const char *vertexProgramPath, const char *fragmentProgramPath) :
    m_positionAttrHandle(-1),
    m_projectionMatrixHandle(-1)
{
    setSourceFromFiles(vertexProgramPath, fragmentProgramPath);
}

void TextureArrayTerrain::BaseShader::initShaderParameters()
{
    m_positionAttrHandle     = getAttribLocation("g_vPosition");
    m_projectionMatrixHandle = getUniformLocation("g_projectionMatrix");

    derivedInitShaderParameters();
}

TextureArrayTerrain::SkyShader::SkyShader() :
    BaseShader("shaders/skybox.vert",  "shaders/skybox.frag"),
    m_inverseViewMatrixHandle(-1),
    m_skyTexCubeHandle(-1)
{
}

void TextureArrayTerrain::SkyShader::derivedInitShaderParameters()
{
    m_inverseViewMatrixHandle = getUniformLocation("g_inverseViewMatrix");
    m_positionAttrHandle      = getAttribLocation("g_vPosition");
    m_skyTexCubeHandle        = getUniformLocation("g_skyTexCube");
}

TextureArrayTerrain::TerrainShader::TerrainShader(const char *vertexProgramPath, 
    const char *fragmentProgramPath ) :
    BaseShader(vertexProgramPath, fragmentProgramPath),        
    m_normalAndHeightAttrHandle(-1),
    m_terrainTexHandle(-1),
    m_modelViewMatrixHandle(-1),
    m_interpOffsetHandle(-1)
{
}

// virtual
void TextureArrayTerrain::TerrainShader::derivedInitShaderParameters()
{
    m_normalAndHeightAttrHandle = getAttribLocation("g_vNormalAndHeight");
    m_modelViewMatrixHandle     = getUniformLocation("g_modelViewMatrix");
    m_terrainTexHandle          = getUniformLocation("g_terrainTex");
    m_interpOffsetHandle         = getUniformLocation("g_interpOffset");
}

TextureArrayTerrain::TextureArrayTerrain() : 
    m_pSkyShader(NULL),
    m_pTerrainShader(NULL)
{
    // Initialize some view parameters
    m_transformer->setRotationVec(nv::vec3f(0.0f, NV_PI*0.25f, 0.0f));
    m_transformer->setTranslationVec(nv::vec3f(0.0f, -5.0f, -25.0f));
    m_transformer->setMaxTranslationVel(30.0f);
    m_transformer->setMotionMode(NvCameraMotionType::FIRST_PERSON);

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

TextureArrayTerrain::~TextureArrayTerrain()
{
    deleteTerrainSurfaces();
    LOGI("TextureArrayTerrain: destroyed\n");
}

void TextureArrayTerrain::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0;
        
    config.apiVer = NvGLAPIVersionES3();
}

void TextureArrayTerrain::initUI(void) {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        // Show the app title
        //mTweakBar->addLabel("Texture Array Terrain Sample App", true);

        mTweakBar->addPadding();
        mTweakBar->addValue("Ridge Octaves:", (uint32_t&)m_simParams.octaves, 0, 12);
        mTweakBar->addValue("Ridgy-ness:", m_simParams.ridgeOffset, 0.0f, 1.0f);
        mTweakBar->addValue("Height Scale:", m_simParams.heightScale, 0.0f, 1.2f);
        mTweakBar->addValue("Height Offset:", m_simParams.heightOffset, 0.0f, 6.0f);
        mTweakBar->addValue("Coord Offset:", m_simParams.uvOffset, 0.0f, 50.0f);
    }
}

void TextureArrayTerrain::initRendering(void) {
    // We need at least _one_ of these two extensions
    const NvGLAPIVersion& api = getGLContext()->getConfiguration().apiVer;
    if (!requireExtension("GL_NV_texture_array", false) &&
        !requireExtension("GL_EXT_texture_array", false) &&
        ((api.api != NvGLAPI::GLES) || (api.majVersion != 3))) {
        errorExit("Texture array extensions not found!");
        return;
    }

    if ( api.majVersion >= 3 ) {
        TerrainSimRenderer::glUnmapBufferPtr = (void (KHRONOS_APIENTRY *)(GLenum))
            getGLContext()->getGLProcAddress("glUnmapBuffer");
    }
    else
    {
        TerrainSimRenderer::glUnmapBufferPtr = NULL;
    }

    if( requireExtension( "GL_NV_map_buffer_range", false ) )
    {
        TerrainSimRenderer::glMapBufferRangePtr = (void* (KHRONOS_APIENTRY *)(GLenum, GLintptr, GLsizeiptr, GLbitfield))
            getGLContext()->getGLProcAddress("glMapBufferRangeNV");

        if (!TerrainSimRenderer::glUnmapBufferPtr)
        {
            TerrainSimRenderer::glUnmapBufferPtr = (void (KHRONOS_APIENTRY *)(GLenum))
                getGLContext()->getGLProcAddress("glUnmapBufferNV");
        }
    }
    else if( requireExtension( "GL_EXT_map_buffer_range", false ) )
    {
        TerrainSimRenderer::glMapBufferRangePtr = (void* (KHRONOS_APIENTRY *)(GLenum, GLintptr, GLsizeiptr, GLbitfield))
            getGLContext()->getGLProcAddress("glMapBufferRangeEXT");

        if (!TerrainSimRenderer::glUnmapBufferPtr)
        {
            TerrainSimRenderer::glUnmapBufferPtr = (void (KHRONOS_APIENTRY *)(GLenum))
                getGLContext()->getGLProcAddress("glUnmapBufferNV");
        }
    }
    else
    {
        TerrainSimRenderer::glMapBufferRangePtr = (void* (KHRONOS_APIENTRY *)(GLenum, GLintptr, GLsizeiptr, GLbitfield))
            getGLContext()->getGLProcAddress("glMapBufferRange");

        TerrainSimRenderer::glUnmapBufferPtr = (void (KHRONOS_APIENTRY *)(GLenum))
            getGLContext()->getGLProcAddress("glUnmapBuffer");
    }

#ifdef GL_OES_texture_3D
    if (requireExtension("GL_EXT_texture_array", false)) {
        glTexImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*))
            getGLContext()->getGLProcAddress("glTexImage3DEXT");
        glTexSubImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*))
            getGLContext()->getGLProcAddress("glTexSubImage3DEXT");
        glCompressedTexImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*))
            getGLContext()->getGLProcAddress("glCompressedTexImage3DEXT");
        glCompressedTexSubImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*))
            getGLContext()->getGLProcAddress("glCompressedTexImage3DEXT");
    } else if (requireExtension("GL_NV_texture_array", false)) {
        glTexImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*))
            getGLContext()->getGLProcAddress("glTexImage3DNV");
        glTexSubImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*))
            getGLContext()->getGLProcAddress("glTexSubImage3DNV");
        glCompressedTexImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*))
            getGLContext()->getGLProcAddress("glCompressedTexImage3DNV");
        glCompressedTexSubImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*))
            getGLContext()->getGLProcAddress("glCompressedTexImage3DNV");
    } else {
        glTexImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*))
            getGLContext()->getGLProcAddress("glTexImage3D");
        glTexSubImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*))
            getGLContext()->getGLProcAddress("glTexSubImage3D");
        glCompressedTexImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*))
            getGLContext()->getGLProcAddress("glCompressedTexImage3D");
        glCompressedTexSubImage3DOES = (void (KHRONOS_APIENTRY *)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*))
            getGLContext()->getGLProcAddress("glCompressedTexImage3D");
    }
#else
    glTexImage3DOES = glTexImage3D;
    glTexSubImage3DOES = glTexSubImage3D;
    glCompressedTexImage3DOES = glCompressedTexImage3D;
    glCompressedTexSubImage3DOES = glCompressedTexSubImage3D;
#endif
    
    NvAssetLoaderAddSearchPath("es2-aurora/TextureArrayTerrain");

    glUseProgram(0);
    createShaders();
    checkGlError("initShaders");
    LOGI("Loaded shaders");

    const int32_t tileWidth = 128, tileHeight = 128;
    initTerrainSurfaces(tileWidth, tileHeight, NUM_TILES);
    checkGlError("initTerrainSurfaces");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.0, 0.0, 0.0, 1.0);

    m_SkyTexture = NvImageGL::UploadTextureFromDDSFile("sky/sky.dds");
    checkGlError("NvImageGL::UploadTextureFromDDSFile");
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    checkGlError("loadCubeMap");
    
    const int32_t count = 5;
    
    const char* arrayNames[] = {
        "terrain/grass_rgba.dds",
        "terrain/rock_rgba.dds",
        "terrain/rock2_rgba.dds",
        "terrain/snow_rgba.dds",
        "terrain/snow_flat_rgba.dds"
    };
    
    m_TerrainTexture = NvImageLoadTextureArrayFromDDSData(arrayNames, count);

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY_EXT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, 0);

    checkGlError("loadTextureArray");
    CHECK_GL_ERROR();
}

void TextureArrayTerrain::reshape(int32_t width, int32_t height)
{
    nv::perspective(m_projectionMatrix, 60.0f * TO_RADIANS,    (float) m_width / (float) m_height, 1.0f, 150.0f);

    glViewport( 0, 0, (GLint) width, (GLint) height );

    //setting the inverse perspective projection matrix
    nv::perspective(m_projectionMatrix, NV_PI / 3.0f,
                    static_cast<float>(NvSampleApp::m_width) /
                    static_cast<float>(NvSampleApp::m_height),
                    1.0f, 150.0f);
    m_inverseProjMatrix = nv::inverse(m_projectionMatrix);

    CHECK_GL_ERROR();
}

void TextureArrayTerrain::shutdownRendering(void)
{
    deleteTerrainSurfaces();
    LOGI("TextureArrayTerrain: destroyed\n");
}


void TextureArrayTerrain::draw(void)
{
    nv::matrix4f transM, rotM;
    m_viewMatrix = m_transformer->getModelViewMat();

    m_normalMatrix = nv::inverse(m_viewMatrix);
    m_normalMatrix = nv::transpose(m_normalMatrix);
    m_lightPositionEye = nv::vec4f(m_lightPosition.x, m_lightPosition.y, m_lightPosition.z, 0);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawTerrainSurfaces();

    // Sky box last with depth test enabled to avoid unnecessary fill.
    const nv::matrix4f invViewMatrix = nv::inverse(m_viewMatrix);
    drawSkyBox(invViewMatrix);
}

void TextureArrayTerrain::createShaders()
{
    destroyShaders();
    CHECK_GL_ERROR();


	if (getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL) {
		NvGLSLProgram::setGlobalShaderHeader("#version 130\n");
	}
	else {
		NvGLSLProgram::setGlobalShaderHeader("#version 100\n");
	}

	m_pSkyShader = new SkyShader;

	NvGLSLProgram::setGlobalShaderHeader(NULL);

	CHECK_GL_ERROR();
    m_pSkyShader->initShaderParameters();
    CHECK_GL_ERROR();

    if( getGLContext()->getConfiguration().apiVer <= NvGLAPIVersionES2() ) {
        m_pTerrainShader = new TerrainShader("shaders/terrain.vert", "shaders/terrain_es2.frag");
    } else if(getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GLES) {
        m_pTerrainShader = new TerrainShader("shaders/terrain_es3.vert", "shaders/terrain_es3.frag");
    } else {
        m_pTerrainShader = new TerrainShader("shaders/terrain.vert", "shaders/terrain.frag");
    }
    CHECK_GL_ERROR();
    m_pTerrainShader->initShaderParameters();
    CHECK_GL_ERROR();
}

void TextureArrayTerrain::destroyShaders()
{
    if (m_pSkyShader)
        delete m_pSkyShader;
    m_pSkyShader = NULL;

    if (m_pTerrainShader)
        delete m_pTerrainShader;
    m_pTerrainShader = NULL;
}

void TextureArrayTerrain::updateBufferData()
{
    for(int32_t i=0; i<NUM_TILES; i++)
        m_ppTerrain[i]->updateBufferData();
}

void TextureArrayTerrain::initTerrainSurfaces(int32_t w, int32_t h, int32_t numTiles)
{
    TerrainSimThread::resetCounter();

	TerrainSimThread::Init(createStopWatch());

    m_ppTerrain = new TerrainGenerator*[numTiles];

    const int32_t tilesOnEdge = (int32_t)sqrtf((float)numTiles);
    const float halfTilesOnEdge = (float) tilesOnEdge / 2.0f;
    NV_ASSERT(tilesOnEdge*tilesOnEdge == numTiles);

    for (int32_t x=0; x<tilesOnEdge; ++x)
    {
        for (int32_t y=0; y<tilesOnEdge; ++y)
        {
            // This is a translation that positions each tile uniquely in a grid layout.  We have this instead of a model matrix.
            const nv::vec2f offset(x-halfTilesOnEdge+1.0f, y-halfTilesOnEdge+1.0f);
            const int32_t i = x+y*tilesOnEdge;

            LOGI("Creating tile at offset (%f,%f)", offset.x, offset.y);
            m_ppTerrain[i] = new TerrainGenerator(w, h, offset);

            // Copy our master set of params down to every sim.
            m_ppTerrain[i]->getSimulation().initParams(m_simParams);
        }
    }
        
    LOGI("\nCreated %d terrain tile(s) with resolution [%dx%d]\n", numTiles, w, h);
}

// Nothing in the native template seems to call this?  Should it?  Probably.
void TextureArrayTerrain::deleteTerrainSurfaces()
{
    if (!m_ppTerrain)
        return;

    for(int32_t i=0; i<NUM_TILES; i++)
        m_ppTerrain[i]->getThread().stop();

    TerrainGenerator::syncAllSimulationThreads();
    TerrainGenerator::waitForAllThreadsToExit();

    for(int32_t i=0; i<NUM_TILES; i++)
        delete m_ppTerrain[i];

    delete [] m_ppTerrain;
    m_ppTerrain = NULL;
}

void TextureArrayTerrain::drawSkyBox(const nv::matrix4f& invViewMatrix)
{
    checkGlError("start", "drawSkyBox()");
    glActiveTexture(GL_TEXTURE3);
    checkGlError("glActiveTexture", "drawSkyBox()");
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyTexture);
    checkGlError("glBindTexture", "drawSkyBox()");
    m_pSkyShader->enable();
    checkGlError("glUseProgram", "drawSkyBox()");
    glUniform1i(m_pSkyShader->m_skyTexCubeHandle, 3);
    checkGlError("glUniform1i", "drawSkyBox()");

    glUniformMatrix4fv(m_pSkyShader->m_projectionMatrixHandle,  1, GL_FALSE, m_inverseProjMatrix._array);
    glUniformMatrix4fv(m_pSkyShader->m_inverseViewMatrixHandle, 1, GL_FALSE, invViewMatrix._array);
    checkGlError("glUniformMatrix4fv", "drawSkyBox()");

    const float skyQuadCoords[] = {    -1.0f, -1.0f, -1.0f, 1.0f,
                                              1.0f, -1.0f, -1.0f, 1.0f,
                                         -1.0f,  1.0f, -1.0f, 1.0f,
                                              1.0f,  1.0f, -1.0f, 1.0f};

    glVertexAttribPointer(m_pSkyShader->m_positionAttrHandle, 4, GL_FLOAT, GL_FALSE, 4*sizeof(float), skyQuadCoords);
    checkGlError("glVertexAttribPointer", "drawSkyBox()");

    glEnableVertexAttribArray(m_pSkyShader->m_positionAttrHandle);
    checkGlError("glEnableVertexAttribArray", "drawSkyBox()");

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays", "drawSkyBox()");

    glDisableVertexAttribArray(m_pSkyShader->m_positionAttrHandle);

    glUseProgram(0);
    checkGlError("end", "drawSkyBox()");
}



void TextureArrayTerrain::renderTerrainSurfaces()
{
    TerrainShader* pShader = m_pTerrainShader;
    pShader->enable();
    
    checkGlError("glUseProgram(terrain shader)");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, m_TerrainTexture);
    glUniform1i(pShader->m_terrainTexHandle, 0);
    checkGlError("glBindTexture (array)");

    glUniformMatrix4fv(pShader->m_projectionMatrixHandle, 1, GL_FALSE, m_projectionMatrix._array);
    glUniformMatrix4fv(pShader->m_modelViewMatrixHandle,  1, GL_FALSE, m_viewMatrix._array);
    glUniform1f(pShader->m_interpOffsetHandle, 
        (getGLContext()->getConfiguration().apiVer == NvGLAPIVersionES2()) ? 0.0f : 0.5f);
    checkGlError("params bound", "renderTerrainSurfaces");

    /* Too much spew:
    LOGI("proj matrix");
    printMatrixLog(appState().m_projectionMatrix);
    LOGI("view matrix");
    printMatrixLog(appState().m_viewMatrix);
    LOGI("normal matrix");
    printMatrixLog(appState().m_normalMatrix);
    */

    int32_t nVtx = 0;
    for(int32_t i=0; i<NUM_TILES; i++)
    {
        nVtx = m_ppTerrain[i]->render(pShader->m_positionAttrHandle, pShader->m_normalAndHeightAttrHandle);
        // Too much spew: LOGI("Number of terrain vertices=%d", nVtx);
        checkGlError("glDrawElements", "renderTerrainSurfaces");
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(pShader->m_positionAttrHandle);
    glDisableVertexAttribArray(pShader->m_normalAndHeightAttrHandle);
    glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, 0);
    glUseProgram(0);
    checkGlError("end", "renderTerrainSurfaces");
}

void TextureArrayTerrain::drawTerrainSurfaces()
{
    // Multi-threaded start all simulation threads
    for(int32_t i=0; i<NUM_TILES; i++)
    {
        m_ppTerrain[i]->getSimulation().setParams(m_simParams);

        if( m_ppTerrain[i]->getSimulation().dirtyParams() )
        {
            m_ppTerrain[i]->startSimulationThread();
        }
    }
    
    // Note this update is not sychronised with the sim threads.  I don't care if the rendered result is a bit
    // inconsistent while the GUI is being manipulated.
    updateBufferData();
    renderTerrainSurfaces();
}

GLuint NvImageLoadTextureArrayFromDDSData(const char* ddsFile[], int32_t count) {
    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, texID);
    int32_t error = glGetError();

    for (int32_t i = 0; i < count; i++) {
        NvImage* image = NvImage::CreateFromDDSFile(ddsFile[i]);

        if (image) {
            int32_t w = image->getWidth();
            int32_t h = image->getHeight();
            if (i == 0) {
                if (image->isCompressed()) 
                {
                    glCompressedTexImage3DOES(GL_TEXTURE_2D_ARRAY_EXT, 0, image->getInternalFormat(), 
                        w, h, count, 0, image->getImageSize(0)*count, NULL);
                    error = glGetError();
                } 
                else 
                {
                       glTexImage3DOES(GL_TEXTURE_2D_ARRAY_EXT, 0, image->getFormat(), w, h, count, 0, 
                        image->getFormat(), image->getType(), NULL);
                    error = glGetError();
                }
            }
            if (image->isCompressed()) 
            {
                glCompressedTexSubImage3DOES(GL_TEXTURE_2D_ARRAY_EXT, 0, 0, 0, i, w, h, 1, image->getInternalFormat(), 
                    image->getImageSize(0), image->getLevel(0));
                error = glGetError();
            } 
            else 
            {
                glTexSubImage3DOES(GL_TEXTURE_2D_ARRAY_EXT, 0, 0, 0, i, w, h, 1, image->getFormat(), 
                    image->getType(), image->getLevel(0));
                error = glGetError();
            }
        }

        delete image;
    }
    
    return texID;
}

NvAppBase* NvAppFactory() {
    return new TextureArrayTerrain();
}
