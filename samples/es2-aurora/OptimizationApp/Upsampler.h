//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/Upsampler.h
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

#ifndef UPSAMPLER_H
#define UPSAMPLER_H

#include <NvSimpleTypes.h>


#include "SceneFBOs.h"
#include "SceneInfo.h"

class UpsampleBilinearProgram;
class UpsampleCrossBilateralProgram;

class Upsampler
{
public:
    struct Params
    {
        Params()
        : upsamplingDepthMult(32.f)
        , upsamplingThreshold(0.1f)
        , useCrossBilateral(true)
        , useBlit(true)
        {
        }
        float upsamplingDepthMult;
        float upsamplingThreshold;
        bool useCrossBilateral;
        bool useBlit;
    };

    Upsampler(SceneFBOs *fbos);
    ~Upsampler();

    void drawQuad(GLint positionAttrib, GLint texcoordAttrib);
    void drawBilinearUpsampling(UpsampleBilinearProgram *prog, GLint texture);
    void drawCrossBilateralUpsampling(UpsampleCrossBilateralProgram *prog);

    void upsampleParticleColors(NvWritableFB& target);
    void upsampleSceneColors(NvWritableFB& target);

    Params& getParams()
    {
        return m_params;
    }

protected:
    Params m_params;
    SceneFBOs *m_fbos;
    UpsampleBilinearProgram *m_upsampleBilinearProg;
    UpsampleCrossBilateralProgram *m_upsampleCrossBilateralProg;
};

#endif // UPSAMPLER_H
