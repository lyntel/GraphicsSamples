//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/ComputeWaterSimulation.h
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

#define CPP 1
#include "NV/NvMath.h"
#include "NV/NvShaderMappings.h"
#include "assets/shaders/uniforms.h"


#define NUM_WAVES		4
#define WAVE_WIDTH		130
#define WAVE_HEIGHT		130
#define WAVE_DAMPING	0.99f


class NvFramerateCounter;
class NvGLSLProgram;
class Wave;


// Water surface grid size
static NvTweakEnum<uint32_t> WATER_GRID_SIZE[] = {
	{ "192", 192 },
	{ "128", 128 },
	{  "96",  96 },
	{  "64",  64 },
//	{   "8",   8 },
};


// Water surface shaders
enum {
	WATER_SHADER_DEFAULT,
	WATER_SHADER_NORMALS,
	WATER_SHADER_FRESNEL,

	WATER_SHADER_COUNT,
};
static NvTweakEnum<uint32_t> WATER_SHADER_TYPE[] = {
	{ "Default", WATER_SHADER_DEFAULT },
	{ "Normals", WATER_SHADER_NORMALS },
	{ "Fresnel", WATER_SHADER_FRESNEL },
};


// Number of simulation threads
static NvTweakEnum<uint32_t> WATER_NUM_THREADS[] = {
	{ "4", 4 },
	{ "2", 2 },
	{ "1", 1 },
};


// Application settings
struct Settings
{
	bool		Animate;
	bool		AddRain;
	bool		UseComputeShader;
	uint		Frequency;
	float		Strength;
	float		Size;
	float		Damping;

	Settings() :
		Animate(true),
		AddRain(true),
		UseComputeShader(true),
		Frequency(5),
		Strength(1.0f),
		Size(8.0f),
		Damping(0.99f)
	{
	}
};


// 
class ComputeWaterSimulation : public NvSampleAppGL
{
public:
	ComputeWaterSimulation();
	~ComputeWaterSimulation();

	void initRendering(void);
	void initUI(void);
	void draw(void);
	void reshape(int32_t width, int32_t height);
	void configurationCallback(NvGLConfiguration& config);

    virtual NvUIEventResponse handleReaction(const NvUIReaction &react);

	// input callbacks
	virtual bool handlePointerInput(NvInputDeviceType::Enum device, NvPointerActionType::Enum action, 
        uint32_t modifiers, int32_t count, NvPointerEvent* points);
	virtual bool handleGamepadButtonChanged(uint32_t button, bool down);
	virtual bool handleGamepadChanged(uint32_t changedPadFlags);

private:
	void runWaterSimulation();
	void simulateWaterCPU();
	void simulateWaterGPU();
	void drawWater();
	void initWaves(int numWaves);
	void doReset();

	GLint createShaderPipelineProgram(GLuint target, const char* src, GLuint &pipeline, GLuint &program);

	// unproject point in screen space to view space
	nv::vec3f unProjectPoint(nv::vec4f point);

	// unproject point in screen space to world space
	nv::vec3f unProject(float x, float y);

	// get random value in range [mins, maxs]
	float Rand(float mins, float maxs);

private:
    GLint mWidth;
    GLint mHeight;

	GLuint mSkyTexture;
	
	Settings mSettings;
	ShaderParams mShaderParams;
	GLuint mShaderUBO;

	nv::vec4f mDisturbance;
	uint mRainFrame;

	GLuint mTransformPipeline;
	GLuint mTransformProgram;

	GLuint mGradientsPipeline;
	GLuint mGradientsProgram;

	GLuint mHeightBuffer[NUM_WAVES];
	GLuint mVelocityBuffer[NUM_WAVES];

	nv::matrix4f mProjectionMatrix;

	// replace memory barriers with glFinish on non-NV cards
	bool m_hackMemoryBarrier;

	// input
	bool mTouchDown;
	bool mJoystickDown;

	bool mPrevFrameUseComputeShader;
	bool mReset;
	float mTime;

	Wave** mWaves;
	uint32_t mNumWaves, mPrevNumWaves;
	uint32_t mGridSize, mPrevGridSize;
	float mWaveScale;

	NvGLSLProgram* mWaterShader[WATER_SHADER_COUNT];
	uint32_t mWaterShaderType;
};
