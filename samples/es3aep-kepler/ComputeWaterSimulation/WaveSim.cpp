//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/WaveSim.cpp
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
#include "WaveSim.h"

WaveSim::WaveSim(int w, int h, float damping) :
	m_width(w),
	m_height(h),
    m_v(w, h),
    m_current(0),
    //m_c2(0.5f),
    //m_h2(1.0f),
    m_damping(damping)
{
	m_u[0].init(m_width,m_height);
	m_u[1].init(m_width,m_height);
	m_gradients = new float[m_width*m_height*2];
    reset();
}

WaveSim::~WaveSim()
{
	delete [] m_gradients;
}

void WaveSim::reset()
{
    int size = m_width * m_height;
    for(int i=0; i<size; i++) {
        m_u[0].data[i] = 0.0f;
        m_u[1].data[i] = 0.0f;
        m_v.data[i] = 0.0f;
    }
}

void WaveSim::setDamping(float _damping)
{
	m_damping = _damping;
}

void WaveSim::addDisturbance(float x, float y, float r, float s)
{
    int ix = (int) floorf(x);
    int iy = (int) floorf(y);
    int ir = (int) ceilf(r);
    int sx = maxi(1, ix - ir);
    int sy = maxi(1, iy - ir);
    int ex = mini(ix + ir, m_width-2);
    int ey = mini(iy + ir, m_height-2);
    for(int j=sy; j<=ey; j++) {
        for(int i=sx; i<=ex; i++) {
            float dx = x - i;
            float dy = y - j;
            float d = sqrtf(dx*dx + dy*dy);
			if (d < r) {
				d = (d / r)*3.0f;
				m_u[m_current].get(i, j) += expf(-d*d)*s;
			}
        }
    }
}

// http://www.matthiasmueller.info/talks/GDC2008.pdf
void WaveSim::simulate(float dt)
{
    for(int j=1; j<m_height-1; j++) {
        for(int i=1; i<m_width-1; i++) {
            //m_v.get(i, j) += dt*m_c2*( m_u.getClamp(i-1, j) + m_u.getClamp(i+1, j) + m_u.getClamp(i, j-1) + m_u.getClamp(i, j+1) - 4.0f*m_u.get(i, j) ) / m_h2;
            //m_v.get(i, j) += dt* (m_u.getClamp(i-1, j) + m_u.getClamp(i+1, j) + m_u.getClamp(i, j-1) + m_u.getClamp(i, j+1))*0.25f - m_u.get(i, j);
            m_v.get(i, j) += dt * (m_u[m_current].get(i-1, j) + m_u[m_current].get(i+1, j) + m_u[m_current].get(i, j-1) + m_u[m_current].get(i, j+1))*0.25f - m_u[m_current].get(i, j);
        }
    }

    //float damping = powf(m_damping, dt);
    for(int j=1; j<m_height-1; j++) {
        for(int i=1; i<m_width-1; i++) {
            float &v = m_v.get(i, j);
            v *= m_damping;
            m_u[m_current].get(i, j) += dt*v;
        }
    }
}

void WaveSim::calcGradients()
{
	float *ptr;
	for(int j=1; j<m_height-1; j++)
	{
		ptr = m_gradients + j*m_width*2 + 2;
		for(int i=1; i<m_width-1; i++)
		{
			*ptr++ = (m_u[m_current].get(i+1, j) - m_u[m_current].get(i-1, j));		//dy/dx
			*ptr++ = (m_u[m_current].get(i, j+1) - m_u[m_current].get(i, j-1));		//dy/dz
		}
	}
}
