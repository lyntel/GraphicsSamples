//----------------------------------------------------------------------------------
// File:        nvpr\ShapedTextES/PathSampleApp.cpp
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
#include "PathSampleApp.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <vector>

#include <Cg/dot.hpp>
#include <Cg/inverse.hpp>
#include <Cg/length.hpp>
#include <Cg/mul.hpp>
#include <Cg/exp2.hpp>
#include <Cg/iostream.hpp>

using namespace Cg;
using std::vector;

#include "countof.h"
#include "cg4cpp_xform.hpp"

#ifndef GLAPIENTRYP
# ifdef _WIN32
#  define GLAPIENTRYP __stdcall *
# else
#  define GLAPIENTRYP *
# endif
#endif

PathSampleApp::PathSampleApp()
    : dragging(false)
    , pinching(false)
    , view(float3x3(1,0,0, 0,1,0, 0,0,1))
    , window_center(0.0f)
    , gamepad_anchor(0.0f)
    , anchor_set(false)
    , zooming(false)
    , rotating(false)
    , zoom_vel(0.0f)
    , rot_vel(0.0f)
    , trans_vel(0.0f)
    , canvas_width(800)
    , render_state(NvRedrawMode::UNBOUNDED)
    , es_context(true)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
    continuousRendering(false);	
}

PathSampleApp::~PathSampleApp()
{
    LOGI("PathSampleApp: destroyed\n");
}

void PathSampleApp::continuousRendering(bool keep_rendering)
{
	if (isTestMode())
		keep_rendering = true;

    // XXX ugh without render_state variable, calling setRedrawMode "too much" casues DVM crashes
    if (keep_rendering) {
        if (render_state != NvRedrawMode::UNBOUNDED) {
			if (getPlatformContext()) {
				getPlatformContext()->setRedrawMode(NvRedrawMode::UNBOUNDED);
				render_state = NvRedrawMode::UNBOUNDED;
			}
        }
    } else {
		if (render_state != NvRedrawMode::ON_DEMAND) {
			if (getPlatformContext()) {
				getPlatformContext()->setRedrawMode(NvRedrawMode::ON_DEMAND);
				render_state = NvRedrawMode::ON_DEMAND;
			}
		}
    }
}

void PathSampleApp::postRedisplay()
{
    getPlatformContext()->requestRedraw();	
}

void PathSampleApp::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 8; 
    config.msaaSamples = 8;
    if (es_context) {
      config.apiVer = NvGLAPIVersionES3_1();
    } else {
      config.apiVer = NvGLAPIVersionGL4();
    }
}

void PathSampleApp::configureProjection()
{
    float3x3 iproj, viewport;

    viewport = ortho(0,window_width, 0,window_height);
    float left = 0, right = canvas_width, top = 0, bottom = canvas_width;
    if (aspect_ratio > 1) {
        top *= aspect_ratio;
        bottom *= aspect_ratio;
    } else {
        left /= aspect_ratio;
        right /= aspect_ratio;
    }

    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixOrthoEXT(GL_PROJECTION, left, right, bottom, top,
      -1, 1);

    iproj = inverse_ortho(left, right, top, bottom);
    win2obj = mul(iproj, viewport);

    window_center = float2(window_width/2, window_height/2);
}

void PathSampleApp::reshape(int32_t width, int32_t height)
{
    glViewport(0,0,width,height);

    window_width = float(width);
    window_height = float(height);
    aspect_ratio = window_height/window_width;

    configureProjection();

    CHECK_GL_ERROR();
}

void PathSampleApp::initRendering(void) {
    if (!requireExtension("GL_NV_path_rendering")) return;
    LOGI("Has path rendering!");

    CHECK_GL_ERROR();
}


// Call this from your app's draw method.
void PathSampleApp::updateView()
{
    float deltaT = getFrameDeltaTime();
    LOGI("draw %g\n", deltaT);
    if (anchor_set) {
        view = mul(translate(-gamepad_anchor), view);  // move origin to anchor
        const float zoom = exp2(zoom_vel*deltaT);
        view = mul(scale(zoom), view);  // preconcatenate scale
        view = mul(rotateRadians(rot_vel*deltaT), view);  // preconcatenate rotate
        view = mul(translate(gamepad_anchor), view);  // revert origin from anchor
    }

    if (trans_vel.x != 0.0f || trans_vel.y != 0.0f) {
        view = mul(translate(trans_vel.x*deltaT, trans_vel.y*deltaT), view);  // preconcatenate translate
    }
}

