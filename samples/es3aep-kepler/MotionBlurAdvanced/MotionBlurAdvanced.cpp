//----------------------------------------------------------------------------------
// File:        es3aep-kepler\MotionBlurAdvanced/MotionBlurAdvanced.cpp
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
#include "MotionBlurAdvanced.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvImageGL.h"
#include "NvGLUtils/NvModelGL.h"
#include "NvGLUtils/NvShapesGL.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NV/NvPlatformGL.h"

#include <ctime>
#include <cstdlib>

// (NOTE: this sample requires either OpenGL ES 3.0 or OpenGL 4.0 as a 
//        minimum API version)
// true is OpenGL ES 3.0, false is OpenGL 4.x
#define USE_OPENGL_ES (true)
//#define USE_OPENGL_ES (false)

// In "MotionBlur" sample, this is controlled by a slider in the TweakBar
// It is just hardcoded in this sample (since we only want it for comparison
// purposes).
#define STRETCH_GLES (1.0f)

#define RANDOM_TEXTURE_SIZE (256U)

// The sample has a dropdown menu in the TweakBar that allows the user to
// select a specific stage or pass of the algorithm for visualization.
// These are the options in that menu (for more details, please see the
// sample documentation)
static NvTweakEnum<uint32_t> RENDER_OPTIONS[] =
{
    {"Color only", 0},
    {"Depth only", 1},
    {"Velocity", 2},
    {"TileMax", 3},
    {"NeighborMax", 4},
    {"Gather (final result)", 5},
    {"MotionBlurES2", 6}
};

// Helper constant buffer for drawing screen-aligned quads
static const float SKY_QUAD_COORDS[] = { -1.0f, -1.0f, -1.0f, 1.0f,
                                          1.0f, -1.0f, -1.0f, 1.0f,
                                         -1.0f,  1.0f, -1.0f, 1.0f,
                                          1.0f,  1.0f, -1.0f, 1.0f};

//-----------------------------------------------------------------------------
// SHADER CLASS HIERARCHY
// Used for simplicity of keeping track of (usually repeated) attribute and
// uniform handles. Also helps for keeping vertex shader file structure more
// consolidated.

// Top of our shader class hierarchy; it handles vertex position
// (All other vertex shaders need it)
class BaseShader : public NvGLSLProgram
{
public:
    BaseShader(const char *vertexProgramPath, 
               const char *fragmentProgramPath) :
        positionAHandle(-1)
    {
        setSourceFromFiles(vertexProgramPath, fragmentProgramPath);

        // vertex attributes
        positionAHandle = getAttribLocation("a_vPosition");

        CHECK_GL_ERROR();
    }

    GLint positionAHandle;
};

// Extends BaseShader with a projection matrix; used by passes that do not
// process an FBO as input
class BaseProjectionShader  : public BaseShader
{
public:
    BaseProjectionShader(const char *vertexProgramPath,
                         const char *fragmentProgramPath) :
        BaseShader(vertexProgramPath, fragmentProgramPath),
        projectionMatUHandle(-1)
    {
        // uniforms
        projectionMatUHandle = getUniformLocation("u_mProjectionMat");

        CHECK_GL_ERROR();
    }

    GLint projectionMatUHandle;
};

// Used to draw the color and depth of the skybox
class SkyboxColorShader : public BaseProjectionShader
{
public:
    SkyboxColorShader(void):
        BaseProjectionShader("shaders/skyboxcolor.vert",
                             "shaders/skyboxcolor.frag"),
        inverseViewMatUHandle(-1),
        skyboxCubemapTexUHandle(-1)
    {
        // uniforms
        inverseViewMatUHandle    = getUniformLocation("u_mInverseViewMat");
        skyboxCubemapTexUHandle  = getUniformLocation("u_tSkyboxCubemapTex");

        CHECK_GL_ERROR();
    }

    GLint inverseViewMatUHandle;
    GLint skyboxCubemapTexUHandle;
};

// Extends BaseProjectionShader with normal vertex information; used for
// shading illuminated objects
class BaseProjNormalShader : public BaseProjectionShader
{
public:
    BaseProjNormalShader(const char *vertexProgramPath,
                         const char *fragmentProgramPath) :
        BaseProjectionShader(vertexProgramPath, fragmentProgramPath),
        normalAHandle(-1),
        normalMatUHandle(-1)
    {
        // vertex attributes
        normalAHandle    = getAttribLocation("a_vNormal");

        // uniforms
        normalMatUHandle = getUniformLocation("u_mNormalMat");

        CHECK_GL_ERROR();
    }

    GLint normalAHandle;

    GLint normalMatUHandle;
};

// Used to draw the color and depth of the scene geometry
class SceneColorShader : public BaseProjNormalShader
{
public:
    SceneColorShader(void) :
        BaseProjNormalShader("shaders/scenecolor.vert",
                             "shaders/scenecolor.frag"),
        texcoordAHandle(-1),
        modelViewMatUHandle(-1),
        lightPositionUHandle(-1),
        lightAmbientUHandle(-1),
        lightDiffuseUHandle(-1),
        lightSpecularUHandle(-1),
        lightShininessUHandle(-1),
        diffuseTexUHandle(-1)
    {
        // vertex attributes
        texcoordAHandle       = getAttribLocation("a_vTexCoord");

        // uniforms
        modelViewMatUHandle   = getUniformLocation("u_mModelViewMat");
        lightPositionUHandle  = getUniformLocation("u_vLightPosition");
        lightAmbientUHandle   = getUniformLocation("u_fLightAmbient");
        lightDiffuseUHandle   = getUniformLocation("u_fLightDiffuse");
        lightSpecularUHandle  = getUniformLocation("u_fLightSpecular");
        lightShininessUHandle = getUniformLocation("u_fLightShininess");
        diffuseTexUHandle     = getUniformLocation("u_tDiffuseTex");

        CHECK_GL_ERROR();
    }

    GLint texcoordAHandle;

    GLint modelViewMatUHandle;
    GLint lightPositionUHandle;
    GLint lightAmbientUHandle;
    GLint lightDiffuseUHandle;
    GLint lightSpecularUHandle;
    GLint lightShininessUHandle;
    GLint diffuseTexUHandle;
};

