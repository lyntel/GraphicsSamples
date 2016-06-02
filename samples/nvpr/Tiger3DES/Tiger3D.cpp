//----------------------------------------------------------------------------------
// File:        nvpr\Tiger3DES/Tiger3D.cpp
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

#include "Tiger3D.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/radians.hpp>

using namespace Cg;

#include "tiger.h"
#include "trackball.h"
#include "cg4cpp_xform.hpp"
#include "render_font.hpp"
#include "glut_teapot.h"
#include "teapot.h"

bool stroking = true,
     filling = true;

bool draw_overlaid_text = true;

bool using_dlist = 1;

bool enable_vsync = true;
float teapotSize = 200;
bool path_depth_offset = true;

float curquat[4];
// Initial slight rotation
float lastquat[4] = { 1.78721e-006, -0.00139029, 3.47222e-005, 0.999999 };
GLfloat m[4][4];
GLfloat mp[4][4];
bool spinning = false, moving = false;
float beginx, beginy;
bool newModel = true;
float ztrans = -150;
uint32_t numTigers = 4;
bool wireframe_teapot = true;
bool force_stencil_clear = true;
bool texture_tigers = false;
bool draw_center_teapot = true;
bool draw_tiger_proxy_texture = false;

typedef enum _Mode {
    FBO_TEXTURE,
    FBO_RENDERBUFFER,
} Mode;

Mode fbo_mode = FBO_RENDERBUFFER;
uint32_t fbo_quality = 42;  // 42 means render tigers with path rendering
bool valid_fbo = false;
int fbo_width = 512, fbo_height = 512;
int renderbuffer_samples = 8;
GLuint tex_color = 0;  // Texture object for pre-rendered tiger proxy
GLfloat tiger_bounds[4];  // (x1,y1,x2,y2) for lower-left and upper-right of tiger scene

float window_width, window_height;

int emScale = 2048;

FontFace *font;
Message *msg;

enum ClearColor {
    CC_BLUE,
    CC_BLACK
} clear_color = CC_BLUE;

void clearToBlue()
{
    glClearColor(0.1, 0.3, 0.6, 0.0);
    clear_color = CC_BLUE;
}

void clearToBlack()
{
    glClearColor(0, 0, 0, 0);
    clear_color = CC_BLACK;
}

void restoreClear()
{
    switch (clear_color) {
    case CC_BLACK:
        glClearColor(0,0,0,0);
        break;
    case CC_BLUE:
        glClearColor(0.1, 0.3, 0.6, 0.0);
        break;
    }
}

void initGraphics()
{
    trackball(curquat, 0.0, 0.0, 0.0, 0.0);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    glMatrixTranslatefEXT(GL_MODELVIEW, 0,0,ztrans);

    // Before rendering to a window with a stencil buffer, clear the stencil
    // buffer to zero and the color buffer to blue:
    glClearStencil(0);
    glClearColor(0.1, 0.3, 0.6, 0.0);

    initTiger();
    initTeapot();
    getTigerBounds(tiger_bounds, 1, 1);

    glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    glEnable(GL_DEPTH_TEST);
    GLfloat slope = -0.05;
    GLint bias = -1;
    glPathStencilDepthOffsetNV(slope, GLfloat(bias));
    glPathCoverDepthFuncNV(GL_ALWAYS);

    // Create a null path object to use as a parameter template for creating fonts.
    GLuint path_template = glGenPathsNV(1);
    glPathCommandsNV(path_template, 0, NULL, 0, GL_FLOAT, NULL);
    glPathParameterfNV(path_template, GL_PATH_STROKE_WIDTH_NV, 0.1f*emScale);  // 10% of emScale
    glPathParameteriNV(path_template, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);

    font = new FontFace(GL_SYSTEM_FONT_NAME_NV, "ParkAvenue BT", 256, path_template);
    float2 to_quad[4];
    to_quad[0] = float2(30,300);
    to_quad[1] = float2(610,230);
    to_quad[2] = float2(610,480);
    to_quad[3] = float2(30,480);
    msg = new Message(font, "Path rendering and 3D meet!", to_quad);
}

