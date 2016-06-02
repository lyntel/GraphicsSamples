//----------------------------------------------------------------------------------
// File:        es2-aurora\InstancedTessellation/InstancedTessellation.cpp
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
#include "InstancedTessellation.h"
#include "NvUI/NvTweakBar.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvModel/NvModel.h"
#include "NvGLUtils/NvModelGL.h"
#include "NV/NvLogs.h"
#include "Half/half.h"
#include <string>

typedef void (KHRONOS_APIENTRY *PFNDrawElementsInstanced)(GLenum mode, GLsizei count,GLenum type, const void* indices, GLsizei primcount);
typedef void (KHRONOS_APIENTRY *PFNVertexAttribDivisor)(GLuint index, GLuint divisor);

PFNDrawElementsInstanced glDrawElementsInstancedInternal = NULL;
PFNVertexAttribDivisor glVertexAttribDivisorInternal = NULL;

// HACK - these are NOT the same value...
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#endif

static const nv::vec3f m_modelRotation[eNumModels] =
{
        nv::vec3f(   0.0f,   45.0f, 90.0f ),
        nv::vec3f(   0.0f,  225.0f, 90.0f ),
        nv::vec3f(   0.0f,    0.0f,  0.0f ),
};

static const char* m_pModelNames[eNumModels] =
{
        "buddha",
        "armadillo",
        "dragon"
};

InstancedTessellation::InstancedTessellation() : 
    m_wireframe(true)
{
    m_lightAmbient   = 0.1f;  //ambient component
    m_lightDiffuse   = 0.7f;  //diffuse component
   m_time           = 0.0f;

    m_lightPosition = nv::vec3f(1.0f, 1.0f, 1.0f);
    m_lightType       = 0.0f; //1.0f = point light / 0.0f = directional light

    m_modelColor.x  = 1.0f;
    m_modelColor.y  = 1.0f;
    m_modelColor.z  = 1.0f;
    m_tessFactor     = 3;

    m_hwInstancing = false;
    m_pausedByPerfHUD = false;
    m_instancing = false;
    m_modelIndex = eBuddha;
    m_tessMode   = eFirstTessellationMode;

    for( int i = 0; i < eNumModels; ++ i )
    {
        m_pModel[ i ] = 0;
        m_pTriangleData[ i ] = 0;
        m_perTriangleDataVboID[ i ] = INVALID_ID;
      m_ModelMatrix[i] = nv::matrix4f();
    }

   m_ModelMatrix[ eBuddha ] =  nv::rotationYawPitchRoll( m_ModelMatrix[ eBuddha ], 11.0f/7.0f, 11.0f/7.0f, 0.0f );
   m_ModelMatrix[ eArmadillo ] =  nv::rotationYawPitchRoll( m_ModelMatrix[ eArmadillo ], -11.0f/7.0f, 33.0f/7.0f, 0.0f );

    for( int i = 0; i < MAX_TESS_LEVEL; ++ i )
    {
        m_tessVboID[ i ] = INVALID_ID;
        m_tessIboID[ i ] = INVALID_ID;
        m_wireframeTessIboID[ i ] = INVALID_ID;

    }

    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -25.0f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

InstancedTessellation::~InstancedTessellation()
{
    delete m_npatchShader;
    delete m_instancedNPatchShader;
    delete m_deformationShader;
    delete m_instancedDeformationShader;

    for( int i = 0; i < eNumModels; ++ i )
    {
        if( m_pModel[ i ] != 0 )
            delete m_pModel[ i ];
        m_pModel[ i ] = 0;

        if( m_pTriangleData[ i ] != 0 )
            delete[] m_pTriangleData[ i ];
        m_pTriangleData[ i ] = 0;

        if( m_perTriangleDataVboID[ i ] != INVALID_ID )
            glDeleteBuffers( 1, & m_perTriangleDataVboID[ i ] );
        m_perTriangleDataVboID[ i ] = INVALID_ID;
    }

    for( int i = 0; i < MAX_TESS_LEVEL; ++ i )
    {
        if( m_tessVboID[ i ] != INVALID_ID )
            glDeleteBuffers( 1, & m_tessVboID[ i ] );
        m_tessVboID[ i ] = INVALID_ID;

        if( m_tessIboID[ i ] != INVALID_ID )
            glDeleteBuffers( 1, & m_tessIboID[ i ] );
        m_tessIboID[ i ] = INVALID_ID;

        if( m_wireframeTessIboID[ i ] != INVALID_ID )
            glDeleteBuffers( 1, & m_wireframeTessIboID[ i ] );
        m_wireframeTessIboID[ i ] = INVALID_ID;
    }

    LOGI("InstancedTessellation: destroyed\n");
}

void InstancedTessellation::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionES3();
}