// Implementation of "MotionBlur", which shows motion blur by stretching some
// vertices in the opposite direction of movement.
class MotionBlurES2Shader : public BaseProjNormalShader
{
public:
    MotionBlurES2Shader(void) :
        BaseProjNormalShader("shaders/motionblures2.vert",
                             "shaders/motionblures2.frag"),
        viewMatUHandle(-1),
        currentModelMatUHandle(-1),
        previousModelMatUHandle(-1),
        stretchScaleUHandle(-1),
        colorTexUHandle(-1)
    {
        // uniforms
        viewMatUHandle =
            getUniformLocation("u_mViewMat");
        currentModelMatUHandle =
            getUniformLocation("u_mCurrentModelMat");
        previousModelMatUHandle =
            getUniformLocation("u_mPreviousModelMat");
        stretchScaleUHandle =
            getUniformLocation("u_fStretchScale");
        colorTexUHandle =
            getUniformLocation("u_tColorTex");

        CHECK_GL_ERROR();
    }

    GLint viewMatUHandle;
    GLint currentModelMatUHandle;
    GLint previousModelMatUHandle;
    GLint stretchScaleUHandle;
    GLint colorTexUHandle;
};

// Extends BaseProjectionShader with the fundamentals for computing vertex
// velocities for a G-buffer
class BaseVelocityShader : public BaseProjectionShader
{
public:
    BaseVelocityShader(const char *vertexProgramPath,
                       const char *fragmentProgramPath) :
        BaseProjectionShader(vertexProgramPath, fragmentProgramPath),
        halfExposureXFramerateUHandle(-1),
        kUHandle(-1)
    {
        // uniforms
        halfExposureXFramerateUHandle = 
            getUniformLocation("u_fHalfExposureXFramerate");
        kUHandle = 
            getUniformLocation("u_fK");

        CHECK_GL_ERROR();
    }

    GLint halfExposureXFramerateUHandle;
    GLint kUHandle;
};

// Computes vertex velocities in scene geometry
class SceneVelocityShader : public BaseVelocityShader
{
public:
    SceneVelocityShader(void) :
        BaseVelocityShader("shaders/scenevelocity.vert",
                           "shaders/scenevelocity.frag"),
        currentModelMatUHandle(-1),
        previousModelMatUHandle(-1),
        currentViewMatUHandle(-1),
        previousViewMatUHandle(-1)
    {
        // uniforms
        currentModelMatUHandle =
            getUniformLocation("u_mCurrentModelMat");
        previousModelMatUHandle = 
            getUniformLocation("u_mPreviousModelMat");
        currentViewMatUHandle = 
            getUniformLocation("u_mCurrentViewMat");
        previousViewMatUHandle = 
            getUniformLocation("u_mPreviousViewMat");

        CHECK_GL_ERROR();
    }

    GLint currentModelMatUHandle;
    GLint previousModelMatUHandle;
    GLint currentViewMatUHandle;
    GLint previousViewMatUHandle;
};

// Computes vertex velocities in the skybox
class SkyboxVelocityShader : public BaseVelocityShader
{
public:
    SkyboxVelocityShader(void) :
        BaseVelocityShader("shaders/skyboxvelocity.vert",
                           "shaders/skyboxvelocity.frag"),
        currentInverseViewMatUHandle(-1),
        previousInverseViewMatUHandle(-1)
    {
        // uniforms
        currentInverseViewMatUHandle =
            getUniformLocation("u_mCurrentInverseViewMat");
        previousInverseViewMatUHandle =
            getUniformLocation("u_mPreviousInverseViewMat");

        CHECK_GL_ERROR();
    }

    GLint currentInverseViewMatUHandle;
    GLint previousInverseViewMatUHandle;
};

// Extends BaseShader with an input texture from a previous render-to-FBO pass.
// Used as the base for further single-input render-to-FBO passes
class BaseFullscreenShader : public BaseShader
{
public:
    BaseFullscreenShader(const char *vertexProgramPath,
                         const char *fragmentProgramPath) :
        BaseShader(vertexProgramPath, fragmentProgramPath),
        inputTexUHandle(-1)
    {
        // uniforms
        inputTexUHandle = getUniformLocation("u_tInputTex");

        CHECK_GL_ERROR();
    }

    GLint inputTexUHandle;
};

// Extends BaseFullscreenShader with texture coordinates. This shader provides
// for simple fullscreen blits with no futher operations.
// NOTE: these are not assumed to be used in all fullscreen passes, as some of
//       them rely on integer-indexed, unfiltered texel fetches.
class TexcoordFullscreenShader : public BaseFullscreenShader
{
public:
    TexcoordFullscreenShader(
        const char *fragmentProgramPath = "shaders/texcoordfullscreen.frag") :
        BaseFullscreenShader("shaders/texcoordfullscreen.vert",
                             fragmentProgramPath),
        texcoordAHandle(-1)
    {
        // vertex attributes
        texcoordAHandle = getAttribLocation("a_vTexCoord");

        CHECK_GL_ERROR();
    }

    GLint texcoordAHandle;
};

// Modifies the blit in TexcoordFullscreenShader to show depth values (the input
// texture is assumed to be the depth attachment of a previous render-to-FBO
// pass).
class DepthDrawShader : public TexcoordFullscreenShader
{
public:
    DepthDrawShader() :
        TexcoordFullscreenShader("shaders/depthdraw.frag")
    {
        CHECK_GL_ERROR();
    }
};

// Implementation of the TileMax pass in the algorithm. This is a fullscreen
// **downsampling** render-to-FBO operation; it creates tiles that represent
// the dominant velocity inside the tile.
class TileMaxShader : public BaseFullscreenShader
{
public:
    TileMaxShader(void) :
        BaseFullscreenShader("shaders/baseshader.vert",
                             "shaders/tilemax.frag"),
        kUHandle(-1)
    {
        kUHandle = getUniformLocation("u_iK");

        CHECK_GL_ERROR();
    }

    GLint kUHandle;
};

// Implementation of the NeighborMax pass in the algorithm. This is a
// fullscreen render-to-FBO operation that spreads the dominant velocity of a
// tile across its neighbors if their respective dominant velocities have a
// lower magnitude. It helps reduce the effect of velocity local maxima.
class NeighborMaxShader : public BaseFullscreenShader
{
public:
    NeighborMaxShader(void) :
        BaseFullscreenShader("shaders/baseshader.vert",
                             "shaders/neighbormax.frag")
    {
        CHECK_GL_ERROR();
    }
};

