//----------------------------------------------------------------------------------
// File:        nvpr\CursiveES/Cursive.h
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
#ifndef Cursive_H
#define Cursive_H

#include "NvAppBase/gl/NvSampleAppGL.h"

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>

using namespace Cg;

class NvStopWatch;
class NvFramerateCounter;

class Cursive : public NvSampleAppGL
{
private:
    const bool es_context;
    static const int canvas_width = 640, canvas_height = 480;
    GLuint program;
    GLuint crazy_path;  // path object name
    GLfloat percent;    // initially 100%
    GLfloat last_percent;

    float window_width, window_height, aspect_ratio;
    float3x3 win2obj;

    bool dragging;
    bool pinching;
    bool animating;

    float3x3 model, view;

    float2 last_t1, last_t2, last_t;
    float3 last_t_obj;

    GLfloat total_length;

    Cursive& operator = (const Cursive &rhs) { NV_ASSERT(!"no assignment"); return *this; }

public:
    Cursive();
    ~Cursive();

    void setLength(float percent);
    void idle();
    
    void initUI();
    NvUIEventResponse handleReaction(const NvUIReaction &react);
    void initRendering();
    void shutdownRendering();
    void initProgram();
    void initPath();
    void drawPath();
    void draw();
    void configureProjection();
    void reshape(int32_t width, int32_t height);

    void initModelAndViewMatrices();

    void configurationCallback(NvGLConfiguration& config);

    bool down(NvPointerEvent* p);
    bool up(NvPointerEvent* p);
    bool motion(NvPointerEvent* p);

    bool pinchAndZoom(float2 touch1_pt1, float2 touch2_pt1, float2 touch1_pt2, float2 touch2_pt2);

    bool handlePointerInput(NvInputDeviceType::Enum device,
                        NvPointerActionType::Enum action, 
                        uint32_t modifiers,
                        int32_t count,
                        NvPointerEvent* points,
                        int64_t timestamp);
};

#endif
