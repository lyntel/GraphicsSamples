//----------------------------------------------------------------------------------
// File:        nvpr\ShapedTextES/PathSampleApp.h
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
#ifndef PathSampleApp_H
#define PathSampleApp_H

#include <vector>

#include "NvAppBase/gl/NvSampleAppGL.h"

class NvStopWatch;
class NvFramerateCounter;

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>

using namespace Cg;

class PathSampleApp : public NvSampleAppGL {
private:
    PathSampleApp& operator = (const PathSampleApp &rhs) { NV_ASSERT(!"no assignment supported"); return *this; }

public:
    PathSampleApp();
    ~PathSampleApp();

    void configureProjection();
    void reshape(int32_t width, int32_t height);
    void initRendering(void);

    bool handlePointerInput(NvInputDeviceType::Enum device,
                        NvPointerActionType::Enum action, 
                        uint32_t modifiers,
                        int32_t count,
                        NvPointerEvent* points,
                        int64_t timestamp);
    bool handleGamepadChanged(uint32_t changedPadFlags);

    void configurationCallback(NvGLConfiguration& config);

protected:
    bool es_context;
    float window_width, window_height, aspect_ratio;
    Cg::float3x3 win2obj;
    Cg::float2 window_center;
    Cg::float2 gamepad_anchor;
    bool anchor_set;  // whether gamepad rotate has been anchored

    bool zooming;
    bool rotating;

    float rotate_x;
    float scale_y;

    bool dragging;
    bool pinching;

    float3x3 view;
    float zoom_vel;
    float rot_vel;
    float2 trans_vel;

    float2 last_t1, last_t2, last_t;
    float3 last_t_obj;

    float canvas_width;

    // Pinch & zoom private methods
    bool downLeft(NvPointerEvent* p);
    bool upLeft(NvPointerEvent* p);
    bool down(NvPointerEvent* p);
    bool up(NvPointerEvent* p);
    bool motion(NvPointerEvent* p);
    bool pinchAndZoom(float2 touch1_pt1, float2 touch2_pt1, float2 touch1_pt2, float2 touch2_pt2);

    void updateView();

    void postRedisplay();
    NvRedrawMode::Enum render_state;
    void continuousRendering(bool);
};

#endif
