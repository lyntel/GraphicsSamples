//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/ComputeWaterSimulation.cpp
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
#include "ComputeWaterSimulation.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvImageGL.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#undef APIENTRY
#include "Wave.h"
#include "WaveSimRenderer.h"


const uint32_t MAX_GRID_SIZE = WATER_GRID_SIZE[0].m_value;

enum UIReactionIDs {   
    REACT_RESET         = 0x10000001,
};

extern std::string loadShaderSourceWithUniformTag(const char* uniformsFile, const char* srcFile);


ComputeWaterSimulation::ComputeWaterSimulation() 
	: mTouchDown(false),
	mJoystickDown(false),
	mRainFrame(0),
	mTime(0.0f),
	mWaves(NULL),
	m_hackMemoryBarrier(false)
{
	m_transformer->setMotionMode(NvCameraMotionType::FIRST_PERSON);
	m_transformer->setTranslationVec(nv::vec3f(0.0f, -0.5f, -3.0f));

	// Required in all subclasses to avoid silent link issues
	forceLinkHack();

	mPrevFrameUseComputeShader = mSettings.UseComputeShader;

	mPrevNumWaves = mNumWaves = WATER_NUM_THREADS[0].m_value;
	mPrevGridSize = mGridSize = WATER_GRID_SIZE[1].m_value;
	mWaterShaderType = WATER_SHADER_TYPE[0].m_value;
}

ComputeWaterSimulation::~ComputeWaterSimulation()
{
	LOGI("ComputeWaterSimulation: destroyed\n");
}

void ComputeWaterSimulation::configurationCallback(NvGLConfiguration& config)
{ 
	config.depthBits = 24; 
	config.stencilBits = 0; 
	config.apiVer = NvGLAPIVersionGL4_3();
}


GLint ComputeWaterSimulation::createShaderPipelineProgram(GLuint target, const char* src, GLuint &pipeline, GLuint &program)
{
	GLint status;

	glGenProgramPipelines(1, &pipeline);
	program = glCreateShaderProgramv(target, 1, (const GLchar **)&src);

	glBindProgramPipeline(pipeline);
	glUseProgramStages(pipeline, GL_COMPUTE_SHADER_BIT, program);
	glValidateProgramPipeline(pipeline);
	glGetProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, &status);

	if (status != GL_TRUE)
	{
		GLint logLength;
		glGetProgramPipelineiv(pipeline, GL_INFO_LOG_LENGTH, &logLength);
		char *log = new char [logLength];
		glGetProgramPipelineInfoLog(pipeline, logLength, 0, log);
		LOGI("Shader pipeline not valid:\n%s\n", log);
		delete [] log;
	}

	glBindProgramPipeline(0);

	return status;
}

void ComputeWaterSimulation::initRendering(void) {
    // OpenGL 4.3 is the minimum for compute shaders
    if (!requireMinAPIVersion(NvGLAPIVersionGL4_3()))
        return;

	// check if we need glMemoryBarrier hack (on non-NV, we are
    // seeing corruption caused by glMemoryBarrier not actually
    // synchronizing)
	const char* vendorName = (const char*)glGetString(GL_VENDOR);
	if (!strstr(vendorName, "NVIDIA"))
	{
		LOGI("glMemoryBarrier() hack enabled. Vendor = %s\n", vendorName);
		m_hackMemoryBarrier = true;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    

	NvAssetLoaderAddSearchPath("es3aep-kepler/ComputeWaterSimulation");

	int cx, cy, cz;
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &cx );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &cy );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &cz );
	LOGI("Max compute work group count = %d, %d, %d\n", cx, cy, cz );

	int sx, sy, sz;
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &sx );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &sy );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &sz );
	LOGI("Max compute work group size  = %d, %d, %d\n", sx, sy, sz );

	CHECK_GL_ERROR();

	// load shaders
	mWaterShader[WATER_SHADER_DEFAULT] = NvGLSLProgram::createFromFiles("shaders/water.vert", "shaders/water.frag");
	mWaterShader[WATER_SHADER_NORMALS] = NvGLSLProgram::createFromFiles("shaders/water.vert", "shaders/waterNormals.frag");
	mWaterShader[WATER_SHADER_FRESNEL] = NvGLSLProgram::createFromFiles("shaders/water.vert", "shaders/waterFresnel.frag");
	
	// load compute shaders
	std::string src;
	src = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/WaterTransformPass.glsl");
	createShaderPipelineProgram(GL_COMPUTE_SHADER, src.c_str(), mTransformPipeline, mTransformProgram);

	src = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/WaterGradientsPass.glsl");
	createShaderPipelineProgram(GL_COMPUTE_SHADER, src.c_str(), mGradientsPipeline, mGradientsProgram);

	mSkyTexture = NvImageGL::UploadTextureFromDDSFile("sky/day.dds");

	// init GPU buffers
	uint32_t maxGridSize = MAX_GRID_SIZE + 2;
	GLuint size = sizeof(GLfloat) * maxGridSize * maxGridSize;
	for(int i = 0; i < 4; i++)
	{
		glGenBuffers(1, &mHeightBuffer[i]);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mHeightBuffer[i]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glGenBuffers(1, &mVelocityBuffer[i]);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mVelocityBuffer[i]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	initWaves(mNumWaves);

	//for(int i = 0; i < 4; i++)
	//{
	//	glGenBuffers(1, &mHeightBuffer[i]);
	//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mHeightBuffer[i]);
	//	glBufferData(GL_SHADER_STORAGE_BUFFER, size, mWaves[0]->getSimulation().getHeightField(), GL_STATIC_DRAW);
	//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	//	glGenBuffers(1, &mVelocityBuffer[i]);
	//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mVelocityBuffer[i]);
	//	glBufferData(GL_SHADER_STORAGE_BUFFER, size, mWaves[0]->getSimulation().getHeightField(), GL_STATIC_DRAW);
	//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	//}
}


