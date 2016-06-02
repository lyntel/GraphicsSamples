//----------------------------------------------------------------------------------
// File:        nvpr\ShapedTextES/ShapedText.h
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
#ifndef ShapedText_H
#define ShapedText_H

#include <vector>

#include "PathSampleApp.h"

class NvStopWatch;
class NvFramerateCounter;

// FreeType2 headers
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftadvanc.h>
#include <freetype/ftsnames.h>
#include <freetype/tttables.h>
#include <freetype/ftoutln.h> // for FT_Outline_Decompose

// HarfBuzz headers
#include <hb.h>
#include <hb-ft.h>
#include <hb-ucdn.h>  // Unicode Database and Normalization
//#include <hb-icu.h> // Alternatively you can use hb-glib.h

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>

using namespace Cg;

enum FontType {  // Avoid "Font" since used by X headers
    ENGLISH=0,
    ARABIC,
    CHINESE,
    TAMIL,
    HEBREW,
    NUM_FONTS
};

struct TextSystem {
    hb_direction_t direction;
    const char *language;
    hb_script_t script;
    FontType font;
};

struct TextSample {
    const char *text;
    const TextSystem *text_system;
};

struct NVprShapedGlyphs {
    GLuint glyph_base;
    float scale;
    std::vector<GLuint> glyphs;
    std::vector<GLfloat> xy;

    NVprShapedGlyphs(GLuint glyph_base_, float scale_)
        : glyph_base(glyph_base_)
        , scale(scale_)
    {}
    void reset() {
        glyphs.clear();
        xy.clear();
    }
    void addGlyph(GLfloat x, GLfloat y, GLuint gindex) {
        glyphs.push_back(gindex);
        xy.push_back(x);
        xy.push_back(y);
    }
    void drawFilled(GLuint stencil_write_mask) {
        NV_ASSERT(2*glyphs.size() == xy.size());
        glMatrixPushEXT(GL_MODELVIEW); {
            glMatrixScalefEXT(GL_MODELVIEW, scale, -scale, 1);
            glStencilFillPathInstancedNV(
                GLsizei(glyphs.size()),
                GL_UNSIGNED_INT, &glyphs[0],
                glyph_base,
                GL_COUNT_UP_NV, stencil_write_mask, 
                GL_TRANSLATE_2D_NV, &xy[0]);
            glCoverFillPathInstancedNV(
                GLsizei(glyphs.size()),
                GL_UNSIGNED_INT, &glyphs[0],
                glyph_base,
                GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                GL_TRANSLATE_2D_NV, &xy[0]);
        } glMatrixPopEXT(GL_MODELVIEW);
    }
    void drawStroked(GLuint stencil_write_mask) {
        NV_ASSERT(2*glyphs.size() == xy.size());
        glMatrixPushEXT(GL_MODELVIEW); {
            glMatrixScalefEXT(GL_MODELVIEW, scale, -scale, 1);
            glStencilStrokePathInstancedNV(
                GLsizei(glyphs.size()),
                GL_UNSIGNED_INT, &glyphs[0],
                glyph_base,
                /*reference*/0x1, stencil_write_mask, 
                GL_TRANSLATE_2D_NV, &xy[0]);
            glCoverStrokePathInstancedNV(
                GLsizei(glyphs.size()),
                GL_UNSIGNED_INT, &glyphs[0],
                glyph_base,
                GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                GL_TRANSLATE_2D_NV, &xy[0]);
        } glMatrixPopEXT(GL_MODELVIEW);
    }
};

class ShapedText : public PathSampleApp {
private:
    GLuint program;
    void initProgram();
    ShapedText& operator = (const ShapedText &rhs) { NV_ASSERT(!"no assignment supported"); return *this; }

public:
    ShapedText();
    ~ShapedText();

   
    void initUI();
    void initRendering();
    void shutdownRendering();
    void display();
    void draw();

    void shapeExampleText(int text_count, const TextSample text[], float width);

    void configurationCallback(NvGLConfiguration& config);

    void updatePointSize(float new_size);

protected:
    bool stroking;
    bool filling;
    // Should be the opposite of stroking and filling.
    bool stroking_state;
    bool filling_state;

    int use_sRGB;
    bool hasFramebufferSRGB;
    GLint sRGB_capable;

    enum NVprAPImode_t {
        GLYPH_INDEX_ARRAY,
        MEMORY_GLYPH_INDEX_ARRAY,
        GLYPH_INDEX_RANGE,
    } NVprAPImode;

    static const int device_hdpi = 72;
    static const int device_vdpi = 72;
    static const int emScale = 2048;
    uint32_t current_scene;
    uint32_t loaded_scene;
    std::vector<NVprShapedGlyphs> shaped_text;
    float point_size;
    float latched_point_size;
    int background;
    GLuint path_template;

    void drawShapedExampleText();
    void initGraphics();

    void setBackground();
    void loadScene();
    void setScene(int scene);

    NvUIEventResponse handleReaction(const NvUIReaction &react);

    void initHarfBuzz();
    void shutdownHarfBuzz();

    void initViewMatrix();
    void reportMode();

    void postRedisplay();
    NvRedrawMode::Enum render_state;
    void continuousRendering(bool);
};

#endif