void InstancedTessellation::initUI() {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        // models
        NvTweakEnum<uint32_t> Models[] =
        {
            {m_pModelNames[eBuddha], eBuddha},
            {m_pModelNames[eArmadillo], eArmadillo},
            {m_pModelNames[eDragon], eDragon},
        };
        mTweakBar->addEnum("Models:", m_modelIndex, Models, TWEAKENUM_ARRAYSIZE(Models));

        // tessellation modes
        mTweakBar->addPadding();
        NvTweakEnum<uint32_t> tessellationModes[] =
        {
            {"NPatch", eNPatch},
            {"Deformation", eDeformation},
        };
        mTweakBar->addEnum("Render Mode:", m_tessMode, tessellationModes, TWEAKENUM_ARRAYSIZE(tessellationModes));

        // instance count
        mTweakBar->addPadding();
        mTweakBar->addValue("Tess Factor:", m_tessFactor, 1, MAX_TESS_LEVEL, 1);

        // tessellation modes
        mTweakBar->addPadding();
        NvTweakEnum<uint32_t> instancingModes[] =
        {
            {"Shader Instancing", 0},
            {"Hardware Instancing", 1},
        };
        if( m_hwInstancing )
         mTweakBar->addEnum("Instancing Mode:", m_instancing, instancingModes, TWEAKENUM_ARRAYSIZE(instancingModes));
        else
         mTweakBar->addEnum("Instancing Mode:", m_instancing, instancingModes, TWEAKENUM_ARRAYSIZE(instancingModes)-1);

        // rednering modes
        mTweakBar->addPadding();
        NvTweakEnum<uint32_t> renderingModes[] =
        {
            {"Solid", 0},
            {"Wireframe", 1},
        };
        mTweakBar->addEnum("Rendering Mode:", m_wireframe, renderingModes, TWEAKENUM_ARRAYSIZE(renderingModes));
    }
}

