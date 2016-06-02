//----------------------------------------------------------------------------------
// File:        es2-aurora\InstancedTessellation\assets\shaders/npatch_instanced.vert
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
#version 100

uniform mat4 g_m4x4ModelViewMatrix;
uniform mat4 g_m4x4ProjectionMatrix;
uniform mat4 g_m4x4NormalMatrix;

uniform vec4  g_v4LightPosition1;
uniform float g_fLightAmbient1;
uniform float g_fLightDiffuse1;

attribute vec3 a_v3Pos0;  // triangle vertices
attribute vec3 a_v3Norm0; // vertex normals
attribute vec2 a_v2Tex0;  // tex coords

attribute vec3 a_v3Pos1;  // triangle vertices
attribute vec3 a_v3Norm1; // vertex normals
attribute vec2 a_v2Tex1;  // tex coords

attribute vec3 a_v3Pos2;  // triangle vertices
attribute vec3 a_v3Norm2; // vertex normals
attribute vec2 a_v2Tex2;  // tex coords

attribute vec2 a_v2B01;

varying float v_fLightIntensity;

vec3 evaluateNPatch( vec3 v0,
                     vec3 v1,
                     vec3 v2,
                     vec3 n0,
                     vec3 n1,
                     vec3 n2,
                     vec3 b )
{
    vec3 b102 = ( 2.0 * v2 + v0 - dot( v0 - v2, n2 ) * n2 );
    vec3 b012 = ( 2.0 * v2 + v1 - dot( v1 - v2, n2 ) * n2 ); 
    vec3 b201 = ( 2.0 * v0 + v2 - dot( v2 - v0, n0 ) * n0 ); 
    vec3 b021 = ( 2.0 * v1 + v2 - dot( v2 - v1, n1 ) * n1 );
    vec3 b210 = ( 2.0 * v0 + v1 - dot( v1 - v0, n0 ) * n0 ); 
    vec3 b120 = ( 2.0 * v1 + v0 - dot( v0 - v1, n1 ) * n1 );
    
    return  b.x*b.x*b.x * v0 + 
            b.y*b.y*b.y * v1 + 
            b.z*b.z*b.z * v2 +
            b.x*b.z*b.z * b102 +
            b.y*b.z*b.z * b012 +
            b.x*b.x*b.z * b201 +
            b.y*b.y*b.z    * b021 +
            b.x*b.x*b.y * b210 +
            b.x*b.y*b.y * b120 +
            b.x*b.y*b.z * ( 0.5 * ( b210 + b120 + b021 + b012 + b102 + b201 ) - v0 - v1 - v2 );
}

void main()
{
    vec3 b            = vec3( a_v2B01.x, a_v2B01.y, 1.0 - a_v2B01.x - a_v2B01.y );
    vec4 vPosition    = vec4( evaluateNPatch( a_v3Pos0,  a_v3Pos1,  a_v3Pos2,
                                              a_v3Norm0, a_v3Norm1, a_v3Norm2, b ), 1.0 );
    vec4 vPosEyeSpace = g_m4x4ModelViewMatrix * vPosition;
    vec3 vNormal      = b.x * a_v3Norm0 + b.y * a_v3Norm1 + b.z * a_v3Norm2;
    
    gl_Position = g_m4x4ProjectionMatrix * vPosEyeSpace;
    
    vec3 normal = normalize(mat3(g_m4x4NormalMatrix) * vNormal);
    
    vec3 lDir;
    
    if(g_v4LightPosition1.w == 0.0)
        lDir = normalize(g_v4LightPosition1.xyz);
    else 
        lDir = normalize(g_v4LightPosition1.xyz - vPosEyeSpace.xyz);
    
    v_fLightIntensity = g_fLightAmbient1;
    
    float NdotL = max(dot(normal,lDir),0.0);
            
    // cosine (dot product) with the normal
    v_fLightIntensity += g_fLightDiffuse1 * NdotL;    
}
