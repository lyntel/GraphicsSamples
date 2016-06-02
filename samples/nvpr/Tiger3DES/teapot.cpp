//----------------------------------------------------------------------------------
// File:        nvpr\Tiger3DES/teapot.cpp
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

#include <NvSimpleTypes.h>
#include <NvAssert.h>
#include <NV/NvPlatformGL.h>
#include <NV/NvLogs.h>
#include "teapot_data.h"


static GLuint program = 0;
static GLuint posBuffer;

void initTeapot()
{
    const GLchar *vertShader =
        "attribute vec3 position;\n"
        "uniform mat4 matrix;\n"
        "uniform mat4 proj;\n"
        "uniform float scale;\n"
        "void main() {\n"
        "    vec4 pos = vec4(position * scale, 1.0);\n"
        "    pos.y = -pos.y + scale*0.1;\n"
        "    gl_Position = proj * matrix * pos;\n"
        "}\n";

    const GLchar *fragShader =
        "precision highp float;\n"
        "uniform vec3 color;\n"
        "void main() {\n"
        "    gl_FragColor = vec4(color, 1.0);\n"
        "}\n";

    const GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    const GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, &vertShader, NULL);
    glShaderSource(fshader, 1, &fragShader, NULL);
    glCompileShader(vshader);
    glCompileShader(fshader);

    program = glCreateProgram();
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);
    glDetachShader(program, vshader);
    glDetachShader(program, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    glGenBuffers(1, &posBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(teapot_vertices), teapot_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void deinitTeapot()
{
    glDeleteBuffers(1, &posBuffer);
    glDeleteProgram(program);
}

void drawTeapot(GLenum mode, GLfloat scale, GLfloat m[4][4], GLfloat mp[4][4]) 
{
    int num_indices = 16368;
    int i = 0, start = 0;
    GLint apos;

    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
    apos = glGetAttribLocation(program, "position");
    glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(apos);
    glUniform3f(glGetUniformLocation(program, "color"), 0, 1, 0);
    glUniform1f(glGetUniformLocation(program, "scale"), scale);
    glUniformMatrix4fv(glGetUniformLocation(program, "matrix"), 1, GL_FALSE, m[0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "proj"), 1, GL_FALSE, mp[0]);

    NV_ASSERT((mode == GL_LINE_STRIP) || (mode == GL_TRIANGLE_STRIP));
    while (i < num_indices) {
        if (teapot_indices[i] == -1) {
            glDrawElements(mode, i - start, GL_UNSIGNED_SHORT, &teapot_indices[start]);
            start = i + 1;
        }
        i++;
    }
    if (start < num_indices)
        glDrawElements(mode, i - start - 1, GL_UNSIGNED_SHORT, &teapot_indices[start]);

    glDisableVertexAttribArray(apos);
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

