//----------------------------------------------------------------------------------
// File:        vk10-kepler\HelloVulkan/HelloVulkan.cpp
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
#include "HelloVulkan.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#define ARRAY_SIZE(a) ( sizeof(a) / sizeof( (a)[0] ))

enum {
	ACTION_CHANGE_GEOM = 1,
	ACTION_CHANGE_CLEAR
};

HelloVulkan::HelloVulkan() 
    : mCmdPool(VK_NULL_HANDLE)
	, mCmd(VK_NULL_HANDLE)
	, mPipeline(VK_NULL_HANDLE)
	, mVertexBuffer()
	, mIndexBuffer()
    , mDrawGeometry(true)
    , mClearImage(false)
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -2.2f));
    m_transformer->setRotationVec(nv::vec3f(NV_PI*0.35f, 0.0f, 0.0f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

HelloVulkan::~HelloVulkan()
{
    LOGI("HelloVulkan: destroyed\n");
}

void HelloVulkan::configurationCallback(NvVKConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
}

VkPipelineLayout pipelineLayout;

void HelloVulkan::initRendering(void) {
	VkResult result = VK_ERROR_INITIALIZATION_FAILED;

    NvAssetLoaderAddSearchPath("vk10-kepler/HelloVulkan");

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = NULL;
	result = vkCreatePipelineLayout(device(), &pipelineLayoutCreateInfo, 0, &pipelineLayout);
	CHECK_VK_RESULT();
	
    {
        VkCommandPoolCreateInfo cmdPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        cmdPoolInfo.queueFamilyIndex = 0;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        result = vkCreateCommandPool(device(), &cmdPoolInfo, NULL, &mCmdPool);
        CHECK_VK_RESULT();

    }

     static struct Vertex {
        float   position[2];
        uint8_t color[4];
    } const vertices[3] = {
        { { -0.5f, -0.5f }, { 0xFF, 0x00, 0x00, 0xFF }, },
        { { 0.5f, -0.5f }, { 0x00, 0xFF, 0x00, 0xFF }, },
        { { 0.5f, 0.5f }, { 0x00, 0x00, 0xFF, 0xFF }, },
    };
    static uint32_t indices[3] = { 0, 1, 2 };

	// Create the main command buffer.
	VkCommandBufferAllocateInfo cmdInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmdInfo.commandPool = mCmdPool;
	cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	cmdInfo.commandBufferCount = 1;
	result = vkAllocateCommandBuffers(device(), &cmdInfo, &mCmd);
	CHECK_VK_RESULT();

	// Create the vertex buffer
	result = vk().createAndFillBuffer(sizeof vertices,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mVertexBuffer, vertices);
	CHECK_VK_RESULT();
    
    // Create the index buffer
	result = vk().createAndFillBuffer(sizeof indices,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mIndexBuffer, indices);
    CHECK_VK_RESULT();
	
    // Create static state info for the mPipeline.
    VkVertexInputBindingDescription vertexBindings[1] = {};
    vertexBindings[0].binding = 0;
	vertexBindings[0].stride = sizeof vertices[0];
	vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attributes[2] = {};
    attributes[0].location = 0;
    attributes[0].binding = 0;
    attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[0].offset = offsetof(Vertex, position);
    attributes[1].location = 1;
    attributes[1].binding = 0;
    attributes[1].format = VK_FORMAT_R8G8B8A8_UNORM;
    attributes[1].offset = offsetof(Vertex, color);

	VkPipelineVertexInputStateCreateInfo viStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	viStateInfo.vertexBindingDescriptionCount = ARRAY_SIZE(vertexBindings);
	viStateInfo.pVertexBindingDescriptions = vertexBindings;
	viStateInfo.vertexAttributeDescriptionCount = ARRAY_SIZE(attributes);
    viStateInfo.pVertexAttributeDescriptions = attributes;
	
    VkPipelineInputAssemblyStateCreateInfo iaStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    iaStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    iaStateInfo.primitiveRestartEnable = VK_FALSE;

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

    VkPipelineShaderStageCreateInfo shaderStages[2];
	uint32_t shaderCount = 0;
#ifdef SOURCE_SHADERS
	shaderCount = vk().createShadersFromSourceFile(
		NvAssetLoadTextFile("src_shaders/simple.glsl"), shaderStages, 2);
#else
	{
		int32_t length;
		char* data = NvAssetLoaderRead("shaders/simple.nvs", length);
		shaderCount = vk().createShadersFromBinaryFile((uint32_t*)data,
			length, shaderStages, 2);
	}
#endif


	VkPipelineDepthStencilStateCreateInfo noDepth;
	noDepth = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	noDepth.depthTestEnable = VK_FALSE;
	noDepth.depthWriteEnable = VK_FALSE;
	noDepth.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	noDepth.depthBoundsTestEnable = VK_FALSE;
	noDepth.stencilTestEnable = VK_FALSE;
	noDepth.minDepthBounds = 0.0f;
	noDepth.maxDepthBounds = 1.0f;

	// Create mPipeline state VI-IA-VS-VP-RS-FS-CB
	VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pVertexInputState = &viStateInfo;
	pipelineInfo.pInputAssemblyState = &iaStateInfo;
	pipelineInfo.pViewportState = &vpStateInfo;
	pipelineInfo.pRasterizationState = &rsStateInfo;
	pipelineInfo.pColorBlendState = &cbStateInfo;
	pipelineInfo.pMultisampleState = &msStateInfo;
	pipelineInfo.pTessellationState = &tessStateInfo;
	pipelineInfo.pDynamicState = &dynStateInfo;
	pipelineInfo.pDepthStencilState = &noDepth;

	pipelineInfo.stageCount = shaderCount;
	pipelineInfo.pStages = shaderStages;
	
	pipelineInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
	pipelineInfo.subpass = 0;
	
	pipelineInfo.layout = pipelineLayout;

	result = vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &mPipeline);
    CHECK_VK_RESULT();
}



void HelloVulkan::shutdownRendering(void) {

    vkDeviceWaitIdle(device());

    // destroy other resources here
	vkDestroyPipeline(device(), mPipeline, NULL);

    if (mVertexBuffer() != VK_NULL_HANDLE)
		vkDestroyBuffer(device(), mVertexBuffer(), NULL);
    if (mIndexBuffer()  != VK_NULL_HANDLE)
		vkDestroyBuffer(device(), mIndexBuffer(), NULL);

	vkDestroyCommandPool(device(), mCmdPool, NULL);

    if (mVertexBuffer.mem != VK_NULL_HANDLE)
		vkFreeMemory(device(), mVertexBuffer.mem, NULL);

	if (mIndexBuffer.mem != VK_NULL_HANDLE)
		vkFreeMemory(device(), mIndexBuffer.mem, NULL);
}

void HelloVulkan::initUI(void) {
    if (mTweakBar) {
        NvTweakVarBase *var;

		var = mTweakBar->addValue("Draw Geometry", mDrawGeometry, ACTION_CHANGE_GEOM);
        addTweakKeyBind(var, NvKey::K_D);

		var = mTweakBar->addValue("Clear Image", mClearImage, ACTION_CHANGE_CLEAR);
        addTweakKeyBind(var, NvKey::K_C);

        mTweakBar->syncValues();
    }
}

NvUIEventResponse HelloVulkan::handleReaction(const NvUIReaction &react)
{
	switch (react.code) {
	case ACTION_CHANGE_GEOM:
	case ACTION_CHANGE_CLEAR:
		updateRenderCommands();
		return nvuiEventHandled;
	}
	return nvuiEventNotHandled;
}


void HelloVulkan::updateRenderCommands() {
	VkResult result = VK_ERROR_INITIALIZATION_FAILED;

	result = vkDeviceWaitIdle(device());
	CHECK_VK_RESULT();

	VkCommandBufferInheritanceInfo inherit = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
	inherit.framebuffer = vk().mainRenderTarget()->frameBuffer();
	inherit.renderPass = vk().mainRenderTarget()->clearRenderPass();

	// Record the commands (resets the buffer)
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	beginInfo.pInheritanceInfo = &inherit;

	result = vkBeginCommandBuffer(mCmd, &beginInfo);
	CHECK_VK_RESULT();

	// Bind the mPipeline state
	vkCmdBindPipeline(mCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

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

		vkCmdSetViewport(mCmd, 0, 1, &vp);
		vkCmdSetScissor(mCmd, 0, 1, &sc);

		if (mDrawGeometry)
		{
			// Bind the vertex and index buffers
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(mCmd, 0, 1, &mVertexBuffer(), offsets);
			vkCmdBindIndexBuffer(mCmd, mIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			// Draw the triangle
			vkCmdDrawIndexed(mCmd, 3, 1, 0, 0, 0);
		}
	}

	result = vkEndCommandBuffer(mCmd);
	CHECK_VK_RESULT();
}


void HelloVulkan::reshape(int32_t width, int32_t height)
{
	updateRenderCommands();
}

void HelloVulkan::draw(void)
{
	VkResult result = VK_ERROR_INITIALIZATION_FAILED;

	VkCommandBuffer cmd = vk().getMainCommandBuffer();

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

	renderPassBeginInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
	renderPassBeginInfo.framebuffer = vk().mainRenderTarget()->frameBuffer();
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_width;
	renderPassBeginInfo.renderArea.extent.height = m_height;

	VkClearValue clearValues[1];
	clearValues[0].color.float32[0] = 0.66f;
	clearValues[0].color.float32[1] = 0.33f;
	clearValues[0].color.float32[2] = 0.44f;
	clearValues[0].color.float32[3] = 1.0f;


	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.clearValueCount = 1;

	vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	vkCmdExecuteCommands(cmd, 1, &mCmd);

	vkCmdEndRenderPass(cmd);
	if (mClearImage)
	{
		VkClearColorValue color = {};
		color.float32[0] = 0.66f;
		color.float32[1] = 0.44f;
		color.float32[2] = 1.0f;
		color.float32[3] = 1.0f;
		VkImageSubresourceRange range = {};
		range.levelCount = ~0;
		range.layerCount = ~0;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vkCmdClearColorImage(cmd, vk().mainRenderTarget()->image(), VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);
	}

	vk().submitMainCommandBuffer();
}

NvAppBase* NvAppFactory() {
    return new HelloVulkan();
}



