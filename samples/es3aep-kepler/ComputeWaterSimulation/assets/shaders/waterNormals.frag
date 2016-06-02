//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation\assets\shaders/waterNormals.frag
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
uniform vec4 vColor;
uniform samplerCube skyTexCube;

varying vec3 reflectedCubeMapCoord;
varying vec3 refractedCubeMapCoord;
varying vec3 surfaceNormal;

varying float fresnel;

void main()
{
	vec4 refraction = textureCube(skyTexCube, refractedCubeMapCoord);
	vec4 reflection = textureCube(skyTexCube, reflectedCubeMapCoord);
	vec4 finalColor = (vColor*0.10) + ((reflection*fresnel) + refraction*(1.0-fresnel))*0.9;
	//float4 finalColor = (vColor*0.1) + refraction*(1.0-fresnel)*0.9;
	
	//transparency                                       
	finalColor.a =0.5 + 0.5*fresnel;
	
	gl_FragColor = finalColor;
	
	//gl_FragColor = (ccol * light_ambientdiffuse) + float4(light_specular);
//	gl_FragColor.xyz = vec3(fresnel);
	gl_FragColor.xyz = 0.5 + 0.5 * surfaceNormal;
}
