//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury/ParticleSystem.h
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
#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H
#include <iostream>
#include "NV/NvMath.h"
#include "ShaderBuffer.h"
#include "MCPolygonizer.h"
#include "ShaderPipeline.h"

class NvGLSLProgram;

using namespace nv;

class ParticleSystem
{
public:
    ParticleSystem(size_t size, const char* shaderPrefix);
    ~ParticleSystem();

	void reset();
    void reset(float radius);
    void update(float timeDelta);
	MCPolygonizer* getMCPolygonizer() const { return m_mcpolygonizer; }
	
    size_t getSize() { return m_size; }

    ShaderBuffer<vec4f> *getPosBuffer() { return m_pos; }
    ShaderBuffer<vec4f> *getVelBuffer() { return m_vel; }
    ShaderBuffer<uint32_t> *getIndexBuffer() { return m_indices; }

private:
    

    size_t m_size;
    ShaderBuffer<vec4f> *m_pos;
    ShaderBuffer<vec4f> *m_vel;
    ShaderBuffer<uint32_t> *m_indices;	

	GLuint m_programPipeline;
	GLuint m_updateProg;

	const char* m_shaderPrefix;

	float m_defaultRadius;

	MCPolygonizer *m_mcpolygonizer;

};
#endif // PARTICLE_SYSTEM_H