// Final gather (or reconstruction) filter pass; it uses the dominant
// velocities in each tile and blurs color values along that direction.
class GatherShader : public BaseShader
{
public:
    GatherShader(void) :
        BaseShader("shaders/baseshader.vert",
                   "shaders/gather.frag"),
        colorTexUHandle(-1),
        velocityTexUHandle(-1),
        depthTexUHandle(-1),
        neighborMaxTexUHandle(-1),
        randomTexUHandle(-1),
        kUHandle(-1),
        sUHandle(-1),
        halfExposureUHandle(-1),
        maxSampleTapDistanceUHandle(-1)
    {
        // uniforms
        colorTexUHandle             = getUniformLocation("u_tColorTex");
        velocityTexUHandle          = getUniformLocation("u_tVelocityTex");
        depthTexUHandle             = getUniformLocation("u_tDepthTex");
        neighborMaxTexUHandle       = getUniformLocation("u_tNeighborMaxTex");
        randomTexUHandle            = getUniformLocation("u_tRandomTex");
        kUHandle                    = getUniformLocation("u_iK");
        sUHandle                    = getUniformLocation("u_iS");
        halfExposureUHandle         = getUniformLocation("u_fHalfExposure");
        maxSampleTapDistanceUHandle =
            getUniformLocation("u_fMaxSampleTapDistance");

        CHECK_GL_ERROR();
    }

    GLint colorTexUHandle;
    GLint velocityTexUHandle;
    GLint depthTexUHandle;
    GLint neighborMaxTexUHandle;
    GLint randomTexUHandle;
    GLint kUHandle;
    GLint sUHandle;
    GLint halfExposureUHandle;
    GLint maxSampleTapDistanceUHandle;
};

//-----------------------------------------------------------------------------
// PUBLIC METHODS, CTOR AND DTOR

MotionBlurAdvanced::MotionBlurAdvanced() :
    mNormalRboID(0U),
    mSmallRboID(0U),
    mCZFboID(0U),
    mVFboID(0U),
    mTileMaxFboID(0U),
    mNeighborMaxFboID(0U),
    mCTexID(0U),
    mZTexID(0U),
    mVTexID(0U),
    mTileMaxTexID(0U),
    mNeighborMaxTexID(0U),
    mSkyBoxTexID(0U),
    mHouseTexID(0U),
    mRandomTexID(0U),
    mSceneColorShader(NULL),
    mSkyboxColorShader(NULL),
    mDepthDrawShader(NULL),
    mSceneVelocityShader(NULL),
    mSkyBoxVelocityShader(NULL),
    mTileMaxShader(NULL),
    mNeighborMaxShader(NULL),
    mGatherShader(NULL),
    mTexcoordFullscreenShader(NULL),
    mMotionBlurES2Shader(NULL),
    mHouseModel(NULL),
    mSailsModel(NULL),
    mModelColor(0.992f, 0.817f, 0.090f),
    mRenderOptions(5U), // Default: final gather pass
    mK(4U),
    mLastK(0U),
    mS(15U),
    mMaxSampleTapDistance(6),
    mCurrSailAngle(0),
    mLastSailAngle(0),
    mSailSpeed(350),
    mExposure(1.0f),
    mLastDt(0.0f),
    mFirstFrameFlag(true),
    mAnimPaused(false),
    mDrawSky(true)
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -15.0f));

    mLastK = mK;

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

MotionBlurAdvanced::~MotionBlurAdvanced()
{
    LOGI("MotionBlurAdvanced: destroyed\n");

    cleanFBO();
    cleanRendering();
}

// Inherited methods

void MotionBlurAdvanced::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 

    // VC++ throws a warning when using constants in conditional expressions
    // http://msdn.microsoft.com/en-us/library/6t66728h(v=vs.90).aspx
    // (These are ignored by the Android NDK's gcc)
#ifdef _WIN32
#pragma warning( disable : 4127 )
#endif
    if(USE_OPENGL_ES)
#ifdef _WIN32
#pragma warning( default : 4127 )
#endif
    {
        config.apiVer = NvGLAPIVersionES3();
    }
    else
    {
        config.apiVer = NvGLAPIVersionGL4();
    }
}

bool MotionBlurAdvanced::handleGamepadChanged(uint32_t changedPadFlags)
{
    if (changedPadFlags)
    {
        NvGamepad* pad = getPlatformContext()->getGamepad();
        if (!pad)
            return false;

        LOGI("gamepads: 0x%08x", changedPadFlags);
    }

    return false;
}

