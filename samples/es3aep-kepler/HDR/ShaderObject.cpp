//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/ShaderObject.cpp
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

#include "ShaderObject.h"
#include <NV/NvLogs.h>
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvAssetLoader/NvAssetLoader.h"

int CShaderObject::GenProgram(const char* vertexShader, const char* fragmentShader, const char** Attribs, int AttribsArraySize, const char** Uniforms, int UniformsArraySize, const char** Textures, int TextureArraySize)
{
	if (Initialized) return -1;

	int i;

    pid = glCreateProgram();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

	const char* vsSrc[2];
	vsSrc[0] = NvGLSLProgram::getGlobalShaderHeader() ?
		NvGLSLProgram::getGlobalShaderHeader() : "";
	vsSrc[1] = vertexShader;

	const char* fsSrc[2];
	fsSrc[0] = NvGLSLProgram::getGlobalShaderHeader() ?
		NvGLSLProgram::getGlobalShaderHeader() : "";
	fsSrc[1] = fragmentShader;

	glShaderSource(vs, 2, vsSrc, 0);
    glShaderSource(fs, 2, fsSrc, 0);

    glCompileShader(vs);

    GLint compiled = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            char* buf = new char[infoLen];
            if (buf) {
                glGetShaderInfoLog(vs, infoLen, NULL, buf);
                LOGE("Shader log:\n%s\n", buf);
                delete[] buf;
            }
        }
        return 0;
	}
	glAttachShader(pid, vs);
	glDeleteShader(vs);

    glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            char* buf = new char[infoLen];
            if (buf) {
                glGetShaderInfoLog(fs, infoLen, NULL, buf);
                LOGI("Shader log:\n%s\n", buf);
                delete[] buf;
            }
        }
		return 0;
	}
    glAttachShader(pid, fs);
	glDeleteShader(fs);


    glLinkProgram(pid);

    // check if program linked
    GLint success = 0;
    glGetProgramiv(pid, GL_LINK_STATUS, &success);

    if (!success)
    {
        GLint bufLength = 0;
        glGetProgramiv(pid, GL_INFO_LOG_LENGTH, &bufLength);
        if (bufLength) {
            char* buf = new char[bufLength];
            if (buf) {
                glGetProgramInfoLog(pid, bufLength, NULL, buf);
                delete [] buf;
            }
        }
        glDeleteProgram(pid);
        pid = 0;
    }

	if (!pid) return -1;

	glUseProgram(pid);


	//bind the vertex attribute from string Attribs[i] to location i
	for (i = 0; i < AttribsArraySize; ++i) {
		glBindAttribLocation(pid, i, Attribs[i]);
	}

	//get location of uniform from string Uniforms[i]
	for (i=0; i<UniformsArraySize; i++) {
		auiLocation[i] = glGetUniformLocation(pid, Uniforms[i]);
	}

	int sid;
	//set the sampler2D named by string Textures[i] to texture i
	for (i=0; i<TextureArraySize; i++) {
		sid = glGetUniformLocation(pid, Textures[i]);
		glUniform1i(sid, i);
	}
	Initialized = 1;
	return 0;
}

int CShaderObject::GenProgram(const char* vertexShader, const char* fragmentShader)
{
	NvGLSLProgram* prog = NvGLSLProgram::createFromStrings(vertexShader, fragmentShader);
	if (prog) {
		pid = prog->getProgram();
		delete prog;
	}
	else pid =-1;

	return pid;
}

int CShaderObject::GenLocation(const char** Attribs, int AttribsArraySize, const char** Uniforms, int UniformsArraySize, const char** Textures, int TextureArraySize)
{
	int i;
	//GLenum err;
	glUseProgram(pid);

	//bind the vertex attribute from string Attribs[i] to location i
	for (i = 0; i < AttribsArraySize; ++i) {
		glBindAttribLocation(pid, i, Attribs[i]);
	}
	//get location of uniform from string Uniforms[i]
	for (i=0; i<UniformsArraySize; i++) {
		auiLocation[i] = glGetUniformLocation(pid, Uniforms[i]);
	}

	//set the sampler2D named by string Textures[i] to texture i
	for (i=0; i<TextureArraySize; i++) {
		glUniform1i(glGetUniformLocation(pid, Textures[i]), i);
	}
	return 0;
}

static 	GLenum err;
/*
int CShaderObject::GenCSPipelineProgram(const char* computeShader, const char** Uniforms, int UniformsArraySize, const char** Textures, int TextureArraySize)
{
	int i;
    GLuint object;
	GLint status;
	glGenProgramPipelines( 1, &pipeline);
	err = glGetError();
    object = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1, &computeShader );

    glBindProgramPipeline(pipeline);
    glUseProgramStages(pipeline, GL_COMPUTE_SHADER_BIT, object);
    glValidateProgramPipeline(pipeline);
    glGetProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, &status);
	err = glGetError();
    if (status != GL_TRUE) {
        GLint logLength;
        glGetProgramPipelineiv(pipeline, GL_INFO_LOG_LENGTH, &logLength);
        char *log = new char [logLength];
        glGetProgramPipelineInfoLog(pipeline, logLength, 0, log);
        //LOGI("Shader pipeline not valid:\n%s\n", log);
        delete [] log;
    }

	//get location of uniform from string Uniforms[i]
	for (i=0; i<UniformsArraySize; i++) {
		auiLocation[i] = glGetUniformLocation(object, Uniforms[i]);
	}
	err = glGetError();
	int sid;
	//set the sampler2D named by string Textures[i] to texture i
	for (i=0; i<TextureArraySize; i++) {
		sid = glGetUniformLocation(object, Textures[i]);
		glUniform1i(sid, i);
	}

	pid = object;
    return pid;
}
*/

int CShaderObject::GenCSProgram(const char* computeShader, const char** Uniforms, int UniformsArraySize, const char** Textures, int TextureArraySize)
{
	int i;
    GLuint program = glCreateProgram();

    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, 1, &(computeShader), 0);
    glCompileShader(cs);
    GLint compiled = 0;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            char* buf = new char[infoLen];
            if (buf) {
                glGetShaderInfoLog(cs, infoLen, NULL, buf);
                //LOGI("Shader log:\n%s\n", buf);
                delete[] buf;
            }
        }
        return 0;
	}

    glAttachShader(program, cs);

    // can be deleted since the program will keep a reference
    glDeleteShader(cs);

    glLinkProgram(program);

    // check if program linked
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        GLint bufLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
        if (bufLength) {
            char* buf = new char[bufLength];
            if (buf) {
                glGetProgramInfoLog(program, bufLength, NULL, buf);
                //LOGI("Could not link program:\n%s\n", buf);
                delete [] buf;
            }
        }
        glDeleteProgram(program);
        program = 0;
    }
	//get location of uniform from string Uniforms[i]
	for (i=0; i<UniformsArraySize; i++) {
		auiLocation[i] = glGetUniformLocation(program, Uniforms[i]);
	}
	err = glGetError();
	int sid;
	//set the sampler2D named by string Textures[i] to texture i
	for (i=0; i<TextureArraySize; i++) {
		sid = glGetUniformLocation(program, Textures[i]);
		glUniform1i(sid, i);
	}

	pid = program;
    return pid;
}


CShaderObject::~CShaderObject()
{
	if (Initialized) {
	}
}
