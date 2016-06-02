//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/rgbe.cpp
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

#include "rgbe.h"
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

/* This file contains code to read and write four byte rgbe file format
 developed by Greg Ward.  It handles the conversions between rgbe and
 pixels consisting of floats.  The data is assumed to be an array of floats.
 By default there are three floats per pixel in the order red, green, blue.
 (RGBE_DATA_??? values control this.)  Only the mimimal header reading and 
 writing is implemented.  Each routine does error checking and will return
 a status value as defined below.  This code is intended as a skeleton so
 feel free to modify it to suit your needs.

 (Place notice here if you modified the code.)
 Modified by NVIDIA to support Android, May 2013

 posted to http://www.graphics.cornell.edu/~bjw/
 written by Bruce Walter  (bjw@graphics.cornell.edu)  5/26/95
 based on code written by Greg Ward
*/

/* offsets to red, green, and blue components in a data (float) pixel */
#define RGBE_DATA_RED    0
#define RGBE_DATA_GREEN  1
#define RGBE_DATA_BLUE   2
/* number of floats per pixel */
#define RGBE_DATA_SIZE   3

enum rgbe_error_codes {
  rgbe_read_error,
  rgbe_write_error,
  rgbe_format_error,
  rgbe_memory_error,
};

/* default error routine.  change this to change error handling */
static int rgbe_error(int rgbe_error_code, char *msg)
{
  switch (rgbe_error_code) {
  case rgbe_read_error:
    perror("RGBE read error");
    break;
  case rgbe_write_error:
    perror("RGBE write error");
    break;
  case rgbe_format_error:
    fprintf(stderr,"RGBE bad file format: %s\n",msg);
    break;
  default:
  case rgbe_memory_error:
    fprintf(stderr,"RGBE error: %s\n",msg);
  }
  return RGBE_RETURN_FAILURE;
}

/* standard conversion from float pixels to rgbe pixels */
/* note: you can remove the "inline"s if your compiler complains about it */
INLINE void 
float2rgbe(unsigned char rgbe[4], float red, float green, float blue)
{
  float v;
  int e;

  v = red;
  if (green > v) v = green;
  if (blue > v) v = blue;
  if (v < 1e-32) {
    rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
  }
  else {
    v = (float)frexp(v,&e) * 256.0f/v;
    rgbe[0] = (unsigned char) (red * v);
    rgbe[1] = (unsigned char) (green * v);
    rgbe[2] = (unsigned char) (blue * v);
    rgbe[3] = (unsigned char) (e + 128);
  }
}

/* standard conversion from rgbe to float pixels */
/* note: Ward uses ldexp(col+0.5,exp-(128+8)).  However we wanted pixels */
/*       in the range [0,1] to map back into the range [0,1].            */
INLINE void 
rgbe2float(float *red, float *green, float *blue, unsigned char rgbe[4])
{
  float f;

  if (rgbe[3]) {   /*nonzero pixel*/
    f = (float)ldexp(1.0,rgbe[3]-(int)(128+8));
    *red = rgbe[0] * f;
    *green = rgbe[1] * f;
    *blue = rgbe[2] * f;
  }
  else
    *red = *green = *blue = 0.0;
}

/* default minimal header. modify if you want more information in header */
int RGBE_WriteHeader(NvFile *fp, int width, int height, rgbe_header_info *info)
{
  //char *programtype = "RGBE";

  //if (info && (info->valid & RGBE_VALID_PROGRAMTYPE))
  //  programtype = info->programtype;
  //if (fprintf(fp,"#?%s\n",programtype) < 0)
  //  return rgbe_error(rgbe_write_error,NULL);
  ///* The #? is to identify file type, the programtype is optional. */
  //if (info && (info->valid & RGBE_VALID_GAMMA)) {
  //  if (fprintf(fp,"GAMMA=%g\n",info->gamma) < 0)
  //    return rgbe_error(rgbe_write_error,NULL);
  //}
  //if (info && (info->valid & RGBE_VALID_EXPOSURE)) {
  //  if (fprintf(fp,"EXPOSURE=%g\n",info->exposure) < 0)
  //    return rgbe_error(rgbe_write_error,NULL);
  //}
  //if (fprintf(fp,"FORMAT=32-bit_rle_rgbe\n\n") < 0)
  //  return rgbe_error(rgbe_write_error,NULL);
  //if (fprintf(fp, "-Y %d +X %d\n", height, width) < 0)
  //  return rgbe_error(rgbe_write_error,NULL);
  return RGBE_RETURN_SUCCESS;
}