void MotionBlurAdvanced::initRendering(void) {
    // This sample requires at least OpenGL ES 3.0 or OpenGL 4.0
    if (!requireMinAPIVersion(NvGLAPIVersionES3())) 
        return;

    cleanRendering();

    NvAssetLoaderAddSearchPath("es3aep-kepler/MotionBlurAdvanced");

    mSceneColorShader         = new SceneColorShader;
    mSkyboxColorShader        = new SkyboxColorShader;
    mDepthDrawShader          = new DepthDrawShader;
    mSceneVelocityShader      = new SceneVelocityShader;
    mSkyBoxVelocityShader     = new SkyboxVelocityShader;
    mTileMaxShader            = new TileMaxShader;
    mNeighborMaxShader        = new NeighborMaxShader;
    mGatherShader             = new GatherShader;
    mTexcoordFullscreenShader = new TexcoordFullscreenShader;
    mMotionBlurES2Shader      = new MotionBlurES2Shader;

	// Load model data for scene geometry
	int32_t length;
	char *modelData = NvAssetLoaderRead("models/house.obj", length);
	mHouseModel = NvModelGL::CreateFromObj(
		(uint8_t *)modelData, -1.0f, true, false);
	NvAssetLoaderFree(modelData);

	modelData = NvAssetLoaderRead("models/fan.obj", length);
	mSailsModel = NvModelGL::CreateFromObj(
		(uint8_t *)modelData, -1.0f, true, false);
	NvAssetLoaderFree(modelData);

	// Load some scene textures
    mHouseTexID =
        NvImageGL::UploadTextureFromDDSFile("textures/windmill_diffuse.dds");
    glBindTexture(GL_TEXTURE_2D, mHouseTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    CHECK_GL_ERROR();

    mSkyBoxTexID = NvImageGL::UploadTextureFromDDSFile("textures/sky_cube.dds");
    glBindTexture(GL_TEXTURE_CUBE_MAP, mSkyBoxTexID);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    CHECK_GL_ERROR();

    // Assign some values which never change.
    nv::vec4f lightPositionEye(1.0f, 1.0f, 1.0f, 0.0f);

    mSceneColorShader->enable();
    glUniform4fv(mSceneColorShader->lightPositionUHandle, 1,
                 lightPositionEye._array);
    glUniform1f(mSceneColorShader->lightAmbientUHandle,   0.1f);
    glUniform1f(mSceneColorShader->lightDiffuseUHandle,   0.7f);
    glUniform1f(mSceneColorShader->lightSpecularUHandle,  1.0f);
    glUniform1f(mSceneColorShader->lightShininessUHandle, 64.0f);
    mSceneColorShader->disable();

    CHECK_GL_ERROR();

    // Set some pipeline state settings that do not change
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    CHECK_GL_ERROR();
}

void MotionBlurAdvanced::initUI(void) {
    if (mTweakBar) {
        mTweakBar->addValue("Pause Animation", mAnimPaused);
        mTweakBar->addValue("Show Sky", mDrawSky);
        mTweakBar->addPadding();
        mTweakBar->addValue("Sail Speed", mSailSpeed, 0.0f, 600.0f);
        mTweakBar->addValue("Exposure Fraction", mExposure, 0.0f, 1.0f);
        mTweakBar->addValue("Max Blur Radius", mK, 1, 20);
        mTweakBar->addValue("Reconstruction Samples", mS, 1, 20);
        mTweakBar->addPadding();
        mTweakBar->addEnum("Show Buffer", mRenderOptions, RENDER_OPTIONS,
                           TWEAKENUM_ARRAYSIZE(RENDER_OPTIONS));
    }
}

void MotionBlurAdvanced::reshape(int32_t width, int32_t height)
{
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    initFBO();

    computeMaxSampleTapDistance();

    //setting the perspective projection matrix
    nv::perspective(mProjectionMatrix, NV_PI / 3.0f,
                    static_cast<float>(NvSampleApp::m_width) /
                    static_cast<float>(NvSampleApp::m_height),
                    1.0f, 150.0f);

    //setting the inverse perspective projection matrix
    nv::perspective(mInverseProjMatrix, NV_PI / 3.0f,
                    static_cast<float>(NvSampleApp::m_width) /
                    static_cast<float>(NvSampleApp::m_height),
                    1.0f, 150.0f);
    mInverseProjMatrix = nv::inverse(mInverseProjMatrix);

    CHECK_GL_ERROR();
}

void MotionBlurAdvanced::draw(void)
{
    // If K has changed (through the TweakBar UI), update FBOs (since these
    // use K to set sizes)
    if(mLastK != mK)
    {
        LOGI("Resizing tiles...");
        initFBO();
        mLastK = mK;
    }

    // Get the current view matrix (according to user input through mouse,
    // gamepad, etc.)
    mCurrViewMatrix = m_transformer->getModelViewMat();
    if(mFirstFrameFlag)
    {
        // If this is the first frame, there is no movement.
        // (Just handling a corner case)
        mPrevViewMatrix = mCurrViewMatrix;
        mFirstFrameFlag = false;
    }

    float deltaFrameTime = getFrameDeltaTime();

    // Clamp dt.  If the frame rate goes very low, the stretch becomes huge 
    // and the blurred geometry covers more of the screen which in turns 
    // decreases the FPS even further.  The sample gets into a feedback loop
    // of ever-decreasing FPS.
    const float FPS_CLAMP = 1.0f / 15.0f;
    if (deltaFrameTime > FPS_CLAMP)
        deltaFrameTime = FPS_CLAMP;

    // If the animation is pasued, we still want to show motion blur on the
    // windmill sails
    if (mAnimPaused)
        deltaFrameTime = mLastDt;

    mCurrSailAngle = mLastSailAngle + mSailSpeed * deltaFrameTime;

    // houseXform puts the axle of the sails about centre-screen.
    nv::matrix4f tmp;
    const nv::matrix4f houseXform = nv::translation(tmp, 0.0f, -10.0f, 0.0f);
    const nv::matrix4f currSailsXform =
        houseXform * sailTransform(mCurrSailAngle);
    const nv::matrix4f prevSailsXform =
        houseXform * sailTransform(mLastSailAngle);

    // Cleaning GL state
    freeGLBindings();

    // Render the requested content (from dropdown menu in TweakBar UI)
    switch(mRenderOptions)
    {
    case 0: // Color C (regular forward rendering with no blurring)
        renderC(houseXform, currSailsXform);
        break;
    case 1: // Depth Z (regular forward rendering with no blurring)
        renderZ(houseXform, currSailsXform);
        break;
    case 2: // Velocity V
        renderV(houseXform, currSailsXform, prevSailsXform);
        break;
    case 3: // TileMax
        renderTileMax(houseXform, currSailsXform, prevSailsXform);
        break;
    case 4: // NeighborMax
        renderNeighborMax(houseXform, currSailsXform, prevSailsXform);
        break;
    case 5: // Final gather (last pass)
        renderGather(houseXform, currSailsXform, prevSailsXform);
        break;
    case 6: // Old Motion Blur GLES algorithm (for comparison purposes)
        renderMotionBlurGLES(houseXform, currSailsXform, prevSailsXform);
        break;
    default:
        LOGI("ERROR:\t INVALID RENDER OPTION");
        break;
    }

    // Cleaning GL state
    freeGLBindings();

    // If the animation is paused, we update these values to keep the motion
    // blur visible in the windmill sails
    if (!mAnimPaused)
    {
        mLastSailAngle = mCurrSailAngle;
        mLastDt = deltaFrameTime;
    }

    // Update the previous matrix with the current one (for next frame)
    mPrevViewMatrix = mCurrViewMatrix;
}

//-----------------------------------------------------------------------------
// PRIVATE METHODS

// Additional rendering setup methods

void MotionBlurAdvanced::freeGLBindings(void) const
{
    glBindFramebuffer(GL_FRAMEBUFFER,     getMainFBO());
    glBindRenderbuffer(GL_RENDERBUFFER,   0);
    glBindBuffer(GL_ARRAY_BUFFER,         0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D,          0);
    glBindTexture(GL_TEXTURE_CUBE_MAP,    0);
    CHECK_GL_ERROR();
}

void MotionBlurAdvanced::cleanFBO(void)
{
    glDeleteFramebuffers(1, &mCZFboID);
    glDeleteFramebuffers(1, &mVFboID);
    glDeleteFramebuffers(1, &mTileMaxFboID);
    glDeleteFramebuffers(1, &mNeighborMaxFboID);

    glDeleteRenderbuffers(1, &mNormalRboID);
    glDeleteRenderbuffers(1, &mSmallRboID);

    glDeleteTextures(1, &mCTexID);
    glDeleteTextures(1, &mZTexID);
    glDeleteTextures(1, &mVTexID);
    glDeleteTextures(1, &mTileMaxTexID);
    glDeleteTextures(1, &mNeighborMaxTexID);
    glDeleteTextures(1, &mRandomTexID);
}

void MotionBlurAdvanced::initFBO()
{
    GLuint prevFBO = 0;
    // Enum has MANY names based on extension/version
    // but they all map to 0x8CA6
    glGetIntegerv(0x8CA6, (GLint*)&prevFBO);

    // C, Z, V are (width, height); TileMax and NeighborMax are
    // (width/K, height/K)
    int32_t widthDividedByK, heightDividedByK;
    computeTiledDimensions(widthDividedByK, heightDividedByK);

    // *** Clean up (in case of a reshape)
    cleanFBO();

    // *** TEXTURES (for FBO attachments)

    // Random texture (used in gather pass)
    srand( static_cast<unsigned int>( time(NULL) ) );
    GLubyte *r = new unsigned char[RANDOM_TEXTURE_SIZE * RANDOM_TEXTURE_SIZE];
    for(unsigned int i = 0U; i < RANDOM_TEXTURE_SIZE; i++)
    {
        for(unsigned int j = 0U; j < RANDOM_TEXTURE_SIZE; j++)
        {
            GLubyte val = static_cast<GLubyte>( rand() ) &
                          static_cast<GLubyte>(0x00ff);
            r[i * RANDOM_TEXTURE_SIZE + j] = val;
        }
    }

    glGenTextures(1, &mRandomTexID);
    glBindTexture(GL_TEXTURE_2D, mRandomTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                 RANDOM_TEXTURE_SIZE, RANDOM_TEXTURE_SIZE, 0, GL_RED,
                 GL_UNSIGNED_BYTE, static_cast<const GLvoid*>(r));
    CHECK_GL_ERROR();

    delete r;

    // Color C
    glGenTextures(1, &mCTexID);
    glBindTexture(GL_TEXTURE_2D, mCTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                NvSampleApp::m_width, NvSampleApp::m_height, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, NULL);
    CHECK_GL_ERROR();

    // Depth Z
    glGenTextures(1, &mZTexID);
    glBindTexture(GL_TEXTURE_2D, mZTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
                 NvSampleApp::m_width, NvSampleApp::m_height, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    CHECK_GL_ERROR();

    // Velocity V
    glGenTextures(1, &mVTexID);
    glBindTexture(GL_TEXTURE_2D, mVTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 NvSampleApp::m_width, NvSampleApp::m_height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);
    CHECK_GL_ERROR();

    // TileMax
    glGenTextures(1, &mTileMaxTexID);
    glBindTexture(GL_TEXTURE_2D, mTileMaxTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthDividedByK, heightDividedByK,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    CHECK_GL_ERROR();

    // NeighborMax
    glGenTextures(1, &mNeighborMaxTexID);
    glBindTexture(GL_TEXTURE_2D, mNeighborMaxTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthDividedByK, heightDividedByK,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    CHECK_GL_ERROR();

    glBindTexture(GL_TEXTURE_2D, 0);

    // *** RENDERBUFFERS (for FBO attachments)

    glGenRenderbuffers(1, &mNormalRboID);
    glBindRenderbuffer(GL_RENDERBUFFER, mNormalRboID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          NvSampleApp::m_width, NvSampleApp::m_height);
    CHECK_GL_ERROR();

    glGenRenderbuffers(1, &mSmallRboID);
    glBindRenderbuffer(GL_RENDERBUFFER, mSmallRboID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          widthDividedByK, heightDividedByK);
    CHECK_GL_ERROR();

    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // *** FRAMEBUFFER OBJECTS (for individual pass)

    GLint st = 0;

    glGenFramebuffers(1, &mCZFboID);
    glBindFramebuffer(GL_FRAMEBUFFER, mCZFboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, mCTexID, 0);
    CHECK_GL_ERROR();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, mZTexID, 0);
    CHECK_GL_ERROR();
    st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    LOGI("Frame Buffer Status : %d", st);
    if(st == GL_FRAMEBUFFER_COMPLETE)
        LOGI("GL_FRAMEBUFFER_COMPLETE");

    glGenFramebuffers(1, &mVFboID);
    glBindFramebuffer(GL_FRAMEBUFFER, mVFboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, mVTexID, 0);
    CHECK_GL_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, mNormalRboID);
    CHECK_GL_ERROR();
    st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    LOGI("Frame Buffer Status : %d", st);
    if(st == GL_FRAMEBUFFER_COMPLETE)
        LOGI("GL_FRAMEBUFFER_COMPLETE");

    glGenFramebuffers(1, &mTileMaxFboID);
    glBindFramebuffer(GL_FRAMEBUFFER, mTileMaxFboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, mTileMaxTexID, 0);
    CHECK_GL_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, mSmallRboID);
    CHECK_GL_ERROR();
    st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    LOGI("Frame Buffer Status : %d", st);
    if(st == GL_FRAMEBUFFER_COMPLETE)
        LOGI("GL_FRAMEBUFFER_COMPLETE");

    glGenFramebuffers(1, &mNeighborMaxFboID);
    glBindFramebuffer(GL_FRAMEBUFFER, mNeighborMaxFboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, mNeighborMaxTexID, 0);
    CHECK_GL_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, mSmallRboID);
    CHECK_GL_ERROR();
    st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    LOGI("Frame Buffer Status : %d", st);
    if(st == GL_FRAMEBUFFER_COMPLETE)
        LOGI("GL_FRAMEBUFFER_COMPLETE");

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
}

