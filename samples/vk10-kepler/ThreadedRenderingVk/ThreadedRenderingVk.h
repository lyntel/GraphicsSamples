//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/ThreadedRenderingVk.h
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
#ifndef ThreadedRenderingVkVK_H_
#define ThreadedRenderingVkVK_H_
#include "NvVkUtil/NvSampleAppVK.h"

#include "NV/NvMath.h"
#include "NvAppBase/NvCPUTimer.h"
#include "NvUI/NvBitFont.h"

#include "NvGamepad/NvGamepad.h"
#include "NvVkUtil/NvSimpleUBO.h"
#include "NvVkUtil/NvSafeCommandBuffer.h"
#include <vector>
#include "NvInstancedModelExtVk.h"
#include "School.h"
#include <cstdlib>
#include <deque>
#include "SchoolStateManager.h"

#include "NvGLUtils/NvTimers.h"
#include "NvInstancedModelExtGL.h"

#define CPU_TIMER_SCOPE(TIMER_ID) NvCPUTimerScope cpuTimer(&m_CPUTimers[TIMER_ID])
#define GPU_TIMER_SCOPE() NvGPUTimerScope gpuTimer(&m_GPUTimer)

class NvInputHandler_CameraFly;
class NvQuadVK;
class NvGLSLProgram;

class ThreadedRenderingVk : public NvSampleAppVK, public NvGLDrawCallbacks
{
public:
	ThreadedRenderingVk();
	virtual ~ThreadedRenderingVk();

	// Inherited methods
	virtual void initRendering(void);
	virtual void initUI(void);
	virtual void reshape(int32_t width, int32_t height);
	virtual void draw(void);

	enum {
		MAX_ANIMATION_THREAD_COUNT = 8,
		THREAD_STACK_SIZE = 8192U
	};

	enum
	{
		CPU_TIMER_MAIN_CMD_BUILD = 0,
		CPU_TIMER_MAIN_WAIT,
        CPU_TIMER_MAIN_COPYVBO,
		CPU_TIMER_THREAD_BASE_CMD_BUILD,
		CPU_TIMER_THREAD_MAX_CMD_BUILD = CPU_TIMER_THREAD_BASE_CMD_BUILD + MAX_ANIMATION_THREAD_COUNT,
		CPU_TIMER_THREAD_BASE_ANIMATE,
		CPU_TIMER_THREAD_MAX_ANIMATE = CPU_TIMER_THREAD_BASE_ANIMATE + MAX_ANIMATION_THREAD_COUNT,
		CPU_TIMER_THREAD_BASE_UPDATE,
		CPU_TIMER_THREAD_MAX_UPDATE = CPU_TIMER_THREAD_BASE_UPDATE + MAX_ANIMATION_THREAD_COUNT,
		CPU_TIMER_THREAD_BASE_TOTAL,
		CPU_TIMER_THREAD_MAX_TOTAL = CPU_TIMER_THREAD_BASE_TOTAL + MAX_ANIMATION_THREAD_COUNT,
		CPU_TIMER_COUNT
	};


	enum
	{
		RENDER_COLOR = 0,
		RENDER_DEPTH,
		RENDER_FINAL,
		RENDER_COUNT
	};

	enum
	{
		RESET_RANDOM = 0,   // Schools will spawn at random locations
		RESET_ORIGIN,       // Schools will spawn at the center of the tank
		RESET_COUNT
	};

    enum StatsMode
    {
        STATS_NONE = 0,
        STATS_SIMPLE,
        STATS_FULL,
        STATS_COUNT
    };

	struct ProjUBO
	{
		// Pipeline matrices
		nv::matrix4f m_projectionMatrix;
		nv::matrix4f m_inverseProjMatrix;
		nv::matrix4f m_viewMatrix;
		nv::matrix4f m_inverseViewMatrix;
	};

	struct ModelUBO
	{
		nv::matrix4f m_modelMatrix;
	};

	struct LightingUBO
	{
		nv::vec4f m_lightPosition;
		nv::vec4f m_lightAmbient;
        nv::vec4f m_lightDiffuse;
        float m_causticOffset;
        float m_causticTiling;
	};

	struct ThreadData {
		NvThread* m_thread;
		ThreadedRenderingVk* m_app;
		uint32_t m_index;
		bool m_cmdBufferOpen;
		uint32_t m_drawCallCount;
		uint32_t m_baseSchoolIndex;
		uint32_t m_schoolCount;
	};
	void AnimateJobFunction(uint32_t threadIndex);
	void UpdateSchool(uint32_t threadIndex, uint32_t schoolIndex,
		School* pSchool);

