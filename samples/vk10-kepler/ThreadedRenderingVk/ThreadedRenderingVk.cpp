//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/ThreadedRenderingVk.cpp
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
#include "ThreadedRenderingVk.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputHandler_CameraFly.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvModel/NvModelExt.h"
#include "NvVkUtil/NvVkContext.h"
#include "NvVkUtil/NvGPUTimerVK.h"
#include "NvVkUtil/NvModelExtVk.h"
#include "NvVkUtil/NvQuadVK.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NvAssert.h"

#include <NvUI/NvBitFont.h>

#include <NvGLUtils/NvGLSLProgram.h>
#include <NvGLUtils/NvImageGL.h>
#include <NvGLUtils/NvShapesGL.h>
#include <NvAppBase/gl/NvAppContextGL.h>

#include <stdint.h>

#include <ctime>
#define ARRAY_SIZE(a) ( sizeof(a) / sizeof( (a)[0] ))

#define NV_UNUSED( variable ) ( void )( variable )

// Currently the number of instances rendered of each model
#define MAX_INSTANCE_COUNT 100
#define INSTANCE_COUNT 100

#ifdef ANDROID
#define MAX_SCHOOL_COUNT 2000
#else
#define MAX_SCHOOL_COUNT 5000
#endif

#define SCHOOL_COUNT 50 // one of each fish model

// Static section for thread data structures

uint32_t s_threadMask = 0;

extern uint32_t neighborOffset;
extern uint32_t neighborSkip;


#ifdef WIN32
DWORD WINAPI AnimateJobFunctionThunk(VOID *arg)
#else
void* AnimateJobFunctionThunk(void *arg)
#endif
{
	ThreadedRenderingVk::ThreadData* data = (ThreadedRenderingVk::ThreadData*)arg;
	data->m_app->AnimateJobFunction(data->m_index);

	return 0;
}


void ThreadedRenderingVk::AnimateJobFunction(uint32_t threadIndex)
{
	NvThreadManager* threadManager = getThreadManagerInstance();
	NV_ASSERT(nullptr != threadManager);

	while (m_running) {
		m_FrameStartLock->lockMutex();
		{
			m_FrameStartCV->waitConditionVariable(
				m_FrameStartLock);

			if (!m_running) {
				m_FrameStartLock->unlockMutex();
				break;
			}
			if (threadIndex >= m_activeThreads)	{
				m_FrameStartLock->unlockMutex();
				continue;
			}
		}
		m_FrameStartLock->unlockMutex();
		uint32_t schoolsDone = 0;

		{
			VkResult result;
			CPU_TIMER_SCOPE(CPU_TIMER_THREAD_BASE_TOTAL + threadIndex);
			s_threadMask |= 1 << threadIndex;

			ThreadData& me = m_Threads[threadIndex];

			schoolsDone = me.m_schoolCount;
			ThreadData& thread = m_Threads[threadIndex];
			uint32_t schoolMax = me.m_baseSchoolIndex + schoolsDone;

			if (m_drawGL) {
				if (!m_animPaused)
				{
					for (uint32_t i = me.m_baseSchoolIndex; i < schoolMax; i++)
					{
						UpdateSchool(threadIndex, i, m_schools[i]);
					}
				}
			}
			else {
				VkCommandBuffer cmd = m_subCommandBuffers[threadIndex + 1];

				for (uint32_t i = me.m_baseSchoolIndex; i < schoolMax; i++) {
					{
						if (!m_animPaused)
							UpdateSchool(threadIndex, i, m_schools[i]);
					}

					{
						CPU_TIMER_SCOPE(CPU_TIMER_THREAD_BASE_CMD_BUILD + threadIndex);
						if (m_threadedRendering) {
							thread.m_drawCallCount += DrawSchool(cmd, m_schools[i], !thread.m_cmdBufferOpen);
							thread.m_cmdBufferOpen = true;
						}
					}
				}

				if (thread.m_cmdBufferOpen) {
					result = vkEndCommandBuffer(cmd);
					CHECK_VK_RESULT();
				}
			}

			{
				m_DoneCountLock->lockMutex();
				{
					m_doneCount += schoolsDone;
					m_DoneCountCV->signalConditionVariable();
				}
				m_DoneCountLock->unlockMutex();
			}
		}
	}

	LOGI("Thread %d Exit.\n", threadIndex);
}

// The sample has a dropdown menu in the TweakBar that allows the user to
// select a specific stage or pass of the algorithm for visualization.
// These are the options in that menu (for more details, please see the
// sample documentation)
static NvTweakEnum<uint32_t> RENDER_OPTIONS[] =
{
	{ "Color only", ThreadedRenderingVk::RENDER_COLOR },
	{ "Depth only", ThreadedRenderingVk::RENDER_DEPTH },
	{ "Gather (final result)", ThreadedRenderingVk::RENDER_FINAL }
};


// The sample has a dropdown menu in the TweakBar that allows the user to
// reset the schools of fish and the camera to their starting position and
// state. These are the options in that menu 
static NvTweakEnum<uint32_t> RESET_OPTIONS[] =
{
	{ "Fish Fireworks", ThreadedRenderingVk::RESET_RANDOM },
	{ "Fishsplosion", ThreadedRenderingVk::RESET_ORIGIN }
};

class ThreadedRenderingModelLoader : public Nv::NvModelFileLoader
{
public:
	ThreadedRenderingModelLoader() {}
	virtual ~ThreadedRenderingModelLoader() {}
	virtual char* LoadDataFromFile(const char* fileName)
	{
		int32_t length;
		return NvAssetLoaderRead(fileName, length);
	}

	virtual void ReleaseData(char* pData)
	{
		NvAssetLoaderFree(pData);
	}
};
//-----------------------------------------------------------------------------
// PUBLIC METHODS, CTOR AND DTOR
nv::vec3f ThreadedRenderingVk::ms_tankMin(-30.0f, 5.0f, -30.0f);
nv::vec3f ThreadedRenderingVk::ms_tankMax(30.0f, 25.0f, 30.0f);

ThreadedRenderingVk::ThreadedRenderingVk() :
	m_bFollowingSchool(false),
	m_FBOCreated(false),
    m_bUseFixedSchools(false),
	m_activeSchools(0),
    m_causticTiling(0.1f),
    m_causticSpeed(0.3f),
	m_renderMode(RENDER_FINAL),
	m_uiResetMode(RESET_RANDOM),
	m_queueMutexPtr(nullptr),
	m_startingCameraPitchYaw(0.0f, 0.0f),
	m_maxSchools(MAX_SCHOOL_COUNT),
	m_schoolStateMgr(m_maxSchools),
	m_animPaused(false),
	m_avoidance(true),
	m_currentTime(0.0f),
	m_frameLogicalClock(0),
	m_bUIDirty(true),
    m_uiSchoolCount(0),
    m_uiFishCount(0),
    m_uiTankSize(30),
    m_bTankSizeChanged(false),
	m_uiThreadCount(0), 
    m_requestedActiveThreads(MAX_ANIMATION_THREAD_COUNT),
    m_activeThreads(0),
    m_requestedThreadedRendering(true),
	m_threadedRendering(true),
	m_uiInstanceCount(INSTANCE_COUNT),
	m_uiBatchSize(m_uiInstanceCount),
	m_uiSchoolInfoId(0),
    m_uiCameraFollow(false),
	m_uiSchoolDisplayModelIndex(0),
	m_running(true),
	m_statsCountdown(STATS_FRAMES),
    m_statsMode(STATS_SIMPLE),
    m_pStatsModeVar(nullptr),
    m_bDisplayLogos(true),
    m_logoNVIDIA(nullptr),
    m_logoVulkan(nullptr),
    m_logoGLES(nullptr), 
    m_drawCallCount(0),
	m_flushPerFrame(false),
    m_requestedDrawGL(false),
	m_drawGL(false),
	m_glInitialized(false)
{
    ms_tankMax.x = ms_tankMax.z = (float)m_uiTankSize;
    ms_tankMin.x = ms_tankMin.z = -ms_tankMax.x;
    m_startingCameraPosition = (ms_tankMin + ms_tankMax) * 0.5f;
	m_startingCameraPosition.z += 40.0f;

	m_pInputHandler = new NvInputHandler_CameraFly();
	SetInputHandler(m_pInputHandler);
	m_pInputHandler->setPosition(m_startingCameraPosition);
	m_pInputHandler->setYaw(m_startingCameraPitchYaw.y);
	m_pInputHandler->setPitch(m_startingCameraPitchYaw.x);
#ifdef ANDROID
    m_pInputHandler->setMouseRotationSpeed(0.001f);  // Touch screen should be a bit less sensitive than mouse
#else
    m_pInputHandler->setMouseRotationSpeed(0.002f);
#endif
	m_pInputHandler->setMouseTranslationSpeed(0.03f);
	m_pInputHandler->setGamepadRotationSpeed(1.0f);
	m_pInputHandler->setGamepadTranslationSpeed(4.0f);
	m_pInputHandler->setKeyboardRotationSpeed(1.0f);
	m_pInputHandler->setKeyboardTranslationSpeed(4.0f);

	for (uint32_t i = 0; i < MODEL_COUNT; i++)
	{
		m_models[i] = nullptr;
	}

	m_FrameStartLock = NULL;
	m_FrameStartCV = NULL;
	m_NeedsUpdateQueueLock = NULL;
	m_DoneCountLock = NULL;
	m_DoneCountCV = NULL;

	bool m_running = true;

	for (uint32_t i = 0; i < MAX_ANIMATION_THREAD_COUNT; i++)
	{
		m_Threads[i].m_thread = NULL;
		m_Threads[i].m_app = this;
		m_Threads[i].m_index = i;
	}

	InitializeSchoolDescriptions(50);

	// Required in all subclasses to avoid silent link issues
	forceLinkHack();

	m_glSupported = true;

	m_initSchools = SCHOOL_COUNT;

	{
		const std::vector<std::string>& cmd = getCommandLine();
		std::vector<std::string>::const_iterator iter = cmd.begin();
		for (std::vector<std::string>::const_iterator iter = cmd.begin(); iter != cmd.end(); ++iter)
		{
			if (*iter == "-idle")
				m_flushPerFrame = true;
			else if (*iter == "-nogl")
				m_glSupported = false;
			else if (*iter == "-schools") {
				iter++;
				m_initSchools = atoi(iter->c_str());
			}
		}
	}

	m_initSchools = (m_initSchools > MAX_SCHOOL_COUNT) ? MAX_SCHOOL_COUNT : m_initSchools;

	if (m_glSupported)
		setGLDrawCallbacks(this);
}

ThreadedRenderingVk::~ThreadedRenderingVk()
{
	LOGI("ThreadedRenderingVk: destroyed\n");
	if (getAppContext() && device())
	    vkDeviceWaitIdle(device());

	CleanThreads();
	CleanRendering();
}

// Inherited methods
ThreadedRenderingModelLoader loader;

