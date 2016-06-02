//----------------------------------------------------------------------------------
// File:        es3-kepler\FXAA/FXAA.cpp
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
#include "FXAA.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NvGLUtils/NvGLSLProgram.h"

const char* model_files[2]={"models/loop","models/triangle"};
const char* fxaa_quality_frg[4]={"shaders/FXAA_Fastest.frag","shaders/FXAA_Default.frag","shaders/FXAA_High_Quality.frag","shaders/FXAA_Extreme_Quality.frag"};
const char* fxaa_quality_frg_es[4]={"shaders/FXAA_FastestES.frag","shaders/FXAA_DefaultES.frag","shaders/FXAA_High_QualityES.frag","shaders/FXAA_Extreme_QualityES.frag"};

#define Z_NEAR 0.4f
#define Z_FAR 100.0f
#define FOV 3.14f*0.5f

int LoadMdlDataFromFile(const char* name, void** buffer)
{
	int32_t len;
    char* data = NvAssetLoaderRead(name, len);

	if (!data) return 0;
	*buffer = malloc(len+1);
    memcpy(*buffer, data, len);
    NvAssetLoaderFree(data);

	return len;
}

GLfloat* genLineSphere(int sphslices, int* bufferSize, float scale)
{
    GLfloat rho, drho, theta, dtheta;
    GLfloat x, y, z;
    GLfloat s, t, ds, dt;
    GLint i, j;
    GLfloat* buffer;

    int count=0;
    int slices = (int)sphslices;
    int stacks = slices;
	buffer = new GLfloat[(slices+1)*stacks*3*2];
	if (bufferSize) *bufferSize = (slices+1)*stacks*3*2*4;
    ds = GLfloat(1.0 / sphslices);//slices;
    dt = GLfloat(1.0 / sphslices);//stacks;
    t = 1.0;
    drho = GLfloat(NV_PI / (GLfloat) stacks);
    dtheta = GLfloat(2.0 * NV_PI / (GLfloat) slices);

    for (i= 0; i < stacks; i++) {
        rho = i * drho;

        s = 0.0;
        for (j = 0; j<=slices; j++) {
            theta = (j==slices) ? GLfloat(0.0) : GLfloat(j * dtheta);
            x = -sin(theta) * sin(rho)*scale;
            z = cos(theta) * sin(rho)*scale;
            y = -cos(rho)*scale;
			buffer[count+0] = x;
			buffer[count+1] = y;
			buffer[count+2] = z;
			count+=3;

            x = -sin(theta) * sin(rho+drho)*scale;
            z = cos(theta) * sin(rho+drho)*scale;
            y = -cos(rho+drho)*scale;
			buffer[count+0] = x;
			buffer[count+1] = y;
			buffer[count+2] = z;
			count+=3;

            s += ds;
        }
        t -= dt;
    }
	return buffer;
}

FXAA::FXAA() :
    m_autoSpin(true),m_FXAALevel(2),m_lineWidth(2.0)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

FXAA::~FXAA()
{
    LOGI("FXAA: destroyed\n");
}

void FXAA::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

void FXAA::initUI() {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
		mTweakBar->addValue("Auto Spin", m_autoSpin);
		mTweakBar->addPadding();
		mTweakBar->addValue("Line Width", m_lineWidth, 0.1f, 3.0f);

		mTweakBar->addPadding();
		NvTweakEnum<uint32_t> FXAALevel[] =
        {
            { "Off", 0 },
            { "Fastest (10)", 1 },
            { "Default (12)", 2 },
            { "High Quality (29)", 3 },
			{ "Extreme Quality (39)", 4 },
        };

        mTweakBar->addEnum("FXAA Level:", m_FXAALevel, FXAALevel, TWEAKENUM_ARRAYSIZE(FXAALevel), 0x11);
    }
}
bool FXAA::genRenderTexture()
{
	int width = getGLContext()->width();
	int height = getGLContext()->height();
	
	glGenFramebuffers(1, &m_sourceFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_sourceFrameBuffer);

	glGenTextures(1, &m_sourceTexture);
	glBindTexture(GL_TEXTURE_2D, m_sourceTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_sourceTexture, 0);

	glGenRenderbuffers(1, &m_sourceDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_sourceDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_sourceDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);    

	// Finalize
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
	return true;
}

