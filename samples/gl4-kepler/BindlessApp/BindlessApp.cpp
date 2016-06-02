//----------------------------------------------------------------------------------
// File:        gl4-kepler\BindlessApp/BindlessApp.cpp
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
#include "BindlessApp.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvImageGL.h"
#include "NV/NvLogs.h"
#include "NvUI/NvTweakBar.h"
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// This sample demonstrates using bindless rendering with GL_NV_shader_buffer_load and GL_NV_vertex_buffer_unified_memory.
// GL_NV_shader_buffer_load allows the program to pass a GPU pointer to the vertex shader to load uniforms directly from GPU memory.
// GL_NV_vertex_buffer_unified_memory allows the program to use GPU pointers to vertex and index data when making rendering calls.
// Both of these extensions can significantly reduce CPU L2 cache misses and pollution; this can dramatically speed up scenes with
// large numbers of draw calls.
//
// For more detailed information see http://www.nvidia.com/object/bindless_graphics.html
//
//
//
// Interesting pieces of code are annotated with "*** INTERESTING ***"
//
// The interesting code in this sample is in this file and Mesh.cpp
//
// Mesh::update() in Mesh.cpp contains the source code for getting the GPU pointers for vertex and index data
// Mesh::renderPrep() in Mesh.cpp sets up the vertex format
// Mesh::render() in Mesh.cpp does the actual rendering
// Mesh::renderFinish() in Mesh.cpp resets related state



////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::draw()
//
//    Performs the actual rendering
//
////////////////////////////////////////////////////////////////////////////////
void BindlessApp::draw(void)
{
    nv::matrix4f modelviewMatrix;

    glClearColor( 0.5, 0.5, 0.5, 1.0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Enable the vertex and pixel shader
    m_shader->enable();

	if (m_useBindlessTextures) {
		GLuint samplersLocation(m_shader->getUniformLocation("samplers"));
		glUniform1ui64vNV(samplersLocation, m_numTextures, m_textureHandles);

	}

	GLuint bBindlessTexture(m_shader->getUniformLocation("useBindless"));
	glUniform1i(bBindlessTexture, m_useBindlessTextures);

	GLuint currentTexture(m_shader->getUniformLocation("currentFrame"));
	glUniform1i(currentTexture, m_currentFrame);

    // Set up the transformation matices up
    modelviewMatrix = m_transformer->getModelViewMat();
    m_transformUniformsData.ModelView = modelviewMatrix;
    m_transformUniformsData.ModelViewProjection= m_projectionMatrix * modelviewMatrix;
    m_transformUniformsData.UseBindlessUniforms = m_useBindlessUniforms;
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_transformUniforms);
    glNamedBufferSubDataEXT(m_transformUniforms, 0, sizeof(TransformUniforms), &m_transformUniformsData);

    
    // If we are going to update the uniforms every frame, do it now
    if(m_updateUniformsEveryFrame == true)
    {
        float deltaTime;
        float dt;

        deltaTime = getFrameDeltaTime();

        if(deltaTime < m_minimumFrameDeltaTime)
        {
            m_minimumFrameDeltaTime = deltaTime;
        }

        dt = std::min(0.00005f / m_minimumFrameDeltaTime, .01f);
        m_t += dt * (float)Mesh::m_drawCallsPerState;

        updatePerMeshUniforms(m_t);
    }


    // Set up default per mesh uniforms. These may be changed on a per mesh basis in the rendering loop below 
    if(m_useBindlessUniforms == true)
    {
        // *** INTERESTING ***
        // Pass a GPU pointer to the vertex shader for the per mesh uniform data via a vertex attribute
        glVertexAttribI2i(m_bindlessPerMeshUniformsPtrAttribLocation, 
                             (int)(m_perMeshUniformsGPUPtr & 0xFFFFFFFF), 
                             (int)((m_perMeshUniformsGPUPtr >> 32) & 0xFFFFFFFF));
    }
    else
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, m_perMeshUniforms);
        glNamedBufferSubDataEXT(m_perMeshUniforms, 0, sizeof(m_perMeshUniformsData[0]), &(m_perMeshUniformsData[0]));
    }

    // If all of the meshes are sharing the same vertex format, we can just set the vertex format once
    if(Mesh::m_setVertexFormatOnEveryDrawCall == false)
    {
        Mesh::renderPrep();
    }    

    // Render all of the meshes
    for(int32_t i=0; i<(int32_t)m_meshes.size(); i++)
    {
        // If enabled, update the per mesh uniforms for each mesh rendered
        if(m_usePerMeshUniforms == true)
        {
            if(m_useBindlessUniforms == true)
            {
                GLuint64EXT perMeshUniformsGPUPtr;

                // *** INTERESTING ***
                // Compute a GPU pointer for the per mesh uniforms for this mesh
                perMeshUniformsGPUPtr = m_perMeshUniformsGPUPtr + sizeof(m_perMeshUniformsData[0]) * i;
                // Pass a GPU pointer to the vertex shader for the per mesh uniform data via a vertex attribute
                glVertexAttribI2i(m_bindlessPerMeshUniformsPtrAttribLocation, 
                                     (int)(perMeshUniformsGPUPtr & 0xFFFFFFFF), 
                                     (int)((perMeshUniformsGPUPtr >> 32) & 0xFFFFFFFF));
            }
            else
            {
                glBindBufferBase(GL_UNIFORM_BUFFER, 3, m_perMeshUniforms);
                glNamedBufferSubDataEXT(m_perMeshUniforms, 0, sizeof(m_perMeshUniformsData[0]), &(m_perMeshUniformsData[i]));
            }
        }

        // If we're not sharing vertex formats between meshes, we have to set the vertex format everytime it changes.
        if(Mesh::m_setVertexFormatOnEveryDrawCall == true)
        {
            Mesh::renderPrep();
        }

        // Now that everything is set up, do the actual rendering
        // The code that selects between rendering with Vertex Array Objects (VAO) and 
        // Vertex Buffer Unified Memory (VBUM) is located in Mesh::render()
        // The code that gets the GPU pointer for use with VBUM rendering is located in Mesh::update()
        m_meshes[i].render();


        // If we're not sharing vertex formats between meshes, we have to reset the vertex format to a default state after each mesh
        if(Mesh::m_setVertexFormatOnEveryDrawCall == true)
        {
            Mesh::renderFinish();
        }
    }

    // If we're sharing vertex formats between meshes, we only have to reset vertex format to a default state once
    if(Mesh::m_setVertexFormatOnEveryDrawCall == false)
    {
        Mesh::renderFinish();
    }

    // Disable the vertex and pixel shader
    m_shader->disable();

    // Update the rendering stats in the UI
    float drawCallsPerSecond;
    drawCallsPerSecond = (float)m_meshes.size() * mFramerate->getMeanFramerate() * (float)Mesh::m_drawCallsPerState;
    m_drawCallsPerSecondText->SetValue(drawCallsPerSecond / 1.0e6f);

	m_currentTime += getFrameDeltaTime();
	if (m_currentTime > ANIMATION_DURATION) m_currentTime = 0.0;
	m_currentFrame = (int)(180.0f*m_currentTime/ANIMATION_DURATION);
}

