//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/WaveSimThread.cpp
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
#include "WaveSimThread.h"
#include "NV/NvStopWatch.h"
#include <NsAtomic.h>

using namespace nvidia::shdfnd;

int WaveSimThread::g_runningThreads = 0;
int WaveSimThread::g_threadsCounter = 0;
Sync* WaveSimThread::g_globalWaveSimThreadCondition = NULL;
NvStopWatch* WaveSimThread::m_threadStopWatch = NULL;

WaveSimThread::WaveSimThread(WaveSim *sim)
:m_simulation(sim), m_threadId(++WaveSimThread::g_threadsCounter),
    m_thread(threadThunk, this)
{
}

void WaveSimThread::Init(NvStopWatch* stopWatch) {
	m_threadStopWatch = stopWatch;
	g_globalWaveSimThreadCondition = new Sync;
}

void WaveSimThread::runSimulation()
{
    m_localStartCondition.set();
}

void WaveSimThread::waitForAllThreads()
{
    while(g_runningThreads>0)
    {
        g_globalWaveSimThreadCondition->wait();
        g_globalWaveSimThreadCondition->reset();
    }
}

void WaveSimThread::pauseAllThreads()
{
    // ???? Did this ever do anything?
//	g_globalWaveSimThreadCondition.Acquire();
//	g_runningThreads = 0;
//	g_globalWaveSimThreadCondition.Release();
}

void WaveSimThread::Run()
{
	while(!m_thread.quitIsSignalled())
	{
        atomicIncrement(&g_runningThreads); 

		m_startTime = m_threadStopWatch->getTime();

		//TODO:
		float timestep = 1.0f;

		m_simulation->simulate(timestep);
		m_simulation->calcGradients();

		m_endTime = m_threadStopWatch->getTime();
		//LOGI("Thread %d = [start, end] - Exec Time : [%f, %f] - %f\n", id, startTime, endTime, endTime - startTime);

        atomicDecrement(&g_runningThreads);
        g_globalWaveSimThreadCondition->set();

        if (!m_thread.quitIsSignalled()) {
            m_localStartCondition.wait();
            m_localStartCondition.reset();
        }
	}
	
	//printf("Thread %d has finished.\n", id);
}

void* WaveSimThread::threadThunk(void* thiz) 
{
    ((WaveSimThread*)thiz)->Run();
    return NULL;
}

WaveSimThread::~WaveSimThread()
{
	m_thread.signalQuit();
	m_localStartCondition.set();
	m_thread.waitForQuit();
    atomicDecrement(&g_threadsCounter);
}
