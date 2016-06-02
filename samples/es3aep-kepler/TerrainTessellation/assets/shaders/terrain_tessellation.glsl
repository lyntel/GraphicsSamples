//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation\assets\shaders/terrain_tessellation.glsl
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
// Version tag will be added in code
#extension GL_ARB_tessellation_shader : enable

#define PROCEDURAL_TERRAIN 1

#UNIFORMS
#line 8
layout(quads, fractional_even_spacing, cw) in;

in gl_PerVertex {
    vec4 gl_Position;
} gl_in[];

layout(location=1) in block {
    mediump vec2 texCoord;
    vec2 tessLevelInner;
} In[];

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location=1) out block {
    vec3 vertex;
    vec3 vertexEye;
    vec3 normal;
} Out;

uniform sampler2D terrainTex;

void main(){

    vec3 pos = gl_in[0].gl_Position.xyz;
    pos.xz += gl_TessCoord.xy * tileSize.xz;

#if PROCEDURAL_TERRAIN
    // calculate terrain height procedurally
    float h = terrain(pos.xz);
    vec3 n = vec3(0, 1, 0);
    pos.y = h;

    // calculate normal
    vec2 triSize = tileSize.xz / In[0].tessLevelInner;
    vec3 pos_dx = pos.xyz + vec3(triSize.x, 0.0, 0.0);
    vec3 pos_dz = pos.xyz + vec3(0.0, 0.0, triSize.y);
    pos_dx.y = terrain(pos_dx.xz);
    pos_dz.y = terrain(pos_dz.xz);
    n = normalize(cross(pos_dz - pos.xyz, pos_dx - pos.xyz));

#else

    // read from pre-calculated texture
    vec2 uv = In[0].texCoord + (vec2(1.0 / gridW, 1.0 / gridH) * gl_TessCoord.xy);
    vec4 t = texture2D(terrainTex, uv);
    float h = t.w;
    pos.y = t.w;
    vec3 n = t.xyz;
#endif

    Out.normal = n;

    Out.vertex = pos;
    Out.vertexEye = vec3(ModelView * vec4(pos, 1));  // eye space

    gl_Position = ModelViewProjection * vec4(pos, 1);
}