void MotionBlurAdvanced::cleanRendering(void)
{
    glDeleteTextures(1, &mSkyBoxTexID);
    glDeleteTextures(1, &mHouseTexID);

    if(mSceneColorShader)
        delete mSceneColorShader;
    if(mSkyboxColorShader)
        delete mSkyboxColorShader;
    if(mDepthDrawShader)
        delete mDepthDrawShader;
    if(mSceneVelocityShader)
        delete mSceneVelocityShader;
    if(mSkyBoxVelocityShader)
        delete mSkyBoxVelocityShader;
    if(mTileMaxShader)
        delete mTileMaxShader;
    if(mNeighborMaxShader)
        delete mNeighborMaxShader;
    if(mGatherShader)
        delete mGatherShader;
    if(mTexcoordFullscreenShader)
        delete mTexcoordFullscreenShader;
    if(mMotionBlurES2Shader)
        delete mMotionBlurES2Shader;

    if(mHouseModel)
        delete mHouseModel;
    if(mSailsModel)
        delete mSailsModel;
}

// Auxiliary methods

// Computes the appropriate max sample tap distance (our implementation needs
// to work both on small and large resolutions).
// Based on subjective observations, for 10.1" screens (1920x1136 surface)
// we need 8 texels, and in desktop displays (720p by default) we need 6
void MotionBlurAdvanced::computeMaxSampleTapDistance(void)
{
    mMaxSampleTapDistance = (2 * NvSampleApp::m_height + 1056) / 416;
    LOGI("mMaxSampleTapDistance = %d", mMaxSampleTapDistance);
}

