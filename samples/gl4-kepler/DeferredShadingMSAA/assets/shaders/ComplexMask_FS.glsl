//----------------------------------------------------------------------------------
// File:        gl4-kepler\DeferredShadingMSAA\assets\shaders/ComplexMask_FS.glsl
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
layout(location=0) out vec4 outColor;

uniform sampler2DMS uTexGBuffer1;
uniform sampler2DMS uTexGBuffer2;
uniform usampler2DMS uTexCoverage;
uniform sampler2DRect uResolvedColorBuffer;
uniform int uUseDiscontinuity;

const uint cMSAACount = 4;

// Coverage Mask version
void main(void)
{
    if (uUseDiscontinuity > 0.5)
    {
        vec4 gBuf1 = texelFetch(uTexGBuffer1, ivec2(gl_FragCoord.xy), 0);
        vec4 gBuf2 = texelFetch(uTexGBuffer2, ivec2(gl_FragCoord.xy), 0);

        vec3 normal = gBuf1.xyz;
        float depth = gBuf1.w;
        vec3 albedo = gBuf2.rgb;

        for (int i = 1; i < cMSAACount; ++i)
        {
            vec4 gBuf1next = texelFetch(uTexGBuffer1, ivec2(gl_FragCoord.xy), i);
            vec4 gBuf2next = texelFetch(uTexGBuffer2, ivec2(gl_FragCoord.xy), i);

            vec3 nextNormal = gBuf1next.xyz;
            float nextDepth = gBuf1next.w;
            vec3 nextAlbedo = gBuf2next.rgb;

            if (abs(depth - nextDepth) > 0.1f ||
                abs(dot(abs(normal - nextNormal), vec3(1, 1, 1))) > 0.1f ||
                abs(dot(albedo - nextAlbedo, vec3(1, 1, 1))) > 0.1f)
            {
                discard;
            }
        }
    }
    else if (texture(uResolvedColorBuffer, gl_FragCoord.xy).a > 0.02)
    {
        discard;
    }

    outColor = vec4(0.0, 0.0, 0.0, 0.0);
}