void ThreadedRenderingVk::initRendering(void)
{
	for (int32_t i = 0; i < CPU_TIMER_COUNT; ++i)
	{
		m_CPUTimers[i].init();
	}

	mFramerate->setReportFrames(20);

	VkResult result;
	NvAssetLoaderAddSearchPath("vk10-kepler/ThreadedRenderingVk");
	Nv::NvModelExt::SetFileLoader(&loader);

	if (!isGLSupported()) {
		showDialog("GL/VK Interop Not Supported", "NVIDIA GL/VK interop extension not supported\n"
			"You will not be able to switch between GL and VK\n"
			"at runtime.  Only VK will be available.", false);
		m_glSupported = false;
	}

	NV_ASSERT(nullptr == m_queueMutexPtr);
	m_queueMutexPtr = getThreadManagerInstance()->initializeMutex(true,
		NvMutex::MutexLockLevelInitial);
	NV_ASSERT(nullptr != m_queueMutexPtr);

	// Load all shaders
	VkPipelineShaderStageCreateInfo fishShaderStages[2];
	uint32_t shaderCount = 0;
#ifdef SOURCE_SHADERS
	shaderCount = vk().createShadersFromSourceFile(
		NvAssetLoadTextFile("src_shaders/staticfish.glsl"), fishShaderStages, 2);
#else
	{
		int32_t length;
		char* data = NvAssetLoaderRead("shaders/staticfish.nvs", length);
		shaderCount = vk().createShadersFromBinaryFile((uint32_t*)data,
			length, fishShaderStages, 2);
	}
#endif

	if (shaderCount == 0) {
		showDialog("Fatal: Cannot Find Assets", "The shader assets cannot be loaded.\n"
			"Please ensure that the assets directory for the sample is in place\n"
			"and has not been moved.  Exiting.", true);
		return;
	}

	VkPipelineShaderStageCreateInfo skyShaderStages[2];
#ifdef SOURCE_SHADERS
	shaderCount = vk().createShadersFromSourceFile(
		NvAssetLoadTextFile("src_shaders/skyboxcolor.glsl"), skyShaderStages, 2);
#else
	{
		int32_t length;
		char* data = NvAssetLoaderRead("shaders/skyboxcolor.nvs", length);
		shaderCount = vk().createShadersFromBinaryFile((uint32_t*)data,
			length, skyShaderStages, 2);
	}
#endif

	if (shaderCount == 0) {
		showDialog("Fatal: Cannot Find Assets", "The shader assets cannot be loaded.\n"
			"Please ensure that the assets directory for the sample is in place\n"
			"and has not been moved.  Exiting.", true);
		return;
	}

	VkPipelineShaderStageCreateInfo groundShaderStages[2];
#ifdef SOURCE_SHADERS
	shaderCount = vk().createShadersFromSourceFile(
		NvAssetLoadTextFile("src_shaders/groundplane.glsl"), groundShaderStages, 2);
#else
	{
		int32_t length;
		char* data = NvAssetLoaderRead("shaders/groundplane.nvs", length);
		shaderCount = vk().createShadersFromBinaryFile((uint32_t*)data,
			length, groundShaderStages, 2);
	}
#endif

	if (shaderCount == 0) {
		showDialog("Fatal: Cannot Find Assets", "The shader assets cannot be loaded.\n"
			"Please ensure that the assets directory for the sample is in place\n"
			"and has not been moved.  Exiting.", true);
		return;
	}

	// Create the standard samplers that will be used
	InitializeSamplers();

	for (uint32_t i = 0; i < MODEL_COUNT; ++i)
	{
		Nv::NvModelExt* pModel =
			Nv::NvModelExt::CreateFromPreprocessed(ms_modelInfo[i].m_filename);
		if (nullptr != pModel)
		{

			Nv::NvModelExtVK* pModelVK = m_models[i] =
				Nv::NvModelExtVK::Create(vk(), pModel);
			if (nullptr == pModelVK)
			{
				continue;
			}

			// Either Assimp or our art is completely broken, so bone
			// transforms are garbage. 
			// Use the static shader and ignore skin until we can untangle it
			ModelDesc& desc = ms_modelInfo[i];
			desc.m_center = Rotate(desc.m_fixupXfm, pModelVK->GetCenter());
			desc.m_halfExtents = Rotate(desc.m_fixupXfm, pModelVK->GetMaxExt() - ms_modelInfo[i].m_center);
		}
	}

	// Load the skybox
	if (!vk().uploadTextureFromDDSFile("textures/sand.dds", m_skyboxSandTex))
	{
		LOGE("ThreadedRenderingVk: Error - Unable to load textures/sand.dds")
	}

	if (!vk().uploadTextureFromDDSFile("textures/Gradient.dds", m_skyboxGradientTex))
	{
		LOGE("ThreadedRenderingVk: Error - Unable to load textures/Gradient.dds")
	}

	if (!vk().uploadTextureFromDDSFile("textures/caustic1.dds", m_caustic1Tex))
    {
        LOGE("ThreadedRenderingVk: Error - Unable to load textures/Caustic1.dds")
    }

	if (!vk().uploadTextureFromDDSFile("textures/caustic2.dds", m_caustic2Tex))
    {
        LOGE("ThreadedRenderingVk: Error - Unable to load textures/Caustic2.dds")
    }

	// Assign some values which apply to the entire scene and update once per frame.
	m_lightingUBO.Initialize(vk());
	m_lightingUBO->m_lightPosition = nv::vec4f(1.0f, 1.0f, 1.0f, 0.0f);
    m_lightingUBO->m_lightAmbient = nv::vec4f(0.4f, 0.4f, 0.4f, 1.0f);
    m_lightingUBO->m_lightDiffuse = nv::vec4f(0.7f, 0.7f, 0.7f, 1.0f);
    m_lightingUBO->m_causticOffset = m_currentTime * m_causticSpeed;
    m_lightingUBO->m_causticTiling = m_causticTiling;
	m_lightingUBO.Update();

	m_projUBO.Initialize(vk());

	VkPipelineDepthStencilStateCreateInfo depthTestAndWrite;

	depthTestAndWrite = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthTestAndWrite.depthTestEnable = VK_TRUE;
	depthTestAndWrite.depthWriteEnable = VK_TRUE;
	depthTestAndWrite.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthTestAndWrite.depthBoundsTestEnable = VK_FALSE;
	depthTestAndWrite.stencilTestEnable = VK_FALSE;
	depthTestAndWrite.minDepthBounds = 0.0f;
	depthTestAndWrite.maxDepthBounds = 1.0f;

	VkPipelineDepthStencilStateCreateInfo depthTestNoWrite;
	depthTestNoWrite = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthTestNoWrite.depthTestEnable = VK_TRUE;
	depthTestNoWrite.depthWriteEnable = VK_FALSE;
	depthTestNoWrite.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthTestNoWrite.depthBoundsTestEnable = VK_FALSE;
	depthTestNoWrite.stencilTestEnable = VK_FALSE;
	depthTestNoWrite.minDepthBounds = 0.0f;
	depthTestNoWrite.maxDepthBounds = 1.0f;

	VkPipelineColorBlendAttachmentState colorStateBlend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorStateBlend.colorWriteMask = ~0;
	colorStateBlend.blendEnable = VK_TRUE;
	colorStateBlend.alphaBlendOp = VK_BLEND_OP_ADD;
	colorStateBlend.colorBlendOp = VK_BLEND_OP_ADD;
	colorStateBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorStateBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorStateBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorStateBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	VkPipelineColorBlendStateCreateInfo colorInfoBlend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorInfoBlend.logicOpEnable = VK_FALSE;
	colorInfoBlend.attachmentCount = 1;
	colorInfoBlend.pAttachments = &colorStateBlend;
	colorInfoBlend.blendConstants[0] = 1.0f;
	colorInfoBlend.blendConstants[1] = 1.0f;
	colorInfoBlend.blendConstants[2] = 1.0f;
	colorInfoBlend.blendConstants[3] = 1.0f;

	VkPipelineColorBlendAttachmentState colorStateNoBlend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorStateNoBlend.colorWriteMask = ~0;
	colorStateNoBlend.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorInfoNoBlend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorInfoNoBlend.logicOpEnable = VK_FALSE;
	colorInfoNoBlend.attachmentCount = 1;
	colorInfoNoBlend.pAttachments = &colorStateNoBlend;

	m_quad = NvQuadVK::Create(vk());

	VkDescriptorSetLayoutBinding binding[DESC_COUNT];
	for (uint32_t i = 0; i < DESC_COUNT; i++) {
		binding[i].binding = i;
		binding[i].descriptorCount = 1;
		binding[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		binding[i].descriptorType = i < DESC_FIRST_TEX
			? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
			: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding[i].pImmutableSamplers = NULL;
	}

	{
		VkDescriptorSetLayoutCreateInfo descriptorSetEntry = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorSetEntry.bindingCount = DESC_COUNT;
		descriptorSetEntry.pBindings = binding;

		result = vkCreateDescriptorSetLayout(device(), &descriptorSetEntry, 0, mDescriptorSetLayout);
		CHECK_VK_RESULT();
	}

	// Create descriptor region and set
	VkDescriptorPoolSize descriptorPoolInfo[2];

	descriptorPoolInfo[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolInfo[0].descriptorCount = DESC_COUNT;
	descriptorPoolInfo[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolInfo[1].descriptorCount = DESC_COUNT;

	VkDescriptorPoolCreateInfo descriptorRegionInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorRegionInfo.maxSets = DESC_COUNT;
	descriptorRegionInfo.poolSizeCount = 2;
	descriptorRegionInfo.pPoolSizes = descriptorPoolInfo;
	VkDescriptorPool descriptorRegion;
	result = vkCreateDescriptorPool(device(), &descriptorRegionInfo, NULL, &descriptorRegion);
	CHECK_VK_RESULT();

	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptorSetAllocateInfo.descriptorPool = descriptorRegion;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = mDescriptorSetLayout;
		result = vkAllocateDescriptorSets(device(), &descriptorSetAllocateInfo, &mDescriptorSet);
		CHECK_VK_RESULT();
	}

	VkWriteDescriptorSet writeDescriptorSets[DESC_COUNT];

	for (uint32_t i = 0; i < DESC_COUNT; i++) {
		writeDescriptorSets[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writeDescriptorSets[i].dstSet = mDescriptorSet;
		writeDescriptorSets[i].dstBinding = i;
		writeDescriptorSets[i].dstArrayElement = 0;
		writeDescriptorSets[i].descriptorCount = 1;
		writeDescriptorSets[i].descriptorType = i < DESC_FIRST_TEX
			? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
			: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}

	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = mDescriptorSetLayout;
		result = vkCreatePipelineLayout(device(), &pipelineLayoutCreateInfo, 0, &m_pipeLayout);
		CHECK_VK_RESULT();
	}

	VkDescriptorSetLayoutBinding fishBinding[3];
	fishBinding[0].binding = 0;
	fishBinding[0].descriptorCount = 1;
	fishBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	fishBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	fishBinding[0].pImmutableSamplers = NULL;
	fishBinding[1].binding = 1;
	fishBinding[1].descriptorCount = 1;
	fishBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	fishBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	fishBinding[1].pImmutableSamplers = NULL;
	fishBinding[2].binding = 2;
	fishBinding[2].descriptorCount = 1;
	fishBinding[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	fishBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	fishBinding[2].pImmutableSamplers = NULL;

	{
		VkDescriptorSetLayoutCreateInfo descriptorSetEntry = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorSetEntry.bindingCount = 3;
		descriptorSetEntry.pBindings = fishBinding;

		result = vkCreateDescriptorSetLayout(device(), &descriptorSetEntry, 0, &mDescriptorSetLayout[1]);
		CHECK_VK_RESULT();
	}

	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutCreateInfo.setLayoutCount = 2;
		pipelineLayoutCreateInfo.pSetLayouts = mDescriptorSetLayout;
		result = vkCreatePipelineLayout(device(), &pipelineLayoutCreateInfo, 0, &m_fishPipeLayout);
		CHECK_VK_RESULT();
	}

	{
		// Create fish descriptor region and set
		VkDescriptorPoolSize fishDescriptorPoolInfo[2];

		fishDescriptorPoolInfo[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		fishDescriptorPoolInfo[0].descriptorCount = MAX_SCHOOL_COUNT;
		fishDescriptorPoolInfo[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		fishDescriptorPoolInfo[1].descriptorCount = 2 * MAX_SCHOOL_COUNT;

		VkDescriptorPoolCreateInfo descriptorRegionInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		descriptorRegionInfo.maxSets = 3 * MAX_SCHOOL_COUNT;
		descriptorRegionInfo.poolSizeCount = 2;
		descriptorRegionInfo.pPoolSizes = fishDescriptorPoolInfo;
		result = vkCreateDescriptorPool(device(), &descriptorRegionInfo, NULL, &mDescriptorRegion);
		CHECK_VK_RESULT();
	}

	m_fishAlloc.Initialize(vk(), MAX_SCHOOL_COUNT * (
		NvSimpleUBO<School::SchoolUBO>::GetBufferSpace() +
		NvSimpleUBO<nv::matrix4f>::GetBufferSpace()));

	m_vboAlloc.Initialize(vk(), 4 * MAX_SCHOOL_COUNT * MAX_INSTANCE_COUNT *
		sizeof(School::FishInstanceData));

	SetNumSchools(m_initSchools);
	vkQueueWaitIdle(queue());

	VkDescriptorBufferInfo projDesc;
	m_projUBO.GetDesc(projDesc);
	writeDescriptorSets[DESC_PROJ_UBO].pBufferInfo = &projDesc;

	VkDescriptorBufferInfo lightDesc;
	m_lightingUBO.GetDesc(lightDesc);
	writeDescriptorSets[DESC_LIGHT_UBO].pBufferInfo = &lightDesc;

	VkDescriptorImageInfo caus1Desc = {};
	caus1Desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	caus1Desc.imageView = m_caustic1Tex.view;
	caus1Desc.sampler = m_wrapSampler;
	writeDescriptorSets[DESC_CAUS1_TEX].pImageInfo = &caus1Desc;

	VkDescriptorImageInfo caus2Desc = {};
	caus2Desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	caus2Desc.imageView = m_caustic2Tex.view;
	caus2Desc.sampler = m_wrapSampler;
	writeDescriptorSets[DESC_CAUS2_TEX].pImageInfo = &caus2Desc;

	VkDescriptorImageInfo sandDesc = {};
	sandDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sandDesc.imageView = m_skyboxSandTex.view;
	sandDesc.sampler = m_wrapSampler;
	writeDescriptorSets[DESC_SAND_TEX].pImageInfo = &sandDesc;

	VkDescriptorImageInfo gradDesc = {};
	gradDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	gradDesc.imageView = m_skyboxGradientTex.view;
	gradDesc.sampler = m_linearSampler;
	writeDescriptorSets[DESC_GRAD_TEX].pImageInfo = &gradDesc;

	vkUpdateDescriptorSets(device(), DESC_COUNT, writeDescriptorSets, 0, 0);

	InitPipeline(2, fishShaderStages, 
		&m_schools[0]->getModel()->GetModel()->GetMesh(0)->getVIInfo(),
		&m_schools[0]->getModel()->GetModel()->GetMesh(0)->getIAInfo(),
		&colorInfoBlend, &depthTestAndWrite, m_fishPipeLayout, &m_fishPipe);

	InitPipeline(2, groundShaderStages, &m_quad->getVIInfo(), &m_quad->getIAInfo(),
		&colorInfoNoBlend, &depthTestNoWrite, m_pipeLayout, &m_groundPlanePipe);

	InitPipeline(2, skyShaderStages, &m_quad->getVIInfo(), &m_quad->getIAInfo(),
		&colorInfoNoBlend, &depthTestNoWrite, m_pipeLayout, &m_skyboxColorPipe);

	ResetFish(false);

	InitThreads();

	if (isTestMode()) {
		for (uint32_t j = 0; j < 120; j++) {

			for (uint32_t i = 0; i < m_activeSchools; i++) {
				m_schools[i]->SetInstanceCount(m_uiInstanceCount);
				m_schools[i]->Animate(1.0f / 60.0f, &m_schoolStateMgr, m_avoidance);
				m_schools[i]->Update();
			}
		}
		m_animPaused = true;
	}
}

void ThreadedRenderingVk::initGLResources(NvAppContextGL* gl) {

	NvGPUTimer::globalInit(*gl);

	Nv::NvGLInstancingSupport::glDrawElementsInstancedInternal =
		(Nv::NvGLInstancingSupport::PFNDrawElementsInstanced)gl->getGLProcAddress("glDrawElementsInstanced");
	Nv::NvGLInstancingSupport::glVertexAttribDivisorInternal =
		(Nv::NvGLInstancingSupport::PFNVertexAttribDivisor)gl->getGLProcAddress("glVertexAttribDivisor");

	getAppContext()->setSwapInterval(0);

	m_GPUTimer.init();

	// Load all shaders
	m_shader_GroundPlane = NvGLSLProgram::createFromFiles("src_shaders/gl/groundplane_VS.glsl", "src_shaders/gl/groundplane_FS.glsl");
	m_shader_Skybox = NvGLSLProgram::createFromFiles("src_shaders/gl/skyboxcolor_VS.glsl", "src_shaders/gl/skyboxcolor_FS.glsl");
	m_shader_Fish = NvGLSLProgram::createFromFiles("src_shaders/gl/staticfish_VS.glsl", "src_shaders/gl/staticfish_FS.glsl");

	if ((nullptr == m_shader_GroundPlane) ||
		(nullptr == m_shader_Skybox) ||
		(nullptr == m_shader_Fish))
	{
		showDialog("Fatal: Cannot Find Assets", "The shader assets cannot be loaded.\n"
			"Please ensure that the assets directory for the sample is in place\n"
			"and has not been moved.  Exiting.", true);
		return;
	}

	for (uint32_t i = 0; i < MODEL_COUNT; ++i)
	{
		Nv::NvModelExt* pModel =
			Nv::NvModelExt::CreateFromPreprocessed(ms_modelInfo[i].m_filename);
		if (nullptr != pModel)
		{

			Nv::NvModelExtGL* pModelGL = m_modelsGL[i] =
				Nv::NvModelExtGL::Create(pModel);
			if (nullptr == pModelGL)
			{
				continue;
			}
			pModelGL->SetDiffuseTextureLocation(0);
			pModelGL->UpdateBoneTransforms();

			// Either Assimp or our art is completely broken, so bone
			// transforms are garbage. 
			// Use the static shader and ignore skin until we can untangle it
			ModelDesc& desc = ms_modelInfo[i];
			desc.m_center = Rotate(desc.m_fixupXfm, pModelGL->GetCenter());
			desc.m_halfExtents = Rotate(desc.m_fixupXfm, pModelGL->GetMaxExt() - ms_modelInfo[i].m_center);
		}
	}

	// Create shared uniform buffers
	m_projUBO_Location = 0; // Fixed in shaders
	glGenBuffers(1, &m_projUBO_Id);
	glBindBuffer(GL_UNIFORM_BUFFER, m_projUBO_Id);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjUBO), &m_projUBO_Data, GL_STREAM_DRAW);

	m_lightingUBO_Location = 1; // Fixed in shaders
	glGenBuffers(1, &m_lightingUBO_Id);
	glBindBuffer(GL_UNIFORM_BUFFER, m_lightingUBO_Id);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightingUBO), &m_lightingUBO_Data, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Load the skybox
	m_skyboxSandTexGL = NvImageGL::UploadTextureFromDDSFile("textures/sand.dds");
	m_skyboxGradientTexGL = NvImageGL::UploadTextureFromDDSFile("textures/Gradient.dds");
	m_caustic1TexGL = NvImageGL::UploadTextureFromDDSFile("textures/caustic1.dds");
	m_caustic2TexGL = NvImageGL::UploadTextureFromDDSFile("textures/caustic2.dds");

	// Assign some values which apply to the entire scene and update once per frame.
	m_lightingUBO_Data.m_lightPosition = nv::vec4f(1.0f, 1.0f, 1.0f, 0.0f);
	m_lightingUBO_Data.m_lightAmbient = nv::vec4f(0.4f, 0.4f, 0.4f, 1.0f);
	m_lightingUBO_Data.m_lightDiffuse = nv::vec4f(0.7f, 0.7f, 0.7f, 1.0f);
	m_lightingUBO_Data.m_causticOffset = m_currentTime * m_causticSpeed;
	m_lightingUBO_Data.m_causticTiling = m_causticTiling;

	for (uint32_t i = 0; i < m_schools.size(); ++i) {
		SchoolDesc& desc = m_schoolDescs[i];
		m_schools[i]->InitializeGL(
			m_modelsGL[desc.m_modelId],
			2, 4);
	}

    // Initially create our logos and add them to the window, but hidden in the upper corner.  
    // Let the reshape call fix up the size and location
    m_logoNVIDIA = InitLogoTexture("textures/LogoNVIDIA.dds");
    m_logoGLES = InitLogoTexture("textures/LogoGLES.dds");
    m_logoVulkan = InitLogoTexture("textures/LogoVulkan.dds");
    UpdateLogos();

	m_glInitialized = true;
}

void ThreadedRenderingVk::drawGL(NvAppContextGL* gl) {
    if (m_requestedDrawGL != m_drawGL)
    {
        m_drawGL = m_requestedDrawGL;
    }

	if(!m_drawGL)
		return;
    // Update the active number of threads
    if (m_requestedActiveThreads != m_activeThreads)
    {
        m_activeThreads = m_requestedActiveThreads;
        m_bUIDirty = true;
        UpdateUI();
    }
    if (m_requestedThreadedRendering != m_threadedRendering)
    {
        m_threadedRendering = m_requestedThreadedRendering;
    }

	neighborOffset = (neighborOffset + 1) % (6 - neighborSkip);
	s_threadMask = 0;

    if (m_bTankSizeChanged)
    {
        UpdateSchoolTankSizes();
    }

	m_currentTime += getClampedFrameTime();
	m_schoolStateMgr.BeginFrame(m_activeSchools);
#if FISH_DEBUG
	LOGI("\n################################################################");
	LOGI("BEGINNING OF FRAME");
#endif
	// Update the camera position if we are following a school
	if (m_uiCameraFollow)
	{
		if (m_uiSchoolInfoId < m_activeSchools)
		{
			// Get the centroid of the school we're following
			School* pSchool = m_schools[m_uiSchoolInfoId];
			nv::vec3f camPos = pSchool->GetCentroid() - (m_pInputHandler->getLookVector() * pSchool->GetRadius() * 4);
			if (camPos.y < 0.01f)
			{
				camPos.y = 0.01f;
			}
			m_pInputHandler->setPosition(camPos);
		}
		else
		{
			m_uiCameraFollow = false;
			m_bUIDirty = true;
			UpdateUI();
		}
	}

	// Get the current view matrix (according to user input through mouse,
	// gamepad, etc.)
	m_projUBO_Data.m_viewMatrix = m_pInputHandler->getViewMatrix();
	m_projUBO_Data.m_inverseViewMatrix = m_pInputHandler->getCameraMatrix();
	glBindBuffer(GL_UNIFORM_BUFFER, m_projUBO_Id);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjUBO), &m_projUBO_Data, GL_STREAM_DRAW);

	m_lightingUBO_Data.m_causticOffset = m_currentTime * m_causticSpeed;
	m_lightingUBO_Data.m_causticTiling = m_causticTiling;
	glBindBuffer(GL_UNIFORM_BUFFER, m_lightingUBO_Id);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightingUBO), &m_lightingUBO_Data, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Render the requested content (from dropdown menu in TweakBar UI)

	{
		CPU_TIMER_SCOPE(CPU_TIMER_MAIN_WAIT);

		for (uint32_t i = 0; i < MAX_ANIMATION_THREAD_COUNT; i++) {
			ThreadData& thread = m_Threads[i];
			thread.m_cmdBufferOpen = false;
			thread.m_drawCallCount = 0;
		}

		uint32_t schoolsPerThread = m_activeSchools / m_activeThreads;
		uint32_t remainderSchools = m_activeSchools % m_activeThreads;
		uint32_t baseSchool = 0;
		for (uint32_t i = 0; i < m_activeThreads; i++) {
			ThreadData& thread = m_Threads[i];
			thread.m_baseSchoolIndex = baseSchool;
			thread.m_schoolCount = schoolsPerThread;
			// distribute the remainder evenly
			if (remainderSchools > 0) {
				thread.m_schoolCount++;
				remainderSchools--;
			}

			baseSchool += thread.m_schoolCount;
		}

		m_doneCount = 0;
		m_drawCallCount = 0;

		m_FrameStartLock->lockMutex();
		{
			m_FrameStartCV->broadcastConditionVariable();
		}
		m_FrameStartLock->unlockMutex();

		m_DoneCountLock->lockMutex();
		{
			while (m_doneCount != m_activeSchools)
			{
				m_DoneCountCV->waitConditionVariable(m_DoneCountLock);
			}
		}
		m_DoneCountLock->unlockMutex();
	}

	// Rendering
	{
		{
			CPU_TIMER_SCOPE(CPU_TIMER_MAIN_CMD_BUILD);
			GPU_TIMER_SCOPE();
			glClear(GL_DEPTH_BUFFER_BIT);
			DrawSkyboxColorDepthGL();
			DrawGroundPlaneGL();
		}

		uint32_t schoolIndex = 0;
		// Cheating somewhat here, since we know that all fish will use the same shader to render
		m_shader_Fish->enable();
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_caustic1TexGL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, m_caustic2TexGL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for (; schoolIndex < m_activeSchools; ++schoolIndex)
		{
			School* pSchool = m_schools[schoolIndex];
			{
				CPU_TIMER_SCOPE(CPU_TIMER_MAIN_COPYVBO);
				pSchool->Update(true);
			}
			{
				CPU_TIMER_SCOPE(CPU_TIMER_MAIN_CMD_BUILD);
				GPU_TIMER_SCOPE();
				m_drawCallCount += pSchool->RenderGL(m_uiBatchSize);
			}
		}
		m_shader_Fish->disable();
	}