void InstancedTessellation::drawModelLitInstancingON()
{
    InstancedTessShader* shader = m_tessMode == eNPatch ? m_instancedNPatchShader : m_instancedDeformationShader;

   shader->enable();

    if( m_tessMode == eDeformation )
    {
        float time = m_time;

        m_deformationShader->setUniform1f( m_deformationShader->m_timeHandle, time );
    }

    shader->setUniformMatrix4fv(shader->m_modelViewMatrixHandle,  m_viewMatrix._array, 1, GL_FALSE);
    shader->setUniformMatrix4fv(shader->m_projectionMatrixHandle, m_projectionMatrixHandle._array, 1, GL_FALSE);
    shader->setUniform4f(shader->m_lightPositionHandle1, m_lightPositionEye.x, m_lightPositionEye.y, m_lightPositionEye.z, m_lightPositionEye.w);
    shader->setUniform4f(shader->m_colorHandle, m_modelColor.x, m_modelColor.y, m_modelColor.z, 1.0f);

    //m_normalMatrixHandle now contains a 3x3 inverse component of the m_modelViewMatrixHandle
    //hence, we need to transpose this before sending it to the shader
    shader->setUniformMatrix4fv(shader->m_normalMatrixHandle, m_normalMatrixHandle._array, 1, GL_FALSE);

    glBindBuffer(GL_ARRAY_BUFFER, m_tessVboID[m_tessFactor-1] );
    if( m_wireframe == false )
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_tessIboID[m_tessFactor-1] );
    else
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_wireframeTessIboID[m_tessFactor-1] );

    glVertexAttribPointer(shader->m_b01Handle, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0 ) ;
    glEnableVertexAttribArray(shader->m_b01Handle);

    NvModel*       pModel         = m_pModel[m_modelIndex]->getModel();
	NvModelPrimType::Enum prim;
	int            numFaces = pModel->getCompiledIndexCount(prim) / 3;

    glBindBuffer(GL_ARRAY_BUFFER, m_perTriangleDataVboID[m_modelIndex] );

    glVertexAttribPointer(shader->m_posHandle0, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float), 0 ) ;
    glEnableVertexAttribArray(shader->m_posHandle0);
    glVertexAttribPointer(shader->m_posHandle1, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float), (GLvoid*)(3 * sizeof(float) ) ) ;
    glEnableVertexAttribArray(shader->m_posHandle1);
    glVertexAttribPointer(shader->m_posHandle2, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float), (GLvoid*)(6 * sizeof(float) ) ) ;
    glEnableVertexAttribArray(shader->m_posHandle2);
    glVertexAttribPointer(shader->m_normHandle0, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float), (GLvoid*)(9 * sizeof(float) ) ) ;
    glEnableVertexAttribArray(shader->m_normHandle0);
    glVertexAttribPointer(shader->m_normHandle1, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float), (GLvoid*)(12 * sizeof(float) ) ) ;
    glEnableVertexAttribArray(shader->m_normHandle1);
    glVertexAttribPointer(shader->m_normHandle2, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float), (GLvoid*)(15 * sizeof(float) ) ) ;
    glEnableVertexAttribArray(shader->m_normHandle2);

    glVertexAttribDivisorInternal( shader->m_posHandle0, 1 );
    glVertexAttribDivisorInternal( shader->m_posHandle1, 1 );
    glVertexAttribDivisorInternal( shader->m_posHandle2, 1 );
    glVertexAttribDivisorInternal( shader->m_normHandle0, 1 );
    glVertexAttribDivisorInternal( shader->m_normHandle1, 1 );
    glVertexAttribDivisorInternal( shader->m_normHandle2, 1 );

    if( m_wireframe == false )
        glDrawElementsInstancedInternal(GL_TRIANGLES, ( m_tessFactor * m_tessFactor ) * 3, GL_UNSIGNED_INT, 0, numFaces );
    else
        glDrawElementsInstancedInternal(GL_LINES, ( ( ( m_tessFactor + 1 ) * m_tessFactor ) / 2 ) * 3 * 2, GL_UNSIGNED_INT, 0, numFaces );

    glVertexAttribDivisorInternal( shader->m_posHandle0, 0 );
    glVertexAttribDivisorInternal( shader->m_posHandle1, 0 );
    glVertexAttribDivisorInternal( shader->m_posHandle2, 0 );
    glVertexAttribDivisorInternal( shader->m_normHandle0, 0 );
    glVertexAttribDivisorInternal( shader->m_normHandle1, 0 );
    glVertexAttribDivisorInternal( shader->m_normHandle2, 0 );

    glDisableVertexAttribArray(shader->m_posHandle0);
    glDisableVertexAttribArray(shader->m_posHandle1);
    glDisableVertexAttribArray(shader->m_posHandle2);
    glDisableVertexAttribArray(shader->m_normHandle0);
    glDisableVertexAttribArray(shader->m_normHandle1);
    glDisableVertexAttribArray(shader->m_normHandle2);

    glBindBuffer(GL_ARRAY_BUFFER, m_tessVboID[m_tessFactor-1] );
    glDisableVertexAttribArray(shader->m_b01Handle);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void InstancedTessellation::drawModelLitInstancingOFF()
{
    BaseShader* shader = m_tessMode == eNPatch ? m_npatchShader : m_deformationShader;

    if( m_tessMode == eNPatch )
        m_npatchShader->enable();
    else
        m_deformationShader->enable();

    if( m_tessMode == eDeformation )
    {
        float time = m_time;

        glUniform1f( m_deformationShader->m_timeHandle, time );
    }

    //send matrices
    glUniformMatrix4fv(shader->m_modelViewMatrixHandle, 1, GL_FALSE,  m_viewMatrix._array);
    glUniformMatrix4fv(shader->m_projectionMatrixHandle, 1, GL_FALSE, m_projectionMatrixHandle._array);

    glUniform4fv(shader->m_lightPositionHandle1, 1, m_lightPositionEye._array);

    glUniform4f(shader->m_colorHandle, m_modelColor.x, m_modelColor.y, m_modelColor.z, 1.0f);

    //m_normalMatrixHandle now contains a 3x3 inverse component of the m_modelViewMatrixHandle
    //hence, we need to transpose this before sending it to the shader
    glUniformMatrix4fv(shader->m_normalMatrixHandle, 1, GL_FALSE, m_normalMatrixHandle._array);

    glBindBuffer(GL_ARRAY_BUFFER, m_tessVboID[m_tessFactor-1] );
    if( m_wireframe == false )
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_tessIboID[m_tessFactor-1] );
    else
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_wireframeTessIboID[m_tessFactor-1] );

    glVertexAttribPointer(shader->m_b01Handle, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0 ) ;
    glEnableVertexAttribArray(shader->m_b01Handle);

    NvModel*       pModel         = m_pModel[m_modelIndex]->getModel();
	NvModelPrimType::Enum prim;
	int              numFaces = pModel->getCompiledIndexCount(prim) / 3;

    for( int i = 0; i < numFaces; ++i )
    {
        if( m_tessMode == eNPatch )
        {
            glUniform3fv( m_npatchShader->m_posHandle, 3, (GLfloat*)&((m_pTriangleData[m_modelIndex])[i].m_p0) );
            glUniform3fv( m_npatchShader->m_normHandle, 3, (GLfloat*)&((m_pTriangleData[m_modelIndex])[i].m_n0) );
        }
        else
        {
            glUniform3fv( m_deformationShader->m_posHandle, 3, (GLfloat*)&((m_pTriangleData[m_modelIndex])[i].m_p0) );
            glUniform3fv( m_deformationShader->m_normHandle, 3, (GLfloat*)&((m_pTriangleData[m_modelIndex])[i].m_n0) );
        }

        if( m_wireframe == false )
            glDrawElements(GL_TRIANGLES, ( m_tessFactor * m_tessFactor ) * 3, GL_UNSIGNED_INT, 0);
        else
            glDrawElements(GL_LINES, ( ( ( m_tessFactor + 1 ) * m_tessFactor ) / 2 ) * 3 * 2, GL_UNSIGNED_INT, 0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_tessVboID[m_tessFactor-1] );
    glDisableVertexAttribArray(shader->m_b01Handle);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void InstancedTessellation::drawModelLit() {

   m_time += getFrameDeltaTime();
    if (m_instancing&&m_hwInstancing)
        drawModelLitInstancingON();
    else
        drawModelLitInstancingOFF();

    CHECK_GL_ERROR();
}

void InstancedTessellation::initShaders() {
    //load shaders
    m_npatchShader = new TessShader;
    m_instancedNPatchShader = new InstancedTessShader;
    m_deformationShader = new DeformShader;
    m_instancedDeformationShader = new InstancedDeformShader;

    //assign some values which never change
    m_npatchShader->enable();

    //send light params
    m_npatchShader->setUniform1f(m_npatchShader->m_lightAmbientHandle1, m_lightAmbient);
    m_npatchShader->setUniform1f(m_npatchShader->m_lightDiffuseHandle1, m_lightDiffuse);

    //assign some values which never change
    m_instancedNPatchShader->enable();

    //send light params
    m_instancedNPatchShader->setUniform1f(m_instancedNPatchShader->m_lightAmbientHandle1, m_lightAmbient);
    m_instancedNPatchShader->setUniform1f(m_instancedNPatchShader->m_lightDiffuseHandle1, m_lightDiffuse);

    //assign some values which never change
    m_instancedDeformationShader->enable();

    //send light params
    m_instancedDeformationShader->setUniform1f(m_instancedDeformationShader->m_lightAmbientHandle1, m_lightAmbient);
    m_instancedDeformationShader->setUniform1f(m_instancedDeformationShader->m_lightDiffuseHandle1, m_lightDiffuse);

    //assign some values which never change
    m_deformationShader->enable();

    //send light params
    m_deformationShader->setUniform1f(m_deformationShader->m_lightAmbientHandle1, m_lightAmbient);
    m_deformationShader->setUniform1f(m_deformationShader->m_lightDiffuseHandle1, m_lightDiffuse);

    m_deformationShader->disable();
}

void InstancedTessellation::initGlobalVariables() {

}

void InstancedTessellation::loadModelFromData(int32_t modelIndex, char *pFileData) {
	m_pModel[modelIndex] = NvModelGL::CreateFromObj((uint8_t*)pFileData, 10.0f, true);

    //print the number of vertices...
    LOGI("Model Loaded %d - %d vertices\n", modelIndex, m_pModel[modelIndex]->getModel()->getCompiledVertexCount());
}

bool InstancedTessellation::initGeneralTessellationInstancingData()
{
    // loop over the number of supported tessellation levels
    for( int tl = 1; tl <= MAX_TESS_LEVEL; ++tl )
    {
        // a triangular patch with n segments per edge has n + 1 vertices on an edge
        // => the whole tessellation has ( ( n + 2 ) * ( n + 1 ) / 2 ) vertices
        int    vertexCount  = ( ( tl + 2 ) * ( tl + 1 ) ) / 2;
        int    floatCount   = 2 * vertexCount;

        // array big enough to hold the barycentric coords of all vertices
        float* pV              = new float[ floatCount ];
        //half* pV              = new half[ floatCount ];
        float  b2 = 0, b2inc = 1.0f / float( tl  );
        int    i = 0;
        for( int b2i = 0; b2i < ( tl + 1 ); ++b2i, b2 += b2inc )
        {
            float b0 = 1.0f - b2, b0dec = b0 / float( tl - b2i );

            for( int b0i = tl  - b2i, b1i = 0; b0i >= 0; ++b1i, --b0i, b0 -= b0dec )
            {
                float b1 = 1.0f - b0 - b2;

                b0 = ( b0 <= 0.0f ? 0.0f : b0 );
                b1 = ( b1 <= 0.0f ? 0.0f : b1 );

                pV[i*2+0] = b0;
                pV[i*2+1] = b1;
                //pV[i*2+0] = half(b0);
                //pV[i*2+1] = half(b1);

                if( tl == 7 )
                {
                    LOGI( "Tf=7 vtx%d(%f,%f,%f)", i, b0, b1, b2 );
                }

                ++i;
            }
        }

        // the triangulation has ( n  * n ) triangles and x 3 indices
        int       intCount   = ( tl  * tl ) * 3;
        GLuint*   pI          = new GLuint[ intCount ];
        int       num        = int( tl + 1 );
        int       cnt          = 0;

        i = 0;
        for( int row = 0; row <= ( tl - 1 ); ++row, --num, ++cnt )
        {
            for( int vtx = 0; vtx < tl - row; ++vtx )
            {
                // add upwards triangle
                pI[ i ] = GLuint(cnt), ++i, ++cnt;
                pI[ i ] = GLuint(cnt), ++i;
                pI[ i ] = GLuint(cnt + num - 1), ++i;

                if( tl == 7 )
                {
                    LOGI( "Tf=7 tri%d(%d,%d,%d)", (i-3)/3, int(pI[i-3]), int(pI[i-2]), int(pI[i-1]) );
                }

                // add downwards triangle if appropriate
                if( vtx < tl - row - 1 )
                {
                    pI[ i ] = GLuint(cnt), ++i;
                    pI[ i ] = GLuint(cnt + num), ++i;
                    pI[ i ] = GLuint(cnt + num - 1), ++i;

                    if( tl == 7 )
                    {
                        LOGI( "Tf=7 tri%d(%d,%d,%d)", (i-3)/3, int(pI[i-3]), int(pI[i-2]), int(pI[i-1]) );
                    }
                }
            }
        }

        // setup index buffer for wireframe line rendering
        // the triangulation has ( ( (n+1)  * n )/2 ) x 3 edges
        int       iwfIntCount = ( ( ( tl + 1 ) * tl ) / 2 ) * 3 * 2;
        GLuint*   pwfI           = new GLuint[ iwfIntCount ];
        num                   = int( tl + 1 );
        cnt                   = 0;

        i = 0;
        for( int row = 0; row <= ( tl - 1 ); ++row, --num, ++cnt )
        {
            for( int vtx = 0; vtx < tl - row; ++vtx )
            {
                // add upwards triangle horz edge
                pwfI[ i ] = GLuint(cnt), ++i, ++cnt;
                pwfI[ i ] = GLuint(cnt), ++i;

                pwfI[ i ] = GLuint(cnt - 1), ++i;
                pwfI[ i ] = GLuint(cnt + num - 1), ++i;

                pwfI[ i ] = GLuint(cnt), ++i;
                pwfI[ i ] = GLuint(cnt + num - 1), ++i;
            }
        }

        glGenBuffers(1, &m_tessVboID[tl-1]);
        glBindBuffer(GL_ARRAY_BUFFER, m_tessVboID[tl-1]);
        glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), pV, GL_STATIC_DRAW);
        //glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(half), pV, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &m_tessIboID[tl-1]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_tessIboID[tl-1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, intCount * sizeof(GLuint), pI, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glGenBuffers(1, &m_wireframeTessIboID[tl-1]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_wireframeTessIboID[tl-1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, iwfIntCount * sizeof(GLuint), pwfI, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        delete[] pV;
        delete[] pI;
        delete[] pwfI;
    }

    return true;
}

