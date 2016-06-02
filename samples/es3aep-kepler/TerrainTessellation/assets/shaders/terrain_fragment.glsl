//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation\assets\shaders/terrain_fragment.glsl
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
// Version tag will be added in code

#UNIFORMS

layout(location=1) in block {
    vec3 vertex;
    vec3 vertexEye;
    vec3 normal;
} In;

layout(location=0) out vec4 fragColor;

float saturate(float v) {
    return clamp( v, 0.0, 1.0);
}

// cheaper than smoothstep
float linearstep(float a, float b, float x)
{
    return saturate((x - a) / (b - a));
}

#define smoothstep linearstep

const vec3 sunColor = vec3(1.0, 1.0, 0.7);
const vec3 lightColor = vec3(1.0, 1.0, 0.7)*1.5;
const vec3 fogColor = vec3(0.7, 0.8, 1.0)*0.7;

const float fogExp = 0.1;

vec3 applyFog(vec3 col, float dist)
{
    float fogAmount = exp(-dist*fogExp);
    return mix(fogColor, col, fogAmount);
}

// fog with scattering effect
// http://www.iquilezles.org/www/articles/fog/fog.htm
vec3 applyFog(vec3 col, float dist, vec3 viewDir)
{
    float fogAmount = exp(-dist*fogExp);
    float sunAmount = max(dot(viewDir, lightDirWorld), 0.0);
    sunAmount = pow(sunAmount, 32.0);
    vec3 fogCol = mix(fogColor, sunColor, sunAmount);
    return mix(fogCol, col, fogAmount);
}

vec3 shadeTerrain(vec3 vertex,
                  vec3 vertexEye, 
                  vec3 normal
                  )
{
    const float shininess = 100.0;
    const vec3 ambientColor = vec3(0.05, 0.05, 0.15 );
    const float wrap = 0.3;

    vec3 rockColor = vec3(0.4, 0.4, 0.4 );
    vec3 snowColor = vec3(0.9, 0.9, 1.0 );
    vec3 grassColor = vec3(77.0 / 255.0, 100.0 / 255.0, 42.0 / 255.0 );
    vec3 brownColor = vec3(82.0 / 255.0, 70.0 / 255.0, 30.0 / 255.0 );
    vec3 waterColor = vec3(0.2, 0.4, 0.5 );
    vec3 treeColor = vec3(0.0, 0.2, 0.0 );

    //vec3 noisePos = vertex.xyz + vec3(translate.x, 0.0, translate.y);
    vec3 noisePos = vertex.xyz;
    float nois = noise(noisePos.xz)*0.5+0.5;

    float height = vertex.y;

    // snow
    float snowLine = 0.7;
    //float snowLine = 0.6 + nois*0.1;
    float isSnow = smoothstep(snowLine, snowLine+0.1, height * (0.5+0.5*normal.y));

    // lighting

    // world-space
    vec3 viewDir = normalize(eyePosWorld.xyz - vertex);
    vec3 h = normalize(-lightDirWorld + viewDir);
    vec3 n = normalize(normal);

    //float diffuse = saturate( dot(n, -lightDir));
    float diffuse = saturate( (dot(n, -lightDirWorld) + wrap) / (1.0 + wrap));   // wrap
    //float diffuse = dot(n, -lightDir)*0.5+0.5;
    float specular = pow( saturate(dot(h, n)), shininess);

#if 0
    // add some noise variation to colors
    grassColor = mix(grassColor*0.5, grassColor*1.5, nois);
    brownColor = mix(brownColor*0.25, brownColor*1.5, nois);
#endif

    // choose material color based on height and normal

    vec3 matColor;
    matColor = mix(rockColor, grassColor, smoothstep(0.6, 0.8, normal.y));
    matColor = mix(matColor, brownColor, smoothstep(0.9, 1.0, normal.y ));
    // snow
    matColor = mix(matColor, snowColor, isSnow);

    float isWater = smoothstep(0.05, 0.0, height);
    matColor = mix(matColor, waterColor, isWater);

    vec3 finalColor = ambientColor*matColor + diffuse*matColor*lightColor + specular*lightColor*isWater;

    // fog
    float dist = length(vertexEye);
    //finalColor = applyFog(finalColor, dist);
    finalColor = applyFog(finalColor, dist, viewDir);
        
    return finalColor;

    //return vec3(dist);
    //return vec3(dnoise(vertex.xz).z*0.5+0.5);
    //return normal*0.5+0.5;
    //return vec3(normal.y);
    //return vec3(fogCoord);
    //return vec3(diffuse);
    //return vec3(specular);
    //return diffuse*matColor + specular.xxx
    //return matColor;
    //return vec3(occ);
    //return vec3(sun)*sunColor;
    //return noise2*0.5+0.5;
}

void main()
{
    fragColor = vec4(shadeTerrain(In.vertex.xyz, In.vertexEye.xyz, In.normal), 1.0);    // shade per pixel
}