#if FISH_DEBUG
	LOGI("END OF FRAME");
	LOGI("################################################################\n");
#endif
    UpdateStats();

	m_frameLogicalClock++;
}


void ThreadedRenderingVk::InitPipeline(uint32_t shaderCount, 
	VkPipelineShaderStageCreateInfo* shaderStages,
	VkPipelineVertexInputStateCreateInfo* pVertexInputState,
	VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
	VkPipelineColorBlendStateCreateInfo* pBlendState,
	VkPipelineDepthStencilStateCreateInfo* pDSState,
	VkPipelineLayout& layout,
	VkPipeline* pipeline)
{
	VkResult result;
	// Create static state info for the mPipeline.

	// set dynamically
	VkPipelineViewportStateCreateInfo vpStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	vpStateInfo.pNext = 0;
	vpStateInfo.viewportCount = 1;
	vpStateInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rsStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rsStateInfo.depthClampEnable = VK_TRUE;
	rsStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rsStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rsStateInfo.cullMode = VK_CULL_MODE_NONE;
	rsStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	VkPipelineColorBlendAttachmentState attachments[1] = {};
	attachments[0].blendEnable = VK_FALSE;
	attachments[0].colorWriteMask = ~0;

	VkPipelineColorBlendStateCreateInfo cbStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	cbStateInfo.logicOpEnable = VK_FALSE;
	cbStateInfo.attachmentCount = ARRAY_SIZE(attachments);
	cbStateInfo.pAttachments = attachments;

	VkPipelineMultisampleStateCreateInfo msStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	msStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	msStateInfo.alphaToCoverageEnable = VK_FALSE;
	msStateInfo.sampleShadingEnable = VK_FALSE;
	msStateInfo.minSampleShading = 1.0f;
	uint32_t smplMask = 0x1;
	msStateInfo.pSampleMask = &smplMask;

	VkPipelineTessellationStateCreateInfo tessStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	tessStateInfo.patchControlPoints = 0;

	VkPipelineDynamicStateCreateInfo dynStateInfo;
	VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	memset(&dynStateInfo, 0, sizeof(dynStateInfo));
	dynStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynStateInfo.dynamicStateCount = 2;
	dynStateInfo.pDynamicStates = dynStates;

	// Shaders
	VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	// Assuming that all sub-meshes in an ModelExt have the same layout...
	pipelineInfo.pVertexInputState = pVertexInputState;
	pipelineInfo.pInputAssemblyState = pInputAssemblyState;
	pipelineInfo.pViewportState = &vpStateInfo;
	pipelineInfo.pRasterizationState = &rsStateInfo;
	pipelineInfo.pColorBlendState = pBlendState;
	pipelineInfo.pDepthStencilState = pDSState;
	pipelineInfo.pMultisampleState = &msStateInfo;
	pipelineInfo.pTessellationState = &tessStateInfo;
	pipelineInfo.pDynamicState = &dynStateInfo;

	pipelineInfo.stageCount = shaderCount;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
	pipelineInfo.subpass = 0;

	pipelineInfo.layout = layout;

	result = vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
		pipeline);
	CHECK_VK_RESULT();
}

