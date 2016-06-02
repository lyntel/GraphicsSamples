//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk\assets\src_shaders/staticfish.glsl
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
#GLSL_VS
#version 440 core

precision highp float;

// INPUT
layout(location = 0) in vec3 a_vPosition; // In BaseShader
layout(location = 1) in vec3 a_vNormal;   // In BaseProjNormalShader
layout(location = 2) in vec2 a_vTexCoord; // In SceneColorShader

// Instancing Input
layout(location = 7) in vec3 a_vInstancePos;
layout(location = 8) in vec3 a_vInstanceHeading;
layout(location = 9) in float a_fInstanceAnimTime;

// OUTPUT
layout(location = 0) out vec2 v_vTexcoord;
layout(location = 1) out vec3 v_vLightIntensity;
layout(location = 2) out vec4 v_vPosEyeSpace;
layout(location = 3) out vec4 v_vPosWorldSpace;
layout(location = 4) out vec3 v_vNormalWorldSpace;

// UNIFORMS
layout(set = 0, binding=0) uniform ProjBlock {
	mat4 u_mProjectionMatrix;
	mat4 u_mInverseProjMatrix;
	mat4 u_mViewMatrix;
	mat4 u_mInverseViewMatrix;
};

layout(set = 0, binding=1) uniform LightingBlock {
	vec4 u_vLightPosition;  
	vec4 u_vLightAmbient;   
	vec4 u_vLightDiffuse;   
	float u_fCausticOffset;
	float u_fCausticTiling;
};

layout(set=1, binding=1) uniform ModelBlock {
	mat4 u_mModelMatrix;
	float u_fTailStart;
};

layout(set=1, binding=2) uniform MeshBlock{
    mat4 u_mMeshOffsetMatrix;
};

void main()
{
    vec3 basisZ = -a_vInstanceHeading;
    vec3 basisX = normalize(cross(vec3(0.0f, 1.0f, 0.0f), basisZ));
    vec3 basisY = normalize(cross(basisZ, basisX));

    mat4 instanceXfm = mat4(vec4(basisX, 0.0f), vec4(basisY, 0.0f), vec4(basisZ, 0.0f), vec4(a_vInstancePos, 1.0f));
	mat4 modelXfm = u_mModelMatrix * u_mMeshOffsetMatrix;
	vec4 vPosModelSpace = modelXfm * vec4(a_vPosition.xyz, 1.0);

	// Determine if this vertex is on the tail
	float tailFactor = vPosModelSpace.z - u_fTailStart;
	if (tailFactor < 0.0f)
	{
		tailFactor *= 0.05f;
	}
	float displacement = sin(a_fInstanceAnimTime) * tailFactor;
	vPosModelSpace.x += displacement;
    mat4 worldXfm = instanceXfm;
    v_vPosWorldSpace = worldXfm * vPosModelSpace;
    v_vPosEyeSpace = u_mViewMatrix * vec4(v_vPosWorldSpace.xyz, 1.0);

    gl_Position = u_mProjectionMatrix * v_vPosEyeSpace;

    v_vNormalWorldSpace = normalize(mat3(worldXfm) * mat3(modelXfm) * a_vNormal);

    vec3 lDir;
    lDir = normalize(u_vLightPosition.xyz);
    v_vLightIntensity = u_vLightAmbient.rgb;

    float NdotL = max(dot(v_vNormalWorldSpace, lDir), 0.0);
    if (NdotL > 0.0) 
    {
        // cosine (dot product) with the normal
        v_vLightIntensity += u_vLightDiffuse.rgb * NdotL;
    }
    v_vTexcoord = a_vTexCoord;
}

#GLSL_FS
#version 440 core

precision highp float;

// INPUT
layout(location = 0) in vec2 v_vTexcoord;
layout(location = 1) in vec3 v_vLightIntensity;
layout(location = 2) in vec4 v_vPosEyeSpace;
layout(location = 3) in vec4 v_vPosWorldSpace;
layout(location = 4) in vec3 v_vNormalWorldSpace;

// OUTPUT
layout(location = 0) out vec4 o_vFragColor;

// UNIFORMS
layout(set = 0, binding = 2) uniform sampler2D u_tCaustic1Tex;
layout(set = 0, binding = 3) uniform sampler2D u_tCaustic2Tex;
layout(set = 1, binding = 0) uniform sampler2D u_tDiffuseTex;

layout(set = 0, binding = 1) uniform LightingBlock {
	vec4 u_vLightPosition;  
	vec4 u_vLightAmbient;   
	vec4 u_vLightDiffuse;   
	float u_fCausticOffset;
	float u_fCausticTiling;
};

float getCausticIntensity(vec4 worldPos)
{
	vec2 sharedUV = worldPos.xz * u_fCausticTiling;
	float caustic1 = texture(u_tCaustic1Tex, sharedUV + vec2(u_fCausticOffset, 0.0f)).r;
	float caustic2 = texture(u_tCaustic2Tex, sharedUV * 1.3f + vec2(0.0f, u_fCausticOffset)).r;
	return (caustic1 * caustic2) * 2.0f;
}

float g_maxFogDist = 75.0f;
vec3 g_fogColor = vec3(0.0f, 0.0f, 0.64f);

float fogIntensity(float dist)
{
	// Push the start of the fog range out, so that we have a nice, bright range of fish
	// near the camera with a steeper falloff further out.
	dist -= 10.0f;
	dist *= 0.8;

	float normalizedDist = clamp((dist / g_maxFogDist), 0.0f, 1.0f);

	// Also use a square distance to maintain some color from the fish vs. the ground
	return normalizedDist * normalizedDist; 
}

void main()
{
    o_vFragColor = texture(u_tDiffuseTex, v_vTexcoord);
    if (o_vFragColor.a < 0.5f)
    {
        discard;
    }
    o_vFragColor.rgb *= v_vLightIntensity + vec3(getCausticIntensity(v_vPosWorldSpace));
    float fog = fogIntensity(length(v_vPosEyeSpace.xyz));
    o_vFragColor.rgb = mix(o_vFragColor.rgb, g_fogColor, fog);
}
