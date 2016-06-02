//----------------------------------------------------------------------------------
// File:        gl4-maxwell\NormalBlendedDecal\assets\shaders/decal_fragment.glsl
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
#if USE_PIXEL_SHADER_INTERLOCK
// Need this extension for begin/endInvocationInterlockNV()
#extension GL_NV_fragment_shader_interlock : enable
// Invocation order is not important in this sample
layout(pixel_interlock_unordered) in;
#endif

layout(location=0) in vec4 vColor;
layout(location=1) in vec4 vNormal;
layout(location=2) in vec4 vWorldPos;
layout(location=3) in vec2 vTexCoord;
layout(location=4) in vec4 vTangent;

// Target normal image to read and write
layout(binding = 0, rgba16f) uniform coherent image2D uNormalImage;

layout(binding = 0) uniform sampler2D uWorldPosTex;

// Source normal texture to blend into the existing normal image
layout(binding = 2) uniform sampler2D uDecalNormalTex;

uniform mat4 uViewMatrix; // World to view conversion
uniform mat4 uInvModelMatrix; // World to local conversion
uniform vec4 uScreenSize; // Screen size in pixel unit
uniform float uBlendWeight; // Weight factor to blend normals
uniform bool uShowDecalBox; // Flag to show decal box

layout(location=0) out vec4 outColor;

// Linear blending with weight
vec3 BlendLinear(vec4 n1, vec4 n2, float weight)
{
    vec3 r = mix(n1.xyz, n2.xyz, weight);
    return normalize(r);
}

// Convert a normal vector in tangent space to world space
// normalWS - tangent space normal basis in world space
// tangentWS - tangent space tangent basis in world space
// normalTS - tangent space normal vector to transform
vec4 TS2WS(vec4 normalWS, vec4 tangentWS, vec4 normalTS)
{
    // Tangent space to world space
    mat3x3 ts2ws;

    // Get the tangent basis vectors in world space
    ts2ws[0] = tangentWS.xyz;
    ts2ws[1] = normalize(cross(normalWS.xyz, tangentWS.xyz));
    ts2ws[2] = normalWS.xyz;
    
    // Convert the normal vector from tangent space to world space
    vec3 outNormalWS = normalize(ts2ws * normalTS.xyz);
    return vec4(outNormalWS.xyz, 0);
}

void main()
{
    vec2 screenCoord = gl_FragCoord.xy / uScreenSize.xy;
    vec4 worldPos = texture2D(uWorldPosTex, screenCoord);

    // Convert world position to model local position
    vec4 localPos = uInvModelMatrix * worldPos;
    localPos /= localPos.w;

    // inBox = abs(localPos) < 1.0;
    bvec3 inBox = lessThan(abs(localPos.xyz), vec3(1.0f));
    vec2 uv = localPos.xy * 0.5f + 0.5f;
    vec4 diffuse = vec4(0, 0, 0, 1);

    if (uShowDecalBox) {
        diffuse = vec4(0.1f, 0, 0, 1);
    }

    // If this pixel is outside of the decal box, then discard this pixel
    if (!all(inBox)) {
        // Except for decal box visualization option
        if (!uShowDecalBox) {
            discard;
        }
    }

#if USE_PIXEL_SHADER_INTERLOCK
    // Begin critical section here to guarantee that other shader invocation 
    // cannot overwrite the normal image.
    beginInvocationInterlockNV();
#endif

    // Read normal from the normal image
    vec4 destNormalWS = normalize(imageLoad(uNormalImage, ivec2(gl_FragCoord.xy)).rgba);
    vec4 decalNormalTS = normalize(textureLod(uDecalNormalTex, uv, 0.0) * 2 - 1);
    vec4 newNormalWS = TS2WS(destNormalWS, vec4(1, 0, 0, 1), decalNormalTS);

    // Blend normal vectors
    destNormalWS.xyz = BlendLinear(newNormalWS, destNormalWS, uBlendWeight);

    // Write new blended normal into normal image
    imageStore(uNormalImage, ivec2(gl_FragCoord.xy), destNormalWS);

#if USE_PIXEL_SHADER_INTERLOCK
    // Done with the normal image writing
    endInvocationInterlockNV();
#endif

    outColor.rgba = diffuse;
}
