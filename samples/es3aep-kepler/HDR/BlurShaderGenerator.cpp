//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/BlurShaderGenerator.cpp
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
#include "BlurShaderGenerator.h"

#include <string>
#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <math.h>

#define NV_PI   float(3.1415926535897932384626433832795)

float gaussian(float x, float s)
{
    return expf(-(s*x)*(s*x));
}


float *
generateGaussianWeights(float s, int &width)
{
    width = (int)(3.0/s);
    int size = width*2+1;
    float *weight = new float [size];

    float sum = 0.0;
    int x;
    for(x=0; x<size; x++) {
        weight[x] = gaussian((float) x-width, s);
        sum += weight[x];
    }

    for(x=0; x<size; x++) {
        weight[x] /= sum;
    }
    return weight;
}

float* generateTriangleWeights(int width)
{
    float *weights = new float [width];
    float sum = 0.0f;
    for(int i=0; i<width; i++) {
        float t = i / (float) (width-1);
        weights[i] = 1.0f - abs(t-0.5f)*2.0f;
        sum += weights[i];
    }
    for(int i=0; i<width; i++) {
        weights[i] /= sum;
    }
    return weights;
}

/*
  Generate fragment program code for a separable convolution, taking advantage of linear filtering.
  This requires roughly half the number of texture lookups.

  We want the general convolution:
    a*f(i) + b*f(i+1)
  Linear texture filtering gives us:
    f(x) = (1-alpha)*f(i) + alpha*f(i+1);
  It turns out by using the correct weight and offset we can use a linear lookup to achieve this:
    (a+b) * f(i + b/(a+b))
  as long as 0 <= b/(a+b) <= 1
*/

unsigned int generate1DConvolutionFP_filter(const char* vs, float *weights, int width, bool vertical, bool tex2D, int img_width, int img_height)
{
    // calculate new set of weights and offsets
    int nsamples = 2*width+1;
    int nsamples2 = (int) ceilf(nsamples/2.0f);
    float *weights2 = new float [nsamples2];
    float *offsets = new float [nsamples2];

    for(int i=0; i<nsamples2; i++) {
        float a = weights[i*2];
        float b;
        if (i*2+1 > nsamples-1)
            b = 0;
        else
            b = weights[i*2+1];
        weights2[i] = a + b;
        offsets[i] = b / (a + b);
        //    printf("%d: %f %f\n", i, weights2[i], offsets[i]);
    }
	//    printf("nsamples = %d\n", nsamples2);

	char szBuffer[16];
    std::ostringstream ost;
    ost <<
		"precision highp float;\n"
		"uniform sampler2D TexSampler;\n"
		"varying vec2 TexCoord0;\n"
		"void main()\n"
		"{\n"
		"vec3 sum = vec3(0.0,0.0,0.0);\n"
		"vec2 texcoord;\n";

    for(int i=0; i<nsamples2; i++) {
        float x_offset = 0, y_offset = 0;
        if (vertical) {
            y_offset = (i*2)-width+offsets[i];
        } else {
            x_offset = (i*2)-width+offsets[i];
        }
        if (tex2D) {
            x_offset = x_offset / img_width;
            y_offset = y_offset / img_height;
        }
        float weight = weights2[i];
		sprintf(szBuffer, "%f", x_offset);
		ost << "texcoord = TexCoord0 + vec2(" << szBuffer;
		sprintf(szBuffer, "%f", y_offset);
		ost << ", " << szBuffer << ");\n";
		sprintf(szBuffer, "%f", weight);
		ost << "sum += texture2D(TexSampler, texcoord).rgb*" << szBuffer << ";\n";
    }

    ost << 
        "gl_FragColor = vec4(sum, 1.0);\n"
        "}\n";

    delete [] weights2;
    delete [] offsets;
	NvGLSLProgram* program = NvGLSLProgram::createFromStrings(vs, (char*)ost.str().c_str());
	int id = program->getProgram();
	return id;
}
