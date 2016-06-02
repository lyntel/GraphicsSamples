//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/Wave.cpp
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
#include "Wave.h"

Wave::Wave(int width, int height, float damping, nv::vec4f color)
:m_simulation(width+2, height+2, damping), m_renderer(&m_simulation), m_thread(&m_simulation)
{
	//set the color
	m_renderer.setColor(color.x, color.y, color.z, color.w);

	//init the vbo's for rendering
	m_renderer.initBuffers();
	
	//start the thread
	m_thread.start();

	//wait until the current thread has completed
	WaveSimThread::waitForAllThreads();
}

WaveSim& Wave::getSimulation()
{
	return m_simulation;
}

WaveSimRenderer& Wave::getRenderer()
{
	return m_renderer;
}

WaveSimThread& Wave::getThread()
{
	return m_thread;
}

void Wave::updateBufferData()
{
	m_renderer.updateBufferData();
}

void Wave::render(GLuint colorHandle, GLuint modelViewMatrixHandle, GLuint modelMatrixHandle, GLuint normalMatrixHandle, GLuint posXYHandle, GLuint posYHandle, GLuint normalHandle)
{
	m_renderer.render(colorHandle, modelViewMatrixHandle, modelMatrixHandle, normalMatrixHandle, posXYHandle, posYHandle, normalHandle);
}

int randBetween(int min, int max)
{
	return min + rand()%(max-min);
}

void Wave::addRandomDisturbance(int radius, float height)
{
	radius++;
	//simulation.addDisturbance(randBetween(radius+1, simulation.getWidth()), randBetween(radius+1, simulation.getHeight()), radius, height);
	m_simulation.addDisturbance(
		(float)randBetween(radius+1, m_simulation.getWidth() - radius),
		(float)randBetween(radius+1, m_simulation.getHeight() - radius),
		(float)(radius-1),
		-height);
}

void Wave::simulateAndCalcNormals(float timestep)
{
	m_simulation.simulate(timestep);
	m_simulation.calcGradients();
}

void Wave::resetThreadStopWatch()
{
//	WaveSimThread::m_threadStopWatch->reset();
}

void Wave::startThreadStopWatch()
{
//	WaveSimThread::m_threadStopWatch->start();
}

void Wave::startSimulationThread()
{
	m_thread.runSimulation();
}

void Wave::pauseAllSimulationThreads()
{
	WaveSimThread::pauseAllThreads();
}

void Wave::syncAllSimulationThreads()
{
	WaveSimThread::waitForAllThreads();
}

bool Wave::mapPointXZToGridPos(nv::vec3f point, nv::vec2f &gridPos)
{
	if(point.x>m_renderer.m_gridRenderPos.x && point.z>m_renderer.m_gridRenderPos.y)
	{
		float ox = (point.x - m_renderer.m_gridRenderPos.x)/2.0f;
		float oy = (point.z - m_renderer.m_gridRenderPos.y)/2.0f;
		if(ox<=1.0f && oy<=1.0f)
		{
			gridPos.x = oy * m_simulation.getWidth();
			gridPos.y = ox * m_simulation.getHeight();
			return true;
		}
		else return false;
	}
	else return false;
}