void BindlessApp::InitBindlessTextures()
{
	char fileName[64]={"textures/NV"};
	char Num[16];
	int i;

	m_textureHandles = new GLuint64[TEXTURE_FRAME_COUNT];
	m_textureIds = new GLuint[TEXTURE_FRAME_COUNT];
	m_numTextures = TEXTURE_FRAME_COUNT;

	for(i = 0; i < TEXTURE_FRAME_COUNT; ++i) {
		sprintf(Num, "%d", i);
		fileName[9]='N';
		fileName[10]='V';
		fileName[11]=0;
		strcat(fileName, Num);
		strcat(fileName, ".dds");
		m_textureIds[i] = NvImageGL::UploadTextureFromDDSFile(fileName);
		glBindTexture(GL_TEXTURE_2D, m_textureIds[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		m_textureHandles[i] = glGetTextureHandleNV(m_textureIds[i]);
		glMakeTextureHandleResidentNV(m_textureHandles[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::updatePerMeshUniforms()
//
//    Computes per mesh uniforms based on t and sends the uniforms to the GPU
//
////////////////////////////////////////////////////////////////////////////////
void BindlessApp::updatePerMeshUniforms(float t)
{
    // If we're using per mesh uniforms, compute the values for the uniforms for all of the meshes and
    // give the data to the GPU.
    if(m_usePerMeshUniforms == true)
    {
        // Update uniforms for the "ground" mesh
        m_perMeshUniformsData[0].r = 1.0f;
        m_perMeshUniformsData[0].g = 1.0f;
        m_perMeshUniformsData[0].b = 1.0f;
        m_perMeshUniformsData[0].a = 0.0f;

        // Compute the per mesh uniforms for all of the "building" meshes
        int32_t index=1;
        for(int32_t i=0; i<SQRT_BUILDING_COUNT; i++)
        {
            for(int32_t j=0; j<SQRT_BUILDING_COUNT; j++, index++)
            {
                float x, z, radius;
                
                x = float(i) / float(SQRT_BUILDING_COUNT) - 0.5f;
                z = float(j) / float(SQRT_BUILDING_COUNT) - 0.5f;
                radius = sqrt((x * x) + (z * z));
                
                m_perMeshUniformsData[index].r = sin(10.0f * radius + t);
                m_perMeshUniformsData[index].g = cos(10.0f * radius + t);
                m_perMeshUniformsData[index].b = radius;
                m_perMeshUniformsData[index].a = 0.0f;
				m_perMeshUniformsData[index].u = float(j) / float(SQRT_BUILDING_COUNT);
				m_perMeshUniformsData[index].v = float(i) / float(SQRT_BUILDING_COUNT);
            }
        }

        // Give the uniform data to the GPU
        glNamedBufferDataEXT(m_perMeshUniforms, m_perMeshUniformsData.size() * sizeof(m_perMeshUniformsData[0]), &(m_perMeshUniformsData[0]), GL_STREAM_DRAW);
    }
    else
    { 
        // All meshes will use these uniforms
        m_perMeshUniformsData[0].r = sin(t);
        m_perMeshUniformsData[0].g = cos(t);
        m_perMeshUniformsData[0].b = 1.0f;
        m_perMeshUniformsData[0].a = 0.0f;

        // Give the uniform data to the GPU
        glNamedBufferSubDataEXT(m_perMeshUniforms, 0, sizeof(m_perMeshUniformsData[0]), &(m_perMeshUniformsData[0]));
    }



    if(m_useBindlessUniforms == true)
    {
        // *** INTERESTING ***
        // Get the GPU pointer for the per mesh uniform buffer and make the buffer resident on the GPU
        // For bindless uniforms, this GPU pointer will later be passed to the vertex shader via a
        // vertex attribute. The vertex shader will then directly use the GPU pointer to access the
        // uniform data.
        int32_t perMeshUniformsDataSize;
        glBindBuffer(GL_UNIFORM_BUFFER, m_perMeshUniforms);
        glGetBufferParameterui64vNV(GL_UNIFORM_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &m_perMeshUniformsGPUPtr); 
        glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &perMeshUniformsDataSize);
        glMakeBufferResidentNV(GL_UNIFORM_BUFFER, GL_READ_ONLY);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::initRendering()
//
//    Sets up initial rendering state and creates meshes
//
////////////////////////////////////////////////////////////////////////////////
void BindlessApp::initRendering(void) {
    // Check extensions; exit on failure
    if(!requireExtension("GL_NV_vertex_buffer_unified_memory")) return;
    if(!requireExtension("GL_NV_shader_buffer_load")) return;
    if(!requireExtension("GL_EXT_direct_state_access")) return;
    if(!requireExtension("GL_NV_bindless_texture")) return;
    	
    NvAssetLoaderAddSearchPath("gl4-kepler/BindlessApp");

    // Create our pixel and vertex shader
    m_shader = NvGLSLProgram::createFromFiles("shaders/simple_vertex.glsl", "shaders/simple_fragment.glsl");
    m_bindlessPerMeshUniformsPtrAttribLocation = m_shader->getAttribLocation("bindlessPerMeshUniformsPtr", true);
    LOGI("m_bindlessPerMeshUniformsPtrAttribLocation = %d", m_bindlessPerMeshUniformsPtrAttribLocation);

    // Set the initial view
    m_transformer->setRotationVec(nv::vec3f(30.0f * (3.14f / 180.0f), 30.0f * (3.14f / 180.0f), 0.0f));
    
    // Create the meshes
    m_meshes.resize(1 + SQRT_BUILDING_COUNT * SQRT_BUILDING_COUNT);
    m_perMeshUniformsData.resize(m_meshes.size());

    // Create a mesh for the ground
    createGround(m_meshes[0], nv::vec3f(0.f, -.001f, 0.f), nv::vec3f(5.0f, 0.0f, 5.0f));

    // Create "building" meshes
    int32_t meshIndex = 0;
    for(int32_t i=0; i<SQRT_BUILDING_COUNT; i++)
    {
        for(int32_t k=0; k<SQRT_BUILDING_COUNT; k++)
        {
            float x, y, z;
            float size;
            
            x = float(i) / (float)SQRT_BUILDING_COUNT - 0.5f;
            y = 0.0f;
            z = float(k) / (float)SQRT_BUILDING_COUNT - 0.5f;
            size = .025f * (100.0f / (float)SQRT_BUILDING_COUNT);
            
            createBuilding(m_meshes[meshIndex + 1], nv::vec3f(5.0f * x, y, 5.0f * z), 
                           nv::vec3f(size, 0.2f + .1f * sin(5.0f * (float)(i * k)), size), nv::vec2f(float(k)/(float)SQRT_BUILDING_COUNT, float(i)/(float)SQRT_BUILDING_COUNT));
            
            meshIndex++;
        }
    }

    // Initialize Bindless Textures
	InitBindlessTextures();

    // create Uniform Buffer Object (UBO) for transform data and initialize 
    glGenBuffers(1, &m_transformUniforms);
    glNamedBufferDataEXT(m_transformUniforms, sizeof(TransformUniforms), &m_transformUniforms, GL_STREAM_DRAW);

    // create Uniform Buffer Object (UBO) for param data and initialize
    glGenBuffers(1, &m_perMeshUniforms);

    // Initialize the per mesh Uniforms
    updatePerMeshUniforms(0.0f);

    CHECK_GL_ERROR();
}


////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::BindlessApp()
//
//
////////////////////////////////////////////////////////////////////////////////
BindlessApp::BindlessApp()
: m_drawCallsPerSecondText(NULL)
, m_useBindlessUniforms(true)
, m_updateUniformsEveryFrame(true)
, m_usePerMeshUniforms(true)
, m_useBindlessTextures(false)
, m_currentFrame(0)
, m_currentTime(0.0f)
, m_t(0.0f)
, m_minimumFrameDeltaTime(1e6)
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -4.0f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}


////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::~BindlessApp()
//
//
////////////////////////////////////////////////////////////////////////////////
BindlessApp::~BindlessApp()
{
    LOGI("BindlessApp: destroyed\n");
}


////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::initUI()
//
//    Sets up the user interface
//
////////////////////////////////////////////////////////////////////////////////
void BindlessApp::initUI() {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        mTweakBar->addValue("Use bindless vertices/indices", Mesh::m_enableVBUM);
        mTweakBar->addValue("Use bindless uniforms", m_useBindlessUniforms);
        mTweakBar->addValue("Use per mesh uniforms", m_usePerMeshUniforms);
        mTweakBar->addValue("Update uniforms every frame", m_updateUniformsEveryFrame);
        mTweakBar->addValue("Set vertex format for each mesh", Mesh::m_setVertexFormatOnEveryDrawCall);
        mTweakBar->addValue("Use heavy vertex format", Mesh::m_useHeavyVertexFormat);
		mTweakBar->addValue("Use bindless textures", m_useBindlessTextures);
        mTweakBar->addValue("Draw calls per state", Mesh::m_drawCallsPerState, 1, 20);
    }

    // statistics
    if (mFPSText) {
        NvUIRect tr;
        mFPSText->GetScreenRect(tr);
        m_drawCallsPerSecondText = new NvUIValueText("M draw calls/sec", NvUIFontFamily::SANS, mFPSText->GetFontSize(), NvUITextAlign::RIGHT,
                                        0.0f, 2, NvUITextAlign::RIGHT);
        m_drawCallsPerSecondText->SetColor(NV_PACKED_COLOR(0x30, 0xD0, 0xD0, 0xB0));
        m_drawCallsPerSecondText->SetShadow();
        mUIWindow->Add(m_drawCallsPerSecondText, tr.left, tr.top+tr.height+8);
    }

    // Change the filtering for the framerate
    mFramerate->setMaxReportRate(.2f);
    mFramerate->setReportFrames(20);

    // Disable wait for vsync
    getGLContext()->setSwapInterval(0);  
}


////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::reshape()
//
//    Called when the viewport dimensions are changed
//
////////////////////////////////////////////////////////////////////////////////
void BindlessApp::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );

    nv::perspective(m_projectionMatrix, 45.0f * 2.0f*3.14159f / 360.0f, (float)width/(float)height, 0.1f, 10.0f);

    CHECK_GL_ERROR();
}


////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::createBuilding()
//
//    Create a very simple building mesh
//
////////////////////////////////////////////////////////////////////////////////
void BindlessApp::createBuilding(Mesh& mesh, nv::vec3f pos, nv::vec3f dim, nv::vec2f uv)
{
    std::vector<Vertex>         vertices;
    std::vector<uint16_t> indices;
    float                  r, g, b;
  
    dim.x *= 0.5f;
    dim.z *= 0.5f;
  
    // Generate a simple building model (i.e. a box). All of the "buildings" are in world space
	
    // +Z face
    r = 0.0f; g = 0.0f; b = 1.0f;
    randomColor(r, g, b);
    vertices.push_back(Vertex(-dim.x + pos.x,    0.0f + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,    0.0f + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,   dim.y + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(-dim.x + pos.x,   dim.y + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));                         

    // -Z face
    r = 0.0f; g = 0.0f; b = 0.5f;
    randomColor(r, g, b);
    vertices.push_back(Vertex(-dim.x + pos.x,  dim.y + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));                         
    vertices.push_back(Vertex(+dim.x + pos.x,  dim.y + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,   0.0f + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(-dim.x + pos.x,   0.0f + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));

    // +X face
    r = 1.0f; g = 0.0f; b = 0.0f;
    randomColor(r, g, b);
    vertices.push_back(Vertex(+dim.x + pos.x,   0.0f + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,   0.0f + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,  dim.y + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,  dim.y + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));                         

    // -X face
    r = 0.5f; g = 0.0f; b = 0.0f;
    randomColor(r, g, b);
    vertices.push_back(Vertex(-dim.x + pos.x,  dim.y + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));                         
    vertices.push_back(Vertex(-dim.x + pos.x,  dim.y + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(-dim.x + pos.x,   0.0f + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(-dim.x + pos.x,   0.0f + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));

    // +Y face
    r = 0.0f; g = 1.0f; b = 0.0f;
    randomColor(r, g, b);
    vertices.push_back(Vertex(-dim.x + pos.x,  dim.y + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,  dim.y + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,  dim.y + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(-dim.x + pos.x,  dim.y + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));                         

    // -Y face
    r = 0.0f; g = 0.5f; b = 0.0f;
    randomColor(r, g, b);
    vertices.push_back(Vertex(-dim.x + pos.x,   0.0f + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));                         
    vertices.push_back(Vertex(+dim.x + pos.x,   0.0f + pos.y, -dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,   0.0f + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(-dim.x + pos.x,   0.0f + pos.y, +dim.z + pos.z,  r, g, b, 1.0f));


    // Create the indices
    for(int32_t i=0; i<24; i+=4)
    {
        indices.push_back((uint16_t)(0 + i)); 
        indices.push_back((uint16_t)(1 + i)); 
        indices.push_back((uint16_t)(2 + i));

        indices.push_back((uint16_t)(0 + i)); 
        indices.push_back((uint16_t)(2 + i)); 
        indices.push_back((uint16_t)(3 + i));
    }


    mesh.update(vertices, indices);
}


////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::createGround()
//
//    Create a mesh for the ground
//
////////////////////////////////////////////////////////////////////////////////
void BindlessApp::createGround(Mesh& mesh, nv::vec3f pos, nv::vec3f dim)
{
    std::vector<Vertex>         vertices;
    std::vector<uint16_t> indices;
    float                  r, g, b;

    dim.x *= 0.5f;
    dim.z *= 0.5f;

    // +Y face
    r = 0.3f; g = 0.3f; b = 0.3f;
    vertices.push_back(Vertex(-dim.x + pos.x,  pos.y, +dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,  pos.y, +dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(+dim.x + pos.x,  pos.y, -dim.z + pos.z,  r, g, b, 1.0f));
    vertices.push_back(Vertex(-dim.x + pos.x,  pos.y, -dim.z + pos.z,  r, g, b, 1.0f));                         

    // Create the indices
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(0); indices.push_back(2); indices.push_back(3);

    mesh.update(vertices, indices);
}


////////////////////////////////////////////////////////////////////////////////
//
//  Method: BindlessApp::randomColor()
//
//    Generates a random color
//
////////////////////////////////////////////////////////////////////////////////
void BindlessApp::randomColor(float &r, float &g, float &b)
{
    r = float(rand() % 255) / 255.0f;
    g = float(rand() % 255) / 255.0f;
    b = float(rand() % 255) / 255.0f;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Miscellaneous stuff
//
////////////////////////////////////////////////////////////////////////////////
void BindlessApp::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
NvAppBase* NvAppFactory() {
    return new BindlessApp();
}
