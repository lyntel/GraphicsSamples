//----------------------------------------------------------------------------------
// File:        es3aep-kepler\MotionBlurAdvanced/MotionBlurAdvanced.h
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
#include "NvGamepad/NvGamepad.h"

class NvStopWatch;
class NvFramerateCounter;
class NvModelGL;
class SceneColorShader;
class SkyboxColorShader;
class DepthDrawShader;
class SceneVelocityShader;
class SkyboxVelocityShader;
class TileMaxShader;
class NeighborMaxShader;
class GatherShader;
class BaseFullscreenShader;
class TexcoordFullscreenShader;
class MotionBlurES2Shader;

class MotionBlurAdvanced : public NvSampleAppGL
{
public:
    MotionBlurAdvanced();
    ~MotionBlurAdvanced();

    // Inherited methods
    void configurationCallback(NvGLConfiguration& config);
    bool handleGamepadChanged(uint32_t changedPadFlags);
    void initRendering(void);
    void initUI(void);
    void reshape(int32_t width, int32_t height);
    void draw(void);

private:

    // Additional rendering setup methods
    void freeGLBindings(void) const;
    void cleanFBO(void);
    void initFBO(void);
    void cleanRendering(void);

    // Auxiliary methods
    void computeMaxSampleTapDistance(void);
    void computeTiledDimensions(int32_t& widthDividedByK,
                                int32_t& heightDividedByK) const;
    nv::matrix4f sailTransform(const float sailAngle) const;
    void drawScreenAlignedQuad(const unsigned int attrHandle) const;

    // Methods that draw a specific scene component
    void drawSkyboxColorDepth(void) const;
    void drawModelColorDepth(NvModelGL* pModel,
                             const nv::matrix4f& modelMatrix) const;
    void drawModelMotionBlurES2(NvModelGL* pModel,
                                const nv::matrix4f& modelMatrix,
                                const nv::matrix4f& prevModelMatrix) const;
    void drawModelVelocity(NvModelGL* pModel,
                           const nv::matrix4f& modelMatrix,
                           const nv::matrix4f& prevModelMatrix) const;
    void drawSkyboxVelocity(void) const;

    // Methods that help with fullscreen passes
    void drawFullScreenQuad(BaseFullscreenShader* shd,
                            const unsigned int  texID) const;
    void drawTexcoordFullScreenQuad(TexcoordFullscreenShader* shd,
                                    const unsigned int  texID) const;

    // Methods that render results to the screen
    void renderC(const nv::matrix4f& houseXform,
                 const nv::matrix4f& sailsXform) const;
    void renderZ(const nv::matrix4f& houseXform,
                 const nv::matrix4f& sailsXform) const;
    void renderV(const nv::matrix4f& houseXform,
                 const nv::matrix4f& sailsXform,
                 const nv::matrix4f& prevSailsXform) const;
    void renderTileMax(const nv::matrix4f& houseXform,
                       const nv::matrix4f& sailsXform,
                       const nv::matrix4f& prevSailsXform) const;
    void renderNeighborMax(const nv::matrix4f& houseXform,
                           const nv::matrix4f& sailsXform,
                           const nv::matrix4f& prevSailsXform) const;
    void renderGather(const nv::matrix4f& houseXform,
                      const nv::matrix4f& sailsXform,
                      const nv::matrix4f& prevSailsXform) const;
    void renderMotionBlurGLES(const nv::matrix4f& houseXform,
                              const nv::matrix4f& sailsXform,
                              const nv::matrix4f& prevSailsXform) const;

    // Member fields that hold FBO and texture handles
    GLuint mNormalRboID;
    GLuint mSmallRboID;

    GLuint mCZFboID;
    GLuint mVFboID;
    GLuint mTileMaxFboID;
    GLuint mNeighborMaxFboID;

    GLuint mCTexID;
    GLuint mZTexID;
    GLuint mVTexID;
    GLuint mTileMaxTexID;
    GLuint mNeighborMaxTexID;

    GLuint mSkyBoxTexID;
    GLuint mHouseTexID;
    GLuint mRandomTexID;

    // Member fields that hold shader objects
    SceneColorShader*         mSceneColorShader;
    SkyboxColorShader*          mSkyboxColorShader;
    DepthDrawShader*          mDepthDrawShader;
    SceneVelocityShader*      mSceneVelocityShader;
    SkyboxVelocityShader*     mSkyBoxVelocityShader;
    TileMaxShader*            mTileMaxShader;
    NeighborMaxShader*        mNeighborMaxShader;
    GatherShader*             mGatherShader;
    TexcoordFullscreenShader* mTexcoordFullscreenShader;
    MotionBlurES2Shader*      mMotionBlurES2Shader;

    // Member fields that hold the scene geometry
    NvModelGL* mHouseModel;
    NvModelGL* mSailsModel;

    // Diffuse color of the objects
    nv::vec3f mModelColor;

    // Pipeline matrices
    nv::matrix4f mProjectionMatrix;
    nv::matrix4f mInverseProjMatrix;
    nv::matrix4f mCurrViewMatrix;
    nv::matrix4f mPrevViewMatrix;

    // Other state-related member fields
    // (some of them are referenced and controlled by the TweakBar UI)
    uint32_t mRenderOptions;
    uint32_t mK;
    uint32_t mLastK;
    uint32_t mS;

    int32_t mMaxSampleTapDistance;

    float mCurrSailAngle;
    float mLastSailAngle;
    float mSailSpeed;
    float mExposure;
    float mLastDt;

    bool mFirstFrameFlag;
    bool mAnimPaused;
    bool mDrawSky;
};
