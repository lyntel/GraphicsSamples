//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation/noise.cpp
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
#include <NV/NvPlatformGL.h>
#include <stdlib.h>

static inline float frand()
{
    return rand() / (float) RAND_MAX;
}

// create 2D texture containing random values
GLuint createNoiseTexture2D(int w, int h, GLint internalFormat, bool mipmap)
{
    float *data = new float [w*h];
    for(int y=0; y<h; y++) {
        for(int x=0; x<w; x++) {
          data[y*w+x] = frand();
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RED, GL_FLOAT, data);
    if (mipmap) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    delete [] data;
    return tex;
}

// create 2D noise texture with neighbouring values in RGBA
GLuint createNoiseTexture2DNeighbours(int w, int h, GLuint internalFormat)
{
    // 2d random texture
    float *v = new float[w*h*4];
    for(int y=0; y<h; y++) {
        for(int x=0; x<w; x++) {
            v[0 + 4*(x + w * y)] = (rand() / (float) RAND_MAX);
            //v(x, y, 0) = (rand() / (float) RAND_MAX)*2.0f-1.0f;
        }
    }

    // store neighbouring values in rgba components
    for(int y=0; y<h; y++) {
        for(int x=0; x<w; x++) {
            v[1 + 4*(x + w * y)] = v[0 + 4*((x+1) + w * y)];
            v[2 + 4*(x + w * y)] = v[0 + 4*(x + w * (y+1))];
            v[3 + 4*(x + w * y)] = v[0 + 4*((x+1) + w * (y+1))];
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, GL_FLOAT, v);

    delete[] v;
    return tex;
}


// create 3D texture
GLuint createNoiseTexture3D(int w, int h, int d, GLint internalFormat, bool mipmap)
{
    float *data = new float [w*h*d];
    for(int z=0; z<d; z++) {
        for(int y=0; y<h; y++) {
            for(int x=0; x<w; x++) {
              data[z*(w*h) + (y*w) + x] = frand();
            }
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);

    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, w, h, d, 0, GL_RED, GL_FLOAT, data);
    if (mipmap) {
        glGenerateMipmap(GL_TEXTURE_3D);
    }

    delete [] data;
    return tex;
}

GLuint createNoiseTexture4f3D(int w, int h, int d, GLint internalFormat, bool mipmap)
{
    float *data = new float [w*h*d*4];
    float *ptr = data;
    for(int z=0; z<d; z++) {
        for(int y=0; y<h; y++) {
            for(int x=0; x<w; x++) {
              *ptr++ = frand();
              *ptr++ = frand();
              *ptr++ = frand();
              *ptr++ = frand();
            }
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);

    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, w, h, d, 0, GL_RGBA, GL_FLOAT, data);
    if (mipmap) {
        glGenerateMipmap(GL_TEXTURE_3D);
    }

    delete [] data;
    return tex;
}
