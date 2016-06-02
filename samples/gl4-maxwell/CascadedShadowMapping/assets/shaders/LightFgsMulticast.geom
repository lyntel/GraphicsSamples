//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CascadedShadowMapping\assets\shaders/LightFgsMulticast.geom
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
#version 440 core

#extension GL_NV_viewport_array2 : require
#extension GL_NV_geometry_shader_passthrough : require

layout(triangles) in;

layout(passthrough) in gl_PerVertex{
    vec4 gl_Position;
};

layout(viewport_relative) out int gl_Layer;

uniform int frustumSegmentCount;

uniform vec4 viewports[4]; // max 4 frustum segments, viewport bounds in pixels, x, y, w, h
uniform vec2 shadowMapSize;

// converts clip coordinates into window coordinates for a given viewport
vec2 getWindowPos(vec4 clip_Pos, uint viewport) {
    vec2 ndc_Pos = (clip_Pos.xy / clip_Pos.w); // -1 to 1
    vec2 blend_factor = (ndc_Pos + 1.0) * 0.5; // 0 to 1
    vec2 view_Pos = (viewports[viewport].zw * blend_factor) + viewports[viewport].xy;
    return view_Pos;
}

// checks if two 2d bounding boxes intersect
bool checkIntersection(vec4 bbox0, vec4 bbox1) {
    bool xmiss = bbox0.x > bbox1.z || bbox0.z < bbox1.x;
    bool ymiss = bbox0.y > bbox1.w || bbox0.w < bbox1.y;
    return !xmiss && !ymiss;
}

void main(void) {
    const vec4 mapBounds = vec4(0, 0, shadowMapSize);
    int viewportMask = 0;
    for (int segment = 0; segment < frustumSegmentCount; ++segment) {
        vec2 start_Pos = getWindowPos(gl_in[0].gl_Position, segment);
        vec4 primBounds = vec4(start_Pos, start_Pos); // minx, miny, maxx, maxy
        for (int i = 1; i < gl_in.length(); ++i) {
            vec2 window_Pos = getWindowPos(gl_in[i].gl_Position, segment);
            primBounds.x = min(primBounds.x, window_Pos.x);
            primBounds.y = min(primBounds.y, window_Pos.y);
            primBounds.z = max(primBounds.x, window_Pos.x);
            primBounds.w = max(primBounds.y, window_Pos.y);
        }
        // we should only emit the primitive if its bounding box intersects the current viewport
        if (checkIntersection(primBounds, mapBounds)) {
            viewportMask |= (1 << segment);
        }
    }

    // this will set viewport mask and layer for the entire primitive
    // if viewport mask is zero, the primitive is culled
    gl_ViewportMask[0] = viewportMask;
    gl_Layer = 0;
}