/* minimal header reading.  modify if you want to parse more information */
int RGBE_ReadHeader(NvFile *fp, int *width, int *height, rgbe_header_info *info)
{
  char buf[128];
  int found_format;
  float tempf;
  int i;

  found_format = 0;
  if (info) {
    info->valid = 0;
    info->programtype[0] = 0;
    info->gamma = info->exposure = 1.0;
  }
  if (NvFGets(buf,sizeof(buf)/sizeof(buf[0]),fp) == NULL)
    return rgbe_error(rgbe_read_error,NULL);

  if ((buf[0] != '#')||(buf[1] != '?')) {
    /* if you want to require the magic token then uncomment the next line */
    /*return rgbe_error(rgbe_format_error,"bad initial token"); */
  }
  else if (info) {
    info->valid |= RGBE_VALID_PROGRAMTYPE;
    for(i=0;i<sizeof(info->programtype)-1;i++) {
      if ((buf[i+2] == 0) || isspace(buf[i+2]))
	break;
      info->programtype[i] = buf[i+2];
    }
    info->programtype[i] = 0;
    if (NvFGets(buf,sizeof(buf)/sizeof(buf[0]),fp) == 0)
      return rgbe_error(rgbe_read_error,NULL);
  }

  for(;;) {
    if ((buf[0] == 0)||(buf[0] == '\n'))
      return rgbe_error(rgbe_format_error,(char*)"no FORMAT specifier found");
    else if (strcmp(buf,"FORMAT=32-bit_rle_rgbe\n") == 0)
      break;       /* format found so break out of loop */
    else if (info && (sscanf(buf,"GAMMA=%g",&tempf) == 1)) {
      info->gamma = tempf;
      info->valid |= RGBE_VALID_GAMMA;
    }
    else if (info && (sscanf(buf,"EXPOSURE=%g",&tempf) == 1)) {
      info->exposure = tempf;
      info->valid |= RGBE_VALID_EXPOSURE;
    }
    if (NvFGets(buf,sizeof(buf)/sizeof(buf[0]),fp) == 0)
      return rgbe_error(rgbe_read_error,NULL);
  }

#if 0
  if (fgets(buf,sizeof(buf)/sizeof(buf[0]),fp) == 0)
    return rgbe_error(rgbe_read_error,NULL);
  if (strcmp(buf,"\n") != 0)
    return rgbe_error(rgbe_format_error,
		      "missing blank line after FORMAT specifier");
#endif

  for(;;) {
    if (NvFGets(buf,sizeof(buf)/sizeof(buf[0]),fp) == 0)
      return rgbe_error(rgbe_read_error,NULL);

    if (sscanf(buf,"-Y %d +X %d",height,width) == 2)
      break;
  }

  return RGBE_RETURN_SUCCESS;
}

/* simple write routine that does not use run length encoding */
/* These routines can be made faster by allocating a larger buffer and
   fread-ing and fwrite-ing the data in larger chunks */
int RGBE_WritePixels(NvFile *fp, float *data, int numpixels)
{
  //unsigned char rgbe[4];

  //while (numpixels-- > 0) {
  //  float2rgbe(rgbe,data[RGBE_DATA_RED],
	 //      data[RGBE_DATA_GREEN],data[RGBE_DATA_BLUE]);
  //  data += RGBE_DATA_SIZE;
  //  if (fwrite(rgbe, sizeof(rgbe), 1, fp) < 1)
  //    return rgbe_error(rgbe_write_error,NULL);
  //}
  return RGBE_RETURN_SUCCESS;
}

