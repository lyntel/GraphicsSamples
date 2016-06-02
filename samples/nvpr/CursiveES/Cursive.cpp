//----------------------------------------------------------------------------------
// File:        nvpr\CursiveES/Cursive.cpp
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

// Cursive.cpp - NV_path_rendering example to "fake" cursive by animating the dash pattern

#include "Cursive.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/atan2.hpp>
#include <Cg/degrees.hpp>
#include <Cg/mul.hpp>
#include <Cg/radians.hpp>
#include <Cg/transpose.hpp>
#include <Cg/stdlib.hpp>
#include <Cg/iostream.hpp>

using namespace Cg;

#include "cg4cpp_xform.hpp"

Cursive::Cursive()
    : es_context(true)
    , program(0)
    , crazy_path(1)
    , percent(1.0f)  // initially 100%
    , dragging(false)
    , pinching(false)
    , animating(true)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();

    last_percent = percent + 0.1f;  // just to force a different value
}

Cursive::~Cursive()
{
    LOGI("Cursive: destroyed\n");
}

void Cursive::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 8; 
    config.msaaSamples = 4;
    if (es_context) {
      config.apiVer = NvGLAPIVersionES3_1();
    }
    else {
      config.apiVer = NvGLAPIVersionGL4();
    }
}

enum UIReactionIDs {   
    REACT_RESET         = 0x10000001,
};

void Cursive::initUI()
{
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        NvTweakVarBase *var;
        mTweakBar->addPadding(2);
        var = mTweakBar->addValue("Animate", animating);
        addTweakKeyBind(var, NvKey::K_SPACE);
        addTweakButtonBind(var, NvGamepad::BUTTON_Y);

        mTweakBar->addPadding(4);
        const float minimum_percent = 0.0f;
        const float maximum_percent = 1.0f;
        const float step = 0.005f;
        var = mTweakBar->addValue("Percent", percent, minimum_percent, maximum_percent, step);
        addTweakKeyBind(var, NvKey::K_COMMA, NvKey::K_PERIOD);
        addTweakButtonBind(var, NvGamepad::BUTTON_DPAD_RIGHT, NvGamepad::BUTTON_DPAD_LEFT);
        mTweakBar->addPadding(4);

        var = mTweakBar->addButton("Reset", REACT_RESET);
        addTweakKeyBind(var, NvKey::K_R);
        addTweakButtonBind(var, NvGamepad::BUTTON_X);
    }
}

NvUIEventResponse Cursive::handleReaction(const NvUIReaction &react)
{
    switch (react.code) {
    case REACT_RESET:
        initModelAndViewMatrices();
        percent = 1.0;
        return nvuiEventHandled;
    }
    return nvuiEventNotHandled;
}

void Cursive::initModelAndViewMatrices()
{
  float3x3 tmp;

  model = scale(0.75, 0.75);
  tmp = translate(-10, 120);  /* magic values to get text centered */
  model = mul(model, tmp);
  view  = translate(0, 0);
}

void Cursive::initProgram()
{
    if (es_context) {
        const GLchar *source = "#extension GL_ARB_separate_shader_objects : enable\n"
            "precision highp float;\n"
            "uniform vec3 color;\n"
            "void main() {\n"
            "    gl_FragColor = vec4(color, 1.0);\n"
            "}\n";
        program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &source);
        glUseProgram(program);
        CHECK_GL_ERROR();
    }
}

extern const char *crazy_svg;  // SVG string in this_is_crazy.cpp