GLuint initFBO(Mode fbo_mode, int width, int height, int num_samples, GLuint program, GLuint mainFBO)
{
    GLuint fbo = 0, tex_depthstencil = 0;
    GLuint rb_color = 0, rb_depth_stencil = 0;
    GLint got_samples = -666;

    int tex_width = width, tex_height = height;

    if (tex_color == 0) {
        glGenTextures(1, &tex_color);
    }
    glBindTexture(GL_TEXTURE_2D, tex_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
        tex_width, tex_height, 0, GL_RGBA, GL_INT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    if (fbo_mode == FBO_RENDERBUFFER) {
        glGenRenderbuffers(1, &rb_color);
        glGenRenderbuffers(1, &rb_depth_stencil);

        glBindRenderbuffer(GL_RENDERBUFFER, rb_depth_stencil);
        if (num_samples > 1) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                num_samples, GL_DEPTH24_STENCIL8, 
                width, height);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER,
                GL_DEPTH24_STENCIL8, 
                width, height);
        }
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER, rb_depth_stencil);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
            GL_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER, rb_depth_stencil);

        glBindRenderbuffer(GL_RENDERBUFFER, rb_color);
        if (num_samples > 1) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                num_samples, GL_RGBA8,
                width, height);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER,
                GL_RGBA8,
                width, height);
        }
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_RENDERBUFFER, rb_color);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glGetIntegerv(GL_SAMPLES, &got_samples);
    } else {
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, tex_color, 0);

        // Setup depth_stencil texture (not mipmap)
        glGenTextures(1, &tex_depthstencil);
        glBindTexture(GL_TEXTURE_2D, tex_depthstencil);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
            width, height, 0, GL_DEPTH_STENCIL,
            GL_UNSIGNED_INT_24_8, NULL);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, tex_depthstencil, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
            GL_STENCIL_ATTACHMENT,
            GL_TEXTURE_2D, tex_depthstencil, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glViewport(0, 0, width, height);
    glClearColor(0,0,0,0);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);

    glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glMatrixOrthoEXT(GL_MODELVIEW,
        tiger_bounds[0], tiger_bounds[2],  // left,right
        tiger_bounds[1], tiger_bounds[3],  // bottom,top
        -1, 1);
    drawTiger(filling, stroking, program);

    if (fbo_mode == FBO_RENDERBUFFER && num_samples > 1) {
        GLuint fbo_texture = 0;
        glGenFramebuffers(1, &fbo_texture);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_texture);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, tex_color, 0);

        glBlitFramebuffer(0, 0, width, height,  // source (x,y,w,h)
            0, 0, tex_width, tex_height,  // destination (x,y,w,h)
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
        glDeleteFramebuffers(1, &fbo_texture);
    } 
	glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
    glBindTexture(GL_TEXTURE_2D, tex_color);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (tex_depthstencil) {
        glDeleteTextures(1, &tex_depthstencil);
    }
    if (rb_color) {
        glDeleteRenderbuffers(1, &rb_color);
    }
    if (rb_depth_stencil) {
        glDeleteRenderbuffers(1, &rb_depth_stencil);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
    glDeleteFramebuffers(1, &fbo);
    return tex_color;
}

void recalcModelView()
{
    build_rotmatrix(m, curquat);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    glMatrixTranslatefEXT(GL_MODELVIEW, 0,0,ztrans);
    newModel = 0;
}

