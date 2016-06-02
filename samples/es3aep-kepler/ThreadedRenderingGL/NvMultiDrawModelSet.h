//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ThreadedRenderingGL/NvMultiDrawModelSet.h
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
#ifndef NvMultiDrawModelSet_H_
#define NvMultiDrawModelSet_H_
#include "NV/NvPlatformGL.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "School.h"
#include <string>
#include <vector>
#include "VertexFormatBinder.h"

namespace Nv
{
    class NvModelExtGL;

    /// Aggregates a set of Models into a single, large IBO/VBO pair and generates
    /// Bindless Texture handles for their associated textures.  When associated
    /// with a set of Schools and their corresponding Pooled VBO Instance data, it
    /// also maintains a set of UBOs that define each school's usage of the shared
    /// buffers and locations within them.  The Model Set also creates a buffer of
    /// DrawCall descriptors that can be passed in, along with the shared IBO, VBO
    /// Instance data and School UBOs to a glMultiDrawIndirect call to render many
    /// schools of fish in a single draw call.
    class NvMultiDrawModelSet
    {
    public:
        /// Initialize the model set
        /// \param maxUBOSize Maximum size, in bytes, that a UBO can be on the 
        ///                   current system.  This is used to determine how many
        ///                   schools can be drawn in a single draw call.
        /// \param pVertexBinder Binder describing the layout of the schools' 
        ///                      associated instance data vertex buffer
        NvMultiDrawModelSet(uint32_t maxUBOSize, VertexFormatBinder* pVertexBinder);
        ~NvMultiDrawModelSet() {}

        /// Defines the set of Models that will be used as the "palette" of 
        /// meshes and textures available to be used to render the schools.
        /// \param pModels Pointer to the array of models to aggregate into 
        ///                the common IBO/VBO pair and for which to create
        ///                Bindless Texture handles.
        /// \param count Number of models in the array
        /// \return True if the models could be aggregated, but false if any
        ///         of the models are not valid to be combined with the others.
        bool SetModels(NvModelExtGL** pModels, uint32_t count);

        /// Define the shader that will be used to render the model set.
        /// \param pShader Pointer to the shader used to render the model set
        void SetShader(NvGLSLProgram* pShader);

        /// Define the vertex format used by the instance data.
        /// \param pVertBinder Pointer to the VertexFormatBinder corresponding to 
        ///                    the vertex layout used by the schools' instance data
        void SetVertexFormatBinder(VertexFormatBinder* pVertBinder) { m_pVertexBinder = pVertBinder; }

        /// Attach the VBO Pool that is used to define the instance
        /// data for the schools rendered by this model set
        /// \param pInstanceData Pointer to the Shared VBO Pool object that
        ///                      all of the schools associated with this
        ///                      model set write their instance data to
        void SetInstanceDataPool(NvSharedVBOGLPool* pInstanceData);

        /// Sets the number of schools to render with this model set
        /// \param numSchools Number of Schools to render
        void SetNumSchools(uint32_t numSchools);

        /// Update the data defining the rendering properties of a singel
        /// school within the model set
        /// \param pSchool Pointer to the school for which to modify the settings
        /// \param baseInstance Offset into the Pooled instance data the indicates the
        ///                     start of the given school's  instances
        /// \return True if the school's data could be updated. False if the school
        ///         was not a valid member of the model set or its base instance
        ///         was invalid.
        bool UpdateSchoolData(School* pSchool, uint32_t baseInstance);

        /// Render the set of schools contained in the model set
        /// \param positionHandle Attribute index of the component in the shader that
        ///                       is to be used for vertex position data
        /// \param normalHandle Attribute index of the component in the shader that
        ///                     is to be used for vertex normal data, if there is one.
        ///                     A value of -1 indicates that the shader does not use
        ///                     normals from the vertex data
        /// \param texcoordHandle Attribute index of the component in the shader that
        ///                       is to be used for texture coordinate data, if there is 
        ///                       one.  A value of -1 indicates that the shader does not use
        ///                       texture coordinates from the vertex data
        /// \param tangentHandle Attribute index of the component in the shader that
        ///                       is to be used for tangent data, if there is one.
        ///                       A value of -1 indicates that the shader does not use
        ///                       tangents from the vertex data
        /// \return The number of glMultiDrawElementsIndirect calls made during the call
        uint32_t Render(GLint positionHandle, GLint normalHandle = -1, GLint texcoordHandle = -1, GLint tangentHandle = -1);