void FXAA::initRendering(void) {
	m_aspectRatio = 1.0;
    NvAssetLoaderAddSearchPath("es3-kepler/FXAA");
     
    if(!requireMinAPIVersion(NvGLAPIVersionES3()))
        return;

    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -3.0f));
    m_transformer->setRotationVec(nv::vec3f(NV_PI*0.15f, 0.0f, 0.0f));

	float* buf = genLineSphere(40, &m_sphereBufferSize, 1);
	glGenBuffers(1, &m_sphereVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, m_sphereBufferSize, buf, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    delete[] buf;

	int i;
	void* pData=0;
	for (i=0;i<2;i++) {
		m_objectFileSize[i] = LoadMdlDataFromFile(model_files[i], &pData);
		glGenBuffers(1, &m_object[i]);
		glBindBuffer(GL_ARRAY_BUFFER, m_object[i]);
		glBufferData(GL_ARRAY_BUFFER, m_objectFileSize[i], pData, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		free(pData);
	}

    //init shaders
	m_FXAAProg[0] = NvGLSLProgram::createFromFiles("shaders/Blit.vert", "shaders/Blit.frag");
	for (i=1;i<5;i++) {
		m_FXAAProg[i] = NvGLSLProgram::createFromFiles("shaders/FXAA.vert", 
            (getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL)
            ? fxaa_quality_frg[i-1] : fxaa_quality_frg_es[i-1]);
	}
	m_LineProg = NvGLSLProgram::createFromFiles("shaders/DrawLine.vert", "shaders/DrawLine.frag");
	m_ObjectProg = NvGLSLProgram::createFromFiles("shaders/Object.vert", "shaders/Object.frag");

	//init textures
	genRenderTexture();
    glBindTexture(GL_TEXTURE_2D, m_sourceTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

	nv::perspective(m_projection_matrix, FOV * 0.5f, getGLContext()->width()/(float)getGLContext()->height(), Z_NEAR, Z_FAR);

    CHECK_GL_ERROR();
}

void FXAA::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );

    CHECK_GL_ERROR();
}

void FXAA::drawObjects(nv::matrix4f* mvp, float* color, int id)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);

	glUseProgram(m_ObjectProg->getProgram());
	m_ObjectProg->setUniformMatrix4fv("mvp", mvp->_array);
	m_ObjectProg->setUniform4fv("color", color, 1);

	glBindBuffer(GL_ARRAY_BUFFER, m_object[id]);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(m_ObjectProg->getAttribLocation("aPosition"), 3, GL_FLOAT, GL_FALSE, 32, 0);
	glVertexAttribPointer(m_ObjectProg->getAttribLocation("aNormal"), 3, GL_FLOAT, GL_FALSE, 32, (void*)12);

    glDrawArrays(GL_TRIANGLES, 0, m_objectFileSize[id]/32);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FXAA::drawSphere(nv::matrix4f* mvp)
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(m_LineProg->getProgram());
	m_LineProg->setUniformMatrix4fv("mvp",  mvp->_array);
	m_LineProg->setUniform4f("color", 0.0,0.0,0.0,1.0);

	glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINES, 0, 41 * 40 * 2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FXAA::draw(void)
{
	nv::matrix4f rotation;
	nv::matrix4f model;
	nv::matrix4f mvp;
	float color0[4]={0.5,0.5,0.8,1.0};
	float color1[4]={0.5,0.9,0.5,1.0};

	m_transformer->setRotationVel(nv::vec3f(0.0f, m_autoSpin ? (NV_PI*0.05f) : 0.0f, 0.0f));

	glBindFramebuffer(GL_FRAMEBUFFER, m_sourceFrameBuffer);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLineWidth(m_lineWidth);

	//draw lined sphere
	nv::matrix4f vp = m_projection_matrix * m_transformer->getModelViewMat();
	nv::rotationX(rotation, float(0.2));
	mvp = m_projection_matrix * m_transformer->getModelViewMat() * rotation;
	drawSphere(&mvp);

	//draw rings
	model.make_identity();
	model.set_scale(0.04);
	nv::rotationX(rotation, float(0.1));
	mvp = m_projection_matrix * m_transformer->getModelViewMat() * rotation* model;
	drawObjects(&mvp, color0, 0);

	model.make_identity();
	model.set_scale(0.04);
	nv::rotationX(rotation, float(NV_PI/2+0.1));
	mvp = m_projection_matrix * m_transformer->getModelViewMat() * rotation * model;
	drawObjects(&mvp, color0, 0);

	//draw triangle
	model.make_identity();
	model.set_scale(0.03);
	model.set_translate(nv::vec3f(0.0f, -0.3f, 0.0f));
	mvp = m_projection_matrix * m_transformer->getModelViewMat() * model;
	drawObjects(&mvp, color1, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
	FXAABlit(m_FXAALevel);
	return;
}
void FXAA::FXAABlit(int level)
{
   float const vertexPosition[] = { 
        m_aspectRatio, -1.0f, 
        -m_aspectRatio, -1.0f,
        m_aspectRatio, 1.0f, 
        -m_aspectRatio, 1.0f};

    float const textureCoord[] = { 
        1.0f, 0.0f, 
        0.0f, 0.0f, 
        1.0f, 1.0f, 
        0.0f, 1.0f };

    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_FXAAProg[level]->getProgram());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sourceTexture);

    if (m_FXAALevel) {
		glUniform1i(m_FXAAProg[level]->getUniformLocation("uSourceTex"), 0);
		m_FXAAProg[level]->setUniform2f("RCPFrame", float(1.0/float(m_width)), float(1.0/float(m_height)));
	}

    int aPosCoord = m_FXAAProg[level]->getAttribLocation("aPosition");
    int aTexCoord = m_FXAAProg[level]->getAttribLocation("aTexCoord");

    glVertexAttribPointer(aPosCoord, 2, GL_FLOAT, GL_FALSE, 0, vertexPosition);
    glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 0, textureCoord);
    glEnableVertexAttribArray(aPosCoord);
    glEnableVertexAttribArray(aTexCoord);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(aPosCoord);
    glDisableVertexAttribArray(aTexCoord);

}

NvAppBase* NvAppFactory() {
    return new FXAA();
}
