//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeParticles/ParticleSystem.cpp
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
#include "ParticleSystem.h"
#include "NV/NvPlatformGL.h"
#include <stdlib.h>
#include <iostream> 

#define CPP 1
#include "NV/NvLogs.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NV/NvShaderMappings.h"
#include "noise.h"
#include "uniforms.h"

extern std::string loadShaderSourceWithUniformTag(const char* uniformsFile, const char* srcFile);

static float frand()
{
    return rand() / (float) RAND_MAX;
}

static float sfrand()
{
    return frand()*2.0f-1.0f;
}

ParticleSystem::ParticleSystem(size_t size, const char* shaderPrefix) :
    m_size(size),
    m_noiseTex(0),
    m_noiseSize(16),
    m_updateProg(0),
    m_shaderPrefix(shaderPrefix)
{
    // Split the velocities and positions, as for now the
    // rendering only cares about positions, so we just index
    // that buffer for rendering
    m_pos = new ShaderBuffer<nv::vec4f>(size);
    m_vel = new ShaderBuffer<nv::vec4f>(size);
    m_indices = new ShaderBuffer<uint32_t>(size*6);

    // the index buffer is a classic "two-tri quad" array.
    // This may seem odd, given that the compute buffer contains a single
    // vector for each particle.  However, the shader takes care of this
    // by indexing the compute shader buffer with a /4.  The value mod 4
    // is used to compute the offset from the vertex site, and each of the
    // four indices in a given quad references the same center point
    uint32_t *indices = m_indices->map();
    for(size_t i=0; i<m_size; i++) {
        uint32_t index = uint32_t(i<<2);
        *(indices++) = index;
        *(indices++) = index+1;
        *(indices++) = index+2;
        *(indices++) = index;
        *(indices++) = index+2;
        *(indices++) = index+3;
    }
    m_indices->unmap();

    m_noiseTex = createNoiseTexture4f3D(m_noiseSize, m_noiseSize, m_noiseSize, GL_RGBA8_SNORM);

    loadShaders();

    reset(0.5f);
}

static inline const char *GetShaderStageName(GLenum target)
{
    switch(target) {
    case GL_VERTEX_SHADER:
        return "VERTEX_SHADER";
        break;
    case GL_FRAGMENT_SHADER:
        return "FRAGMENT_SHADER";
        break;
    case GL_COMPUTE_SHADER:
        return "COMPUTE_SHADER";
        break;
    }
    return "";
}

GLuint ParticleSystem::createShaderPipelineProgram(GLuint target, const char* src)
{
    GLuint object;
    GLint status;

    const GLchar* fullSrc[2] = { m_shaderPrefix, src };
    object = glCreateShaderProgramv( target, 2, fullSrc);

    {
        GLint logLength;
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLength);
        char *log = new char [logLength];
        glGetProgramInfoLog(object, logLength, 0, log);
        LOGI("Shader pipeline program not valid:\n%s\n", log);
        delete [] log;
    }

    glBindProgramPipeline(m_programPipeline);
    glUseProgramStages(m_programPipeline, GL_COMPUTE_SHADER_BIT, object);
    glValidateProgramPipeline(m_programPipeline);
    glGetProgramPipelineiv(m_programPipeline, GL_VALIDATE_STATUS, &status);

    if (status != GL_TRUE) {
        GLint logLength;
        glGetProgramPipelineiv(m_programPipeline, GL_INFO_LOG_LENGTH, &logLength);
        char *log = new char [logLength];
        glGetProgramPipelineInfoLog(m_programPipeline, logLength, 0, log);
        LOGI("Shader pipeline not valid:\n%s\n", log);
        delete [] log;
    }

    glBindProgramPipeline(0);
    CHECK_GL_ERROR();

    return object;
}

void ParticleSystem::loadShaders()
{
    if (m_updateProg) {
        glDeleteProgram(m_updateProg);
        m_updateProg = 0;
    }

    glGenProgramPipelines( 1, &m_programPipeline);


    std::string src = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/particlesCS.glsl");
    m_updateProg = createShaderPipelineProgram(GL_COMPUTE_SHADER, src.c_str());

    glBindProgramPipeline(m_programPipeline);

    GLint loc = glGetUniformLocation(m_updateProg, "invNoiseSize");
    glProgramUniform1f(m_updateProg, loc, 1.0f / m_noiseSize);

    loc = glGetUniformLocation(m_updateProg, "noiseTex3D");
    glProgramUniform1i(m_updateProg, loc, 0);

    glBindProgramPipeline(0);
}

ParticleSystem::~ParticleSystem()
{
    delete m_pos;
    delete m_vel;

    glDeleteProgram(m_updateProg);
}

void ParticleSystem::reset(float size)
{
    vec4f *pos = m_pos->map();
    for(size_t i=0; i<m_size; i++) {
        pos[i] = vec4f(sfrand()*size, sfrand()*size, sfrand()*size, 1.0f);
    }
    m_pos->unmap();

    vec4f *vel = m_vel->map();
    for(size_t i=0; i<m_size; i++) {
        vel[i] = vec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }
    m_vel->unmap();
}

void ParticleSystem::update()
{
    // Invoke the compute shader to integrate the particles
    glBindProgramPipeline(m_programPipeline);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_noiseTex);

    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  m_pos->getBuffer() );
    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  m_vel->getBuffer() );

    glDispatchCompute(GLuint (m_size / WORK_GROUP_SIZE), 1,  1 );

    // We need to block here on compute completion to ensure that the
    // computation is done before we render
    glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );

    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  0 );
    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );

    glBindProgramPipeline(0);
}

