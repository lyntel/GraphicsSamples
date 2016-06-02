//----------------------------------------------------------------------------------
// File:        nvpr\Tiger3DES/cg4cpp_xform.hpp
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
#ifndef XFORM_HPP
#define XFORM_HPP

/* cg4cpp_xform.hpp - C++ transform routines for path rendering */

#include <Cg/double.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>

extern Cg::float3x3 ortho(float l, float r, float b, float t);
extern Cg::float3x3 inverse_ortho(float l, float r, float b, float t);
extern Cg::float3x3 translate(float x, float y);
extern Cg::float3x3 scale(float x, float y);
extern Cg::float3x3 scale(float s);
extern Cg::float3x3 rotate(float angle);

extern Cg::double3x3 square2quad(const Cg::float2 v[4]);
extern Cg::double3x3 quad2square(const Cg::float2 v[4]);
extern Cg::double3x3 quad2quad(const Cg::float2 from[4], const Cg::float2 to[4]);
extern Cg::double3x3 box2quad(const Cg::float4 &box, const Cg::float2 to[4]);

extern void MatrixMultToGL(Cg::float3x3 m);
extern void MatrixLoadToGL(Cg::float3x3 m);

#endif /* XFORM_HPP */