// Just a quick sanitization (TweakBar UI slider doesn't allow zero, but just
// to be paranoid).
void MotionBlurAdvanced::computeTiledDimensions(int32_t& widthDividedByK,
                                               int32_t& heightDividedByK) const
{
    int32_t k = (mK > 0) ? mK : 1;
    widthDividedByK  = NvSampleApp::m_width  / k;
    heightDividedByK = NvSampleApp::m_height / k;
}

// Computes transform for the windmill sails. Assumes the angle has already
// been modified according to time (as to allow for motion blur when the
// animation is stopped)
nv::matrix4f MotionBlurAdvanced::sailTransform(const float sailAngle) const
{
    nv::matrix4f tmp;
    nv::matrix4f transBackM = nv::translation(tmp,
                                               mSailsModel->m_center.x,
                                               mSailsModel->m_center.y,
                                               mSailsModel->m_center.z);
    nv::matrix4f transOrigM = nv::translation(tmp,
                                              -mSailsModel->m_center.x,
                                              -mSailsModel->m_center.y,
                                              -mSailsModel->m_center.z);

    nv::matrix4f rotation = nv::rotationZ(tmp, sailAngle / 180.0f * NV_PI);

    return transBackM * rotation * transOrigM;
}

// The framework that we provided does not have NvDrawQuadGL call that takes
// position with 4 components, and I decided not to pass that to the vertex shader.
void MotionBlurAdvanced::drawScreenAlignedQuad(const unsigned int attrHandle) const
{
    glVertexAttribPointer(attrHandle, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          SKY_QUAD_COORDS);
    glEnableVertexAttribArray(attrHandle);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(attrHandle);
}

// Methods that draw a specific scene component

//  Draws the skybox with lighting in color and depth
void MotionBlurAdvanced::drawSkyboxColorDepth(void) const
{
    if (!mDrawSky)
        return;

    // Skybox computations use the camera matrix (inverse of the view matrix)
    const nv::matrix4f invVM = nv::inverse(mCurrViewMatrix);

    mSkyboxColorShader->enable();

    // In BaseProjectionShader
    glUniformMatrix4fv(mSkyboxColorShader->projectionMatUHandle,
                       1, GL_FALSE, mInverseProjMatrix._array);

    // In SkyboxColorShader
    glUniformMatrix4fv(mSkyboxColorShader->inverseViewMatUHandle,
                       1, GL_FALSE, invVM._array);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mSkyBoxTexID);
    glUniform1i(mSkyboxColorShader->skyboxCubemapTexUHandle, 0);

    drawScreenAlignedQuad(mSkyboxColorShader->positionAHandle);

    mSkyboxColorShader->disable();
}

//  Draws a model with lighting in color and depth
void MotionBlurAdvanced::drawModelColorDepth(NvModelGL* pModel,
                                         const nv::matrix4f& modelMatrix) const
{
    const nv::matrix4f mv = mCurrViewMatrix * modelMatrix;

    //NOTE: NormalMatrix now contains a 3x3 inverse component of the
    //      ModelMatrix; hence, we need to transpose this before sending it to
    //      the shader
    const nv::matrix4f n = nv::transpose(nv::inverse(mv));

    mSceneColorShader->enable();

    // In BaseProjectionShader
    glUniformMatrix4fv(mSceneColorShader->projectionMatUHandle,
                       1, GL_FALSE, mProjectionMatrix._array);

    // In BaseProjNormalShader
    glUniformMatrix4fv(mSceneColorShader->normalMatUHandle,
                       1, GL_FALSE, n._array);

    // In SceneColorShader
    glUniformMatrix4fv(mSceneColorShader->modelViewMatUHandle,
                       1, GL_FALSE, mv._array);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mHouseTexID);
    glUniform1i(mSceneColorShader->diffuseTexUHandle, 0);

    //NOTE: the rest of the uniforms are set once in
    //      MotionBlurAdvanced::initRendering

    pModel->drawElements(mSceneColorShader->positionAHandle,
                         mSceneColorShader->normalAHandle,
                         mSceneColorShader->texcoordAHandle);

    mSceneColorShader->disable();
}