void ComputeWaterSimulation::initUI(void) {
	if (mTweakBar) {
        NvTweakVarBase *var;

		// Animation params
		var = mTweakBar->addValue("Animate", mSettings.Animate);
        addTweakKeyBind(var, NvKey::K_SPACE);
        addTweakButtonBind(var, NvGamepad::BUTTON_Y);
		var = mTweakBar->addValue("Use Compute Shader", mSettings.UseComputeShader);
		var = mTweakBar->addValue("Add Rain", mSettings.AddRain);

		// Surface params
		mTweakBar->addPadding();
		var = mTweakBar->addMenu("Num Surfaces", mNumWaves, WATER_NUM_THREADS, TWEAKENUM_ARRAYSIZE(WATER_NUM_THREADS));
		var = mTweakBar->addMenu("Grid Size", mGridSize, WATER_GRID_SIZE, TWEAKENUM_ARRAYSIZE(WATER_GRID_SIZE));
		var = mTweakBar->addMenu("Water Shader", mWaterShaderType, WATER_SHADER_TYPE, TWEAKENUM_ARRAYSIZE(WATER_SHADER_TYPE));

		// Wave simulation params
		mTweakBar->addPadding();
		var = mTweakBar->addValue("Damping", mSettings.Damping, 0.8f, 1.0f, 0.01f);
		var = mTweakBar->addValue("Wave Size", mSettings.Size, 0.0f, 20.0f, 1.0f);	
		var = mTweakBar->addValue("Wave Strength", mSettings.Strength, 0.0f, 3.0f, 0.1f);
		var = mTweakBar->addValue("Rain Frequency", mSettings.Frequency, 0, 20, 1);

        // reset button.
        mTweakBar->addPadding();
        var = mTweakBar->addButton("Reset", REACT_RESET);
        addTweakKeyBind(var, NvKey::K_R);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_THUMB);
    }
}


NvUIEventResponse ComputeWaterSimulation::handleReaction(const NvUIReaction &react)
{
    switch (react.code) {
        case REACT_RESET:
            mTweakBar->resetValues();
            m_transformer->reset();
           	m_transformer->setTranslationVec(nv::vec3f(0.0f, -0.5f, -3.0f));
            reshape(mWidth, mHeight);
            // have to manually reset some things...  best to do AFTER values since that changes things...
		    initWaves(mNumWaves);
		    mPrevNumWaves = mNumWaves;
		    mPrevGridSize = mGridSize;
            doReset();
            return nvuiEventHandled;
        }
    return nvuiEventNotHandled;
}


void ComputeWaterSimulation::reshape(int32_t width, int32_t height)
{
    mWidth = width;
    mHeight = height;

	glViewport(0, 0, mWidth, mHeight);

	nv::perspective(mProjectionMatrix, 60.0f * 3.14f / 180.0f,
		(float)width / (float)height, 0.1f, 150.0f);

	CHECK_GL_ERROR();
}

