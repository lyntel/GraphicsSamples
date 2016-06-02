//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/HDR.h
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
#include "NvAppBase/gl/NvSampleAppGL.h"

#include "KHR/khrplatform.h"
#include "NvGamepad/NvGamepad.h"
#include "NV/NvMath.h"
#include "RenderTexture.h"
#include "VertexBufferObject.h"
#include "ShaderObject.h"
#include "ShaderDeclaration.h"
#include "BlurShaderGenerator.h"
#include "CubeData.h"
#include "HDRImages.h"

class HDR : public NvSampleAppGL
{
public:
    HDR();
    ~HDR();
    
    void initRendering(void);
    void initUI(void);
    void draw(void);
    void reshape(int32_t width, int32_t height);
	void updateDynamics();
    void configurationCallback(NvGLConfiguration& config);

	enum HDR_PASS { //Program ID enum
		SKYBOX = 0,
		MATTEOBJECT,
		REFRACTOBJECT,
		REFLECTOBJECT,
		DOWNSAMPLE,
		DOWNSAMPLE4X,
		BLURH4,
		BLURV4,
		BLURH6,
		BLURV6,
		BLURH10,
		BLURV10,
		BLURH12,
		BLURV12,
		TONEMAPPING,
		EXTRACTHL,
		GAUSCOMP,
		STARSTREAK,
		GLARECOMP,
		STARCOMP,
		GHOSTIMAGE,
		CALCLUMINANCE,
		CALCADAPTEDLUM,
		PROGSIZE,
	};

	enum BUFFER_PYRAMID {
		LEVEL_0 = 0, 
		LEVEL_1, 
		LEVEL_2,     
		LEVEL_3,
		LEVEL_4,
		LEVEL_5,
		LEVEL_TOTAL,
	};

	enum GLARE_TYPE {
		CAMERA_GLARE, 
		FILMIC_GLARE, 
	};

private:

	void InitRenderTexture(int width, int height);
	void DrawScene();
	void DrawObject();
	void DrawSkyBox();

	void toneMappingPass();
	void downsample(RenderTexture *src, RenderTexture *dest);
	void downsample4x(RenderTexture *src, RenderTexture *dest);
	void extractHL(RenderTexture *src, RenderTexture *dest);
	void ComposeEffect();
	void genStarStreak(int dir);
	void genHorizontalGlare(int dir);
	void genGhostImage(float* ghost_modulation1st, float* ghost_modulation2nd);
	void calculateLuminance();
	void blitBuffer(RenderTexture* src);
	void blur(RenderTexture *src, RenderTexture *dest, RenderTexture *temp, int blurWidth);
	void run_pass(HDR_PASS prog, RenderTexture *src, RenderTexture *dest);
	void switchScene();
    void GetBufferPyramidSize(BUFFER_PYRAMID level, int* w, int* h);
	bool initModels();
	bool initShaders();
	void initBlurCode();
	void render();

	uint32_t m_postProcessingWidth;
	uint32_t m_postProcessingHeight;
	uint32_t m_sceneIndex;
	uint32_t m_objectIndex;
	uint32_t m_materialIndex;
	uint32_t m_glareType;

 	nv::matrix4f projection_matrix;
	VertexBufferObject m_skybox;
	VertexBufferObject m_object[3];
	int m_objectFileSize[3];
	GLuint m_lumCurrent;
	GLuint m_lum[2];
	GLuint m_lensMask;

	RenderTexture*	scene_buffer;
	RenderTexture*	compose_buffer[LEVEL_TOTAL];
	RenderTexture*	blur_bufferA[LEVEL_TOTAL];
	RenderTexture*	blur_bufferB[LEVEL_TOTAL];
	RenderTexture*	streak_bufferA[4];
	RenderTexture*	streak_bufferB[4];
	RenderTexture*	streak_bufferFinal;
	RenderTexture*	ghost1st_buffer;
	RenderTexture*	ghost2nd_buffer;
	RenderTexture*	glare_buffer;
	RenderTexture*	offscreen_buffer;
	RenderTexture*	exp_buffer[2];		//exposure info buffer
	BUFFER_PYRAMID	m_starGenLevel;
	CShaderObject   m_shaders[PROGSIZE];

	bool m_bInitialized;
	float m_aspectRatio;
	float m_blendAmount;
	float m_gamma;
	float m_lumThreshold;
	float m_lumScaler;
	float m_expAdjust;
    bool m_drawHDR;
	bool m_drawBackground;
	bool m_autoExposure;
    bool m_autoSpin;

};