void scene(GLuint program)
{
    float separation = 60;

    if (texture_tigers) {
        glBindTexture(GL_TEXTURE_2D, tex_color);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_STENCIL_TEST);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.25);
        for (uint32_t i=0; i<numTigers; i++) {
            float angle = i*360.0f/numTigers;
            glMatrixPushEXT(GL_MODELVIEW); {
                glRotatef(angle, 0,1,0);
                glTranslatef(0, 0, -separation);
                glScalef(0.3, 0.3, 1);
                // Draw a textured rectangle.
                glBegin(GL_TRIANGLE_FAN); {
                    glTexCoord2f(0,0);
                    glVertex2f(tiger_bounds[0],tiger_bounds[1]);
                    glTexCoord2f(1,0);
                    glVertex2f(tiger_bounds[2],tiger_bounds[1]);
                    glTexCoord2f(1,1);
                    glVertex2f(tiger_bounds[2],tiger_bounds[3]);
                    glTexCoord2f(0,1);
                    glVertex2f(tiger_bounds[0],tiger_bounds[3]);
                } glEnd();
            } glMatrixPopEXT(GL_MODELVIEW);
        }
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_TEXTURE_2D);
    } else {
        glEnable(GL_STENCIL_TEST);
        for (uint32_t i=0; i<numTigers; i++) {
            float angle = i*360.0f/numTigers;
            glMatrixPushEXT(GL_MODELVIEW); {
                glRotatef(angle, 0,1,0);
                glTranslatef(0, 0, -separation);
                glScalef(0.3, 0.3, 1);
                renderTiger(filling, stroking, program);
            } glMatrixPopEXT(GL_MODELVIEW);
        }
    }

    if (draw_center_teapot) {
        glDisable(GL_STENCIL_TEST);
        if (!program) {
          glColor3f(0,1,0);
        }
        glMatrixPushEXT(GL_MODELVIEW); {
            glScalef(1,-1,1);
            if (!program) {
                if (wireframe_teapot) {
                    glutWireTeapot(teapotSize);
                } else {
                    glEnable(GL_LIGHTING);
                    glEnable(GL_LIGHT0);
                    glutSolidTeapot(teapotSize);
                    glDisable(GL_LIGHTING);
                }
            } else {
                // apply z trans
                m[3][2] = ztrans;

                if (wireframe_teapot) {
                    drawTeapot(GL_LINE_STRIP, (GLfloat)teapotSize, m, mp);
                } else {
                    drawTeapot(GL_TRIANGLE_STRIP, (GLfloat)teapotSize, m, mp);
                }

                // revert z trans
                m[3][2] = 0.0f;
            }
        } glMatrixPopEXT(GL_MODELVIEW);
    }
}

// For debugging
void drawTigerProxyTexture()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    glBindTexture(GL_TEXTURE_2D, tex_color);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLE_FAN); {
        glColor3f(1,0,0);
        glTexCoord2f(0,1);
        glVertex2f(-1,-1);
        glColor3f(0,1,0);
        glTexCoord2f(1,1);
        glVertex2f(1,-1);
        glColor3f(0,0,1);
        glTexCoord2f(1,0);
        glVertex2f(1,1);
        glColor3f(1,1,0);
        glTexCoord2f(0,0);
        glVertex2f(-1,1);
    } glEnd();
}

void display(GLuint program)
{
    if (draw_tiger_proxy_texture) {
        // Debugging mode
        drawTigerProxyTexture();
        return;
    }

    if (force_stencil_clear) {
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        force_stencil_clear = false;
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    if (newModel) {
        recalcModelView();
    }
    glEnable(GL_DEPTH_TEST);
    glMatrixPushEXT(GL_MODELVIEW); {
        glMatrixMultfEXT(GL_MODELVIEW, &m[0][0]);
        scene(program);
    } glMatrixPopEXT(GL_MODELVIEW);

    if (draw_overlaid_text) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glMatrixPushEXT(GL_PROJECTION); {
            glMatrixLoadIdentityEXT(GL_PROJECTION);
            glMatrixOrthoEXT(GL_PROJECTION, 0, 640, 0, 480, -1, 1);

            glMatrixPushEXT(GL_MODELVIEW); {
                glMatrixLoadIdentityEXT(GL_MODELVIEW);
                msg->setStroking(stroking);
                msg->setFilling(filling);
                msg->setUnderline(0); // TODO: underline not working for ES
                msg->multMatrix();
                msg->render();
            } glMatrixPopEXT(GL_MODELVIEW);
        } glMatrixPopEXT(GL_PROJECTION);
    }
    glDisable(GL_STENCIL_TEST);
}

void Tiger3D::animate()
{
    if (spinning || moving) {
        add_quats(lastquat, curquat, curquat);
        newModel = 1;
        getPlatformContext()->requestRedraw();
    }
}

