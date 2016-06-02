//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/WaveSim.h
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
#ifndef _WAVE_SIM_
#define _WAVE_SIM_

#include <NvSimpleTypes.h>
#include "Array2D.h"
#include "NV/NvMath.h"

// simple heightfield fluid surface
class WaveSim
{
public:
	//ctor
    WaveSim(int w, int h, float damping);
    //dtor
    ~WaveSim();

    //reset the heights to 0
    void reset();
    //add a disturbance at a position in the grid
    void addDisturbance(float x, float y, float r, float i);

    //set the damping
    void setDamping(float _damping);

    //simulate using math for indexing
    void simulate(float dt);

    //calculate the gradients
	void calcGradients();

	//get the width of the surface
    int getWidth() { return m_width; }

    //get the height of the surface
    int getHeight() { return m_height; }

    //get the height at a specific grid point
    float getHeight(int i, int j) { return m_v.get(i, j); }

    //get the pointer to the current heights array
    float *getHeightField() { return m_u[m_current].data; }

    //get the pointer to the current gradients array
	float *getGradients() { return m_gradients; }

	// get the pointer to the current velocity array
	float* getVelocity() { return m_v.data; }

	//get the normal at a grid point (x,y)
	nv::vec3f getNormal(int x, int y)
	{
		float *ptr = &m_gradients[y*m_width*2+x*2];
		return nv::vec3f(*ptr, 2.0f, *(ptr+1));
	}

	//variable to store the size in bytes of the heights and gradients arrays
	int m_heightFieldSize, m_gradientsSize;

private:

    Array2D<float> m_u[2];    // height
    Array2D<float> m_v;    // velocity
	
	//float m_scaleY; //used to hold the relative height (y) * 2.0f wrt world coords
	float *m_gradients;

	//used for alternate equations (not used now)
    //float m_c2, m_h2;

	//fuid damping
    float m_damping;

    //width/height of the grid
    int m_width, m_height;

    //and index to the current(front) height array
    int m_current;
};

#endif
