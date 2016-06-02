//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation/TerrainTessellation.cpp
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
#include "TerrainTessellation.h"

#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvSimpleFBO.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvShapesGL.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#include <algorithm>
#include <string>
#include "noise.h"

TerrainTessellation::TerrainTessellation() 
    : mTerrainVertexProg(0),
    mTerrainTessControlProg(0),
    mTerrainTessEvalProg(0),
    mWireframeGeometryProg(0),
    mTerrainFragmentProg(0),
    mTerrainPipeline(0),
    mGPUQuery(0),
    mNumPrimitives(0),
    mSkyProg(NULL),
    mGenerateTerrainProg(NULL),
    mUBO(0),
    mVBO(0),
    mRandTex(0),
    mRandTex3D(0),
    mTerrainFbo(NULL),
    mLightDir(-1.0f, -0.25f, 1.0f),
    mTerrainVbo(0),
    mTerrainIbo(0),
    mQuality(0),
    mCull(true),
    mLod(true),
    mWireframe(false),
    mAnimate(true),
    mHeightScale(0.1f),
    mReload(false),
    mTime(0.0f),
    mStatsText(NULL)
{
    mLightDir = normalize(mLightDir);

    // set view
    m_transformer->setMotionMode(NvCameraMotionType::FIRST_PERSON);
    m_transformer->setTranslationVec(nv::vec3f(0.0f, -1.0f, 0.0f));
    m_transformer->setRotationVec(nv::vec3f(0.0f, 0.7f, 0.0f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

TerrainTessellation::~TerrainTessellation()
{
    LOGI("TerrainTessellation: destroyed\n");
}

void TerrainTessellation::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    // we need ES3.1AEP at least, but we'll take big GL if we can get it
    config.apiVer = NvGLAPIVersionGL4_4();
}

// Some Linux OpenGL headers lack the EXT token/commands but have the core versions
#ifndef GL_TESS_CONTROL_SHADER_EXT
# define GL_TESS_CONTROL_SHADER_EXT GL_TESS_CONTROL_SHADER
# define GL_TESS_EVALUATION_SHADER_EXT GL_TESS_EVALUATION_SHADER
# define GL_TESS_CONTROL_SHADER_BIT_EXT GL_TESS_CONTROL_SHADER_BIT
# define GL_TESS_EVALUATION_SHADER_BIT_EXT GL_TESS_EVALUATION_SHADER_BIT
# define GL_PATCH_VERTICES_EXT GL_PATCH_VERTICES
# define GL_GEOMETRY_SHADER_BIT_EXT GL_GEOMETRY_SHADER_BIT
# define GL_PATCHES_EXT GL_PATCHES
# define glPatchParameteriEXT glPatchParameteri
#endif

static inline const char *GetShaderStageName(GLenum target)
{
    switch(target) {
    case GL_VERTEX_SHADER:
        return "VERTEX_SHADER";
        break;
    case GL_GEOMETRY_SHADER_EXT:
        return "GEOMETRY_SHADER";
        break;
    case GL_FRAGMENT_SHADER:
        return "FRAGMENT_SHADER";
        break;
    case GL_TESS_CONTROL_SHADER_EXT:
        return "TESS_CONTROL_SHADER";
        break;
    case GL_TESS_EVALUATION_SHADER_EXT:
        return "TESS_EVALUATION_SHADER";
        break;
    case GL_COMPUTE_SHADER:
        return "COMPUTE_SHADER";
        break;
    }
    return "";
}

GLuint TerrainTessellation::createShaderPipelineProgram(GLuint target, const char* src)
{
    GLuint object;
    GLint status;

    const char* shaderPrefix =
        (getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL)
        ? "#version 440\n" : "#version 310 es\n#extension GL_ANDROID_extension_pack_es31a : enable\n";

    const GLchar* fullSrc[2] = {
        shaderPrefix, src };

    object = glCreateShaderProgramv( target, 2, (const GLchar **)fullSrc);
    glGetProgramiv(object, GL_LINK_STATUS, &status);

    if (!status)
    {
        GLint charsWritten, infoLogLength;
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &infoLogLength);
        char * infoLog = new char[infoLogLength];
        glGetProgramInfoLog(object, infoLogLength, &charsWritten, infoLog);
        LOGI("Error compiling %s:\n", GetShaderStageName(target));
        LOGI("Log: %s", infoLog);
        delete [] infoLog;

        glDeleteProgram( object);
        object = 0;
    }

    return object;
}

static std::string createStringFromAsset(const char* filename) {
    int32_t len;

    char *asset = NvAssetLoaderRead(filename, len);
    std::string str = asset;
    NvAssetLoaderFree(asset);

    return str;
}

static std::string loadShaderSourceWithIncludeTag(const char* includeSrc, const char* srcFile) {
    int32_t len;

    char *srcStr = NvAssetLoaderRead(srcFile, len);
    if (!srcStr)
        return "";

    std::string dest = "";

    const char* uniformTag = "#UNIFORMS";

    char* uniformTagStart = strstr(srcStr, uniformTag);

    if (uniformTagStart) {
        // NULL the start of the tag
        *uniformTagStart = 0;
        dest += srcStr; // source up to tag
        dest += "\n";
        dest += includeSrc;
        dest += "\n";
        char* uniformTagEnd = uniformTagStart + strlen(uniformTag);
        dest += uniformTagEnd;
    } else {
        dest += srcStr;
    }

    NvAssetLoaderFree(srcStr);

    return dest;
}

void TerrainTessellation::loadShaders()
{
    NvScopedShaderPrefix switched(
    (getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL)
    ? "#version 440\n" : "#version 310 es\n#extension GL_ANDROID_extension_pack_es31a : enable\n");
    CHECK_GL_ERROR();

    std::string uniformsHeader = createStringFromAsset("shaders/uniforms.h");
    std::string noiseHeader = createStringFromAsset("shaders/noise.glsl");
    std::string noise3DHeader = createStringFromAsset("shaders/noise3D.glsl");
    std::string terrainHeader = createStringFromAsset("shaders/terrain.glsl");

    std::string generateTerrain_vs = createStringFromAsset("shaders/generateTerrain_vs.glsl");

    // ("/uniforms.h")
    std::string terrain_vertex = loadShaderSourceWithIncludeTag(uniformsHeader.c_str(),
        "shaders/terrain_vertex.glsl");
    std::string terrain_control = loadShaderSourceWithIncludeTag(uniformsHeader.c_str(),
        "shaders/terrain_control.glsl");
    std::string wireframe_geometry = loadShaderSourceWithIncludeTag(uniformsHeader.c_str(),
        "shaders/wireframe_geom.glsl");
    std::string fragment_wireframe = loadShaderSourceWithIncludeTag(uniformsHeader.c_str(),
        "shaders/fragment_wireframe.glsl");

    std::string sky_vs = loadShaderSourceWithIncludeTag(uniformsHeader.c_str(),
        "shaders/sky_vs.glsl");

    // ("/uniforms.h", "noise3D.glsl")
    std::string currentHeader = uniformsHeader + "\n" + noise3DHeader;
    std::string sky_fs = loadShaderSourceWithIncludeTag(currentHeader.c_str(),
        "shaders/sky_fs.glsl");

    // "/uniforms.h", "/noise.glsl", "/terrain.glsl"
    currentHeader = uniformsHeader + "\n" + noiseHeader + "\n" + terrainHeader;
    std::string terrain_tessellation = loadShaderSourceWithIncludeTag(currentHeader.c_str(),
        "shaders/terrain_tessellation.glsl");
    std::string generateTerrain_fs = loadShaderSourceWithIncludeTag(currentHeader.c_str(),
        "shaders/generateTerrain_fs.glsl");
    
    // ("uniforms.h", "/noise.glsl", "noise3D.glsl")
    currentHeader = uniformsHeader + "\n" + noiseHeader + "\n" + noise3DHeader;
    std::string terrain_fragment = loadShaderSourceWithIncludeTag(currentHeader.c_str(),
        "shaders/terrain_fragment.glsl");

    LOGI( "Compiling vertex shader\n");
    mTerrainVertexProg = createShaderPipelineProgram( GL_VERTEX_SHADER, terrain_vertex.c_str());

    LOGI( "Compiling tessellation control shader\n");
    mTerrainTessControlProg = createShaderPipelineProgram( GL_TESS_CONTROL_SHADER_EXT, terrain_control.c_str()); 
    
    LOGI( "Compiling tessellation evaluation shader\n");
    mTerrainTessEvalProg = createShaderPipelineProgram( GL_TESS_EVALUATION_SHADER_EXT, terrain_tessellation.c_str()); 

    LOGI( "Compiling wireframe geometry shader\n");
    mWireframeGeometryProg = createShaderPipelineProgram( GL_GEOMETRY_SHADER_EXT, wireframe_geometry.c_str()); 

    LOGI( "Compiling fragment shader\n");
    mTerrainFragmentProg = createShaderPipelineProgram( GL_FRAGMENT_SHADER, terrain_fragment.c_str());

    LOGI( "Compiling wireframe fragment shader\n");
    mWireframeFragmentProg = createShaderPipelineProgram( GL_FRAGMENT_SHADER, fragment_wireframe.c_str());

    GLint loc;

    CHECK_GL_ERROR();
    loc = glGetUniformLocation(mTerrainTessEvalProg, "terrainTex");
    CHECK_GL_ERROR();
    if (loc >= 0) {
        glProgramUniform1i(mTerrainTessEvalProg, loc, 2);
    }
    CHECK_GL_ERROR();

    mSkyProg = NvGLSLProgram::createFromStrings(sky_vs.c_str(), sky_fs.c_str());
    CHECK_GL_ERROR();
    mSkyProg->enable();
    mSkyProg->setUniform1i("randTex3D", 0);
    mSkyProg->disable();
    CHECK_GL_ERROR();

    mGenerateTerrainProg = NvGLSLProgram::createFromStrings(generateTerrain_vs.c_str(), generateTerrain_fs.c_str());
    CHECK_GL_ERROR();
    mGenerateTerrainProg->enable();
    mGenerateTerrainProg->setUniform1i("randTex", 0);
    mGenerateTerrainProg->disable();
    CHECK_GL_ERROR();
}

void TerrainTessellation::initTerrainFbo()
{
    if (mTerrainFbo) {
        delete mTerrainFbo;
    }
    NvSimpleFBO::Desc fboDesc;
    fboDesc.width = mParams.gridW * 64;
    fboDesc.height = mParams.gridH * 64;
    fboDesc.color.format = GL_RGBA;
    fboDesc.color.type = GL_FLOAT;
    fboDesc.color.filter = GL_LINEAR;

    mTerrainFbo = new NvSimpleFBO(fboDesc);
}

void TerrainTessellation::initRendering(void) {
    bool isGL = getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL;

    if(!(requireMinAPIVersion(NvGLAPIVersionES3_1(), false) &&
         requireExtension("GL_ANDROID_extension_pack_es31a", false)) &&
        !(isGL && requireExtension("GL_ARB_tessellation_shader", false)))
    {
        errorExit("The system does not support the required OpenGL[ES] features\n"
            "This sample requires either OpenGL 4.4 or\n"
            "OpenGL ES 3.1 with the Android Extension Pack");
        return;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    

    NvAssetLoaderAddSearchPath("es3aep-kepler/TerrainTessellation");

    loadShaders();
    CHECK_GL_ERROR();

    //
    // Create a single default program pipeline to handle binding and unbinding
    // separate shader objects
    //
    glGenProgramPipelines( 1, &mTerrainPipeline);
    glBindProgramPipeline( mTerrainPipeline);
    CHECK_GL_ERROR();

    //create ubo and initialize it with the structure data
    glGenBuffers( 1, &mUBO);
    glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
    glBufferData( GL_UNIFORM_BUFFER, sizeof(TessellationParams), &mParams, GL_STREAM_DRAW);
    CHECK_GL_ERROR();
    
    //create simple single-vertex VBO
    float vtx_data[] = { 0.0f, 0.0f, 0.0f, 1.0f};
    glGenBuffers( 1, &mVBO);
    glBindBuffer( GL_ARRAY_BUFFER, mVBO);
    glBufferData( GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);
    CHECK_GL_ERROR();

    srand(0);

    int noiseSize = 256;
    int noiseSize3D = 64;
    mRandTex = createNoiseTexture2D(noiseSize, noiseSize, GL_R16F);
    CHECK_GL_ERROR();

    mRandTex3D = createNoiseTexture4f3D(noiseSize3D, noiseSize3D, noiseSize3D, GL_RGBA16F);
    CHECK_GL_ERROR();

    mParams.invNoiseSize = 1.0f / noiseSize;
    mParams.invNoise3DSize = 1.0f / noiseSize3D;

    glGenQueries(1, &mGPUQuery);
    CHECK_GL_ERROR();

    //initTerrainFbo();
    //updateQuality();
    //CHECK_GL_ERROR();
}

#define REACT_QUALITY_MODE  0x10000001

void TerrainTessellation::initUI(void) {
    if (mTweakBar) {
        NvTweakVarBase *var;

        NvTweakEnum<uint32_t> qualityModes[] = {
            {"Low", 0},
            {"Medium", 1},
            {"High", 2},
            {"Ultra", 3}
        };
        mTweakBar->addPadding();
        mTweakBar->addMenu("Quality", mQuality, qualityModes, TWEAKENUM_ARRAYSIZE(qualityModes), REACT_QUALITY_MODE);

        mTweakBar->addPadding();
        var = mTweakBar->addValue("Wireframe", mWireframe);
        addTweakKeyBind(var, 'W');
        var = mTweakBar->addValue("Animate", mAnimate);
        addTweakKeyBind(var, 'A');
        var = mTweakBar->addValue("Cull to Frustum", mCull);
        addTweakKeyBind(var, 'C');

        mTweakBar->addPadding();
        var = mTweakBar->addValue("Auto LOD", mLod);
        mTweakBar->subgroupSwitchStart(var);
            mTweakBar->subgroupSwitchCase(true);
                var = mTweakBar->addValue("Triangle size", mParams.triSize, 1.0f, 50.0f, 1.0f);
                addTweakKeyBind(var, ']', '[');
            mTweakBar->subgroupSwitchCase(false);
                mTweakBar->addValue("Inner tessellation factor", mParams.innerTessFactor, 1.0f, 64.0f, 1.0f);
                mTweakBar->addValue("Outer tessellation factor", mParams.outerTessFactor, 1.0f, 64.0f, 1.0f);
        mTweakBar->subgroupSwitchEnd();

        mTweakBar->addValue("Noise frequency", mParams.noiseFreq, 0.0f, 2.0f, 0.05f);
        mTweakBar->addValue("Terrain height", mParams.heightScale, 0.0f, 2.0f, 0.05f);
        mTweakBar->addValue("Noise octaves", (uint32_t&)mParams.noiseOctaves, 1, 12);
    
        mTweakBar->addPadding(2);
        var = mTweakBar->addValue("Reload shaders", mReload, true);
        addTweakKeyBind(var, 'R');

        mTweakBar->syncValues();
    }

    // UI element for display triangle statistics
    if (mFPSText) {
        NvUIRect tr;
        mFPSText->GetScreenRect(tr);
        mStatsText = new NvUIValueText("Triangles", NvUIFontFamily::SANS, mFPSText->GetFontSize(), NvUITextAlign::RIGHT,
                                        mNumPrimitives, NvUITextAlign::RIGHT);
        mStatsText->SetColor(NV_PACKED_COLOR(0x30,0xD0,0xD0,0xB0));
        mStatsText->SetShadow();
        mUIWindow->Add(mStatsText, tr.left, tr.top+tr.height+8);
    }
}

NvUIEventResponse TerrainTessellation::handleReaction(const NvUIReaction& react) {
    switch(react.code) {
    case REACT_QUALITY_MODE:
        // mQuality should have been adjusted already.
        updateQuality();
        return nvuiEventHandled;
    default:
        break;
    }
    return nvuiEventNotHandled;
}

void TerrainTessellation::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );

    CHECK_GL_ERROR();
}

