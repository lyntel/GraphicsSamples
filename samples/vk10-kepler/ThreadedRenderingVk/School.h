//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/School.h
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
#ifndef SCHOOL_H_
#define SCHOOL_H_
#include "NV/NvMath.h"
#include <vector>
#include "NvVkUtil/NvSimpleUBO.h"
#include "NvVkUtil/NvModelExtVK.h"
#include "NvSharedVBOVK.h"

#include <NV/NvPlatformGL.h>
#include "NvGLUtils/NvModelExtGL.h"
#include "NvSharedVBOGL.h"

namespace Nv
{
	class NvInstancedModelExtVK;
	class NvInstancedModelExtGL;
};

class SchoolStateManager;

/// Class to hold settings for controlling the behavior
/// of a school's flocking
class SchoolFlockingParams
{
public:
    SchoolFlockingParams()
        : m_maxSpeed(0.05f)
        , m_inertia(2.0f)
        , m_arrivalDistance(1.0f)
        , m_spawnZoneMin(-20.0f, 5.0f, -20.0f)
        , m_spawnZoneMax( 20.0f, 25.0f, 20.0f)
        , m_neighborDistance(0.5f)
        , m_spawnRange(0.01f)
        , m_aggression(0.5f)
        , m_goalScale(0.03f)
        , m_alignmentScale(0.1f)
        , m_repulsionScale(0.5f)
        , m_cohesionScale(0.1f)
        , m_schoolAvoidanceScale(0.5f)
    {}

    SchoolFlockingParams(
        float maxSpeed, 
        float inertia, 
        float arrivalDistance,
        const nv::vec3f& spawnZoneMin,
        const nv::vec3f& spawnZoneMax,
        float neighborDistance,
        float spawnRange,
        float aggression,
        float goalScale,
        float alignmentScale,
        float repulsionScale,
        float cohesionScale,
        float schoolAvoidanceScale)
        : m_maxSpeed(maxSpeed)
        , m_inertia(inertia)
        , m_arrivalDistance(arrivalDistance)
        , m_spawnZoneMin(spawnZoneMin)
        , m_spawnZoneMax(spawnZoneMax)
        , m_neighborDistance(neighborDistance)
        , m_spawnRange(spawnRange)
        , m_aggression(aggression)
        , m_goalScale(goalScale)
        , m_alignmentScale(alignmentScale)
        , m_repulsionScale(repulsionScale)
        , m_cohesionScale(cohesionScale)
        , m_schoolAvoidanceScale(schoolAvoidanceScale)
    {}

    /// Maximum speed that a fish in the school will move (in m/s)
    float m_maxSpeed;

	/// Amount of influence the fish's current heading has on its future
	/// heading.
	float m_inertia;

    /// Distance from the goal position that the current centroid must be to trigger the 
    /// determination of a new goal position.
    float m_arrivalDistance;

	/// Maximum distance from the origin in each direction that a goal
	/// position will be created
    nv::vec3f m_spawnZoneMin;
    nv::vec3f m_spawnZoneMax;

	/// Maximum distance from a fish that another fish can be to still be
	/// considered a neighbor
	float m_neighborDistance;

	/// Maximum distance from the initial position that each fish will spawn
	/// in at
	float m_spawnRange;

    /// Tendency to approach other fish versus avoid them
    float m_aggression;

    /// Dials for controlling the relative strengths of all influencing factors
    /// on each fish's movement
    float m_goalScale;            /// Swim toward goal
    float m_alignmentScale;       /// Align heading with neighbors
    float m_repulsionScale;       /// Avoid neighbors
    float m_cohesionScale;        /// Keep the school together
    float m_schoolAvoidanceScale; /// Avoid other schools
};

/// School class holds data required to render a school of fish that share
/// a model, with per-instance data controlling the position and orientation
/// of each fish.  Implements flocking behavior for the school of fish.
class School
{
public:
	struct SchoolUBO
	{
		nv::matrix4f m_modelMatrix;
		float m_tailStart;
	};

	struct MeshUBO
	{
		nv::matrix4f m_offsetMatrix;
	};

	School();
	School(const SchoolFlockingParams& params);
	~School();

