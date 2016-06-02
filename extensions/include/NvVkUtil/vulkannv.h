//----------------------------------------------------------------------------------
// File:        NvVkUtil/vulkannv.h
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
//
// Extra NVIDIA declarations and prototypes specific to our implementation.
//

#ifndef __VULKANNV_H__
#define __VULKANNV_H__

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C"
{
#endif

// TODO: These enum values need to be finalized from the NVIDIA
// enum range for Vulkan. They are placeholds for now. Also
// it is TBD whether we even want to define our own return codes.
#define VK_ERROR_INVALID_PARAMETER_NV VkResult(-1000)
#define VK_ERROR_INVALID_ALIGNMENT_NV VkResult(-1001)
#define VK_ERROR_INVALID_SHADER_NV    VkResult(-1002)

typedef PFN_vkVoidFunction (VKAPI_CALL * PFN_vkGetProcAddressNV) (const char *name);
typedef void (VKAPI_PTR * PFN_vkQueueSemaphoreWaitNV)(VkQueue queue, VkSemaphore semaphore);
typedef void (VKAPI_PTR * PFN_vkQueuePresentNV) (VkQueue queue, VkImage image);
typedef void (VKAPI_PTR * PFN_vkQueuePresentNoWaitNV) (VkQueue queue, VkImage image);
typedef void (VKAPI_PTR * PFN_vkSignalPresentDoneNV) (VkDevice device, VkSemaphore semaphore, VkFence fence);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetProcAddressNV (const char *name);
VKAPI_ATTR void VKAPI_CALL vkQueueSemaphoreWaitNV(VkQueue queue, VkSemaphore semaphore);
VKAPI_ATTR void VKAPI_CALL vkQueuePresentNV (VkQueue queue, VkImage image);
VKAPI_ATTR void VKAPI_CALL vkQueuePresentNoWaitNV (VkQueue queue, VkImage image);
VKAPI_ATTR void VKAPI_CALL vkSignalPresentDoneNV (VkDevice device, VkSemaphore semaphore, VkFence fence);
#endif // VK_NO_PROTOTYPES

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __VULKANNV_H__
