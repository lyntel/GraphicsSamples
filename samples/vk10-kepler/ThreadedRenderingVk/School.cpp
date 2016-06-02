//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/School.cpp
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
#include "School.h"
#include "SchoolStateManager.h"
#include "NV/NvQuaternion.h"

#include "NvInstancedModelExtVK.h"
#include "NvInstancedModelExtGL.h"

#define ENABLE_AVOIDANCE 1

School::School()
	: m_pInstancedModel(nullptr)
	, m_pInstanceData(nullptr)
	, m_pInstancedModelGL(nullptr)
	, m_pInstanceDataGL(nullptr)
	, m_instancesCapacity(0)
	, m_instancesActive(0)
    , m_fishHalfExtents(0.0f, 0.0f, 0.0f)
    , m_schoolGoal(0.0f, 0.0f, 0.0f)
    , m_lastCentroid(0.0f, 0.0f, 0.0f)
    , m_lastRadius(0.0f)
	, m_tailStartZ(0.0f)
{}

School::School(const SchoolFlockingParams& params)
	: m_pInstancedModel(nullptr)
	, m_pInstanceData(nullptr)
	, m_pInstancedModelGL(nullptr)
	, m_pInstanceDataGL(nullptr)
	, m_instancesCapacity(0)
	, m_instancesActive(0)
	, m_flockParams(params)
    , m_fishHalfExtents(0.0f, 0.0f, 0.0f)
    , m_schoolGoal(0.0f, 0.0f, 0.0f)
    , m_lastCentroid(0.0f, 0.0f, 0.0f)
    , m_lastRadius(0.0f)
{}

School::~School()
{
	m_pInstancedModel = nullptr;
	if (nullptr != m_pInstanceData)
	{
		delete m_pInstanceData;
	}

	m_pInstancedModelGL = nullptr;
	if (nullptr != m_pInstanceDataGL)
	{
		delete m_pInstanceDataGL;
	}

	m_schoolUBO.Finalize();

	glDeleteBuffers(1, &m_schoolUBO_Id);
	glDeleteBuffers(1, &m_meshUBO_Id);
}

