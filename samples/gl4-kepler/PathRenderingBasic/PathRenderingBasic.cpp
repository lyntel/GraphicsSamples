//----------------------------------------------------------------------------------
// File:        gl4-kepler\PathRenderingBasic/PathRenderingBasic.cpp
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
#include "PathRenderingBasic.h"

#include "NvUI/NvTweakBar.h"

#include "NvAssetLoader/NvAssetLoader.h"
#include "NV/NvLogs.h"
#include "NV/NvMath.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvGamepad/NvGamepad.h"
#include "NvAppBase/NvKeyboard.h"

using namespace nv;

PathRenderingBasic::PathRenderingBasic()
: mPathObj(0)
, mPathDefMode(0)
, mOptFill(true)
, mOptStroke(true)
, mOptEvenOdd(false)
, mOptDashLines(false)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

PathRenderingBasic::~PathRenderingBasic() {
    if (mPathObj!=0) {
        glDeletePathsNV(mPathObj, 1);
        mPathObj = 0;
    }
    LOGI("PathRenderingBasic: destroyed\n");
}

void PathRenderingBasic::configurationCallback(NvGLConfiguration& config) { 
    config.depthBits = 24; 
    config.stencilBits = 8; 
    config.msaaSamples = 4; 
    config.apiVer = NvGLAPIVersionGL4();
}


void PathRenderingBasic::initPathFromSVG() {
    /* Here is an example of specifying and then rendering a five-point
    star and a heart as a path using Scalable Vector Graphics (SVG)
    path description syntax: */
    
    const char *svgPathString =
      // star
      "M100,180 L40,10 L190,120 L10,120 L160,10 z"
      // heart
      "M300 300 C 100 400,100 200,300 100,600 200,500 400,300 300Z";
    glPathStringNV(mPathObj, GL_PATH_FORMAT_SVG_NV,
                   (GLsizei)strlen(svgPathString), svgPathString);
}


void PathRenderingBasic::initPathFromPS() {
    /* Alternatively applications oriented around the PostScript imaging
    model can use the PostScript user path syntax instead: */

    const char *psPathString =
      // star
      "100 180 moveto"
      " 40 10 lineto 190 120 lineto 10 120 lineto 160 10 lineto closepath"
      // heart
      " 300 300 moveto"
      " 100 400 100 200 300 100 curveto"
      " 500 200 500 400 300 300 curveto closepath";
    glPathStringNV(mPathObj, GL_PATH_FORMAT_PS_NV,
                   (GLsizei)strlen(psPathString), psPathString);
}


void PathRenderingBasic::initPathFromData() {
    /* The PostScript path syntax also supports compact and precise binary
    encoding and includes PostScript-style circular arcs.

    Or the path's command and coordinates can be specified explicitly: */

    static const GLubyte pathCommands[10] =
      { GL_MOVE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV,
        GL_LINE_TO_NV, GL_CLOSE_PATH_NV,
        'M', 'C', 'C', 'Z' };  // character aliases
    static const GLshort pathCoords[12][2] =
      { {100, 180}, {40, 10}, {190, 120}, {10, 120}, {160, 10},
        {300,300}, {200,400}, {100,200}, {300,100},
        {500,200}, {500,400}, {300,300} };
    glPathCommandsNV(mPathObj, 10, pathCommands, 24, GL_SHORT, pathCoords);
}


void PathRenderingBasic::initPaths() {
    if (mPathObj==0)
        mPathObj = glGenPathsNV(1);

    switch (mPathDefMode) {
    case 0:
        LOGI("specifying path via SVG string\n");
        initPathFromSVG();
        break;
    case 1:
        LOGI("specifying path via PS string\n");
        initPathFromPS();
        break;
    case 2:
        LOGI("specifying path via explicit data\n");
        initPathFromData();
        break;
    }

    /* Before rendering, configure the path object with desirable path
    parameters for stroking.  Specify a wider 6.5-unit stroke and
    the round join style: */

    glPathParameteriNV(mPathObj, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
    glPathParameterfNV(mPathObj, GL_PATH_STROKE_WIDTH_NV, 6.5);

    CHECK_GL_ERROR();
}


void PathRenderingBasic::initRendering(void) {
    if (!requireExtension("GL_NV_path_rendering")) return;
    LOGI("Has path rendering!");
    if (!requireExtension("GL_EXT_direct_state_access")) return;
    LOGI("Has DSA!");

    NvAssetLoaderAddSearchPath("PathRenderingBasic");

    initPaths();

    CHECK_GL_ERROR();

    m_transformer->setMotionMode(NvCameraMotionType::PAN_ZOOM);
    m_transformer->setMaxTranslationVel(500); // seems to work decently.
}

// we define an explicit reaction code for path defn change
// so that we can react to it and call initPaths again.
// an alternative would be to let the system generate a code,
// and we would cache either the tweakvar or its reaction
// code in a variable for later comparison.
#define REACT_PATH_MODE 0x10000001

void PathRenderingBasic::initUI() {
    if (mTweakBar) {
        NvTweakVarBase *var;

        // expose the path definition modes
        NvTweakEnum<uint32_t> pathModes[] = {
            {"SVG String", 0},
            {"PS String", 1},
            {"PR Binary", 2}
        };
        mTweakBar->addPadding();
        var = mTweakBar->addEnum("Path Definition Mode:", mPathDefMode, pathModes, TWEAKENUM_ARRAYSIZE(pathModes), REACT_PATH_MODE);
        addTweakKeyBind(var, NvKey::K_P);
        addTweakButtonBind(var, NvGamepad::BUTTON_DPAD_RIGHT, NvGamepad::BUTTON_DPAD_LEFT);

        mTweakBar->addPadding();
        mTweakBar->addLabel("Drawing Options:");

        var = mTweakBar->addValue("Fill Paths", mOptFill);
        addTweakKeyBind(var, NvKey::K_F);
        addTweakButtonBind(var, NvGamepad::BUTTON_LEFT_SHOULDER);

        var = mTweakBar->addValue("Stroke Paths", mOptStroke);
        addTweakKeyBind(var, NvKey::K_S);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER);

        var = mTweakBar->addValue("Even-Odd Fill", mOptEvenOdd);
        addTweakKeyBind(var, NvKey::K_E);
        addTweakButtonBind(var, NvGamepad::BUTTON_X);

        var = mTweakBar->addValue("Dashed Lines", mOptDashLines);
        addTweakButtonBind(var, NvGamepad::BUTTON_Y);
    }
}


