//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/RenderTexture.cpp
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
#include "RenderTexture.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef GL_RGBA16F
#define GL_RGBA16F GL_RGBA16F_ARB
#endif

bool RenderTexture::ms_useFiltering = true;

RenderTexture::RenderTexture():
	m_width(0U),
	m_height(0U),
	m_fboId(0),
	m_texId(0),
	m_pboId(0),
	m_depthRBId(0),
	m_colorRBId(0)
{
}

RenderTexture::~RenderTexture() {
	if (m_texId) {
		glDeleteTextures(1, &m_texId);
		m_texId = 0;
	}
	if (m_colorRBId) {
		glDeleteRenderbuffers(1, &m_colorRBId);
		m_colorRBId = 0;
	}
	if (m_depthRBId) {
		glDeleteRenderbuffers(1, &m_depthRBId);
		m_depthRBId = 0;
	}
	if (m_fboId) {
		glDeleteFramebuffers(1, &m_fboId);
		m_fboId = 0;
	}
	if (m_pboId) {
		glDeleteBuffers(1, &m_pboId);
	}
}

bool RenderTexture::Init(int width, int height, ColorType color, DepthType depth)
{
	GLenum err;
	m_width = width;
	m_height = height;
	glGenFramebuffers(1, &m_fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
	err = glGetError();

	if (color >= HDR) {
		// Create the HDR render target
		switch (color) {
		case RGB16F: m_format = GL_RGB;  m_type = GL_HALF_FLOAT; break;
		default:
		case RGBA16F: m_format = GL_RGBA; m_type = GL_HALF_FLOAT; break;
		}
		glGenTextures(1, &m_texId);
		glBindTexture(GL_TEXTURE_2D, m_texId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ms_useFiltering ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ms_useFiltering ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//note that internalformat and format have both to be GL_RGB or GL_RGBA for creating floating point texture.
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, width, height); 
		err = glGetError();
		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texId, 0);
		err = glGetError();

	} else {
		// Create the LDR render target
		m_type = GL_UNSIGNED_BYTE;
		glGenTextures(1, &m_texId);
		glBindTexture(GL_TEXTURE_2D, m_texId);
		switch (color) {
		case RGBA8888: m_format = GL_RGBA; break;
		default:
		case RGB888: m_format = GL_RGB; break;
		}
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexStorage2D(GL_TEXTURE_2D, 1, m_format, width, height); 

		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texId, 0);
	}
	// Create depth buffer
	if (depth != NoDepth) {
		glGenRenderbuffers(1, &m_depthRBId);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depthRBId);
		err = glGetError();
		unsigned int depth_format = 0;
		switch (depth) {
		case Depth24: depth_format = GL_DEPTH_COMPONENT24; break;
		default:
		case Depth16: depth_format = GL_DEPTH_COMPONENT16; break;
		}
		glRenderbufferStorage(GL_RENDERBUFFER, depth_format, width, height);
		err = glGetError();
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRBId);
		err = glGetError();
		glBindRenderbuffer(GL_RENDERBUFFER, 0);    
	}
	// Finalize
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return false;
	}
	return true;
}

void RenderTexture::ActivateFB()
{
	//first save old FB, and then activate new FB
	//glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_prevFbo);
	//glGetIntegerv(GL_VIEWPORT, m_prevViewport);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fboId); 
	glViewport(0, 0, m_width, m_height);
}

//void RenderTexture::DeactivateFB()
//{
//	//restore from saved FB
//	glBindFramebuffer(GL_FRAMEBUFFER, m_prevFbo);
//	glViewport(m_prevViewport[0], m_prevViewport[1], m_prevViewport[2], m_prevViewport[3]);
//}

//initialize render texture with pixel buffer object attached to it
bool RenderTexture::InitWithPBO(int width, int height, ColorType color, DepthType depth)
{
	////get extension function pointer from string
	//pglGetTexImageNV = (PFNGLGETTEXIMAGENVPROC)eglGetProcAddress("glGetTexImageNV");
	//pglMapBufferOES = (PFNGLMAPBUFFEROESPROC)eglGetProcAddress("glMapBufferOES");
	//pglUnmapBufferOES = (PFNGLUNMAPBUFFEROESPROC)eglGetProcAddress("glUnmapBufferOES");

	//if (!pglGetTexImageNV || !pglMapBufferOES || !pglUnmapBufferOES) return false;

	//int BytePP;

	//if (color >= HDR) BytePP = 8;
	//else BytePP = 4;

	//glGenBuffers(1, &m_pboId);
	//glBindBuffer(GL_PIXEL_PACK_BUFFER_NV, m_pboId);
	//glBufferData(GL_PIXEL_PACK_BUFFER_NV, width*height*BytePP, NULL, GL_DYNAMIC_DRAW);
	//m_pboSize = width*height*BytePP;
	//Init(width, height, color, depth);
	//glBindBuffer(GL_PIXEL_PACK_BUFFER_NV, 0);

	return 0;
}

void RenderTexture::GetTextureData(void* pData)
{
	//GLenum err;
	//if (m_pboId == -1) return;
 //   glBindTexture(GL_TEXTURE_2D, m_texId);
 //   pglGetTexImageNV(GL_TEXTURE_2D, 0,m_format, m_type, pData);
}

void RenderTexture::SetPBOData(void* pData)
{
	//if (m_pboId == -1) return;

	//glBindBuffer(GL_PIXEL_PACK_BUFFER_NV, m_pboId);
	//void* data = pglMapBufferOES(GL_PIXEL_PACK_BUFFER_NV, GL_WRITE_ONLY_OES);
	//if (!data) return;
	//memcpy(data, pData, m_pboSize);
	//pglUnmapBufferOES(GL_PIXEL_PACK_BUFFER_NV);

	//glBindTexture(GL_TEXTURE_2D, m_texId);
	//glTexImage2D( GL_TEXTURE_2D, 0, m_format, m_width, m_height, 0, m_format, m_type, pData);

	//glBindBuffer(GL_PIXEL_PACK_BUFFER_NV, 0);
}
