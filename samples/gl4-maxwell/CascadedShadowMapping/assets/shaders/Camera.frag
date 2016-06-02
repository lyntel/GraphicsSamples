//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CascadedShadowMapping\assets\shaders/Camera.frag
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
#version 440 core

in vec3 N;
in vec3 L;
in vec3 V;
in vec4 worldPos;

out vec4 color;

layout (binding = 0) uniform sampler2DArrayShadow shadowTex;

// Light view-projection-scale-bias matrix.
uniform mat4 lightVPSBMatrices[4];
uniform vec4 normalizedFarPlanes;
uniform vec3 ambient;
uniform vec3 diffuseAlbedo;
uniform vec3 specularAlbedo;
uniform float specularPower;

// Debug tool to emphasize different shadow maps.
vec4 layerColor;

vec3 computeShadowCoords(int slice)
{
    // Orthographic projection doesn't need division by w.
    vec4 shadowCoords = lightVPSBMatrices[slice] * worldPos;
    return shadowCoords.xyz;
}

float computeShadowCoef()
{
    int slice = 3;
    layerColor = vec4(1.0, 0.5, 1.0, 1.0);
    if (gl_FragCoord.z < normalizedFarPlanes.x) {
        slice = 0;
        layerColor = vec4(1.0, 0.5, 0.5, 1.0);
    } else if (gl_FragCoord.z < normalizedFarPlanes.y) {
        slice = 1;
        layerColor = vec4(0.5, 1.0, 0.5, 1.0);
    } else if (gl_FragCoord.z < normalizedFarPlanes.z) {
        slice = 2;
        layerColor = vec4(0.5, 0.5, 1.0, 1.0);
    }

    vec4 shadowCoords;
    // Swizzling specific for shadow sampler.
    shadowCoords.xyw = computeShadowCoords(slice);

    // Bias to reduce shadow acne.
    shadowCoords.w -= 0.001;

    shadowCoords.z = float(slice);
    return texture(shadowTex, shadowCoords);
}

void main(void) {
    vec3 nN = normalize(N);
    vec3 nL = normalize(L);
    vec3 nV = normalize(V);

    vec3 R = reflect(nL, nN);

    float cosTheta = max(dot(nN, nL), 0.0);

    vec3 diffuse = max(cosTheta, 0.0) * diffuseAlbedo;
    vec3 specular = pow(max(dot(R, nV), 0.0), specularPower) * specularAlbedo;

    color = vec4(ambient, 1.0);
    color += computeShadowCoef() * vec4(diffuse + specular, 1.0) * layerColor;
}