//  Draws the stretched vertices (with lighting)
void MotionBlurAdvanced::drawModelMotionBlurES2(NvModelGL* pModel,
                                     const nv::matrix4f& modelMatrix,
                                     const nv::matrix4f& prevModelMatrix) const
{
    //NOTE: NormalMatrix now contains a 3x3 inverse component of the
    //      ModelMatrix; hence, we need to transpose this before sending it to
    //      the shader
    nv::matrix4f n = nv::transpose(nv::inverse(modelMatrix));

    mMotionBlurES2Shader->enable();

    // In BaseProjectionShader
    glUniformMatrix4fv(mMotionBlurES2Shader->projectionMatUHandle,
                       1, GL_FALSE, mProjectionMatrix._array);

    // In BaseProjNormalShader
    glUniformMatrix4fv(mMotionBlurES2Shader->normalMatUHandle,
                       1, GL_FALSE, n._array);

    // In MotionBlurES2Shader
    glUniformMatrix4fv(mMotionBlurES2Shader->viewMatUHandle,
                       1, GL_FALSE, mCurrViewMatrix._array);
    glUniformMatrix4fv(mMotionBlurES2Shader->currentModelMatUHandle,
                       1, GL_FALSE, modelMatrix._array);
    glUniformMatrix4fv(mMotionBlurES2Shader->previousModelMatUHandle,
                       1, GL_FALSE, prevModelMatrix._array);

    glUniform1f(mMotionBlurES2Shader->stretchScaleUHandle, STRETCH_GLES);

    // Takes the previous color values into account (as it blends with them)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mCTexID);
    glUniform1i(mMotionBlurES2Shader->colorTexUHandle, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pModel->drawElements(mMotionBlurES2Shader->positionAHandle,
                         mMotionBlurES2Shader->normalAHandle);

    glDisable(GL_BLEND);

    mMotionBlurES2Shader->disable();
}

// Draws the vertex velocities for a specific model.
// NOTE1: we pass two different model matrices for the windmill sails, but we
//        pass the same one twice for the windmill building (for simplicity's
//        sake).
void MotionBlurAdvanced::drawModelVelocity(NvModelGL* pModel,
                                     const nv::matrix4f& currModelMatrix,
                                     const nv::matrix4f& prevModelMatrix) const
{
    // Exposure is controller by a TweabBar slider
    GLfloat expFPS = 0.5f * mExposure / mLastDt;

    mSceneVelocityShader->enable();

    // In BaseProjectionShader
    glUniformMatrix4fv(mSceneVelocityShader->projectionMatUHandle,
                       1, GL_FALSE, mProjectionMatrix._array);

    // In BaseVelocityShader
    glUniform1f(mSceneVelocityShader->halfExposureXFramerateUHandle, expFPS);
    glUniform1f(mSceneVelocityShader->kUHandle, static_cast<GLfloat>(mK));

    // In SceneVelocityShader
    glUniformMatrix4fv(mSceneVelocityShader->currentModelMatUHandle,
                       1, GL_FALSE, currModelMatrix._array);
    glUniformMatrix4fv(mSceneVelocityShader->previousModelMatUHandle,
                       1, GL_FALSE, prevModelMatrix._array);
    glUniformMatrix4fv(mSceneVelocityShader->currentViewMatUHandle,
                       1, GL_FALSE, mCurrViewMatrix._array);
    glUniformMatrix4fv(mSceneVelocityShader->previousViewMatUHandle,
                       1, GL_FALSE, mPrevViewMatrix._array);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pModel->drawElements(mSceneVelocityShader->positionAHandle);

    glDisable(GL_BLEND);

    mSceneVelocityShader->disable();
}

// Draws the velocities in the skybox. It uses the differences between the
// current and previous view matrices
void MotionBlurAdvanced::drawSkyboxVelocity(void) const
{
    if (!mDrawSky)
        return;

    // Skybox computations use the camera matrix (inverse of the view matrix)
    const nv::matrix4f civm = nv::inverse(mCurrViewMatrix);
    const nv::matrix4f pivm = nv::inverse(mPrevViewMatrix);

    const GLfloat expFPS = 0.5f * mExposure / mLastDt;

    mSkyBoxVelocityShader->enable();

    // In BaseProjectionShader
    glUniformMatrix4fv(mSkyBoxVelocityShader->projectionMatUHandle,
                       1, GL_FALSE, mInverseProjMatrix._array);

    // In BaseVelocityShader
    glUniform1f(mSkyBoxVelocityShader->halfExposureXFramerateUHandle, expFPS);
    glUniform1f(mSkyBoxVelocityShader->kUHandle, static_cast<GLfloat>(mK));

    // In SkyboxVelocityShader
    glUniformMatrix4fv(mSkyBoxVelocityShader->currentInverseViewMatUHandle,
                       1, GL_FALSE, civm._array);
    glUniformMatrix4fv(mSkyBoxVelocityShader->previousInverseViewMatUHandle,
                       1, GL_FALSE, pivm._array);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawScreenAlignedQuad(mSkyBoxVelocityShader->positionAHandle);

    glDisable(GL_BLEND);

    mSkyBoxVelocityShader->disable();
}

// Methods that help with fullscreen passes

void MotionBlurAdvanced::drawFullScreenQuad(BaseFullscreenShader* shd,
                                            const unsigned int texID) const
{
    glDisable(GL_DEPTH_TEST);

    // In BaseFullscreenShader
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(shd->inputTexUHandle, 0);

    NvDrawQuadGL(shd->positionAHandle);

    CHECK_GL_ERROR();

    glEnable(GL_DEPTH_TEST);
}

void MotionBlurAdvanced::drawTexcoordFullScreenQuad(
                                                TexcoordFullscreenShader* shd,
                                                const unsigned int texID) const
{
    glDisable(GL_DEPTH_TEST);

    // In BaseFullscreenShader
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(shd->inputTexUHandle, 0);

    NvDrawQuadGL(shd->positionAHandle, shd->texcoordAHandle);

    CHECK_GL_ERROR();

    glEnable(GL_DEPTH_TEST);
}

// Methods that render results to the screen

// Renders the color buffer for the scene and skybox
void MotionBlurAdvanced::renderC(const nv::matrix4f& houseXform,
                                 const nv::matrix4f& sailsXform) const
{
    // Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Yellow, high-contrast if we're not showing the sky box.
    glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelColorDepth(mSailsModel, sailsXform);
    drawModelColorDepth(mHouseModel, houseXform);
    glDepthMask(false);
    drawSkyboxColorDepth();
    glDepthMask(true);
}

// Renders the depth buffer for the scene and skybox
void MotionBlurAdvanced::renderZ(const nv::matrix4f& houseXform,
                                 const nv::matrix4f& sailsXform) const
{
    // Render to color/depth FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mCZFboID);
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // White (the default depth value)
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelColorDepth(mSailsModel, sailsXform);
    drawModelColorDepth(mHouseModel, houseXform);

    //--------------------------------------

    // Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mDepthDrawShader->enable();
    drawTexcoordFullScreenQuad(mDepthDrawShader, mZTexID);
    mDepthDrawShader->disable();
}

// Renders the velocity buffer for the scene and skybox
void MotionBlurAdvanced::renderV(const nv::matrix4f& houseXform, 
                                 const nv::matrix4f& sailsXform, 
                                 const nv::matrix4f& prevSailsXform) const
{
    // Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Gray (to be consistent with read/scale bias)
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthMask(false);
    drawSkyboxVelocity();
    glDepthMask(true);
    drawModelVelocity(mHouseModel, houseXform, houseXform);
    drawModelVelocity(mSailsModel, sailsXform, prevSailsXform);
}

// Renders the TileMax buffer for visualization
void MotionBlurAdvanced::renderTileMax(const nv::matrix4f& houseXform, 
                                      const nv::matrix4f& sailsXform, 
                                      const nv::matrix4f& prevSailsXform) const
{
    // C, Z, V are (width, height); TileMax and NeighborMax are
    // (width/K, height/K)
    int32_t widthDividedByK, heightDividedByK;
    computeTiledDimensions(widthDividedByK, heightDividedByK);

    //--------------------------------------

    // Render to velocity FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mVFboID);
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Gray (to be consistent with read/scale bias)
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelVelocity(mHouseModel, houseXform, houseXform);
    drawModelVelocity(mSailsModel, sailsXform, prevSailsXform);
    drawSkyboxVelocity();

    //--------------------------------------

    // Render to TileMax FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mTileMaxFboID);
    glViewport(0, 0, widthDividedByK, heightDividedByK);

    // Gray (to be consistent with read/scale bias)
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mTileMaxShader->enable();
    glUniform1i(mTileMaxShader->kUHandle, mK);
    CHECK_GL_ERROR();
    drawFullScreenQuad(mTileMaxShader, mVTexID);
    mTileMaxShader->disable();

    //--------------------------------------

    // Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Yellow, high-contrast if we're not showing the sky box.
    glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mTexcoordFullscreenShader->enable();
    drawTexcoordFullScreenQuad(mTexcoordFullscreenShader, mTileMaxTexID);
    mTexcoordFullscreenShader->disable();
}

// Renders the NeighborMax buffer for visualization
void MotionBlurAdvanced::renderNeighborMax(const nv::matrix4f& houseXform, 
                                      const nv::matrix4f& sailsXform, 
                                      const nv::matrix4f& prevSailsXform) const
{
    // C, Z, V are (width, height); TileMax and NeighborMax are
    // (width/K, height/K)
    int32_t widthDividedByK, heightDividedByK;
    computeTiledDimensions(widthDividedByK, heightDividedByK);

    //--------------------------------------

    //Render to velocity FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mVFboID);
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Gray (to be consistent with read/scale bias)
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelVelocity(mHouseModel, houseXform, houseXform);
    drawModelVelocity(mSailsModel, sailsXform, prevSailsXform);
    drawSkyboxVelocity();

    //--------------------------------------

    // Render to TileMax FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mTileMaxFboID);
    glViewport(0, 0, widthDividedByK, heightDividedByK);

    // Gray (to be consistent with read/scale bias)
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mTileMaxShader->enable();
    glUniform1i(mTileMaxShader->kUHandle, mK);
    CHECK_GL_ERROR();
    drawFullScreenQuad(mTileMaxShader, mVTexID);
    mTileMaxShader->disable();

    //--------------------------------------

    // Render to NeighborMax FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mNeighborMaxFboID);
    glViewport(0, 0, widthDividedByK, heightDividedByK);

    // Gray (to be consistent with read/scale bias)
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mNeighborMaxShader->enable();
    CHECK_GL_ERROR();
    drawFullScreenQuad(mNeighborMaxShader, mTileMaxTexID);
    mNeighborMaxShader->disable();

    //--------------------------------------

    // Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Yellow, high-contrast if we're not showing the sky box.
    glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mTexcoordFullscreenShader->enable();
    drawTexcoordFullScreenQuad(mTexcoordFullscreenShader, mNeighborMaxTexID);
    mTexcoordFullscreenShader->disable();
}

// Renders the final result of the reconstruction filter.
void MotionBlurAdvanced::renderGather(const nv::matrix4f& houseXform, 
                                      const nv::matrix4f& sailsXform, 
                                      const nv::matrix4f& prevSailsXform) const
{
    // C, Z, V are (width, height); TileMax and NeighborMax are
    // (width/K, height/K)
    int32_t widthDividedByK, heightDividedByK;
    computeTiledDimensions(widthDividedByK, heightDividedByK);

    // If S is even, bumping to next odd integer
    // (slider widget does not take a stride for selectable value range)
    int32_t S = ( (mS % 2) == 0 ) ? (mS + 1) : mS;

    //--------------------------------------

    // Render to color/depth FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mCZFboID);
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // White (the default depth value)
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelColorDepth(mSailsModel, sailsXform);
    drawModelColorDepth(mHouseModel, houseXform);
    glDepthMask(false);
    drawSkyboxColorDepth();
    glDepthMask(true);

    //--------------------------------------

    //Render to velocity FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mVFboID);
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Gray (to be consistent with read/scale bias)
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelVelocity(mHouseModel, houseXform, houseXform);
    drawModelVelocity(mSailsModel, sailsXform, prevSailsXform);
    drawSkyboxVelocity();

    //--------------------------------------

    // Render to TileMax FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mTileMaxFboID);
    glViewport(0, 0, widthDividedByK, heightDividedByK);

    // Gray (to be consistent with read/scale bias)
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mTileMaxShader->enable();
    glUniform1i(mTileMaxShader->kUHandle, mK);
    CHECK_GL_ERROR();
    drawFullScreenQuad(mTileMaxShader, mVTexID);
    mTileMaxShader->disable();

    //--------------------------------------

    // Render to NeighborMax FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mNeighborMaxFboID);
    glViewport(0, 0, widthDividedByK, heightDividedByK);

    // Gray (to be consistent with read/scale bias)
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mNeighborMaxShader->enable();
    CHECK_GL_ERROR();
    drawFullScreenQuad(mNeighborMaxShader, mTileMaxTexID);
    mNeighborMaxShader->disable();

    //--------------------------------------

    // Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Yellow, high-contrast if we're not showing the sky box.
    glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mGatherShader->enable();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mCTexID);
    glUniform1i(mGatherShader->colorTexUHandle, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mVTexID);
    glUniform1i(mGatherShader->velocityTexUHandle, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mZTexID);
    glUniform1i(mGatherShader->depthTexUHandle, 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mNeighborMaxTexID);
    glUniform1i(mGatherShader->neighborMaxTexUHandle, 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mRandomTexID);
    glUniform1i(mGatherShader->randomTexUHandle, 4);

    glUniform1i(mGatherShader->kUHandle, mK);
    glUniform1i(mGatherShader->sUHandle, S);

    glUniform1f(mGatherShader->halfExposureUHandle,
                static_cast<GLfloat>(mExposure * 0.5f));
    glUniform1f(mGatherShader->maxSampleTapDistanceUHandle,
                static_cast<GLfloat>(mMaxSampleTapDistance));

    glDisable(GL_DEPTH_TEST);

    NvDrawQuadGL(mGatherShader->positionAHandle);

    glEnable(GL_DEPTH_TEST);

    mGatherShader->disable();
}

// Renders the result of the "MotionBlur" sample for comparison purposes.
// NOTE: this cannot show fullscreen motion blur.
void MotionBlurAdvanced::renderMotionBlurGLES(const nv::matrix4f& houseXform,
                                      const nv::matrix4f& sailsXform,
                                      const nv::matrix4f& prevSailsXform) const
{    
    // Render to color/depth FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mCZFboID);
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelColorDepth(mSailsModel, sailsXform);

    // Pass 2: Render the motion blurred moving geometry over static geometry.
    // Render the Static geometry's depth, Use pass2 (color texture) as motion
    // blur lookup and write onto Pass1 (color texture).
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Yellow, high-contrast if we're not showing the sky box.
    glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelColorDepth(mHouseModel, houseXform);
    glDepthMask(false);
    drawSkyboxColorDepth();
    drawModelMotionBlurES2(mSailsModel, sailsXform, prevSailsXform);
    glDepthMask(true);
}

//-----------------------------------------------------------------------------
// FUNCTION NEEDED BY THE FRAMEWORK

NvAppBase* NvAppFactory()
{
    return new MotionBlurAdvanced();
}