bool School::Initialize(NvVkContext& vk, 
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
						Nv::NvSharedVBOVKAllocator& vboAlloc)
{
	if (!m_schoolUBO.Initialize(vk, alloc))
	{
		return false;
	}
	
	if (!m_meshUBO.Initialize(vk, alloc))
	{
		return false;
	}
	
	// Create our instance data that we will be updating each frame
    m_pInstanceData = new Nv::NvSharedVBOVK();
    if ((nullptr == m_pInstanceData) || 
		!m_pInstanceData->Initialize(&vk, 
			sizeof(FishInstanceData) * maxFish, 4, vboAlloc))
    {
        delete m_pInstanceData;
        m_pInstanceData = nullptr;
        return false;
    }

    m_index = index;

	// Initially our number of active fish will be the same as our capacity
	m_instancesActive = numFish;
	m_instancesCapacity = maxFish;

	// Set up data defining which model to instance from
    SetModel(vk, pModel, tailStartZ, modelTransform, extents);

	VkWriteDescriptorSet writeDescriptorSet[3];
	writeDescriptorSet[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet[0].dstSet = desc;
	writeDescriptorSet[0].dstBinding = 0;
	writeDescriptorSet[0].dstArrayElement = 0;
	writeDescriptorSet[0].descriptorCount = 1;
	writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet[1].dstSet = desc;
	writeDescriptorSet[1].dstBinding = 1;
	writeDescriptorSet[1].dstArrayElement = 0;
	writeDescriptorSet[1].descriptorCount = 1;
	writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescriptorSet[2] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet[2].dstSet = desc;
	writeDescriptorSet[2].dstBinding = 2;
	writeDescriptorSet[2].dstArrayElement = 0;
	writeDescriptorSet[2].descriptorCount = 1;
	writeDescriptorSet[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

	VkDescriptorImageInfo texDesc = {};
	texDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texDesc.imageView = pModel->GetTexture(0)->view;
	texDesc.sampler = pModel->GetSampler();
	writeDescriptorSet[0].pImageInfo = &texDesc;

	VkDescriptorBufferInfo modelDesc;
	getModelUBO().GetDesc(modelDesc);
	writeDescriptorSet[1].pBufferInfo = &modelDesc;

	VkDescriptorBufferInfo meshDesc;
	getMeshUBO().GetDesc(meshDesc);
	writeDescriptorSet[2].pBufferInfo = &meshDesc;

	vkUpdateDescriptorSets(vk.device(), 3, writeDescriptorSet, 0, 0);

	m_descSet = desc;
	m_layout = layout;

	// Seed our random number generator for each different model of fish and
	// school
	m_rndState =
		(uint32_t)(((uint64_t)m_pInstancedModel >> 4) * ((uint64_t)this >> 4));

	// Set our initial position to our initial goal.  This allows us 
	// to choose a starting position but also generate a new goal position
	// on the first update.
    m_schoolGoal = ScaledRandomVector(m_flockParams.m_spawnZoneMax 
									  - m_flockParams.m_spawnZoneMin) 
									  + m_flockParams.m_spawnZoneMin;
	/*LOGI("School Initialize: Position=(%f, %f, %f); Goal=(%f, %f, %f)",
		position.x, position.y, position.z,
		m_schoolGoal.x, m_schoolGoal.y, m_schoolGoal.z);*/

	// Initialize the fish
	m_fish.resize(m_instancesCapacity);
	nv::vec3f centroid(0.0f, 0.0f, 0.0f);

	for (uint32_t fishIndex = 0; fishIndex < m_instancesActive; ++fishIndex)
	{
		FishState& f = m_fish[fishIndex];

		f.m_position = ScaledRandomVector(m_flockParams.m_spawnRange);
		f.m_position += position;
		if (f.m_position.y < m_fishHalfExtents.y)
		{
			f.m_position.y = m_fishHalfExtents.y;
		}
		f.m_heading = nv::vec3f(0.0f, 0.0f, 1.0f);
		f.m_speed = 0;
        f.m_animTime = 0.0f;
        f.m_animStartOffset = Random01() * NV_PI * 2.0f;
		centroid += f.m_position;
	}
	if (m_instancesActive > 0)
	{
		m_lastCentroid = centroid / m_instancesActive;
	}
	else
	{
		m_lastCentroid = centroid;
	}
	return true;
}

bool School::InitializeGL(Nv::NvModelExtGL* pModel,
	GLint schoolUniformLocation,
	GLint meshUniformLocation)
{
	// Create our instance data that we will be updating each frame
	m_pInstanceDataGL = new Nv::NvSharedVBOGL();
	if ((nullptr == m_pInstanceDataGL) ||
		!m_pInstanceDataGL->Initialize(sizeof(FishInstanceData) * m_instancesCapacity, 4))
	{
		delete m_pInstanceDataGL;
		m_pInstanceDataGL = nullptr;
		return false;
	}

	glGenBuffers(1, &m_schoolUBO_Id);
	glBindBuffer(GL_UNIFORM_BUFFER, m_schoolUBO_Id);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(SchoolUBO), NULL, GL_STATIC_DRAW);
	m_schoolUBO_Location = schoolUniformLocation;

	glGenBuffers(1, &m_meshUBO_Id);
	glBindBuffer(GL_UNIFORM_BUFFER, m_meshUBO_Id);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(MeshUBO), NULL, GL_STATIC_DRAW);
	m_meshUBO_Location = meshUniformLocation;

	// Set up data defining which model to instance from
	SetModelGL(pModel);

	return true;
}


void School::ResetToLocation(const nv::vec3f& loc)
{
	for (uint32_t fishIndex = 0; fishIndex < m_instancesActive; ++fishIndex)
	{
		FishState& f = m_fish[fishIndex];

		f.m_position = ScaledRandomVector(m_flockParams.m_spawnRange);
		f.m_position += loc;
		f.m_heading = ScaledRandomVector(1.0f);
		f.m_speed = 0;
	}
	m_lastCentroid = loc;
	m_schoolGoal = loc;
}

void School::SetInstanceCount(uint32_t instances)
{
	m_instancesActive = instances;
	if (nullptr != m_pInstancedModelGL)
	{
		m_pInstancedModelGL->SetInstanceCount(instances);
	}
	if (nullptr != m_pInstancedModel)
	{
		m_pInstancedModel->SetInstanceCount(instances);
	}
}

