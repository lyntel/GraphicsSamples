//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation\assets\shaders/terrain_control.glsl
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
#extension GL_ARB_tessellation_shader : enable

// control/hull shader
// executed once per input patch, computes LOD (level of detail) and performs culling

#UNIFORMS
#line 9

layout(vertices=1) out;

in gl_PerVertex {
    vec4 gl_Position;
} gl_in[];

layout(location=1) in block {
    mediump vec2 texCoord;
} In[];


out gl_PerVertex {
    vec4 gl_Position;
} gl_out[];

layout(location=1) out block {
    mediump vec2 texCoord;
    vec2 tessLevelInner;
} Out[];

//layout(location=1) out vec2 tessLevelInner[];

// test if sphere is entirely contained within frustum planes
bool sphereInFrustum(vec3 pos, float r, vec4 plane[6])
{
    for(int i=0; i<6; i++) {
        if (dot(vec4(pos, 1.0), plane[i]) + r < 0.0) {
            // sphere outside plane
            return false;
        }
    }
    return true;
}

// transform from world to screen coordinates
vec2 worldToScreen(vec3 p)
{
    vec4 r = ModelViewProjection * vec4(p, 1.0);   // to clip space
    r.xy /= r.w;            // project
    r.xy = r.xy*0.5 + 0.5;  // to NDC
    r.xy *= viewport.zw;    // to pixels
    return r.xy;
}

// calculate edge tessellation level from two edge vertices in screen space
float calcEdgeTessellation(vec2 s0, vec2 s1)
{
    float d = distance(s0, s1);
    return clamp(d / triSize, 1, 64);
}

vec2 eyeToScreen(vec4 p)
{
    vec4 r = Projection * p;   // to clip space
    r.xy /= r.w;            // project
    r.xy = r.xy*0.5 + 0.5;  // to NDC
    r.xy *= viewport.zw;    // to pixels
    return r.xy;
}

// calculate tessellation level by fitting sphere to edge
float calcEdgeTessellationSphere(vec3 w0, vec3 w1, float diameter)
{
    vec3 centre = (w0 + w1) * 0.5;
    vec4 view0 = ModelView * vec4(centre, 1.0);
    vec4 view1 = view0 + vec4(diameter, 0, 0, 0);
    vec2 s0 = eyeToScreen(view0);
    vec2 s1 = eyeToScreen(view1);
    return calcEdgeTessellation(s0, s1);
}

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    Out[gl_InvocationID].texCoord = In[gl_InvocationID].texCoord;

    // bounding sphere for patch
    vec3 spherePos = gl_in[gl_InvocationID].gl_Position.xyz + vec3(tileSize.x, heightScale, tileSize.z)*0.5;

    // test if patch is visible
    bool visible = sphereInFrustum(spherePos, tileBoundingSphereR, frustumPlanes);

    if (cull && !visible) {
        // cull patch
        gl_TessLevelOuter[0] = -1;
        gl_TessLevelOuter[1] = -1;
        gl_TessLevelOuter[2] = -1;
        gl_TessLevelOuter[3] = -1;

        gl_TessLevelInner[0] = -1;
        gl_TessLevelInner[1] = -1;
    } else {
        if (lod) {
            // compute automatic LOD

            // calculate edge tessellation levels
            // see tessellation diagram in OpenGL 4 specification for vertex order details
            vec3 v0 = gl_in[0].gl_Position.xyz;
            vec3 v1 = v0 + vec3(tileSize.x, 0, 0);
            vec3 v2 = v0 + vec3(tileSize.x, 0, tileSize.z);
            vec3 v3 = v0 + vec3(0, 0, tileSize.z);
#if 0
            // use screen-space length of each edge
            // this method underestimates tessellation level when looking along edge
            vec2 s0 = worldToScreen(v0);
            vec2 s1 = worldToScreen(v1);
            vec2 s2 = worldToScreen(v2);
            vec2 s3 = worldToScreen(v3);

            gl_TessLevelOuter[0] = calcEdgeTessellation(s3, s0);
            gl_TessLevelOuter[1] = calcEdgeTessellation(s0, s1);
            gl_TessLevelOuter[2] = calcEdgeTessellation(s1, s2);
            gl_TessLevelOuter[3] = calcEdgeTessellation(s2, s3);
#else
            // use screen space size of sphere fit to each edge
            float sphereD = tileSize.x*2.0f;
            gl_TessLevelOuter[0] = calcEdgeTessellationSphere(v3, v0, sphereD);
            gl_TessLevelOuter[1] = calcEdgeTessellationSphere(v0, v1, sphereD);
            gl_TessLevelOuter[2] = calcEdgeTessellationSphere(v1, v2, sphereD);
            gl_TessLevelOuter[3] = calcEdgeTessellationSphere(v2, v3, sphereD);
#endif

            // calc interior tessellation level - use average of outer levels
            gl_TessLevelInner[0] = 0.5 * (gl_TessLevelOuter[1] + gl_TessLevelOuter[3]);
            gl_TessLevelInner[1] = 0.5 * (gl_TessLevelOuter[0] + gl_TessLevelOuter[2]);

            Out[gl_InvocationID].tessLevelInner = vec2(gl_TessLevelInner[0], gl_TessLevelInner[1]);

        } else {
            gl_TessLevelOuter[0] = outerTessFactor;
            gl_TessLevelOuter[1] = outerTessFactor;
            gl_TessLevelOuter[2] = outerTessFactor;
            gl_TessLevelOuter[3] = outerTessFactor;

            gl_TessLevelInner[0] = innerTessFactor;
            gl_TessLevelInner[1] = innerTessFactor;

            Out[gl_InvocationID].tessLevelInner = vec2(innerTessFactor);
        }
    }
}