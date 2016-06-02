//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/TerrainGenerator.cpp
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

#include "TerrainGenerator.h"
#include "NV/NvStopWatch.h"

TerrainGenerator::TerrainGenerator(int32_t width, int32_t height, const nv::vec2f& offset)
:m_simulation(width, height, offset), m_renderer(&m_simulation, offset), m_thread(&m_simulation)
{
    //init the vbo's for rendering
    m_renderer.initBuffers();
    
    //start the thread
    m_thread.start();

    //wait until the current thread has completed
    TerrainSimThread::waitForAllThreads();
}

TerrainSim& TerrainGenerator::getSimulation()
{
    return m_simulation;
}

TerrainSimRenderer& TerrainGenerator::getRenderer()
{
    return m_renderer;
}

TerrainSimThread& TerrainGenerator::getThread()
{
    return m_thread;
}

void TerrainGenerator::updateBufferData()
{
    m_renderer.updateBufferData();
}

int32_t TerrainGenerator::render(GLuint posXYHandle, GLuint normalAndHeightHandle)
{
    return m_renderer.render(posXYHandle, normalAndHeightHandle);
}

void TerrainGenerator::resetThreadStopWatch()
{
    TerrainSimThread::getThreadStopWatch().reset();
}

void TerrainGenerator::startThreadStopWatch()
{
    TerrainSimThread::getThreadStopWatch().start();
}

void TerrainGenerator::startSimulationThread()
{
    m_thread.runSimulation();
}

void TerrainGenerator::waitForAllThreadsToExit()
{
    TerrainSimThread::waitForAllThreadsToExit();
}

void TerrainGenerator::syncAllSimulationThreads()
{
    TerrainSimThread::waitForAllThreads();
}

bool TerrainGenerator::mapPointXZToGridPos(nv::vec3f point, nv::vec2f &gridPos)
{
    return false;
}