bool School::SetModel(NvVkContext& vk, 
					  Nv::NvModelExtVK* pModel, 
					  float tailStartZ, 
					  const nv::matrix4f& modelTransform, 
					  const nv::vec3f& extents)
{
	if (nullptr != m_pInstancedModel)
	{
		delete m_pInstancedModel;
	}

	// Create the instanced version of our mesh, using it to tie together our
	// shared model and our instance data
	m_pInstancedModel = Nv::NvInstancedModelExtVK::Create(&vk,
		m_instancesCapacity, pModel);
	if (nullptr == m_pInstancedModel)
	{
		return false;
	}
	if (m_pInstancedModel->EnableInstancing(sizeof(FishInstanceData), m_pInstanceData)) {
		m_pInstancedModel->AddInstanceAttrib(7, VK_FORMAT_R32G32B32_SFLOAT, 0);
		m_pInstancedModel->AddInstanceAttrib(8, VK_FORMAT_R32G32B32_SFLOAT, 12);
		m_pInstancedModel->AddInstanceAttrib(9, VK_FORMAT_R32_SFLOAT, 24);
	}
    m_modelTransform = modelTransform;

    // Store off our half extents to use in our update to detect ground collision.
    m_fishHalfExtents = extents;

	m_tailStartZ = tailStartZ;

	// Update the school's UBO
	m_schoolUBO->m_modelMatrix = m_modelTransform;
	m_schoolUBO->m_tailStart = m_tailStartZ;
	m_schoolUBO.Update();
	
	*m_meshUBO = getModel()->GetModel()->GetMesh(0)->GetMeshOffset();
	m_meshUBO.Update();
	
	return true;
}

bool School::SetModelGL(Nv::NvModelExtGL* pModel)
{
	if (nullptr != m_pInstancedModelGL)
	{
		delete m_pInstancedModelGL;
	}

	// Create the instanced version of our mesh, using it to tie together our
	// shared model and our instance data
	m_pInstancedModelGL = Nv::NvInstancedModelExtGL::Create(m_instancesCapacity, pModel);
	if (nullptr == m_pInstancedModelGL)
	{
		return false;
	}
	if (m_pInstancedModelGL->EnableInstancing(sizeof(FishInstanceData), m_pInstanceDataGL))
	{
		// Instancing Input (as defined in staticfis_VS.glsl)
		//layout(location = 7) in vec3 a_vInstancePos;
		//layout(location = 8) in vec3 a_vInstanceHeading;
		//layout(location = 9) in float a_fInstanceAnimTime;

		m_pInstancedModelGL->AddInstanceAttrib(7, 3, GL_FLOAT, GL_FALSE, sizeof(FishInstanceData), 1, 0);
		m_pInstancedModelGL->AddInstanceAttrib(8, 3, GL_FLOAT, GL_FALSE, sizeof(FishInstanceData), 1, 12);
		m_pInstancedModelGL->AddInstanceAttrib(9, 1, GL_FLOAT, GL_FALSE, sizeof(FishInstanceData), 1, 24);
	}

	// Update the school's UBO
	m_schoolUBO_Data.m_modelMatrix = m_modelTransform;
	m_schoolUBO_Data.m_tailStart = m_tailStartZ;
	glBindBuffer(GL_UNIFORM_BUFFER, m_schoolUBO_Id);
	// Orphan our previous School Uniform buffer
	glBufferData(GL_UNIFORM_BUFFER, sizeof(SchoolUBO), nullptr, GL_STATIC_DRAW);
	// Allocate a new School Uniform buffer, initializing it with the data for the new model
	glBufferData(GL_UNIFORM_BUFFER, sizeof(SchoolUBO), &m_schoolUBO_Data, GL_STATIC_DRAW);

	m_pInstancedModelGL->GetModel()->UpdateBoneTransforms();

	const nv::matrix4f& offset = m_pInstancedModelGL->GetModel()->GetMesh(0)->GetMeshOffset();
	m_meshUBO_Data.m_offsetMatrix = offset;
	glBindBuffer(GL_UNIFORM_BUFFER, m_meshUBO_Id);
	// Orphan our previous Model Uniform buffer
	glBufferData(GL_UNIFORM_BUFFER, sizeof(MeshUBO), nullptr, GL_STATIC_DRAW);
	// Allocate a new Model Uniform buffer, initializing it with the data for the new model
	glBufferData(GL_UNIFORM_BUFFER, sizeof(MeshUBO), &m_meshUBO_Data, GL_STATIC_DRAW);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return true;
}


uint32_t neighborOffset = 0;
uint32_t neighborSkip = 1;

