//----------------------------------------------------------------------------------
// File:        nvpr\ShapedTextES/ShapedText.cpp
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
#include "ShapedText.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <vector>

#include <Cg/dot.hpp>
#include <Cg/inverse.hpp>
#include <Cg/length.hpp>
#include <Cg/mul.hpp>
#include <Cg/round.hpp>
#include <Cg/iostream.hpp>

using namespace Cg;
using std::vector;

#include "sRGB_math.hpp"
#include "countof.h"
#include "cg4cpp_xform.hpp"

#ifndef GLAPIENTRYP
# ifdef _WIN32
#  define GLAPIENTRYP __stdcall *
# else
#  define GLAPIENTRYP *
# endif
#endif

// New NV_path_rendering API version 1.2 command for getting path objects ordered by font glyph index
typedef GLenum (GLAPIENTRYP PFNGLPATHGLYPHINDEXARRAYNVPROC) (GLuint firstPathName, GLenum fontTarget, const GLvoid *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
typedef GLenum (GLAPIENTRYP PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC) (GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const GLvoid *fontData, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
typedef GLenum (GLAPIENTRYP PFNGLPATHGLYPHINDEXRANGENVPROC) (GLenum fontTarget, const GLvoid *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint *baseAndCount);
static PFNGLPATHGLYPHINDEXARRAYNVPROC glPathGlyphIndexArrayNV = NULL;
static PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC glPathMemoryGlyphIndexArrayNV = NULL;
static PFNGLPATHGLYPHINDEXRANGENVPROC glPathGlyphIndexRangeNV = NULL;
// Possible return tokens from glPathGlyphIndexRangeNV:
#define GL_FONT_GLYPHS_AVAILABLE_NV                         0x9368
#define GL_FONT_TARGET_UNAVAILABLE_NV                       0x9369
#define GL_FONT_UNAVAILABLE_NV                              0x936A
#define GL_FONT_CORRUPT_NV                                  0x936B
#define GL_STANDARD_FONT_FORMAT_NV                          0x936C
#define GL_FONT_NUM_GLYPH_INDICES_BIT_NV                    0x20000000

// Text systems
const TextSystem latin_text_system = {
    HB_DIRECTION_LTR,
    "en",
    HB_SCRIPT_LATIN,
    ENGLISH
};
const TextSystem arabic_text_system = {
    HB_DIRECTION_RTL,
    "ar",
    HB_SCRIPT_ARABIC,
    ARABIC
};
const TextSystem vertical_chinese_text_system = {
    HB_DIRECTION_TTB,
    "ch",
    HB_SCRIPT_HAN,
    CHINESE
};
const TextSystem horizontal_chinese_text_system = {
    HB_DIRECTION_LTR,
    "ch",
    HB_SCRIPT_HAN,
    CHINESE
};
const TextSystem tamil_text_system = {
    HB_DIRECTION_RTL,
    "ta",
    HB_SCRIPT_TAMIL,
    TAMIL
};
const TextSystem hebrew_text_system = {
    HB_DIRECTION_RTL,
    "iw",  // Hebrew
    HB_SCRIPT_HEBREW,
    HEBREW
};

// These text samples contain UTF-8 strings...
TextSample cat_text[] = {
    { "Ленивый рыжий кот",     // "Lazy ginger cat" in Russian
    &latin_text_system },
    { "كسول الزنجبيل القط",  // "Lazy ginger cat" in Arabic
    &arabic_text_system },
    { "懶惰的姜貓",             // "Lazy ginger cat" in Chinese
    &vertical_chinese_text_system }
};

TextSample some_text[] = {
    { "This is some english text",    // English
    &latin_text_system },
    { "هذه هي بعض النصوص العربي",  // "These are some of the Arab texts" in Arabic
    &arabic_text_system },
    { "這是一些中文",                  // "This is some Chinese" in Chinese
    &vertical_chinese_text_system }
};

TextSample cloud_text[] = {
    { "Every cloud is sky poetry",    // English
    &latin_text_system },
    { "Каждое облако небо поэзии",    // Russian
    &latin_text_system },
    { "كل سحابة السماء الشعر",      // Arabic
    &arabic_text_system },
    { "每個雲是天空的詩",               // Chinese
    &horizontal_chinese_text_system }
};

TextSample change_text[] = {
    { "Writing changes everything",    // English
    &latin_text_system },
    { "Написание все меняется",    // Russian
    &latin_text_system },
    { "كتابة كل شيء يتغير",      // Arabic
    &arabic_text_system },
    { "寫作改變了一切",               // Chinese
    &horizontal_chinese_text_system },
    { "எழுதுதல் மாற்றங்கள் எல்லாம்",    // Tamil
    &tamil_text_system },
    { "Pisanie wszystko się zmienia",    // Polish
    &latin_text_system },
    { "שינויי כתיבת הכל",  // Hebrew
    &hebrew_text_system },
};

FT_Library ft_library;

hb_font_t *hb_ft_font[NUM_FONTS];
GLuint nvpr_glyph_base[NUM_FONTS];
GLuint nvpr_glyph_count[NUM_FONTS];
const char *font_names[NUM_FONTS] = {
    "fonts/noto/NotoSerif-Regular.ttf",
    "fonts/noto/NotoNaskhArabic-Regular.ttf",
    "fonts/noto/NotoSansCJKtc-Light.otf",
    "fonts/noto/NotoSansTamil-Regular.ttf",
    "fonts/noto/NotoSansHebrew-Regular.ttf",
};

const float EM_SCALE = 2048;

#define PRE_glPathGlyphIndexRangeNV  // Uncomment for older drivers.
#ifdef PRE_glPathGlyphIndexRangeNV
struct PathGenState {
    bool needs_close;
    vector<char> cmds;
    vector<float> coords;
    const float glyph_scale;

    PathGenState(float glyph_scale_)
        : needs_close(false)
        , glyph_scale(glyph_scale_)
    {}

    PathGenState& operator = (const PathGenState& rhs) {
        NV_ASSERT(!"no operator = supported/needed");
        return *this;
    }
};

// Callbacks to populate FT_Outline_Funcs 
// http://freetype.sourceforge.net/freetype2/docs/reference/ft2-outline_processing.html#FT_Outline_Funcs

// FT_Outline_Funcs move_to callback
static int move_to(const FT_Vector* to,
                   void* user)
{
    PathGenState *info = reinterpret_cast<PathGenState*>(user);
    if (info->needs_close) {
        info->cmds.push_back('z');
    }
    info->cmds.push_back('M');
    const float scale = info->glyph_scale;
    info->coords.push_back(scale * float(to->x));
    info->coords.push_back(scale * float(to->y));
    info->needs_close = true;
    return 0;
}

// FT_Outline_Funcs line_to callback
static int line_to(const FT_Vector* to,
                   void* user)
{
    PathGenState *info = reinterpret_cast<PathGenState*>(user);
    info->cmds.push_back('L');
    const float scale = info->glyph_scale;
    info->coords.push_back(scale * float(to->x));
    info->coords.push_back(scale * float(to->y));
    return 0;
}

// FT_Outline_Funcs conic_to callback (for quadratic Bezier segments)
static int conic_to(const FT_Vector* control,
                    const FT_Vector* to,
                    void* user)
{
    PathGenState *info = reinterpret_cast<PathGenState*>(user);
    info->cmds.push_back('Q');
    const float scale = info->glyph_scale;
    info->coords.push_back(scale * float(control->x));
    info->coords.push_back(scale * float(control->y));
    info->coords.push_back(scale * float(to->x));
    info->coords.push_back(scale * float(to->y));
    return 0;
}

// FT_Outline_Funcs cubic_to callback (for cubic Bezier segments)
static int cubic_to(const FT_Vector* control1,
                    const FT_Vector *control2,
                    const FT_Vector* to,
                    void* user)
{
    PathGenState *info = reinterpret_cast<PathGenState*>(user);
    info->cmds.push_back('C');
    const float scale = info->glyph_scale;
    info->coords.push_back(scale * float(control1->x));
    info->coords.push_back(scale * float(control1->y));
    info->coords.push_back(scale * float(control2->x));
    info->coords.push_back(scale * float(control2->y));
    info->coords.push_back(scale * float(to->x));
    info->coords.push_back(scale * float(to->y));
    return 0;
}

void generateGlyph(GLuint path, FT_Face face, FT_UInt glyph_index)
{
    const float em_scale = EM_SCALE;
    const FT_UShort units_per_em = face->units_per_EM;

    FT_Error error;

    // http://freetype.sourceforge.net/freetype2/docs/reference/ft2-base_interface.html#FT_Load_Glyph
    error = FT_Load_Glyph(face, glyph_index,
        FT_LOAD_NO_SCALE |   // Don't scale the outline glyph loaded, but keep it in font units.
        FT_LOAD_NO_BITMAP);  // Ignore bitmap strikes when loading.
    NV_ASSERT(!error);
    FT_GlyphSlot slot = face->glyph;

    NV_ASSERT(slot->format == FT_GLYPH_FORMAT_OUTLINE);

    const float glyph_scale = em_scale / units_per_em;

    FT_Outline outline = slot->outline;

    static const FT_Outline_Funcs funcs = {
        move_to,
        line_to,
        conic_to,
        cubic_to,
        0,  // shift
        0   // delta
    };

    PathGenState info(glyph_scale);
    // http://freetype.sourceforge.net/freetype2/docs/reference/ft2-outline_processing.html#FT_Outline_Decompose
    FT_Outline_Decompose(&outline, &funcs, &info);
    if (info.needs_close) {
        info.cmds.push_back('z');
    }

    const GLsizei num_cmds = info.cmds.size() ? GLsizei(info.cmds.size()) : 0;
    const GLsizei num_coords = info.coords.size() ? GLsizei(info.coords.size()) : 0;
    const GLubyte *cmds = info.cmds.size() ? reinterpret_cast<GLubyte*>(&info.cmds[0]) : NULL;
    const GLfloat *coords = info.coords.size() ? &info.coords[0] : NULL;
    glPathCommandsNV(path,
        num_cmds, cmds, 
        num_coords, GL_FLOAT, coords);

    glPathParameterfNV(path, GL_PATH_STROKE_WIDTH_NV, 0.1f*EM_SCALE);  // 10% of emScale
    glPathParameteriNV(path, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
}
#endif // PRE_glPathGlyphIndexRangeNV

void ShapedText::reportMode()
{
    switch (NVprAPImode) {
    case GLYPH_INDEX_ARRAY:
        LOGI("Using glPathGlyphIndexArrayNV\n");
        break;
    case MEMORY_GLYPH_INDEX_ARRAY:
        LOGI("Using glPathMemoryGlyphIndexArrayNV\n");
        break;
    case GLYPH_INDEX_RANGE:
        LOGI("Using glPathGlyphIndexRangeNV\n");
        break;
    default:
        NV_ASSERT(!"bogus NVprAPImode");
        break;
    }
}

void ShapedText::initHarfBuzz()
{
    int error_count = 0;
    const FT_F26Dot6 ptSize26Dot6 = FT_F26Dot6(round(point_size * 64));

    FT_Error err;

    /* Init freetype */
    err = FT_Init_FreeType(&ft_library);
    NV_ASSERT(!err);

    /* Load our fonts */
    for (size_t ii=0; ii<countof(font_names); ii++) {
        FT_Face ft_face;

        NV_ASSERT(ii < NUM_FONTS);
#if 0
        err = FT_New_Face(ft_library, font_names[ii], 0, &ft_face);
        NV_ASSERT(!err);
#else
        int32_t font_memory_image_length = 0;
        char *font_memory_image = NvAssetLoaderRead(font_names[ii], font_memory_image_length);
        const FT_Byte *ft2_font_memory_image = reinterpret_cast<FT_Byte*>(font_memory_image);
        NV_ASSERT(font_memory_image);
        err = FT_New_Memory_Face(ft_library, ft2_font_memory_image, font_memory_image_length, 0, &ft_face);
        NV_ASSERT(!err);
#endif
        err = FT_Set_Char_Size(ft_face, 0, ptSize26Dot6, device_hdpi, device_vdpi );
        NV_ASSERT(!err);

#ifdef PRE_glPathGlyphIndexRangeNV
        if (!glPathGlyphIndexRangeNV) {
            // XXX instead of glPathGlyphIndexRangeNV, just reserve glyphs and create them lazily
            const GLsizei num_glyphs = ft_face->num_glyphs;
            nvpr_glyph_base[ii] = glGenPathsNV(num_glyphs);
            nvpr_glyph_count[ii] = num_glyphs;
        } else
#endif // PRE_glPathGlyphIndexRangeNV
        {
            GLuint base_and_count[2];
            GLenum fontStatus = GL_FONT_TARGET_UNAVAILABLE_NV;

            switch (NVprAPImode) {
            case GLYPH_INDEX_ARRAY:
                nvpr_glyph_base[ii] = glGenPathsNV(ft_face->num_glyphs);
                nvpr_glyph_count[ii] = ft_face->num_glyphs;
                fontStatus = glPathGlyphIndexArrayNV(nvpr_glyph_base[ii],
                    GL_FILE_NAME_NV, font_names[ii], /*fontStyle */0,
                    /*firstGlyphIndex*/0, ft_face->num_glyphs,
                    path_template, EM_SCALE);
                break;
            case MEMORY_GLYPH_INDEX_ARRAY:
                {
                    nvpr_glyph_base[ii] = glGenPathsNV(ft_face->num_glyphs);
                    nvpr_glyph_count[ii] = ft_face->num_glyphs;
                    long file_size = 0;
#if 0
                    char *file_contents = read_binary_file(font_names[ii], &file_size);
#else
                    NV_ASSERT(!"use asset API");
                    char *file_contents = NULL;  // bogus
#endif
                    fontStatus = glPathMemoryGlyphIndexArrayNV(nvpr_glyph_base[ii],
                        GL_STANDARD_FONT_FORMAT_NV, file_size, file_contents, /*fontStyle */0,
                        /*firstGlyphIndex*/0, ft_face->num_glyphs,
                        path_template, EM_SCALE);
                }
                break;
            case GLYPH_INDEX_RANGE:
                fontStatus = glPathGlyphIndexRangeNV(GL_FILE_NAME_NV, 
                    font_names[ii], 0,
                    path_template, EM_SCALE, base_and_count);
                nvpr_glyph_base[ii] = base_and_count[0];
                nvpr_glyph_count[ii] = base_and_count[1];
                break;
            default:
                NV_ASSERT(!"unknown NVprAPImode");
            }
            if (fontStatus == GL_FONT_GLYPHS_AVAILABLE_NV) {
                printf("Font <%s> is available @ %d for %d glyphs\n",
                    font_names[ii], nvpr_glyph_base[ii], nvpr_glyph_count[ii]);

                GLuint ids[3] = { 0, nvpr_glyph_count[ii]-1, nvpr_glyph_count[ii] };
                GLfloat num_glyphs[3] = {-666,-666,-666};
                glGetPathMetricsNV(GL_FONT_NUM_GLYPH_INDICES_BIT_NV,
                    3, GL_UNSIGNED_INT,
                    &ids, nvpr_glyph_base[ii],
                    0, num_glyphs);
                for (size_t i=0; i<countof(ids); i++) {
                    printf("num_glyphs = %f\n", num_glyphs[i]);
                }
            } else {
                error_count++;
                // Driver should always write zeros on failure
                if (NVprAPImode == GLYPH_INDEX_ARRAY) {
                    NV_ASSERT(base_and_count[0] == 0);
                    NV_ASSERT(base_and_count[1] == 0);
                }
                printf("Font glyphs could not be populated (0x%x)\n", fontStatus);
                switch (fontStatus) {
                case GL_FONT_TARGET_UNAVAILABLE_NV:
                    printf("> Font target unavailable\n");
                    break;
                case GL_FONT_UNAVAILABLE_NV:
                    printf("> Font unavailable\n");
                    break;
                case GL_FONT_CORRUPT_NV:
                    printf("> Font corrupt\n");
                    break;
                case GL_OUT_OF_MEMORY:
                    printf("> Out of memory\n");
                    break;
                case GL_INVALID_VALUE:
                    printf("> Invalid value for glPathGlyphIndexRangeNV (should not happen)\n");
                    break;
                case GL_INVALID_ENUM:
                    printf("> Invalid enum for glPathGlyphIndexRangeNV (should not happen)\n");
                    break;
                default:
                    printf("> UNKNOWN reason (should not happen)\n");
                    break;
                }
            }
        }

        /* Get our harfbuzz font/face structs */
        hb_ft_font[ii] = hb_ft_font_create(ft_face, hb_destroy_func_t(FT_Done_Face));
    }

    if (error_count > 0) {
        printf("NVpr font setup failed with %d errors\n", error_count);
#ifdef _WIN32
        printf("\nCould it be you don't have freetype6.dll in your current directory?\n");
#endif
        exit(1);
    }

#ifdef _WIN32
    if (glPathGlyphIndexRangeNV) {
        const GLbitfield styleMask = 0;
        const char *font_name = "Arial";
        GLuint base_and_count[2];
        GLenum fontStatus;
        fontStatus = glPathGlyphIndexRangeNV(GL_SYSTEM_FONT_NAME_NV, 
            font_name, styleMask,
            path_template, EM_SCALE, base_and_count);
        GLuint arial_base = base_and_count[0];
        GLuint arial_count = base_and_count[1];
        printf("arial_base = %d, count = %d\n", arial_base, arial_count);
        font_name = "Wingdings";
        fontStatus = glPathGlyphIndexRangeNV(GL_SYSTEM_FONT_NAME_NV, 
            font_name, styleMask,
            path_template, EM_SCALE, base_and_count);
        GLuint wingding_base = base_and_count[0];
        GLuint wingding_count = base_and_count[1];
        printf("wingding_base = %d, count = %d\n", wingding_base, wingding_count);
        font_name = "Arial Unicode MS";
        fontStatus = glPathGlyphIndexRangeNV(GL_SYSTEM_FONT_NAME_NV, 
            font_name, styleMask,
            path_template, EM_SCALE, base_and_count);
        GLuint arialuni_base = base_and_count[0];
        GLuint arialuni_count = base_and_count[1];
        printf("arialuni_base = %d, count = %d\n", arialuni_base, arialuni_count);
    }
#endif
}

void ShapedText::updatePointSize(float new_size)
{

    point_size = new_size;
    if (latched_point_size != new_size) {
        const FT_F26Dot6 ptSize26Dot6 = FT_F26Dot6(round(point_size * 64));
        for (size_t ii=0; ii<countof(font_names); ii++) {
            FT_Error err = FT_Set_Char_Size(hb_ft_font_get_face(hb_ft_font[ii]),
                0, ptSize26Dot6, device_hdpi, device_vdpi );
            NV_ASSERT(!err);
            err = err;  // so used
        }
        latched_point_size = new_size;
        loaded_scene = ~0U;  // invalid value
    }
}

void ShapedText::shapeExampleText(int text_count, const TextSample text[], float width)
{
    float scale = point_size/EM_SCALE;
    float inv_scale = 1/scale;
    float x = inv_scale*0;
    float y = -inv_scale*point_size;

    shaped_text.clear();

    /* Create a buffer for harfbuzz to use */
    hb_buffer_t *buf = hb_buffer_create();
    //alternatively you can use hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
    hb_buffer_set_unicode_funcs(buf, hb_ucdn_make_unicode_funcs());

    for (int i=0; i < text_count; ++i) {
        hb_buffer_clear_contents(buf);

        hb_buffer_set_direction(buf, text[i].text_system->direction); /* or LTR */
        hb_buffer_set_script(buf, text[i].text_system->script); /* see hb-unicode.h */
        const char *language = text[i].text_system->language;
        hb_buffer_set_language(buf, hb_language_from_string(language, int(strlen(language))));

        /* Layout the text */
        int text_length(int(strlen(text[i].text)));
        hb_buffer_add_utf8(buf, text[i].text, text_length, 0, text_length);
        FontType font = text[i].text_system->font;
        hb_shape(hb_ft_font[font], buf, NULL, 0);

        /* Hand the layout to cairo to render */
        unsigned int glyph_count;
        hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
        hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

        int string_width_in_pixels = 0;
        for (unsigned int j=0; j < glyph_count; ++j) {
            string_width_in_pixels += glyph_pos[j].x_advance/64;
        }

        switch (text[i].text_system->direction) {
        case HB_DIRECTION_LTR:
            x = inv_scale*20; /* left justify */
            break;
        case HB_DIRECTION_RTL:
            x = inv_scale*(width - string_width_in_pixels -20); /* right justify */
            break;
        case HB_DIRECTION_TTB:
            x = inv_scale*(width/2 - string_width_in_pixels/2);  /* center */
            break;
        default:
            NV_ASSERT(!"unknown direction");
        }

        NVprShapedGlyphs a(nvpr_glyph_base[font], scale);

        for (unsigned int j=0; j < glyph_count; ++j) {
            GLuint path = nvpr_glyph_base[font] + glyph_info[j].codepoint;
#ifdef PRE_glPathGlyphIndexRangeNV
            if (!glPathGlyphIndexRangeNV) {
                generateGlyph(path, hb_ft_font_get_face(hb_ft_font[font]), glyph_info[j].codepoint);
            } else {
                // glPathGlyphIndexRangeNV has already generated the glyph.
                NV_ASSERT(glIsPathNV(path));
            }
#endif // PRE_glPathGlyphIndexRangeNV
            a.addGlyph(x + inv_scale*glyph_pos[j].x_offset/64,
                y + inv_scale*glyph_pos[j].y_offset/64,
                glyph_info[j].codepoint);
            x += inv_scale*glyph_pos[j].x_advance/64;
            y += inv_scale*glyph_pos[j].y_advance/64;  
        }

        shaped_text.push_back(a);

        y += -inv_scale*(point_size*1.5f);
    }
    hb_buffer_destroy(buf);
}

void ShapedText::drawShapedExampleText()
{
    const GLuint stencil_write_mask = ~0U;
    GLuint color = 0;

    if (es_context) {
      color = glGetUniformLocation(program, "color");
      glUseProgram(program);
    }

    for (size_t i=0; i<shaped_text.size(); i++) {
        if (stroking) {
            if (es_context) {
              glUniform3f(color, 0, 0, 0); // black
            } else {
              glColor3f(0,0,0); // black
            }
            shaped_text[i].drawStroked(stencil_write_mask);
        }
        if (filling) {
          if (es_context) {
            glUniform3f(color, 1, 1, 0); // yellow
          }
          else {
            glColor3f(1, 1, 0); // yellow
          }
            shaped_text[i].drawFilled(stencil_write_mask);
        }
    }
    if (es_context) {
      glUseProgram(0);
    }
}

void ShapedText::shutdownHarfBuzz()
{
    /* Cleanup */
    for (int i=0; i < NUM_FONTS; ++i) {
        // This will implicitly call FT_Done_Face on each hb_ft_font element's FT_Face
        // because FT_Done_Face is each hb_font_t's hb_destroy_func_t callback
        hb_font_destroy(hb_ft_font[i]);

#if !defined(PRE_glPathGlyphIndexRangeNV)
        NV_ASSERT(glIsPathNV(nvpr_glyph_base[i]));
        NV_ASSERT(glIsPathNV(nvpr_glyph_base[i]+nvpr_glyph_count[i]-1));
        glDeletePathsNV(nvpr_glyph_base[i], nvpr_glyph_count[i]);
        NV_ASSERT(!glIsPathNV(nvpr_glyph_base[i]));
        NV_ASSERT(!glIsPathNV(nvpr_glyph_base[i]+nvpr_glyph_count[i]-1));
#endif
    }

    FT_Done_FreeType(ft_library);
}

void ShapedText::setBackground()
{
    float r, g, b, a;

    switch (background) {
    default:
    case 0:
        r = g = b = 0.0;
        break;
    case 1:
        r = g = b = 1.0;
        break;
    case 2:
        r = 0.1;
        g = 0.3;
        b = 0.6;
        break;
    case 3:
        r = g = b = 0.5;
        break;
    }
    if (sRGB_capable) {
        r = convertSRGBColorComponentToLinearf(r);
        g = convertSRGBColorComponentToLinearf(g);
        b = convertSRGBColorComponentToLinearf(b);
    }
    a = 1.0;
    glClearColor(r,g,b,a);
}

ShapedText::ShapedText()
    : stroking(true)
    , filling(true)
    , NVprAPImode(GLYPH_INDEX_ARRAY)
    , current_scene(3)
    , point_size(50)  // 50 point
    , background(2) // nice blue
    , path_template(0)
    , render_state(NvRedrawMode::UNBOUNDED)
    , program(0)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();

    // Should be the opposite of stroking and filling.
    stroking_state = !stroking;
    filling_state = !filling;
    loaded_scene = current_scene+1;
    latched_point_size = point_size+1;

    continuousRendering(false);	
}

ShapedText::~ShapedText()
{
    LOGI("ShapedText: destroyed\n");
}

void ShapedText::continuousRendering(bool keep_rendering)
{
	if (isTestMode())
		keep_rendering = true;

	// XXX ugh without render_state variable, calling setRedrawMode "too much" casues DVM crashes
    if (keep_rendering) {
        if (render_state != NvRedrawMode::UNBOUNDED) {
			if (getPlatformContext()) {
				getPlatformContext()->setRedrawMode(NvRedrawMode::UNBOUNDED);
				render_state = NvRedrawMode::UNBOUNDED;
			}
        }
    } else {
		if (render_state != NvRedrawMode::ON_DEMAND) {
			if (getPlatformContext()) {
				getPlatformContext()->setRedrawMode(NvRedrawMode::ON_DEMAND);
				render_state = NvRedrawMode::ON_DEMAND;
			}
		}
    }
}

void ShapedText::postRedisplay()
{
    getPlatformContext()->requestRedraw();	
}

void ShapedText::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 8; 
    config.msaaSamples = 8;
    if (es_context) {
      config.apiVer = NvGLAPIVersionES3_1();
    }
    else {
      config.apiVer = NvGLAPIVersionGL4();
    }
}

const NvTweakEnum<uint32_t> sceneNames[] = {
    { "Writing changes", 3 },
    { "Some text", 0 },
    { "Cat words",    1 },
    { "Sky poetry",  2 },
};

enum UIReactionIDs {   
    REACT_RESET         = 0x10000001,
};

NvUIEventResponse ShapedText::handleReaction(const NvUIReaction &react)
{
    switch (react.code) {
    case REACT_RESET:
        initViewMatrix();
        point_size = 50;
        mTweakBar->syncValues();
        postRedisplay();	
        return nvuiEventHandled;
    }
    return nvuiEventNotHandled;
}

void ShapedText::initUI() {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        NvTweakVarBase *var;

        mTweakBar->addPadding(2);
        var = mTweakBar->addValue("Filling", filling);
        addTweakKeyBind(var, NvKey::K_F);
        addTweakButtonBind(var, NvGamepad::BUTTON_LEFT_SHOULDER);

        mTweakBar->addPadding(2);
        var = mTweakBar->addValue("Stroking", stroking);
        addTweakKeyBind(var, NvKey::K_S);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER);
        
        mTweakBar->addPadding(2);
        mTweakBar->addEnum("Text sample", current_scene, sceneNames, TWEAKENUM_ARRAYSIZE(sceneNames), 0x0);

        mTweakBar->addPadding(2);
        const float minimum_point_size = 6;
        const float maximum_point_size = 80;
        const float step = 0.5f;
        var = mTweakBar->addValue("Point size", point_size,
            minimum_point_size, maximum_point_size, step);
        addTweakKeyBind(var, NvKey::K_COMMA, NvKey::K_PERIOD);
        addTweakButtonBind(var, NvGamepad::BUTTON_DPAD_RIGHT, NvGamepad::BUTTON_DPAD_LEFT);

        mTweakBar->addPadding(4);
        var = mTweakBar->addButton("Reset", REACT_RESET);
        addTweakKeyBind(var, NvKey::K_R);
        addTweakButtonBind(var, NvGamepad::BUTTON_X);
    }
}

