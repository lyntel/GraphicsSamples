//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/WaveSimRenderer.cpp
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
#include "WaveSimRenderer.h"

//extern nv::matrix4f viewMatrix;

int WaveSimRenderer::m_renderersCount = 0;

WaveSimRenderer::WaveSimRenderer(WaveSim *sim)
:m_simulation(sim), m_rendererId(m_renderersCount++), m_current(0), m_previous(0), m_renderCurrent(1)
{
	if(m_rendererId%2 != 0)
		m_gridRenderPos = nv::vec2f(-2.1f + (m_rendererId/2)*2.2f, -2.1f);
	else
		m_gridRenderPos = nv::vec2f(-2.1f + (m_rendererId/2)*2.2f, 0.1f);

	m_modelMatrix = nv::translation(m_modelMatrix, m_gridRenderPos.x, 0.0f, m_gridRenderPos.y);
	m_modelMatrix.set_scale(nv::vec3f(2.0f/sim->getWidth(), 2.0f/((sim->getWidth()+sim->getHeight())/2.0f), 2.0f/sim->getHeight()));

	m_invModelMatrix = nv::inverse(m_modelMatrix);
}

WaveSimRenderer::~WaveSimRenderer()
{
	m_renderersCount--;
}

void WaveSimRenderer::setColor(float x, float y, float z, float a)
{
	m_color.x = x;
	m_color.y = y;
	m_color.z = z;
	m_color.w = a;
}


void WaveSimRenderer::setViewMatrix(const nv::matrix4f& view)
{
	m_viewMatrix = view;
}

