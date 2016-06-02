//----------------------------------------------------------------------------------
// File:        vk10-kepler\SkinningAppVk/SkinningAppVk.cpp
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
#include "SkinningAppVk.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NV/NvMath.h"

const static float TO_RADIANS = 0.01745329f;
#define ARRAY_SIZE(a) ( sizeof(a) / sizeof( (a)[0] ))

SkinningAppVk::SkinningAppVk()
	: mPipeline(VK_NULL_HANDLE)
	, mSingleBoneSkinning(false)
	, mPauseTime(false)
	, mTime(0.0f)
	, mRenderMode(0)        // 0: Render Color   1: Render Normals    2: Render Weights
{
	m_transformer->setRotationVec(nv::vec3f(0.0f, NV_PI*0.25f, 0.0f));
	m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -25.0f));
	m_transformer->setMaxTranslationVel(50.0f); // seems to work decently.

	// Required in all subclasses to avoid silent link issues
	forceLinkHack();
}

SkinningAppVk::~SkinningAppVk()
{
	LOGI("SkinningAppVk: destroyed\n");
}

void SkinningAppVk::configurationCallback(NvVKConfiguration& config)
{
	config.depthBits = 24;
	config.stencilBits = 0;
}

void SkinningAppVk::initRendering(void) {
	setGLDrawCallbacks(this);
	VkResult result = VK_ERROR_INITIALIZATION_FAILED;

	NvAssetLoaderAddSearchPath("vk10-kepler/SkinningAppVk");

	mMesh.initRendering(vk());

	SkinnedMesh::UBOBlock& ubo = *mMesh.mUBO;
	ubo.mRenderMode[2] = 0;

	ubo.mLightDir0 = nv::vec4f(0.267f, 0.535f, 0.802f, 0.0f);
	ubo.mLightDir1 = nv::vec4f(-0.408f, 0.816f, -0.408f, 0.0f);

	mMesh.mUBO.Update();

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &mMesh.mDescriptorSetLayout;
	result = vkCreatePipelineLayout(device(), &pipelineLayoutCreateInfo, 0, &mPipelineLayout);
	CHECK_VK_RESULT();

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
	rsStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rsStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	VkPipelineColorBlendAttachmentState attachments[1] = {};
	attachments[0].blendEnable = VK_FALSE;
	attachments[0].colorWriteMask = ~0;

	VkPipelineColorBlendStateCreateInfo cbStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	cbStateInfo.logicOpEnable = VK_FALSE;
	cbStateInfo.attachmentCount = ARRAY_SIZE(attachments);
	cbStateInfo.pAttachments = attachments;

	VkPipelineDepthStencilStateCreateInfo dsStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	dsStateInfo.depthTestEnable = VK_TRUE;
	dsStateInfo.depthWriteEnable = VK_TRUE;
	dsStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	dsStateInfo.depthBoundsTestEnable = VK_FALSE;
	dsStateInfo.stencilTestEnable = VK_FALSE;
	dsStateInfo.minDepthBounds = 0.0f;
	dsStateInfo.maxDepthBounds = 1.0f;

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

	VkPipelineShaderStageCreateInfo shaderStages[2];
	uint32_t shaderCount = 0;
#ifdef SOURCE_SHADERS
	shaderCount = vk().createShadersFromSourceFile(
		NvAssetLoadTextFile("src_shaders/skinning.glsl"), shaderStages, 2);
#else
	{
		int32_t length;
		char* data = NvAssetLoaderRead("shaders/skinning.nvs", length);
		shaderCount = vk().createShadersFromBinaryFile((uint32_t*)data,
			length, shaderStages, 2);
	}
#endif

	// Create mPipeline state VI-IA-VS-VP-RS-FS-CB
	VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pVertexInputState = &mMesh.mVIStateInfo;
	pipelineInfo.pInputAssemblyState = &mMesh.mIAStateInfo;
	pipelineInfo.pViewportState = &vpStateInfo;
	pipelineInfo.pRasterizationState = &rsStateInfo;
	pipelineInfo.pColorBlendState = &cbStateInfo;
	pipelineInfo.pDepthStencilState = &dsStateInfo;
	pipelineInfo.pMultisampleState = &msStateInfo;
	pipelineInfo.pTessellationState = &tessStateInfo;
	pipelineInfo.pDynamicState = &dynStateInfo;

	pipelineInfo.stageCount = shaderCount;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
	pipelineInfo.subpass = 0;

	pipelineInfo.layout = mPipelineLayout;

	result = vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &mPipeline);
	CHECK_VK_RESULT();

	mMesh.UpdateDescriptorSet(vk());
}



void SkinningAppVk::shutdownRendering(void) {

	vkDeviceWaitIdle(device());

	// destroy other resources here

	vkDestroyPipeline(device(), mPipeline, NULL);
}

