//----------------------------------------------------------------------------------
// File:        NvVkUtil/vkfnptrinline.c
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

#ifdef ANDROID

#include "vulkannv.h"
#include "vkfnptrinline.h"

PFN_vkQueueSemaphoreWaitNV pfn_vkQueueSemaphoreWaitNV;
PFN_vkQueuePresentNV pfn_vkQueuePresentNV;
PFN_vkQueuePresentNoWaitNV pfn_vkQueuePresentNoWaitNV;
PFN_vkSignalPresentDoneNV pfn_vkSignalPresentDoneNV;
PFN_vkCreateInstance pfn_vkCreateInstance;
PFN_vkDestroyInstance pfn_vkDestroyInstance;
PFN_vkEnumeratePhysicalDevices pfn_vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceFeatures pfn_vkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceFormatProperties pfn_vkGetPhysicalDeviceFormatProperties;
PFN_vkGetPhysicalDeviceImageFormatProperties pfn_vkGetPhysicalDeviceImageFormatProperties;
PFN_vkGetPhysicalDeviceProperties pfn_vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceQueueFamilyProperties pfn_vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkGetPhysicalDeviceMemoryProperties pfn_vkGetPhysicalDeviceMemoryProperties;
PFN_vkGetInstanceProcAddr pfn_vkGetInstanceProcAddr;
PFN_vkGetDeviceProcAddr pfn_vkGetDeviceProcAddr;
PFN_vkCreateDevice pfn_vkCreateDevice;
PFN_vkDestroyDevice pfn_vkDestroyDevice;
PFN_vkEnumerateInstanceExtensionProperties pfn_vkEnumerateInstanceExtensionProperties;
PFN_vkEnumerateDeviceExtensionProperties pfn_vkEnumerateDeviceExtensionProperties;
PFN_vkEnumerateInstanceLayerProperties pfn_vkEnumerateInstanceLayerProperties;
PFN_vkEnumerateDeviceLayerProperties pfn_vkEnumerateDeviceLayerProperties;
PFN_vkGetDeviceQueue pfn_vkGetDeviceQueue;
PFN_vkQueueSubmit pfn_vkQueueSubmit;
PFN_vkQueueWaitIdle pfn_vkQueueWaitIdle;
PFN_vkDeviceWaitIdle pfn_vkDeviceWaitIdle;
PFN_vkAllocateMemory pfn_vkAllocateMemory;
PFN_vkFreeMemory pfn_vkFreeMemory;
PFN_vkMapMemory pfn_vkMapMemory;
PFN_vkUnmapMemory pfn_vkUnmapMemory;
PFN_vkFlushMappedMemoryRanges pfn_vkFlushMappedMemoryRanges;
PFN_vkInvalidateMappedMemoryRanges pfn_vkInvalidateMappedMemoryRanges;
PFN_vkGetDeviceMemoryCommitment pfn_vkGetDeviceMemoryCommitment;
PFN_vkBindBufferMemory pfn_vkBindBufferMemory;
PFN_vkBindImageMemory pfn_vkBindImageMemory;
PFN_vkGetBufferMemoryRequirements pfn_vkGetBufferMemoryRequirements;
PFN_vkGetImageMemoryRequirements pfn_vkGetImageMemoryRequirements;
PFN_vkGetImageSparseMemoryRequirements pfn_vkGetImageSparseMemoryRequirements;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties pfn_vkGetPhysicalDeviceSparseImageFormatProperties;
PFN_vkQueueBindSparse pfn_vkQueueBindSparse;
PFN_vkCreateFence pfn_vkCreateFence;
PFN_vkDestroyFence pfn_vkDestroyFence;
PFN_vkResetFences pfn_vkResetFences;
PFN_vkGetFenceStatus pfn_vkGetFenceStatus;
PFN_vkWaitForFences pfn_vkWaitForFences;
PFN_vkCreateSemaphore pfn_vkCreateSemaphore;
PFN_vkDestroySemaphore pfn_vkDestroySemaphore;
PFN_vkCreateEvent pfn_vkCreateEvent;
PFN_vkDestroyEvent pfn_vkDestroyEvent;
PFN_vkGetEventStatus pfn_vkGetEventStatus;
PFN_vkSetEvent pfn_vkSetEvent;
PFN_vkResetEvent pfn_vkResetEvent;
PFN_vkCreateQueryPool pfn_vkCreateQueryPool;
PFN_vkDestroyQueryPool pfn_vkDestroyQueryPool;
PFN_vkGetQueryPoolResults pfn_vkGetQueryPoolResults;
PFN_vkCreateBuffer pfn_vkCreateBuffer;
PFN_vkDestroyBuffer pfn_vkDestroyBuffer;
PFN_vkCreateBufferView pfn_vkCreateBufferView;
PFN_vkDestroyBufferView pfn_vkDestroyBufferView;
PFN_vkCreateImage pfn_vkCreateImage;
PFN_vkDestroyImage pfn_vkDestroyImage;
PFN_vkGetImageSubresourceLayout pfn_vkGetImageSubresourceLayout;
PFN_vkCreateImageView pfn_vkCreateImageView;
PFN_vkDestroyImageView pfn_vkDestroyImageView;
PFN_vkCreateShaderModule pfn_vkCreateShaderModule;
PFN_vkDestroyShaderModule pfn_vkDestroyShaderModule;
PFN_vkCreatePipelineCache pfn_vkCreatePipelineCache;
PFN_vkDestroyPipelineCache pfn_vkDestroyPipelineCache;
PFN_vkGetPipelineCacheData pfn_vkGetPipelineCacheData;
PFN_vkMergePipelineCaches pfn_vkMergePipelineCaches;
PFN_vkCreateGraphicsPipelines pfn_vkCreateGraphicsPipelines;
PFN_vkCreateComputePipelines pfn_vkCreateComputePipelines;
PFN_vkDestroyPipeline pfn_vkDestroyPipeline;
PFN_vkCreatePipelineLayout pfn_vkCreatePipelineLayout;
PFN_vkDestroyPipelineLayout pfn_vkDestroyPipelineLayout;
PFN_vkCreateSampler pfn_vkCreateSampler;
PFN_vkDestroySampler pfn_vkDestroySampler;
PFN_vkCreateDescriptorSetLayout pfn_vkCreateDescriptorSetLayout;
PFN_vkDestroyDescriptorSetLayout pfn_vkDestroyDescriptorSetLayout;
PFN_vkCreateDescriptorPool pfn_vkCreateDescriptorPool;
PFN_vkDestroyDescriptorPool pfn_vkDestroyDescriptorPool;
PFN_vkResetDescriptorPool pfn_vkResetDescriptorPool;
PFN_vkAllocateDescriptorSets pfn_vkAllocateDescriptorSets;
PFN_vkFreeDescriptorSets pfn_vkFreeDescriptorSets;
PFN_vkUpdateDescriptorSets pfn_vkUpdateDescriptorSets;
PFN_vkCreateFramebuffer pfn_vkCreateFramebuffer;
PFN_vkDestroyFramebuffer pfn_vkDestroyFramebuffer;
PFN_vkCreateRenderPass pfn_vkCreateRenderPass;
PFN_vkDestroyRenderPass pfn_vkDestroyRenderPass;
PFN_vkGetRenderAreaGranularity pfn_vkGetRenderAreaGranularity;
PFN_vkCreateCommandPool pfn_vkCreateCommandPool;
PFN_vkDestroyCommandPool pfn_vkDestroyCommandPool;
PFN_vkResetCommandPool pfn_vkResetCommandPool;
PFN_vkAllocateCommandBuffers pfn_vkAllocateCommandBuffers;
PFN_vkFreeCommandBuffers pfn_vkFreeCommandBuffers;
PFN_vkBeginCommandBuffer pfn_vkBeginCommandBuffer;
PFN_vkEndCommandBuffer pfn_vkEndCommandBuffer;
PFN_vkResetCommandBuffer pfn_vkResetCommandBuffer;
PFN_vkCmdBindPipeline pfn_vkCmdBindPipeline;
PFN_vkCmdSetViewport pfn_vkCmdSetViewport;
PFN_vkCmdSetScissor pfn_vkCmdSetScissor;
PFN_vkCmdSetLineWidth pfn_vkCmdSetLineWidth;
PFN_vkCmdSetDepthBias pfn_vkCmdSetDepthBias;
PFN_vkCmdSetBlendConstants pfn_vkCmdSetBlendConstants;
PFN_vkCmdSetDepthBounds pfn_vkCmdSetDepthBounds;
PFN_vkCmdSetStencilCompareMask pfn_vkCmdSetStencilCompareMask;
PFN_vkCmdSetStencilWriteMask pfn_vkCmdSetStencilWriteMask;
PFN_vkCmdSetStencilReference pfn_vkCmdSetStencilReference;
PFN_vkCmdBindDescriptorSets pfn_vkCmdBindDescriptorSets;
PFN_vkCmdBindIndexBuffer pfn_vkCmdBindIndexBuffer;
PFN_vkCmdBindVertexBuffers pfn_vkCmdBindVertexBuffers;
PFN_vkCmdDraw pfn_vkCmdDraw;
PFN_vkCmdDrawIndexed pfn_vkCmdDrawIndexed;
PFN_vkCmdDrawIndirect pfn_vkCmdDrawIndirect;
PFN_vkCmdDrawIndexedIndirect pfn_vkCmdDrawIndexedIndirect;
PFN_vkCmdDispatch pfn_vkCmdDispatch;
PFN_vkCmdDispatchIndirect pfn_vkCmdDispatchIndirect;
PFN_vkCmdCopyBuffer pfn_vkCmdCopyBuffer;
PFN_vkCmdCopyImage pfn_vkCmdCopyImage;
PFN_vkCmdBlitImage pfn_vkCmdBlitImage;
PFN_vkCmdCopyBufferToImage pfn_vkCmdCopyBufferToImage;
PFN_vkCmdCopyImageToBuffer pfn_vkCmdCopyImageToBuffer;
PFN_vkCmdUpdateBuffer pfn_vkCmdUpdateBuffer;
PFN_vkCmdFillBuffer pfn_vkCmdFillBuffer;
PFN_vkCmdClearColorImage pfn_vkCmdClearColorImage;
PFN_vkCmdClearDepthStencilImage pfn_vkCmdClearDepthStencilImage;
PFN_vkCmdClearAttachments pfn_vkCmdClearAttachments;
PFN_vkCmdResolveImage pfn_vkCmdResolveImage;
PFN_vkCmdSetEvent pfn_vkCmdSetEvent;
PFN_vkCmdResetEvent pfn_vkCmdResetEvent;
PFN_vkCmdWaitEvents pfn_vkCmdWaitEvents;
PFN_vkCmdPipelineBarrier pfn_vkCmdPipelineBarrier;
PFN_vkCmdBeginQuery pfn_vkCmdBeginQuery;
PFN_vkCmdEndQuery pfn_vkCmdEndQuery;
PFN_vkCmdResetQueryPool pfn_vkCmdResetQueryPool;
PFN_vkCmdWriteTimestamp pfn_vkCmdWriteTimestamp;
PFN_vkCmdCopyQueryPoolResults pfn_vkCmdCopyQueryPoolResults;
PFN_vkCmdPushConstants pfn_vkCmdPushConstants;
PFN_vkCmdBeginRenderPass pfn_vkCmdBeginRenderPass;
PFN_vkCmdNextSubpass pfn_vkCmdNextSubpass;
PFN_vkCmdEndRenderPass pfn_vkCmdEndRenderPass;
PFN_vkCmdExecuteCommands pfn_vkCmdExecuteCommands;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR pfn_vkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfn_vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pfn_vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pfn_vkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkCreateSwapchainKHR pfn_vkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR pfn_vkDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR pfn_vkGetSwapchainImagesKHR;
PFN_vkAcquireNextImageKHR pfn_vkAcquireNextImageKHR;
PFN_vkQueuePresentKHR pfn_vkQueuePresentKHR;


