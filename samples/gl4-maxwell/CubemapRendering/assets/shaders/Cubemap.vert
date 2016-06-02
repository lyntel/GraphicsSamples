//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CubemapRendering\assets\shaders/Cubemap.vert
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

layout (location = 0) in vec4 in_position;
layout (location = 1) in vec3 in_normal;

out VertexData {
	vec3 normal;
} outData;

#if USE_CULLING
out float planeMask;
#endif

uniform mat4 g_modelMatrix;
uniform mat4 g_viewMatrix;

#if USE_CULLING
int getVertexPlaneMask(vec3 v)
{ 
	return int(v.x < v.y) | 
		(int(v.x < -v.y) << 1) | 
		(int(v.x <  v.z) << 2) | 
		(int(v.x < -v.z) << 3) | 
		(int(v.z <  v.y) << 4) | 
		(int(v.z < -v.y) << 5) |
		(int(v.x <    1) << 8) |
		(int(v.x >   -1) << 9) |
		(int(v.y <    1) << 10) |
		(int(v.y >   -1) << 11) |
		(int(v.z <    1) << 12) |
		(int(v.z >   -1) << 13);
}
#endif

void main(void) {
    vec4 worldPos = g_modelMatrix * in_position;
    vec4 viewPos = g_viewMatrix * worldPos;
	viewPos.xyz /= viewPos.w;
	viewPos.w = 1;
    gl_Position = viewPos;

	outData.normal = in_normal;
#if USE_CULLING
	planeMask = float(getVertexPlaneMask(worldPos.xyz));
#endif
}
