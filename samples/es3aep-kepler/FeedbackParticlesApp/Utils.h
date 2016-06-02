//----------------------------------------------------------------------------------
// File:        es3aep-kepler\FeedbackParticlesApp/Utils.h
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
#ifndef _UTILS_H
#define _UTILS_H
#include <string>
#include <NvAssetLoader/NvAssetLoader.h>

//-----------------------------------------------------------------------------
//
class NvGLSLProgram;


//-----------------------------------------------------------------------------
//
class NvAsset
{
public:
    NvAsset(const std::string& rstrName)
        : m_pAssetData(NULL)
    {
        m_pAssetData = NvAssetLoaderRead(rstrName.c_str(), m_Length);
    }
    ~NvAsset()
    {
        if (NULL != m_pAssetData)
            NvAssetLoaderFree((char*)m_pAssetData);
    }

    operator const unsigned char*() const
    {
        return (unsigned char*)m_pAssetData;
    }

    operator const char*() const
    {
        return (char*)m_pAssetData;
    }

private:
    void*   m_pAssetData;
    int32_t m_Length;
};


//-----------------------------------------------------------------------------
//
template<typename T>
static inline void NvSafeDelete(T* obj)
{
    if (NULL != obj)
    {
        delete obj;
        obj = NULL;
    }
}


//-----------------------------------------------------------------------------
//
template<typename T>
static inline void NvSafeDeleteArray(T* obj)
{
    if (NULL != obj)
    {
        delete [] obj;
        obj = NULL;
    }
}


//-----------------------------------------------------------------------------
//
#define countof(A) (sizeof(A) / sizeof(*A))


//-----------------------------------------------------------------------------
//
struct NvVertexAttribute
{
    const char* name;
    uint32_t    type;
    uint32_t    size;
    intptr_t    offset;
    uint32_t    stride;
    bool        isNormalized;
    
    void        apply(NvGLSLProgram* program);
    void        reset(NvGLSLProgram* program);
};


//-----------------------------------------------------------------------------
//
template<int N>
struct Ring
{
    Ring() : position(0),fullCircle(0) {};

    void     advance() { fullCircle |= position == N - 2; position = (position + 1)%N; }
    uint32_t begin()   { return position; };
    uint32_t end()     { return (position + N - 1)%N; };
    bool     isFullCycle() { return fullCircle; };

private:
    uint32_t position   :30;
    uint32_t fullCircle :1;
};

#endif
