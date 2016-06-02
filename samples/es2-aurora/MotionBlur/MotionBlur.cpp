//----------------------------------------------------------------------------------
// File:        es2-aurora\MotionBlur/MotionBlur.cpp
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
#include "MotionBlur.h"
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
        inverseViewMatUHandle   = getUniformLocation("u_mInverseViewMat");
        skyboxCubemapTexUHandle = getUniformLocation("u_tSkyboxCubemapTex");

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
class MotionBlurShader : public BaseProjNormalShader
{
public:
    MotionBlurShader(void) :
        BaseProjNormalShader("shaders/motionblur.vert",
                             "shaders/motionblur.frag"),
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

//-----------------------------------------------------------------------------
// PUBLIC METHODS, CTOR AND DTOR

MotionBlur::MotionBlur() :
    mRboID(0U),
    mFboID(0U),
    mTargetTexID(0U),
    mSkyBoxTexID(0U),
    mHouseTexID(0U),
    mSceneColorShader(NULL),
    mSkyboxColorShader(NULL),
    mMotionBlurShader(NULL),
    mHouseModel(NULL),
    mSailsModel(NULL),
    mModelColor(0.992f, 0.817f, 0.090f),
    mCurrSailAngle(0),
    mLastSailAngle(0),
    mSailSpeed(350),
    mStretch(1.0f),
    mLastDt(0.0f),
    mDoMotionBlur(true),
    mAnimPaused(false),
    mDrawSky(true)
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -15.0f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

MotionBlur::~MotionBlur()
{
    LOGI("MotionBlur: destroyed\n");

    cleanFBO();
    cleanRendering();
}

// Inherited methods

void MotionBlur::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits   = 24; 
    config.stencilBits = 0; 
    config.apiVer      = NvGLAPIVersionES2();
}

bool MotionBlur::handleGamepadChanged(uint32_t changedPadFlags)
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

void MotionBlur::initRendering(void) {
    // This sample requires at least OpenGL ES 2.0
    if (!requireMinAPIVersion(NvGLAPIVersionES2())) 
        return;

    cleanRendering();

    NvAssetLoaderAddSearchPath("es2-aurora/MotionBlur");

	if (getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL) {
		NvGLSLProgram::setGlobalShaderHeader("#version 130\n");
	}
	else {
		NvGLSLProgram::setGlobalShaderHeader("#version 300 es\n");
	}

	mSceneColorShader  = new SceneColorShader;
    mSkyboxColorShader = new SkyboxColorShader;
    mMotionBlurShader  = new MotionBlurShader;

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

    // Assign some uniform values that never change.
    nv::vec4f lightPositionEye(1.0f, 1.0f, 1.0f, 0.0f);

    mSceneColorShader->enable();
    glUniform4fv(mSceneColorShader->lightPositionUHandle, 1,
                 lightPositionEye._array);
    glUniform1f(mSceneColorShader->lightAmbientUHandle,    0.1f);
    glUniform1f(mSceneColorShader->lightDiffuseUHandle,    0.7f);
    glUniform1f(mSceneColorShader->lightSpecularUHandle,   1.0f);
    glUniform1f(mSceneColorShader->lightShininessUHandle, 64.0f);
    CHECK_GL_ERROR();
    mSceneColorShader->disable();

    // Set some pipeline state settings that do not change
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    CHECK_GL_ERROR();

	NvGLSLProgram::setGlobalShaderHeader(NULL);
}

void MotionBlur::initUI(void) {
    if (mTweakBar) {
        // Show the app title
        mTweakBar->addLabel("Motion Blur ES2.0 Sample App", true);

        mTweakBar->addValue("Pause Animation", mAnimPaused);
        mTweakBar->addValue("Motion Blur", mDoMotionBlur);
        mTweakBar->addValue("Sky Box", mDrawSky);
        mTweakBar->addValue("Sail Speed", mSailSpeed, 0.0f, 600.0f);
        mTweakBar->addValue("Stretch Factor", mStretch, 0.0f, 10.0f);
    }
}