void WaveSimRenderer::initBuffers()
{
	int w = m_simulation->getWidth();
	int h = m_simulation->getHeight();

	float *surfacexz = new float[w*h*2];
	for(int i=0; i<h; i++)
	{
		for(int j=0; j<w; j++)
		{
			surfacexz[i*w*2 + j*2 + 0] = (float)i;
			surfacexz[i*w*2 + j*2 + 1] = (float)j;
		}
	}

	glGenBuffers(1, &m_waterXZVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_waterXZVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*w*h*2, surfacexz, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	delete [] surfacexz;

	//no of quads for a grid of wxh = (w-1)*(h-1);
	//no of triangles = no of quads * 2;
	//no of vertices = no of triangles * 3;
	//no of components per vertex = 2 [x,z];

	m_vertCount = (w-3)*(h-3) * 2 * 3;

#if 1
	unsigned short *indices = new unsigned short[m_vertCount];
	for(int i=0; i<w-3; i++)
	{
		for(int j=0; j<h-3; j++)
		{
			int k,l;
			if(i%2==0)
			{
				k = i+1;
				l = j+1;
				//for each quad on even row
				//triangle 1
				//vertex 1
				indices[i*(h-3)*6 + j*6 + 0] = (unsigned short) ((k)*h + (l));
				//vertex 2
				indices[i*(h-3)*6 + j*6 + 1] = (unsigned short) ((k)*h + (l+1));
				//vertex 3
				indices[i*(h-3)*6 + j*6 + 2] = (unsigned short) ((k+1)*h + (l));

				//triangle 2
				//vertex 1
				indices[i*(h-3)*6 + j*6 + 3] = (unsigned short) ((k+1)*h + (l));
				//vertex 2
				indices[i*(h-3)*6 + j*6 + 4] = (unsigned short) ((k)*h + (l+1));
				//vertex 3
				indices[i*(h-3)*6 + j*6 + 5] = (unsigned short) ((k+1)*h + (l+1));
			}
			else
			{
				k = 1 + (w - 4) - i;
				l = 1 + (h - 4) - j;
				//for each quad on odd row
				//triangle 1
				//vertex 1
				indices[i*(h-3)*6 + j*6 + 0] = (unsigned short) ((k)*h + (l));
				//vertex 2
				indices[i*(h-3)*6 + j*6 + 1] = (unsigned short) ((k)*h + (l+1));
				//vertex 3
				indices[i*(h-3)*6 + j*6 + 2] = (unsigned short) ((k+1)*h + (l));

				//triangle 2
				//vertex 1
				indices[i*(h-3)*6 + j*6 + 3] = (unsigned short) ((k+1)*h + (l));
				//vertex 2
				indices[i*(h-3)*6 + j*6 + 4] = (unsigned short) ((k)*h + (l+1));
				//vertex 3
				indices[i*(h-3)*6 + j*6 + 5] = (unsigned short) ((k+1)*h + (l+1));
			}
		}
	}
#else
	unsigned short *indices = new unsigned short[m_vertCount];
	unsigned short* ptr = indices;
	for(int i=0; i<w-3; i++)
	{
		for(int j=0; j<h-3; j++)
		{
			*ptr++ = (unsigned short)(i * w + j);
			*ptr++ = (unsigned short)(i * w + 1 + j);
			*ptr++ = (unsigned short)(i * w + h + j);
			
			*ptr++ = (unsigned short)(i * w + h + j);
			*ptr++ = (unsigned short)(i * w + 1 + j);
			*ptr++ = (unsigned short)(i * w + h + j + 1);
		}
	}
#endif
	
	glGenBuffers(1, &m_waterIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_waterIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*m_vertCount, indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	m_simulation->m_heightFieldSize = sizeof(float)*w*h;
	for(int i=0; i<NUM_BUFFERS; i++)
	{
		glGenBuffers(1, &m_waterYVBO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, m_waterYVBO[i]);
		glBufferData(GL_ARRAY_BUFFER, m_simulation->m_heightFieldSize, m_simulation->getHeightField(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	m_simulation->m_gradientsSize = sizeof(float)*w*h*2;
	
	for(int i=0; i<NUM_BUFFERS; i++)
	{
		glGenBuffers(1, &m_waterGVBO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, m_waterGVBO[i]);
		glBufferData(GL_ARRAY_BUFFER, m_simulation->m_gradientsSize, m_simulation->getGradients(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	delete [] indices;
}

void WaveSimRenderer::updateBufferData()
{
#ifdef USE_GL_BUFFER_SUB_DATA
	//update the new vertex y positions
	glBindBuffer(GL_ARRAY_BUFFER, m_waterYVBO[m_current]);
	glBufferSubData(GL_ARRAY_BUFFER, GLintptr(0), m_simulation->m_heightFieldSize, m_simulation->getHeightField());

	//update the new normals
	glBindBuffer(GL_ARRAY_BUFFER, m_waterGVBO[m_current]);
	glBufferSubData(GL_ARRAY_BUFFER, GLintptr(0), m_simulation->m_gradientsSize, m_simulation->getGradients());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
	glBindBuffer(GL_ARRAY_BUFFER, m_waterYVBO[m_current]);
	//glBufferData(GL_ARRAY_BUFFER, m_simulation->m_heightFieldSize, NULL, GL_DYNAMIC_DRAW);
	//GL_OES_mapbuffer
	void *ptr1 = glMapBufferRange(GL_ARRAY_BUFFER, 
        0, m_simulation->m_heightFieldSize, GL_MAP_WRITE_BIT);
	memcpy(ptr1, m_simulation->getHeightField(), m_simulation->m_heightFieldSize);
	//for(int i = 0; i < 100; i++)
	//{
	//	((float*)ptr1)[0] = rand() * 10000.0f / RAND_MAX;
	//}
	//((float*)ptr1)[0] = 1.125f;
	//((float*)ptr1)[1] = 5.225f;
	//((float*)ptr1)[2] = 100.525f;
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, m_waterGVBO[m_current]);
	//glBufferData(GL_ARRAY_BUFFER, m_simulation->m_gradientsSize, NULL, GL_DYNAMIC_DRAW);
	//GL_OES_mapbuffer
	void *ptr2 = glMapBufferRange(GL_ARRAY_BUFFER, 
        0, m_simulation->m_gradientsSize, GL_MAP_WRITE_BIT);
	memcpy(ptr2, m_simulation->getGradients(), m_simulation->m_gradientsSize);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif

	//swap the vbo's
	m_current = ++m_current % NUM_BUFFERS;
}

void WaveSimRenderer::render(GLuint colorHandle, GLuint modelViewMatrixHandle, GLuint modelMatrixHandle, GLuint normalMatrixHandle, GLuint posXYHandle, GLuint posYHandle, GLuint gradientHandle)
{
	m_modelViewMatrix = m_viewMatrix * m_modelMatrix;

	//normalMatrix = _transpose(_inverse(ModelMiewMatrix));
	m_normalMatrix = nv::inverse(m_modelViewMatrix);
	m_normalMatrix = nv::transpose(m_normalMatrix);

	glUniform4fv(colorHandle, 1,  m_color._array);
	glUniformMatrix4fv(modelViewMatrixHandle, 1, GL_FALSE, m_modelViewMatrix._array);
	glUniformMatrix4fv(normalMatrixHandle, 1, GL_FALSE, m_normalMatrix._array);
	glUniformMatrix4fv(modelMatrixHandle, 1, GL_FALSE, m_modelMatrix._array);

	glBindBuffer(GL_ARRAY_BUFFER, m_waterXZVBO);
	glVertexAttribPointer(posXYHandle, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, m_waterYVBO[m_renderCurrent]);
	glVertexAttribPointer(posYHandle, 1, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, m_waterGVBO[m_renderCurrent]);
	glVertexAttribPointer(gradientHandle, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glEnableVertexAttribArray(posXYHandle);
	glEnableVertexAttribArray(posYHandle);
	glEnableVertexAttribArray(gradientHandle);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_waterIBO);
	glDrawElements(GL_TRIANGLES, m_vertCount, GL_UNSIGNED_SHORT, 0);
	//glDrawElements(GL_LINE_LOOP, m_vertCount, GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(posXYHandle);
	glDisableVertexAttribArray(posYHandle);
	glDisableVertexAttribArray(gradientHandle);

	m_renderCurrent = ++m_renderCurrent % NUM_BUFFERS;
}
