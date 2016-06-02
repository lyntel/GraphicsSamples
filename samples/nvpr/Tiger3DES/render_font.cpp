//----------------------------------------------------------------------------------
// File:        nvpr\Tiger3DES/render_font.cpp
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

// render_font.cpp - class for rendering messia via font with NV_path_rendering

#include <string.h>

#include "render_font.hpp"

using namespace Cg;

FontFace::FontFace(GLenum target, const char *name, int num_chars_, GLuint path_param_template)
  : font_name(name)
  , num_chars(num_chars_)
  , horizontal_advance(num_chars)
{
  /* Create a range of path objects corresponding to Latin-1 character codes. */
  glyph_base = glGenPathsNV(num_chars);
  glPathGlyphRangeNV(glyph_base,
    target, name, GL_BOLD_BIT_NV,
    0, num_chars,
    GL_USE_MISSING_GLYPH_NV, path_param_template,
    GLfloat(em_scale));
  glPathGlyphRangeNV(glyph_base,
    GL_STANDARD_FONT_NAME_NV, "Sans", GL_BOLD_BIT_NV,
    0, num_chars,
    GL_USE_MISSING_GLYPH_NV, path_param_template,
    GLfloat(em_scale));

  /* Query font and glyph metrics. */
  GLfloat font_data[4];
  glGetPathMetricRangeNV(GL_FONT_Y_MIN_BOUNDS_BIT_NV|GL_FONT_Y_MAX_BOUNDS_BIT_NV|
    GL_FONT_UNDERLINE_POSITION_BIT_NV|GL_FONT_UNDERLINE_THICKNESS_BIT_NV,
    glyph_base+' ', /*count*/1,
    4*sizeof(GLfloat),
    font_data);

  y_min = font_data[0];
  y_max = font_data[1];
  underline_position = font_data[2];
  underline_thickness = font_data[3];
  glGetPathMetricRangeNV(GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV,
    glyph_base, num_chars,
    0, /* stride of zero means sizeof(GLfloat) since 1 bit in mask */
    &horizontal_advance[0]);
}

FontFace::~FontFace()
{
  glDeletePathsNV(glyph_base, num_chars);
}

const GLchar *vertShader =
"attribute vec2 position;\n"
"void main() {\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"}\n";

const GLchar *fragShader = "#extension GL_ARB_separate_shader_objects : enable\n"
"precision highp float;\n"
"uniform vec3 color;\n"
"void main() {\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";

Message::Message(const FontFace *font_, const char *message_, Cg::float2 to_quad[4])
  : message(message_)
  , font(font_)
  , message_length(strlen(message_))
  , xtranslate(message_length)
  , stroking(true)
  , filling(true)
  , underline(1)
  , fill_gradient(0)
  , es_context(true)
{
  glGetPathSpacingNV(GL_ACCUM_ADJACENT_PAIRS_NV,
    (GLsizei)message_length, GL_UNSIGNED_BYTE, message.c_str(),
    font->glyph_base,
    1.0, 1.0,
    GL_TRANSLATE_X_NV,
    &xtranslate[1]);

  /* Total advance is accumulated spacing plus horizontal advance of
  the last glyph */
  total_advance = xtranslate[message_length-1] +
    font->horizontal_advance[GLubyte(message[message_length-1])];

  Cg::float4 from_box = float4(0, font->y_min, total_advance, font->y_max);
  matrix = float3x3(box2quad(from_box, to_quad));

  const GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
  const GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(vshader, 1, &vertShader, NULL);
  glShaderSource(fshader, 1, &fragShader, NULL);
  glCompileShader(vshader);
  glCompileShader(fshader);

  quadProgram = glCreateProgram();
  glAttachShader(quadProgram, vshader);
  glAttachShader(quadProgram, fshader);
  glLinkProgram(quadProgram);
  glDetachShader(quadProgram, vshader);
  glDetachShader(quadProgram, fshader);
  glDeleteShader(vshader);
  glDeleteShader(fshader);

  colorProgram = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fragShader);
}

Message::~Message()
{
  glDeleteProgram(quadProgram);
  glDeleteProgram(colorProgram);
}

