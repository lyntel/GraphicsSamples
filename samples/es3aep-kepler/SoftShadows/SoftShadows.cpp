//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/SoftShadows.cpp
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
#include "SoftShadowsCommon.h"
#include "SoftShadows.h"

#include "KnightModel.h"
#include "PodiumModel.h"

#include <NvAppBase/NvFramerateCounter.h>
#include <NV/NvStopWatch.h>
#include <NvAssetLoader/NvAssetLoader.h>
#include <NvUI/NvTweakBar.h>

#include <algorithm>
#include <ctype.h>
#include <sstream>

////////////////////////////////////////////////////////////////////////////////
// UI Reaction IDs
////////////////////////////////////////////////////////////////////////////////
enum UIReactionIDs
{   
    REACT_USE_TEXTURE         = 0x10000001,
    REACT_VISUALIZE_DEPTH     = 0x10000002,
    REACT_LIGHT_SIZE          = 0x10000003,
    REACT_SHADOW_TECHNIQUE    = 0x10000004,
    REACT_PCSS_SAMPLE_PATTERN = 0x10000005,
    REACT_PCF_SAMPLE_PATTERN  = 0x10000006    
};

////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////
static const float GROUND_PLANE_RADIUS = 8.0f;

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::SoftShadows()
////////////////////////////////////////////////////////////////////////////////
SoftShadows::SoftShadows()
    : m_renderer(*m_transformer)
    , m_knightMesh()
    , m_podiumMesh()
    , m_triStats(0)
    , m_lightStats(0)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();

    // Parse command line for default overrides
    const std::vector<std::string>& cmd = NvAppBase::getCommandLine();
    for (std::vector<std::string>::const_iterator it = cmd.begin(); it != cmd.end(); ++it)
    {
        std::string key(*it);
        std::transform(key.begin(), key.end(), key.begin(), tolower);
        if (0 == key.compare("-technique")) {
            ++it;
            std::string val(*it);
            std::transform(val.begin(), val.end(), val.begin(), tolower);
            if (0 == val.compare("none")) {
                m_renderer.setShadowTechnique(SoftShadowsRenderer::None);
            } else if (0 == val.compare("pcss")) {
                m_renderer.setShadowTechnique(SoftShadowsRenderer::PCSS);
            } else if (0 == val.compare("pcf")) {
                m_renderer.setShadowTechnique(SoftShadowsRenderer::PCF);
            }
        } else if (0 == key.compare("-preset")) {
            ++it;
            std::string val(*it);
            std::transform(val.begin(), val.end(), val.begin(), tolower);
            if (0 == val.compare("poisson_25_25")) {
                m_renderer.setPcssSamplePattern(SoftShadowsRenderer::Poisson_25_25);
                m_renderer.setPcfSamplePattern(SoftShadowsRenderer::Poisson_25_25);                
            } else if (0 == val.compare("poisson_32_64")) {
                m_renderer.setPcssSamplePattern(SoftShadowsRenderer::Poisson_32_64);
                m_renderer.setPcfSamplePattern(SoftShadowsRenderer::Poisson_32_64);
            } else if (0 == val.compare("poisson_100_100")) {
                m_renderer.setPcssSamplePattern(SoftShadowsRenderer::Poisson_100_100);
                m_renderer.setPcfSamplePattern(SoftShadowsRenderer::Poisson_100_100);
            } else if (0 == val.compare("poisson_64_128")) {
                m_renderer.setPcssSamplePattern(SoftShadowsRenderer::Poisson_64_128);
                m_renderer.setPcfSamplePattern(SoftShadowsRenderer::Poisson_64_128);
            } else if (0 == val.compare("regular_49_225")) {
                m_renderer.setPcssSamplePattern(SoftShadowsRenderer::Regular_49_225);
                m_renderer.setPcfSamplePattern(SoftShadowsRenderer::Regular_49_225);
            }
        }
    }

    // Initialize tweakable values
    m_useTexture = m_renderer.useTexture();
    m_visualizeDepth = m_renderer.visualizeDepthTexture();
    m_lightSize = m_renderer.getLightRadiusWorld();
    m_shadowTechnique = m_renderer.getShadowTechnique();
    m_pcssSamplePattern = m_renderer.getPcssSamplePattern();
    m_pcfSamplePattern = m_renderer.getPcfSamplePattern();    
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::~SoftShadows()
////////////////////////////////////////////////////////////////////////////////
SoftShadows::~SoftShadows()
{
    LOGI("SoftShadows: destroyed\n");
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::initUI()
////////////////////////////////////////////////////////////////////////////////
void SoftShadows::initUI()
{
    if (mTweakBar)
    {
        NvTweakVarBase *var;        
        
        // Use texture
        mTweakBar->addPadding();
        var = mTweakBar->addValue("Use Texture", m_useTexture, REACT_USE_TEXTURE);
        addTweakKeyBind(var, NvKey::K_X); // use teXture

        // Visualize depth
        mTweakBar->addPadding();
        var = mTweakBar->addValue("Visualize Depth", m_visualizeDepth, REACT_VISUALIZE_DEPTH);
        addTweakKeyBind(var, NvKey::K_V); // Visualize depth    

        // Light radius
        mTweakBar->addPadding();
        var = mTweakBar->addValue("Light Size", m_lightSize, 0.0f, 1.0f, 0.01f, REACT_LIGHT_SIZE);
        addTweakKeyBind(var, NvKey::K_RBRACKET, NvKey::K_LBRACKET);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER, NvGamepad::BUTTON_LEFT_SHOULDER);

        // Shadow technique
        mTweakBar->addPadding();
        NvTweakEnum<uint32_t> shadowTechniques[] =
        {
            { "None", SoftShadowsRenderer::None },
            { "PCSS", SoftShadowsRenderer::PCSS },
            { "PCF", SoftShadowsRenderer::PCF }
        };
        var = mTweakBar->addEnum("Shadow Technique", m_shadowTechnique, shadowTechniques, TWEAKENUM_ARRAYSIZE(shadowTechniques), REACT_SHADOW_TECHNIQUE);
        addTweakKeyBind(var, NvKey::K_T); // shadow Technique - can't use S because WASD controls the camera
        addTweakButtonBind(var, NvGamepad::BUTTON_X);

        // PCSS presets
        NvTweakEnum<uint32_t> samplePatterns[] =
        {
            { "25 / 25 Poisson", SoftShadowsRenderer::Poisson_25_25 },
            { "32 / 64 Poisson", SoftShadowsRenderer::Poisson_32_64 },
            { "100 / 100 Poisson", SoftShadowsRenderer::Poisson_100_100 },
            { "64 / 128 Poisson", SoftShadowsRenderer::Poisson_64_128 },
            { "49 / 225 Regular", SoftShadowsRenderer::Regular_49_225 }
        };
        mTweakBar->subgroupSwitchStart(var);
            mTweakBar->subgroupSwitchCase(static_cast<uint32_t>(SoftShadowsRenderer::PCSS));
                var = mTweakBar->addEnum("PCSS Blocker Search / Filter", m_pcssSamplePattern, samplePatterns, TWEAKENUM_ARRAYSIZE(samplePatterns), REACT_PCSS_SAMPLE_PATTERN);
                addTweakKeyBind(var, NvKey::K_B); // Blocker search
                addTweakButtonBind(var, NvGamepad::BUTTON_Y);    
            mTweakBar->subgroupSwitchCase(static_cast<uint32_t>(SoftShadowsRenderer::PCF));
                var = mTweakBar->addEnum("PCF Blocker Search / Filter", m_pcfSamplePattern, samplePatterns, TWEAKENUM_ARRAYSIZE(samplePatterns), REACT_PCF_SAMPLE_PATTERN);
                addTweakKeyBind(var, NvKey::K_B); // Blocker search
                addTweakButtonBind(var, NvGamepad::BUTTON_Y);            
        mTweakBar->subgroupSwitchEnd();
    }
    
    // UI elements for displaying triangle statistics
    if (mFPSText) {
        NvUIRect tr;
        mFPSText->GetScreenRect(tr); // base off of fps element.
        m_triStats = new NvUIValueText("K Tris", NvUIFontFamily::SANS, mFPSText->GetFontSize(), NvUITextAlign::RIGHT,
                                        0, NvUITextAlign::RIGHT);
        m_triStats->SetColor(NV_PACKED_COLOR(0x30,0xD0,0xD0,0xB0));
        m_triStats->SetShadow();
        mUIWindow->Add(m_triStats, tr.left, tr.top+tr.height+8);
        m_triStats->GetScreenRect(tr); // base off of new element
        m_lightStats = new NvUIValueText("LightRes", NvUIFontFamily::SANS, mFPSText->GetFontSize(), NvUITextAlign::RIGHT,
                                        0, NvUITextAlign::RIGHT);
        m_lightStats->SetColor(NV_PACKED_COLOR(0x30,0xD0,0xD0,0xB0));
        m_lightStats->SetShadow();
        mUIWindow->Add(m_lightStats, tr.left, tr.top+tr.height+8);
    }

    // Change the filtering for the framerate
    mFramerate->setMaxReportRate(.2f);
    mFramerate->setReportFrames(20);

    // Disable wait for vsync
    getGLContext()->setSwapInterval(0);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::handleReaction()
////////////////////////////////////////////////////////////////////////////////
NvUIEventResponse SoftShadows::handleReaction(const NvUIReaction &react)
{
    switch (react.code)
    {
    case REACT_USE_TEXTURE:
        m_renderer.useTexture(m_useTexture);
        break;

    case REACT_VISUALIZE_DEPTH:
        m_renderer.visualizeDepthTexture(m_visualizeDepth);
        break;

    case REACT_LIGHT_SIZE:
        m_renderer.setLightRadiusWorld(m_lightSize);
        break;

    case REACT_SHADOW_TECHNIQUE:
        m_renderer.setShadowTechnique(static_cast<SoftShadowsRenderer::ShadowTechnique>(m_shadowTechnique));
        break;

    case REACT_PCSS_SAMPLE_PATTERN:
        m_renderer.setPcssSamplePattern(static_cast<SoftShadowsRenderer::SamplePattern>(m_pcssSamplePattern));
        break;

    case REACT_PCF_SAMPLE_PATTERN:
        m_renderer.setPcfSamplePattern(static_cast<SoftShadowsRenderer::SamplePattern>(m_pcfSamplePattern));
        break;

    default:
        return nvuiEventNotHandled;
    }

    return nvuiEventHandled;    
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::configurationCallback()
////////////////////////////////////////////////////////////////////////////////
void SoftShadows::configurationCallback(NvGLConfiguration& config)
{ 
    m_renderer.configurationCallback(config);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::initRendering()
////////////////////////////////////////////////////////////////////////////////
void SoftShadows::initRendering()
{
    NvAssetLoaderAddSearchPath("es3aep-kepler/SoftShadows");
    bool isGL = getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL;

    // Require GL for now.
    if(!(requireMinAPIVersion(NvGLAPIVersionES3_1(), false) &&
         requireExtension("GL_ANDROID_extension_pack_es31a", false)) &&
        !isGL)
    {
        errorExit("The system does not support the required OpenGL[ES] features\n"
            "This sample requires either OpenGL 4.4 or\n"
            "OpenGL ES 3.1 with the Android Extension Pack");
        return;
    }

    NvScopedShaderPrefix switched(
    (getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL)
    ? "#version 440\n" : 
    "#version 310 es\n#extension GL_ANDROID_extension_pack_es31a : enable\nprecision mediump float;");
    CHECK_GL_ERROR();

    // Load the meshes
    bool useES2 = getGLContext()->getConfiguration().apiVer == NvGLAPIVersionES2();
    m_knightMesh.reset(new RigidMesh(
        KnightModel::vertices,
        KnightModel::numVertices,
        KnightModel::indices,
        KnightModel::numIndices,
        useES2));
    m_podiumMesh.reset(new RigidMesh(
        PodiumModel::vertices,
        PodiumModel::numVertices,
        PodiumModel::indices,
        PodiumModel::numIndices,
        useES2));

    // Build the scene
    nv::matrix4f identity;
    m_renderer.setKnightMesh(m_renderer.addMeshInstance(m_knightMesh.get(), identity, L"Knight"));
    m_renderer.setPodiumMesh(m_renderer.addMeshInstance(m_podiumMesh.get(), identity, L"Podium"));

    // Setup the ground plane
    SoftShadowsRenderer::MeshInstance *knight = m_renderer.getKnightMesh();
    nv::vec3f extents = knight->getExtents();
    nv::vec3f center = knight->getCenter();
    float height = center.y - extents.y;
    m_renderer.setGroundPlane(height, GROUND_PLANE_RADIUS);

    // Initialize the renderer
    m_renderer.initRendering();
    CHECK_GL_ERROR();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::reshape()
////////////////////////////////////////////////////////////////////////////////
void SoftShadows::reshape(int32_t width, int32_t height)
{
    m_renderer.reshape(width, height);
    CHECK_GL_ERROR();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::draw()
////////////////////////////////////////////////////////////////////////////////
void SoftShadows::draw()
{
    float dt = getFrameDeltaTime();
    m_renderer.updateFrame(dt);
    m_renderer.render(dt);
    CHECK_GL_ERROR();
    renderText();
    CHECK_GL_ERROR();	
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::handleGamepadChanged()
////////////////////////////////////////////////////////////////////////////////
bool SoftShadows::handleKeyInput(uint32_t code, NvKeyActionType::Enum action)
{
    if (action != NvKeyActionType::UP) // so down OR repeat...
    {
        switch (code)
        {
        case NvKey::K_A:
            m_renderer.changeWorldWidthOffset(0.1f);
            break;

        case NvKey::K_D:
            m_renderer.changeWorldWidthOffset(-0.1f);
            break;

        case NvKey::K_W:
            m_renderer.changeWorldHeightOffset(0.1f);
            break;

        case NvKey::K_S:
            m_renderer.changeWorldHeightOffset(-0.1f);
            break;

        default:
            return false;
        };

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadows::renderText()
////////////////////////////////////////////////////////////////////////////////
void SoftShadows::renderText()
{
    uint64_t numIndices = 0;
    uint64_t numVerts = 0;
    uint32_t lightRes = 0;
    m_renderer.getSceneStats(numIndices, numVerts, lightRes);
    uint64_t kTris = numIndices / 3000L;
    if (m_triStats)
        m_triStats->SetValue((uint32_t)kTris);
    if (m_lightStats)
        m_lightStats->SetValue(lightRes);
}

////////////////////////////////////////////////////////////////////////////////
// NvAppFactory()
////////////////////////////////////////////////////////////////////////////////
NvAppBase* NvAppFactory()
{
    return new SoftShadows();
}
