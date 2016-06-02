//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/TerrainGenerator.h
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

#ifndef _TERRAIN_GENERATOR_
#define _TERRAIN_GENERATOR_

#include <NvSimpleTypes.h>


#include "TerrainSim.h"
#include "TerrainSimThread.h"
#include "TerrainSimRenderer.h"


// A wrapper for the terrain surface simulation, thread handling and rendering.
class TerrainGenerator
{
public:
    TerrainGenerator(int32_t width, int32_t height, const nv::vec2f& offset);

    TerrainSim& getSimulation();
    TerrainSimRenderer& getRenderer();
    TerrainSimThread& getThread();
    
    //updates the VBO data
    void updateBufferData();

    int32_t render(GLuint posXYHandle, GLuint normalAndHeightHandle);

    //starts the underlying simulation thread for 1 loop and the thread goes to wait for the next call to this function
    void startSimulationThread();

    //pauses all the simulation threads
    static void waitForAllThreadsToExit();
    
    //synchronizes all simulation threads (waits until all threads have completed simulation and reached wait.
    static void syncAllSimulationThreads();

    //starts the thread stopwatch for recording the time
    static void startThreadStopWatch();

    //resets the thread stopwatch
    static void resetThreadStopWatch();

    //maps a world-space 3D point on XZ plane to a grid position in the current surface grid (does nothing if the point lies outside the surface)
    bool mapPointXZToGridPos(nv::vec3f point, nv::vec2f &gridPos);

private:
    TerrainSim m_simulation;
    TerrainSimRenderer m_renderer;
    TerrainSimThread m_thread;
};

#endif