void ThreadedRenderingVk::InitializeSamplers()
{
	VkResult result;

	VkSamplerCreateInfo samplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0.0;
	samplerCreateInfo.maxAnisotropy = 1;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0;
	samplerCreateInfo.maxLod = 16.0;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	result = vkCreateSampler(device(), &samplerCreateInfo, 0, &m_linearSampler);

	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

	result = vkCreateSampler(device(), &samplerCreateInfo, 0, &m_nearestSampler);

	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	result = vkCreateSampler(device(), &samplerCreateInfo, 0, &m_wrapSampler);
}

uint32_t ThreadedRenderingVk::SetNumSchools(uint32_t numSchools)
{
	VkResult result;

	if (numSchools > m_maxSchools)
    {
		numSchools = m_maxSchools;
    }
	m_activeSchools = numSchools;

	if (m_schoolDescs.size() < m_activeSchools)
	{
		InitializeSchoolDescriptions(m_activeSchools);
	}

	nv::vec3f location(0.0f, 0.0f, 0.0f);

	if (m_schools.size() < m_activeSchools)
	{
		// We need to increase the size of our array of schools and initialize the new ones
		uint32_t schoolIndex = m_schools.size();
		m_schools.resize(m_activeSchools);

		int32_t newSchools = m_activeSchools - schoolIndex;

		VkDescriptorSet* descSets = NULL;
		if (newSchools > 0) {
			for (; schoolIndex < m_schools.size(); ++schoolIndex)
			{
				SchoolDesc& desc = m_schoolDescs[schoolIndex];
				School* pSchool = new School(desc.m_flocking);
				if (m_uiResetMode == RESET_RANDOM)
				{
					nv::vec3f tankSize = ms_tankMax - ms_tankMin;
					location = nv::vec3f((float)rand() / (float)RAND_MAX * tankSize.x,
						(float)rand() / (float)RAND_MAX * tankSize.y,
						(float)rand() / (float)RAND_MAX * tankSize.z);
					location += ms_tankMin;
				}
				// Account for scaling in the transforms and extents that we are providing
				// to the school
				nv::matrix4f finalTransform;
				finalTransform.set_scale(desc.m_scale);
				finalTransform *= ms_modelInfo[desc.m_modelId].GetCenteringTransform();

				VkDescriptorSet descSet;
				{
					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
					descriptorSetAllocateInfo.descriptorPool = mDescriptorRegion;
					descriptorSetAllocateInfo.descriptorSetCount = 1;
					descriptorSetAllocateInfo.pSetLayouts = &mDescriptorSetLayout[1];
					result = vkAllocateDescriptorSets(device(), &descriptorSetAllocateInfo, &descSet);
					CHECK_VK_RESULT();
				}

				if (!pSchool->Initialize(vk(),
					schoolIndex,
					m_models[desc.m_modelId],
					ms_modelInfo[desc.m_modelId].m_tailStartZ * desc.m_scale,
					finalTransform,
					ms_modelInfo[desc.m_modelId].m_halfExtents * desc.m_scale,
					desc.m_numFish,
					MAX_INSTANCE_COUNT,
					location,
					descSet,
					m_fishPipeLayout,
					m_fishAlloc,
					m_vboAlloc)) {
					return 0;
				}

				if (m_glInitialized)
					if (!pSchool->InitializeGL(
						m_modelsGL[desc.m_modelId],
						2, 4)) {
						return 0;
					}
				m_schools[schoolIndex] = pSchool;
			}
		}
	}

    // Update our readout of total number of active fish
	m_uiFishCount = m_activeSchools * m_uiInstanceCount;
	UpdateUI();
	return m_activeSchools;
}

void ThreadedRenderingVk::InitializeSchoolDescriptions(uint32_t numSchools)
{
	uint32_t schoolIndex = m_schoolDescs.size();
	m_schoolDescs.resize(numSchools);
	for (; schoolIndex < numSchools; ++schoolIndex)
	{
		SchoolDesc& desc = m_schoolDescs[schoolIndex];
		desc = ms_schoolInfo[schoolIndex % MODEL_COUNT];
		desc.m_numFish = INSTANCE_COUNT;
	}
}

void ThreadedRenderingVk::UpdateSchoolTankSizes()
{
    SchoolSet::iterator schoolIt = m_schools.begin();
    SchoolSet::iterator schoolEnd = m_schools.end();

    for (; schoolIt != schoolEnd; ++schoolIt)
    {
        School* pSchool = (*schoolIt);
        SchoolFlockingParams params = pSchool->GetFlockingParams();
        params.m_spawnZoneMin = ms_tankMin;
        params.m_spawnZoneMax = ms_tankMax;
        pSchool->SetFlockingParams(params);

        // Poke them and force them to start heading to a new spot
        // within the new constraints of the "tank"
        pSchool->FindNewGoal();
    }
    m_bTankSizeChanged = false;
}

uint32_t ThreadedRenderingVk::SetAnimationThreadNum(uint32_t numThreads)
{
	if (MAX_ANIMATION_THREAD_COUNT < numThreads)
	{
		numThreads = MAX_ANIMATION_THREAD_COUNT;
	}
    m_requestedActiveThreads = numThreads;
    /*
    m_activeThreads = numThreads;

#if FISH_DEBUG
	LOGI("Active Animation Thread Count = %d", m_activeAnimThreads);
#endif

	UpdateUI();

	return m_activeThreads;
    */
    return m_requestedActiveThreads;
}

// The sample has a dropdown menu in the TweakBar that allows the user to
// select a mode for displaying on-screen stats
// These are the options in that menu (for more details, please see the
// sample documentation)
static NvTweakEnum<uint32_t> STATSMODE_OPTIONS[] =
{
    { "None", ThreadedRenderingVk::STATS_NONE},
    { "Simple", ThreadedRenderingVk::STATS_SIMPLE},
    { "Full", ThreadedRenderingVk::STATS_FULL }
};

void ThreadedRenderingVk::initUI(void)
{
	if (mTweakBar)
	{
		mTweakBar->setCompactLayout(true);
        mTweakBar->SetNumRows(22);

		NvTweakVarBase* var;
		mTweakBar->addLabel("Fish Settings", true);
		mTweakBar->addValue("Number of Schools", m_uiSchoolCount, MAX_ANIMATION_THREAD_COUNT, m_maxSchools, 10, UIACTION_SCHOOLCOUNT);
		mTweakBar->addValue("Fish per School", m_uiInstanceCount, 1, MAX_INSTANCE_COUNT, 1, UIACTION_INSTCOUNT);
        mTweakBar->addValue("Max Roam Distance", m_uiTankSize, 1, 60, 1, UIACTION_TANKSIZE);

		mTweakBar->addPadding();
		mTweakBar->addLabel("Rendering Settings", true);
		if (isGLSupported() && m_glSupported) {
			var = mTweakBar->addValue("Draw using GLES3 AEP", m_requestedDrawGL, UIACTION_DRAWGL);
			addTweakKeyBind(var, NvKey::K_G);
		}
        var = mTweakBar->addValue("Threaded Rendering", m_requestedThreadedRendering, 0, &m_pThreadedRenderingButton);
		addTweakKeyBind(var, NvKey::K_T);
		mTweakBar->addValue("Number of Worker Threads", m_uiThreadCount, 1, MAX_ANIMATION_THREAD_COUNT, 1, UIACTION_ANIMTHREADCOUNT);
		mTweakBar->addValue("Batch Size (Fish per Draw Call)", m_uiBatchSize, 1, MAX_INSTANCE_COUNT, 1, UIACTION_BATCHSIZE,
			&m_pBatchSlider, &m_pBatchVar);

		mTweakBar->addPadding();
		mTweakBar->addLabel("Animation Settings", true);
		var = mTweakBar->addValue("Pause Animation", m_animPaused);
		addTweakKeyBind(var, NvKey::K_P);

		var = mTweakBar->addValue("Use Avoidance", m_avoidance);
		addTweakKeyBind(var, NvKey::K_R);

		mTweakBar->addValue("Flocking Complexity", neighborSkip, 1, 5);

		mTweakBar->addLabel("Reset Schools", true);
		m_pFishFireworksVar = mTweakBar->addButton("Fish Fireworks", UIACTION_RESET_FISHFIREWORKS);
        addTweakButtonBind(m_pFishFireworksVar, NvGamepad::BUTTON_Y);
        m_pFishplosionVar = mTweakBar->addButton("Fishsplosion", UIACTION_RESET_FISHPLOSION);

		mTweakBar->addPadding();
		// mTweakBar->addLabel("On-screen Stats", true);
        m_pStatsModeVar = mTweakBar->addButton("Cycle Stats", UIACTION_STATSTOGGLE);
        addTweakKeyBind(m_pStatsModeVar, NvKey::K_C, NvKey::K_C);
        addTweakButtonBind(m_pStatsModeVar, NvGamepad::BUTTON_LEFT_SHOULDER, NvGamepad::BUTTON_LEFT_SHOULDER);
		// m_pStatsModeVar = mTweakBar->addMenu("On-screen Stats", m_statsMode, STATSMODE_OPTIONS, STATS_COUNT, UIACTION_STATSTOGGLE);

		m_uiSchoolInfoId = 0;
		SchoolDesc& desc = m_schoolDescs[m_uiSchoolInfoId];
		m_uiSchoolDisplayModelIndex = desc.m_modelId;
	}

	// UI elements for displaying triangle statistics
	if (mFPSText) {
		NvUIRect fpsRect;
		mFPSText->GetScreenRect(fpsRect); // base off of fps element.
		// pre-size the rectangle with fake text
        NvUIRect textRect;

        m_fullTimingStats = new NvUIText("______________________________----------\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",
            NvUIFontFamily::SANS, (mFPSText->GetFontSize() * 2) / 3, NvUITextAlign::LEFT);
        m_fullTimingStats->SetColor(NV_PACKED_COLOR(255, 255, 255, 255));
        m_fullTimingStats->SetShadow();
        m_fullTimingStats->GetScreenRect(textRect);
        m_fullStatsBox = new NvUIContainer(textRect.width, textRect.height, new NvUIGraphicFrame("popup_frame.dds", 24, 24));
        m_fullStatsBox->Add(m_fullTimingStats, 8.0f, 8.0f);
        mUIWindow->Add(m_fullStatsBox, fpsRect.left + fpsRect.width - textRect.width, fpsRect.top + fpsRect.height);
        m_fullStatsBox->SetVisibility(m_statsMode == STATS_FULL);

        m_simpleTimingStats = new NvUIText("__________________----------\n\n\n\n\n\n\n\n\n\n\n\n\n",
            NvUIFontFamily::SANS, mFPSText->GetFontSize(), NvUITextAlign::LEFT);
        m_simpleTimingStats->SetColor(NV_PACKED_COLOR(218, 218, 0, 255));
        m_simpleTimingStats->SetShadow();
		m_simpleTimingStats->GetScreenRect(textRect);
		m_simpleStatsBox = new NvUIContainer(textRect.width, textRect.height, new NvUIGraphicFrame("popup_frame.dds", 24, 24));
		m_simpleStatsBox->Add(m_simpleTimingStats, 8.0f, 8.0f);
        mUIWindow->Add(m_simpleStatsBox, fpsRect.left + fpsRect.width - textRect.width, fpsRect.top + fpsRect.height);
        m_simpleStatsBox->SetVisibility(m_statsMode == STATS_SIMPLE);
	}

    // Initially create our logos and add them to the window, but hidden in the upper corner.  
    // Let the reshape call fix up the size and location
    //m_logoNVIDIA = InitLogoTexture("textures/LogoNVIDIA.dds");
    //m_logoGLES = InitLogoTexture("textures/LogoGLES.dds");
    //m_logoVulkan = InitLogoTexture("textures/LogoVulkan.dds");
}

