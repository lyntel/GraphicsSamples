//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury/Mercury.cpp
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
#include "Mercury.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvImageGL.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include <string>

#include "ParticleSystem.h"

extern std::string loadShaderSourceWithUniformTag(const char* uniformsFile, const char* srcFile);
const int Mercury::gbuffer_size;

Mercury::Mercury() 
	: mAnimate(true),
	mPolygonize(true),
    mReset(true),
    mTime(0.0f),
	mScreenQuadPos(NULL)
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -3.0f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

Mercury::~Mercury()
{	
	if (mScreenQuadPos) {
		delete mScreenQuadPos;
	}

	glDeleteRenderbuffers(1, &rbo_depth);
	glDeleteTextures(gbuffer_size, gbuffer_tex);
	glDeleteFramebuffers(1, &fbo);	

    LOGI("Mercury: destroyed\n");
}

void Mercury::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
	config.apiVer = NvGLAPIVersionES3();
}

void Mercury::initRendering(void) {

	// OpenGL 4.3 is the minimum for compute shaders
	if (!requireMinAPIVersion(NvGLAPIVersionES3_1()))
		return;

	// Set Clear Color
	glClearColor(0.25f, 0.25f, 0.25f, 1.0f);

	CHECK_GL_ERROR();
	NvAssetLoaderAddSearchPath("es3aep-kepler/Mercury");

	const char* shaderPrefix =
		(getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL)
		? "#version 430\n" : "#version 310 es\n";

	CHECK_GL_ERROR();
	{
		int32_t len;
		// Initialize Particles Render Program
		NvScopedShaderPrefix switched(shaderPrefix);

		std::string renderPartVS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/renderPartVS.glsl");
		std::string renderPartGS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/renderPartGS.glsl");
		mParticlesRenderProg = new NvGLSLProgram;


		NvGLSLProgram::ShaderSourceItem sourcesP[2];
		sourcesP[0].type = GL_VERTEX_SHADER;
		sourcesP[0].src = renderPartVS.c_str();
		sourcesP[1].type = GL_FRAGMENT_SHADER;
		sourcesP[1].src = NvAssetLoaderRead("shaders/renderPartFS.glsl", len);
		mParticlesRenderProg->setSourceFromStrings(sourcesP, 2);
		NvAssetLoaderFree((char*)sourcesP[1].src);

		// Initialize Surface Render Program
		std::string renderSurfVS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/renderSurfVS.glsl");
		std::string renderSurfGS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/renderSurfGS.glsl");
		std::string renderSurfFS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/renderSurfFS.glsl");
		mSurfaceRenderProg = new NvGLSLProgram;

		NvGLSLProgram::ShaderSourceItem sourcesS[3];
		sourcesS[0].type = GL_VERTEX_SHADER;
		sourcesS[0].src = renderSurfVS.c_str();
		sourcesS[1].type = GL_GEOMETRY_SHADER_EXT;
		sourcesS[1].src = renderSurfGS.c_str();
		sourcesS[2].type = GL_FRAGMENT_SHADER;
		sourcesS[2].src = renderSurfFS.c_str();
		mSurfaceRenderProg->setSourceFromStrings(sourcesS, 3);

		std::string quadVS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/renderQuadVS.glsl");
		std::string quadFS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/renderQuadFS.glsl");
		mQuadProg = new NvGLSLProgram;

		NvGLSLProgram::ShaderSourceItem sourcesQ[2];
		sourcesQ[0].type = GL_VERTEX_SHADER;
		sourcesQ[0].src = quadVS.c_str();
		sourcesQ[1].type = GL_FRAGMENT_SHADER;
		sourcesQ[1].src = quadFS.c_str();
		mQuadProg->setSourceFromStrings(sourcesQ, 2);
		
		std::string blurVS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/blurVS.glsl");
		std::string blurFS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/blurFS.glsl");
		mBlurProg = new NvGLSLProgram;

		NvGLSLProgram::ShaderSourceItem sourcesB[2];
		sourcesB[0].type = GL_VERTEX_SHADER;
		sourcesB[0].src = blurVS.c_str();
		sourcesB[1].type = GL_FRAGMENT_SHADER;
		sourcesB[1].src = blurFS.c_str();
		mBlurProg->setSourceFromStrings(sourcesB, 2);

	}
	CHECK_GL_ERROR();

	// Set up cubemap for skybox 
	mSkyBoxTexID = NvImageGL::UploadTextureFromDDSFile("textures/sky_cube.dds");
	glBindTexture(GL_TEXTURE_CUBE_MAP, mSkyBoxTexID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	CHECK_GL_ERROR();

	// Initialize skybox for screen quad
	mScreenQuadPos = new ShaderBuffer<nv::vec4f>(4);
	vec4f* pos = mScreenQuadPos->map();
	pos[0] = vec4f(-1.0f, -1.0f, -1.0f, 1.0f);
    pos[1] = vec4f( 1.0f, -1.0f, -1.0f, 1.0f);
    pos[2] = vec4f(-1.0f, 1.0f, -1.0f, 1.0f);
    pos[3] = vec4f( 1.0f, 1.0f, -1.0f, 1.0f);
	mScreenQuadPos->unmap();	


    //create ubo and initialize it with the structure data
    glGenBuffers( 1, &mUBO);
    glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
    glBufferData( GL_UNIFORM_BUFFER, sizeof(ShaderParams), &mShaderParams, GL_STREAM_DRAW);
    CHECK_GL_ERROR();

    //create simple single-vertex VBO
    float vtx_data[] = { 0.0f, 0.0f, 0.0f, 1.0f};
    glGenBuffers( 1, &mVBO);
    glBindBuffer( GL_ARRAY_BUFFER, mVBO);
    glBufferData( GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);
    CHECK_GL_ERROR();

    // For now, scale back the particle count on mobile.
	//int32_t particleCount = isMobilePlatform() ? (mNumParticles >> 2) : mNumParticles;
	int32_t particleCount = mNumParticles;

    mParticles = new ParticleSystem(particleCount, shaderPrefix);
    CHECK_GL_ERROR();

    int cx, cy, cz;
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &cx );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &cy );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &cz );
    LOGI("Max compute work group count = %d, %d, %d\n", cx, cy, cz );

    int sx, sy, sz;
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &sx );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &sy );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &sz );
    LOGI("Max compute work group size  = %d, %d, %d\n", sx, sy, sz );

    CHECK_GL_ERROR();

	//Set clockwise winding
	glFrontFace(GL_CW);

	// Texture
	const int screen_width = getAppContext()->width();
	const int screen_height = getAppContext()->height();
	
	// Frame buffer for final scene
	glGenTextures(gbuffer_size, gbuffer_tex);
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	for (int i = 0; i < gbuffer_size; i++) {
		glActiveTexture(GL_TEXTURE0 + 3 + i);
		glBindTexture(GL_TEXTURE_2D, gbuffer_tex[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glFramebufferTextureEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, gbuffer_tex[i], 0);
	}

	// Depth buffer 
	glGenRenderbuffers(1, &rbo_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screen_width, screen_height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);


	GLuint attachments[gbuffer_size];
	for (int i = 0; i < gbuffer_size; i++) {
		attachments[i] = GL_COLOR_ATTACHMENT0 + i;
	}

	glDrawBuffers(gbuffer_size, attachments);

	GLenum status;
	if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
		LOGE("glCheckFramebufferStatus: error %p", status);
		exit(0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
}

void Mercury::initUI(void) {
    if (mTweakBar) {
        mTweakBar->addPadding();
        mTweakBar->addValue("Animate", mAnimate);
		mTweakBar->addValue("Polygonize", mPolygonize);
        mTweakBar->addPadding();
        mTweakBar->addValue("Reset", mReset, true);
    }
}

void Mercury::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );

	// Rescale FBO and RBO as well
	glBindTexture(GL_TEXTURE_2D, gbuffer_tex[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, gbuffer_tex[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, gbuffer_tex[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	
    CHECK_GL_ERROR();
}



void Mercury::drawScreenAlignedQuad(const GLuint program)
{
	
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mScreenQuadPos->getBuffer());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mercury::draw(void)
{   
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (mReset) {
        mReset = false;
        mParticles->reset();
        mTweakBar->syncValues();
    }

    //
    // Compute matrices without the legacy matrix stack support
    //
    nv::matrix4f projectionMatrix;
    nv::perspective( projectionMatrix, 45.0f * 2.0f*3.14159f / 360.0f, (float)m_width/(float)m_height, 0.1f, 10.0f);

    nv::matrix4f viewMatrix = m_transformer->getModelViewMat();

    //
    // update struct representing UBO
    //
    mShaderParams.numParticles = mParticles->getSize();
    mShaderParams.ModelView = viewMatrix;
	mShaderParams.InvViewMatrix = inverse(viewMatrix);
    mShaderParams.ModelViewProjection = projectionMatrix * viewMatrix;
    mShaderParams.ProjectionMatrix = projectionMatrix;

    // bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
    glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
    glBufferSubData( GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams), &mShaderParams);

    if (mAnimate) {
		float timeDelta = getFrameDeltaTime();
        mParticles->update(timeDelta);
    } 
	else {
//		float timeDelta = getFrameDeltaTime();
		mParticles->update(0);
	}

	// Display Binary Space Partitions
	if (mPolygonize) {
		mSurfaceRenderProg->enable();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		uint32_t cellsDim = mParticles->getMCPolygonizer()->getCellsDim();
		mShaderParams.cellsize = mParticles->getMCPolygonizer()->getCellSize();


		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		// Bind packed vertex-isovalue buffer.
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mParticles->getMCPolygonizer()->getPackedPosValBuffer());
		// Bind triangle list hash table for marching cubes algorithm to be referenced from Geometry Shader
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mParticles->getMCPolygonizer()->getTriTableBuffer());

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mParticles->getMCPolygonizer()->getIndexBuffer());

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mSkyBoxTexID);
		GLuint loc = glGetUniformLocation(mSurfaceRenderProg->getProgram(), "skyBoxTex");
		glProgramUniform1i(mSurfaceRenderProg->getProgram(), loc, 1);	

		glDrawElements(GL_TRIANGLES, cellsDim * cellsDim * cellsDim * 3, GL_UNSIGNED_INT, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		mSurfaceRenderProg->disable();
		
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Draw final scene
		mQuadProg->enable();
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gbuffer_tex[0]);
		loc = glGetUniformLocation(mQuadProg->getProgram(), "gbuf0");
		glProgramUniform1i(mQuadProg->getProgram(), loc, 3);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, gbuffer_tex[1]);
		loc = glGetUniformLocation(mQuadProg->getProgram(), "gbuf1");
		glProgramUniform1i(mQuadProg->getProgram(), loc, 4);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, gbuffer_tex[2]);
		loc = glGetUniformLocation(mQuadProg->getProgram(), "gbuf2");
		glProgramUniform1i(mQuadProg->getProgram(), loc, 5);
		drawScreenAlignedQuad(mQuadProg->getProgram());		
		mQuadProg->disable();
		
		
	} 
	else {
		mParticlesRenderProg->enable();
		// draw particles  

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blend

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mParticles->getPosBuffer()->getBuffer());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mParticles->getIndexBuffer()->getBuffer());
		glDrawElements(GL_TRIANGLES, mParticles->getSize() * 6, GL_UNSIGNED_INT, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

		glDisable(GL_BLEND);
		mParticlesRenderProg->disable();
	}
}
 

