//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/CubeData.h
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
#ifndef _CUBEDATA_H_
#define _CUBEDATA_H_

#define TEX_COORD_MINX 0.0f
#define TEX_COORD_MAXX 1.0f
#define TEX_COORD_MINY 1.0f
#define TEX_COORD_MAXY 0.0f
#define CUBE_SCLAE 6.0f

// Interleaved vertex data
float verticesCube[] = {
	// Front Face
	-CUBE_SCLAE,-CUBE_SCLAE, CUBE_SCLAE,
	0.0f, 0.0f, 1.0f,
	TEX_COORD_MINX, TEX_COORD_MINY,
	CUBE_SCLAE,-CUBE_SCLAE, CUBE_SCLAE,
	0.0f, 0.0f, 1.0f,
	TEX_COORD_MAXX, TEX_COORD_MINY,
	CUBE_SCLAE, CUBE_SCLAE, CUBE_SCLAE,
	0.0f, 0.0f, 1.0f,
	TEX_COORD_MAXX, TEX_COORD_MAXY,
	-CUBE_SCLAE, CUBE_SCLAE, CUBE_SCLAE,
	0.0f, 0.0f, 1.0f,
	TEX_COORD_MINX, TEX_COORD_MAXY,

	// Back Face
	-CUBE_SCLAE,-CUBE_SCLAE,-CUBE_SCLAE,
	0.0f, 0.0f,-1.0f,
	TEX_COORD_MINX, TEX_COORD_MINY,
	CUBE_SCLAE,-CUBE_SCLAE,-CUBE_SCLAE,
	0.0f, 0.0f,-1.0f,
	TEX_COORD_MAXX, TEX_COORD_MINY,
	CUBE_SCLAE, CUBE_SCLAE,-CUBE_SCLAE,
	0.0f, 0.0f,-1.0f,
	TEX_COORD_MAXX, TEX_COORD_MAXY,
	-CUBE_SCLAE, CUBE_SCLAE,-CUBE_SCLAE,
	0.0f, 0.0f,-1.0f,
	TEX_COORD_MINX, TEX_COORD_MAXY,

	// Top Face
	-CUBE_SCLAE, CUBE_SCLAE, CUBE_SCLAE,
	0.0f, 1.0f, 0.0f,
	TEX_COORD_MINX, TEX_COORD_MINY,
	CUBE_SCLAE, CUBE_SCLAE, CUBE_SCLAE,
	0.0f, 1.0f, 0.0f,
	TEX_COORD_MAXX, TEX_COORD_MINY,
	CUBE_SCLAE, CUBE_SCLAE,-CUBE_SCLAE,
	0.0f, 1.0f, 0.0f,
	TEX_COORD_MAXX, TEX_COORD_MAXY,
	-CUBE_SCLAE, CUBE_SCLAE,-CUBE_SCLAE,
	0.0f, 1.0f, 0.0f,
	TEX_COORD_MINX, TEX_COORD_MAXY,

	// Bottom Face
	-CUBE_SCLAE,-CUBE_SCLAE, CUBE_SCLAE,
	0.0f,-1.0f, 0.0f,
	TEX_COORD_MINX, TEX_COORD_MINY,
	CUBE_SCLAE,-CUBE_SCLAE, CUBE_SCLAE,
	0.0f,-1.0f, 0.0f,
	TEX_COORD_MAXX, TEX_COORD_MINY,
	CUBE_SCLAE,-CUBE_SCLAE,-CUBE_SCLAE,
	0.0f,-1.0f, 0.0f,
	TEX_COORD_MAXX, TEX_COORD_MAXY,
	-CUBE_SCLAE,-CUBE_SCLAE,-CUBE_SCLAE,
	0.0f,-1.0f, 0.0f,
	TEX_COORD_MINX, TEX_COORD_MAXY,

	// Left Face
	-CUBE_SCLAE,-CUBE_SCLAE,-CUBE_SCLAE,
	-1.0f, 0.0f, 0.0f,
	TEX_COORD_MINX, TEX_COORD_MINY,
	-CUBE_SCLAE,-CUBE_SCLAE, CUBE_SCLAE,
	-1.0f, 0.0f, 0.0f,
	TEX_COORD_MAXX, TEX_COORD_MINY,
	-CUBE_SCLAE, CUBE_SCLAE, CUBE_SCLAE,
	-1.0f, 0.0f, 0.0f,
	TEX_COORD_MAXX, TEX_COORD_MAXY,
	-CUBE_SCLAE, CUBE_SCLAE,-CUBE_SCLAE,
	-1.0f, 0.0f, 0.0f,
	TEX_COORD_MINX, TEX_COORD_MAXY,

	// Right Face
	CUBE_SCLAE,-CUBE_SCLAE,-CUBE_SCLAE,
	1.0f, 0.0f, 0.0f,
	TEX_COORD_MINX, TEX_COORD_MINY,
	CUBE_SCLAE,-CUBE_SCLAE, CUBE_SCLAE,
	1.0f, 0.0f, 0.0f,
	TEX_COORD_MAXX, TEX_COORD_MINY,
	CUBE_SCLAE, CUBE_SCLAE, CUBE_SCLAE,
	1.0f, 0.0f, 0.0f,
	TEX_COORD_MAXX, TEX_COORD_MAXY,
	CUBE_SCLAE, CUBE_SCLAE,-CUBE_SCLAE,
	1.0f, 0.0f, 0.0f,
	TEX_COORD_MINX, TEX_COORD_MAXY
};

static unsigned short indicesCube[] = {0,1,3,3,1,2,
									4,7,5,7,6,5,
									8,9,11,11,9,10,
									12,15,13,15,14,13,
									16,17,19,19,17,18,
									20,23,21,23,22,21};

#endif  // _CUBEDATA_H_
