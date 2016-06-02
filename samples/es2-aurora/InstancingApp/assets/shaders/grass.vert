//----------------------------------------------------------------------------------
// File:        es2-aurora\InstancingApp\assets\shaders/grass.vert
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
uniform mat4 ModelViewMatrix;
uniform mat4 ProjectionMatrix;
uniform float g_fTime;

uniform vec3 InstanceOffsets[100];
uniform vec3 InstanceRotations[100];
uniform vec3 InstanceColors[ 6 ];

attribute vec4 vPosition;
attribute vec3 vTexCoord;

varying vec2 var_tex_coord;
varying vec3 var_color;

void main() 
{
    vec3 vOff = InstanceOffsets[ int(vTexCoord.z) ];
    vec3 vRot = InstanceRotations[ int(vTexCoord.z) ];
    vec4 vPos = vec4( dot( vPosition.xz,  vRot.xy ), 
                      vPosition.y,
                      dot( vPosition.xz,  vec2( vRot.y, -vRot.x ) ), vPosition.w );
    vec4 vPosEyeSpace = ModelViewMatrix * ( vec4( 0.025 * vPos.xyz + vOff, 1.0 ) );
    
    gl_Position = ProjectionMatrix * ( vPosEyeSpace + vec4( 0.0, 0.0, -5.0, 0.0 ) );
    
    var_tex_coord = vTexCoord.xy;
    var_color     = InstanceColors[ int(vRot.z) ];
}