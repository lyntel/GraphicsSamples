//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/Shaders.h
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

#ifndef SHADERS_H
#define SHADERS_H

#include <NvSimpleTypes.h>


#include "SceneRenderer.h"
#include "ParticleRenderer.h"
#include "Upsampler.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "utils.h"

struct CameraViewParticleProgram : public NvGLSLProgram
{
    CameraViewParticleProgram(bool isES2) : NvGLSLProgram()
    {
        if (isES2)
            setSourceFromFiles("shaders/cameraViewParticle_es2.vert", "shaders/cameraViewParticle_es2.frag");
        else
            setSourceFromFiles("shaders/cameraViewParticle.vert", "shaders/cameraViewParticle.frag");
        m_positionAttrib     = getAttribLocation("g_positionAndColor");
        m_depthTex            = getUniformLocation("g_depthTex");
        m_pointScale         = getUniformLocation("g_pointScale");
        m_alpha             = getUniformLocation("g_alpha");
        m_modelViewMatrix     = getUniformLocation("g_modelViewMatrix");
        m_projectionMatrix     = getUniformLocation("g_projectionMatrix");

        // soft-particle constants
        m_invViewport        = getUniformLocation("g_invViewport");
        m_depthConstants    = getUniformLocation("g_depthConstants");
        m_depthDeltaScale    = getUniformLocation("g_depthDeltaScale");
    }

    void setUniforms(SceneInfo &scene, ParticleRenderer::Params &params)
    {
        glUseProgram(m_program); CHECK_GL_ERROR();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, scene.m_fbos->m_particleFbo->depthTexture);
        glUniform1i(m_depthTex, 1);

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        glUniform2f(m_invViewport, 1.f / viewport[2], 1.f / viewport[3]);
        glUniform2f(m_depthConstants, -1.0f/EYE_ZFAR + 1.0f/EYE_ZNEAR, -1.0f/EYE_ZNEAR);
        glUniform1f(m_depthDeltaScale, 1.f / params.softness);

        glUniform1f(m_alpha, params.spriteAlpha * params.particleScale);
        glUniform1f(m_pointScale, params.getPointScale(scene.m_eyeProj));
        glUniformMatrix4fv(m_modelViewMatrix, 1, GL_FALSE, scene.m_eyeView.get_value());
        glUniformMatrix4fv(m_projectionMatrix, 1, GL_FALSE, scene.m_eyeProj.get_value());
    }

    GLint getPositionAndColorAttrib()
    {
        return m_positionAttrib;
    }

protected:
    GLint m_positionAttrib;
    GLint m_depthTex;
    GLint m_pointScale;
    GLint m_alpha;
    GLint m_invViewport;
    GLint m_depthConstants;
    GLint m_depthDeltaScale;
    GLint m_modelViewMatrix;
    GLint m_projectionMatrix;
};

class OpaqueColorProgram : public NvGLSLProgram
{
public:
    OpaqueColorProgram(bool isES2) : NvGLSLProgram()
    {
        if (isES2)
            setSourceFromFiles("shaders/base_es2.vert", "shaders/base_es2.frag");
        else
            setSourceFromFiles("shaders/base.vert", "shaders/base.frag");
        m_positionAttrib     = getAttribLocation("g_Position");
        m_normalAttrib       = getAttribLocation("g_Normal");
    }

    void setUniforms(SceneInfo &scene)
    {
        glUseProgram(m_program);

        setUniform3fv("g_lightDirection",  scene.m_lightVector._array);
    }

    GLint getPositionAttrib()
    {
        return m_positionAttrib;
    }

    GLint getNormalAttrib()
    {
        return m_normalAttrib;
    }

protected:
    GLint m_positionAttrib;
    GLint m_normalAttrib;
};

class OpaqueDepthProgram : public NvGLSLProgram
{
public:
    OpaqueDepthProgram(const char* fraggy)
    {
        setSourceFromFiles("shaders/unshaded.vert", fraggy);
        m_positionAttrib     = getAttribLocation("g_Position");
    }

