//----------------------------------------------------------------------------------
// File:        gl4-maxwell\ConservativeRaster/ConservativeRaster.cpp
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
// Simple example to show conservative rasterization
// sgreen 9/9/2014

/*
    This sample demonstrates conservative rasterization by showing a single
    rasterized triangle zoomed up.
    The dots represent the pixel sample positions.
    Normal rasterization only generates a fragment if the primitive covers the sample point.
    Conservative rasterization generates a fragment if the primitive intersects any part of the pixel.
*/

#include "ConservativeRaster.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NvAppBase/NvInputTransformer.h"

#define GL_CONSERVATIVE_RASTERIZATION_NV 0x9346
#define GL_FILL_RECTANGLE_NV             0x933C

ConservativeRaster::ConservativeRaster() :
    mConservative(true),
	mFillRect(false),
    mEnableZoom(true),
    mWireframe(false),
    mEnableGrid(true),
    mZoom(16),
    mPixels(0)
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -2.f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

ConservativeRaster::~ConservativeRaster()
{
    delete [] mPixels;
    LOGI("ConservativeRaster: destroyed\n");
}

void ConservativeRaster::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

void ConservativeRaster::initUI() {
    // create our tweak ui
    if (mTweakBar) {
        mTweakBar->addValue("Conservative rasterization", mConservative);
        mTweakBar->addValue("Fill bounding rectangle", mFillRect);
        mTweakBar->addValue("Draw lines", mWireframe);
        mTweakBar->addValue("Draw grid", mEnableGrid);
        mTweakBar->addValue("Enable zoom", mEnableZoom);
    }
}

void ConservativeRaster::initRendering(void) {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);    
    glDisable(GL_DEPTH_TEST);

    NvAssetLoaderAddSearchPath("gl4-maxwell/ConservativeRaster");

    if (!requireExtension("GL_NV_conservative_raster")) return;
    if (!requireExtension("GL_NV_fill_rectangle")) return;

    CHECK_GL_ERROR();
}

void ConservativeRaster::reshape(int32_t width, int32_t height)
{
    mWidth = width;
    mHeight = height;

	changeZoom();

    CHECK_GL_ERROR();
}

void ConservativeRaster::changeZoom()
{
    mRenderWidth = (mWidth + mZoom - 1) / mZoom;
    mRenderHeight = (mHeight + mZoom - 1) / mZoom;

	mScaledWidth = mRenderWidth * mZoom;
	mScaledHeight = mRenderHeight * mZoom;

    if (mPixels) {
        delete [] mPixels;
    }
    mPixels = new GLubyte [mRenderWidth*mRenderHeight*4];

	nv::perspective(mProjMatrix, 45.0f, (GLfloat)mScaledWidth / (GLfloat)mScaledHeight, 0.01f, 100.0f);
}

void ConservativeRaster::draw(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
 
    // set transforms
    nv::matrix4f matMV = m_transformer->getModelViewMat();

    // old-school fixed function!
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(mProjMatrix.get_value());

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(matMV.get_value());

    if (mFillRect) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL_RECTANGLE_NV);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, mWireframe ? GL_LINE : GL_FILL);
    }

    if (mEnableZoom) {
        // render at reduced resolution
	    glViewport(0, 0, mRenderWidth, mRenderHeight);
    } else {
        glViewport(0, 0, mWidth, mHeight);
    }

    // draw triangle
    if (mConservative) {
        glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
    }

    glColor3f(1.0f, 0.0f, 0.0f);
    drawTriangle();

    glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);

	if (mEnableZoom) {
		glViewport(0, 0, (GLint)mScaledWidth, (GLint)mScaledHeight);
	}
	else {
		glViewport(0, 0, mWidth, mHeight);
	}

    if (mEnableZoom) {
        // readback and display zoomed up
        // note - not very efficient, but simple
        glReadPixels(0, 0, mRenderWidth, mRenderHeight, GL_RGBA, GL_UNSIGNED_BYTE, mPixels);
        glPixelZoom((GLfloat) mZoom, (GLfloat) mZoom);
        glDrawPixels(mRenderWidth, mRenderHeight, GL_RGBA, GL_UNSIGNED_BYTE, mPixels);

        // draw original triangle
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor3f(1.0f, 1.0f, 0.0f);
        drawTriangle();
    }

    if (mEnableZoom && mEnableGrid) {
        glColor3f(0.0f, 0.0f, 0.0f);
        drawGrid();
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glViewport(0, 0, mWidth, mHeight);
}

void ConservativeRaster::drawTriangle()
{
    glBegin(GL_TRIANGLES);
        glVertex2f(-1.0f, -1.0f);
        glVertex2f(1.5f, -0.5f);
        glVertex2f(0.0f, 1.0f);
    glEnd();
}

void ConservativeRaster::drawGrid()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
	if (mEnableZoom) {
		glOrtho(0.0, mScaledWidth, 0.0, mScaledHeight, -1.0, 1.0);
	}
	else {
		glOrtho(0.0, mWidth, 0.0, mHeight, -1.0, 1.0);
	}

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_LINES);
    for(uint32_t x=0; x<=mWidth; x+=mZoom) {
        glVertex2i(x, 0);
        glVertex2i(x, mHeight);
    }
    for(uint32_t y=0; y<=mHeight; y+=mZoom) {
        glVertex2i(0, y);
        glVertex2i(mWidth, y);
    }
    glEnd();

    // draw sample points
    glBegin(GL_POINTS);
        for(uint32_t x=0; x<=mWidth; x+=mZoom) {
            for(uint32_t y=0; y<=mHeight; y+=mZoom) {
                glVertex2f(x + mZoom*0.5f, y + mZoom*0.5f);
            }
        }
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

NvAppBase* NvAppFactory() {
    return new ConservativeRaster();
}