void School::Animate(float frameTime, SchoolStateManager* pStateManager, bool avoid)
{
	// We need to calculate a new centroid
	nv::vec3f newCentroid = nv::vec3f(0.0f, 0.0f, 0.0f);
	// ...and approximate radius
	float newRadius2 = 0.0f;

	float frameInertia = m_flockParams.m_inertia;
//#define ORIGINAL_FRAMEINERTIA
#ifdef ORIGINAL_FRAMEINERTIA
    const float targetFrameTime = 1.0f / 100.0f;
    if (frameTime < targetFrameTime) 
    {
		frameInertia *= sqrt(targetFrameTime / frameTime);
	}
#else
    const float targetFrameTime = 1.0f / 50.0f;
    if (frameTime < 0.00001f)
    {
        frameTime = 0.00001f;
    }
    frameInertia *= (targetFrameTime / frameTime);
#endif
	// Check to see if the school is close enough to its goal to start moving
	// to a new one
	float dist2goalSqr = nv::square_norm(m_schoolGoal - m_lastCentroid);
	if (dist2goalSqr <
		(m_flockParams.m_arrivalDistance * m_flockParams.m_arrivalDistance))
	{
        // Fish have arrived, choose new goal
        FindNewGoal();
	}

	float minAvoidanceSpeed = m_flockParams.m_maxSpeed * 0.5f;
	float maxAvoidanceSpeed = m_flockParams.m_maxSpeed * 2.0f;

	// For speed, use a squared distance when checking for neighbor status
	float neighborDistance2 =
		m_flockParams.m_neighborDistance * m_flockParams.m_neighborDistance;

	uint32_t numSchools = pStateManager->GetNumReadStates();
	SchoolState* pSchools = pStateManager->GetReadStates();
	// We will avoid, at most, 8 other schools
	const uint32_t cMaxSchoolsToAvoid = 8;
	uint32_t schoolsToAvoid[cMaxSchoolsToAvoid];
	uint32_t numSchoolsToAvoid = 0;
	SchoolState* pSchool = pSchools;

	if (avoid) {
		// Find schools that are at least as aggressive as our school, and overlap our school
		for (uint32_t schoolIndex = 0; (schoolIndex < numSchools) && (numSchoolsToAvoid < cMaxSchoolsToAvoid); ++schoolIndex, ++pSchool)
		{
			if (schoolIndex == m_index)
			{
				// Don't avoid ourself
				continue;
			}

			// Should we avoid the school?
			if (pSchool->m_aggression < m_flockParams.m_aggression)
			{
				// No.
				continue;
			}

			nv::vec3f toSchool = pSchool->m_center - m_lastCentroid;
			float schoolDist2 = nv::square_norm(toSchool);
			float sumRadii = pSchool->m_radius + m_lastRadius;  // Add some buffer distance of avoidance?

			if (schoolDist2 < (sumRadii * sumRadii))
			{
				schoolsToAvoid[numSchoolsToAvoid] = schoolIndex;
				++numSchoolsToAvoid;
			}
		}
	}

	int32_t clampedNeighborOffset = neighborOffset % m_instancesActive;

	for (uint32_t fishIndex = 0; fishIndex < m_instancesActive; ++fishIndex)
	{
		FishState& fish = m_fish[fishIndex];
		nv::vec3f goalHeading = nv::normalize(m_schoolGoal - fish.m_position);
		nv::vec3f alignmentSum(0.0f, 0.0f, 0.0f);
		uint32_t alignmentCount = 0;
		nv::vec3f repulsionSum(0.0f, 0.0f, 0.0f);
		uint32_t repulsionCount = 0;
		uint32_t accelerateCount = 0;
		uint32_t decelerateCount = 0;
		nv::vec3f cohesionSum(0.0f, 0.0f, 0.0f);
		uint32_t cohesionCount = 0;

		uint32_t neighborIndex = clampedNeighborOffset;
		uint32_t skip = 6 - neighborSkip;
		for (; neighborIndex < m_instancesActive; neighborIndex += skip)
		{
			if (neighborIndex == fishIndex) continue;
			FishState& neighbor = m_fish[neighborIndex];
			nv::vec3f deltaPos = neighbor.m_position - fish.m_position;
			float dist2 = nv::dot(deltaPos, deltaPos);
			if (dist2 <= neighborDistance2)
			{
				// It's too close, so we need to avoid
				float dist = sqrt(dist2);

				// If we're so close that we can't get a good normalized direction, then speed up or slow
				// down.  Just to make sure they don't choose to do the same thing, we'll use the
				// order of their indices to choose which to do.
				if (dist < 0.001f)
				{
					// The first one will accelerate
					if (fishIndex > neighborIndex)
					{
						++decelerateCount;
					}
					else
					{
						++accelerateCount;
					}
				}
				else
				{
					// If our neighbor is to the side of use, we'll 
					// use repulsion to move away.  If he's in front of
					// or behind us, we'll decelerate or accelerate to
					// move away, respectively.
					nv::vec3f deltaNorm = deltaPos / dist;
					float dotPosition = nv::dot(fish.m_heading, deltaNorm);
					if (dotPosition > 0.95f)
					{
						// Neighbor is forward
						++decelerateCount;
					}
					else if (dotPosition < -0.95f)
					{
						// Neighbor is behind
						++accelerateCount;
					}
					else
					{
						// Neighbor is to side
						repulsionSum -= deltaNorm;
						++repulsionCount;
					}
				}
			}
			else if (dist2 > (neighborDistance2 * 2.0f))
			{
				// It's far enough away that we should head towards him
				cohesionSum += deltaPos;
				++cohesionCount;
			}
			else
			{
				// It's far enough to not need to avoid but close enough to not need to swim
				// towards, so just try to go the same direction
				alignmentSum += neighbor.m_heading;
				++alignmentCount;
			}
		}
		nv::vec3f avoidanceVec(0.0f, 0.0f, 0.0f);
		if (avoid) {
			if (numSchoolsToAvoid > 0)
			{
				for (uint32_t avoidSchoolIndex = 0; avoidSchoolIndex < numSchoolsToAvoid; ++avoidSchoolIndex)
				{
					// Get a pointer to the school state for the school we're avoiding
					pSchool = pSchools + schoolsToAvoid[avoidSchoolIndex];

					nv::vec3f fromSchool = fish.m_position - pSchool->m_center;
					float schoolDist2 = nv::square_norm(fromSchool);
					if (schoolDist2 < 0.0001f)
					{
						// Way too close
						avoidanceVec += nv::vec3f(0.0f, 1.0f, 0.0f);
					}
					else if (schoolDist2 <= (pSchool->m_radius * pSchool->m_radius))
					{
						avoidanceVec += nv::normalize(fromSchool / sqrt(schoolDist2)) * (pSchool->m_aggression - m_flockParams.m_aggression + 0.1f);
					}
				}
				avoidanceVec /= numSchoolsToAvoid;
			}
		}
#define USE_MAX_DIST_AS_RADIUS 0
#if USE_MAX_DIST_AS_RADIUS
		// Use the distance between our last position and our last centroid, squared, 
		// as the new radius squared if it's greater than the currently calculated one
		float distFromCentroid2 = nv::square_norm(fish.m_position - m_lastCentroid);
		if (distFromCentroid2 > newRadius2)
		{
			newRadius2 = distFromCentroid2;
		}
#else
		// Add the distance between our last position and our last centroid, squared, to the
		// calculation for the new radius squared
		float distFromCentroid2 = nv::square_norm(fish.m_position - m_lastCentroid);
		newRadius2 += distFromCentroid2;
#endif
		// Update our position with our current heading and speed before
		// calculating new values
		fish.m_position += fish.m_speed * frameTime * fish.m_heading;
		if (fish.m_position.y < m_fishHalfExtents.y)
		{
			fish.m_position.y = m_fishHalfExtents.y;
		}

		// Combine all of our "forces" into our new driving vector
		nv::vec3f desiredHeading = (goalHeading * m_flockParams.m_goalScale);
		// Repulsion overrides everything
		if (repulsionCount > 0)
		{
			desiredHeading += repulsionSum * m_flockParams.m_repulsionScale;
		}
		else
		{
			// Alignment and cohesion can work together
			desiredHeading += nv::normalize(alignmentSum) * m_flockParams.m_alignmentScale;
			desiredHeading += nv::normalize(cohesionSum) * m_flockParams.m_cohesionScale;
		}

		if (avoid) {
			// Always try to avoid other schools, if necessary
			desiredHeading += avoidanceVec * m_flockParams.m_schoolAvoidanceScale;
		}

		// Modify our current heading by the new influences
		if (frameInertia <= 0.0f)
		{
			fish.m_heading = nv::normalize(desiredHeading);
		}
		else
		{
			fish.m_heading = nv::normalize(fish.m_heading + (desiredHeading / frameInertia));
		}

		// Headings too close to the vertical axis will cause rotational instability
		// in the shader, and possibly collapse of the reconstructed transform.
		// check to see if we're too close to the vertical, and if so, then try to
		// move in a reasonable direction away from it
		float vertical = nv::dot(fish.m_heading, nv::vec3f(0.0f, 1.0f, 0.0f));
		if (vertical > 0.999f || vertical < -0.999f)
		{
			// Try biasing our heading toward the goal.  
			fish.m_heading = nv::normalize(fish.m_heading + (goalHeading * 0.2f));

			// Test the new vector
			vertical = nv::dot(fish.m_heading, nv::vec3f(0.0f, 1.0f, 0.0f));
			if (vertical > 0.999f || vertical < -0.999f)
			{
				// also head away from the centroid.

				fish.m_heading = nv::normalize(fish.m_heading +
					(nv::normalize(fish.m_position - m_lastCentroid) * 0.2f));

				// Test the new vector
				vertical = nv::dot(fish.m_heading, nv::vec3f(0.0f, 1.0f, 0.0f));
				if (vertical > 0.999f || vertical < -0.999f)
				{
					// Still not good? Bias it by a horizontal axis.  It won't look good, but
					// it's better than nothing.
					fish.m_heading = nv::normalize(
						fish.m_heading + nv::vec3f(0.0f, 0.0f, -0.4f));
				}
			}
		}

		// Accelerate if necessary
		// Assume that acceleration == speed/second
		// (i.e. fish can go from 0 to max speed in 1 second)
		if (decelerateCount > accelerateCount)
		{
			// Decelerate to avoid fish in front
			fish.m_speed -= m_flockParams.m_maxSpeed * frameTime;
			if (fish.m_speed < minAvoidanceSpeed)
			{
				fish.m_speed = minAvoidanceSpeed;
			}
		}
		else if (accelerateCount > 0)
		{
			// Assume that speed == acceleration 
			// (i.e. fish can go from 0 to max speed in 1 second)
			fish.m_speed += m_flockParams.m_maxSpeed * frameTime;
			if (fish.m_speed > maxAvoidanceSpeed)
			{
				fish.m_speed = maxAvoidanceSpeed;
			}
		}
		else if (fish.m_speed < m_flockParams.m_maxSpeed)
		{
			// Accelerate to maximum cruising speed
			fish.m_speed += m_flockParams.m_maxSpeed * frameTime;
			if (fish.m_speed > m_flockParams.m_maxSpeed)
			{
				fish.m_speed = m_flockParams.m_maxSpeed;
			}
		}
		else if (fish.m_speed > m_flockParams.m_maxSpeed)
		{
			// Decelerate to maximum cruising speed
			fish.m_speed -= m_flockParams.m_maxSpeed * frameTime;
			if (fish.m_speed < m_flockParams.m_maxSpeed)
			{
				fish.m_speed = m_flockParams.m_maxSpeed;
			}
		}

		// Increase our animation time, using our current speed to make the 
		// tail move at a reasonable rate
		fish.m_animTime += frameTime * fish.m_speed * fish.m_speed;

		// Add our new position to the calculation of the school centroid for
		// this frame
		newCentroid += fish.m_position;
	}
	// Update our centroid based on the school's fish positions
	m_lastCentroid = newCentroid / m_instancesActive;
#if USE_MAX_DIST_AS_RADIUS
	m_lastRadius = sqrt(newRadius2);
#else
	// Give a bit of a buffer (20%) to the average radius to account for most 
	// of the school, but still ignore the outliers
	m_lastRadius = sqrt(newRadius2 / m_instancesActive) * 1.2f;
#endif

	if (avoid) {
		// Write our current state to the SchoolStateManager
		NV_ASSERT(m_index < pStateManager->GetNumWriteStates());

		SchoolState* pOurState = pStateManager->GetWriteStates() + m_index;
		pOurState->m_aggression = m_flockParams.m_aggression;
		pOurState->m_center = m_lastCentroid;
		pOurState->m_radius = m_lastRadius;
	}
}