    void setUniforms(SceneInfo &scene)
    {
        glUseProgram(m_program);
    }

    GLint getPositionAttrib()
    {
        return m_positionAttrib;
    }

protected:
    GLint m_positionAttrib;
    GLint m_modelViewMatrix;
    GLint m_projectionMatrix;
};

class UpsampleBilinearProgram : public NvGLSLProgram
{
public:
    UpsampleBilinearProgram()
    {
        setSourceFromFiles("shaders/upsampleBilinear.vert", "shaders/upsampleBilinear.frag");
        m_positionAttrib = getAttribLocation("g_position");
        m_texCoordAttrib = getAttribLocation("g_texCoords");
        m_texture        = getUniformLocation("g_texture");
    }

    void setTexture(GLint texture)
    {
        glUseProgram(m_program);
        glUniform1i(m_texture, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    GLint getPositionAttrib()
    {
        return m_positionAttrib;
    }

    GLint getTexCoordAttrib()
    {
        return m_texCoordAttrib;
    }

protected:
    GLint m_positionAttrib;
    GLint m_texCoordAttrib;
    GLint m_texture;
};

class UpsampleCrossBilateralProgram : public NvGLSLProgram
{
public:
    UpsampleCrossBilateralProgram()
    {
        setSourceFromFiles("shaders/upsampleCrossBilateral.vert", "shaders/upsampleCrossBilateral.frag");
        m_positionAttrib             = getAttribLocation("g_position");
        m_texCoordAttrib             = getAttribLocation("g_texCoords");

        m_fullResDepthTex           = getUniformLocation("g_fullResDepthTex");
        m_lowResDepthTex               = getUniformLocation("g_lowResDepthTex");
        m_lowResParticleColorTex     = getUniformLocation("g_lowResParticleColorTex");

        m_depthConstants             = getUniformLocation("g_depthConstants");
        m_lowResTextureSize         = getUniformLocation("g_lowResTextureSize");
        m_lowResTexelSize             = getUniformLocation("g_lowResTexelSize");
        m_depthMult                 = getUniformLocation("g_depthMult");
        m_threshold                    = getUniformLocation("g_threshold");
    }

    void setUniforms(SceneFBOs *fbos, Upsampler::Params& params)
    {
        glUseProgram(m_program);

        glBindTexture(GL_TEXTURE_2D, fbos->m_particleFbo->colorTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbos->m_sceneFbo->depthTexture);
        glUniform1i(m_fullResDepthTex, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, fbos->m_particleFbo->depthTexture);
        glUniform1i(m_lowResDepthTex, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, fbos->m_particleFbo->colorTexture);
        glUniform1i(m_lowResParticleColorTex, 2);

        vec2f lowResolution((float)fbos->m_particleFbo->width, (float)fbos->m_particleFbo->height);
        vec2f lowResTexelSize(1.f/lowResolution.x, 1.f/lowResolution.y);

        glUniform2f(m_depthConstants, -1.f/EYE_ZFAR + 1.f/EYE_ZNEAR, -1.f/EYE_ZNEAR);
        glUniform2f(m_lowResTextureSize, lowResolution.x, lowResolution.y);
        glUniform2f(m_lowResTexelSize, lowResTexelSize.x, lowResTexelSize.y);
        glUniform1f(m_depthMult, params.upsamplingDepthMult);
        glUniform1f(m_threshold, params.upsamplingThreshold);
    }

    GLint getPositionAttrib()
    {
        return m_positionAttrib;
    }

    GLint getTexCoordAttrib()
    {
        return m_texCoordAttrib;
    }

protected:
    GLint m_positionAttrib;
    GLint m_texCoordAttrib;

    GLint m_fullResDepthTex;
    GLint m_lowResDepthTex;
    GLint m_lowResParticleColorTex;

    GLint m_depthConstants;
    GLint m_lowResTextureSize;
    GLint m_lowResTexelSize;
    GLint m_depthMult;
    GLint m_threshold;
};

#endif // SHADERS_H