        /// Returns the maximum number of schools that can be rendered at once based on 
        /// the size of the per-school UBO data and the maximum size of a UBO as supported
        /// by the current hardware.
        /// \return Number of schools worth of data that can be contained in a single UBO,
        ///         thus the maximum number that may be drawn in a single draw call.
        uint32_t GetMaxSchoolsPerUBO() const { return m_schoolsPerUBO; }

    private:
        NvGLSLProgram* m_pShader;

        // Bindless Textures
        // Array of handles to fish textures, one per fish model, and in the same order
        // as the models are in their array
        GLuint64EXT* m_bindlessTextureHandles;  
        uint32_t     m_numBindlessTextures;

        // Vertex buffer containing all vertices from all models in the set,
        // concatenated in order
        GLuint m_vboID;

        // Index buffer containing all indices from all models in the set,
        // concatenated in order
        GLuint m_iboID;

        // Total number of vertices after combining the vertices from all models
        uint32_t m_vertexCount;

        // Total number of indices after combining the indices from all models
        GLsizei m_indexCount;

        // Vertex layout descriptors
        int32_t m_vertexSize;       // in bytes

        int32_t m_positionSize;     // in floats
		intptr_t m_positionOffset;  // in bytes
        
        int32_t m_normalSize;       // in floats
		intptr_t m_normalOffset;    // in bytes

        int32_t m_texCoordSize;     // in floats
		intptr_t m_texCoordOffset;  // in bytes

        int32_t m_tangentSize;      // in floats
		intptr_t m_tangentOffset;   // in bytes

        // Struct to hold the size and offsets for each model contained
        // in our shared vertex/index buffers
        struct ModelDef
        {
            GLuint   count;
            GLuint   baseIndex;
            GLuint   baseVertex;
        };
        ModelDef* m_models;
        uint32_t  m_modelCount;

        // Struct based on GL_ARB_draw_indirect format
        struct DrawElementsIndirectCommand
        {
            GLuint   count;
            GLuint   instanceCount;
            GLuint   baseIndex;
            GLuint   baseVertex;
            GLuint   baseInstance;
        };

        // We maintain our own copy of all current draw commands as we have to
        // initialize the new draw command buffer with existing commands every
        // time that we grow the buffer.
        DrawElementsIndirectCommand* m_drawCommands;
        
        // The actual indirect draw commands buffer is mapped persistently
        DrawElementsIndirectCommand* m_mappedCommands;

        // Generated buffer ID for the Indirect Draw Command buffer
        GLuint m_indirectBufferID;

        // Current size of the m_drawCommands array
        uint32_t m_drawCommandsCapacity;

        // Number of valid commands in the m_drawCommands array 
        // (sequential, starting from the beginning of the array)
        uint32_t m_numActiveDrawCommands;

        // Structure defining the layout for the per-school data that will be sent
        // to the shader via UBO.  
        struct SchoolData
        {
            nv::matrix4f m_modelMatrix;
            uint64_t     m_textureHandle;   // Must be 8-byte aligned
            float        m_tailStart;
            uint32_t m_padding4;            // Structure will be padded out to 16-byte
                                            // alignment in the shader, so we need to 
                                            // make sure that our representation matches
        };

        // Number of schools worth of SchoolData that will fit into a single UBO
        // given the UBO size limitations of the current hardware.
        uint32_t m_schoolsPerUBO;

        // Wrapper to hold each UBOs generated name along with its mapped 
        // memory
        struct UBO
        {
            GLuint m_uboID;
            SchoolData* m_pData;
        };
        typedef std::vector<UBO> UBOArray;
        UBOArray m_ubos;

        // Locations within the shader for each of the parameters that we need
        // to set before rendering.
        GLuint   m_uboLocation;             // Location of the per-school SchoolData array UBO
        GLuint   m_startingSchoolLocation;  // Index of the first school to be drawn in this call.
                                            // Used to determine where in the current UBO a particular
                                            // school's SchoolData resides.

        // Binder defining the schools' instance data vertex layout
        VertexFormatBinder* m_pVertexBinder;

        // Pooled VBO containing the instance data for all schools
        NvSharedVBOGLPool* m_instanceDataPool;
    };
}

#endif // NvMultiDrawModelSet_H_