bool InstancedTessellation::initPerModelTessellationInstancingData( NvModelGL* mdl, int32_t modelIndex )
{
	NvModelPrimType::Enum prim;
	NvModel*       pModel = mdl->getModel();
    int              numFaces       = pModel->getCompiledIndexCount(prim) / 3;
	const GLuint*    pIndices = pModel->getCompiledIndices(prim);
    const float*     pVertices      = pModel->getCompiledVertices();
    int                 floatsPerV     = pModel->getCompiledVertexSize();
    int                 vtxOffset      = pModel->getCompiledPositionOffset();
    int                 normalOffset   = pModel->getCompiledNormalOffset();
    int                 textureOffset  = pModel->getCompiledTexCoordOffset();
    PerInstanceData* pId            = new PerInstanceData[ numFaces ];

    m_pTriangleData[ modelIndex ] = pId;

    // build per instance/triangle data in memory
    for( int i = 0; i < numFaces; ++i )
    {
        GLuint          idx[ 3 ];
        const float* pVertex[ 3 ];
        nv::vec3f    v[ 3 ];
        nv::vec3f    n[ 3 ];
        nv::vec2f    txt[ 3 ];

        // get indices
        idx[0] = pIndices[ i * 3 + 0 ];
        idx[1] = pIndices[ i * 3 + 1 ];
        idx[2] = pIndices[ i * 3 + 2 ];

        // get vertices and normals
        pVertex[0] = pVertices + floatsPerV * idx[ 0 ];
        pVertex[1] = pVertices + floatsPerV * idx[ 1 ];
        pVertex[2] = pVertices + floatsPerV * idx[ 2 ];

        // copy vertex data
        v[0] = *((nv::vec3f*)(pVertex[0]+vtxOffset));
        v[1] = *((nv::vec3f*)(pVertex[1]+vtxOffset));
        v[2] = *((nv::vec3f*)(pVertex[2]+vtxOffset));
        n[0] = nv::normalize( *((nv::vec3f*)(pVertex[0]+normalOffset)) );
        n[1] = nv::normalize( *((nv::vec3f*)(pVertex[1]+normalOffset)) );
        n[2] = nv::normalize( *((nv::vec3f*)(pVertex[2]+normalOffset)) );
        txt[0] = *( (nv::vec2f*) (pVertex[0]+textureOffset) );
        txt[1] = *( (nv::vec2f*) (pVertex[1]+textureOffset) );
        txt[2] = *( (nv::vec2f*) (pVertex[2]+textureOffset) );

        // initialize interpolated vertices
        pId[ i ].m_p0         = v[0];
        pId[ i ].m_p1         = v[1];
        pId[ i ].m_p2         = v[2];
        pId[ i ].m_n0         = n[0];
        pId[ i ].m_n1         = n[1];
        pId[ i ].m_n2         = n[2];
        //pId[ i ].m_t0t1       = nv::vec4f(txt[0].x, txt[0].y, txt[1].x, txt[1].y );
        //pId[ i ].m_t2         = nv::vec2f(txt[2].x, txt[2].y );
    }

    glGenBuffers(1, &m_perTriangleDataVboID[modelIndex]);
    glBindBuffer(GL_ARRAY_BUFFER, m_perTriangleDataVboID[modelIndex]);
    glBufferData(GL_ARRAY_BUFFER, numFaces * sizeof(PerInstanceData), pId, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void InstancedTessellation::initRendering(void) {
   if( requireMinAPIVersion(NvGLAPIVersionES3(), false) ) {
         m_hwInstancing = true;
         glDrawElementsInstancedInternal = (PFNDrawElementsInstanced)getGLContext()->getGLProcAddress("glDrawElementsInstanced");
         glVertexAttribDivisorInternal = (PFNVertexAttribDivisor)getGLContext()->getGLProcAddress("glVertexAttribDivisor");
   }
   else {
      // We need at least _one_ of these two extensions
      if (!requireExtension("GL_ARB_instanced_arrays", false) &&
          !requireExtension("GL_NV_draw_instanced", false)) {
         m_hwInstancing = false;
      }
      else {
         m_hwInstancing = true;

         if (requireExtension("GL_ARB_instanced_arrays", false) ) {
            glDrawElementsInstancedInternal = (PFNDrawElementsInstanced)getGLContext()->getGLProcAddress("glDrawElementsInstancedARB");
            glVertexAttribDivisorInternal = (PFNVertexAttribDivisor)getGLContext()->getGLProcAddress("glVertexAttribDivisorARB");
         }
         else {
            glDrawElementsInstancedInternal = (PFNDrawElementsInstanced)getGLContext()->getGLProcAddress("glDrawElementsInstancedNV");
            glVertexAttribDivisorInternal = (PFNVertexAttribDivisor)getGLContext()->getGLProcAddress("glVertexAttribDivisorNV");
         }
      }
   }

   if( m_hwInstancing ) {
      m_instancing = true;
   }

   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    

   NvAssetLoaderAddSearchPath("es2-aurora/InstancedTessellation");

   LOGI("Hardware Instancing %s\n", m_hwInstancing ? "Available" : "Not available" );

    int len;
    char *pBuff;

   initShaders();

    for( int i = 0; i < eNumModels; ++i )
    {
        std::string path = "models/";
        path += m_pModelNames[i];
        path += ".obj";
        pBuff = NvAssetLoaderRead(path.c_str(), len);
        loadModelFromData(i,pBuff);
        initPerModelTessellationInstancingData(m_pModel[i], i );

        delete [] pBuff;
    }

    initGeneralTessellationInstancingData();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.0, 0.0, 0.0, 1.0);

   CHECK_GL_ERROR();
}

void InstancedTessellation::reshape(int32_t width, int32_t height)
{
    nv::perspective(m_projectionMatrixHandle, NV_PI / 3.0f,    (float)m_width / (float)m_height, 1.0f, 150.0f);

    glViewport( 0, 0, (GLint) width, (GLint) height );

    CHECK_GL_ERROR();
}

void InstancedTessellation::draw(void)
{
    //matrices for translation and rotation

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (isMobilePlatform()) {
        nv::matrix4f translation;
        m_viewMatrix = m_transformer->getModelViewMat() * m_ModelMatrix[ m_modelIndex ] ;

            //note: there is no m_pModel matrix as the m_pModel is always at origin(0,0,0) in the world-space

            m_normalMatrixHandle = nv::inverse(m_viewMatrix);
            m_normalMatrixHandle = nv::transpose(m_normalMatrixHandle);

            m_lightPositionEye = nv::vec4f(m_lightPosition.x, m_lightPosition.y, m_lightPosition.z, m_lightType);

        drawModelLit();
    } else {
       for( int x = -1; x <= 1; ++x )
       {
          for( int y = 0; y <= 0; ++y )
          {
             for( int z = 0; z <= 3; ++z )
             {
                nv::matrix4f translation = nv::translation( translation, 20.0f * float(x), 18.0f *  float(y), -18.0f *  float( z ) );
                m_viewMatrix = m_transformer->getModelViewMat() * translation * m_ModelMatrix[ m_modelIndex ] ;

                 //note: there is no m_pModel matrix as the m_pModel is always at origin(0,0,0) in the world-space

                 m_normalMatrixHandle = nv::inverse(m_viewMatrix);
                 m_normalMatrixHandle = nv::transpose(m_normalMatrixHandle);

                 m_lightPositionEye = nv::vec4f(m_lightPosition.x, m_lightPosition.y, m_lightPosition.z, m_lightType);

                 drawModelLit();
             }
          }
       }
    }
}

bool InstancedTessellation::handleGamepadChanged(uint32_t changedPadFlags) {
    if (changedPadFlags) {
        NvGamepad* pad = getPlatformContext()->getGamepad();
        if (!pad) return false;

        LOGI("gamepads: 0x%08x", changedPadFlags);
    }

    return false;
}


NvAppBase* NvAppFactory() {
    return new InstancedTessellation();
}