void ComputeWaterSimulation::draw(void)
{
	glClearColor( 0.25f, 0.25f, 0.25f, 1.0f);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	runWaterSimulation();
	drawWater();

	CHECK_GL_ERROR();
}


float ComputeWaterSimulation::Rand(float mins, float maxs)
{
	float d = maxs - mins;
	float r = mins + d * rand() / RAND_MAX;
	return r;
}


void ComputeWaterSimulation::runWaterSimulation()
{
	// update surface params
	if (mGridSize != mPrevGridSize || mNumWaves != mPrevNumWaves)
	{
		initWaves(mNumWaves);
		mPrevNumWaves = mNumWaves;
		mPrevGridSize = mGridSize;
	}

	// add rain
	if (mSettings.AddRain)
	{
		if (mSettings.Frequency != 0 && mDisturbance.w <= 0.0f)
		{
			if (++mRainFrame >= mSettings.Frequency)
			{
				mRainFrame = 0;
				mDisturbance = nv::vec4f(Rand(-2.0f, 2.0f), Rand(-2.0f, 2.0f), mSettings.Size, Rand(-mSettings.Strength, mSettings.Strength));
			}
		}
	}

	// project ray using joystick
	if (mJoystickDown)
	{
		// use screen center
		nv::vec3f point = unProject((float)m_width / 2, (float)m_height / 2);
		nv::vec2f gridPos;

		// project onto each wave
		for(uint32_t i = 0; i < mNumWaves; i++)
		{
			if (mWaves[i]->mapPointXZToGridPos(point, gridPos))
			{
				mDisturbance = nv::vec4f(point.x, point.z, mSettings.Size, mSettings.Strength);
			}
		}
	}

	// sync CPU and GPU buffers if settings changed
	if (mSettings.UseComputeShader != mPrevFrameUseComputeShader)
	{
		if (mSettings.UseComputeShader)
		{
			for(uint32_t i = 0; i < mNumWaves; i++)
			{
				void* ptr;

				// sync height data
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, mHeightBuffer[i]);
				ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 
                    0, mWaves[i]->getSimulation().m_heightFieldSize, GL_MAP_WRITE_BIT);
				memcpy(ptr, mWaves[i]->getSimulation().getHeightField(), mWaves[i]->getSimulation().m_heightFieldSize);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

				// sync velocity data
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, mVelocityBuffer[i]);
				ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 
                    0, mWaves[i]->getSimulation().m_heightFieldSize, GL_MAP_WRITE_BIT);
				memcpy(ptr, mWaves[i]->getSimulation().getVelocity(), mWaves[i]->getSimulation().m_heightFieldSize);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}
		}
		else
		{
			for(uint32_t i = 0; i < mNumWaves; i++)
			{
				void* ptr;

				// sync height data
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, mHeightBuffer[i]);
				ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 
                    0, mWaves[i]->getSimulation().m_heightFieldSize, GL_MAP_READ_BIT);
				memcpy(mWaves[i]->getSimulation().getHeightField(), ptr, mWaves[i]->getSimulation().m_heightFieldSize);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

				// sync velocity data
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, mVelocityBuffer[i]);
				ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 
                    0, mWaves[i]->getSimulation().m_heightFieldSize, GL_MAP_READ_BIT);
				memcpy(mWaves[i]->getSimulation().getVelocity(), ptr, mWaves[i]->getSimulation().m_heightFieldSize);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}

			//HACK: simulate once even if animation disabled
			bool mCurAnimate = mSettings.Animate;
			mSettings.Animate = true;
			simulateWaterCPU();
			mSettings.Animate = mCurAnimate;
		}

		mPrevFrameUseComputeShader = mSettings.UseComputeShader;
	}

	if (mSettings.UseComputeShader)
	{
		simulateWaterGPU();
	}
	else
	{
		simulateWaterCPU();
	}

	// reset disturbance
	mDisturbance = nv::vec4f(0.0f);
}