// A replacement for the ARB_shader_include extension, which is not widely supported
// This loads a "header" file (uniformsFile) and a "source" file (srcFile).  It scans
// the source file for "#UNIFORMS" and replaces this tag with the contents of the
// uniforms file.  Limited, but functional for the cases used here
std::string loadShaderSourceWithUniformTag(const char* uniformsFile, const char* srcFile) {
    int32_t len;

    char *uniformsStr = NvAssetLoaderRead(uniformsFile, len);
    if (!uniformsStr)
        return "";

    char *srcStr = NvAssetLoaderRead(srcFile, len);
    if (!srcStr)
        return "";

    std::string dest = "";

    const char* uniformTag = "#UNIFORMS";

    char* uniformTagStart = strstr(srcStr, uniformTag);

    if (uniformTagStart) {
        // NULL the start of the tag
        *uniformTagStart = 0;
        dest += srcStr; // source up to tag
        dest += "\n";
        dest += uniformsStr;
        dest += "\n";
        char* uniformTagEnd = uniformTagStart + strlen(uniformTag);
        dest += uniformTagEnd;
    } else {
        dest += srcStr;
    }

    NvAssetLoaderFree(uniformsStr);
    NvAssetLoaderFree(srcStr);

    return dest;
}

NvAppBase* NvAppFactory() {
    return new Mercury();
}
