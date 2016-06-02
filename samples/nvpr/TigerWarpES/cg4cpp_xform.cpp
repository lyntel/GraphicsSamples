
/* xform.cpp - C++ transform routines for path rendering */

// Copyright (c) NVIDIA Corporation. All rights reserved.

// Matrix convention: row major (C style arrays)

#include <math.h>

#include <NvSimpleTypes.h>
#include <NV/NvPlatformGL.h>

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/mul.hpp>
#include <Cg/transpose.hpp>
#include <Cg/stdlib.hpp>
//----------------------------------------------------------------------------------
// File:        nvpr\TigerWarpES/cg4cpp_xform.cpp
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
#include <Cg/iostream.hpp>

#include "cg4cpp_xform.hpp"

using namespace Cg;

float3x3 ortho(float l, float r, float b, float t)
{
  float3x3 rv = float3x3(2/(r-l),0,-(r+l)/(r-l),
                         0,2/(t-b),-(t+b)/(t-b),
                         0,0,1);
  return rv;
}

float3x3 inverse_ortho(float l, float r, float b, float t)
{
  float3x3 rv = float3x3((r-l)/2,0,(r+l)/2,
                         0,(t-b)/2,(t+b)/2,
                         0,0,1);
  return rv;
}

float3x3 translate(float x, float y)
{
  float3x3 rv = float3x3(1,0,x,
                         0,1,y,
                         0,0,1);
  return rv;
}

float3x3 scale(float x, float y)
{
  float3x3 rv = float3x3(x,0,0,
                         0,y,0,
                         0,0,1);
  return rv;
}

float3x3 scale(float s)
{
  float3x3 rv = float3x3(s,0,0,
                         0,s,0,
                         0,0,1);
  return rv;
}

float3x3 rotate(float angle)
{
  float radians = angle*3.14159f/180.0f,
        s = ::sin(radians),
        c = ::cos(radians);
  float3x3 rv = float3x3(c,-s,0,
                         s,c,0,
                         0,0,1);
  return rv;
}

// Math from page 54-56 of "Digital Image Warping" by George Wolberg,
// though credited to Paul Heckert's "Fundamentals of Texture
// Mapping and Image Warping" 1989 Master's thesis.
//
// NOTE: This matrix assumes vectors are treated as rows so pre-multiplied
// by the matrix; use the transpose when transforming OpenGL-style column
// vectors.
double3x3 square2quad(const float2 v[4])
{
  double3x3 a;

  double2 d1 = double2(v[1]-v[2]),
          d2 = double2(v[3]-v[2]),
          d3 = double2(v[0]-v[1]+v[2]-v[3]);

  double denom = d1.x*d2.y - d2.x*d1.y;
  a._13 = (d3.x*d2.y - d2.x*d3.y) / denom;
  a._23 = (d1.x*d3.y - d3.x*d1.y) / denom;
  a._11_12 = v[1] - v[0] + a._13*v[1];
  a._21_22 = v[3] - v[0] + a._23*v[3];
  a._31_32 = v[0];
  a._33 = 1;

  return transpose(a);
}

double3x3 quad2square(const float2 v[4])
{
  return inverse(square2quad(v));
}

double3x3 quad2quad(const float2 from[4], const float2 to[4])
{
  return mul(square2quad(to), quad2square(from));
}

double3x3 box2quad(const float4 &box, const float2 to[4])
{
  float2 from[4] = { box.xy, box.zy, box.zw, box.xw };

  return quad2quad(from, to);
}

static void float3x3_to_GLMatrix(GLfloat mm[16], const float3x3 &m)
{
  /* First column. */
  mm[0] = m[0][0];
  mm[1] = m[1][0];
  mm[2] = 0;
  mm[3] = m[2][0];

  /* Second column. */
  mm[4] = m[0][1];
  mm[5] = m[1][1];
  mm[6] = 0;
  mm[7] = m[2][1];;

  /* Third column. */
  mm[8] = 0;
  mm[9] = 0;
  mm[10] = 1;
  mm[11] = 0;

  /* Fourth column. */
  mm[12] = m[0][2];
  mm[13] = m[1][2];
  mm[14] = 0;
  mm[15] = m[2][2];;
}

void MatrixMultToGL(float3x3 m)
{
  GLfloat mm[16];  /* Column-major OpenGL-style 4x4 matrix. */

  float3x3_to_GLMatrix(mm, m);
  glMatrixMultfEXT(GL_MODELVIEW, &mm[0]);
}

void MatrixLoadToGL(float3x3 m)
{
  GLfloat mm[16];  /* Column-major OpenGL-style 4x4 matrix. */

  float3x3_to_GLMatrix(mm, m);
  glMatrixLoadfEXT(GL_MODELVIEW, &mm[0]);
}