/* simple read routine.  will not correctly handle run length encoding */
int RGBE_ReadPixels(NvFile *fp, float *data, int numpixels)
{
  unsigned char rgbe[4];

  while(numpixels-- > 0) {
    if (NvFRead(rgbe, sizeof(rgbe), 1, fp) < 1)
      return rgbe_error(rgbe_read_error,NULL);
    rgbe2float(&data[RGBE_DATA_RED],&data[RGBE_DATA_GREEN],
	       &data[RGBE_DATA_BLUE],rgbe);
    data += RGBE_DATA_SIZE;
  }
  return RGBE_RETURN_SUCCESS;
}


int RGBE_ReadPixels_Raw(NvFile *fp, unsigned char *data, int numpixels)
{
  if (NvFRead(data, 4, numpixels, fp) < (size_t)numpixels)
    return rgbe_error(rgbe_read_error,NULL);

  return RGBE_RETURN_SUCCESS;
}



      
int RGBE_ReadPixels_RLE(NvFile *fp, float *data, int scanline_width,
			int num_scanlines)
{
  unsigned char rgbe[4], *scanline_buffer, *ptr, *ptr_end;
  int i, count;
  unsigned char buf[2];

  if ((scanline_width < 8)||(scanline_width > 0x7fff))
    /* run length encoding is not allowed so read flat*/
    return RGBE_ReadPixels(fp,data,scanline_width*num_scanlines);
  scanline_buffer = NULL;
  /* read in each successive scanline */
  while(num_scanlines > 0) {
    if (NvFRead(rgbe,sizeof(rgbe),1,fp) < 1) {
      free(scanline_buffer);
      return rgbe_error(rgbe_read_error,NULL);
    }
    if ((rgbe[0] != 2)||(rgbe[1] != 2)||(rgbe[2] & 0x80)) {
      /* this file is not run length encoded */
      rgbe2float(&data[0],&data[1],&data[2],rgbe);
      data += RGBE_DATA_SIZE;
      free(scanline_buffer);
      return RGBE_ReadPixels(fp,data,scanline_width*num_scanlines-1);
    }
    if ((((int)rgbe[2])<<8 | rgbe[3]) != scanline_width) {
      free(scanline_buffer);
      return rgbe_error(rgbe_format_error,(char*)"wrong scanline width");
    }
    if (scanline_buffer == NULL)
      scanline_buffer = (unsigned char *)
	malloc(sizeof(unsigned char)*4*scanline_width);
    if (scanline_buffer == NULL) 
      return rgbe_error(rgbe_memory_error,(char*)"unable to allocate buffer space");
    
    ptr = &scanline_buffer[0];
    /* read each of the four channels for the scanline into the buffer */
    for(i=0;i<4;i++) {
      ptr_end = &scanline_buffer[(i+1)*scanline_width];
      while(ptr < ptr_end) {
	if (NvFRead(buf,sizeof(buf[0])*2,1,fp) < 1) {
	  free(scanline_buffer);
	  return rgbe_error(rgbe_read_error,NULL);
	}
	if (buf[0] > 128) {
	  /* a run of the same value */
	  count = buf[0]-128;
	  if ((count == 0)||(count > ptr_end - ptr)) {
	    free(scanline_buffer);
	    return rgbe_error(rgbe_format_error,(char*)"bad scanline data");
	  }
	  while(count-- > 0)
	    *ptr++ = buf[1];
	}
	else {
	  /* a non-run */
	  count = buf[0];
	  if ((count == 0)||(count > ptr_end - ptr)) {
	    free(scanline_buffer);
	    return rgbe_error(rgbe_format_error,(char*)"bad scanline data");
	  }
	  *ptr++ = buf[1];
	  if (--count > 0) {
	    if (NvFRead(ptr,sizeof(*ptr)*count,1,fp) < 1) {
	      free(scanline_buffer);
	      return rgbe_error(rgbe_read_error,NULL);
	    }
	    ptr += count;
	  }
	}
      }
    }
    /* now convert data from buffer into floats */
    for(i=0;i<scanline_width;i++) {
      rgbe[0] = scanline_buffer[i];
      rgbe[1] = scanline_buffer[i+scanline_width];
      rgbe[2] = scanline_buffer[i+2*scanline_width];
      rgbe[3] = scanline_buffer[i+3*scanline_width];
      rgbe2float(&data[RGBE_DATA_RED],&data[RGBE_DATA_GREEN],
		 &data[RGBE_DATA_BLUE],rgbe);
      data += RGBE_DATA_SIZE;
    }
    num_scanlines--;
  }
  free(scanline_buffer);
  return RGBE_RETURN_SUCCESS;
}