void vkLoadProcs(PFN_vkGetProcAddressNV getProc)
{
    pfn_vkQueueSemaphoreWaitNV = (PFN_vkQueueSemaphoreWaitNV)getProc("vkQueueSemaphoreWaitNV");
    pfn_vkQueuePresentNV = (PFN_vkQueuePresentNV)getProc("vkQueuePresentNV");
    pfn_vkQueuePresentNoWaitNV = (PFN_vkQueuePresentNoWaitNV)getProc("vkQueuePresentNoWaitNV");
    pfn_vkSignalPresentDoneNV = (PFN_vkSignalPresentDoneNV)getProc("vkSignalPresentDoneNV");
    pfn_vkCreateInstance = (PFN_vkCreateInstance)getProc("vkCreateInstance");
    pfn_vkDestroyInstance = (PFN_vkDestroyInstance)getProc("vkDestroyInstance");
    pfn_vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)getProc("vkEnumeratePhysicalDevices");
    pfn_vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)getProc("vkGetPhysicalDeviceFeatures");
    pfn_vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)getProc("vkGetPhysicalDeviceFormatProperties");
    pfn_vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties)getProc("vkGetPhysicalDeviceImageFormatProperties");
    pfn_vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)getProc("vkGetPhysicalDeviceProperties");
    pfn_vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)getProc("vkGetPhysicalDeviceQueueFamilyProperties");
    pfn_vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)getProc("vkGetPhysicalDeviceMemoryProperties");
    pfn_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)getProc("vkGetInstanceProcAddr");
    pfn_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)getProc("vkGetDeviceProcAddr");
    pfn_vkCreateDevice = (PFN_vkCreateDevice)getProc("vkCreateDevice");
    pfn_vkDestroyDevice = (PFN_vkDestroyDevice)getProc("vkDestroyDevice");
    pfn_vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)getProc("vkEnumerateInstanceExtensionProperties");
    pfn_vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)getProc("vkEnumerateDeviceExtensionProperties");
    pfn_vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)getProc("vkEnumerateInstanceLayerProperties");
    pfn_vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)getProc("vkEnumerateDeviceLayerProperties");
    pfn_vkGetDeviceQueue = (PFN_vkGetDeviceQueue)getProc("vkGetDeviceQueue");
    pfn_vkQueueSubmit = (PFN_vkQueueSubmit)getProc("vkQueueSubmit");
    pfn_vkQueueWaitIdle = (PFN_vkQueueWaitIdle)getProc("vkQueueWaitIdle");
    pfn_vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)getProc("vkDeviceWaitIdle");
    pfn_vkAllocateMemory = (PFN_vkAllocateMemory)getProc("vkAllocateMemory");
    pfn_vkFreeMemory = (PFN_vkFreeMemory)getProc("vkFreeMemory");
    pfn_vkMapMemory = (PFN_vkMapMemory)getProc("vkMapMemory");
    pfn_vkUnmapMemory = (PFN_vkUnmapMemory)getProc("vkUnmapMemory");
    pfn_vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)getProc("vkFlushMappedMemoryRanges");
    pfn_vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)getProc("vkInvalidateMappedMemoryRanges");
    pfn_vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)getProc("vkGetDeviceMemoryCommitment");
    pfn_vkBindBufferMemory = (PFN_vkBindBufferMemory)getProc("vkBindBufferMemory");
    pfn_vkBindImageMemory = (PFN_vkBindImageMemory)getProc("vkBindImageMemory");
    pfn_vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)getProc("vkGetBufferMemoryRequirements");
    pfn_vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)getProc("vkGetImageMemoryRequirements");
    pfn_vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)getProc("vkGetImageSparseMemoryRequirements");
    pfn_vkGetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties)getProc("vkGetPhysicalDeviceSparseImageFormatProperties");
    pfn_vkQueueBindSparse = (PFN_vkQueueBindSparse)getProc("vkQueueBindSparse");
    pfn_vkCreateFence = (PFN_vkCreateFence)getProc("vkCreateFence");
    pfn_vkDestroyFence = (PFN_vkDestroyFence)getProc("vkDestroyFence");
    pfn_vkResetFences = (PFN_vkResetFences)getProc("vkResetFences");
    pfn_vkGetFenceStatus = (PFN_vkGetFenceStatus)getProc("vkGetFenceStatus");
    pfn_vkWaitForFences = (PFN_vkWaitForFences)getProc("vkWaitForFences");
    pfn_vkCreateSemaphore = (PFN_vkCreateSemaphore)getProc("vkCreateSemaphore");
    pfn_vkDestroySemaphore = (PFN_vkDestroySemaphore)getProc("vkDestroySemaphore");
    pfn_vkCreateEvent = (PFN_vkCreateEvent)getProc("vkCreateEvent");
    pfn_vkDestroyEvent = (PFN_vkDestroyEvent)getProc("vkDestroyEvent");
    pfn_vkGetEventStatus = (PFN_vkGetEventStatus)getProc("vkGetEventStatus");
    pfn_vkSetEvent = (PFN_vkSetEvent)getProc("vkSetEvent");
    pfn_vkResetEvent = (PFN_vkResetEvent)getProc("vkResetEvent");
    pfn_vkCreateQueryPool = (PFN_vkCreateQueryPool)getProc("vkCreateQueryPool");
    pfn_vkDestroyQueryPool = (PFN_vkDestroyQueryPool)getProc("vkDestroyQueryPool");
    pfn_vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)getProc("vkGetQueryPoolResults");
    pfn_vkCreateBuffer = (PFN_vkCreateBuffer)getProc("vkCreateBuffer");
    pfn_vkDestroyBuffer = (PFN_vkDestroyBuffer)getProc("vkDestroyBuffer");
    pfn_vkCreateBufferView = (PFN_vkCreateBufferView)getProc("vkCreateBufferView");
    pfn_vkDestroyBufferView = (PFN_vkDestroyBufferView)getProc("vkDestroyBufferView");
    pfn_vkCreateImage = (PFN_vkCreateImage)getProc("vkCreateImage");
    pfn_vkDestroyImage = (PFN_vkDestroyImage)getProc("vkDestroyImage");
    pfn_vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)getProc("vkGetImageSubresourceLayout");
    pfn_vkCreateImageView = (PFN_vkCreateImageView)getProc("vkCreateImageView");
    pfn_vkDestroyImageView = (PFN_vkDestroyImageView)getProc("vkDestroyImageView");
    pfn_vkCreateShaderModule = (PFN_vkCreateShaderModule)getProc("vkCreateShaderModule");
    pfn_vkDestroyShaderModule = (PFN_vkDestroyShaderModule)getProc("vkDestroyShaderModule");
    pfn_vkCreatePipelineCache = (PFN_vkCreatePipelineCache)getProc("vkCreatePipelineCache");
    pfn_vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)getProc("vkDestroyPipelineCache");
    pfn_vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)getProc("vkGetPipelineCacheData");
    pfn_vkMergePipelineCaches = (PFN_vkMergePipelineCaches)getProc("vkMergePipelineCaches");
    pfn_vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)getProc("vkCreateGraphicsPipelines");
    pfn_vkCreateComputePipelines = (PFN_vkCreateComputePipelines)getProc("vkCreateComputePipelines");
    pfn_vkDestroyPipeline = (PFN_vkDestroyPipeline)getProc("vkDestroyPipeline");
    pfn_vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)getProc("vkCreatePipelineLayout");
    pfn_vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)getProc("vkDestroyPipelineLayout");
    pfn_vkCreateSampler = (PFN_vkCreateSampler)getProc("vkCreateSampler");
    pfn_vkDestroySampler = (PFN_vkDestroySampler)getProc("vkDestroySampler");
    pfn_vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)getProc("vkCreateDescriptorSetLayout");
    pfn_vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)getProc("vkDestroyDescriptorSetLayout");
    pfn_vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)getProc("vkCreateDescriptorPool");
    pfn_vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)getProc("vkDestroyDescriptorPool");
    pfn_vkResetDescriptorPool = (PFN_vkResetDescriptorPool)getProc("vkResetDescriptorPool");
    pfn_vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)getProc("vkAllocateDescriptorSets");
    pfn_vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)getProc("vkFreeDescriptorSets");
    pfn_vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)getProc("vkUpdateDescriptorSets");
    pfn_vkCreateFramebuffer = (PFN_vkCreateFramebuffer)getProc("vkCreateFramebuffer");
    pfn_vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)getProc("vkDestroyFramebuffer");
    pfn_vkCreateRenderPass = (PFN_vkCreateRenderPass)getProc("vkCreateRenderPass");
    pfn_vkDestroyRenderPass = (PFN_vkDestroyRenderPass)getProc("vkDestroyRenderPass");
    pfn_vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)getProc("vkGetRenderAreaGranularity");
    pfn_vkCreateCommandPool = (PFN_vkCreateCommandPool)getProc("vkCreateCommandPool");
    pfn_vkDestroyCommandPool = (PFN_vkDestroyCommandPool)getProc("vkDestroyCommandPool");
    pfn_vkResetCommandPool = (PFN_vkResetCommandPool)getProc("vkResetCommandPool");
    pfn_vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)getProc("vkAllocateCommandBuffers");
    pfn_vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)getProc("vkFreeCommandBuffers");
    pfn_vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)getProc("vkBeginCommandBuffer");
    pfn_vkEndCommandBuffer = (PFN_vkEndCommandBuffer)getProc("vkEndCommandBuffer");
    pfn_vkResetCommandBuffer = (PFN_vkResetCommandBuffer)getProc("vkResetCommandBuffer");
    pfn_vkCmdBindPipeline = (PFN_vkCmdBindPipeline)getProc("vkCmdBindPipeline");
    pfn_vkCmdSetViewport = (PFN_vkCmdSetViewport)getProc("vkCmdSetViewport");
    pfn_vkCmdSetScissor = (PFN_vkCmdSetScissor)getProc("vkCmdSetScissor");
    pfn_vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)getProc("vkCmdSetLineWidth");
    pfn_vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)getProc("vkCmdSetDepthBias");
    pfn_vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)getProc("vkCmdSetBlendConstants");
    pfn_vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)getProc("vkCmdSetDepthBounds");
    pfn_vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)getProc("vkCmdSetStencilCompareMask");
    pfn_vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)getProc("vkCmdSetStencilWriteMask");
    pfn_vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)getProc("vkCmdSetStencilReference");
    pfn_vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)getProc("vkCmdBindDescriptorSets");
    pfn_vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)getProc("vkCmdBindIndexBuffer");
    pfn_vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)getProc("vkCmdBindVertexBuffers");
    pfn_vkCmdDraw = (PFN_vkCmdDraw)getProc("vkCmdDraw");
    pfn_vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)getProc("vkCmdDrawIndexed");
    pfn_vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)getProc("vkCmdDrawIndirect");
    pfn_vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)getProc("vkCmdDrawIndexedIndirect");
    pfn_vkCmdDispatch = (PFN_vkCmdDispatch)getProc("vkCmdDispatch");
    pfn_vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)getProc("vkCmdDispatchIndirect");
    pfn_vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)getProc("vkCmdCopyBuffer");
    pfn_vkCmdCopyImage = (PFN_vkCmdCopyImage)getProc("vkCmdCopyImage");
    pfn_vkCmdBlitImage = (PFN_vkCmdBlitImage)getProc("vkCmdBlitImage");
    pfn_vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)getProc("vkCmdCopyBufferToImage");
    pfn_vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)getProc("vkCmdCopyImageToBuffer");
    pfn_vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)getProc("vkCmdUpdateBuffer");
    pfn_vkCmdFillBuffer = (PFN_vkCmdFillBuffer)getProc("vkCmdFillBuffer");
    pfn_vkCmdClearColorImage = (PFN_vkCmdClearColorImage)getProc("vkCmdClearColorImage");
    pfn_vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)getProc("vkCmdClearDepthStencilImage");
    pfn_vkCmdClearAttachments = (PFN_vkCmdClearAttachments)getProc("vkCmdClearAttachments");
    pfn_vkCmdResolveImage = (PFN_vkCmdResolveImage)getProc("vkCmdResolveImage");
    pfn_vkCmdSetEvent = (PFN_vkCmdSetEvent)getProc("vkCmdSetEvent");
    pfn_vkCmdResetEvent = (PFN_vkCmdResetEvent)getProc("vkCmdResetEvent");
    pfn_vkCmdWaitEvents = (PFN_vkCmdWaitEvents)getProc("vkCmdWaitEvents");
    pfn_vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)getProc("vkCmdPipelineBarrier");
    pfn_vkCmdBeginQuery = (PFN_vkCmdBeginQuery)getProc("vkCmdBeginQuery");
    pfn_vkCmdEndQuery = (PFN_vkCmdEndQuery)getProc("vkCmdEndQuery");
    pfn_vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)getProc("vkCmdResetQueryPool");
    pfn_vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)getProc("vkCmdWriteTimestamp");
    pfn_vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)getProc("vkCmdCopyQueryPoolResults");
    pfn_vkCmdPushConstants = (PFN_vkCmdPushConstants)getProc("vkCmdPushConstants");
    pfn_vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)getProc("vkCmdBeginRenderPass");
    pfn_vkCmdNextSubpass = (PFN_vkCmdNextSubpass)getProc("vkCmdNextSubpass");
    pfn_vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)getProc("vkCmdEndRenderPass");
    pfn_vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)getProc("vkCmdExecuteCommands");
    pfn_vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)getProc("vkGetPhysicalDeviceSurfaceSupportKHR");
    pfn_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)getProc("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pfn_vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)getProc("vkGetPhysicalDeviceSurfaceFormatsKHR");
    pfn_vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)getProc("vkGetPhysicalDeviceSurfacePresentModesKHR");
    pfn_vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)getProc("vkCreateSwapchainKHR");
    pfn_vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)getProc("vkDestroySwapchainKHR");
    pfn_vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)getProc("vkGetSwapchainImagesKHR");
    pfn_vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)getProc("vkAcquireNextImageKHR");
    pfn_vkQueuePresentKHR = (PFN_vkQueuePresentKHR)getProc("vkQueuePresentKHR");

} // vkLoadProcs

#endif