bool PathSampleApp::downLeft(NvPointerEvent* p)
{
    rotate_x = p->m_x;
    scale_y = p->m_y;

    last_t = float2(p->m_x, p->m_y);
    last_t_obj = mul(win2obj, float3(last_t, 1.0f));
    zooming = true;
    rotating = true;
    return true;
}

bool PathSampleApp::upLeft(NvPointerEvent* p)
{
    zooming = false;
    rotating = false;

    dragging = false;
    pinching = false;
    return true;
}

bool PathSampleApp::down(NvPointerEvent* p)
{
    last_t = float2(p->m_x, p->m_y);
    last_t_obj = mul(win2obj, float3(last_t, 1.0f));
    dragging = true;
    return true;
}

bool PathSampleApp::up(NvPointerEvent* p)
{
    zooming = false;
    rotating = false;

    dragging = false;
    pinching = false;
    return true;
}

float2 clipToSurfaceScales(float w, float h, float scene_ratio)
{
    float window_ratio = h/w;
    if (scene_ratio < 1.0) {
        if (scene_ratio < window_ratio) {
            return float2(1,1.0/window_ratio);
        } else {
            return float2(window_ratio/scene_ratio,1.0/scene_ratio);
        }
    } else {
        if (scene_ratio < window_ratio) {
            return float2(scene_ratio,scene_ratio/window_ratio);
        } else {
            return float2(window_ratio,1);
        }
    }
}

bool PathSampleApp::motion(NvPointerEvent* p)
{
    if (zooming || rotating) {
        float x = p->m_x;
        float y = p->m_y;

        float angle = 0;
        float zoom = 1;
        if (rotating) {
            angle = 0.003f * (rotate_x - x);
        }
        if (zooming) {
            zoom = powf(2, (y - scale_y) * 2.0f/window_height);
        }
        float xx = last_t.x,
              yy = last_t.y;

        float3x3 t, r, s, m;
        t = translate(xx, yy);
        r = rotateRadians(angle);
        s = scale(zoom);
        r = mul(r,s);
        m = mul(t,r);
        t = translate(-xx, -yy);
        m = mul(m,t);
        view = mul(m,view);
        rotate_x = x;
        scale_y = y;
        postRedisplay();	
    } else
    if (dragging) {
        float2 cur_t = float2(p->m_x, p->m_y);
        float3 cur_t_obj = mul(win2obj, float3(cur_t, 1.0f));
        float2 offset = cur_t_obj.xy - last_t_obj.xy;
        view = mul(translate(offset.x, offset.y), view);  // preconcatenate translate
        last_t = cur_t;
        last_t_obj = cur_t_obj;
        postRedisplay();	
        return true;
    }
    return false;
}

bool PathSampleApp::pinchAndZoom(float2 touch1_pt1, float2 touch2_pt1, float2 touch1_pt2, float2 touch2_pt2)
{
    const float2 unskew = float2(window_width, window_height);
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
    postRedisplay();	
    return true;
}

bool PathSampleApp::handleGamepadChanged(uint32_t changedPadFlags)
{
    // For each controller...
    for (int32_t i = 0; i < NvGamepad::MAX_GAMEPADS; i++) {
        if (changedPadFlags & (1<<i)) {
            NvGamepad* pad = getPlatformContext()->getGamepad();
            if (pad) {
                NvGamepad::State state;
                bool got = pad->getState(i, state);
                if (got) {
                    if (state.mThumbRY == 0 && state.mThumbRX == 0) {
                        anchor_set = false;
                    } else {
                        if (!anchor_set) {
                            const float3 p = mul(win2obj, float3(window_center, 1.0f));
                            gamepad_anchor = p.xy / p[2]; // perspective divide
                            anchor_set = true;
                        }
                        const float zoom_rate = 2.4f;
                        zoom_vel = zoom_rate * state.mThumbRY;

                        const float rotation_rate = 3.13159f;  // maximum half revolution per section
                        rot_vel = rotation_rate*state.mThumbRX;
                    }

                    const float translation_rate = 900.0;
                    trans_vel = translation_rate*float2(state.mThumbLX, state.mThumbLY);

                    bool keep_rendering = false;
                    if (state.mThumbLX || state.mThumbLY || state.mThumbRX || state.mThumbRY) {
                        keep_rendering = true;
                    }
                    continuousRendering(keep_rendering);

                    postRedisplay();	
                    return false;  // allow further processing
                }
            }
        }
    }
    return false;  // allow further processing
}

bool PathSampleApp::handlePointerInput(NvInputDeviceType::Enum device,
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
                ate_it = downLeft(points);
            } else 
            if (points->m_id & NvMouseButton::MIDDLE) {
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
                ate_it = upLeft(points);
            } else 
            if (points->m_id & NvMouseButton::MIDDLE) {
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
