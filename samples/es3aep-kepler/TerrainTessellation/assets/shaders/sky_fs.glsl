//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation\assets\shaders/sky_fs.glsl
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
// sky shader with clouds - quite expensive, but looks pretty!

// Version tag will be added in code

#UNIFORMS

in block {
    vec4 pos;
    vec4 posEye;
} In;

layout(location=0) out vec4 fragColor;

const vec3 skyColor = vec3(0.7, 0.8, 1.0)*0.7;
const vec3 fogColor = vec3(0.8, 0.8, 1.0);
const vec3 cloudColor = vec3(1.0);
const vec3 sunColor = vec3(1.0, 1.0, 0.3);
const float skyHeight = 5.0;
const float skyTop = 6.0;
const int cloudSteps = 8;
const float cloudStepSize = 1.0;
const float cloudDensity = 0.25;

vec4 cloudMap(vec3 p)
{
    float d = turbulence(p*0.1 + vec3(time*0.05, -time*0.05, 0), 4, 2.0, 0.5);
    d = smoothstep(0.2, 0.5, d);    // threshold density

    float c = smoothstep(skyHeight, skyTop, p.y)*0.5+0.5;    // darken base
    return vec4(c, c, c, d*cloudDensity);
}

vec4 rayMarchClouds(vec3 ro, vec3 rd, float stepsize)
{
    vec4 sum = vec4(0);
    vec3 p = ro;
    vec3 step = rd * stepsize;

    // ray march front to back
    for(int i=0; i<cloudSteps; i++) {
        vec4 col = cloudMap(p);
        col.rgb *= col.a;               // pre-multiply alpha
        sum = sum + col*(1.0 - sum.a);
        p += step;
    }

    return sum;
}

float intersectPlane(vec3 n, float d, vec3 ro, vec3 rd)
{
    return (-d - dot(ro, n)) / dot(rd, n);
}

vec4 sky(vec3 ro, vec3 rd)
{
    // intersect ray with sky plane
    float t = intersectPlane(vec3(0.0, -1.0, 0.0), skyHeight, ro, rd);
    float tfar = intersectPlane(vec3(0.0, -1.0, 0.0), skyTop, ro, rd);
    
    float stepsize = (tfar - t) / float(cloudSteps);

    vec4 c = vec4(0.0);
    if (t > 0.0 && rd.y > 0.0) {
        vec3 hitPos = ro.xyz + t*rd;
        hitPos.xz += translate;
        c = rayMarchClouds(hitPos, rd, stepsize);
    }

    // fade with angle
    c *= smoothstep(0.0, 0.1, rd.y);

    // add sky
    vec3 sky = mix(skyColor, fogColor, pow(min(1.0, 1.0-rd.y), 10.0));

    // add sun under clouds
    float sun = pow( max(0.0, dot(rd, -lightDirWorld)), 500.0);
    sky += sunColor*sun;

    c.rgb = c.rgb + sky*(1.0-c.a);

    return c;
}

void main()
{
    // calculate ray in world space
    vec3 eyePos = eyePosWorld.xyz;
    vec3 viewDir = normalize(In.pos.xyz - eyePos);

    fragColor = sky(eyePos, viewDir);
}
