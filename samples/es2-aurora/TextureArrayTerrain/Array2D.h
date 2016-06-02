//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/Array2D.h
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

#ifndef ARRAY2D_H
#define ARRAY2D_H

#include <NvSimpleTypes.h>


inline int32_t mini(const int32_t a, const int32_t b)
{
    return (a < b) ? a : b;
}

inline int32_t maxi(const int32_t a, const int32_t b)
{
    return (a > b) ? a : b;
}

inline int32_t clamp(int32_t x, int32_t a, int32_t b)
{
    return mini(maxi(x, a), b);
}

// 2d array
template <class T>
class Array2D
{
public:
    typedef T value_type;

    Array2D()
    {
        data = NULL;
        rowPointers = NULL;
        w=h=0;
        border = 0;
    }

    Array2D(int32_t width, int32_t height)
    {
        data = NULL;
        rowPointers = NULL;
        init(width, height);
    }

    void init(int32_t width, int32_t height)
    {
        if(data) delete [] data;
        if(rowPointers) delete [] rowPointers;

        w = width;
        h = height;
        data = new T [w*h];
        rowPointers = new T*[h];
        for(int32_t i=0; i<h; i++)
            rowPointers[i] = &data[i*w];
        border = 0;
    }

    // create from external data
    Array2D(T *ptr, int32_t width, int32_t height)
    {
        w = width;
        h = height;
        data = ptr;
        border = 0;
    }

    ~Array2D()
    {
        delete [] rowPointers;
        delete [] data;
    }

    T get(int32_t x, int32_t y) const
    {
        //return data[y*w+x];
        return rowPointers[y][x];
    }

    T & get(int32_t x, int32_t y)
    {
        //return data[y*w+x];
        return rowPointers[y][x];
    }

    // clamps to border
    T & getBorder(int32_t x, int32_t y)
    {
        if ((x < 0) || (y < 0) || (x > w-1) || (y > h-1)) {
            return border;
        } else {
            //return data[y*w+x];
            return get(x,y);
        }
    }

    // clamps to edge
    T & getClamp(int32_t x, int32_t y)
    {
        x = clamp(x, 0, w-1);
        y = clamp(y, 0, h-1);
        //return data[y*w+x];
        return get(x, y);
    }

    T lerp(T a, T b, float t)
    {
        return a + t*(b - a);
    }

//    float getInterpolated(float x, float y)
//    {
//        int32_t ix = (int32_t) floorf(x);
//        int32_t iy = (int32_t) floorf(y);
//        float u = x - ix;
//        float v = y - iy;
//        return lerp( lerp(getClamp(ix, iy), getClamp(ix + 1, iy), u),
//                     lerp(getClamp(ix, iy+1), getClamp(ix + 1, iy+1), u), v);
//    }

    void set(int32_t x, int32_t y, T v)
    {
        //data[y*w+x] = v;
        rowPointers[y][x] = v;
    }

    void setSafe(int32_t x, int32_t y, T v)
    {
        if ((x >= 0) && (y >= 0) && (x < w) && (y < h)) {
            //data[y*w+x] = v;
            rowPointers[y][x] = v;
        }
    }

    T * getData()
    {
        return data;
    }

public:
    int32_t w, h;
    T border;
    T **rowPointers;
    T *data;
};

#endif