	/// Initialize the School with beginning state as well as system objects
	/// that it will need to allocate instancing data
	/// \param vk Vulkan context object
    /// \param index Index of the school; used to reference the school's state in the SchoolStateManager
	/// \param pModel Source model to use as the template object when rendering
	///				  instances 
    /// \param tailStartZ Z-position where the tail starts for the fish 
	///                   (for vertex animation)
	/// \param modelTransform Fixup transform to align the provided model to a
	///						  forward of -Z and up of +Y so that the meshes'
	///						  orientation matches the alignment determined by
	///						  flocking, as well as a translation that places
	///						  the center of the model at a local origin.
	/// \param extents Vector containing all positive elements indicating the
	///				   half extents of an axis-aligned bounding box fitting the
	///				   model
	/// \param numFish Initial number of fish to be contained in the school
	/// \param maxFish Maximum number of fish that the school can contain
	/// \param position Spawn position for the school.  Fish will be created
	///					near this point, within the spawn range specified in
	///					the SchoolFlockingParams object passed into the
	///					constructor.
	/// \return True if the school could be initialized with the given values,
	///			false if an error occurred.
    bool Initialize(
        NvVkContext& vk, 
		uint32_t index,
		Nv::NvModelExtVK* pModel, 
        float tailStartZ, 
		const nv::matrix4f& modelTransform, 
        const nv::vec3f& extents, 
		uint32_t numFish, 
		uint32_t maxFish,
        const nv::vec3f& position,
		VkDescriptorSet& desc,
		VkPipelineLayout& layout,
		NvSimpleUBOAllocator& alloc,
		Nv::NvSharedVBOVKAllocator& vboAlloc);

	/// Initialize the GL data associated with the School
	/// \param pModel Source model to use as the template object when rendering
	///				  instances 
    /// \param schoolUniformLocation Uniform Buffer Object Index in fish shaders
    ///                 that contains the per-school data.
    /// \param meshUniformLocation Uniform Index in fish shaders
    ///                 that contains the per-mesh data.
    /// \return True if the school could be initialized with the given values,
	///			false if an error occurred.
	bool InitializeGL(Nv::NvModelExtGL* pModel,
		GLint schoolUniformLocation,
		GLint meshUniformLocation);

	void ResetToLocation(const nv::vec3f& loc);

	void SetInstanceCount(uint32_t instances);

	bool SetModel(NvVkContext& vk, Nv::NvModelExtVK* pModel,
				  float tailStartZ, 
				  const nv::matrix4f& modelTransform, 
				  const nv::vec3f& extents);
	bool SetModelGL(Nv::NvModelExtGL* pModel);

	Nv::NvInstancedModelExtVK* getModel() { return m_pInstancedModel; }
	Nv::NvInstancedModelExtGL* getModelGL() { return m_pInstancedModelGL; }

	/// Updates the flocking simulation and pushes the new state of each fish
	/// into the instance data buffer
	/// \param frameTime Elapsed time for the current frame in seconds
    /// \param pStateManager Pointer to the state manager that holds state
    ///                      information for all schools in the simulation
	/// \param avoid Flag indicating whether the fish in this school should 
	///				 attempt to avoid other schools
	void Animate(float frameTime, SchoolStateManager* pStateManager, bool avoid);

	void Update(bool useGL = false);

	/// Injects commands needed to render the school into the given
	/// CommandBuffer
	/// \param cb Command buffer to append rendering calls to
	/// \param batchSize Number of instances rendered per draw call
	/// \return Returns the number of draw calls invoked during the Render call
	uint32_t Render(VkCommandBuffer& cb, uint32_t batchSize);

	/// Render the school using GL commands and structures
	/// \param batchSize Number of instances rendered per draw call
	/// \return Returns the number of draw calls invoked during the Render call
	uint32_t RenderGL(uint32_t batchSize);

	/// Set the values used by the flocking simulation that drives the school
	/// \param params A SchoolFlockingParams object initialized with the
	///				  desired settings to use for flocking behavior
	void SetFlockingParams(const SchoolFlockingParams& params)
	{
		m_flockParams = params;
	}

	/// Get the current values being used by the school's flocking
	/// simulation
	SchoolFlockingParams GetFlockingParams() { return m_flockParams; }

    /// Retrieve the number of fish in the school
	uint32_t GetNumFish() const { return m_instancesActive; }