void Cursive::initPath()
{
    GLint segments = 0;
    GLfloat stroke_width = 3.1f;

    glPathStringNV(crazy_path, GL_PATH_FORMAT_SVG_NV,
        (GLsizei)strlen(crazy_svg), crazy_svg);

    glGetPathParameterivNV(crazy_path, GL_PATH_COMMAND_COUNT_NV, &segments);
    LOGI("segments = %d\n", segments);
    total_length = glGetPathLengthNV(crazy_path, 0, segments);
    LOGI("totalLength = %f\n", total_length);
    glPathParameterfNV(crazy_path, GL_PATH_STROKE_WIDTH_NV, stroke_width);
    glPathParameterfNV(crazy_path, GL_PATH_END_CAPS_NV, GL_ROUND_NV);
    glPathParameterfNV(crazy_path, GL_PATH_DASH_CAPS_NV, GL_ROUND_NV);
    glPathParameterfNV(crazy_path, GL_PATH_DASH_CAPS_NV, GL_ROUND_NV);
}

void Cursive::setLength(float percent)
{
    GLfloat pattern[2];

    if (percent != last_percent) {
        pattern[0] = percent*total_length;
        pattern[1] = (1.01f-percent)*total_length;
        glPathDashArrayNV(crazy_path, 2, pattern);
        last_percent = percent;
    }
}

void Cursive::initRendering() {
	if (isTestMode())
		animating = false;

    if (!requireExtension("GL_NV_path_rendering")) return;
    LOGI("Has path rendering!");

    glClearStencil(0);
    glClearColor(0.1, 0.3, 0.6, 0.0);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    initModelAndViewMatrices();
    initProgram();
    initPath();

    NvAssetLoaderAddSearchPath("nvpr/CursiveES");

    CHECK_GL_ERROR();
}

void Cursive::shutdownRendering()
{
    glDeleteProgram(program);
}

void Cursive::drawPath()
{
    glEnable(GL_STENCIL_TEST); {
        const GLint reference = 0x1;
        const GLuint mask = 0xFF;
        glStencilStrokePathNV(crazy_path, reference, mask);
        // Yellow color
        if (es_context) {
            glUseProgram(program);
            glUniform3f(glGetUniformLocation(program, "color"), 1, 1, 0);
        } else {
            glColor3f(1,1,0);
        }
        glCoverStrokePathNV(crazy_path, GL_BOUNDING_BOX_NV);
        if (es_context) {
            glUseProgram(0);
        }
    } glDisable(GL_STENCIL_TEST);
}

void Cursive::configureProjection()
{
    float3x3 iproj, viewport;

    viewport = ortho(0,window_width, 0,window_height);
    float left = 0, right = float(canvas_width), top = 0, bottom = float(canvas_height);
    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixOrthoEXT(GL_PROJECTION, 0, canvas_width,
        canvas_height, 0,  // upper-left origin
        -1, 1);
    iproj = inverse_ortho(left, right, top, bottom);
    win2obj = mul(iproj, viewport);
}

void Cursive::reshape(int32_t w, int32_t h)
{
    glViewport(0,0,w,h);
    window_width = float(w);
    window_height = float(h);
    aspect_ratio = window_height/window_width;

    configureProjection();

    CHECK_GL_ERROR();
}

void Cursive::idle()
{
    if (animating) {
        percent += 0.0025;
        if (percent > 1.0) {
            percent = 0.0;
        }
    }
    setLength(percent);
}

void Cursive::draw()
{
    idle();

    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixPushEXT(GL_MODELVIEW); {
        float3x3 mat;

        mat = mul(view, model);
        MatrixLoadToGL(mat);
        drawPath();
    } glMatrixPopEXT(GL_MODELVIEW);
}

bool Cursive::down(NvPointerEvent* p)
{
    last_t = float2(p->m_x, p->m_y);
    last_t_obj = mul(win2obj, float3(last_t, 1.0f));
    dragging = true;
    return true;
}

bool Cursive::up(NvPointerEvent* p)
{
    dragging = false;
    pinching = false;
    return true;
}

bool Cursive::motion(NvPointerEvent* p)
{
    if (dragging) {
        float2 cur_t = float2(p->m_x, p->m_y);
        float3 cur_t_obj = mul(win2obj, float3(cur_t, 1.0f));
        float2 offset = cur_t_obj.xy - last_t_obj.xy;
        view = mul(translate(offset.x, offset.y), view);  // preconcatenate translate
        last_t = cur_t;
        last_t_obj = cur_t_obj;
        return true;
    }
    return false;
}