void MotionBlur::reshape(int32_t width, int32_t height)
{
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    initFBO();

    //setting the perspective projection matrix
    nv::perspective(mProjectionMatrix, NV_PI / 3.0f,
                    static_cast<float>(width) /
                    static_cast<float>(height),
                    1.0f, 150.0f);

    //setting the inverse perspective projection matrix
    nv::perspective(mInverseProjMatrix, NV_PI / 3.0f,
                    static_cast<float>(NvSampleApp::m_width) /
                    static_cast<float>(NvSampleApp::m_height),
                    1.0f, 150.0f);
    mInverseProjMatrix = nv::inverse(mInverseProjMatrix);

    CHECK_GL_ERROR();
}

void MotionBlur::draw(void)
{
    // Get the current view matrix (according to user input through mouse,
    // gamepad, etc.)
    mViewMatrix = m_transformer->getModelViewMat();

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
    const nv::matrix4f currSailsXform = houseXform *
                                        sailTransform(mCurrSailAngle);
    const nv::matrix4f prevSailsXform = houseXform *
                                        sailTransform(mLastSailAngle);

    // Cleaning GL state
    freeGLBindings();

    if (!mDoMotionBlur)
        renderSceneUnblurred(houseXform, currSailsXform);
    else
        renderSceneBlurred(houseXform, currSailsXform, prevSailsXform);

    // Cleaning GL state
    freeGLBindings();

    // If the animation is paused, we update these values to keep the motion
    // blur visible in the windmill sails
    if (!mAnimPaused)
    {
        mLastSailAngle = mCurrSailAngle;
        mLastDt = deltaFrameTime;
    }
}

//-----------------------------------------------------------------------------
// PRIVATE METHODS

// Additional rendering setup methods
void MotionBlur::freeGLBindings(void) const
{
    glBindFramebuffer(GL_FRAMEBUFFER,     getMainFBO());
    glBindRenderbuffer(GL_RENDERBUFFER,   0);
    glBindBuffer(GL_ARRAY_BUFFER,         0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D,          0);
    glBindTexture(GL_TEXTURE_CUBE_MAP,    0);
    CHECK_GL_ERROR();
}

void MotionBlur::cleanFBO(void)
{
    if(mFboID != 0U)
        glDeleteFramebuffers(1, &mFboID);

    if(mRboID != 0U)
        glDeleteRenderbuffers(1, &mRboID);

    if(mTargetTexID != 0U)
        glDeleteTextures(1, &mTargetTexID);
}

void MotionBlur::initFBO(void)
{
    // *** Clean up (in case of a reshape)
    cleanFBO();

    // *** TEXTURES (for FBO attachments)

    glGenTextures(1, &mTargetTexID);
    glBindTexture(GL_TEXTURE_2D, mTargetTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 NvSampleApp::m_width, NvSampleApp::m_height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);
    CHECK_GL_ERROR();

    glBindTexture(GL_TEXTURE_2D, 0);

    // *** RENDERBUFFERS (for FBO attachments)

    glGenRenderbuffers(1, &mRboID);
    glBindRenderbuffer(GL_RENDERBUFFER, mRboID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          NvSampleApp::m_width, NvSampleApp::m_height);
    CHECK_GL_ERROR();

    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // *** FRAMEBUFFER OBJECTS (for individual pass)

    glGenFramebuffers(1, &mFboID);
    glBindFramebuffer(GL_FRAMEBUFFER, mFboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, mTargetTexID, 0);
    CHECK_GL_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                             GL_RENDERBUFFER, mRboID);
    CHECK_GL_ERROR();
    GLint st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    LOGI("Frame Buffer Status : %d", st);
    if(st == GL_FRAMEBUFFER_COMPLETE)
        LOGI("GL_FRAMEBUFFER_COMPLETE");

    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
}

void MotionBlur::cleanRendering(void)
{
    glDeleteTextures(1, &mSkyBoxTexID);
    glDeleteTextures(1, &mHouseTexID);

    if(mSceneColorShader)
        delete mSceneColorShader;
    if(mSkyboxColorShader)
        delete mSkyboxColorShader;
    if(mMotionBlurShader)
        delete mMotionBlurShader;

    if(mHouseModel)
        delete mHouseModel;
    if(mSailsModel)
        delete mSailsModel;
}

// Auxiliary methods

// Computes transform for the windmill sails. Assumes the angle has already
// been modified according to time (as to allow for motion blur when the
// animation is stopped)
nv::matrix4f MotionBlur::sailTransform(const float sailAngle) const
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
void MotionBlur::drawScreenAlignedQuad(const unsigned int attrHandle) const
{
    glVertexAttribPointer(attrHandle, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          SKY_QUAD_COORDS);
    glEnableVertexAttribArray(attrHandle);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(attrHandle);
}

