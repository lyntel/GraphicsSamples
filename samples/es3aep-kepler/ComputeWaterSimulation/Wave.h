//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/Wave.h
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
#ifndef _WAVE_
#define _WAVE_

#include "WaveSim.h"
#include "WaveSimThread.h"
#include "WaveSimRenderer.h"

//A wrapper for the fluid surface simulation, thread handling and rendering
class Wave{
private:

	//simulation
	WaveSim m_simulation;

	//rendering
	WaveSimRenderer m_renderer;

	//thread handling
	WaveSimThread m_thread;

public:
	//ctor
	Wave(int width, int height, float damping, nv::vec4f color);

	//no dtor as no pointers are used

	//returs a reference to the simulation variable
	WaveSim& getSimulation();

	//return a reference to the renderer variable
	WaveSimRenderer& getRenderer();

	//returns a reference to the thread handling variable
	WaveSimThread& getThread();
	
	//updates the VBO data
	void updateBufferData();

	//renders the current surface
	void render(GLuint colorHandle, GLuint modelViewMatrixHandle, GLuint modelMatrixHandle, GLuint normalMatrixHandle, GLuint posXYHandle, GLuint posYHandle, GLuint normalHandle);

	//simulates and calculates the normals
	void simulateAndCalcNormals(float timestep);

	//starts the underlying simulation thread for 1 loop and the thread goes to wait for the next call to this function
	void startSimulationThread();

	//adds a random disturbance
	void addRandomDisturbance(int radius, float height);

	//pauses all the simulation threads
	static void pauseAllSimulationThreads();
	
	//synchronizes all simulation threads (waits until all threads have completed simulation and reached wait.
	static void syncAllSimulationThreads();

	//starts the thread stopwatch for recording the time
	static void startThreadStopWatch();

	//resets the thread stopwatch
	static void resetThreadStopWatch();

	//maps a world-space 3D point on XZ plane to a grid position in the current surface grid (does nothing if the point lies outside the surface)
	bool mapPointXZToGridPos(nv::vec3f point, nv::vec2f &gridPos);
};

//returns a random integer between the two values
int randBetween(int min, int max);

#endif
