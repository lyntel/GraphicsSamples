//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury\assets\shaders/cubesCS.glsl
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
// Version ID added via C-code prefixing
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

precision highp sampler3D;

#UNIFORMS

uniform vec4 startPos;
uniform uint cellsDim;
uniform float cellSize;
uniform float timeStep;

layout( std140, binding=3 ) buffer Pos {
    vec4 c_pos[];
};

layout( std140, binding=4 ) buffer Val {
    float c_val[];
};

layout (std140, binding=5) buffer ParticlePos {
    vec4 particle_pos[];
};

layout (std140, binding=6) buffer ParticleVel {
    vec4 particle_vel[];
};

layout(local_size_x = CELLS_DIM,  local_size_y = CELLS_DIM, local_size_z = CELLS_DIM) in;


// compute shader to update particles
void main() {

    uint i = gl_GlobalInvocationID.x;
    uint j = gl_GlobalInvocationID.y;
    uint k = gl_GlobalInvocationID.z;

    // trivial rejection
    if (i >= cellsDim * 8U || j >= cellsDim || k >= cellsDim) return;

    uint mod = i % 8U;
    uint div = uint(i / 8U);

    vec4 cellStartPos = vec4(startPos.x + float(div) * cellSize, startPos.y + float(j) * cellSize, startPos.z + float(k) * cellSize, 1.0);
    
    // All cell vertices (8)
    uint base = k * 8U * cellsDim * cellsDim + j * 8U * cellsDim + div * 8U;
    uint index = base + mod;


    // Calculation for start positions for each of the 8 vertices
    uint modChk = 0x1U << mod;
    float xSizes = min(float(modChk & 0xCCU), 1.0) * cellSize;
    float ySizes = min(float(modChk & 0x66U), 1.0) * cellSize;
    float zSizes = min(float(modChk & 0xF0U), 1.0) * cellSize;

    c_pos[index] = vec4(cellStartPos.x + xSizes, cellStartPos.y + ySizes, cellStartPos.z + zSizes, 1.0);

    float radii[6] = float[](0.7, 0.55, 0.5, 0.62, 0.64, 0.72);    

    // Generate a ball for each particle    
    vec4 cubeVertexPos = c_pos[base + mod];

    // Overall contribution at a point
    float contribution = 0.0;
    for (uint m = 0U; m < numParticles; m++) {
        vec4 metaballCenter = particle_pos[m];        
        vec4 distance = metaballCenter - cubeVertexPos;

        float radius = radii[m%6U];

        // Subtracting 1 since isovalue has values between 0 and infinity. Marching cubes considers values below 0 as outside the isosurface.
        float isovalue = (radius * radius)/(distance.x * distance.x + distance.y * distance.y + distance.z * distance.z) - 1.0;

        contribution += isovalue;
    }

    c_val[index] = contribution ;    
}