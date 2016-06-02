//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/FileLoader.h
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
#ifndef __INCLUDED_FILE_LOADER_H
#define __INCLUDED_FILE_LOADER_H

#include <stdio.h>

struct NvFile;

/**
  A wrapper similar to fopen.  Only provides read-access to files, since the
  file returned may be in the read-only APK.  Can be called from any 
  JNI-connected thread.
  @param path The path to the file.  This path is searched within /data and 
  within the APK
  @return A pointer to an open file on success, NULL on failure
  */
NvFile*     NvFOpen(char const* path);

/**
  A wrapper similar to fclose.  Can be called from any JNI-connected thread.
  @param file A pointer to an open file opened via NvFOpen()
  */
void        NvFClose(NvFile* file);

/**
  A wrapper similar to fgets.  Can be called from any JNI-connected thread.
  @param s A char buffer to receive the string
  @param size The size of the buffer pointed to by s
  @param stream A pointer to an open file opened via NvFOpen()
  @return A pointer to s on success or NULL on failure
  */
char*       NvFGets(char* s, int size, NvFile* stream);

/**
  A wrapper similar to fread.  Can be called from any JNI-connected thread.
  @param ptr A buffer of size size into which data will be read
  @param size size of element to be read
  @param nmemb count of elements to be read
  @param stream A pointer to an open file opened via NvFOpen()
  @return The number of elements read
  */
size_t      NvFRead(void* ptr, size_t size, size_t nmemb, NvFile* stream);

#endif