	virtual bool skipVKDraw() { return m_drawGL; }
	virtual void initGLResources(NvAppContextGL* gl);
	virtual void drawGL(NvAppContextGL* gl);

private:
	// Additional rendering setup methods
	void CleanRendering(void);
	bool LoadAllShadersFromSource();
	bool LoadAllShaderBinaries();
	void InitializeSamplers();
	void InitializeSchoolDescriptions(uint32_t numSchools);
	void InitializeSchools();
	void InitPipeline(uint32_t shaderCount,
		VkPipelineShaderStageCreateInfo* shaderStages,
		VkPipelineVertexInputStateCreateInfo* pVertexInputState,
		VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
		VkPipelineColorBlendStateCreateInfo* pBlendState,
		VkPipelineDepthStencilStateCreateInfo* pDSState,
		VkPipelineLayout& layout,
		VkPipeline* pipeline);

	virtual NvUIEventResponse handleReaction(const NvUIReaction &react);
	virtual bool handleGamepadButtonChanged(uint32_t button, bool down);
	void UpdateUI();
    void UpdateLogos();

	float getClampedFrameTime();
    NvUIGraphic* InitLogoTexture(const char* pTexFilename);
    void ResetFish(bool fishsplosion);

	// Methods that draw a specific scene component
	uint32_t DrawSchool(VkCommandBuffer& cmd, School* pSchool, bool openBuffer);
	void DrawSkyboxColorDepth(VkCommandBuffer& cmd);
	void DrawGroundPlane(VkCommandBuffer& cmd);

	void DrawSkyboxColorDepthGL();
	void DrawGroundPlaneGL();

	// Per frame render call that uses existing command buffers, but uses
	// updated UBOs where necessary
	void RenderScene(VkCommandBuffer& cmd);

	void RenderSceneGL();

	// Thread handling methods
	void InitThreads(void);
	void CleanThreads(void);

	// Methods to affect the current settings of the app
	uint32_t SetNumSchools(uint32_t numSchools);
    void UpdateSchoolTankSizes();
    uint32_t SetAnimationThreadNum(uint32_t numThreads);

    void UpdateStats();
   
	void BuildSimpleStatsString(char* buffer, int32_t size);
    void BuildFullStatsString(char* buffer, int32_t size);

	NVTHREAD_STACK_ALIGN char m_ThreadStacks[MAX_ANIMATION_THREAD_COUNT][THREAD_STACK_SIZE];
	bool m_bFollowingSchool;

	ThreadData m_Threads[MAX_ANIMATION_THREAD_COUNT];

	struct ThreadJob
	{
		School* school;
		uint32_t index;
	};

	NvMutex* m_FrameStartLock;
	NvConditionVariable* m_FrameStartCV;

	std::deque<ThreadJob> m_NeedsUpdateQueue;
	NvMutex* m_NeedsUpdateQueueLock;

	NvMutex* m_DoneCountLock;
	uint32_t m_doneCount;
	NvConditionVariable* m_DoneCountCV;

    uint32_t m_requestedActiveThreads;
	uint32_t m_activeThreads;
    bool m_requestedThreadedRendering;
	bool m_threadedRendering;

	bool m_running;

	NvInputHandler_CameraFly* m_pInputHandler;
	// Member fields that hold FBO and texture handles
	bool m_FBOCreated;

    // Flag to allow schools to be created with same parameters
    // except for model.  Setting this to false will use the "schoolInfo" 
    // array to initialize schools.
    bool m_bUseFixedSchools;

	// Member fields that hold shader objects
	NvGLSLProgram* m_shader_GroundPlane;
	NvGLSLProgram* m_shader_Skybox;
	NvGLSLProgram* m_shader_Fish;

	GLuint m_skyboxSandTexGL;
	GLuint m_skyboxGradientTexGL;

	GLuint m_caustic1TexGL;
	GLuint m_caustic2TexGL;

	VkSampler m_linearSampler;
	VkSampler m_nearestSampler;
	VkSampler m_wrapSampler;

	// Member fields that hold shader objects
	VkPipeline m_fishPipe;
	VkPipelineLayout m_fishPipeLayout;

	VkPipeline m_skyboxColorPipe;
	VkPipeline m_groundPlanePipe;
	VkPipelineLayout m_pipeLayout;

	VkDescriptorPool mDescriptorRegion;

	VkDescriptorSet mDescriptorSet;

	VkDescriptorSetLayout mDescriptorSetLayout[2];

	NvQuadVK* m_quad;