bool Cursive::pinchAndZoom(float2 touch1_pt1, float2 touch2_pt1, float2 touch1_pt2, float2 touch2_pt2)
{
    const float2 unskew = float2(window_width, -window_height);
    const float2 start = (touch2_pt1 - touch1_pt1) * unskew;
    const float2 end = (touch2_pt2 - touch1_pt2) * unskew;

    const float tiny_value = 1e-3;
    if (dot(start, start) < tiny_value || dot(end, end) < tiny_value) {
        return true;
    }

    const float3 anchor = mul(inverse(view), mul(win2obj, float3(touch2_pt1, 1.0f)));

    const float start_length = length(start);
    const float end_length = length(end);
    const float length_ratio = end_length / start_length;
    view = mul(scale(length_ratio), view);  // preconcatenate scale

    const float start_angle = atan2(start.y, start.x);
    const float end_angle = atan2(end.y, end.x);
    const float diff_angle = end_angle - start_angle;
    view = mul(rotateRadians(-diff_angle), view);  // preconcatenate rotate
    
    const float2 offset = mul(win2obj, float3(touch2_pt2, 1)).xy - mul(view, anchor).xy;
    view = mul(translate(offset.x, offset.y), view);  // preconcatenate translate
    return true;
}

bool Cursive::handlePointerInput(NvInputDeviceType::Enum device,
                        NvPointerActionType::Enum action, 
                        uint32_t modifiers,
                        int32_t count,
                        NvPointerEvent* points,
                        int64_t timestamp)
{
    bool ate_it = false;  // until proven otherwise
    switch (action) {
    case NvPointerActionType::DOWN:
        if (device == NvInputDeviceType::MOUSE) {
            NV_ASSERT(count == 1);
            if (points->m_id & NvMouseButton::LEFT) {
                ate_it = down(points);
            }
        } else {
            // Touch or stylus
            NV_ASSERT(count == 1);
            // If multi touch events, just follow the primary touch.
            if (points->m_id != 0) {
                return false;
            }
            ate_it = down(points);
        }
        break;
    case NvPointerActionType::UP:
        if (device == NvInputDeviceType::MOUSE) {
            NV_ASSERT(count == 1);
            if (points->m_id & NvMouseButton::LEFT) {
                ate_it = up(points);
            }
        } else {
            // Touch or stylus
            // If multi touch events, just follow the primary touch.
            if (points->m_id != 0) {
                return false;
            }
            ate_it = up(points);
        }
        break;
    case NvPointerActionType::MOTION:
        if (pinching) {
            NV_ASSERT(count >= 2);
            float2 current_t1 = float2(points[0].m_x, points[0].m_y);
            float2 current_t2 = float2(points[1].m_x, points[1].m_y);
            pinchAndZoom(last_t1, last_t2, current_t1, current_t2);
            last_t1 = current_t1;
            last_t2 = current_t2;
        } else {
            // If corner being actively dragged...
            for (int32_t i=0; i<count; i++) {
                // If multi touch events, just follow the primary touch.
                if (device != NvInputDeviceType::MOUSE && points[i].m_id != 0) {
                    return false;
                }
                ate_it |= motion(&points[i]);
            }
        }
        break;
    case NvPointerActionType::EXTRA_DOWN:
        pinching = true;
        NV_ASSERT(count >= 2);
        last_t1 = float2(points[0].m_x, points[0].m_y);
        last_t2 = float2(points[1].m_x, points[1].m_y);
        break;
    case NvPointerActionType::EXTRA_UP:
        pinching = false;
        dragging = false;
        break;
    }
    return ate_it;
}

NvAppBase* NvAppFactory() {
    return new Cursive();
}