void ShapedText::initGraphics()
{
    if (hasFramebufferSRGB) {
        glGetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &sRGB_capable);
    }

    setBackground();

    // Create a null path object to use as a parameter template for creating fonts.
    path_template = glGenPathsNV(1);
    glPathCommandsNV(path_template, 0, NULL, 0, GL_FLOAT, NULL);
    glPathParameterfNV(path_template, GL_PATH_STROKE_WIDTH_NV, 0.1*emScale);  // 10% of emScale
    glPathParameteriNV(path_template, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);

    glStencilFunc(GL_NOTEQUAL, 0, ~0U);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
}

float canvas_width = 800;

void ShapedText::loadScene()
{
    switch (current_scene) {
    case 0:
        shapeExampleText(countof(some_text), some_text, canvas_width);
        break;
    case 1:
        shapeExampleText(countof(cat_text), cat_text, canvas_width);
        break;
    case 2:
        shapeExampleText(countof(cloud_text), cloud_text, canvas_width);
        break;
    case 3:
        shapeExampleText(countof(change_text), change_text, canvas_width);
        break;
    }
}

void ShapedText::setScene(int scene)
{
    current_scene = scene;
    loadScene();
}

void ShapedText::initProgram()
{
    const GLchar *source = "#extension GL_ARB_separate_shader_objects : enable\n"
      "precision highp float;\n"
      "uniform vec3 color;\n"
      "void main() {\n"
      "    gl_FragColor = vec4(color, 1.0);\n"
      "}\n";
    program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &source);
}

