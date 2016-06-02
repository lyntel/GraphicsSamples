//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/WaveSimRenderer.h
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
#ifndef _WAVE_SIM_RENDERER_
#define _WAVE_SIM_RENDERER_

//#define USE_GL_BUFFER_SUB_DATA 1
//else use glmap/memcpy/glunmap

#include "NV/NvPlatformGL.h"
#include "NV/NvMath.h"

#include "WaveSim.h"

class WaveSimRenderer
{
	//id of the current renderer
	int m_rendererId;

	//view matrix
	nv::matrix4f m_viewMatrix;

public:
	//stores the number of renderers
	static int m_renderersCount;

	//number of buffers for round-robin VBO's
	static const int NUM_BUFFERS = 4;

	//matrices for the surface
	nv::matrix4f m_modelMatrix, m_modelViewMatrix, m_normalMatrix;

	//inverse modelMatrix
	nv::matrix4f m_invModelMatrix;

	//color of the surface
	nv::vec4f m_color;

	//position in world space where the grid(surface) is rendered
	nv::vec2f m_gridRenderPos;

	//ctor
	WaveSimRenderer(WaveSim *sim);
	//dtor
	~WaveSimRenderer();

	//set the color of the surface
	void setColor(float x, float y, float z, float a);

	//set the current view matrix
	void setViewMatrix(const nv::matrix4f& view);

	//init the Vertex Buffers
	void initBuffers();

	//update the buffer data using round-robin VBO's
	void updateBufferData();

	//render the surface
	void render(GLuint colorHandle, GLuint modelViewMatrixHandle, GLuint modelMatrixHandle, GLuint normalMatrixHandle, GLuint posXYHandle, GLuint posYHandle, GLuint gradientHandle);

	//pointer to the simulation object
	WaveSim *m_simulation;

	//handles to the water's XZ VBO and Index Buffers (static and donot change)
	GLuint m_waterXZVBO, m_waterIBO;

	//handles to the water Y VBO(heights) and the waver G VBO(gradients)
	GLuint m_waterYVBO[NUM_BUFFERS], m_waterGVBO[NUM_BUFFERS];

	//index variables to point at the current, previous vbo's for update buffers and a renderCurrent for the current rendering VBO.
	int m_current, m_previous, m_renderCurrent;

	// variable to store the number of vertices for glDrawElements();
	int m_vertCount;
};


#endif
