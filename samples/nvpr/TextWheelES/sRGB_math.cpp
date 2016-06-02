//----------------------------------------------------------------------------------
// File:        nvpr\TextWheelES/sRGB_math.cpp
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

// sRGB_math.cpp - sRGB color space conversion utilities

#include <math.h>

#include "sRGB_math.hpp"

#define FLOAT_TO_UB(_f) \
        (unsigned char)(floorf((_f) * 255.0f + 0.5f))

float convertLinearColorComponentToSRGBf(const float cl)
{
    float csf;

    if (cl > 1.0f) {
        csf = 1.0f;
    } else if (cl > 0.0f) {
        if (cl < 0.0031308f) {
            csf = 12.92f * cl;
        } else {
            csf = 1.055f * (float)pow(cl, 0.41666f) - 0.055f;
        }
    } else {
        /* IEEE NaN should get here since comparisons with NaN always
           fail. */
        csf = 0.0f;
    }
    return csf;
}

unsigned char convertLinearColorComponentToSRGBub(const float cl)
{
    unsigned char cs;

    if (cl > 1.0f) {
        cs = 255;
    } else if (cl > 0.0f) {
        float csf;

        if (cl < 0.0031308f) {
            csf = 12.92f * cl;
        } else {
            csf = 1.055f * (float)pow(cl, 0.41666f) - 0.055f;
        }
        cs = FLOAT_TO_UB(csf);
    } else {
        /* IEEE NaN should get here since comparisons with NaN always
           fail. */
        cs = 0;
    }
    return cs;
}

float convertSRGBColorComponentToLinearf(const float cs)
{
    float cl;

    if (cs <= 0.04045f) {
        cl = cs / 12.92f;
    } else {
        cl = (float)pow((cs + 0.055f)/1.055f, 2.4f);
    }
    return cl;
}
