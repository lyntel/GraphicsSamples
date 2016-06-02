//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CubemapRendering\assets\shaders/CubemapFgs.geom
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
#extension GL_EXT_geometry_shader4 : enable
#extension GL_NV_viewport_array2 : enable
#extension GL_NV_geometry_shader_passthrough : require

layout(triangles) in;
layout(passthrough) in gl_PerVertex {
	vec4 gl_Position;
};

layout(passthrough) in VertexData {
	vec3 normal;
} passthroughData[];

#if USE_CULLING
in float planeMask[];
#endif

layout(viewport_relative) out int gl_Layer;

void main()
{
#if USE_CULLING
	int pm0 = int(planeMask[0]);
	int pm1 = int(planeMask[1]);
	int pm2 = int(planeMask[2]);
	int prim_plane_mask_0 = pm0 & pm1 & pm2;
	int prim_plane_mask_1 = ~pm0 & ~pm1 & ~pm2;
	int combined_mask = prim_plane_mask_0 | (prim_plane_mask_1 << 16);

	int face_mask = 0;
	if((combined_mask & 0x00010f) == 0) face_mask |= 0x01;
	if((combined_mask & 0x0f0200) == 0) face_mask |= 0x02;
	if((combined_mask & 0x110422) == 0) face_mask |= 0x04;
	if((combined_mask & 0x220811) == 0) face_mask |= 0x08;
	if((combined_mask & 0x041038) == 0) face_mask |= 0x10;
	if((combined_mask & 0x382004) == 0) face_mask |= 0x20;
	gl_ViewportMask[0] = face_mask;
#else
	gl_ViewportMask[0] = 0x3f;
#endif
}