NvUIGraphic* ThreadedRenderingVk::InitLogoTexture(const char* pTexFilename)
{
    float logoAlpha = 0.5f;
    std::string texName = pTexFilename;
    NvUIGraphic* pLogo;
    pLogo = new NvUIGraphic(texName);
    {
        //NvUITexture* tex = NvUITexture::CacheTexture(texName);
        //NvUITexture* tex = new NvUITexture(texName);
        //pLogo = new NvUIGraphic(tex);

    }
    pLogo->SetAlpha(logoAlpha);
    pLogo->SetVisibility(false);
    mUIWindow->Add(pLogo, 0, 0);
    return pLogo;
}

void ThreadedRenderingVk::ResetFish(bool fishsplosion) {
	if (fishsplosion) {
		nv::vec3f location = (ms_tankMin + ms_tankMax) * 0.5f;
		for (uint32_t schoolIndex = 0; schoolIndex < m_schools.size(); ++schoolIndex)
		{
			School* pSchool = m_schools[schoolIndex];
			pSchool->ResetToLocation(location);
		}
	}
	else {
		for (uint32_t schoolIndex = 0; schoolIndex < m_schools.size(); ++schoolIndex)
		{
			School* pSchool = m_schools[schoolIndex];
			nv::vec3f tankSize = ms_tankMax - ms_tankMin;
			nv::vec3f location((float)rand() / (float)RAND_MAX * tankSize.x,
				(float)rand() / (float)RAND_MAX * tankSize.y,
				(float)rand() / (float)RAND_MAX * tankSize.z);
			location += ms_tankMin;
			pSchool->ResetToLocation(location);
		}
	}
}

NvUIEventResponse ThreadedRenderingVk::handleReaction(const NvUIReaction &react)
{
	// Simple input hook to ensure that all settings are compatible after
	// a user has chosen to change something.  We'll use a flag to indicate that
	// we actually modified something so that we can force the other UI elements
	// to update to match their new state before we return.
	bool bStateModified = false;
	switch (react.code)
	{
	case UIACTION_RESET_FISHPLOSION:
	{
		ResetFish(true);
        m_pFishplosionVar->reset();
        bStateModified = true;
		break;
	}
	case UIACTION_RESET_FISHFIREWORKS:
	{
		ResetFish(false);
        m_pFishFireworksVar->reset();
        bStateModified = true;
        break;
	}
	case UIACTION_RESET:
	{
		switch (react.ival)
		{
		case RESET_ORIGIN:
		{
			ResetFish(true);
			break;
		}
		case RESET_RANDOM:
		{
			ResetFish(false);
			break;
		}
		}

		// Reset the camera
		m_pInputHandler->setPosition(m_startingCameraPosition);
		m_pInputHandler->setYaw(m_startingCameraPitchYaw.y);
		m_pInputHandler->setPitch(m_startingCameraPitchYaw.x);
		break;
	}
	case UIACTION_SCHOOLCOUNT:
	{
		m_uiSchoolCount = SetNumSchools(react.ival);
		bStateModified = true;
		break;
	}
	case UIACTION_INSTCOUNT:
	{
		// Update our readout of total number of active fish
		m_uiFishCount = m_activeSchools * m_uiInstanceCount;

		if (m_pBatchSlider->GetValue() > m_uiInstanceCount) {
			m_pBatchSlider->SetValue(m_uiInstanceCount);
			*m_pBatchVar = m_uiInstanceCount;
		}

		m_pBatchSlider->SetMaxValue(m_uiInstanceCount);
		m_pBatchVar->SetMaxValue(m_uiInstanceCount);

		bStateModified = true;
		m_bUIDirty = true;
		UpdateUI();
		break;
	}
    case UIACTION_TANKSIZE:
    {
        ms_tankMax.x = ms_tankMax.z = (float)m_uiTankSize;
        ms_tankMin.x = ms_tankMin.z = -ms_tankMax.x;
        m_bTankSizeChanged = true;
        break;
    }
	case UIACTION_BATCHSIZE:
	{
		break;
	}
	case UIACTION_SCHOOLINFOID:
	{
		m_uiSchoolInfoId = react.ival;
		if (m_schoolDescs.size() <= m_uiSchoolInfoId)
		{
			InitializeSchoolDescriptions(m_uiSchoolInfoId + 1);
		}
		SchoolDesc& desc = m_schoolDescs[m_uiSchoolInfoId];
		m_uiSchoolDisplayModelIndex = desc.m_modelId;
		m_uiSchoolMaxSpeed = desc.m_flocking.m_maxSpeed;
		m_uiSchoolInertia = desc.m_flocking.m_inertia;
		m_uiSchoolNeighborDistance = desc.m_flocking.m_neighborDistance;
		m_uiSchoolGoalScale = desc.m_flocking.m_goalScale;
		m_uiSchoolAlignmentScale = desc.m_flocking.m_alignmentScale;
		m_uiSchoolRepulsionScale = desc.m_flocking.m_repulsionScale;
		m_uiSchoolCohesionScale = desc.m_flocking.m_cohesionScale;
		m_uiSchoolAvoidanceScale = desc.m_flocking.m_schoolAvoidanceScale;
		bStateModified = true;
		break;
	}
	case UIACTION_SCHOOLMODELINDEX:
	{
		uint32_t modelId = react.ival;
		m_uiSchoolDisplayModelIndex = modelId;
		m_schoolDescs[m_uiSchoolInfoId].m_modelId = modelId;
		m_schoolDescs[m_uiSchoolInfoId].m_scale = ms_schoolInfo[modelId].m_scale;

		if (m_schools.size() > m_uiSchoolInfoId)
		{
			m_schools[m_uiSchoolInfoId]->SetModel(vk(),
				m_models[modelId],
				ms_modelInfo[modelId].m_tailStartZ * ms_schoolInfo[modelId].m_scale,
				ms_modelInfo[modelId].GetCenteringTransform(),
				ms_modelInfo[modelId].m_halfExtents);
		}
		bStateModified = true;
		break;
	}
	case UIACTION_SCHOOLMAXSPEED:
	{
		m_uiSchoolMaxSpeed = react.fval;
		m_schoolDescs[m_uiSchoolInfoId].m_flocking.m_maxSpeed = m_uiSchoolMaxSpeed;
		if (m_schools.size() > m_uiSchoolInfoId)
		{
			School* pSchool = m_schools[m_uiSchoolInfoId];
			SchoolFlockingParams params = pSchool->GetFlockingParams();
			params.m_maxSpeed = m_uiSchoolMaxSpeed;
			pSchool->SetFlockingParams(params);
		}
		bStateModified = true;
		break;
	}
	case UIACTION_SCHOOLINERTIA:
	{
		m_uiSchoolInertia = react.fval;
		m_schoolDescs[m_uiSchoolInfoId].m_flocking.m_inertia = m_uiSchoolInertia;
		if (m_schools.size() > m_uiSchoolInfoId)
		{
			School* pSchool = m_schools[m_uiSchoolInfoId];
			SchoolFlockingParams params = pSchool->GetFlockingParams();
			params.m_inertia = m_uiSchoolInertia;
			pSchool->SetFlockingParams(params);
		}
		bStateModified = true;
		break;
	}
	case UIACTION_SCHOOLNEIGHBORDIST:
	{
		m_uiSchoolNeighborDistance = react.fval;
		m_schoolDescs[m_uiSchoolInfoId].m_flocking.m_neighborDistance = m_uiSchoolNeighborDistance;
		if (m_schools.size() > m_uiSchoolInfoId)
		{
			School* pSchool = m_schools[m_uiSchoolInfoId];
			SchoolFlockingParams params = pSchool->GetFlockingParams();
			params.m_neighborDistance = m_uiSchoolNeighborDistance;
			pSchool->SetFlockingParams(params);
		}
		bStateModified = true;
		break;
	}
	case UIACTION_SCHOOLGOAL:
	{
		m_uiSchoolGoalScale = react.fval;
		m_schoolDescs[m_uiSchoolInfoId].m_flocking.m_goalScale = m_uiSchoolGoalScale;
		if (m_schools.size() > m_uiSchoolInfoId)
		{
			School* pSchool = m_schools[m_uiSchoolInfoId];
			SchoolFlockingParams params = pSchool->GetFlockingParams();
			params.m_goalScale = m_uiSchoolGoalScale;
			pSchool->SetFlockingParams(params);
		}
		bStateModified = true;
		break;
	}
	case UIACTION_SCHOOLALIGNMENT:
	{
		m_uiSchoolAlignmentScale = react.fval;
		m_schoolDescs[m_uiSchoolInfoId].m_flocking.m_alignmentScale = m_uiSchoolAlignmentScale;
		if (m_schools.size() > m_uiSchoolInfoId)
		{
			School* pSchool = m_schools[m_uiSchoolInfoId];
			SchoolFlockingParams params = pSchool->GetFlockingParams();
			params.m_alignmentScale = m_uiSchoolAlignmentScale;
			pSchool->SetFlockingParams(params);
		}
		bStateModified = true;
		break;
	}
	case UIACTION_SCHOOLREPULSION:
	{
		m_uiSchoolRepulsionScale = react.fval;
		m_schoolDescs[m_uiSchoolInfoId].m_flocking.m_repulsionScale = m_uiSchoolRepulsionScale;
		if (m_schools.size() > m_uiSchoolInfoId)
		{
			School* pSchool = m_schools[m_uiSchoolInfoId];
			SchoolFlockingParams params = pSchool->GetFlockingParams();
			params.m_repulsionScale = m_uiSchoolRepulsionScale;
			pSchool->SetFlockingParams(params);
		}
		bStateModified = true;
		break;
	}
	case UIACTION_SCHOOLCOHESION:
	{
		m_uiSchoolCohesionScale = react.fval;
		m_schoolDescs[m_uiSchoolInfoId].m_flocking.m_cohesionScale = m_uiSchoolCohesionScale;
		if (m_schools.size() > m_uiSchoolInfoId)
		{
			School* pSchool = m_schools[m_uiSchoolInfoId];
			SchoolFlockingParams params = pSchool->GetFlockingParams();
			params.m_cohesionScale = m_uiSchoolCohesionScale;
			pSchool->SetFlockingParams(params);
		}
		bStateModified = true;
		break;
	}
	case UIACTION_SCHOOLAVOIDANCE:
	{
		m_uiSchoolAvoidanceScale = react.fval;
		m_schoolDescs[m_uiSchoolInfoId].m_flocking.m_schoolAvoidanceScale = m_uiSchoolAvoidanceScale;
		if (m_schools.size() > m_uiSchoolInfoId)
		{
			School* pSchool = m_schools[m_uiSchoolInfoId];
			SchoolFlockingParams params = pSchool->GetFlockingParams();
			params.m_schoolAvoidanceScale = m_uiSchoolAvoidanceScale;
			pSchool->SetFlockingParams(params);
		}
		bStateModified = true;
		break;
	}
	case UIACTION_ANIMTHREADCOUNT:
	{
		m_uiThreadCount = SetAnimationThreadNum(react.ival);
		bStateModified = true;
		break;
	}
	case UIACTION_STATSTOGGLE:
	{
        m_statsMode = (m_statsMode + 1) % STATS_COUNT;

        switch (m_statsMode)
        {
        case STATS_FULL:
            if (nullptr != m_fullStatsBox)
            {
                m_fullStatsBox->SetVisibility(true);
            }
            if (nullptr != m_simpleStatsBox)
            {
                m_simpleStatsBox->SetVisibility(false);
            }
            break;
        case STATS_SIMPLE:
            if (nullptr != m_fullStatsBox)
            {
                m_fullStatsBox->SetVisibility(false);
            }
            if (nullptr != m_simpleStatsBox)
            {
                m_simpleStatsBox->SetVisibility(true);
            }
            break;
        default:
            if (nullptr != m_fullStatsBox)
            {
                m_fullStatsBox->SetVisibility(false);
            }
            if (nullptr != m_simpleStatsBox)
            {
                m_simpleStatsBox->SetVisibility(false);
            }
            break;
        }
        m_pStatsModeVar->reset();
        bStateModified = true;
        break;
	}
    case UIACTION_DRAWGL:
    {
        m_pThreadedRenderingButton->SetVisibility(react.ival ? false : true);
        // Update the GLES/Vulkan logos to reflect the current state
        if (nullptr != m_logoGLES)
        {
            m_logoGLES->SetVisibility(!!react.ival);
        }
        if (nullptr != m_logoVulkan)
        {
            m_logoVulkan->SetVisibility(!react.ival);
        }
		break;
	}
	}
	m_bUIDirty |= bStateModified;
	UpdateUI();
	return nvuiEventNotHandled;
}

