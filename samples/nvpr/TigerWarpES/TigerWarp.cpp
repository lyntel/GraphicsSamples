//----------------------------------------------------------------------------------
// File:        nvpr\TigerWarpES/TigerWarp.cpp
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

// TigerWarp.cpp - NV_path_rendering example of projective warping of path content

#include "TigerWarp.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NV/NvLogs.h"

// Cg for C++ headers to emulate Cg data types and standard library
#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/distance.hpp>
#include <Cg/mul.hpp>
#include <Cg/radians.hpp>
#include <Cg/transpose.hpp>
#include <Cg/stdlib.hpp>
#include <Cg/iostream.hpp>

using namespace Cg;  // put Cg for C++ in default namespace 

#include "cg4cpp_xform.hpp"

#include "tiger.h"

bool stroking = true,
     filling = true;

uint32_t path_count;
float canvas_width = 640.0, canvas_height = 480.0;
float4 box;
bool show_bounding_box = true;

float2 corners[4];
float2 moved_corners[4];
float3x3 model, view, inverse_view;

// Animation state
int animated_point = 0;
int last_time = 0;
float2 initial_point;
int flip = 1;

int close_corner = -1;  // What corner is being dragged around?  Negative one means none.

// Projection transform state
float window_width, window_height;
float view_width, view_height;
float3x3 win2view, view2win;

const GLchar *vertShader =
"attribute vec2 position;\n"
"void main() {\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"}\n";

const GLchar *fragShader =
"precision highp float;\n"
"uniform vec3 color;\n"
"void main() {\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";

void initGraphics()
{
    // Use an orthographic path-to-clip-space transform to map the
    // [0..640]x[0..480] range of the star's path coordinates to the [-1..1]
    // clip space cube:
    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixOrthoEXT(GL_PROJECTION, 0, canvas_width, canvas_height, 0, -1, 1);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);

    /* Before rendering to a window with a stencil buffer, clear the stencil
    buffer to zero and the color buffer to blue: */
    glClearStencil(0);
    glClearColor(0.1, 0.3, 0.6, 0.0);

    glPointSize(5.0);

    initTiger();

    int path_count = getTigerPathCount();
    NV_ASSERT(path_count >= 1);
    GLuint path_base = getTigerBasePath();
    GLfloat bounds[4];
    glGetPathParameterfvNV(path_base, GL_PATH_OBJECT_BOUNDING_BOX_NV, bounds);
    for (int i=1; i<path_count; i++) {
        GLfloat tmp[4];
        glGetPathParameterfvNV(path_base+i, GL_PATH_OBJECT_BOUNDING_BOX_NV, tmp);
        if (tmp[0] < bounds[0]) {
            bounds[0] = tmp[0];
        }
        if (tmp[1] < bounds[1]) {
            bounds[1] = tmp[1];
        }
        if (tmp[2] > bounds[2]) {
            bounds[2] = tmp[2];
        }
        if (tmp[3] > bounds[3]) {
            bounds[3] = tmp[3];
        }
    }
    box = float4(bounds[0], bounds[1], bounds[2], bounds[3]);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    glEnable(GL_LINE_STIPPLE);
    glLineStipple(3, 0x8888);
}

void TigerWarp::setModelMatrix()
{
    model = quad2quad(corners, moved_corners);
    getPlatformContext()->requestRedraw();	
}

void initCornersFromBox(const float4 box, float2 corners[4])
{
    corners[0] = box.xy;
    corners[1] = box.zy;
    corners[2] = box.zw;
    corners[3] = box.xw;
}

void TigerWarp::initModelAndViewMatrices()
{
    initCornersFromBox(box, corners);
    float4 moved_box = float4(canvas_width/2-0.95*canvas_height/2, 0.05*canvas_height,
           canvas_width/2+0.95*canvas_height/2, 0.95*canvas_height);
    initCornersFromBox(moved_box, moved_corners);
    initial_point = moved_corners[animated_point];
    model = quad2quad(corners, moved_corners);

    setModelMatrix();
    view = float3x3(1,0,0,
                    0,1,0,
                    0,0,1);
    inverse_view = inverse(view);
}

static float3x3 convert(const nv::matrix4f &m)
{
    return float3x3(m._11, m._21, m._41,
                    m._12, m._22, m._42,
                    m._14, m._24, m._44);
}

