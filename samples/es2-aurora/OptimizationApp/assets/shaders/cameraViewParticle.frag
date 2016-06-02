//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp\assets\shaders/cameraViewParticle.frag
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
#version 300 es

precision mediump float;

uniform lowp float g_pointRadius;
uniform lowp float g_alpha;
uniform mediump vec2 g_invViewport;
uniform mediump vec2 g_depthConstants;
uniform mediump float g_depthDeltaScale;
uniform sampler2D g_depthTex;

in mediump vec4 eyeSpacePos;
in lowp float perParticleColor;

in float lightDepth;

out vec4 fragColor;

mediump float fetchLinearDepth(mediump vec2 uv)
{
	// texture texcoord arguments should always be mediump
	// otherwise they will be converted to mediump with alu ops.
	return 1.0 / (texture(g_depthTex, uv).x * g_depthConstants.x + g_depthConstants.y);
}

void main()
{
	lowp vec3 color = vec3(0.6, 0.5, 0.3);
	lowp vec2 spriteTexCoord = vec2(1.0) - gl_PointCoord.xy;

    // calculate eye-space sphere normal from texture coordinates
	lowp vec3 N;
    N.xy = spriteTexCoord.xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);

    lowp float r2 = dot(N.xy, N.xy);
    lowp float alpha = 1.0 - r2;
    alpha *= g_alpha;

    // "soft" particles
    mediump vec2 uv = (floor(gl_FragCoord.xy) + vec2(0.5)) * g_invViewport;
    mediump float sceneEyeDepth = fetchLinearDepth(uv);
    alpha *= clamp((eyeSpacePos.z - sceneEyeDepth) * g_depthDeltaScale, 0.0, 1.0);

	fragColor = vec4(perParticleColor * color.xyz * alpha, alpha);  // premul alpha
}