bool ThreadedRenderingVk::handleGamepadButtonChanged(uint32_t button, bool down) 
{
	// allow fishsplosion with closed UI
	if (!mUIWindow->GetVisibility() && (button == NvGamepad::BUTTON_Y)) 
    {
        if (down)
        {
            ResetFish(true);
        }
		return true;
	}

	if (button == NvGamepad::BUTTON_X) 
    {
		if (down) 
        {
			mUIWindow->SetVisibility(!mUIWindow->GetVisibility());
			mTweakBar->SetVisibility(false);
		}
		return true;
	}
	return false; 
}


void ThreadedRenderingVk::UpdateUI()
{
	if (m_uiSchoolCount != m_activeSchools)
	{
		m_uiSchoolCount = m_activeSchools;
		m_bUIDirty = true;
	}

	if (m_bUIDirty && (nullptr != mTweakBar))
	{
		mTweakBar->syncValues();
		m_bUIDirty = false;
	}
}

void ThreadedRenderingVk::reshape(int32_t width, int32_t height)
{
	glViewport(0, 0, NvSampleApp::m_width, NvSampleApp::m_height);

	//setting the perspective projection matrix
	nv::perspective(m_projUBO_Data.m_projectionMatrix, NV_PI / 3.0f,
		static_cast<float>(NvSampleApp::m_width) /
		static_cast<float>(NvSampleApp::m_height),
		0.1f, 100.0f);

	//setting the inverse perspective projection matrix
	m_projUBO_Data.m_inverseProjMatrix =
		nv::inverse(m_projUBO_Data.m_projectionMatrix);

	//setting the perspective projection matrix
	nv::perspectiveLH(m_projUBO->m_projectionMatrix, NV_PI / 3.0f,
		static_cast<float>(NvSampleApp::m_width) /
		static_cast<float>(NvSampleApp::m_height),
		0.1f, 100.0f);

	//setting the inverse perspective projection matrix
	m_projUBO->m_inverseProjMatrix =
		nv::inverse(m_projUBO->m_projectionMatrix);
	// m_projUBO will be updated in the next draw() call

	// adjusting the position to the new size
	NvUIRect fpsRect;
	mFPSText->GetScreenRect(fpsRect); // base off of fps element.
	NvUIRect textRect;
	m_simpleTimingStats->GetScreenRect(textRect);
	mUIWindow->Remove(m_simpleStatsBox);
	mUIWindow->Add(m_simpleStatsBox, fpsRect.left + fpsRect.width - textRect.width, fpsRect.top + fpsRect.height);
    m_fullTimingStats->GetScreenRect(textRect);
    mUIWindow->Remove(m_fullStatsBox);
    mUIWindow->Add(m_fullStatsBox, fpsRect.left + fpsRect.width - textRect.width, fpsRect.top + fpsRect.height);
    UpdateLogos();
}

void ThreadedRenderingVk::UpdateLogos()
{
    // Use the smallest dimension to determine scaling for logos
    bool bFitToHeight = mUIWindow->GetHeight() < mUIWindow->GetWidth();

    // Offset the logos from the sides of the window
    float logoPadding = 20.0f;
    
    // Scale the logo to a percentage of the main window
    float logoRelSize = 0.1f;

    float logoOrigWidth;
    float logoOrigHeight;
    // Initialize both of our display dimensions based on the current window size,
    // then we will leave the primary dimension alone, but overwrite the dependant one
    float logoDisplayWidth = mUIWindow->GetWidth() * logoRelSize;
    float logoDisplayHeight = mUIWindow->GetHeight() * logoRelSize;

    // Nvidia Logo
    if (nullptr != m_logoNVIDIA)
    {
        logoOrigWidth = m_logoNVIDIA->GetTex()->GetWidth();
        logoOrigHeight = m_logoNVIDIA->GetTex()->GetHeight();
        if (bFitToHeight)
        {
            // When fitting to height, we keep our calculated height value and then calculate
            // a new width that will preserve the original aspect ratio
            logoDisplayWidth = (logoDisplayHeight / logoOrigHeight) * logoOrigWidth;
        }
        else
        {
            // When fitting to width, we keep our calculated width value and then calculate
            // a new height that will preserve the original aspect ratio
            logoDisplayHeight = (logoDisplayWidth / logoOrigWidth) * logoOrigHeight;
        }
        m_logoNVIDIA->SetDimensions(logoDisplayWidth, logoDisplayHeight);
        mUIWindow->Remove(m_logoNVIDIA);
        mUIWindow->Add(m_logoNVIDIA, mUIWindow->GetWidth() - logoPadding - logoDisplayWidth, mUIWindow->GetHeight() - logoPadding - logoDisplayHeight);
        m_logoNVIDIA->SetVisibility(m_bDisplayLogos);
    }

    // Vulkan Logo
    if (nullptr != m_logoVulkan)
    {
        logoOrigWidth = m_logoVulkan->GetTex()->GetWidth();
        logoOrigHeight = m_logoVulkan->GetTex()->GetHeight();
        if (bFitToHeight)
        {
            // When fitting to height, we keep our calculated height value and then calculate
            // a new width that will preserve the original aspect ratio
            logoDisplayWidth = (logoDisplayHeight / logoOrigHeight) * logoOrigWidth;
        }
        else
        {
            // When fitting to width, we keep our calculated width value and then calculate
            // a new height that will preserve the original aspect ratio
            logoDisplayHeight = (logoDisplayWidth / logoOrigWidth) * logoOrigHeight;
        }
        m_logoVulkan->SetDimensions(logoDisplayWidth, logoDisplayHeight);
        mUIWindow->Remove(m_logoVulkan);
        mUIWindow->Add(m_logoVulkan, logoPadding, mUIWindow->GetHeight() - logoPadding - logoDisplayHeight);
        m_logoVulkan->SetVisibility(!m_requestedDrawGL && m_bDisplayLogos);
    }

    // GLES Logo
    if (nullptr != m_logoGLES)
    {
        logoOrigWidth = m_logoGLES->GetTex()->GetWidth();
        logoOrigHeight = m_logoGLES->GetTex()->GetHeight();
        if (bFitToHeight)
        {
            // When fitting to height, we keep our calculated height value and then calculate
            // a new width that will preserve the original aspect ratio
            logoDisplayWidth = (logoDisplayHeight / logoOrigHeight) * logoOrigWidth;
        }
        else
        {
            // When fitting to width, we keep our calculated width value and then calculate
            // a new height that will preserve the original aspect ratio
            logoDisplayHeight = (logoDisplayWidth / logoOrigWidth) * logoOrigHeight;
        }
        m_logoGLES->SetDimensions(logoDisplayWidth, logoDisplayHeight);
        mUIWindow->Remove(m_logoGLES);
        mUIWindow->Add(m_logoGLES, logoPadding, mUIWindow->GetHeight() - logoPadding - logoDisplayHeight);
        m_logoGLES->SetVisibility(m_requestedDrawGL && m_bDisplayLogos);
    }
}

float ThreadedRenderingVk::getClampedFrameTime() {
	float delta = getFrameDeltaTime();
	if (delta > 0.2f)
		delta = 0.2f;
	return delta;
}

void ThreadedRenderingVk::draw(void)
{
	UpdateStats();
	if (m_requestedDrawGL != m_drawGL)
    {
        m_drawGL = m_requestedDrawGL;
    }
    if (m_drawGL)
		return;

    // Update the active number of threads
    if (m_requestedActiveThreads != m_activeThreads)
    {
        m_activeThreads = m_requestedActiveThreads;
        vkDeviceWaitIdle(device());
        m_bUIDirty = true;
        UpdateUI();
    }
    if (m_requestedThreadedRendering != m_threadedRendering)
    {
        m_threadedRendering = m_requestedThreadedRendering;
        vkDeviceWaitIdle(device());
    }

	if (m_flushPerFrame)
		vkDeviceWaitIdle(device());
	VkResult result;

	neighborOffset = (neighborOffset + 1) % (6 - neighborSkip);

	s_threadMask = 0;

    if (m_bTankSizeChanged)
    {
        UpdateSchoolTankSizes();
    }

	m_currentTime += getClampedFrameTime();
    m_schoolStateMgr.BeginFrame(m_activeSchools);
#if FISH_DEBUG
	LOGI("\n################################################################");
	LOGI("BEGINNING OF FRAME");
#endif
    // Update the camera position if we are following a school
    if (m_uiCameraFollow)
    {
        if (m_uiSchoolInfoId < m_activeSchools)
        {
            // Get the centroid of the school we're following
            School* pSchool = m_schools[m_uiSchoolInfoId];
            nv::vec3f camPos = pSchool->GetCentroid() - (m_pInputHandler->getLookVector() * pSchool->GetRadius() * 4);
            if (camPos.y < 0.01f)
            {
                camPos.y = 0.01f;
            }
            m_pInputHandler->setPosition(camPos);
        }
        else
        {
            m_uiCameraFollow = false;
            m_bUIDirty = true;
            UpdateUI();
        }
    }

	// Get the current view matrix (according to user input through mouse,
	// gamepad, etc.)
	m_projUBO->m_viewMatrix = m_pInputHandler->getViewMatrix();
	m_projUBO->m_inverseViewMatrix = m_pInputHandler->getCameraMatrix();
	m_projUBO.Update();

	m_subCommandBuffers.BeginFrame();
	m_lightingUBO->m_causticOffset = m_currentTime * m_causticSpeed;
    m_lightingUBO->m_causticTiling = m_causticTiling;
    m_lightingUBO.Update();
#if 0	
	if (!m_animPaused)
	{
		// Update schools
		uint32_t schoolIndex = 0;
		for (; schoolIndex < m_activeSchools; ++schoolIndex)
		{
			School* pSchool = m_schools[schoolIndex];
			pSchool->Update(getClampedFrameTime(), &m_schoolStateMgr);
		}
	}
#endif	
	// Render the requested content (from dropdown menu in TweakBar UI)


	// BEGIN NEW RENDERING CODE

	// The SimpleCommandBuffer class overloads the * operator
	VkCommandBuffer cmd = vk().getMainCommandBuffer();

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

	renderPassBeginInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
	renderPassBeginInfo.framebuffer = vk().mainRenderTarget()->frameBuffer();
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_width;
	renderPassBeginInfo.renderArea.extent.height = m_height;

	VkClearValue clearValues[2];
	clearValues[0].color.float32[0] = 0.33f;
	clearValues[0].color.float32[1] = 0.44f;
	clearValues[0].color.float32[2] = 0.66f;
	clearValues[0].color.float32[3] = 1.0f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;


	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.clearValueCount = 2;

	vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	VkCommandBufferInheritanceInfo inherit = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
	inherit.framebuffer = vk().mainRenderTarget()->frameBuffer();
	inherit.renderPass = vk().mainRenderTarget()->clearRenderPass();

	VkCommandBuffer secCmd = m_subCommandBuffers[0];

	VkCommandBufferBeginInfo secBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	secBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT |
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	secBeginInfo.pInheritanceInfo = &inherit;
	result = vkBeginCommandBuffer(secCmd, &secBeginInfo);

	CHECK_VK_RESULT();
	{
		VkViewport vp;
		VkRect2D sc;
		vp.x = 0;
		vp.y = 0;
		vp.height = (float)(m_height);
		vp.width = (float)(m_width);
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;

		sc.offset.x = 0;
		sc.offset.y = 0;
		sc.extent.width = vp.width;
		sc.extent.height = vp.height;

		vkCmdSetViewport(secCmd, 0, 1, &vp);
		vkCmdSetScissor(secCmd, 0, 1, &sc);
	}
    {
        uint32_t offsets[DESC_COUNT] = { 0 };
		offsets[0] = m_projUBO.getDynamicOffset();
		offsets[1] = m_lightingUBO.getDynamicOffset();
		offsets[2] = m_schools[0]->getModelUBO().getDynamicOffset();
		offsets[3] = m_schools[0]->getMeshUBO().getDynamicOffset();
		vkCmdBindDescriptorSets(secCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeLayout, 0, 1, &mDescriptorSet, DESC_FIRST_TEX, offsets);

		DrawSkyboxColorDepth(secCmd);
		DrawGroundPlane(secCmd);
	}

	result = vkEndCommandBuffer(secCmd);
	CHECK_VK_RESULT();

	vkCmdExecuteCommands(cmd, 1, &secCmd);

	{
		CPU_TIMER_SCOPE(CPU_TIMER_MAIN_WAIT);

		for (uint32_t i = 0; i < MAX_ANIMATION_THREAD_COUNT; i++) {
			ThreadData& thread = m_Threads[i];
			thread.m_cmdBufferOpen = false;
			thread.m_drawCallCount = 0;
		}

		uint32_t schoolsPerThread = m_activeSchools / m_activeThreads;
		uint32_t remainderSchools = m_activeSchools % m_activeThreads;
		uint32_t baseSchool = 0;
		for (uint32_t i = 0; i < m_activeThreads; i++) {
			ThreadData& thread = m_Threads[i];
			thread.m_baseSchoolIndex = baseSchool;
			thread.m_schoolCount = schoolsPerThread;
			// distribute the remainder evenly
			if (remainderSchools > 0) {
				thread.m_schoolCount++;
				remainderSchools--;
			}
				
			baseSchool += thread.m_schoolCount;
		}

		m_doneCount = 0;
		m_drawCallCount = 0;

		m_FrameStartLock->lockMutex();
		{
			m_FrameStartCV->broadcastConditionVariable();
		}
		m_FrameStartLock->unlockMutex();

		m_DoneCountLock->lockMutex();
		{
			while (m_doneCount != m_activeSchools)
			{
				m_DoneCountCV->waitConditionVariable(m_DoneCountLock);
			}
		}
		m_DoneCountLock->unlockMutex();
	}

	{
		CPU_TIMER_SCOPE(CPU_TIMER_MAIN_CMD_BUILD);

		if (m_threadedRendering) {
			for (uint32_t i = 0; i < m_activeThreads; i++) {
				VkCommandBuffer secCmd = m_subCommandBuffers[i + 1];
				vkCmdExecuteCommands(cmd, 1, &secCmd);
				m_Threads[i].m_cmdBufferOpen = false;
				m_drawCallCount += m_Threads[i].m_drawCallCount;
			}
		}
		else
		{
			// round robin through the available buffers, so they are all used
			// over a few frames; this tries to match the idea that the threading
			// code will also hit all of these buffers when enabled
			int bufIndex = (m_frameLogicalClock % MAX_ANIMATION_THREAD_COUNT) + 1;
			VkCommandBuffer secCmd = m_subCommandBuffers[bufIndex];

			uint32_t schoolIndex = 0;
			for (; schoolIndex < m_activeSchools; ++schoolIndex)
			{
				School* pSchool = m_schools[schoolIndex];
				m_drawCallCount += DrawSchool(secCmd, pSchool, schoolIndex == 0);
			}
			result = vkEndCommandBuffer(secCmd);
			CHECK_VK_RESULT();
			vkCmdExecuteCommands(cmd, 1, &secCmd);
		}
	}

	vkCmdEndRenderPass(cmd);

	vk().submitMainCommandBuffer();

	m_subCommandBuffers.DoneWithFrame();

#if FISH_DEBUG
	LOGI("END OF FRAME");
	LOGI("################################################################\n");
#endif
	m_frameLogicalClock++;
}

