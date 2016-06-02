//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/WaveSimThread.h
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
#ifndef _WAVE_SIM_THREAD_
#define _WAVE_SIM_THREAD_

#include <NvSimpleTypes.h>
#include <NsThread.h>
#include <NsSync.h>

#include "WaveSim.h"
#include "NV/NvPlatformGL.h"
#include "NV/NvLogs.h"


class NvStopWatch;

//#include <StopWatch.h>
//#include <nvports/androidlogs.h>

//The class to handle the threads for simulation
class WaveSimThread
{

public:
	//ctor
	WaveSimThread(WaveSim *sim);

	//dtor
	~WaveSimThread();

	//implementing the run method called by the Start() method from r3::Thread class
	void Run();

	//get the Start Time of the Thread
	float getStartTime() const{ return m_startTime; }

	//get the end time of the Thread
	float getEndTime() const{ return m_endTime; }

	//get the execution time of the Thread
	float getExecTime() const{ return m_endTime - m_startTime; }

	//run a simulation on the respective thread
	void runSimulation();

	//wait for ay threads still running the simulation
	static void waitForAllThreads();

	//pause all the threads
	static void pauseAllThreads();

    void start()
    {
        m_thread.start();
    }
    
    //stop the thread
	void stop()
	{ 
		m_thread.signalQuit();
        runSimulation();
	}
	
	//reset the thread counter
	static void resetCounter(){ g_threadsCounter = 0; }

	// a watch to compute the time
	static NvStopWatch& getThreadStopWatch() { return *m_threadStopWatch; }

	static void Init(NvStopWatch* stopWatch);

private:
	static NvStopWatch* m_threadStopWatch;

	// a global threads counter
	static int g_threadsCounter;

	//a global counter to specify the number of currently running threads
	static int g_runningThreads;

	//global Condition Variable for thread synchronization
	static nvidia::shdfnd::Sync* g_globalWaveSimThreadCondition;

	//an ID for the thread
	int m_threadId;

	//variables to store the start and the end times
	float m_startTime, m_endTime;

	//pointer to the simulation object
	WaveSim *m_simulation;

	//local condition variable to handle the running of the next loop (simulation)
	nvidia::shdfnd::Sync m_localStartCondition;

    static void* threadThunk(void* thiz);
    nvidia::shdfnd::Thread m_thread;
};


#endif
