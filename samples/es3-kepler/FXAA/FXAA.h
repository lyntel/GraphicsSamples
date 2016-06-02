//----------------------------------------------------------------------------------
// File:        es3-kepler\FXAA/FXAA.h
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
#ifndef FXAA_H
#define FXAA_H

#include "NvAppBase/gl/NvSampleAppGL.h"
#include "NvGLUtils/NvImageGL.h"
#include "NV/NvMath.h"
class NvStopWatch;
class NvFramerateCounter;
class NvGLSLProgram;

class FXAA : public NvSampleAppGL
{
public:
    FXAA();
    ~FXAA();
    
    void initUI(void);
    void initRendering(void);
    void draw(void);
	void drawObjects(nv::matrix4f* mvp, float* color, int id);
    void reshape(int32_t width, int32_t height);
	void drawSphere(nv::matrix4f* mvp);
	void FXAABlit(int level);
    void configurationCallback(NvGLConfiguration& config);
	bool genRenderTexture();

protected:
	NvGLSLProgram* m_ObjectProg;
	NvGLSLProgram* m_LineProg;
	NvGLSLProgram* m_FXAAProg[5];
	nv::matrix4f m_projection_matrix;
    NvImage * m_sourceImage; 
    GLuint m_sourceTexture;
	GLuint m_sourceFrameBuffer;
	GLuint m_sourceDepthBuffer;
	GLuint m_sphereVBO;
	GLuint m_object[2];
	int m_objectFileSize[2];
	int	m_sphereBufferSize;
	float m_lineWidth;
	uint32_t m_FXAALevel;
	float m_aspectRatio;
	bool m_autoSpin;
};

#endif
