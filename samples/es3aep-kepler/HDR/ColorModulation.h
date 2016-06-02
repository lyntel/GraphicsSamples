//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/ColorModulation.h
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
#ifndef _COLOR_MODULATION_H_
#define _COLOR_MODULATION_H_


float star_modulation1st[16];
float star_modulation2nd[16];
float star_modulation3rd[16];

float filmic_ghost_modulation1st[16];
float filmic_ghost_modulation2nd[16];
float camera_ghost_modulation1st[16];
float camera_ghost_modulation2nd[16];

float hori_modulation1st[16];
float hori_modulation2nd[16];
float hori_modulation3rd[16];


void ColorModulationRedShift(float* color, float r, float g, float b, int num)
{
	color[4*num] = color[4*num]*r;
	color[4*num+1] = color[4*num]*g;
	color[4*num+2] = color[4*num]*b;
	//color[4*num+3] = color[4*num]*a;
}

void ColorModulation(float* color, float r, float g, float b, int num)
{
	color[4*num] = color[4*num+0]*r;
	color[4*num+1] = color[4*num+1]*g;
	color[4*num+2] = color[4*num+2]*b;
	//color[4*num+3] = color[4*num]*a;
}


void modulateColor()
{
	//color modulation coefficients for star streak & ghost image
	for (int i=0;i<16; i++) {
	   star_modulation1st[i]=0.25;
	   star_modulation2nd[i]=0.25;
	   star_modulation3rd[i]=0.25;
	   hori_modulation1st[i]=0.5;
	   hori_modulation2nd[i]=0.5;
	   hori_modulation3rd[i]=0.5;
	   filmic_ghost_modulation1st[i] = 1.0;
	   filmic_ghost_modulation2nd[i] = 1.0;
	   camera_ghost_modulation1st[i] = 1.0;
	   camera_ghost_modulation2nd[i] = 1.0;
	}
	//star
    ColorModulationRedShift(star_modulation1st, 1.0, 0.95, 0.9,0);
	ColorModulationRedShift(star_modulation1st, 0.8, 1.0, 0.9,1);
	ColorModulationRedShift(star_modulation1st, 0.9, 0.9, 1.0,2);
	ColorModulationRedShift(star_modulation1st, 0.9, 1.0, 0.9,3);

	ColorModulationRedShift(star_modulation2nd, 1.0, 0.9, 0.8,0);
    ColorModulationRedShift(star_modulation2nd, 1.0, 0.6, 0.5,1);
	ColorModulationRedShift(star_modulation2nd, 0.5, 1.0, 0.6,2);
	ColorModulationRedShift(star_modulation2nd, 0.6, 0.4, 1.0,3);

	ColorModulationRedShift(star_modulation3rd, 1.0, 0.6, 0.6,1);
	ColorModulationRedShift(star_modulation3rd, 0.6, 1.0, 0.6,2);
	ColorModulationRedShift(star_modulation3rd, 0.6, 0.6, 1.0,3);

#define BLUE_SHIFT0 1.0, 1.0, 1.0
#define BLUE_SHIFT1 0.2, 0.3, 0.95
#define BLUE_SHIFT2 0.1, 0.2, 0.9
#define BLUE_SHIFT3 0.02, 0.1, 0.99

	//horizontal
    ColorModulation(hori_modulation1st, BLUE_SHIFT1,0);
	ColorModulation(hori_modulation1st, BLUE_SHIFT1,1);
	ColorModulation(hori_modulation1st, BLUE_SHIFT2,2);
	ColorModulation(hori_modulation1st, BLUE_SHIFT1,3);

    ColorModulation(hori_modulation2nd, BLUE_SHIFT1,0);
	ColorModulation(hori_modulation2nd, BLUE_SHIFT2,1);
	ColorModulation(hori_modulation2nd, BLUE_SHIFT3,2);
	ColorModulation(hori_modulation2nd, BLUE_SHIFT3,3);

    ColorModulation(hori_modulation3rd, BLUE_SHIFT0,0);
	ColorModulation(hori_modulation3rd, BLUE_SHIFT0,1);
	ColorModulation(hori_modulation3rd, BLUE_SHIFT0,2);
	ColorModulation(hori_modulation3rd, BLUE_SHIFT0,3);

	//ghost camera
	ColorModulationRedShift(camera_ghost_modulation1st, 1.0, 0.9, 0.8,0);
    ColorModulationRedShift(camera_ghost_modulation1st, 1.0, 0.6, 0.5,1);
	ColorModulationRedShift(camera_ghost_modulation1st, 0.5, 1.0, 0.6,2);
	ColorModulationRedShift(camera_ghost_modulation1st, 1.0, 0.7, 0.3,3);

	ColorModulationRedShift(camera_ghost_modulation2nd, 0.2, 0.3, 0.7,0);
    ColorModulationRedShift(camera_ghost_modulation2nd, 0.5, 0.3, 0.2,1);
	ColorModulationRedShift(camera_ghost_modulation2nd, 0.1, 0.5, 0.2,2);
	ColorModulationRedShift(camera_ghost_modulation2nd, 0.1, 0.1, 1.0,3);

	//ghost filmic
	ColorModulation(filmic_ghost_modulation1st, 0.1, 0.1, 1.0,0);
    ColorModulation(filmic_ghost_modulation1st, 0.2, 0.3, 1.0,1);
	ColorModulation(filmic_ghost_modulation1st, 0.1, 0.2, 0.6,2);
	ColorModulation(filmic_ghost_modulation1st, 0.6, 0.3, 1.0,3);

	ColorModulation(filmic_ghost_modulation2nd, 0.6, 0.2, 0.2,0);
    ColorModulation(filmic_ghost_modulation2nd, 0.2, 0.06, 0.6,1);
	ColorModulation(filmic_ghost_modulation2nd, 0.15, 0.00, 0.1,2);
	ColorModulation(filmic_ghost_modulation2nd, 0.06, 0.00, 0.55,3);
}

#endif  // _COLOR_MODULATION_H_