void ComputeWaterSimulation::simulateWaterCPU()
{
	Wave::resetThreadStopWatch();

	if (mSettings.Animate)
	{
		if (mDisturbance.w > 0.0f)
		{
			nv::vec3f point(mDisturbance.x, 0.0f, mDisturbance.y);
			nv::vec2f gridPos;
			for(uint32_t i = 0; i < mNumWaves; i++)
			{
				if (mWaves[i]->mapPointXZToGridPos(point, gridPos))
				{
					mWaves[i]->getSimulation().addDisturbance(gridPos.x, gridPos.y,
						mDisturbance.z * mWaveScale, mDisturbance.w * mWaveScale);
				}
			}
		}

		for(uint32_t i = 0; i < mNumWaves; i++)
		{
			mWaves[i]->getSimulation().setDamping(mSettings.Damping);
			mWaves[i]->startSimulationThread();
		}
	}
	else
	{
		Wave::pauseAllSimulationThreads();
	}

	Wave::syncAllSimulationThreads();
	for(uint32_t i = 0; i < mNumWaves; i++)
	{
		mWaves[i]->updateBufferData();
	}
}


void ComputeWaterSimulation::simulateWaterGPU()
{
	nv::vec4f disturbance[NUM_WAVES];
	GLint loc;

	if (mSettings.Animate)
	{
		nv::vec3f point(mDisturbance.x, 0.0f, mDisturbance.y);
		nv::vec2f gridPos;

		// compute disturbance for each wave
		for(uint32_t i = 0; i < mNumWaves; i++)
		{
			if (mWaves[i]->mapPointXZToGridPos(point, gridPos))
			{
				disturbance[i] = nv::vec4f(gridPos.x, gridPos.y, mSettings.Size * mWaveScale, mSettings.Strength * mWaveScale);
			}
		}

		loc = glGetUniformLocation(mTransformProgram, "Damping");
		glProgramUniform1f(mTransformProgram, loc, mSettings.Damping);

		loc = glGetUniformLocation(mTransformProgram, "GridSize");
		glProgramUniform1ui(mTransformProgram, loc, mGridSize + 2);

		glBindProgramPipeline(mTransformPipeline);

		for(uint32_t i = 0; i < mNumWaves; i++)
		{
			loc = glGetUniformLocation(mTransformProgram, "Disturbance");
			glProgramUniform4fv(mTransformProgram, loc, 1, disturbance[i]._array);

			GLuint heightBuffer = mWaves[i]->getRenderer().m_waterYVBO[mWaves[i]->getRenderer().m_renderCurrent];

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mHeightBuffer[i]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, heightBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mVelocityBuffer[i]);
			
			glDispatchCompute((mGridSize / WORK_GROUP_SIZE) + 1, (mGridSize / WORK_GROUP_SIZE) + 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			//glMemoryBarrier(GL_ALL_BARRIER_BITS);
			CHECK_GL_ERROR();

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
		}

		glBindProgramPipeline(0);
		CHECK_GL_ERROR();
	}	

    // On non-NV, we are seeing corruption caused by glMemoryBarrier not actually synchronizing
    if (m_hackMemoryBarrier)
		glFinish();

	// calculate gradients
	glBindProgramPipeline(mGradientsPipeline);

	loc = glGetUniformLocation(mGradientsProgram, "GridSize");
	glProgramUniform1ui(mGradientsProgram, loc, mGridSize + 2);

	for(uint32_t i = 0; i < mNumWaves; i++)
	{
		GLuint heightBuffer = mWaves[i]->getRenderer().m_waterYVBO[mWaves[i]->getRenderer().m_renderCurrent];
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, heightBuffer);

		GLuint gradientsBuffer = mWaves[i]->getRenderer().m_waterGVBO[mWaves[i]->getRenderer().m_renderCurrent];
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gradientsBuffer);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mHeightBuffer[i]);
		
		glDispatchCompute((mGridSize / WORK_GROUP_SIZE) + 1, (mGridSize / WORK_GROUP_SIZE) + 1, 1);
		//glDispatchCompute(4, 4, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BUFFER);
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);
		CHECK_GL_ERROR();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
	}

	glBindProgramPipeline(0);
	CHECK_GL_ERROR();

    // On non-NV, we are seeing corruption caused by glMemoryBarrier not actually synchronizing
	if (m_hackMemoryBarrier)
		glFinish();
}


bool ComputeWaterSimulation::handleGamepadChanged(uint32_t changedPadFlags) {
	if (changedPadFlags) {
		NvGamepad* pad = getPlatformContext()->getGamepad();
		if (!pad) return false;

		LOGI("gamepads: 0x%08x", changedPadFlags);
	}

	return false;
}

