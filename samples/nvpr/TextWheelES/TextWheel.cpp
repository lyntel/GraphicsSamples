//----------------------------------------------------------------------------------
// File:        nvpr\TextWheelES/TextWheel.cpp
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

// TextWheel.cpp - spinning projected text on a wheel with NV_path_rendering

#include "TextWheel.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#include <string>
#include <vector>

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
using std::string;
using std::vector;

#include "sRGB_math.hpp"
#include "countof.h"
#include "cg4cpp_xform.hpp"
#include "render_font.hpp"

void TextWheel::setBackground()
{
    float r, g, b, a;

    switch (background) {
    default:
    case 0:
        r = g = b = 0.0;
        break;
    case 1:
        r = g = b = 1.0;
        break;
    case 2:
        r = 0.1;
        g = 0.3;
        b = 0.6;
        break;
    case 3:
        r = g = b = 0.5;
        break;
    }
    if (sRGB_capable) {
        r = convertSRGBColorComponentToLinearf(r);
        g = convertSRGBColorComponentToLinearf(g);
        b = convertSRGBColorComponentToLinearf(b);
    }
    a = 1.0;
    glClearColor(r,g,b,a);
}

static const char *phrases[] = {
    "Free spin",
    "Collect $100",
    "Bankrupt",
    "New car",
    "Spin again",
    "Lose your turn",
    "Take a chance",
    "Three wishes",
    "Lose $25",
    "Your lucky day",
    "Blue screen of death",
    "One million dollars",
    "Go directly to jail"
};

void TextWheel::initGraphics(int emScale)
{
    if (hasFramebufferSRGB) {
        glGetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &sRGB_capable);
    }

    setBackground();

    // Create a null path object to use as a parameter template for creating fonts.
    GLuint path_template = glGenPathsNV(1);
    glPathCommandsNV(path_template, 0, NULL, 0, GL_FLOAT, NULL);
    glPathParameterfNV(path_template, GL_PATH_STROKE_WIDTH_NV, 0.1f*emScale);  // 10% of emScale
    glPathParameteriNV(path_template, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);

    FontFace *font = new FontFace(GL_STANDARD_FONT_NAME_NV, "Sans", 256, path_template);

    int count = countof(phrases);
    float2 to_quad[4];
    float rad = radians(0.5f * 360.0f/count);
    float inner_radius = 4,
          outer_radius = 20;
    to_quad[0] = float2(::cos(-rad)*inner_radius, ::sin(-rad)*inner_radius);
    to_quad[1] = float2(::cos(-rad)*outer_radius, ::sin(-rad)*outer_radius);
    to_quad[2] = float2(::cos(rad)*outer_radius, ::sin(rad)*outer_radius);
    to_quad[3] = float2(::cos(rad)*inner_radius, ::sin(rad)*inner_radius);
    for (int i=0; i<count; i++) {
        msg_list.push_back(new Message(font, phrases[i], to_quad));
        msg_list.back()->setEsContext(es_context);
        msg_list.back()->setUnderline(underline);
    }

    glStencilFunc(GL_NOTEQUAL, 0, ~0U);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
}

void TextWheel::initViewMatrix()
{
    view = float3x3(1,0,0,
                    0,1,0,
                    0,0,1);
}

void TextWheel::configureProjection()
{
    float3x3 iproj, viewport;

    viewport = ortho(0,window_width, 0,window_height);
    float left = -20, right = 20, top = 20, bottom = -20;
    if (aspect_ratio > 1) {
        top *= aspect_ratio;
        bottom *= aspect_ratio;
    } else {
        left /= aspect_ratio;
        right /= aspect_ratio;
    }
    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
    //glOrtho(left, right, bottom, top,
    //  -1, 1);

    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixOrthoEXT(GL_PROJECTION, left, right, bottom, top,
      -1, 1);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    iproj = inverse_ortho(left, right, top, bottom);
    win2obj = mul(iproj, viewport);
}