	enum {
		DESC_PROJ_UBO = 0,
		DESC_LIGHT_UBO = 1,
		DESC_CAUS1_TEX = 2,
		DESC_FIRST_TEX = DESC_CAUS1_TEX,
		DESC_CAUS2_TEX = 3,
		DESC_SAND_TEX = 4,
		DESC_GRAD_TEX = 5,
		DESC_COUNT = 6,
	};
	// Member fields that hold the scene geometry
	// Enum giving identifiers for all Fish models available for use
	enum
	{
		MODEL_BLACK_WHITE_FISH,
		MODEL_BLUE_FISH,
		MODEL_BLUE_FISH_02,
		MODEL_BLUE_FISH_03,
		MODEL_BLUE_FISH_04,
		MODEL_BLUE_FISH_05,
		MODEL_BLUE_FISH_06,
		MODEL_BLUE_FISH_07,
		MODEL_BLUE_FISH_08,
		MODEL_BLUE_FISH_09,
		MODEL_CYAN_FISH,
		MODEL_PINK_FISH,
		MODEL_RED_FISH,
		MODEL_VIOLET_FISH,
		MODEL_YELLOW_FISH,
		MODEL_YELLOW_FISH_02,
		MODEL_YELLOW_FISH_03,
		MODEL_YELLOW_FISH_04,
		MODEL_YELLOW_FISH_05,
		MODEL_YELLOW_FISH_06,
		MODEL_YELLOW_FISH_07,
		MODEL_YELLOW_FISH_08,
		MODEL_YELLOW_FISH_09,
		MODEL_YELLOW_FISH_10,
		MODEL_YELLOW_FISH_11,
		MODEL_COUNT
	};

	NvSimpleUBO<ProjUBO> m_projUBO;
	NvSimpleUBO<LightingUBO> m_lightingUBO;

	/// Uniform buffer object providing per-scene projection parameters to the shader
	ProjUBO m_projUBO_Data;       // Actual values for the UBO
	GLuint  m_projUBO_Id;         // UBO Id
	GLint   m_projUBO_Location;   // Uniform Index

	/// Uniform buffer object providing per-scene lighting parameters to the shader
	LightingUBO m_lightingUBO_Data;       // Actual values for the UBO
	GLuint      m_lightingUBO_Id;         // UBO Id
	GLint       m_lightingUBO_Location;   // Uniform Index

	// Mutex to protect access to the VK queue
	NvMutex* m_queueMutexPtr;

    // define the volume that the fish will remain within
    static nv::vec3f ms_tankMin;
    static nv::vec3f ms_tankMax;
	nv::vec3f m_startingCameraPosition;
	nv::vec2f m_startingCameraPitchYaw;

	nv::vec3f Rotate(const nv::matrix4f& m, const nv::vec3f& n) const
	{
		nv::vec3f r;
		r.x = n.x * m._11 + n.y * m._21 + n.z * m._31;
		r.y = n.x * m._12 + n.y * m._22 + n.z * m._32;
		r.z = n.x * m._13 + n.y * m._23 + n.z * m._33;
		return r;
	}

	// Structure describing a fish model that will
	// be loaded by the application
	struct ModelDesc
	{
		const char* m_name;         // Basic name of the model
		const char* m_filename;     // Filename of the model to load
		nv::matrix4f m_fixupXfm;    // Transform to rotate the model from its
									// authored space to the application's
									// coordinate system.
		nv::vec3f m_halfExtents;    // Vector from the center to the maximum
									// corner of a bounding box containing the
									// model in the application's coordinate
									// system.
		nv::vec3f m_center;         // Vector indicating the center of a
									// bounding box containing the model in
									// the application's coordinate system.
        float m_tailStartZ;         // Z position at which the tail starts (for vertex animation)

		nv::matrix4f GetCenteringTransform() const
		{
			nv::matrix4f m = m_fixupXfm;
			m.set_translate(-m_center);
			return m;
		}
	};
	static ModelDesc ms_modelInfo[];
	Nv::NvModelExtVK* m_models[MODEL_COUNT];
	Nv::NvModelExtGL* m_modelsGL[MODEL_COUNT];

	struct SchoolDesc
	{
		uint32_t m_modelId;
		uint32_t m_numFish;
        float    m_scale;
		SchoolFlockingParams m_flocking;
	};
	typedef std::vector<SchoolDesc> SchoolDescSet;

	uint32_t m_maxSchools;
	SchoolDescSet m_schoolDescs;
    SchoolStateManager m_schoolStateMgr;

    // Static data used to initialize schools and their flocking parameters, etc.
    enum FishTypes
    {
        FISHTYPE_EXTRALARGE = 0,
        FISHTYPE_LARGESLOW,
        FISHTYPE_LARGE,
        FISHTYPE_LARGEFAST,
        FISHTYPE_MEDIUMSLOW,
        FISHTYPE_MEDIUM,
        FISHTYPE_MEDIUMFAST,
        FISHTYPE_MEDIUMSPARSE,
        FISHTYPE_SMALLSLOW,
        FISHTYPE_SMALL,
        FISHTYPE_SMALLFAST,
        FISHTYPE_COUNT
    };
    static SchoolFlockingParams ms_fishTypeDefs[];
    static SchoolDesc ms_schoolInfo[];

