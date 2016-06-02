//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/scene.h
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
#ifndef SCENE_H
#define SCENE_H

#include <NvGLUtils/NvModelGL.h>
#include <map>
#include <string>
#include <vector>

struct MeshObj
{
  MeshObj() : m_specularValue(1.0f), m_color(1.0f, 1.0f, 1.0f, 1.0f), m_pModelData(NULL), m_texId(0), m_cullFacing(true) {}

  nv::matrix4f m_modelMatrix;
  
  nv::vec4f    m_color;
  float        m_specularValue;
  GLuint       m_texId;
  bool         m_cullFacing;
  bool           m_alphaTest;
  
  NvModelGL*     m_pModelData; // some 

};

void CreateRandomObjectsOnLandScape(std::vector<MeshObj>& a_modelsArray, std::map<std::string, NvModelGL*>& a_modelStorage, std::map<std::string, GLuint>& a_texStorage, float treeHeight=0);

#endif