//-----------------------------------------------------------------------------
// PRIVATE METHODS

void ThreadedRenderingVk::CleanRendering(void)
{
}

void ThreadedRenderingVk::InitThreads(void)
{
	NV_ASSERT(nullptr != pDevice);

	NvThreadManager* threadManager = getThreadManagerInstance();
	NV_ASSERT(nullptr != threadManager);

	NV_ASSERT(m_FrameStartLock == NULL);
	NV_ASSERT(m_FrameStartCV == NULL);
	NV_ASSERT(m_NeedsUpdateQueueLock == NULL);
	NV_ASSERT(m_DoneCountLock == NULL);
	NV_ASSERT(m_DoneCountCV == NULL);

	m_FrameStartLock =
		threadManager->initializeMutex(false, NvMutex::MutexLockLevelInitial);
	m_FrameStartCV =
		threadManager->initializeConditionVariable();
	m_NeedsUpdateQueueLock =
		threadManager->initializeMutex(false, NvMutex::MutexLockLevelInitial);
	m_DoneCountLock =
		threadManager->initializeMutex(false, NvMutex::MutexLockLevelInitial);
	m_DoneCountCV = threadManager->initializeConditionVariable();

	NV_ASSERT(m_FrameStartLock != NULL);
	NV_ASSERT(m_FrameStartCV != NULL);
	NV_ASSERT(m_NeedsUpdateQueueLock != NULL);
	NV_ASSERT(m_DoneCountLock != NULL);
	NV_ASSERT(m_DoneCountCV != NULL);

	m_subCommandBuffers.Initialize(&vk(), m_queueMutexPtr, false);

	for (intptr_t i = 0; i < MAX_ANIMATION_THREAD_COUNT; i++)
	{
		ThreadData& thread = m_Threads[i];
		if (thread.m_thread != NULL)
			delete thread.m_thread;

		void* threadIndex = reinterpret_cast<void*>(i);
		m_Threads[i].m_thread =
			threadManager->createThread(AnimateJobFunctionThunk, &thread,
										&(m_ThreadStacks[i]),
										THREAD_STACK_SIZE,
										NvThread::DefaultThreadPriority);

		NV_ASSERT(thread.m_thread != NULL);

		thread.m_thread->startThread();
	}

	m_uiThreadCount = SetAnimationThreadNum(MAX_ANIMATION_THREAD_COUNT);
}

void ThreadedRenderingVk::CleanThreads(void)
{
	NV_ASSERT(nullptr != pDevice);

	NvThreadManager* threadManager = getThreadManagerInstance();
	NV_ASSERT(nullptr != threadManager);

	m_running = false;
	if (m_FrameStartCV)
		m_FrameStartCV->broadcastConditionVariable();

	for (uint32_t i = 0; i < MAX_ANIMATION_THREAD_COUNT; i++)
	{
		if (m_Threads[i].m_thread != NULL)
		{
			m_Threads[i].m_thread->waitThread();
			threadManager->destroyThread(m_Threads[i].m_thread);
			m_Threads[i].m_thread = NULL;
		}
	}

	if (m_NeedsUpdateQueueLock)
		threadManager->finalizeMutex(m_NeedsUpdateQueueLock);
	if (m_DoneCountLock)
		threadManager->finalizeMutex(m_DoneCountLock);
	if (m_DoneCountCV)
		threadManager->finalizeConditionVariable(m_DoneCountCV);

	m_FrameStartLock = NULL;
	m_FrameStartCV = NULL;
	m_NeedsUpdateQueueLock = NULL;
	m_DoneCountLock = NULL;
	m_DoneCountCV = NULL;
}

uint32_t ThreadedRenderingVk::DrawSchool(VkCommandBuffer& cmd, School* pSchool, bool openBuffer) {
	if (openBuffer) {
		VkCommandBufferInheritanceInfo inherit = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
		inherit.framebuffer = vk().mainRenderTarget()->frameBuffer();
		inherit.renderPass = vk().mainRenderTarget()->clearRenderPass();

		VkCommandBufferBeginInfo secBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		secBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT |
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		secBeginInfo.pInheritanceInfo = &inherit;
		VkResult result = vkBeginCommandBuffer(cmd, &secBeginInfo);
		{
			VkViewport vp;
			VkRect2D sc;
			vp.x = 0;
			vp.y = 0;
			vp.height = (float)(m_height);
			vp.width = (float)(m_width);
			vp.minDepth = 0.0f;
			vp.maxDepth = 1.0f;

			sc.offset.x = 0;
			sc.offset.y = 0;
			sc.extent.width = vp.width;
			sc.extent.height = vp.height;

			vkCmdSetViewport(cmd, 0, 1, &vp);
			vkCmdSetScissor(cmd, 0, 1, &sc);
		}

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_fishPipe);

        uint32_t offsets[DESC_COUNT] = { 0 };
		offsets[0] = m_projUBO.getDynamicOffset();
		offsets[1] = m_lightingUBO.getDynamicOffset();
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_fishPipeLayout, 0, 1, &mDescriptorSet, DESC_FIRST_TEX, offsets);
	}
	return pSchool->Render(cmd, m_uiBatchSize);
}

//  Draws the skybox with lighting in color and depth
void ThreadedRenderingVk::DrawSkyboxColorDepth(VkCommandBuffer& cmd)
{
	VkViewport vp;
	VkRect2D sc;
	vp.x = 0;
	vp.y = 0;
	vp.height = (float)(m_height);
	vp.width = (float)(m_width);
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	sc.offset.x = 0;
	sc.offset.y = 0;
	sc.extent.width = vp.width;
	sc.extent.height = vp.height;

	vkCmdSetViewport(cmd, 0, 1, &vp);
	vkCmdSetScissor(cmd, 0, 1, &sc);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxColorPipe);
	{
		m_quad->Draw(cmd);
	}
}

void ThreadedRenderingVk::DrawGroundPlane(VkCommandBuffer& cmd)
{
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_groundPlanePipe);
	{
		m_quad->Draw(cmd);
	}
}

//  Draws the skybox with lighting in color and depth
void ThreadedRenderingVk::DrawSkyboxColorDepthGL()
{
	glDisable(GL_DEPTH_TEST);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_projUBO_Location, m_projUBO_Id);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, m_skyboxSandTexGL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, m_skyboxGradientTexGL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	m_shader_Skybox->enable();
	NvDrawQuadGL(0);
	m_shader_Skybox->disable();

	glEnable(GL_DEPTH_TEST);
}

void ThreadedRenderingVk::DrawGroundPlaneGL()
{
	// vertex positions in NDC tex-coords
	static const float groundplaneQuadData[] =
	{
		1.0f, 1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 1.0f
	};

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), groundplaneQuadData);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), &groundplaneQuadData[2]);
	glEnableVertexAttribArray(1);

	glDisable(GL_DEPTH_TEST);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_projUBO_Location, m_projUBO_Id);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_lightingUBO_Location, m_lightingUBO_Id);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_caustic1TexGL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_caustic2TexGL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, m_skyboxSandTexGL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glDisable(GL_CULL_FACE);
	m_shader_GroundPlane->enable();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	m_shader_GroundPlane->disable();

	glEnable(GL_DEPTH_TEST);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void ThreadedRenderingVk::UpdateSchool(uint32_t threadIndex,
	uint32_t schoolIndex, School* pSchool)
{
	pSchool->SetInstanceCount(m_uiInstanceCount);
	{
		CPU_TIMER_SCOPE(CPU_TIMER_THREAD_BASE_ANIMATE + threadIndex);
		pSchool->Animate(getClampedFrameTime(), &m_schoolStateMgr, m_avoidance);
	}
	{
		CPU_TIMER_SCOPE(CPU_TIMER_THREAD_BASE_UPDATE + threadIndex);
		pSchool->Update();
	}
}

// Static Data to define available models
static const nv::matrix4f sc_yawNeg90(
	 0.0f,  0.0f,  1.0f,  0.0f,
	 0.0f,  1.0f,  0.0f,  0.0f,
	-1.0f,  0.0f,  0.0f,  0.0f,
	 0.0f,  0.0f,  0.0f,  1.0f
);

static const nv::matrix4f sc_yaw180(
	-1.0f,  0.0f,  0.0f,  0.0f,
	 0.0f,  1.0f,  0.0f,  0.0f,
	 0.0f,  0.0f, -1.0f,  0.0f,
	 0.0f,  0.0f,  0.0f,  1.0f
);

static const nv::vec3f sc_zeroVec(0.0f, 0.0f, 0.0f);

