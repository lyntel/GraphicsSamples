//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/rgbe.h
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
/* THIS CODE CARRIES NO GUARANTEE OF USABILITY OR FITNESS FOR ANY PURPOSE.
 * WHILE THE AUTHORS HAVE TRIED TO ENSURE THE PROGRAM WORKS CORRECTLY,
 * IT IS STRICTLY USE AT YOUR OWN RISK.  */

/* utility for reading and writing Ward's rgbe image format.
   See rgbe.txt file for more details.
*/
#ifndef _H_RGBE
#define _H_RGBE
#include "FileLoader.h"
#include <NV/NvPlatformGL.h>
#include <stdio.h>

typedef struct {
  int valid;            /* indicate which fields are valid */
  char programtype[16]; /* listed at beginning of file to identify it 
                         * after "#?".  defaults to "RGBE" */ 
  float gamma;          /* image has already been gamma corrected with 
                         * given gamma.  defaults to 1.0 (no correction) */
  float exposure;       /* a value of 1.0 in an image corresponds to
						 * <exposure> watts/steradian/m^2. 
						 * defaults to 1.0 */
} rgbe_header_info;

/* flags indicating which fields in an rgbe_header_info are valid */
#define RGBE_VALID_PROGRAMTYPE 0x01
#define RGBE_VALID_GAMMA       0x02
#define RGBE_VALID_EXPOSURE    0x04

/* return codes for rgbe routines */
#define RGBE_RETURN_SUCCESS 0
#define RGBE_RETURN_FAILURE -1

#ifdef __cplusplus
extern "C" {
#endif
/* read or write headers */
/* you may set rgbe_header_info to null if you want to */
int RGBE_WriteHeader(NvFile *fp, int width, int height, rgbe_header_info *info);
int RGBE_ReadHeader(NvFile *fp, int *width, int *height, rgbe_header_info *info);

/* read or write pixels */
/* can read or write pixels in chunks of any size including single pixels*/
int RGBE_WritePixels(NvFile *fp, float *data, int numpixels);
int RGBE_ReadPixels(NvFile *fp, float *data, int numpixels);

/* read or write run length encoded files */
/* must be called to read or write whole scanlines */
int RGBE_WritePixels_RLE(NvFile *fp, float *data, int scanline_width,
			 int num_scanlines);
int RGBE_ReadPixels_RLE(NvFile *fp, float *data, int scanline_width,
			int num_scanlines);

int RGBE_ReadPixels_Raw_RLE(NvFile *fp, unsigned char *data, int scanline_width,
            int num_scanlines);

#ifdef _CPLUSPLUS
/* define if your compiler understands inline commands */
#define INLINE inline
#else
#define INLINE
#endif

INLINE void float2rgbe(unsigned char rgbe[4], float red, float green, float blue);
INLINE void rgbe2float(float *red, float *green, float *blue, unsigned char rgbe[4]);


#ifdef __cplusplus
}
#endif

#endif /* _H_RGBE */