void PathRenderingBasic::reshape(int32_t width, int32_t height) {
    glViewport( 0, 0, (GLint) width, (GLint) height );

    CHECK_GL_ERROR();
}


NvUIEventResponse PathRenderingBasic::handleReaction(const NvUIReaction& react) {
    switch (react.code)
    {
    case REACT_PATH_MODE:
        initPaths();
        break;
    default:
        break;
    }
    return nvuiEventNotHandled;
}


void PathRenderingBasic::draw(void) {
    /* Before rendering to a window with a stencil buffer, clear the stencil
    buffer to zero and the color buffer to black: */

    glClearStencil(0);
    //glClearColor(0,0,0,0);
    glClearColor(0.2f,0.2f,0.8f,0);
    glStencilMask(0xFFFFFFFF);
    glEnable(GL_STENCIL_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CHECK_GL_ERROR();

    /* Use an orthographic path-to-clip-space transform to map the
    [0..500]x[0..400] range of the star's path coordinates to the [-1..1]
    clip space cube: */

    nv::matrix4f mvp, temp;
    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    // added some padding to better center in widescreen displays/windows.
    // so now 500x400 => 800x500
    nv::ortho3D(mvp, 0.f-150.f, 500.f+150.f, 0.f-50.f, 400.f+50.f, -500.f, 500.f);
    nv::translation(temp, 500*0.5f, 400*0.5f, 0.0f);
    mvp *= temp;
    mvp *= m_transformer->getTranslationMat();
    //mvp *= m_transformer->getRotationMat();
    mvp *= m_transformer->getScaleMat();
    nv::translation(temp, -500*0.5f, -400*0.5f, 0.0f);
    mvp *= temp;
    glLoadMatrixf(mvp.get_value());

    CHECK_GL_ERROR();

    float dashLen[] = { 24.0f, 16.0f};
    if (mOptDashLines)
        glPathDashArrayNV(mPathObj, 2, dashLen);
    else
        glPathDashArrayNV(mPathObj, 0, dashLen);

    if (mOptFill) {

        /* Stencil the path: */

        glStencilFillPathNV(mPathObj, GL_COUNT_UP_NV, 0x1F);

        /* The 0x1F mask means the counting uses modulo-32 arithmetic. In
        principle the star's path is simple enough (having a maximum winding
        number of 2) that modulo-4 arithmetic would be sufficient so the mask
        could be 0x3.  Or a mask of all 1's (~0) could be used to count with
        all available stencil bits.

        Now that the coverage of the star and the heart have been rasterized
        into the stencil buffer, cover the path with a non-zero fill style
        (indicated by the GL_NOTEQUAL stencil function with a zero reference
        value): */

        if (mOptEvenOdd) {
            glStencilFunc(GL_NOTEQUAL, 0, 0x1);
        } else {
            glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
        }
        glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
        glColor3f(0,1,0); // green
        glCoverFillPathNV(mPathObj, GL_BOUNDING_BOX_NV);

    }

    /* The result is a yellow star (with a filled center) to the left of
    a yellow heart.

    The GL_ZERO stencil operation ensures that any covered samples
    (meaning those with non-zero stencil values) are zero'ed when
    the path cover is rasterized. This allows subsequent paths to be
    rendered without clearing the stencil buffer again.

    A similar two-step rendering process can draw a white outline
    over the star and heart. */

     /* Now stencil the path's stroked coverage into the stencil buffer,
     setting the stencil to 0x1 for all stencil samples within the
     transformed path. */

    if (mOptStroke) {

        glStencilStrokePathNV(mPathObj, 0x1, 0xFFFFFFFF);

         /* Cover the path's stroked coverage (with a hull this time instead
         of a bounding box; the choice doesn't really matter here) while
         stencil testing that writes white to the color buffer and again
         zero the stencil buffer. */

        glColor3f(1,1,0); // yellow
        glCoverStrokePathNV(mPathObj, GL_CONVEX_HULL_NV);

         /* In this example, constant color shading is used but the application
         can specify their own arbitrary shading and/or blending operations,
         whether with Cg compiled to fragment program assembly, GLSL, or
         fixed-function fragment processing.

         More complex path rendering is possible such as clipping one path to
         another arbitrary path.  This is because stencil testing (as well
         as depth testing, depth bound test, clip planes, and scissoring)
         can restrict path stenciling. */
    }

    CHECK_GL_ERROR();
}


NvAppBase* NvAppFactory() {
    return new PathRenderingBasic();
}
