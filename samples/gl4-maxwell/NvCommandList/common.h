//----------------------------------------------------------------------------------
// File:        gl4-maxwell\NvCommandList/common.h
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
#include <NV/NvMatrix.h>

typedef GLuint64 sampler2D;

#define VERTEX_POS    0
#define VERTEX_NORMAL 1
#define VERTEX_UV     2

#define UBO_SCENE     0
#define UBO_OBJECT    1

#if defined(GL_core_profile) || defined(GL_compatibility_profile) || defined(GL_es_profile)

#extension GL_ARB_bindless_texture : require

#endif

#ifdef __cplusplus
namespace basiccmdlist
{
#endif

struct SceneData {
  nv::matrix4f  viewProjMatrix;
  nv::matrix4f  viewProjMatrixI;
  nv::matrix4f  viewMatrix;
  nv::matrix4f  viewMatrixI;
  nv::matrix4f  viewMatrixIT;
  
  nv::vec4f  wLightPos;
  
  nv::vec2<unsigned int> viewport;
  float shrinkFactor;
  float time;
};

struct ObjectData {
  nv::matrix4f  worldMatrix;
  nv::matrix4f  worldMatrixIT;
  nv::vec4f  color;
  nv::vec2f  texScale;
  nv::vec2f  _pad;
  sampler2D texColor;
};

#ifdef __cplusplus
}
#endif

#if defined(GL_core_profile) || defined(GL_compatibility_profile) || defined(GL_es_profile)

#extension GL_NV_command_list : enable
#if GL_NV_command_list
layout(commandBindableNV) uniform;
#endif

layout(std140,binding=UBO_SCENE) uniform sceneBuffer {
  SceneData   scene;
};

layout(std140,binding=UBO_OBJECT) uniform objectBuffer {
  ObjectData  object;
};

#endif


/*-----------------------------------------------------------------------
  Copyright (c) 2014, NVIDIA. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Neither the name of its contributors may be used to endorse 
     or promote products derived from this software without specific
     prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/