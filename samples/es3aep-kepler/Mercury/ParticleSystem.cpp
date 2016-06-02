//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury/ParticleSystem.cpp
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
#include "assets/shaders/uniforms.h"

extern std::string loadShaderSourceWithUniformTag(const char* uniformsFile, const char* srcFile);

static float frand()
{
    return rand() / (float) RAND_MAX;
}

// Returns a value between -0.5 and 0.5
static float sfrand()
{
    return frand()*2.0f-1.0f;
}

ParticleSystem::ParticleSystem(size_t size, const char* shaderPrefix) :
    m_size(size),
    m_updateProg(0),
    m_shaderPrefix(shaderPrefix),
	m_defaultRadius(1.0)
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
        size_t index = i<<2;
        *(indices++) = index;
        *(indices++) = index+1;
        *(indices++) = index+2;
        *(indices++) = index;
        *(indices++) = index+2;
        *(indices++) = index+3;
    }
    m_indices->unmap();

	ShaderPipeline::loadShaders(m_programPipeline, m_updateProg, m_shaderPrefix, "shaders/particlesCS.glsl");

	//Initialize the polygonizer

	const float boxSize = 4.0f;       
	const float cellSize = 0.1f;      
	
	// Same shader prefix is being reused
	m_mcpolygonizer = new MCPolygonizer(boxSize, cellSize, m_shaderPrefix);
	

    reset();
}

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
    case GL_COMPUTE_SHADER:
        return "COMPUTE_SHADER";
        break;
    }
    return "";
}


ParticleSystem::~ParticleSystem()
{
    delete m_pos;
    delete m_vel;
	delete m_mcpolygonizer;
	
	glDeleteProgram(m_updateProg);    
}

void ParticleSystem::reset() 
{
	reset(m_defaultRadius);
}

void ParticleSystem::reset(float radius)
{
	vec4f *pos = m_pos->map();
	for (size_t i = 0; i < m_size; i++) {
		vec3f generated_vec3 = vec3f(sfrand(), sfrand(), sfrand());
		generated_vec3 = normalize(generated_vec3) * radius;
		pos[i] = vec4f(generated_vec3, 1.0);
	}
	m_pos->unmap();
	

	vec4f *vel = m_vel->map();
	for (size_t i = 0; i < m_size; i++) {		
		float max_velocity = 0.02f;
		vel[i] = vec4f(sfrand()*max_velocity, sfrand()*max_velocity, sfrand()*max_velocity, 0.0f);
	}
	m_vel->unmap();
}

void ParticleSystem::update(float timeDelta)
{	
    static bool updated = false;
    if (!updated)
    {
        LOGI("ParticleSystem: First Update Time: %f", timeDelta);
        updated = true;
    }
    m_mcpolygonizer->generateMesh(m_pos, m_vel, timeDelta);

    // Invoke the compute shader to integrate the particles
    glBindProgramPipeline(m_programPipeline);

    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  m_pos->getBuffer() );
    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  m_vel->getBuffer() );

    // Update the timestep in the shaders
    GLuint loc = glGetUniformLocation(m_updateProg, "timeStep");
    glProgramUniform1f(m_updateProg, loc, timeDelta);

    uint xGroups = (m_size + (WORK_GROUP_SIZE - 1)) / WORK_GROUP_SIZE;
    glDispatchCompute(xGroups, 1, 1);
    
    // We need to block here on compute completion to ensure that the
    // computation is done before we render
    glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );

    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  0 );
    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );

	// Update the timestep in the shaders
	//GLuint loc = glGetUniformLocation(m_updateProg, "timeStep");
	//glProgramUniform1f(m_updateProg, loc, timeDelta);

    glBindProgramPipeline(0);
}