// Initialize the model desc array with data that we know, leaving the bounding
// box settings as zeroes.  We'll fill those in when the models are loaded.
ThreadedRenderingVk::ModelDesc ThreadedRenderingVk::ms_modelInfo[MODEL_COUNT] =
{
    { "Black & White Fish", "models/Black_White_Fish.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.10f },
    { "Blue Fish 1", "models/Blue_Fish.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.25f },
    { "Blue Fish 2", "models/Blue_Fish_02.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.30f },
    { "Blue Fish 3", "models/Blue_Fish_03.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.25f },
    { "Blue Fish 4", "models/Blue_Fish_04.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.30f },
    { "Blue Fish 5", "models/Blue_Fish_05.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.25f },
    { "Blue Fish 6", "models/Blue_Fish_06.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.15f },
    { "Blue Fish 7", "models/Blue_Fish_07.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.35f },
    { "Blue Fish 8", "models/Blue_Fish_08.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.25f },
    { "Blue Fish 9", "models/Blue_Fish_09.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.20f },
    { "Cyan Fish", "models/Cyan_Fish.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.25f },
    { "Pink Fish", "models/Pink_Fish.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.20f },
    { "Red Fish", "models/Red_Fish.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.28f },
    { "Violet Fish", "models/Violet_Fish.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.30f },
    { "Yellow Fish 1", "models/Yellow_Fish.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.40f },
    { "Yellow Fish 2", "models/Yellow_Fish_02.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.15f },
    { "Yellow Fish 3", "models/Yellow_Fish_03.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.25f },
    { "Yellow Fish 4", "models/Yellow_Fish_04.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.30f },
    { "Yellow Fish 5", "models/Yellow_Fish_05.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.25f },
    { "Yellow Fish 6", "models/Yellow_Fish_06.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.30f },
    { "Yellow Fish 7", "models/Yellow_Fish_07.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.25f },
    { "Yellow Fish 8", "models/Yellow_Fish_08.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.23f },
    { "Yellow Fish 9", "models/Yellow_Fish_09.nve", sc_yawNeg90, sc_zeroVec, sc_zeroVec, 0.25f },
    { "Yellow Fish 10", "models/Yellow_Fish_10.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.30f },
    { "Yellow Fish 11", "models/Yellow_Fish_11.nve", sc_yaw180, sc_zeroVec, sc_zeroVec, 0.32f }
};

SchoolFlockingParams ThreadedRenderingVk::ms_fishTypeDefs[FISHTYPE_COUNT] =
{
    //     |       |Goal|      Spawn Zone       |Neighbor|Spawn|          |<***************** Strengths ****************>|             
    // Spd |Inertia|Size|    Min         Max    |Distance|Range|Aggression| Goal |Alignment|Repulsion |Cohesion|Avoidance|
    SchoolFlockingParams(1.5f,  16.0f, 8.0f, ms_tankMin, ms_tankMax,  4.00f,  0.01f,   0.9f,    0.1f,   0.1f,    0.10f,      0.5f,     0.1f), // EXTRALARGE
    SchoolFlockingParams(1.5f,  16.0f, 6.0f, ms_tankMin, ms_tankMax,  3.50f,  0.01f,   0.8f,    0.1f,   0.1f,    0.10f,      0.5f,     1.0f ), // LARGESLOW
    SchoolFlockingParams(2.5f,  16.0f, 6.0f, ms_tankMin, ms_tankMax,  3.00f,  0.01f,   0.7f,    0.1f,   0.1f,    0.15f,      0.5f,     1.0f ), // LARGE
    SchoolFlockingParams(3.5f,  12.0f, 5.0f, ms_tankMin, ms_tankMax,  2.50f,  0.01f,   0.6f,    0.2f,   0.2f,    0.10f,      0.5f,     1.0f ), // LARGEFAST
    SchoolFlockingParams(2.5f,  14.0f, 5.0f, ms_tankMin, ms_tankMax,  2.00f,  0.01f,   0.5f,    0.1f,   0.1f,    0.15f,      0.5f,     2.0f ), // MEDIUMSLOW
    SchoolFlockingParams(3.5f,  12.0f, 4.0f, ms_tankMin, ms_tankMax,  1.60f,  0.01f,   0.4f,    0.1f,   0.1f,    0.15f,      0.5f,     2.0f ), // MEDIUM
    SchoolFlockingParams(6.0f,  10.0f, 3.0f, ms_tankMin, ms_tankMax,  1.40f,  0.01f,   0.3f,    0.2f,   0.1f,    0.10f,      0.5f,     2.0f ), // MEDIUMFAST
    SchoolFlockingParams(5.0f,  10.0f,10.0f, ms_tankMin, ms_tankMax,  1.50f,  0.01f,   0.1f,    0.1f,   0.1f,    0.15f,      0.5f,     3.0f ), // MEDIUMSPARSE
    SchoolFlockingParams(3.0f,   8.0f, 3.0f, ms_tankMin, ms_tankMax,  1.00f,  0.01f,   0.2f,    0.1f,   0.2f,    0.10f,      0.5f,     4.0f ), // SMALLSLOW
    SchoolFlockingParams(5.0f,   5.0f, 2.0f, ms_tankMin, ms_tankMax,  0.25f,  0.01f,   0.1f,    0.1f,   0.4f,    0.15f,      0.5f,     5.0f ), // SMALL
    SchoolFlockingParams(7.0f,   4.0f, 1.0f, ms_tankMin, ms_tankMax,  0.25f,  0.01f,   0.1f,    0.2f,   0.5f,    0.40f,      0.1f,     6.0f )  // SMALLFAST
};

ThreadedRenderingVk::SchoolDesc ThreadedRenderingVk::ms_schoolInfo[MODEL_COUNT] = 
{
    // ModelId,           NumFish, Scale, 
    { MODEL_BLACK_WHITE_FISH,  75, 2.00f, ms_fishTypeDefs[FISHTYPE_LARGEFAST] },
    { MODEL_BLUE_FISH,         75, 2.00f, ms_fishTypeDefs[FISHTYPE_LARGE] },          
    { MODEL_BLUE_FISH_02,     100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUM] },
    { MODEL_BLUE_FISH_03,     100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUMSLOW] },
    { MODEL_BLUE_FISH_04,     100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUM] },
    { MODEL_BLUE_FISH_05,     100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUM] },
    { MODEL_BLUE_FISH_06,     100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUMFAST] },
    { MODEL_BLUE_FISH_07,     200, 0.50f, ms_fishTypeDefs[FISHTYPE_SMALLFAST] },
    { MODEL_BLUE_FISH_08,     200, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUMSPARSE] },
    { MODEL_BLUE_FISH_09,      50, 3.00f, ms_fishTypeDefs[FISHTYPE_EXTRALARGE] },
    { MODEL_CYAN_FISH,        100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUM] },
    { MODEL_PINK_FISH,        150, 0.75f, ms_fishTypeDefs[FISHTYPE_SMALLSLOW] },
    { MODEL_RED_FISH,         50,  3.00f, ms_fishTypeDefs[FISHTYPE_LARGESLOW] },
    { MODEL_VIOLET_FISH,      250, 0.50f, ms_fishTypeDefs[FISHTYPE_SMALLFAST] },
    { MODEL_YELLOW_FISH,      100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUMFAST] },
    { MODEL_YELLOW_FISH_02,   100, 1.50f, ms_fishTypeDefs[FISHTYPE_MEDIUMSLOW] },
    { MODEL_YELLOW_FISH_03,   100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUMFAST] },
    { MODEL_YELLOW_FISH_04,   100, 0.75f, ms_fishTypeDefs[FISHTYPE_MEDIUMFAST] },
    { MODEL_YELLOW_FISH_05,   150, 0.80f, ms_fishTypeDefs[FISHTYPE_SMALLSLOW] },
    { MODEL_YELLOW_FISH_06,   100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUM] },
    { MODEL_YELLOW_FISH_07,   100, 1.20f, ms_fishTypeDefs[FISHTYPE_MEDIUMSLOW] },
    { MODEL_YELLOW_FISH_08,   100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUM] },
    { MODEL_YELLOW_FISH_09,   150, 0.80f, ms_fishTypeDefs[FISHTYPE_SMALLSLOW] },
    { MODEL_YELLOW_FISH_10,   150, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUMSPARSE] },
    { MODEL_YELLOW_FISH_11,   100, 1.00f, ms_fishTypeDefs[FISHTYPE_MEDIUM] }
};

static void sprintComma(uint32_t val, char* str) {
	uint32_t billions = val / 1000000000;
	val = val % 1000000000;

	uint32_t millions = val / 1000000;
	val = val % 1000000;

	uint32_t thousands = val / 1000;

	uint32_t units = val % 1000;

	if (billions) {
		sprintf(str, "%d,%03d,%03d,%03d", billions, millions, thousands, units);
	}
	else if (millions) {
		sprintf(str, "%d,%03d,%03d", millions, thousands, units);
	}
	else if (thousands) {
		sprintf(str, "%d,%03d", thousands, units);
	}
	else {
		sprintf(str, "%d", units);
	}
}

void ThreadedRenderingVk::UpdateStats()
{
    if (!m_statsCountdown) 
    {
        float frameConv = 1000.0f / STATS_FRAMES;
        m_meanFPS = mFramerate->getMeanFramerate();
        m_meanCPUMainCmd = m_CPUTimers[CPU_TIMER_MAIN_CMD_BUILD].getScaledCycles() *
            frameConv;
        m_meanCPUMainWait = m_CPUTimers[CPU_TIMER_MAIN_WAIT].getScaledCycles() *
            frameConv;
        m_meanCPUMainCopyVBO = m_CPUTimers[CPU_TIMER_MAIN_COPYVBO].getScaledCycles() *
            frameConv;

        for (uint32_t i = 0; i < m_activeThreads; i++) {
            ThreadTimings& t = m_threadTimings[i];
            t.cmd = m_CPUTimers[CPU_TIMER_THREAD_BASE_CMD_BUILD + i].getScaledCycles() *
                frameConv;
            t.anim = m_CPUTimers[CPU_TIMER_THREAD_BASE_ANIMATE + i].getScaledCycles() *
                frameConv;
            t.update = m_CPUTimers[CPU_TIMER_THREAD_BASE_UPDATE + i].getScaledCycles() *
                frameConv;
            t.tot = m_CPUTimers[CPU_TIMER_THREAD_BASE_TOTAL + i].getScaledCycles() *
                frameConv;
        }

        if (m_drawGL) {
            m_meanGPUFrameMS = m_GPUTimer.getScaledCycles() / STATS_FRAMES;
            m_GPUTimer.reset();
        }
        else {
            NvGPUTimerVK& gpu = vk().getFrameTimer();
            m_meanGPUFrameMS = gpu.getScaledCycles();
        }

        for (int32_t i = 0; i < CPU_TIMER_COUNT; ++i)
        {
            m_CPUTimers[i].reset();
        }

        m_statsCountdown = STATS_FRAMES;

        char str[1024];
        BuildSimpleStatsString(str, 1024);
        m_simpleTimingStats->SetString(str);

        BuildFullStatsString(str, 1024);
        m_fullTimingStats->SetString(str);
    }
    else
    {
        m_statsCountdown--;
    }
}

void ThreadedRenderingVk::BuildSimpleStatsString(char* buffer, int32_t size)
{
    setlocale(LC_NUMERIC, "");
    char fishCountStr[32];
    sprintComma(m_uiFishCount, fishCountStr);
    char fishRateStr[32];
    uint32_t fishRate = (uint32_t)(m_uiFishCount * m_meanFPS);
    sprintComma(fishRate, fishRateStr);
    char drawCallRateStr[32];
    uint32_t drawCallRate = (uint32_t)(m_drawCallCount * m_meanFPS);
    sprintComma(drawCallRate, drawCallRateStr);

    int32_t offset = sprintf(buffer,
        NvBF_COLORSTR_GREEN 
        "%s\n" 
        NvBF_COLORSTR_WHITE
        "Fish/frame: %s\n"
        NvBF_COLORSTR_NORMAL
        NVBF_STYLESTR_BOLD
        "Draw Calls/sec: %s\n"
        NvBF_COLORSTR_WHITE
        NVBF_STYLESTR_NORMAL
        "CPU Thd0: %5.1fms\n"
        "ThdID, Time\n",
        m_drawGL ? "OpenGL ES3.1 AEP" : "Vulkan",
        fishCountStr,
        drawCallRateStr, 
        m_meanCPUMainCmd + m_meanCPUMainCopyVBO);

    for (uint32_t i = 0; i < m_activeThreads; ++i) {
        offset += sprintf(buffer + offset,
            "Thr%01d ( %5.1fms)\n",
            i + 1, m_threadTimings[i].tot);
    }
}

void ThreadedRenderingVk::BuildFullStatsString(char* buffer, int32_t size)
{
    setlocale(LC_NUMERIC, "");
	char fishCountStr[32];
	sprintComma(m_uiFishCount, fishCountStr);
	char fishRateStr[32];
	uint32_t fishRate = (uint32_t)(m_uiFishCount * m_meanFPS);
	sprintComma(fishRate, fishRateStr);
	char drawCallRateStr[32];
    uint32_t drawCallRate = (uint32_t)(m_drawCallCount * m_meanFPS);
	sprintComma(drawCallRate, drawCallRateStr);

	int32_t offset = sprintf(buffer,
		NvBF_COLORSTR_GREEN 
        "%s\n" 
        NvBF_COLORSTR_WHITE
		"Fish/frame: %s\n"
        "Fish/sec: %s\n"
		"Draw Calls/sec: %s\n"
		"CPU Thd0 CmdBuf: %5.1fms\n"
		"CPU Thd0 Wait: %5.1fms\n"
        "CPU Thd0 CopyVBO: %5.1fms\n"
		"GPU: %5.1fms\n"
		"ThdID, CmdBuf,   Anim,  Update,  TOTAL\n",
		m_drawGL ? "OpenGL ES3.1 AEP" : "Vulkan",
		fishCountStr, 
		fishRateStr, 
        drawCallRateStr, m_meanCPUMainCmd, m_meanCPUMainWait, m_meanCPUMainCopyVBO,
        m_meanGPUFrameMS);

	for (uint32_t i = 0; i < m_activeThreads; ++i) {
		offset += sprintf(buffer + offset,
			"Thr%01d ( %5.1fms, %5.1fms, %5.1fms, %5.1fms)\n",
            i + 1, m_threadTimings[i].cmd, m_threadTimings[i].anim, m_threadTimings[i].update, m_threadTimings[i].tot);
	}
}

//-----------------------------------------------------------------------------
// FUNCTION NEEDED BY THE FRAMEWORK

NvAppBase* NvAppFactory()
{
	return new ThreadedRenderingVk();
}