void School::Update(bool useGL)
{
	UpdateInstanceDataBuffer(useGL);
	if (!useGL) {
		// Push our new state data into the instance data buffer
		void* pInstanceData = m_pInstanceData->GetData();

		// Update per-mesh changes
		m_pInstancedModel->GetModel()->UpdateBoneTransforms();
		*m_meshUBO = m_pInstancedModel->GetModel()->GetMesh(0)->GetMeshOffset();
		m_meshUBO.Update();
	}
}

void School::UpdateInstanceDataBuffer(bool useGL)
{
	FishInstanceData* pCurrInstance = NULL;

	if (useGL) {
		if (!m_pInstanceDataGL->BeginUpdate()) {
			return;
		}

		pCurrInstance =
			(FishInstanceData*)m_pInstanceDataGL->GetData();
	}
	else {
		if (!m_pInstanceData->BeginUpdate()) {
			return;
		}

		pCurrInstance =
			(FishInstanceData*)m_pInstanceData->GetData();
	}

	if (nullptr == pCurrInstance) {
		return;
	}

	uint32_t fishIndex = 0;
	for (; fishIndex < m_instancesActive; ++fishIndex, ++pCurrInstance)
	{
		FishState& f = m_fish[fishIndex];
		pCurrInstance->m_position = f.m_position;
		pCurrInstance->m_heading = f.m_heading;
		pCurrInstance->m_tailTime = f.m_animStartOffset + f.m_animTime;
	}

	if (useGL) {
		m_pInstanceDataGL->EndUpdate();
	}
	else {
		m_pInstanceData->EndUpdate();
	}
}

