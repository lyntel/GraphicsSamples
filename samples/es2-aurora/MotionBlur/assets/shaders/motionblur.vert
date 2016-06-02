//----------------------------------------------------------------------------------
// File:        es2-aurora\MotionBlur\assets\shaders/motionblur.vert
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
precision mediump float;

// INPUT
attribute vec4 a_vPosition; // In BaseShader
attribute vec3 a_vNormal;   // In BaseProjNormalShader

// OUTPUT
varying vec4 v_vCorrectedPosScreenSpace;
varying vec4 v_vCurrentPosScreenSpace;
varying vec4 v_vPreviousPosScreenSpace;

// UNIFORMS
uniform  mat4 u_mProjectionMat;    // In BaseProjectionShader
uniform  mat4 u_mNormalMat;        // In BaseProjNormalShader
uniform  mat4 u_mViewMat;          // In MotionBlurES2Shader
uniform  mat4 u_mCurrentModelMat;  // In MotionBlurES2Shader
uniform  mat4 u_mPreviousModelMat; // In MotionBlurES2Shader
uniform float u_fStretchScale;     // In MotionBlurES2Shader

void main()
{
    vec4 vCurrPosWorldSpace = u_mCurrentModelMat  * a_vPosition;
    vec4 vPrevPosWorldSpace = u_mPreviousModelMat * a_vPosition;
    
    vec3 vMotionVecWorldSpace =
        vCurrPosWorldSpace.xyz - vPrevPosWorldSpace.xyz;
    vec3 vNormalVecWorldSpace = normalize(mat3(u_mNormalMat) * a_vNormal);

    // Scaling the stretch.
    vec4 vStretchPosWorldSpace = vCurrPosWorldSpace;
    vStretchPosWorldSpace.xyz -= (vMotionVecWorldSpace * u_fStretchScale);

    v_vCurrentPosScreenSpace =
        u_mProjectionMat * u_mViewMat * vCurrPosWorldSpace;
    v_vPreviousPosScreenSpace =
        u_mProjectionMat * u_mViewMat * vPrevPosWorldSpace;
    vec4 vStretchPosScreenSpace =
        u_mProjectionMat * u_mViewMat * vStretchPosWorldSpace;

    v_vCorrectedPosScreenSpace =
        dot(vMotionVecWorldSpace, vNormalVecWorldSpace) > 0.0 ?
            v_vCurrentPosScreenSpace :
            vStretchPosScreenSpace;

    gl_Position = v_vCorrectedPosScreenSpace;
}
