//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/TerrainSimThread.cpp
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

#include "TerrainSimThread.h"
#include "NV/NvStopWatch.h"
#include <NsAtomic.h>

using namespace nvidia::shdfnd;

int32_t TerrainSimThread::g_runningThreads = 0;
int32_t TerrainSimThread::g_livingThreads = 0;
int32_t TerrainSimThread::g_threadsCounter = 0;
Sync* TerrainSimThread::g_globalTerrainSimThreadCondition = NULL;
NvStopWatch* TerrainSimThread::m_threadStopWatch;

TerrainSimThread::TerrainSimThread(TerrainSim *sim)
:m_simulation(sim), m_threadId(++TerrainSimThread::g_threadsCounter),
    m_thread(threadThunk, this)
{

}

void TerrainSimThread::Init(NvStopWatch* stopWatch) {
	g_globalTerrainSimThreadCondition = new Sync;
	m_threadStopWatch = stopWatch;
}


void TerrainSimThread::runSimulation()
{
    //LOGI("runSimulation %d\n", m_threadId);
    m_localStartCondition.set();
}

void TerrainSimThread::waitForAllThreads()
{
    while(g_runningThreads>0)
    {
        g_globalTerrainSimThreadCondition->wait();
        g_globalTerrainSimThreadCondition->reset();
    }
}

void TerrainSimThread::waitForAllThreadsToExit()
{
    while(g_livingThreads>0)
    {
        g_globalTerrainSimThreadCondition->wait();
        g_globalTerrainSimThreadCondition->reset();
    }
}

void TerrainSimThread::Run()
{
    atomicIncrement(&g_livingThreads);

    while(!m_thread.quitIsSignalled())
    {
        //LOGI("TerrainSimThread::Run() %d entry\n", m_threadId);
        atomicIncrement(&g_runningThreads); 
        m_startTime = m_threadStopWatch->getTime();

        m_simulation->simulate();

        m_endTime = m_threadStopWatch->getTime();
        //LOGI("Thread Run %d = [start, end] - Exec Time : [%f, %f] - %f\n", m_threadId, m_startTime, m_endTime, m_endTime - m_startTime);

        atomicDecrement(&g_runningThreads);
        g_globalTerrainSimThreadCondition->set();

        if (!m_thread.quitIsSignalled()) {
            m_localStartCondition.wait();
            m_localStartCondition.reset();
        }
    }
    
    atomicDecrement(&g_livingThreads);
    g_globalTerrainSimThreadCondition->set();
}

void* TerrainSimThread::threadThunk(void* thiz) 
{
    ((TerrainSimThread*)thiz)->Run();
    return NULL;
}

TerrainSimThread::~TerrainSimThread()
{
    atomicDecrement(&g_threadsCounter);
}
