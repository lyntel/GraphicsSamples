//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/FileLoader.cpp
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
#include "FileLoader.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include <string.h>
#include <algorithm>

struct NvFile {
    int32_t mLen;
    int32_t mIndex;
    char* mData;
};

NvFile* NvFOpen(char const* path)
{
    NvFile* file = new NvFile;
    file->mData = NvAssetLoaderRead(path, file->mLen);
    file->mIndex = 0;
    return file;
}

void NvFClose(NvFile* file)
{
    NvAssetLoaderFree(file->mData);
    delete file;
}

char* NvFGets(char* s, int size, NvFile* stream)
{
    char* ptr = s;

    if (stream->mIndex == stream->mLen || !size)
        return NULL;

    while (stream->mIndex < stream->mLen) {
        if (size == 1) {
            break;
        }

        char next = stream->mData[stream->mIndex++];
        *(ptr++) = next;
        size--;

        if (next == '\r' || next == '\n') {
            break;
        }
    }

    // terminator (we always terminate when there is at least one byte
    // of space in the output buffer)
    *ptr = '\0';

    return s;
}

size_t NvFRead(void* ptr, size_t size, size_t nmemb, NvFile* stream)
{
    size_t totalCount = (stream->mLen - stream->mIndex) / size;
    totalCount = (totalCount > nmemb) ? nmemb : totalCount;

    memcpy(ptr, stream->mData + stream->mIndex, size * totalCount);
    stream->mIndex += int32_t(size * totalCount);

    return totalCount;
}

