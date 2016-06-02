//----------------------------------------------------------------------------------
// File:        gl4-kepler\DeferredShadingMSAA\assets\shaders/Lighting_PerSample_FS.glsl
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
layout(location = 0) in vec4 inProjPos;

layout(location = 0) out vec4 outColor;

uniform int uLightingModel;

uniform sampler2DMS uTexGBuffer1;
uniform sampler2DMS uTexGBuffer2;
uniform usampler2DMS uTexCoverage;
uniform sampler2DRect uResolvedColorBuffer;

uniform mat4 uProjViewMatrix;
uniform mat4 uViewWorldMatrix;
const float cRoughness = 0.8;

void main(void)
{
    vec3 diffuse = vec3(0.0);
    vec3 dirToLight = normalize(vec3(-1, 1, -1));
    vec3 lightColor = vec3(1, 1, 1);

    // Read G-Buffer
    vec4 gBuf = texelFetch(uTexGBuffer1, ivec2(gl_FragCoord.xy), gl_SampleID);
    vec3 worldNormal = gBuf.xyz;
    float viewLinearDepth = gBuf.w;
    
    vec3 mtlColor = texelFetch(uTexGBuffer2, ivec2(gl_FragCoord.xy), gl_SampleID).rgb;

    // Calculate position
    vec3 viewPos = DepthToPosition(viewLinearDepth, inProjPos, uProjViewMatrix, 100.0f);
    vec3 worldPos = (uViewWorldMatrix * vec4(viewPos, 1.0f)).xyz;

    // Calculate to eye vector
    vec3 eyeWorldPos = (uViewWorldMatrix * vec4(0, 0, 0, 1)).xyz;
    vec3 toEyeVector = normalize(eyeWorldPos - worldPos);
    float normLength = length(worldNormal);
    if (normLength > 0.0)
    {
        vec3 normal = worldNormal / normLength;
        switch (uLightingModel)
        {
        case 1:
            diffuse += OrenNayar(dirToLight, toEyeVector, normal, lightColor, mtlColor, cRoughness);
            break;
        case 0:
        default:
            diffuse += LambertDiffuse(dirToLight, normal, lightColor, mtlColor);
            break;
        }
    }
    else
    {
        diffuse += vec3(0.2, 0.5, 1.0);
    }

    outColor = vec4(diffuse, 1.0);
}