void TextWheel::display()
{
    if (sRGB_capable) {
        glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CHECK_GL_ERROR();

    glEnable(GL_STENCIL_TEST); {
        const int count = countof(phrases);
        CHECK_GL_ERROR();
        for (int i=0; i<count; i++) {
            float deg = i*360.0f/count + angle;
            float3x3 rot = rotateDegrees(deg);
            float3x3 model = msg_list[i]->getMatrix();
            float3x3 mat = mul(view, mul(rot, model));
            MatrixLoadToGL(mat);
            msg_list[i]->render();
        }
        CHECK_GL_ERROR();
    } glDisable(GL_STENCIL_TEST);
    CHECK_GL_ERROR();
    if (sRGB_capable) {
        glDisable(GL_FRAMEBUFFER_SRGB_EXT);
    }
    CHECK_GL_ERROR();
}

void TextWheel::idle()
{
    angle += 0.5;
}

TextWheel::TextWheel()
    : mAnimate(true)
    , stroking(true)
    , filling(true)
    , dragging(false)
    , pinching(false)
    , underline(0)
    , use_sRGB(0)
    , hasFramebufferSRGB(false)
    , sRGB_capable(0)
    , angle(0)
    , animating(false)
    , background(2)
    , es_context(true)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();

    // Should be the opposite of stroking and filling.
    stroking_state = !stroking;
    filling_state = !filling;
}

TextWheel::~TextWheel()
{
    LOGI("TextWheel: destroyed\n");
}

void TextWheel::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 8; 
    config.msaaSamples = 8;
    if (es_context) {
      config.apiVer = NvGLAPIVersionES3_1();
    }
    else {
      config.apiVer = NvGLAPIVersionGL4();
    }
}

void TextWheel::initUI()
{
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        NvTweakVarBase *var;

        mTweakBar->addPadding(2);
        var = mTweakBar->addValue("Animate", mAnimate);
        addTweakKeyBind(var, NvKey::K_SPACE);
        addTweakButtonBind(var, NvGamepad::BUTTON_Y);

        mTweakBar->addPadding(2);
        var = mTweakBar->addValue("Filling", filling);
        addTweakKeyBind(var, NvKey::K_F);
        addTweakButtonBind(var, NvGamepad::BUTTON_LEFT_SHOULDER);

        mTweakBar->addPadding(2);
        var = mTweakBar->addValue("Stroking", stroking);
        addTweakKeyBind(var, NvKey::K_S);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER);
    }
}

void TextWheel::initRendering()
{
    if(!requireExtension("GL_NV_path_rendering")) return;
    if (!es_context && !requireExtension("GL_EXT_direct_state_access")) return;

    hasFramebufferSRGB = getGLContext()->isExtensionSupported("GL_EXT_framebuffer_sRGB");

    initViewMatrix();
    initGraphics(emScale);

    NvAssetLoaderAddSearchPath("nvpr/TextWheelES");

    CHECK_GL_ERROR();
}

void TextWheel::reshape(int32_t w, int32_t h)
{
    glViewport(0,0,w,h);
    window_width = float(w);
    window_height = float(h);
    aspect_ratio = window_height/window_width;

    configureProjection();

    CHECK_GL_ERROR();
}

void TextWheel::draw()
{
    if (mAnimate) {
        angle += 0.5;
    }
    if (filling != filling_state) {
        for (size_t i=0; i<msg_list.size(); i++) {
            msg_list[i]->setFilling(filling);
        }
        filling_state = filling;
    }
    if (stroking != stroking_state) {
        for (size_t i=0; i<msg_list.size(); i++) {
            msg_list[i]->setStroking(stroking);
        }
        stroking_state = stroking;
    }
    CHECK_GL_ERROR();
    display();
    CHECK_GL_ERROR();
}

bool TextWheel::down(NvPointerEvent* p)
{
    last_t = float2(p->m_x, p->m_y);
    last_t_obj = mul(win2obj, float3(last_t, 1.0f));
    dragging = true;
    return true;
}

bool TextWheel::up(NvPointerEvent* p)
{
    dragging = false;
    pinching = false;
    return true;
}

bool TextWheel::motion(NvPointerEvent* p)
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

bool TextWheel::pinchAndZoom(float2 touch1_pt1, float2 touch2_pt1, float2 touch1_pt2, float2 touch2_pt2)
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
    view = mul(rotateRadians(diff_angle), view);  // preconcatenate rotate
    
    const float2 offset = mul(win2obj, float3(touch2_pt2, 1)).xy - mul(view, anchor).xy;
    view = mul(translate(offset.x, offset.y), view);  // preconcatenate translate
    return true;
}

bool TextWheel::handlePointerInput(NvInputDeviceType::Enum device,
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

NvAppBase* NvAppFactory()
{
    return new TextWheel();
}