// Methods that draw a specific scene component

//  Draws the skybox with lighting in color and depth
void MotionBlur::drawSkybox(void) const
{
    if (!mDrawSky)
        return;

    // Skybox computations use the camera matrix (inverse of the view matrix)
    const nv::matrix4f invVM = nv::inverse(mViewMatrix);

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
void MotionBlur::drawModelUnblurred(NvModelGL* pModel,
                                    const nv::matrix4f& modelMatrix) const
{
    const nv::matrix4f mv = mViewMatrix * modelMatrix;

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
    //      MotionBlur::initRendering

    pModel->drawElements(mSceneColorShader->positionAHandle,
                         mSceneColorShader->normalAHandle,
                         mSceneColorShader->texcoordAHandle);

    mSceneColorShader->disable();
}

//  Draws the stretched vertices (with lighting)
void MotionBlur::drawModelBlurred(NvModelGL* pModel,
                                  const nv::matrix4f& modelMatrix,
                                  const nv::matrix4f& prevModelMatrix) const
{
    //NOTE: NormalMatrix now contains a 3x3 inverse component of the
    //      ModelMatrix; hence, we need to transpose this before sending it to
    //      the shader
    nv::matrix4f n = nv::transpose(nv::inverse(modelMatrix));

    mMotionBlurShader->enable();

    // In BaseProjectionShader
    glUniformMatrix4fv(mMotionBlurShader->projectionMatUHandle,
                       1, GL_FALSE, mProjectionMatrix._array);

    // In BaseProjNormalShader
    glUniformMatrix4fv(mMotionBlurShader->normalMatUHandle,
                       1, GL_FALSE, n._array);

    // In MotionBlurShader
    glUniformMatrix4fv(mMotionBlurShader->viewMatUHandle,
                       1, GL_FALSE, mViewMatrix._array);
    glUniformMatrix4fv(mMotionBlurShader->currentModelMatUHandle,
                       1, GL_FALSE, modelMatrix._array);
    glUniformMatrix4fv(mMotionBlurShader->previousModelMatUHandle,
                       1, GL_FALSE, prevModelMatrix._array);

    glUniform1f(mMotionBlurShader->stretchScaleUHandle, mStretch);

    // Takes the previous color values into account (as it blends with them)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTargetTexID);
    glUniform1i(mMotionBlurShader->colorTexUHandle, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pModel->drawElements(mMotionBlurShader->positionAHandle,
                         mMotionBlurShader->normalAHandle);

    glDisable(GL_BLEND);

    mMotionBlurShader->disable();
}

// Methods that render results to the screen

// Renders the color buffer for the scene and skybox
void MotionBlur::renderSceneUnblurred(const nv::matrix4f& houseXform,
                                      const nv::matrix4f& sailsXform) const
{
    // Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Yellow, high-contrast if we're not showing the sky box.
    glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelUnblurred(mSailsModel, sailsXform);
    drawModelUnblurred(mHouseModel, houseXform);
    glDepthMask(false);
    drawSkybox();
    glDepthMask(true);
}

// Renders the result of the "MotionBlur" sample for comparison purposes.
// NOTE: this cannot show fullscreen motion blur.
void MotionBlur::renderSceneBlurred(const nv::matrix4f& houseXform,
                                    const nv::matrix4f& sailsXform,
                                    const nv::matrix4f& prevSailsXform) const
{    
    // Render to color/depth FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mFboID);
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelUnblurred(mSailsModel, sailsXform);

    // Pass 2: Render the motion blurred moving geometry over static geometry.
    // Render the Static geometry's depth, Use pass2 (color texture) as motion
    // blur lookup and write onto Pass1 (color texture).
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

    // Yellow, high-contrast if we're not showing the sky box.
    glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModelUnblurred(mHouseModel, houseXform);
    glDepthMask(false);
    drawSkybox();
    drawModelBlurred(mSailsModel, sailsXform, prevSailsXform);
    glDepthMask(true);
}

//-----------------------------------------------------------------------------
// FUNCTION NEEDED BY THE FRAMEWORK

NvAppBase* NvAppFactory()
{
    return new MotionBlur();
}
