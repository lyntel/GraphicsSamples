//----------------------------------------------------------------------------------
// File:        es3aep-kepler\FeedbackParticlesApp\assets/emitter_feedback.geom
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

#define T_EMITTER 1
#define T_NORMAL  2

layout(points)             in;
layout(points)             out;
layout(max_vertices = 120) out;

in int            vs_Type[];
in vec3           vs_Color[];
in float          vs_Age[];
in vec3           vs_Position[];
in vec3           vs_Velocity[];

out uint          gs_TypeColor;
out float         gs_Age;
out vec3          gs_Position;
out vec3          gs_Velocity;

uniform float     u_Time;
uniform float     u_DeltaTime;
uniform float     u_EmitPeriod;
uniform float     u_EmitCount;
uniform sampler2D u_RandomTexture;


//------------------------------------------------------------------------------
//misc stuff
vec3 getRandomVector(float a)
{
    return texture(u_RandomTexture,vec2(a,0.5f)).xyz - vec3(0.5f);
}

float getRandom(float a)
{
    return texture(u_RandomTexture,vec2(a,0.5f)).x;
}

uint packColor(vec3 c)
{
    uvec3 i = uvec3(c);
    return i.r|(i.g<<8)|(i.b<<16);
}

uint packTypeColor(float type,vec3 color)
{
    return (packColor(color)<<8)|uint(type);
}


//------------------------------------------------------------------------------
//
void emitParticles(int num)
{
    float seed = (u_Time*123525.0 + gl_PrimitiveIDIn*1111.f)/1234.f;
    for (int i = 0; i < num; i++) 
    {
        vec3  dir   = normalize(getRandomVector(seed));

        gs_TypeColor= packTypeColor(T_NORMAL,vs_Color[0]);
        gs_Velocity = dir/(10.0 + getRandom(seed)*20.0)*0.3f;
        gs_Position = vs_Position[0] + gs_Velocity*2.77;
        gs_Age      = 0;

        EmitVertex();
        EndPrimitive();

        seed += 4207.56;
    }
}


void main()
{
    if (vs_Age[0] >= u_EmitPeriod)
        emitParticles(int(u_EmitCount));
}