	typedef std::vector<School*> SchoolSet;
	SchoolSet m_schools;
	uint32_t m_activeSchools;
	uint32_t m_initSchools;

	NvSimpleUBOAllocator m_fishAlloc;
	Nv::NvSharedVBOVKAllocator m_vboAlloc;

	NvVkTexture m_skyboxSandTex;
	NvVkTexture m_skyboxGradientTex;

	NvVkTexture m_caustic1Tex;
	NvVkTexture m_caustic2Tex;

    float m_causticTiling;
    float m_causticSpeed;

	NvSafeCommandBuffer<3, MAX_ANIMATION_THREAD_COUNT+1> m_subCommandBuffers;

	// Other state-related member fields
	// (some of them are referenced and controlled by the TweakBar UI)
	uint32_t m_renderMode;

	// Reset mode is the current setting for the Reset dropdown.
	// See the RESET_* enum for values
	uint32_t m_uiResetMode;

    NvTweakVarBase* m_pFishFireworksVar;
    NvTweakVarBase* m_pFishplosionVar;

    bool m_animPaused;
	bool m_avoidance;
    float m_currentTime;
	uint32_t m_frameLogicalClock;

	// UI trackers
	bool m_bUIDirty;

	enum UIReactionIDs
	{
		  UIACTION_SCHOOLCOUNT          = 0x10000001
		, UIACTION_FISHCOUNT         
		, UIACTION_SCHOOLINFOID       
		, UIACTION_SCHOOLMODELINDEX    
		, UIACTION_SCHOOLMAXSPEED      
		, UIACTION_SCHOOLINERTIA       
		, UIACTION_SCHOOLNEIGHBORDIST  
		, UIACTION_SCHOOLGOAL          
		, UIACTION_SCHOOLALIGNMENT      
		, UIACTION_SCHOOLREPULSION    
		, UIACTION_SCHOOLCOHESION    
		, UIACTION_SCHOOLAVOIDANCE      
		, UIACTION_RESET               
		, UIACTION_CAMERAFOLLOWSCHOOL   
		, UIACTION_RESET_FISHPLOSION	
		, UIACTION_RESET_FISHFIREWORKS	
		, UIACTION_ANIMTHREADCOUNT		
		, UIACTION_INSTCOUNT		
		, UIACTION_BATCHSIZE
		, UIACTION_STATSTOGGLE
		, UIACTION_DRAWGL
        , UIACTION_TANKSIZE
    };

	uint32_t m_uiSchoolCount;
    uint32_t m_uiFishCount;
    uint32_t m_uiTankSize;
    bool     m_bTankSizeChanged;
	uint32_t m_uiThreadCount;
	uint32_t m_uiInstanceCount;
	uint32_t m_uiBatchSize;
	NvUISlider* m_pBatchSlider;
	NvTweakVar<uint32_t>* m_pBatchVar;
	NvUIButton* m_pThreadedRenderingButton;

	// Details UI
	uint32_t m_uiSchoolInfoId;
    bool m_uiCameraFollow;
	uint32_t m_uiSchoolDisplayModelIndex;
	float m_uiSchoolMaxSpeed;
	float m_uiSchoolInertia;
	float m_uiSchoolNeighborDistance;
	float m_uiSchoolGoalScale;
	float m_uiSchoolAlignmentScale;
	float m_uiSchoolRepulsionScale;
    float m_uiSchoolCohesionScale;
    float m_uiSchoolAvoidanceScale;

	NvUIText* m_simpleTimingStats;
	NvUIContainer* m_simpleStatsBox;

    NvUIText* m_fullTimingStats;
    NvUIContainer* m_fullStatsBox;

    bool m_bDisplayLogos;
    NvUIGraphic* m_logoNVIDIA;
    NvUIGraphic* m_logoVulkan;
    NvUIGraphic* m_logoGLES;


	uint32_t m_drawCallCount;

	enum { STATS_FRAMES = 5 };
	NvCPUTimer m_CPUTimers[CPU_TIMER_COUNT];
	NvGPUTimer m_GPUTimer;
	int32_t m_statsCountdown;

    uint32_t m_statsMode;
    NvTweakVarBase* m_pStatsModeVar;

    // Current Stats
    float m_meanFPS;
    float m_meanCPUMainCmd;
    float m_meanCPUMainWait;
    float m_meanCPUMainCopyVBO;
    float m_meanGPUFrameMS = 0.0f;

    struct ThreadTimings {
        float anim;
        float update;
        float cmd;
        float tot;
    };

    ThreadTimings m_threadTimings[MAX_ANIMATION_THREAD_COUNT];

	bool m_flushPerFrame;

	bool m_glSupported;
    bool m_requestedDrawGL;
	bool m_drawGL;
	bool m_glInitialized;
};
#endif // ThreadedRenderingVk_H_