Tiger3D::Tiger3D()
    : mAnimate(true)
    , es_context(true)
    , program(0)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

Tiger3D::~Tiger3D() 
{
    LOGI("Tiger3D: destroyed\n");
}

void Tiger3D::configurationCallback(NvGLConfiguration& config)
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
    REACT_TOGGLE_STROKING         = 0x10000001,
    REACT_TOGGLE_FILLING          = 0x10000002,
    REACT_TIGER_QUALITY           = 0x10000003,
};


void Tiger3D::initUI() {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        NvTweakVarBase *var;
        var = mTweakBar->addValue("Animate", mAnimate);
        addTweakKeyBind(var, NvKey::K_SPACE);
        addTweakButtonBind(var, NvGamepad::BUTTON_Y);

        mTweakBar->addValue("Tigers", numTigers, 1, 9);
        mTweakBar->addValue("Draw teapot", draw_center_teapot);
        mTweakBar->addValue("Toggle display lists", using_dlist);

        var = mTweakBar->addValue("Filling", filling, REACT_TOGGLE_FILLING);
        addTweakKeyBind(var, NvKey::K_F);
        addTweakButtonBind(var, NvGamepad::BUTTON_LEFT_SHOULDER);

        var = mTweakBar->addValue("Stroking", stroking, REACT_TOGGLE_STROKING);
        addTweakKeyBind(var, NvKey::K_S);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER);

        mTweakBar->addValue("Wireframe teapot", wireframe_teapot);

        var = mTweakBar->addValue("Show text", draw_overlaid_text);
        addTweakKeyBind(var, NvKey::K_T);
        addTweakButtonBind(var, NvGamepad::BUTTON_X);

        mTweakBar->addValue("Teapot size", teapotSize, 100, 500);
        mTweakBar->addValue("Scene distance", ztrans, -200, -50);

        if (!es_context) {
            //TODO: Enable below once textured_tiger works on ES
            NvTweakEnum<uint32_t> TextureQuality[] = {
			    { "Path rendering!", 42 },
			    { "512x512x4 texture", 1 },
			    { "1024x1024x2 texture", 2 },
			    { "64x64x16 texture", 4 },
			    { "256x256x8 texture", 5 },
		    };
            mTweakBar->addEnum("Tiger quality", fbo_quality, TextureQuality, TWEAKENUM_ARRAYSIZE(TextureQuality), REACT_TIGER_QUALITY);
        }
    }
}

NvUIEventResponse Tiger3D::handleReaction(const NvUIReaction &react)
{
    switch (react.code) {
    case REACT_TOGGLE_FILLING:
        valid_fbo = false;
        getPlatformContext()->requestRedraw();
        return nvuiEventHandled;
    case REACT_TOGGLE_STROKING:
        valid_fbo = false;
        getPlatformContext()->requestRedraw();
        return nvuiEventHandled;
    case REACT_TIGER_QUALITY:
        valid_fbo = false;
        getPlatformContext()->requestRedraw();
        return nvuiEventHandled;
    }
    return nvuiEventNotHandled;
}

void Tiger3D::initProgram()
{
    const GLchar *source = "#extension GL_ARB_separate_shader_objects : enable\n"
      "precision highp float;\n"
      "uniform vec3 color;\n"
      "void main() {\n"
      "    gl_FragColor = vec4(color, 1.0);\n"
      "}\n";
    program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &source);
    CHECK_GL_ERROR();
}

void Tiger3D::initRendering()
{
	if (!isTestMode())
		getPlatformContext()->setRedrawMode(NvRedrawMode::ON_DEMAND);

	if (!requireExtension("GL_NV_path_rendering")) return;
    if (!es_context && !requireExtension("GL_EXT_direct_state_access")) return;

    initGraphics();
    msg->setEsContext(es_context);
    if (es_context) {
      initProgram();
    }
  //  initFBO(fbo_mode, fbo_width, fbo_height, renderbuffer_samples);
  //  restoreClear();

    NvAssetLoaderAddSearchPath("nvpr/Tiger3DES");

    CHECK_GL_ERROR();
}