void TerrainTessellation::updateTerrainTex()
{
    GLuint prevFBO = 0;
    // Enum has MANY names based on extension/version
    // but they all map to 0x8CA6
    glGetIntegerv(0x8CA6, (GLint*)&prevFBO);

    mTerrainFbo->bind();
    CHECK_GL_ERROR();
    glDisable(GL_DEPTH_TEST);
    CHECK_GL_ERROR();
    glDisable(GL_BLEND);

    CHECK_GL_ERROR();
    // bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
    glBindBufferBase( GL_UNIFORM_BUFFER, 1, mUBO);
    glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
    CHECK_GL_ERROR();
    glBufferSubData( GL_UNIFORM_BUFFER, 0, sizeof(TessellationParams), &mParams);
    CHECK_GL_ERROR();

    mGenerateTerrainProg->enable();
    CHECK_GL_ERROR();

    glActiveTexture(GL_TEXTURE0);
    CHECK_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, mRandTex);
    CHECK_GL_ERROR();

    NvDrawQuadGL(0, 8);
    CHECK_GL_ERROR();

    mGenerateTerrainProg->disable();
    CHECK_GL_ERROR();

    // restore framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    CHECK_GL_ERROR();
    glViewport(0, 0, m_width, m_height);
    CHECK_GL_ERROR();

    LOGI("Generated terrain texture %d x %d\n", mTerrainFbo->width, mTerrainFbo->height);
}

