//----------------------------------------------------------------------------------
// File:        nvpr\Tiger3DES/render_font.hpp
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

#ifndef __render_font_hpp__
#define __render_font_hpp__

#include <vector>
#include <string>

// In order to get OpenGL API...
#include <NvSimpleTypes.h>
#include <NV/NvPlatformGL.h>

#include <Cg/double.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>

#include "cg4cpp_xform.hpp"

struct FontFace {
  static const int em_scale = 2048;

  const char *font_name;
  GLuint glyph_base;
  int num_chars;
  std::vector<GLfloat> horizontal_advance;
  GLfloat y_min, y_max;
  GLfloat underline_position, underline_thickness;

  FontFace(GLenum target, const char *name, int num_chars_, GLuint path_param_template);
  ~FontFace();
};

struct Message {
  std::string message;
  const FontFace *font;
  size_t message_length;
  std::vector<GLfloat> xtranslate;
  GLfloat total_advance;
  Cg::float3x3 matrix;
  bool stroking, filling;
  int underline;
  int fill_gradient;
  GLuint colorProgram;
  GLuint quadProgram;
  bool es_context;

  Message(const FontFace *font_, const char *message_, Cg::float2 to_quad[4]);

  ~Message();

  void render();
  Cg::float3x3 getMatrix();
  void multMatrix();
  void loadMatrix();

  void setFilling(bool filling_) {
    filling = filling_;
  }
  void setStroking(bool stroking_) {
    stroking = stroking_;
  }
  void setUnderline(int underline_) {
    underline = underline_;
  }
  void setEsContext(bool es_context_) {
    es_context = es_context_;
  }

  Message& operator = (const Message& val);
};

#endif // __render_font_hpp__