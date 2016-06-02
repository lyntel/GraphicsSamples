//----------------------------------------------------------------------------------
// File:        es3aep-kepler\FeedbackParticlesApp\assets/feedback.vert
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

in float          a_Type;
in vec3           a_Color;
in float          a_Age;
in vec3           a_Position;
in vec3           a_Velocity;

out uint          vs_TypeColor;
out float         vs_Age;
out vec3          vs_Position;
out vec3          vs_Velocity;

uniform float     u_Time;
uniform float     u_DeltaTime;
uniform float     u_cTime;
uniform sampler2D u_RandomTexture;
uniform sampler3D u_FBMTexture;


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

vec3 getFbm(vec3 p)
{
    return texture(u_FBMTexture,(p + vec3(2))*0.25).xyz;
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
//Distant field stuff
float sphereDistance(vec3 p,vec3 c,float r)
{
    return length(p - c) - r;
}

float smin(float a,float b, float k)
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float opUnion(float d1,float d2)
{
    return smin(d1,d2,0.1);
}

float sampleDistance(vec3 p)
{
    float s1 = sphereDistance(p,vec3(sin(u_cTime)*0.3,0.0,cos(u_cTime)*0.3),0.3);
    float s2 = sphereDistance(p,vec3(sin(u_cTime/2)*0.2,-0.5,cos(u_cTime/2)*0.2),0.2);
    float s3 = sphereDistance(p,vec3(sin(u_cTime/2.5)*0.4,0.6,cos(u_cTime/2.5)*0.4),0.3);

    return opUnion(opUnion(s1,s2),s3);
}


//------------------------------------------------------------------------------
//Curl (divergence free) noise + distant field obstacles
//http://www.cs.ubc.ca/~rbridson/
//http://prideout.net/blog/?p=63
float smoothStep(float r)
{
    if(r < 0.0) return 0.0;
    else if(r > 1.0) return 1.0;

    return r*r*r*(10.0 + r*(-15.0 + r*6.0));
}

float ramp(float r)
{
    return smoothStep((r + 1.0)/2.0)*2.0 - 1.0;
}

vec3 computeGradient(vec3 p)
{
    const float e = 0.01;
    vec3 dx = vec3(e, 0, 0);
    vec3 dy = vec3(0, e, 0);
    vec3 dz = vec3(0, 0, e);
     
    float d =    sampleDistance(p);
    float dfdx = sampleDistance(p + dx) - d;
    float dfdy = sampleDistance(p + dy) - d;
    float dfdz = sampleDistance(p + dz) - d;

    return normalize(vec3(dfdx, dfdy, dfdz));
}

vec3 blendVectors(vec3 potential, float alpha, vec3 distanceGradient)
{
    float dp = dot(potential, distanceGradient);
    return alpha * potential + (1-alpha) * dp * distanceGradient;
}

vec3 samplePotential(vec3 p)
{
    vec3 gradient          = computeGradient(p);
    float obstacleDistance = sampleDistance(p);

    float d  = ramp(abs(obstacleDistance));
    vec3 psi = blendVectors(getFbm(p)*2.0,d,gradient);
    psi     += blendVectors(vec3(p.z,0,-p.x)*3.0,d,gradient);

    return psi/25.0;
}

vec3 curlOperator(vec3 pos)
{
    const float e = 0.0001;//1e-4;
    vec3 dx = vec3(e, 0, 0);
    vec3 dy = vec3(0, e, 0);
    vec3 dz = vec3(0, 0, e);

    float x = samplePotential(pos + dy).z - samplePotential(pos - dy).z 
            - samplePotential(pos + dz).y + samplePotential(pos - dz).y;
    float y = samplePotential(pos + dz).x - samplePotential(pos - dz).x
            - samplePotential(pos + dx).z + samplePotential(pos - dx).z;
    float z = samplePotential(pos + dx).y - samplePotential(pos - dx).y
            - samplePotential(pos + dy).x + samplePotential(pos - dy).x;

    return vec3(x, y, z) / (2*e);
}


//------------------------------------------------------------------------------
//
void main()
{
    float age = a_Age + u_DeltaTime;

    vec3 p   = a_Position;
    vec3 v   = curlOperator(p);
    vec3 mid = p + 0.5*u_DeltaTime*v;
    p       += u_DeltaTime*curlOperator(mid);

    vs_TypeColor= packTypeColor(T_NORMAL,a_Color);
    vs_Position = p;
    vs_Velocity = v;
    vs_Age      = age;
}
