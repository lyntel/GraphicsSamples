//----------------------------------------------------------------------------------
// File:        es3aep-kepler\FeedbackParticlesApp\assets/billboard.geom
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

layout(points)           in;
layout(triangle_strip)   out;
layout(max_vertices = 4) out;

uniform mat4  u_MVP;
uniform mat4  u_MV;
uniform vec3  u_Right;
uniform vec3  u_Up;
uniform float u_BillboardSize;
uniform float u_VelocityScale;

in vec3       vs_Velocity[];
in vec4       vs_Tint[];
out vec2      gs_TexCoord;
out vec4      gs_Tint;


void simpleBillboard()
{
    vec3 pos     = gl_in[0].gl_Position.xyz;
    vec3 up      = u_Up*u_BillboardSize;
    vec3 right   = u_Right*u_BillboardSize;

    gs_Tint      = vs_Tint[0];

    pos         -= right;
    gl_Position  = u_MVP*vec4(pos, 1.0);
    gs_TexCoord  = vec2(0.0, 0.0);
    EmitVertex();

    pos         += up;
    gl_Position  = u_MVP*vec4(pos, 1.0);
    gs_TexCoord  = vec2(0.0, 1.0);
    EmitVertex();

    pos         -= up;
    pos         += right;
    gl_Position  = u_MVP*vec4(pos, 1.0);
    gs_TexCoord  = vec2(1.0, 0.0);
    EmitVertex();

    pos         += up;
    gl_Position  = u_MVP*vec4(pos, 1.0);
    gs_TexCoord  = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}


void velocityAwareBillboard()
{
    mat3 MV = mat3(u_MV);

    //screen-space velocity
    vec3 v      = MV*vs_Velocity[0];
    v.z         = 0.0;
    float len   = length(v);
    float scale = min(len*u_VelocityScale*10.0,10.0);
    vec3 v0     = v/len;

    //velocity basis from screen-space velocity vector
    vec3 v1     = vec3(-v0.y,v0.x,0);
    vec3 xaxis  = v0*MV;
    vec3 yaxis  = v1*MV;
    vec3 zaxis  = cross(xaxis,yaxis);
    mat3 basis  = mat3(xaxis*scale,yaxis,zaxis);

    float w = u_BillboardSize;
    float h = u_BillboardSize;
    vec3 pos = gl_in[0].gl_Position.xyz;
    gs_Tint = vs_Tint[0];

    //top right point
    vec3 topright = basis*vec3(w,h,0);
    gl_Position   = u_MVP*vec4(pos + topright,1); 
    gs_TexCoord   = vec2(0,0); 
    EmitVertex();

    //top left point
    vec3 topleft = basis*vec3(-w,h,0);
    gl_Position = u_MVP*vec4(pos + topleft,1); 
    gs_TexCoord = vec2(1,0); 
    EmitVertex();

    //bottom right point
    vec3 bottomleft = basis*vec3(w,-h,0);
    gl_Position = u_MVP*vec4(pos + bottomleft,1); 
    gs_TexCoord = vec2(0,1); 
    EmitVertex();

    //bottom left point
    vec3 bottomright = basis*vec3(-w,-h,0);
    gl_Position = u_MVP*vec4(pos + bottomright,1);
    gs_TexCoord = vec2(1,1);
    EmitVertex();

    EndPrimitive();
}

void main()
{
    if (u_VelocityScale > 0)
        velocityAwareBillboard();
    else
        simpleBillboard();
}