std::string loadShaderSourceWithUniformTag(const char* uniformsFile, const char* srcFile) {
	int32_t len;

	char *uniformsStr = NvAssetLoaderRead(uniformsFile, len);
	if (!uniformsStr)
		return "";

	char *srcStr = NvAssetLoaderRead(srcFile, len);
	if (!srcStr)
		return "";

	std::string dest = "";

	const char* uniformTag = "#UNIFORMS";

	char* uniformTagStart = strstr(srcStr, uniformTag);

	if (uniformTagStart) {
		// NULL the start of the tag
		*uniformTagStart = 0;
		dest += srcStr; // source up to tag
		dest += "\n";
		dest += uniformsStr;
		dest += "\n";
		char* uniformTagEnd = uniformTagStart + strlen(uniformTag);
		dest += uniformTagEnd;
	} else {
		dest += srcStr;
	}

	NvAssetLoaderFree(uniformsStr);
	NvAssetLoaderFree(srcStr);

	return dest;
}

NvAppBase* NvAppFactory() {
	return new ComputeWaterSimulation();
}

void ComputeWaterSimulation::drawWater()
{
	//float timestamp = 0.001f;
	CHECK_GL_ERROR();

	//glPolygonMode(GL_FRONT, GL_LINE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	mWaterShader[mWaterShaderType]->enable();

	glUniformMatrix4fv(mWaterShader[mWaterShaderType]->getUniformLocation("ProjectionMatrix"), 1, GL_FALSE, mProjectionMatrix._array);

	nv::vec4f eyePos = nv::vec4f(-m_transformer->getTranslationVec(), 1.0f);
	glUniform4fv(mWaterShader[mWaterShaderType]->getUniformLocation("eyePositionWorld"), 1, eyePos);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mSkyTexture);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glUniform1i(mWaterShader[mWaterShaderType]->getUniformLocation("skyTexCube"), 3);

	for(uint32_t i = 0; i < mNumWaves; i++)
	{
		//HACK: swap height buffers
		uint currentBuffer = mWaves[i]->getRenderer().m_renderCurrent;
		GLuint tmp = mWaves[i]->getRenderer().m_waterYVBO[currentBuffer];
		if (mSettings.UseComputeShader)
		{
			mWaves[i]->getRenderer().m_waterYVBO[currentBuffer] = mHeightBuffer[i];
		}

		mWaves[i]->getRenderer().setViewMatrix(m_transformer->getModelViewMat());
		mWaves[i]->getRenderer().setColor(1.0f, 1.0f, 1.0f, 1.0f);
		mWaves[i]->render(
			mWaterShader[mWaterShaderType]->getUniformLocation("vColor"),
			mWaterShader[mWaterShaderType]->getUniformLocation("ModelViewMatrix"),
			mWaterShader[mWaterShaderType]->getUniformLocation("ModelMatrix"),
			mWaterShader[mWaterShaderType]->getUniformLocation("NormalMatrix"),
			mWaterShader[mWaterShaderType]->getAttribLocation("vPositionXZ"),
			mWaterShader[mWaterShaderType]->getAttribLocation("vPositionY"),
			mWaterShader[mWaterShaderType]->getAttribLocation("vGradient"));

		if (mSettings.UseComputeShader)
		{
			mWaves[i]->getRenderer().m_waterYVBO[currentBuffer] = tmp;
		}
	}

	mWaterShader[mWaterShaderType]->disable();

	glDisable(GL_CULL_FACE);
}


void ComputeWaterSimulation::initWaves(int numWaves)
{
	mWaveScale = (float)mGridSize / MAX_GRID_SIZE;

	WaveSimThread::Init(createStopWatch());

	// TODO: fix delete
	//WaveSimRenderer::m_renderersCount = 0;
	if (mWaves)
	{
		Wave::syncAllSimulationThreads();
		for(uint32_t i = 0; i < mPrevNumWaves; i++)
		{
			delete mWaves[i];
		}

		delete [] mWaves;
	}

	mNumWaves = numWaves;
	mWaves = new Wave*[numWaves];
	for(int i=0; i<numWaves; i++)
	{
		mWaves[i] = new Wave(mGridSize, mGridSize, mSettings.Damping,
			nv::vec4f(0.8f*rand()/(float)RAND_MAX, 0.8f*rand()/(float)RAND_MAX, 0.7f + 0.3f* rand()/(float)RAND_MAX, 0.5f));

		//mWaves[i]->addRandomDisturbance((int)mSettings.Size, mSettings.Strength);
		//for(int j=0; j<WaveSimRenderer::NUM_BUFFERS; j++)
		//{
		//	mWaves[i]->updateBufferData();
		//}
	}
	
	doReset();
}