void TigerWarp::display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixPushEXT(GL_PROJECTION); {
        // Apply the pan/zoom transformer state to the projection matrix.
        const float middle_x = (box.z-box.x)/2;
        const float middle_y = (box.w-box.y)/2;

        const nv::matrix4f trans = m_transformer->getTranslationMat();
        const nv::matrix4f scale = m_transformer->getScaleMat();

        nv::matrix4f t, tinv;
        t.set_translate(nv::vec3f(middle_x + trans._41, middle_y - trans._42, 0));
        tinv.set_translate(nv::vec3f(-middle_x, -middle_y, 0));

        const nv::matrix4f m = t * scale * tinv;
        view = convert(m);
        inverse_view = inverse(view);

        glMatrixPushEXT(GL_MODELVIEW); {
            float3x3 mat = mul(view, model);
            MatrixLoadToGL(mat);
            glEnable(GL_STENCIL_TEST);
            drawTigerRange(filling, stroking, 0, path_count, es_context);
        } glMatrixPopEXT(GL_MODELVIEW);
        glDisable(GL_STENCIL_TEST);
        if (show_bounding_box) {
            glDisable(GL_STENCIL_TEST);
            if (es_context) {
              float vertices[8];
              int vert = 0;
              for (int i = 0; i<4; i++) {
                vertices[vert++] = (moved_corners[i].x / 320) - 1;
                vertices[vert++] = 1 - (moved_corners[i].y / 240);
              }

              glUseProgram(mLineProgram);
              glVertexAttribPointer(mPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, vertices);
              glEnableVertexAttribArray(mPositionLoc);
              glUniform3f(mColorLoc, 1.0, 1.0, 0.0);
              glDrawArrays(GL_LINE_LOOP, 0, 4);
              glUniform3f(mColorLoc, 1.0, 0.0, 0.0);
              glDrawArrays(GL_POINTS, 0, 4);
              glUseProgram(0);
              glDisableVertexAttribArray(mPositionLoc);
            } else {
                glMatrixPushEXT(GL_MODELVIEW); {
                  MatrixLoadToGL(view);
                  glColor3f(1,1,0);
                  glBegin(GL_LINE_LOOP); {
                      for (int i=0; i<4; i++) {
                          glVertex2f(moved_corners[i].x, moved_corners[i].y);
                      }
                  } glEnd();
                  glColor3f(1,0,1);
                  glBegin(GL_POINTS); {
                      for (int i=0; i<4; i++) {
                          glVertex2f(moved_corners[i].x, moved_corners[i].y);
                      }
                  } glEnd();
              } glMatrixPopEXT(GL_MODELVIEW);
          }
        }
    } glMatrixPopEXT(GL_PROJECTION);
}

void configureProjection()
{
    float3x3 iproj, viewport;

    viewport = ortho(0,window_width, 0,window_height);
    float left = 0, right = float(canvas_width), top = 0, bottom = float(canvas_height);
    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixOrthoEXT(GL_PROJECTION, 0, canvas_width, canvas_height, 0, -1, 1);
    iproj = inverse_ortho(left, right, top, bottom);
    view_width = right - left;
    view_height = bottom - top;
    win2view = mul(iproj, viewport);
    view2win = inverse(win2view);
}

void TigerWarp::idle(int now)
{
    int elapsed = now-last_time;
    float t = 0.0007f*(elapsed);

    // (x,y) of Figure-eight knot
    // http://en.wikipedia.org/wiki/Figure-eight_knot_%28mathematics%29
    float x = (2+::cos(2*t))*::cos(3*t) - 3;  // bias by -3 so t=0 is @ (0,0)
    float y = (2+::cos(2*t))*::sin(3*t);
    // Flip the sense of x & y to avoid a bias in a particular direction over time.
    if (flip & 2) {
        x = -x;
        y = -y;
    }
    float radius = 20;
    float2 xy = radius*float2(x,y);
    moved_corners[animated_point] = initial_point + xy;
    setModelMatrix();

    if (elapsed > 3500) {
        if (animated_point == 3) {
            flip++;
        }
        animated_point = (animated_point+1)%4;
        initial_point = moved_corners[animated_point];
        last_time = now;
    }
    getPlatformContext()->requestRedraw();
}

float2 project(float3 v)
{
    return v.xy/v.z;
}

bool TigerWarp::down(NvPointerEvent* p)
{
    const float2 win = float2(p->m_x, p->m_y);

    float closest = 50;
    close_corner = -1;
    float2 close_corner_location;
    for (int i=0; i<4; i++) {
        float3 p = float3(moved_corners[i], 1);
        p = mul(view, p);
        p = mul(view2win, p);
        float2 window_p = project(p);
        float distance_to_corner = distance(window_p, win);
        if (distance_to_corner < closest) {
            close_corner = i;
            closest = distance_to_corner;
            close_corner_location = window_p;
        }
    }
    if (close_corner < 0) {
        return false;
    } else {
        return true;
    }
}

bool TigerWarp::up(NvPointerEvent* p)
{
    if (close_corner < 0) {
        return false;
    } else {
        // Stop corner dragging
        close_corner = -1;
        return true;
    }
}

bool TigerWarp::motion(NvPointerEvent* p)
{
  if (close_corner >= 0) {
    float3 win = float3(p->m_x, p->m_y, 1);
    float3 p = mul(win2view, win);
    p = mul(inverse_view, p);
    float2 new_corner = project(p);
    // Are we animating and is the selected corner the animated corner?
    if (mAnimate && (close_corner == animated_point)) {
      // Yes, so just adjust the initial point.
      initial_point += (new_corner - moved_corners[close_corner]);
    } else {
      // No, so reposition the actual moved_corner.
      moved_corners[close_corner] = new_corner;
    }
    setModelMatrix();
    return true;
  }
  return false;
}

