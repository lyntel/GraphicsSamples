//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation\assets\shaders/generateTerrain_fs.glsl
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

#UNIFORMS

in block {
    mediump vec2 texCoord;
} In;

layout(location=0) out vec4 fragColor;

void main()
{
    vec3 pos = gridOrigin + vec3(In.texCoord.x * float(gridW) * tileSize.x, 0.0, In.texCoord.y * float(gridH) * tileSize.z);
    float h = terrain(pos.xz);
    pos.y = h;

    // calculate normal
    vec2 triSize = tileSize.xz / 64.0;
    vec3 pos_dx = pos.xyz + vec3(triSize.x, 0.0, 0.0);
    vec3 pos_dz = pos.xyz + vec3(0.0, 0.0, triSize.y);
    pos_dx.y = terrain(pos_dx.xz);
    pos_dz.y = terrain(pos_dz.xz);
    vec3 n = normalize(cross(pos_dz - pos.xyz, pos_dx - pos.xyz));

    fragColor = vec4(n, h);
}
