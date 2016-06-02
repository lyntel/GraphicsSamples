//----------------------------------------------------------------------------------
// File:        gl4-maxwell\NormalBlendedDecal\assets\shaders/combiner_fragment.glsl
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
in vec2 vTexCoord;

layout(binding = 0) uniform sampler2D uColorTex;
layout(binding = 1) uniform sampler2D uNormalTex;

uniform mat4 uViewMatrix;
uniform bool uShowDecalBox;
uniform bool uShowNormals;
uniform vec4 uLightDir;
uniform vec4 uViewDir;

layout(location=0) out vec4 outColor;

void main(void)
{
    // Screen coordinates
    vec2 screenCoord = vTexCoord;

    // Get color
    vec4 color = texture2D(uColorTex, screenCoord).rgba;

    // Get normal
    vec4 normal = texture2D(uNormalTex, screenCoord).rgba;

    // Diffuse color
    float diffuse = clamp(dot(normal.xyz, normalize(uLightDir.xyz)), 0, 1);

    // Specular color
    float specular = pow(max(dot(reflect(uLightDir.xyz, normal.xyz), uViewDir.xyz), 0.0), 100.0);

    // Example material color
    vec3 matColor = vec3(118.0f/255.0f, 185.0f/255.0f, 0); // Green color

    // Simple diffuse + specular lighting
    outColor.rgb = matColor*diffuse + vec3(specular);
    outColor.a = 1.0f;

    // Override with normal when normal view option in on
    if (uShowNormals) {
        outColor.rgb = normal.xyz*.5f + .5f;
    }

    // Add decal color when decal box view option is on
    if (uShowDecalBox) {
        outColor.rgba += color;
    }
}