void SkinningAppVk::initUI(void) {
	if (mTweakBar) {
		NvTweakVarBase *var;
		// expose the rendering modes
		NvTweakEnum<uint32_t> renderModes[] = {
			{ "Color", 0 },
			{ "Normals", 1 },
			{ "Weights", 2 }
		};
		var = mTweakBar->addEnum("Render Mode", mRenderMode, renderModes, TWEAKENUM_ARRAYSIZE(renderModes));
		addTweakKeyBind(var, NvKey::K_M);
		addTweakButtonBind(var, NvGamepad::BUTTON_Y);

		mTweakBar->addPadding();
		var = mTweakBar->addValue("Single Bone Skinning", mSingleBoneSkinning);
		addTweakKeyBind(var, NvKey::K_B);
		addTweakButtonBind(var, NvGamepad::BUTTON_X);

		mTweakBar->addPadding();
		mTimeScalar = 1.0f;
		var = mTweakBar->addValue("Animation Speed", mTimeScalar, 0, 5.0, 0.1f);
		addTweakKeyBind(var, NvKey::K_RBRACKET, NvKey::K_LBRACKET);
		addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER, NvGamepad::BUTTON_LEFT_SHOULDER);
	}
}

void SkinningAppVk::updateRenderCommands() {
}


void SkinningAppVk::reshape(int32_t width, int32_t height)
{
	updateRenderCommands();
}