TigerWarp::TigerWarp() :
	  mAnimate(false)
    , es_context(true)
    , program(0)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
    timer = createStopWatch();  // start running
    timer->start();
}

TigerWarp::~TigerWarp()
{
    LOGI("TigerWarp: destroyed\n");
}

void TigerWarp::configurationCallback(NvGLConfiguration& config)
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

enum UIReactionIDs
{   
    REACT_RESET         = 0x10000001,
    REACT_ANIMATE,
};

void TigerWarp::initUI() {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        NvTweakVarBase *var;

        var = mTweakBar->addValue("Animate", mAnimate, REACT_ANIMATE);
        addTweakKeyBind(var, NvKey::K_SPACE);
        addTweakButtonBind(var, NvGamepad::BUTTON_Y);

        var = mTweakBar->addValue("Filling", filling);
        addTweakKeyBind(var, NvKey::K_F);
        addTweakButtonBind(var, NvGamepad::BUTTON_LEFT_SHOULDER);

        var = mTweakBar->addValue("Stroking", stroking);
        addTweakKeyBind(var, NvKey::K_S);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER);

        var = mTweakBar->addValue("Show bounds", show_bounding_box);
        addTweakKeyBind(var, NvKey::K_B);

        var = mTweakBar->addValue("Path count", path_count, 0, getTigerPathCount());
        addTweakKeyBind(var, NvKey::K_COMMA, NvKey::K_PERIOD);
        addTweakButtonBind(var, NvGamepad::BUTTON_DPAD_RIGHT, NvGamepad::BUTTON_DPAD_LEFT);

        var = mTweakBar->addButton("Reset", REACT_RESET);
        addTweakKeyBind(var, NvKey::K_R);
        addTweakButtonBind(var, NvGamepad::BUTTON_X);
    }
}

NvUIEventResponse TigerWarp::handleReaction(const NvUIReaction &react)
{
    switch (react.code) {
    case REACT_ANIMATE:
		if (!isTestMode())
		{
			if (mAnimate) {
				getPlatformContext()->setRedrawMode(NvRedrawMode::UNBOUNDED);
			}
			else {
				getPlatformContext()->setRedrawMode(NvRedrawMode::ON_DEMAND);
			}
		}
        break;
    case REACT_RESET:
        mTweakBar->resetValues();
        initModelAndViewMatrices();
        getPlatformContext()->requestRedraw();
        return nvuiEventHandled;
    }
    return nvuiEventNotHandled;
}

bool TigerWarp::handlePointerInput(NvInputDeviceType::Enum device,
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
            NV_ASSERT(count == 1);
            // Touch or stylus
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
            ate_it = up(points);
        }
        break;
    case NvPointerActionType::MOTION:
        // If corner being actively dragged...
        if (close_corner >= 0) {
            for (int32_t i=0; i<count; i++) {
                ate_it |= motion(&points[i]);
            }
        }
        break;
    }
    return ate_it;
}

void TigerWarp::initRendering()
{
	if (!isTestMode())
		getPlatformContext()->setRedrawMode(NvRedrawMode::ON_DEMAND);

	if (!requireExtension("GL_NV_path_rendering")) return;
    if(!es_context && !requireExtension("GL_EXT_direct_state_access")) return;

    initGraphics();
    initModelAndViewMatrices();
    path_count = getTigerPathCount();

    NvAssetLoaderAddSearchPath("nvpr/TigerWarpES");

    m_transformer->setMotionMode(NvCameraMotionType::PAN_ZOOM);
    m_transformer->setMaxTranslationVel(500); // seems to work decently.

    CHECK_GL_ERROR();

    const GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    const GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vshader, 1, &vertShader, NULL);
    glShaderSource(fshader, 1, &fragShader, NULL);
    glCompileShader(vshader);
    glCompileShader(fshader);
    
    mLineProgram = glCreateProgram();
    glAttachShader(mLineProgram, vshader);
    glAttachShader(mLineProgram, fshader);
    glLinkProgram(mLineProgram);
    glDetachShader(mLineProgram, vshader);
    glDetachShader(mLineProgram, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    mPositionLoc = glGetAttribLocation(mLineProgram, "position");
    mColorLoc = glGetUniformLocation(mLineProgram, "color");
    CHECK_GL_ERROR();
}

void TigerWarp::shutdownRendering()
{
    deinitTiger();
}

void TigerWarp::reshape(int32_t width, int32_t height)
{
    glViewport(0,0,width,height);
    window_width = float(width);
    window_height = float(height);

    configureProjection();

    CHECK_GL_ERROR();
}

void TigerWarp::draw()
{
    static double accum_time = 0;
    if (mAnimate) {
        accum_time += getFrameDeltaTime();
        int now = int(accum_time * 1000);
        idle(now);
    }
    display();
}

NvAppBase* NvAppFactory() {
    return new TigerWarp();
}
