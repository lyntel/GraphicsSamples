//----------------------------------------------------------------------------------
// File:        es3aep-kepler\FeedbackParticlesApp/ParticleSystem.cpp
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
#include "ParticleSystem.h"
#include <NV/NvPlatformGL.h>
#include <NvGLUtils/NvGLSLProgram.h>
#include <NvGLUtils/NvShapesGL.h>
#include <NV/NvLogs.h>
#include "TextureUtils.h"
#include "Utils.h"


//------------------------------------------------------------------------------
//
NvVertexAttribute ParticleSystem::ms_attributes[] = 
{
    {"a_Type",    GL_BYTE,1,          offsetof(Particle,type),    sizeof(Particle),false},
    {"a_Color",   GL_UNSIGNED_BYTE,3, offsetof(Particle,color),   sizeof(Particle),false},
    {"a_Age",     GL_FLOAT,1,         offsetof(Particle,age),     sizeof(Particle),false},
    {"a_Position",GL_FLOAT,3,         offsetof(Particle,position),sizeof(Particle),false},
    {"a_Velocity",GL_FLOAT,3,         offsetof(Particle,velocity),sizeof(Particle),false},

    {NULL,0,0,0,0,false}
};


//------------------------------------------------------------------------------
//
ParticleSystem::ParticleSystem()
    : m_feedbackProgram(NULL),
      m_billboardProgram(NULL),
      m_emitterProgram(NULL),
      m_emittersBuffer(0),
      m_particleTexture(0),
      m_randomTexture(0),
      m_FBMTexture(0)
{
    memset(&m_emitters,0,sizeof(m_emitters));
}


//------------------------------------------------------------------------------
//
bool ParticleSystem::initFeedbackProgram()
{
    NvAsset vertShader("feedback.vert");
    NvAsset geomShader("feedback.geom");

    NvGLSLProgram::ShaderSourceItem sources[2] = 
    {
        vertShader, GL_VERTEX_SHADER,
        geomShader, GL_GEOMETRY_SHADER,
    };

    m_feedbackProgram = new NvGLSLProgram();
    m_feedbackProgram->setSourceFromStrings(sources,countof(sources));

    //the order is important!
    const GLchar* feedbackVaryings[4] = 
    {
        "gs_TypeColor",
        "gs_Age",
        "gs_Position",
        "gs_Velocity",
    };

    glTransformFeedbackVaryings(m_feedbackProgram->getProgram(),countof(feedbackVaryings),
                                feedbackVaryings,GL_INTERLEAVED_ATTRIBS_EXT);
    m_feedbackProgram->relink();

    m_feedbackProgram->enable();
    m_feedbackProgram->setUniform1i("u_RandomTexture",RANDOM_TEX_UNIT);
    m_feedbackProgram->setUniform1i("u_FBMTexture",FBM_TEX_UNIT);

    glGenQueries(countof(m_countQuery),m_countQuery);

    return true;
}


//------------------------------------------------------------------------------
//
bool ParticleSystem::initEmitterProgram()
{
    NvAsset vertShader("emitter_feedback.vert");
    NvAsset geomShader("emitter_feedback.geom");

    NvGLSLProgram::ShaderSourceItem sources[2] = 
    {
        vertShader, GL_VERTEX_SHADER,
        geomShader, GL_GEOMETRY_SHADER
    };

    m_emitterProgram = new NvGLSLProgram();
    m_emitterProgram->setSourceFromStrings(sources,countof(sources));

    //the order is important!
    const GLchar* feedbackVaryings[4] = 
    {
        "gs_TypeColor",
        "gs_Age",
        "gs_Position",
        "gs_Velocity",
    };

    glTransformFeedbackVaryings(m_emitterProgram->getProgram(),countof(feedbackVaryings),
                                feedbackVaryings,GL_INTERLEAVED_ATTRIBS_EXT);
    m_emitterProgram->relink();

    m_emitterProgram->enable();
    m_emitterProgram->setUniform1i("u_RandomTexture",RANDOM_TEX_UNIT);

    return true;
}


//------------------------------------------------------------------------------
//
bool ParticleSystem::initBillboardProgram()
{
    NvAsset vertShader("billboard.vert");
    NvAsset fragShader("billboard.frag");
    NvAsset geomShader("billboard.geom");

    NvGLSLProgram::ShaderSourceItem sources[3] = 
    {
        vertShader, GL_VERTEX_SHADER,
        fragShader, GL_FRAGMENT_SHADER,
        geomShader, GL_GEOMETRY_SHADER
    };

    m_billboardProgram = new NvGLSLProgram();
    m_billboardProgram->setSourceFromStrings(sources,countof(sources));

    m_billboardProgram->enable();
    m_billboardProgram->setUniform1i("u_Texture",PARTICLE_TEX_UNIT);

    return true;
}


