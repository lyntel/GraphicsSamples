//----------------------------------------------------------------------------------
// File:        gl4-kepler\MultiDrawIndirect\assets\shaders/scenecolormdi.vert
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
#version 420

precision mediump float;

// INPUT
layout(location = 0) in vec4 a_vPosition;   // In BaseShader
layout(location = 1) in vec3 a_vNormal;     // In BaseProjNormalShader
layout(location = 2) in vec2 a_vTexCoord;   // In SceneColorShader
layout(location = 3) in vec2 a_vInstance;   // In SceneColorShader

// OUTPUT
out vec2 v_vTexcoord;
out float v_fLightIntensity;

// UNIFORMS
uniform mat4            u_mProjectionMat;      // In BaseProjectionShader
uniform mat4            u_mNormalMat;          // In BaseProjNormalShader
uniform mat4            u_mModelViewMat;       // In SceneColorShader

uniform vec4            u_vLightPosition;      // In SceneColorShader
uniform float           u_fLightAmbient;      // In SceneColorShader
uniform float           u_fLightDiffuse;      // In SceneColorShader
uniform float           u_fLightSpecular;     // In SceneColorShader
uniform float           u_fLightShininess;    // In SceneColorShader

void main()
{
    vec4 vPosEyeSpace = u_mModelViewMat * (a_vPosition + vec4(a_vInstance.x, 0.0f, a_vInstance.y, 0.0f));

    gl_Position = u_mProjectionMat * vPosEyeSpace;

    vec3 normal = normalize(mat3(u_mModelViewMat) * a_vNormal);

    vec3 lDir;

    if(u_vLightPosition.w == 0.0)
        lDir = normalize(u_vLightPosition.xyz);
    else
        lDir = normalize(u_vLightPosition.xyz - vPosEyeSpace.xyz);

    //here half vector = normalize (lightVector+eyeVector)
    //but the vPosEye if vertexposition in eye space (eye = 0,0,0)
    //hence vector from vertex to eye = [-vPosEye]

    vec3 halfVector = normalize(lDir - normalize(vPosEyeSpace.xyz));
    v_fLightIntensity = u_fLightAmbient;

    float NdotL = max(dot(normal,lDir),0.0), NdotHV;

    if (NdotL > 0.0) 
    {
        // cosine (dot product) with the normal

        v_fLightIntensity += u_fLightDiffuse * NdotL;
        NdotHV = max(dot(normal, halfVector),0.0);
        v_fLightIntensity += u_fLightSpecular * pow(NdotHV, u_fLightShininess);
    }

    v_vTexcoord = a_vTexCoord;
}
