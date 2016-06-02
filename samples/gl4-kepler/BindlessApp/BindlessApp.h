//----------------------------------------------------------------------------------
// File:        gl4-kepler\BindlessApp/BindlessApp.h
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
#ifndef BINDLESS_APP_H
#define BINDLESS_APP_H

#include "NvAppBase/gl/NvSampleAppGL.h"

#include "NV/NvMath.h"
#include "NvAppBase/NvInputTransformer.h"
#include "Mesh.h"

#include <vector>

#define SQRT_BUILDING_COUNT 100
#define TEXTURE_FRAME_COUNT 181
#define ANIMATION_DURATION 5.0f
class NvGLSLProgram;
class NvStopWatch;
class NvFramerateCounter;

class BindlessApp : public NvSampleAppGL
{
public:
    BindlessApp();
    ~BindlessApp();
    
    void initUI();
    void initRendering(void);
    void draw(void);
    void reshape(int32_t width, int32_t height);
    void updatePerMeshUniforms(float t);
	void InitBindlessTextures();

    void configurationCallback(NvGLConfiguration& config);

private:
    struct TransformUniforms
    {
        nv::matrix4f ModelView;
        nv::matrix4f ModelViewProjection;
        int32_t      UseBindlessUniforms;
    };

    struct PerMeshUniforms
    {
        float r, g, b, a, u, v;
    };

    void createBuilding(Mesh& mesh, nv::vec3f pos, nv::vec3f dim, nv::vec2f uv);
    void createGround(Mesh& mesh, nv::vec3f pos, nv::vec3f dim);
    void randomColor(float &r, float &g, float &b);
    
    // Simple collection of meshes to render
    std::vector<Mesh>             m_meshes;

    // Shader stuff
    NvGLSLProgram*                m_shader;
    GLuint                        m_bindlessPerMeshUniformsPtrAttribLocation;

    // uniform buffer object (UBO) for tranform data
    GLuint                        m_transformUniforms;
    TransformUniforms             m_transformUniformsData;
    nv::matrix4f                  m_projectionMatrix;

    // uniform buffer object (UBO) for mesh param data
    GLuint                        m_perMeshUniforms;
    std::vector<PerMeshUniforms>  m_perMeshUniformsData; 
    GLuint64EXT                   m_perMeshUniformsGPUPtr;

	//bindless texture handle
	GLuint64EXT*				  m_textureHandles;
	GLuint*						  m_textureIds;
	GLint					      m_numTextures;
	bool						  m_useBindlessTextures;
	int							  m_currentFrame;
	float						  m_currentTime;

    // UI stuff
    NvUIValueText*                m_drawCallsPerSecondText;
    bool                          m_useBindlessUniforms;
    bool                          m_updateUniformsEveryFrame;
    bool                          m_usePerMeshUniforms;

    // Timing related stuff
    float                         m_t;
    float                         m_minimumFrameDeltaTime;
};

#endif