    /// Retrieve the last computed centroid for the school
    const nv::vec3f& GetCentroid() const { return m_lastCentroid; }

    /// Retrieve the last computed radius for the school
    float GetRadius() const { return m_lastRadius; }

	NvSimpleUBO<SchoolUBO>& getModelUBO() { return m_schoolUBO; }
	NvSimpleUBO<nv::matrix4f>& getMeshUBO() { return m_meshUBO; }

    void FindNewGoal();

	/// Structure that defines the layout of the data in the instance data
	/// buffer
	struct FishInstanceData
	{
		nv::vec3f m_position;
		nv::vec3f m_heading;
		float     m_tailTime;
	};

private:
	/// Updates the current instance data buffer with the current states of
	/// each fish in the school in preparation for rendering.
	void UpdateInstanceDataBuffer(bool useGL = false);

    /// Index of the school to identify it in the SchoolStateManager
    uint32_t m_index;

	/// Model to be instanced to represent the many fish in the school
	Nv::NvInstancedModelExtVK* m_pInstancedModel;
	Nv::NvInstancedModelExtGL* m_pInstancedModelGL;

	/// Vertex Buffer of instance data that describes the per-fish parameters,
	/// such as position, rotation, etc.
	Nv::NvSharedVBOVK* m_pInstanceData;
	Nv::NvSharedVBOGL* m_pInstanceDataGL;

	/// Number of instances that can be held in the current instance data
	/// buffer
	uint32_t m_instancesCapacity;

	/// Current number of fish active, thus the number of instance data
	/// structures that have valid data
	uint32_t m_instancesActive;

	/// Uniform buffer object providing school-specific parameters to the
	/// shader
	NvSimpleUBO<SchoolUBO> m_schoolUBO;

	NvSimpleUBO<nv::matrix4f> m_meshUBO;

	SchoolUBO   m_schoolUBO_Data;       // Actual values for the UBO
	GLuint      m_schoolUBO_Id;         // UBO Id
	GLint       m_schoolUBO_Location;   // UBO Index in the shader

	MeshUBO m_meshUBO_Data;       // Actual values for the UBO
	GLuint   m_meshUBO_Id;         // UBO Id
	GLint    m_meshUBO_Location;   // Uniform Index

	/// Initial transform that needs to be applied to the mesh to get it into
	/// our sample's coordinate space
	nv::matrix4f m_modelTransform;

	/// Half the extents of the fish model.  Will be used to keep the fish from
	/// intersecting the ground, etc.;
	nv::vec3f m_fishHalfExtents;

	VkDescriptorSet m_descSet;
	VkPipelineLayout m_layout;
	////////////////////////////////
	// Flocking Data
	////////////////////////////////

	/// Current target location which the school is moving towards
	nv::vec3f m_schoolGoal;

	/// location of the centroid of the school at the last update
	nv::vec3f m_lastCentroid;

    /// Radius of the school in the last update
    float m_lastRadius;

	SchoolFlockingParams m_flockParams;

	/// Current state of each fish in the school
	struct FishState
	{
		nv::vec3f m_position;
		nv::vec3f m_heading;
		float m_speed;
        float m_animTime;
        float m_animStartOffset;
	};
	typedef std::vector<FishState> FishSet;
	FishSet m_fish;

	float m_tailStartZ;

	/// Random number generation
	uint32_t m_rndState;
	float Random01()
	{
		m_rndState = (m_rndState * 71359) + 468029;
		return (m_rndState / (float)0xFFFFFFFF);
	}

	/// Helper functions for generating random vectors within a box
	nv::vec3f ScaledRandomVector(float scale)
	{
		return nv::vec3f(Random01() * scale, Random01() * scale,
			Random01() * scale);
	}

	nv::vec3f ScaledRandomVector(float scaleX, float scaleY, float scaleZ)
	{
		return nv::vec3f(Random01() * scaleX, Random01() * scaleY,
			Random01() * scaleZ);
	}

	nv::vec3f ScaledRandomVector(const nv::vec3f& scale)
	{
		return nv::vec3f(Random01() * scale.x, Random01() * scale.y,
			Random01() * scale.z);
	}

};

#endif // SCHOOL_H_
