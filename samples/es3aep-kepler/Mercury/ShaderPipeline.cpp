//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury/ShaderPipeline.cpp
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
#include "ShaderPipeline.h"
#include "NV/NvPlatformGL.h"
#include <stdlib.h>
#include <iostream> 

#define CPP 1
#include "NV/NvLogs.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NV/NvShaderMappings.h"
#include "assets/shaders/uniforms.h"

ShaderPipeline::ShaderPipeline() {}

ShaderPipeline::~ShaderPipeline() {}

extern std::string loadShaderSourceWithUniformTag(const char* uniformsFile, const char* srcFile);

GLuint ShaderPipeline::createShaderPipelineProgram(GLuint target, const char* src, const char* m_shaderPrefix, GLuint m_programPipeline)
{
	GLuint object;
	GLint status;

	const GLchar* fullSrc[2] = { m_shaderPrefix, src };
	object = glCreateShaderProgramv(target, 2, fullSrc);

	{
		GLint logLength;
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0)
        {
            // This log contains shader compile errors.
            char *log = new char[logLength];
            glGetProgramInfoLog(object, logLength, 0, log);
            LOGI("Shader pipeline program not valid:\n%s\n", log);
            delete[] log;
        }
	}

	glBindProgramPipeline(m_programPipeline);
	glUseProgramStages(m_programPipeline, GL_COMPUTE_SHADER_BIT, object);
	glValidateProgramPipeline(m_programPipeline);
	glGetProgramPipelineiv(m_programPipeline, GL_VALIDATE_STATUS, &status);

	if (status != GL_TRUE) {
		GLint logLength;
		glGetProgramPipelineiv(m_programPipeline, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0)
        {
            char *log = new char[logLength];
            glGetProgramPipelineInfoLog(m_programPipeline, logLength, 0, log);
            LOGI("Shader pipeline not valid:\n%s\n", log);
            delete[] log;
        }
	}

	glBindProgramPipeline(0);
	CHECK_GL_ERROR();

	return object;
}

void ShaderPipeline::loadShaders(GLuint& m_programPipeline, GLuint& m_updateProg, const char* m_shaderPrefix, const char* shaderFile)
{
	if (m_updateProg) {
		glDeleteProgram(m_updateProg);
		m_updateProg = 0;
	}

	glGenProgramPipelines(1, &m_programPipeline);


	std::string src = loadShaderSourceWithUniformTag("shaders/uniforms.h", shaderFile);
	m_updateProg = createShaderPipelineProgram(GL_COMPUTE_SHADER, src.c_str(), m_shaderPrefix, m_programPipeline);

}