void TerrainTessellation::draw(void)
{
    glClearColor( 0.7f, 0.8f, 1.0f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    CHECK_GL_ERROR();

    //
    // Compute matrices without the legacy matrix stack support
    //
    nv::matrix4f projectionMatrix;
    nv::perspective( projectionMatrix, 45.0f * 2.0f*NV_PI / 360.0f, (float)m_width/(float)m_height, 0.01f, 100.0f);
    nv::matrix4f invProjection = nv::inverse(projectionMatrix);

    nv::matrix4f viewMatrix;
    viewMatrix = m_transformer->getModelViewMat();
    nv::matrix4f invView = nv::inverse(viewMatrix);

    // compute frustum planes for culling
    nv::vec4f frustumPlanes[6];
    computeFrustumPlanes(viewMatrix, projectionMatrix, frustumPlanes);

    glViewport(0, 0, m_width, m_height);

    glBindProgramPipeline(0);

    //
    // update struct representing UBO
    //
    mParams.ModelView = viewMatrix;
    mParams.ModelViewProjection= projectionMatrix * viewMatrix;
    mParams.Projection = projectionMatrix;
    mParams.InvProjection = invProjection;
    mParams.InvView = invView;

    mParams.cull = mCull;
    mParams.lod = mLod;
    mParams.viewport = nv::vec4f(0.0, 0.0, (float) m_width, (float) m_height);
    mParams.lightDirWorld = mLightDir;
    mParams.lightDir = nv::vec3f(viewMatrix * nv::vec4f(normalize(mLightDir), 0.0));   // transform to eye space
    mParams.time = mTime;
    mParams.eyePosWorld = invView * nv::vec4f(0.0f, 0.0f, 0.0f, 1.0f);

    if (mAnimate) {
        mParams.translate.y -= getFrameDeltaTime()*2.0f;
    }

    for(int i=0; i<6; i++) {
        mParams.frustumPlanes[i] = frustumPlanes[i];
    }

    CHECK_GL_ERROR();
    // bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
    glBindBufferBase( GL_UNIFORM_BUFFER, 1, mUBO);
    glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
    glBufferSubData( GL_UNIFORM_BUFFER, 0, sizeof(TessellationParams), &mParams);
    CHECK_GL_ERROR();

    // enable / disable wireframe
//    glPolygonMode( GL_FRONT_AND_BACK, mWireframe ? GL_LINE : GL_FILL);

    // query number of primitives
    glBeginQuery(GL_PRIMITIVES_GENERATED_EXT, mGPUQuery);
    CHECK_GL_ERROR();

    drawTerrain();
    CHECK_GL_ERROR();

    glEndQuery(GL_PRIMITIVES_GENERATED_EXT);
    CHECK_GL_ERROR();

//    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

    drawSky();
    CHECK_GL_ERROR();

    // update triangle count so we can display in UI
    glGetQueryObjectuiv(mGPUQuery, GL_QUERY_RESULT, &mNumPrimitives);
    mStatsText->SetValue(mNumPrimitives);
    CHECK_GL_ERROR();

    if (mReload) {
        loadShaders();
        mReload = false;
        mTweakBar->syncValues();
    }
}

// calculate frustum planes (in world space) from current projection and view matrices
// http://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
void TerrainTessellation::computeFrustumPlanes(nv::matrix4f &viewMatrix, nv::matrix4f &projMatrix, nv::vec4f *plane)
{
    nv::matrix4f viewProj = projMatrix * viewMatrix;
    plane[0] = viewProj.get_row(3) + viewProj.get_row(0);   // left
    plane[1] = viewProj.get_row(3) - viewProj.get_row(0);   // right
    plane[2] = viewProj.get_row(3) + viewProj.get_row(1);   // bottom
    plane[3] = viewProj.get_row(3) - viewProj.get_row(1);   // top
    plane[4] = viewProj.get_row(3) + viewProj.get_row(2);   // far
    plane[5] = viewProj.get_row(3) - viewProj.get_row(2);   // near
    // normalize planes
    for(int i=0; i<6; i++) {
        float l = length(nv::vec3f(plane[i]));
        plane[i] = plane[i] / l;
    }
}

// test if sphere is entirely contained within frustum planes
bool TerrainTessellation::sphereInFrustum(nv::vec3f pos, float r, nv::vec4f *plane)
{
    nv::vec4f hp = nv::vec4f(pos, 1.0f);
    for(int i=0; i<6; i++) {
        if (dot(hp, plane[i]) + r < 0.0f) {
            // sphere outside plane
            return false;
        }
    }
    return true;
}

// set tessellation quality
void TerrainTessellation::updateQuality()
{
    switch(mQuality) {
    case 0:
        mParams.gridW = mParams.gridH = 16;
        mParams.tileSize = nv::vec3f(1.0f, 0.0f, 1.0f);
        mParams.noiseOctaves = 8;
        break;
    case 1:
        mParams.gridW = mParams.gridH = 32;
        mParams.tileSize = nv::vec3f(0.5f, 0.0f, 0.5f);
        mParams.noiseOctaves = 9;
        break;
    case 2:
        mParams.gridW = mParams.gridH = 64;
        mParams.tileSize = nv::vec3f(0.25f, 0.0f, 0.25f);
        mParams.noiseOctaves = 10;
        break;
    case 3:
        mParams.gridW = mParams.gridH = 128;
        mParams.tileSize = nv::vec3f(0.125f, 0.0f, 0.125f);
        mParams.noiseOctaves = 11;
        break;
    }

    mParams.gridOrigin = nv::vec3f(-mParams.tileSize.x*mParams.gridW*0.5f, 0.0f, -mParams.tileSize.z*mParams.gridH*0.5f);

    nv::vec3f halfTileSize = nv::vec3f(mParams.tileSize.x, mParams.heightScale, mParams.tileSize.z)*0.5f;
    mParams.tileBoundingSphereR = length(halfTileSize);

    // update texture
    //initTerrainFbo();
    //updateTerrainTex();
}

void TerrainTessellation::drawTerrain()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    CHECK_GL_ERROR();

    //set up the program stage independently
    glBindProgramPipeline(mTerrainPipeline);
    CHECK_GL_ERROR();
    glUseProgramStages(mTerrainPipeline, GL_VERTEX_SHADER_BIT, mTerrainVertexProg);
    CHECK_GL_ERROR();
    glUseProgramStages(mTerrainPipeline, GL_TESS_CONTROL_SHADER_BIT_EXT, mTerrainTessControlProg);
    CHECK_GL_ERROR();
    glUseProgramStages(mTerrainPipeline, GL_TESS_EVALUATION_SHADER_BIT_EXT, mTerrainTessEvalProg);
    if (mWireframe)
    {
        glUseProgramStages(mTerrainPipeline, GL_GEOMETRY_SHADER_BIT_EXT, mWireframeGeometryProg);
        CHECK_GL_ERROR();
        glUseProgramStages( mTerrainPipeline, GL_FRAGMENT_SHADER_BIT, mWireframeFragmentProg);
    }
    else
    {
        glUseProgramStages(mTerrainPipeline, GL_GEOMETRY_SHADER_BIT_EXT, 0);
        CHECK_GL_ERROR();
        glUseProgramStages( mTerrainPipeline, GL_FRAGMENT_SHADER_BIT, mTerrainFragmentProg);
    }
    CHECK_GL_ERROR();

    glActiveTexture(GL_TEXTURE0);
    CHECK_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, mRandTex);
    CHECK_GL_ERROR();

    glActiveTexture(GL_TEXTURE1);
    CHECK_GL_ERROR();
    glBindTexture(GL_TEXTURE_3D, mRandTex3D);
    CHECK_GL_ERROR();

    //draw patches
    glPatchParameteriEXT( GL_PATCH_VERTICES_EXT, 1);

    int instances = mParams.gridW*mParams.gridH;
    glDrawArraysInstanced( GL_PATCHES_EXT, 0, 1, instances);

    glBindProgramPipeline(0);

    glDisable(GL_CULL_FACE);
}

void TerrainTessellation::drawQuad(float z)
{
    GLfloat v[] = {
        -1.0f, -1.0f, z,
         1.0f, -1.0f, z,
        -1.0f,  1.0f, z,
         1.0f,  1.0f, z,
    };

    glBindBuffer( GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3, v);
    glEnableVertexAttribArray( 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray( 0);
}

void TerrainTessellation::drawSky()
{
    glEnable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, mRandTex3D);

    mSkyProg->enable();
    drawQuad(0.9999f);
    mSkyProg->disable();

    glBindTexture(GL_TEXTURE_3D, 0);
}

NvAppBase* NvAppFactory() {
    return new TerrainTessellation();
}
