//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/HDRImages.h
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
#ifndef _HDRI_H_
#define _HDRI_H_

#include <vector>
#include "rgbe.h"
#include <NV/NvPlatformGL.h>

typedef unsigned short    hfloat;

hfloat convertFloatToHFloat(float *f);
float convertHFloatToFloat(hfloat hf);

void FP32toFP16(float* pt, hfloat* out,int width, int height);

class HDRImage {
public:

    HDRImage();
    virtual ~HDRImage();

    // return the width of the image
   int getWidth() const { return _width; }

    //return the height of the image
   int getHeight() const { return _height; }

    //return the dpeth of the image (0 for images with no depth)
   int getDepth() const { return _depth; }

    //return the number of mipmap levels available for the image
   int getMipLevels() const { return _levelCount; }

    //return the number of cubemap faces available for the image (0 for non-cubemap images)
   int getFaces() const { return _faces; }

    //return the format of the image data (GL_RGB, GL_BGR, etc)
   GLenum getFormat() const { return _format; }

    //return the suggested internal format for the data
   GLenum getInternalFormat() const { return _internalFormat; }

    //return the type of the image data
   GLenum getType() const { return _type; }

    //return the Size in bytes of a level of the image 
   int getImageSize(int level = 0) const;

    //return whether the image represents a cubemap
   bool isCubeMap() const { return _faces > 0; }

    //return whether the image represents a volume
   bool isVolume() const { return _depth > 0; }

    //get a pointer to level data
   const void* getLevel( int level, GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X) const;
   void* getLevel( int level, GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X);

    //initialize an image   from a file
   bool loadHDRIFromFile( const char* file);

    //convert a suitable image from a cubemap cross to a cubemap (returns false for unsuitable images)
   bool convertCrossToCubemap();

    //load an image from memory, for the purposes of saving
   bool setImage( int width, int height, GLenum format, GLenum type, const void* data);

    //save an image to a file
   bool saveImageToFile( const char* file);

protected:
    int _width;
    int _height;
    int _depth;
    int _levelCount;
    int _faces;
    GLenum _format;
    GLenum _internalFormat;
    GLenum _type;
    int _elementSize;

    //pointers to the levels
    std::vector<GLubyte*> _data;

   void freeData();
   void flipSurface(GLubyte *surf, int width, int height, int depth);
};


#endif

