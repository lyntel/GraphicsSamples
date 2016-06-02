//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/RenderTexture.h
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
#ifndef __RENDERTEXTURE_H
#define __RENDERTEXTURE_H

#include <NV/NvPlatformGL.h>


/*
#ifndef ANDROID
#define GL_HALF_FLOAT_OES GL_HALF_FLOAT
#define GL_DEPTH_COMPONENT24_OES GL_DEPTH_COMPONENT24
#define FLOAT_INTERNAL_RGB GL_RGBA16F_ARB 
#define FLOAT_INTERNAL_RGBA GL_RGBA16F_ARB 
#else
#define FLOAT_INTERNAL_RGBA GL_RGBA
#define FLOAT_INTERNAL_RGB GL_RGB
#endif
*/

class RenderTexture {
public:
	enum DepthType {
		Depth24,
		Depth16,
		NoDepth
	};
	enum ColorType {
		RGB888,
		RGBA8888,
		HDR = 0xFFFF,
		RG16F,
		RGB16F,
		RGBA16F,
	};

	RenderTexture();
	~RenderTexture();
	bool Init(int width, int height, ColorType color, DepthType depth);
	bool InitWithPBO(int width, int height, ColorType color, DepthType depth);
	void GetTextureData(void* pData);
	void SetPBOData(void* pData);

	void ActivateFB();
	//void DeactivateFB();
	int GetWidth() {
		return m_width;
	}
	int GetHeight() {
		return m_height;
	}
	void Bind() { glBindTexture(GL_TEXTURE_2D, m_texId); }
	void Release() { glBindTexture(GL_TEXTURE_2D, 0); }
	unsigned int GetTexId() { return m_texId; } 
	static bool ms_useFiltering;

private:
	int m_width;
	int m_height;
	int m_pboSize;
	unsigned int m_format;
	unsigned int m_type;
	unsigned int m_fboId;
	unsigned int m_texId;
	unsigned int m_pboId;
	unsigned int m_depthRBId;
	unsigned int m_colorRBId;
	int m_prevFbo;
	int m_prevViewport[4];
};

#endif  // __RENDERTEXTURE_H_