void Message::render()
{
  GLint ucolor = -1;
  GLint apos = -1;

  if (underline) {
    // Currently not working on ES, need modelview multiplication in VS??
    float position = font->underline_position,
      half_thickness = font->underline_thickness / 2;
    glDisable(GL_STENCIL_TEST);
    if (es_context) {
      GLfloat vtx1[] = { 0, position + half_thickness,
        0, position - half_thickness,
        total_advance, position + half_thickness,
        total_advance, position - half_thickness };
      glUseProgram(quadProgram);
      apos = glGetAttribLocation(quadProgram, "position");
      ucolor = glGetUniformLocation(quadProgram, "color");
      glVertexAttribPointer(apos, 2, GL_FLOAT, GL_FALSE, sizeof(vtx1), vtx1);
      glEnableVertexAttribArray(apos);
      if (underline == 2) {
        glUniform3f(ucolor, 1, 1, 1);
      } else {
        glUniform3f(ucolor, 0.5, 0.5, 0.5);
      }
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glUseProgram(0);
    } else {
      if (underline == 2) {
        glColor3f(1, 1, 1);
      }
      else {
        glColor3f(0.5, 0.5, 0.5);
      }
      glBegin(GL_QUAD_STRIP); {
        glVertex2f(0, position + half_thickness);
        glVertex2f(0, position - half_thickness);
        glVertex2f(total_advance, position + half_thickness);
        glVertex2f(total_advance, position - half_thickness);
      } glEnd();
    }
    glEnable(GL_STENCIL_TEST);
  }

  if (es_context) {
    ucolor = glGetUniformLocation(colorProgram, "color");
  }

  if (stroking) {
    glStencilStrokePathInstancedNV((GLsizei)message_length,
      GL_UNSIGNED_BYTE, message.c_str(), font->glyph_base,
      1, ~0U,  /* Use all stencil bits */
      GL_TRANSLATE_X_NV, &xtranslate[0]);
    if (es_context) {
      glUseProgram(colorProgram);
      glUniform3f(ucolor, 0.5, 0.5, 0.5); // gray
    } else {
      glColor3f(0.5, 0.5, 0.5);  // gray
    }
    glCoverStrokePathInstancedNV((GLsizei)message_length,
      GL_UNSIGNED_BYTE, message.c_str(), font->glyph_base,
      GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
      GL_TRANSLATE_X_NV, &xtranslate[0]);
    if (es_context) {
      glUseProgram(0);
    }
  }

  if (filling) {
    /* STEP 1: stencil message into stencil buffer.  Results in samples
    within the message's glyphs to have a non-zero stencil value. */
    glStencilFillPathInstancedNV((GLsizei)message_length,
      GL_UNSIGNED_BYTE, message.c_str(), font->glyph_base,
      GL_PATH_FILL_MODE_NV, ~0U,  /* Use all stencil bits */
      GL_TRANSLATE_X_NV, &xtranslate[0]);

    /* STEP 2: cover region of the message; color covered samples (those
    with a non-zero stencil value) and set their stencil back to zero. */
    switch (fill_gradient) {
    case 0:
    {
      GLfloat rgb_gen[3][3] = { { 0, 0, 1 },
      { 0, 1, 0 },
      { 0, -1, 1 } };
      glPathColorGenNV(GL_PRIMARY_COLOR,
        GL_PATH_OBJECT_BOUNDING_BOX_NV, GL_RGB, &rgb_gen[0][0]);
    }
    break;
    case 1:
      if (es_context) {
        glUseProgram(colorProgram);
        glUniform3f(ucolor, 0.5, 0.5, 0.5); // gray
      } else {
        glColor3ub(192, 192, 192);  // gray
      }
      break;
    case 2:
      if (es_context) {
        glUseProgram(colorProgram);
        glUniform3f(ucolor, 1, 1, 1); // white
      } else {
        glColor3ub(255, 255, 255);  // white
      }
      break;
    case 3:
      if (es_context) {
        glUseProgram(colorProgram);
        glUniform3f(ucolor, 0, 0, 0); // black
      } else {
        glColor3ub(0, 0, 0);  // black
      }
      break;
    }

    glCoverFillPathInstancedNV((GLsizei)message_length,
      GL_UNSIGNED_BYTE, message.c_str(), font->glyph_base,
      GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
      GL_TRANSLATE_X_NV, &xtranslate[0]);
    if (fill_gradient == 0) {
      /* Disable gradient. */
      glPathColorGenNV(GL_PRIMARY_COLOR, GL_NONE, 0, NULL);
    }
  }
  if (es_context) {
    glUseProgram(0);
  }
}
Cg::float3x3 Message::getMatrix()
{
  return matrix;
}
void Message::multMatrix()
{
  MatrixMultToGL(matrix);
}

void Message::loadMatrix()
{
  MatrixLoadToGL(matrix);
}

Message& Message::operator = (const Message& val)
{
    message = val.message;
    font = val.font;
    message_length = val.message_length;
    xtranslate = val.xtranslate;
    total_advance = val.total_advance;
    matrix = val.matrix;
    stroking = val.stroking;
    filling = val.filling;
    underline = val.underline;
    fill_gradient = val.fill_gradient;
    return *this;
}