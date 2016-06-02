//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/Upsampler.cpp
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

#include "Upsampler.h"
#include "Shaders.h"

Upsampler::Upsampler(SceneFBOs *fbos, bool isGL)
: m_fbos(fbos)
{
    m_params.useBlit = isGL;

    m_upsampleBilinearProg             = new UpsampleBilinearProgram();
    m_upsampleCrossBilateralProg     = new UpsampleCrossBilateralProgram();
}

Upsampler::~Upsampler()
{
    delete m_upsampleBilinearProg;
    delete m_upsampleCrossBilateralProg;
}

void Upsampler::drawQuad(GLint positionAttrib, GLint texcoordAttrib)
{
    //    vertex positions in NDC        tex-coords
    static const float fullScreenQuadData[] = {
            -1.0f, -1.0f, -1.0f, 1.0f,     0.0f, 0.0f,
             1.0f, -1.0f, -1.0f, 1.0f,     1.0f, 0.0f,
            -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f,
            1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f
    };

    glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE, 6*sizeof(float), fullScreenQuadData);
    glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), &fullScreenQuadData[4]);

    glEnableVertexAttribArray(positionAttrib);
    glEnableVertexAttribArray(texcoordAttrib);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(positionAttrib);
    glDisableVertexAttribArray(texcoordAttrib);
}

void Upsampler::drawBilinearUpsampling(UpsampleBilinearProgram *prog, GLint texture)
{
    prog->enable();
    prog->setTexture(texture);

    drawQuad(prog->getPositionAttrib(), prog->getTexCoordAttrib());
}

void Upsampler::drawCrossBilateralUpsampling(UpsampleCrossBilateralProgram *prog)
{
    prog->enable();
    prog->setUniforms(m_fbos, m_params);

    drawQuad(prog->getPositionAttrib(), prog->getTexCoordAttrib());
}

void Upsampler::upsampleParticleColors(SceneInfo& scene)
{
    m_fbos->m_sceneFbo->bind();

    // composite the particle colors with the scene colors, preserving destination alpha
    // dst.rgb = src.rgb + (1.0 - src.a) * dst.rgb
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    glEnable(GL_BLEND);

    glDisable(GL_DEPTH_TEST);

    if (m_params.useCrossBilateral)
    {
        drawCrossBilateralUpsampling(m_upsampleCrossBilateralProg);
    }
    else
    {
        drawBilinearUpsampling(m_upsampleBilinearProg, m_fbos->m_particleFbo->colorTexture);
    }

    glDisable(GL_BLEND);
}

void Upsampler::upsampleSceneColors(SceneInfo& scene)
{
    GLuint prevFBO = 0;
    // Enum has MANY names based on extension/version
    // but they all map to 0x8CA6
    glGetIntegerv(0x8CA6, (GLint*)&prevFBO);

    if (m_params.useBlit)
    {
        const NvSimpleFBO *srcFbo = m_fbos->m_sceneFbo;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo->fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFBO);

        glBlitFramebuffer(0, 0, srcFbo->width, srcFbo->height,
                            0, 0, scene.m_screenWidth, scene.m_screenHeight,
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        glViewport(0, 0, scene.m_screenWidth, scene.m_screenHeight);

        drawBilinearUpsampling(m_upsampleBilinearProg, m_fbos->m_sceneFbo->colorTexture);
    }
}
