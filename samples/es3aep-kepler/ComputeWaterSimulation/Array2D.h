//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeWaterSimulation/Array2D.h
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
// 2D array class

#ifndef ARRAY2D_H
#define ARRAY2D_H

inline int mini(const int a, const int b)
{
    return (a < b) ? a : b;
}

inline int maxi(const int a, const int b)
{
    return (a > b) ? a : b;
}

inline int clamp(int x, int a, int b)
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

	Array2D(int width, int height)
	{
		data = NULL;
		rowPointers = NULL;
		init(width, height);
	}

	void init(int width, int height)
	{
		if(data) delete [] data;
		if(rowPointers) delete [] rowPointers;

		w = width;
		h = height;
		data = new T [w*h];
		rowPointers = new T*[h];
		for(int i=0; i<h; i++)
			rowPointers[i] = &data[i*w];
		border = 0;
	}

    // create from external data
    Array2D(T *ptr, int width, int height)
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

	T & get(int x, int y)
    {
        //return data[y*w+x];
		return rowPointers[y][x];
    }

    // clamps to border
	T & getBorder(int x, int y)
	{
        if ((x < 0) || (y < 0) || (x > w-1) || (y > h-1)) {
            return border;
        } else {
		    //return data[y*w+x];
			return get(x,y);
        }
	}

    // clamps to edge
	T & getClamp(int x, int y)
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
//        int ix = (int) floorf(x);
//        int iy = (int) floorf(y);
//        float u = x - ix;
//        float v = y - iy;
//        return lerp( lerp(getClamp(ix, iy), getClamp(ix + 1, iy), u),
//                     lerp(getClamp(ix, iy+1), getClamp(ix + 1, iy+1), u), v);
//    }

    void set(int x, int y, T v)
    {
        //data[y*w+x] = v;
		rowPointers[y][x] = v;
    }

    void setSafe(int x, int y, T v)
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
	int w, h;
    T border;
	T **rowPointers;
	T *data;
};

#endif