void ShapedText::initRendering()
{
    if(!requireExtension("GL_NV_path_rendering")) return;
    if (!es_context && !requireExtension("GL_EXT_direct_state_access")) return;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    

    NvAssetLoaderAddSearchPath("nvpr/ShapedTextES");

    initViewMatrix();
    initGraphics();
    if (es_context) {
      initProgram();
    }
    initHarfBuzz();
    loadScene();
    reportMode();

    CHECK_GL_ERROR();
}

void ShapedText::shutdownRendering()
{
  if (es_context) {
      glDeleteProgram(program);
  }
}

void ShapedText::initViewMatrix()
{
    view = float3x3(1,0,canvas_width * 0.4f,
                    0,1,canvas_width * 0.15f,
                    0,0,1);
}

void ShapedText::display()
{
    if (sRGB_capable) {
        glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST); {
        MatrixLoadToGL(view);
        drawShapedExampleText();
    } glDisable(GL_STENCIL_TEST);
    if (sRGB_capable) {
        glDisable(GL_FRAMEBUFFER_SRGB_EXT);
    }
    CHECK_GL_ERROR();
}

void ShapedText::draw()
{
    updatePointSize(point_size);
    if (current_scene != loaded_scene) {
        loadScene();
        loaded_scene = current_scene;
    }

    updateView();

    display();
}

NvAppBase* NvAppFactory() {
    return new ShapedText();
}