void ComputeWaterSimulation::doReset()
{
	uint32_t maxGridSize = MAX_GRID_SIZE + 2;
	GLuint size = sizeof(GLfloat) * maxGridSize * maxGridSize;

	float* ptr;

	for(uint32_t i = 0; i < mNumWaves; i++)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mVelocityBuffer[i]);
		ptr = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, GL_MAP_WRITE_BIT);
		memset(ptr, 0, size);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mHeightBuffer[i]);
		ptr = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, GL_MAP_WRITE_BIT);
		memset(ptr, 0, size);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		size_t heightFieldSize = sizeof(float) * mWaves[i]->getSimulation().getWidth() * mWaves[i]->getSimulation().getHeight();

		memset(mWaves[i]->getSimulation().getHeightField(), 0, heightFieldSize);
		memset(mWaves[i]->getSimulation().getVelocity(), 0, heightFieldSize);
		mWaves[i]->updateBufferData();
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


nv::vec3f ComputeWaterSimulation::unProjectPoint(nv::vec4f point)
{
	//tranform the points from (0/0)-(width/height) to (-1/-1)-(1/1)
	point.x /= (float)m_width;
	point.y /= (float)m_height;
	point.x = point.x*2.0f - 1.0f;
	point.y = point.y*2.0f - 1.0f;
	point.z = 2.0f*point.z - 1.0f;

	nv::vec4f pos = nv::inverse(mProjectionMatrix * m_transformer->getModelViewMat()) * point;
	pos = pos/pos.w;

	return nv::vec3f(pos.x, pos.y, pos.z);
}


nv::vec3f ComputeWaterSimulation::unProject(float x, float y)
{
	nv::vec3f xzNormal(0.0f, 1.0f, 0.0f);
	nv::vec3f xzPoint(1.0f, 0.0f, 1.0f);
	nv::vec3f projPoint;
	nv::vec3f f = unProjectPoint(nv::vec4f(x, m_height - y, 1.0f, 1.0f));
	nv::vec3f n = -nv::vec3f(m_transformer->getTranslationVec()._array);

	//LOGI("far [ %f %f %f ]", far.x, far.y, far.z);
	nv::vec3f direction = nv::normalize( f - n );

	float dirDotN = nv::dot(direction, xzNormal);

	float d = 0.0f;

	if (dirDotN != 0.0f)
	{
		d = nv::dot((xzPoint - n), xzNormal) / dirDotN;
		projPoint = n + d * direction;
	}

	return projPoint;
}


bool ComputeWaterSimulation::handlePointerInput(NvInputDeviceType::Enum device, NvPointerActionType::Enum action, uint32_t modifiers, int32_t count, NvPointerEvent* points)
{
	if (mTouchDown || !NvSampleApp::handlePointerInput(device, action, modifiers, count, points))
	{
		if (action == NvPointerActionType::UP)
		{
			mTouchDown = false;
		}
		else if (action == NvPointerActionType::DOWN)
		{
			// check if we hit any water surface
			nv::vec3f point = unProject(points[0].m_x, points[0].m_y);
			nv::vec2f gridPos;

			// project onto each wave
			for(uint32_t i = 0; i < mNumWaves; i++)
			{
				if (mWaves[i]->mapPointXZToGridPos(point, gridPos))
				{
					mDisturbance = nv::vec4f(point.x, point.z, mSettings.Size, mSettings.Strength);
					mTouchDown = true;
				}
			}
		}
		else if (action == NvPointerActionType::MOTION && mTouchDown)
		{
			nv::vec3f point = unProject(points[0].m_x, points[0].m_y);
			nv::vec2f gridPos;

			// project onto each wave
			for(uint32_t i = 0; i < mNumWaves; i++)
			{
				if (mWaves[i]->mapPointXZToGridPos(point, gridPos))
				{
					mDisturbance = nv::vec4f(point.x, point.z, mSettings.Size, mSettings.Strength);
				}
			}
		}
	}
	return mTouchDown;
}

bool ComputeWaterSimulation::handleGamepadButtonChanged(uint32_t button, bool down)
{
	if (!NvSampleApp::handleGamepadButtonChanged(button, down))
    {
        // note that this is dangerous if it hooks a button that the tweakbar
        // wants to use when open, which we don't ever 'know' about unfortunately.
        if (button==NvGamepad::BUTTON_X)
        {
    		mJoystickDown = down;
    	    return true; // we'll handle X since UI never takes that.
        }
    }
	return false;
}
