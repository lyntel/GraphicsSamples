//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation\assets\shaders/water.vert
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
#version 430

uniform mat4 ModelViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;

uniform mat4 ModelMatrix;

uniform vec4 eyePositionWorld;

attribute vec2 vPositionXZ;
attribute float vPositionY;
attribute vec2 vGradient;

varying vec3 reflectedCubeMapCoord;
varying vec3 refractedCubeMapCoord;
varying vec3 surfaceNormal;

varying float fresnel;

float pow2(float x)
{
	return x*x;
}

void main()
{
	vec3 vNormal = normalize(vec3(vGradient.x, 1.0, vGradient.y));
	
	vec3 normal = normalize(mat3(NormalMatrix) * vNormal);

	vec4 vPosEyeSpace = ModelViewMatrix * vec4(vPositionXZ.x, vPositionY, vPositionXZ.y, 1.0); 
	
	gl_Position = ProjectionMatrix * vPosEyeSpace;
	
	vec3 viewVec = -normalize(vPosEyeSpace.xyz);
	
	//fresnel term
	fresnel = 0.12 + ( 0.88 * pow2( 1.0 - abs( dot(normal, viewVec) ) ) );
	
	vec4 vPosWorld = ModelMatrix * vec4(vPositionXZ.x, vPositionY, vPositionXZ.y, 1.0);
	vec3 viewVecWorld = normalize(vPosWorld.xyz - eyePositionWorld.xyz);
	
	refractedCubeMapCoord = (viewVecWorld - vNormal*0.16);
	reflectedCubeMapCoord = reflect(viewVecWorld, normalize(vNormal));

	surfaceNormal = vNormal.xzy;
}