void Tiger3D::shutdownRendering()
{
  if (es_context) {
    glDeleteProgram(program);
  }
  deinitTeapot();
}

void Tiger3D::reshape(int32_t w, int32_t h)
{
    glViewport(0,0, w,h);
    window_width = float(w);
    window_height = float(h);
    float aspect_ratio = window_width/window_height;
    glMatrixLoadIdentityEXT(GL_PROJECTION);
    float nearF = 1,
          farF = 1200;
    glMatrixFrustumEXT(GL_PROJECTION, -aspect_ratio,aspect_ratio, 1,-1, nearF,farF);

    mp[0][0] = nearF / aspect_ratio;
    mp[0][1] = 0.0f;
    mp[0][2] = 0.0f;
    mp[0][3] = 0.0f;
    mp[1][0] = 0.0f;
    mp[1][1] = -nearF;
    mp[1][2] = 0.0f;
    mp[1][3] = 0.0f;
    mp[2][0] = 0.0f;
    mp[2][1] = 0.0f;
    mp[2][2] = -(farF + nearF) / (farF - nearF);
    mp[2][3] = -1.0f;
    mp[3][0] = 0.0f;
    mp[3][1] = 0.0f;
    mp[3][2] = -2.0f * farF * nearF / (farF - nearF);
    mp[3][3] = 0.0f;
    force_stencil_clear = true;

    CHECK_GL_ERROR();
}

void Tiger3D::drawTigerProxyTexture(uint32_t item)
{
	GLuint mainFBO = getGLContext()->getMainFBO();
    texture_tigers = true;
    switch (item) {
    case 1:
		initFBO(FBO_RENDERBUFFER, 512, 512, 8, program, mainFBO);
        break;
    case 2:
		initFBO(FBO_RENDERBUFFER, 1024, 1024, 2, program, mainFBO);
        break;
    case 3:
		initFBO(FBO_TEXTURE, 64, 64, 1, program, mainFBO);
        break;
    case 4:
		initFBO(FBO_RENDERBUFFER, 64, 64, 16, program, mainFBO);
        break;
    case 5:
		initFBO(FBO_RENDERBUFFER, 256, 256, 16, program, mainFBO);
        break;
    case 42:  // Real path rendering for the win!
        texture_tigers = false;
        break;
    }
    reshape(int(window_width), int(window_height));
    restoreClear();
}

void Tiger3D::draw(void)
{
    if (!valid_fbo) {
        drawTigerProxyTexture(fbo_quality);
        valid_fbo = true;  // so draws just once
    }
    if (mAnimate || moving) {
        animate();
    }
    newModel = true;  // always regenerate the model matrix
    display(program);
    CHECK_GL_ERROR();
}

void stopSpinning()
{
    spinning = 0;
}

bool Tiger3D::down(NvPointerEvent* p)
{
    const float x = p->m_x,
                y = window_height - p->m_y;
    stopSpinning();
    moving = true;
    beginx = x;
    beginy = y;
    return true;
}

bool Tiger3D::up(NvPointerEvent* p)
{
    moving = false;
    return true;
}

bool Tiger3D::motion(NvPointerEvent* p)
{
    if (moving) {
        const float x = p->m_x,
                    y = window_height - p->m_y;
        trackball(lastquat,
            (2.0f * beginx - window_width) / window_width,
            (window_height - 2.0f * beginy) / window_height,
            (2.0f * x - window_width) / window_width,
            (window_height - 2.0f * y) / window_height
            );
        beginx = x;
        beginy = y;
        spinning = 1;
        newModel = 1;
        getPlatformContext()->requestRedraw();	
        return true;
    }
    return false;
}

bool Tiger3D::handlePointerInput(NvInputDeviceType::Enum device,
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
        // If corner being actively dragged...
        for (int32_t i=0; i<count; i++) {
            // If multi touch events, just follow the primary touch.
            if (device != NvInputDeviceType::MOUSE && points[i].m_id != 0) {
                return false;
            }
            ate_it |= motion(&points[i]);
        }
        break;
    }
    return ate_it;
}

NvAppBase* NvAppFactory() {
    return new Tiger3D();
}