////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinningApp::computeBones()
//
//   This function creates a simple "Look out! the zombies are coming" animation  
//
////////////////////////////////////////////////////////////////////////////////
void SkinningAppVk::computeBones(float t, SkinnedMesh::UBOBlock& ubo)
{
	// These are the bones in our simple skeleton
	// The indices dictate where the bones exist in the constant buffer
	const int32_t Back = 0;
	const int32_t UpperLeg_L = 1;
	const int32_t  LowerLeg_L = 2;
	const int32_t UpperLeg_R = 3;
	const int32_t  LowerLeg_R = 4;
	const int32_t UpperArm_L = 5;
	const int32_t  ForeArm_L = 6;
	const int32_t UpperArm_R = 7;
	const int32_t  ForeArm_R = 8;


	// Locations of the heads of the bones in modelspace
	float position_UpperLeg_L[3] = { 0.863f, 0.87f, 0.1f };
	float position_LowerLeg_L[3] = { 1.621f, -2.79f, 0.12f };
	float position_UpperLeg_R[3] = { -0.863f, 0.87f, 0.1f };
	float position_LowerLeg_R[3] = { -1.621f, -2.79f, 0.12f };

	float position_UpperArm_L[3] = { 1.70f, 5.81f, 0.3f };
	float position_LowerArm_L[3] = { 5.27f, 5.94f, 0.4f };
	float position_UpperArm_R[3] = { -1.70f, 5.81f, 0.3f };
	float position_LowerArm_R[3] = { -5.27f, 5.94f, 0.4f };


	nv::matrix4f M;
	nv::matrix4f temp;


	// Compute Back bone
	nv::rotationY(ubo.mBones[Back], (float)sin(2.0f * t) * 20.0f * TO_RADIANS);

	// Compute left leg bones (Both UpperLeg_L and LowerLeg_L)
	// Since we're using column vectors, the operations are applied in reverse order as read.
	// That's because if we have M2 * M1 * M0 * v, first M0 transforms v, then M1 transforms (M0 * v)
	// then M2 transforms (M1 * (M0 * v)). So the first transformation applied is by the last/right-most matrix.
	// F: Translate back to model space
	nv::translation(M, position_UpperLeg_L[0], position_UpperLeg_L[1], position_UpperLeg_L[2]);
	// E: Rotate the UpperLeg_L bone
	M *= nv::rotationX(temp, (float)sin(2.0f * t) * 20.0f * TO_RADIANS);
	// D: Translate to head of the UpperLeg_L bone where the rotation occurs
	M *= nv::translation(temp, -position_UpperLeg_L[0], -position_UpperLeg_L[1], -position_UpperLeg_L[2]);
	ubo.mBones[UpperLeg_L] = M;

	// C: Translate back to model space
	M *= nv::translation(temp, position_LowerLeg_L[0], position_LowerLeg_L[1], position_LowerLeg_L[2]);
	// B: Rotate the LowerLeg_L bone
	M *= nv::rotationX(temp, (1.0f + (float)sin(2.0f * t)) * 50.0f * TO_RADIANS);
	// A: Translate to head of the LowerLeg_L bone where the rotation occurs
	M *= nv::translation(temp, -position_LowerLeg_L[0], -position_LowerLeg_L[1], -position_LowerLeg_L[2]);
	ubo.mBones[LowerLeg_L] = M;

	// So if you read the transformations backwards, this is what happens to the UpperLeg_L bone
	// D: Translate to head of the UpperLeg_L bone where the rotation occurs
	// E: Rotate the UpperLeg_L bone
	// F: Translate back to model space

	// This is what happens to the LowerLeg_L bone
	// A: Translate to head of the LowerLeg_L bone where the rotation occurs
	// B: Rotate the LowerLeg_L bone
	// C: Translate back to model space
	// D: Translate to head of the UpperLeg_L bone where the rotation occurs
	// E: Rotate the UpperLeg_L bone
	// F: Translate back to model space



	// Compute right leg bones (Both UpperLeg_R and LowerLeg_R)
	nv::translation(M, position_UpperLeg_R[0], position_UpperLeg_R[1], position_UpperLeg_R[2]);
	M *= nv::rotationX(temp, (float)sin(2.0f * t + 3.14f) * 20.0f * TO_RADIANS);
	M *= nv::translation(temp, -position_UpperLeg_R[0], -position_UpperLeg_R[1], -position_UpperLeg_R[2]);
	ubo.mBones[UpperLeg_R] = M;

	M *= nv::translation(temp, position_LowerLeg_R[0], position_LowerLeg_R[1], position_LowerLeg_R[2]);
	M *= nv::rotationX(temp, (1.0f + (float)sin(2.0f * t + 3.14f)) * 50.0f * TO_RADIANS);
	M *= nv::translation(temp, -position_LowerLeg_R[0], -position_LowerLeg_R[1], -position_LowerLeg_R[2]);
	ubo.mBones[LowerLeg_R] = M;

	// Compute left arm bones (Both UpperArm_L and ForeArm_L)
	nv::translation(M, position_UpperArm_L[0], position_UpperArm_L[1], position_UpperArm_L[2]);
	M *= nv::rotationZ(temp, (10.0f + (float)sin(4.0f * t) * 40.0f) * TO_RADIANS);
	M *= nv::translation(temp, -position_UpperArm_L[0], -position_UpperArm_L[1], -position_UpperArm_L[2]);
	ubo.mBones[UpperArm_L] = M;

	M *= nv::translation(temp, position_LowerArm_L[0], position_LowerArm_L[1], position_LowerArm_L[2]);
	M *= nv::rotationX(temp, (-30.0f + -(1.0f + (float)sin(4.0f * t)) * 30.0f) * TO_RADIANS);
	M *= nv::translation(temp, -position_LowerArm_L[0], -position_LowerArm_L[1], -position_LowerArm_L[2]);
	ubo.mBones[ForeArm_L] = M;

	// Compute right arm bones (Both UpperArm_R and ForeArm_R)
	nv::translation(M, position_UpperArm_R[0], position_UpperArm_R[1], position_UpperArm_R[2]);
	M *= nv::rotationZ(temp, (-(10.0f + (float)sin(4.0f * t) * 40.0f)) * TO_RADIANS);
	M *= nv::translation(temp, -position_UpperArm_R[0], -position_UpperArm_R[1], -position_UpperArm_R[2]);
	ubo.mBones[UpperArm_R] = M;

	M *= nv::translation(temp, position_LowerArm_R[0], position_LowerArm_R[1], position_LowerArm_R[2]);
	M *= nv::rotationX(temp, (-30.0f + -(1.0f + (float)sin(4.0f * t)) * 30.0f) * TO_RADIANS);
	M *= nv::translation(temp, -position_LowerArm_R[0], -position_LowerArm_R[1], -position_LowerArm_R[2]);
	ubo.mBones[ForeArm_R] = M;
}

void SkinningAppVk::draw(void)
{
	VkResult result = VK_ERROR_INITIALIZATION_FAILED;

	NvSimpleUBO<SkinnedMesh::UBOBlock>& uboObj = mMesh.mUBO;
	SkinnedMesh::UBOBlock& ubo = *uboObj;

	// Compute and update the ModelViewProjection matrix
	nv::perspectiveLH(ubo.mMVP, 45.0f, (float)m_width / (float)m_height, 0.1f, 100.0f);
	
	ubo.mMVP *= m_transformer->getModelViewMat();

	// Compute and update the bone matrices
	computeBones(mTime, ubo);
	mTime += mTimeScalar * getFrameDeltaTime();

	// Update other uniforms
	ubo.mRenderMode[0] = (int32_t)mSingleBoneSkinning;
	ubo.mRenderMode[1] = (int32_t)mRenderMode;

	uboObj.Update();

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


	vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		// Bind the mPipeline state
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

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

		mMesh.draw(mPipelineLayout, cmd);
	}

	vkCmdEndRenderPass(cmd);

	vk().submitMainCommandBuffer();
}

void SkinningAppVk::initGLResources(NvAppContextGL* gl) {

}
void SkinningAppVk::drawGL(NvAppContextGL* gl) {

}

NvAppBase* NvAppFactory() {
	return new SkinningAppVk();
}

