//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury\assets\shaders/renderQuadFS.glsl
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
#extension GL_EXT_shader_io_blocks : enable
precision highp float;
#UNIFORMS

layout(location=0) out vec4 fragColor;
layout(binding=3)uniform sampler2D gbuf0; // albedo + stencil
layout(binding=4)uniform sampler2D gbuf1; // normal + depth
layout(binding=5)uniform sampler2D gbuf2; // world space coords


in block {
	vec4 color;
	vec2 textureCoords;
} In;

void main() {

	// Draw background
	vec4 viewDir_WorldSpace = InvViewMatrix * vec4(0.0,0.0,-1.0, 0.0);
	float stencilBG = pow(texture(gbuf1,In.textureCoords).w, 100.0);
	vec3 color = (vec3(0.2,0.2,0.2) + vec3(sin(In.textureCoords.y), sin(In.textureCoords.y), sin(In.textureCoords.y))) * 0.2;
	vec3 finalColor = color * stencilBG;

	// Draw foreground
	vec4 normal_ViewSpace = vec4(texture(gbuf1,In.textureCoords).xyz, 0.0);	
	vec4 lightPos = vec4(-12.0, 2.0, -30.0, 1.0);
	vec4 origin = vec4(0.0,0.0,0.0,1.0);

	vec4 lightDir_ViewSpace = ModelView * normalize(origin - lightPos);

	vec3 viewDir = vec3(0.0,0.0,-1.0);

	vec3 reflectedLight = reflect(lightDir_ViewSpace.xyz, normal_ViewSpace.xyz);
	float specExponent = 100.0;

	color = vec3(sin(viewDir_WorldSpace.x), cos(In.textureCoords.y), viewDir_WorldSpace.z) * 0.4;
	float diffuse = max(0.3,dot(normalize(normal_ViewSpace.xyz),lightDir_ViewSpace.xyz)) * 2.0; 
	float specular = pow(max(0.1,dot(reflectedLight, viewDir)), specExponent);

	float stencilFG = 1.0 - stencilBG;	
    finalColor +=  color * (diffuse + specular ) * stencilFG;

    fragColor = vec4(finalColor, 1.0);
}