uint32_t School::Render(VkCommandBuffer& cmd, uint32_t batchSize)
{
	uint32_t drawCallCount = 0;

	if (nullptr == m_pInstancedModel)
		return drawCallCount;

	m_pInstancedModel->BindInstanceData(cmd);

	Nv::NvModelExtVK* model = m_pInstancedModel->GetModel();

	uint32_t meshCount = model->GetMeshCount();

	uint32_t offsets[2];
	offsets[0] = getModelUBO().getDynamicOffset();
	offsets[1] = getMeshUBO().getDynamicOffset();
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 1, 1, &m_descSet, 2, offsets);

	for (uint32_t i = 0; i < meshCount; i++) {
		Nv::NvMeshExtVK* mesh = model->GetMesh(i);
		if (batchSize == 0 || batchSize > m_instancesActive)
			batchSize = m_instancesActive;

		uint32_t j = 0;
		for (; (j + batchSize) <= m_instancesActive; j += batchSize) {
			mesh->Draw(cmd, batchSize, j);
			drawCallCount++;
		}

		int32_t instRemaining = m_instancesActive - j;

		if (instRemaining > 0) {
			mesh->Draw(cmd, instRemaining, j);
			drawCallCount++;
		}
	}

	return drawCallCount;
}

uint32_t School::RenderGL(uint32_t batchSize)
{
	uint32_t drawCallCount = 0;

	if (nullptr == m_pInstancedModelGL)
	{
		return drawCallCount;
	}

	// Activate our Uniform buffers
	glBindBufferBase(GL_UNIFORM_BUFFER, m_schoolUBO_Location, m_schoolUBO_Id);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_meshUBO_Location, m_meshUBO_Id);

	m_pInstancedModelGL->SetBatchSize(batchSize);
	drawCallCount += m_pInstancedModelGL->Render(0, 1, 2);
	//for (uint32_t i = 0; i < meshCount; i++) {
	//	Nv::NvMeshExtVK* mesh = model->GetMesh(i);
	//	if (batchSize == 0 || batchSize > m_instancesActive)
	//		batchSize = m_instancesActive;

	//	uint32_t j = 0;
	//	for (; (j + batchSize) <= m_instancesActive; j += batchSize) {
	//		mesh->Draw(cmd, batchSize, j);
	//		drawCallCount++;
	//	}

	//	int32_t instRemaining = m_instancesActive - j;

	//	if (instRemaining > 0) {
	//		mesh->Draw(cmd, instRemaining, j);
	//		drawCallCount++;
	//	}
	//}

	return drawCallCount;
}

void School::FindNewGoal()
{
    m_schoolGoal = ScaledRandomVector(m_flockParams.m_spawnZoneMax
        - m_flockParams.m_spawnZoneMin)
        + m_flockParams.m_spawnZoneMin;
    /*LOGI("School Goal: (%f, %f, %f)",
    m_schoolGoal.x, m_schoolGoal.y, m_schoolGoal.z);*/
}