int RGBE_ReadPixels_Raw_RLE(NvFile *fp, unsigned char *data, int scanline_width,
			int num_scanlines)
{
  unsigned char rgbe[4], *scanline_buffer, *ptr, *ptr_end;
  int i, count;
  unsigned char buf[2];

  if ((scanline_width < 8)||(scanline_width > 0x7fff))
    /* run length encoding is not allowed so read flat*/
    return RGBE_ReadPixels_Raw(fp,data,scanline_width*num_scanlines);

  scanline_buffer = NULL;
  /* read in each successive scanline */
  while(num_scanlines > 0) {
    if (NvFRead(rgbe,sizeof(rgbe),1,fp) < 1) {
      free(scanline_buffer);
      return rgbe_error(rgbe_read_error,NULL);
    }

    if ((rgbe[0] != 2)||(rgbe[1] != 2)||(rgbe[2] & 0x80)) {
      /* this file is not run length encoded */
      data[0] = rgbe[0];
      data[1] = rgbe[1];
      data[2] = rgbe[2];
      data[3] = rgbe[3];
      data += RGBE_DATA_SIZE;
      free(scanline_buffer);
      return RGBE_ReadPixels_Raw(fp,data,scanline_width*num_scanlines-1);
    }

    if ((((int)rgbe[2])<<8 | rgbe[3]) != scanline_width) {
      free(scanline_buffer);
      return rgbe_error(rgbe_format_error,(char*)"wrong scanline width");
    }

    if (scanline_buffer == NULL)
      scanline_buffer = (unsigned char *) malloc(sizeof(unsigned char)*4*scanline_width);

    if (scanline_buffer == NULL) 
      return rgbe_error(rgbe_memory_error,(char*)"unable to allocate buffer space");
    
    ptr = &scanline_buffer[0];
    /* read each of the four channels for the scanline into the buffer */
    for(i=0;i<4;i++) {
      ptr_end = &scanline_buffer[(i+1)*scanline_width];
      while(ptr < ptr_end) {
	    if (NvFRead(buf,sizeof(buf[0])*2,1,fp) < 1) {
	      free(scanline_buffer);
	      return rgbe_error(rgbe_read_error,NULL);
	    }
	    if (buf[0] > 128) {
	      /* a run of the same value */
	      count = buf[0]-128;
	      if ((count == 0)||(count > ptr_end - ptr)) {
	        free(scanline_buffer);
	        return rgbe_error(rgbe_format_error,(char*)"bad scanline data");
	      }
	      while(count-- > 0)
	        *ptr++ = buf[1];
	    }
	    else {
	      /* a non-run */
	      count = buf[0];
	      if ((count == 0)||(count > ptr_end - ptr)) {
	        free(scanline_buffer);
	        return rgbe_error(rgbe_format_error,(char*)"bad scanline data");
	      }
	      *ptr++ = buf[1];
	      if (--count > 0) {
	        if (NvFRead(ptr,sizeof(*ptr)*count,1,fp) < 1) {
	          free(scanline_buffer);
	          return rgbe_error(rgbe_read_error,NULL);
	        }
	        ptr += count;
	      }
	    }
      }
    }
    /* copy byte data to output */
    for(i=0;i<scanline_width;i++) {
      data[0] = scanline_buffer[i];
      data[1] = scanline_buffer[i+scanline_width];
      data[2] = scanline_buffer[i+2*scanline_width];
      data[3] = scanline_buffer[i+3*scanline_width];
      data += 4;
    }
    num_scanlines--;
  }
  free(scanline_buffer);
  return RGBE_RETURN_SUCCESS;
}