//------------------------------------------------------------------------------
//
void ParticleSystem::initParticleBuffers()
{
    Particle* p = new Particle[MAX_PARTICLES];
    memset(p,0,MAX_PARTICLES*sizeof(Particle));

    const uint32_t numFeedbacks = FEEDBACK_QUEUE_LEN + 1;
    glGenTransformFeedbacks(numFeedbacks,m_transformFeedback);
    glGenBuffers(numFeedbacks,m_particlesBuffers);
    for (uint32_t n = 0;n < numFeedbacks;n++)
    {
        glBindBuffer(GL_ARRAY_BUFFER,m_particlesBuffers[n]);
        glBufferData(GL_ARRAY_BUFFER,MAX_PARTICLES*sizeof(Particle),p,GL_STATIC_DRAW);

        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK,m_transformFeedback[n]);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,m_particlesBuffers[n]);
        glBindBuffer(GL_ARRAY_BUFFER,0);
    }

    delete [] p;
}


//------------------------------------------------------------------------------
//
bool ParticleSystem::init()
{
    initFeedbackProgram();
    initBillboardProgram();
    initEmitterProgram();
    initParticleBuffers();

    m_particleTexture   = TexturesUtils::spotTexture(PARTICLE_TEX_SIZE,PARTICLE_TEX_SIZE);
    m_randomTexture     = TexturesUtils::random1DTexture(RANDOM_TEX_SIZE);
    m_FBMTexture        = TexturesUtils::FBM3DTexture(FBM_TEX_SIZE);

    m_time              = 0.0f;
    m_isStopped         = false;
    m_emittersCount     = 0;
    m_particleCount     = 0;
    m_params            = Parameters();
    m_ring              = RingCycle();

    return true;
}


//------------------------------------------------------------------------------
//
void ParticleSystem::release()
{
    NvSafeDelete(m_feedbackProgram);
    NvSafeDelete(m_billboardProgram);
    NvSafeDelete(m_emitterProgram);
    
    glDeleteTextures(1,&m_randomTexture);
    glDeleteTextures(1,&m_particleTexture);
    glDeleteTextures(1,&m_FBMTexture);

    glDeleteTransformFeedbacks(countof(m_transformFeedback),m_transformFeedback);
    glDeleteBuffers(countof(m_particlesBuffers),m_particlesBuffers);
    glDeleteBuffers(1,&m_emittersBuffer);

    glDeleteQueries(countof(m_countQuery),m_countQuery);
}


//------------------------------------------------------------------------------
//
void ParticleSystem::update(float deltaTime)
{
    moveEmitters();

    m_deltaTime = deltaTime;
    m_time      += m_deltaTime;

    //separate time var to be able to freeze them
    m_collidersTime = m_params.freezeColliders ? m_collidersTime : m_time;
}


//------------------------------------------------------------------------------
//
void ParticleSystem::seedEmitters(uint32_t num)
{
    m_emittersCount = std::min(num,(uint32_t)MAX_EMITTERS);

    Particle* p = m_emitters;
    memset(p,0,m_emittersCount*sizeof(Particle));
    for (uint32_t n = 0;n < m_emittersCount;n++)
    {
        p->type     = Particle::EMITTER;
        p->color[0] = rand()&255;
        p->color[1] = rand()&255;
        p->color[2] = rand()&255;
        ++p;
    }

    moveEmitters();
}


//------------------------------------------------------------------------------
//
void ParticleSystem::moveEmitters()
{
    Particle* p = m_emitters;
    for (uint32_t n = 0;n < m_emittersCount;n++)
    {
        if (!m_params.freezeEmitters)
            p->position = nv::vec3f(sinf(m_time + 2*3.14f*p->color[0]/255.f)*0.2f,
                                    -1.5f,
                                    cosf(m_time + 2*3.14f*p->color[1]/255.f)*0.2f);
        
        p->age = p->age > m_params.emitPeriod ? 0 : p->age + m_deltaTime;

        ++p;
    }
}


//------------------------------------------------------------------------------
//
void ParticleSystem::updateEmitters()
{
    if (0 == m_emittersBuffer)
        glGenBuffers(1,&m_emittersBuffer);

    glBindBuffer(GL_ARRAY_BUFFER,m_emittersBuffer);
    glBufferData(GL_ARRAY_BUFFER,sizeof(m_emitters),m_emitters,GL_STATIC_DRAW);          //orphan old data
}


//------------------------------------------------------------------------------
//
void ParticleSystem::emitParticles()
{
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK,m_transformFeedback[EMITTERS_FEEDBACK]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,m_particlesBuffers[EMITTERS_FEEDBACK]);

    glBindBuffer(GL_ARRAY_BUFFER,m_emittersBuffer);
    ms_attributes->apply(m_emitterProgram);

    m_emitterProgram->enable();
    m_emitterProgram->setUniform1f("u_EmitPeriod",m_params.emitPeriod);
    m_emitterProgram->setUniform1f("u_EmitCount",(float)m_params.emitCount);
    m_emitterProgram->setUniform1f("u_DeltaTime",m_deltaTime);
    m_emitterProgram->setUniform1f("u_Time",m_time);

    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS,0,m_emittersCount);
    glEndTransformFeedback();

    ms_attributes->reset(m_emitterProgram);
}


//------------------------------------------------------------------------------
//
void ParticleSystem::processParticles()
{
    const uint32_t from = m_ring.begin();
    const uint32_t to   = m_ring.end();

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK,m_transformFeedback[to]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,m_particlesBuffers[to]);

    m_feedbackProgram->enable();
    m_feedbackProgram->setUniform1f("u_ParticleLifetime",m_params.particleLifetime);
    m_feedbackProgram->setUniform1f("u_cTime",m_collidersTime);
    m_feedbackProgram->setUniform1f("u_DeltaTime",m_deltaTime);
    m_feedbackProgram->setUniform1f("u_Time",m_time);

    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,m_countQuery[to]);
    glBeginTransformFeedback(GL_POINTS);

    //process newly generated particles from emitters feedback buffer
    glBindBuffer(GL_ARRAY_BUFFER,m_particlesBuffers[EMITTERS_FEEDBACK]);
    ms_attributes->apply(m_feedbackProgram);
    glDrawTransformFeedback(GL_POINTS,m_transformFeedback[EMITTERS_FEEDBACK]);

    //feedback particles; kill the old ones
    if (m_ring.isFullCycle())
    {
        glBindBuffer(GL_ARRAY_BUFFER,m_particlesBuffers[from]);
        ms_attributes->apply(m_feedbackProgram);
        glDrawTransformFeedback(GL_POINTS,m_transformFeedback[from]);
    }

    glEndTransformFeedback();
    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

    ms_attributes->reset(m_feedbackProgram);
}


//------------------------------------------------------------------------------
//
void ParticleSystem::advanceFeedback()
{
    //query particle count from the last frame
    if (m_ring.isFullCycle())
        glGetQueryObjectuiv(m_countQuery[m_ring.begin()],GL_QUERY_RESULT,&m_particleCount);

    glEnable(GL_RASTERIZER_DISCARD_EXT);

    glActiveTexture(GL_TEXTURE0 + RANDOM_TEX_UNIT);
    glBindTexture(GL_TEXTURE_2D,m_randomTexture);
    glActiveTexture(GL_TEXTURE0 + FBM_TEX_UNIT);
    glBindTexture(GL_TEXTURE_3D,m_params.removeNoise ? 0 : m_FBMTexture);

    updateEmitters();
    emitParticles();
    processParticles();

    glDisable(GL_RASTERIZER_DISCARD_EXT);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK,0);
}


//------------------------------------------------------------------------------
//
void ParticleSystem::drawParticles(const nv::matrix4f& mMV, const nv::matrix4f& mProj)
{
    nv::matrix4f MVP = mProj*mMV;
    nv::vec4f right  = mMV.get_row(0);
    nv::vec4f up     = mMV.get_row(1);

    m_billboardProgram->enable();
    m_billboardProgram->setUniformMatrix4fv("u_MVP",(GLfloat*)MVP.get_value(),1);
    m_billboardProgram->setUniformMatrix4fv("u_MV",(GLfloat*)mMV.get_value(),1);
    m_billboardProgram->setUniform3f("u_Right",right.x,right.y,right.z);
    m_billboardProgram->setUniform3f("u_Up",up.x,up.y,up.z);
    m_billboardProgram->setUniform1f("u_BillboardSize",m_params.billboardSize);
    m_billboardProgram->setUniform1f("u_VelocityScale",m_params.velocityScale);
    m_billboardProgram->setUniform1f("u_ParticleLifetime",m_params.particleLifetime);
    m_billboardProgram->setUniform1f("u_UseColors",m_params.useColors ? 1.f : 0.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,m_particleTexture);

    glEnable(GL_BLEND);
    if (m_params.useColors)
        glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    else
        glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ZERO,GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ARRAY_BUFFER,m_particlesBuffers[m_ring.end()]);
    ms_attributes->apply(m_billboardProgram);
    glDrawTransformFeedback(GL_POINTS,m_transformFeedback[m_ring.end()]);
    ms_attributes->reset(m_billboardProgram);
    glBindBuffer(GL_ARRAY_BUFFER,0);

    glDisable(GL_BLEND);
}


//------------------------------------------------------------------------------
//
void ParticleSystem::draw(const nv::matrix4f& mMV, const nv::matrix4f& mProj)
{
    if (m_emittersCount != m_params.emittersCount)
        seedEmitters(m_params.emittersCount);

    if (!m_isStopped)
        advanceFeedback();

    if (m_ring.isFullCycle())
        drawParticles(mMV, mProj);

    if (!m_isStopped)
        m_ring.advance();
}
