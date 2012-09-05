/*
 * ff.c
 *
 * Processing fonts using Fontforge
 *
 * by WangLu
 * 2012.09.03
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <fontforge/config.h>
#include <fontforge.h>
#include <baseviews.h>

#include "ff.h"

static FontViewBase * cur_fv = NULL;
static Encoding * original_enc = NULL;

static void err(const char * format, ...)
{
    va_list al;
    va_start(al, format);
    vfprintf(stderr, format, al);
    va_end(al);
    exit(-1);
}
static char * strcopy(const char * str)
{
    if(str == NULL) return NULL;

    char * _ = strdup(str);
    if(!_)
        err("Not enough memory");
    return _;
}

static int max(int a, int b)
{
    return (a>b) ? a : b;
}

static void dummy(const char * format, ...)
{
    va_list al;
    va_start(al, format);
    va_end(al);
}

void ff_init(void)
{
    InitSimpleStuff();
    if ( default_encoding==NULL )
        default_encoding=FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding==NULL )
        default_encoding=&custom;	/* In case iconv is broken */

    //disable error output of Fontforge
    ui_interface->logwarning = &dummy;

    original_enc = FindOrMakeEncoding("original");
}
void ff_load_font(const char * filename)
{
    char * _filename = strcopy(filename);
    SplineFont * font = LoadSplineFont(_filename, 1);
    free(_filename);

    if(!font)
        err("Cannot load font %s\n", filename);

    if(!font->fv)
        FVAppend(_FontViewCreate(font));

    cur_fv = font->fv;
}

static void ff_do_reencode(Encoding * encoding, int force)
{
    assert(encoding);

    if(force)
    {
        SFForceEncoding(cur_fv->sf, cur_fv->map, encoding);
    }
    else
    {
        EncMapFree(cur_fv->map);
        cur_fv->map = EncMapFromEncoding(cur_fv->sf, encoding);
    }
    if(cur_fv->normal)
    {
        EncMapFree(cur_fv->normal);
        cur_fv->normal = NULL;
    }

    SFReplaceEncodingBDFProps(cur_fv->sf, cur_fv->map);
}

void ff_reencode_glyph_order(void)
{
    ff_do_reencode(original_enc, 0);
}

void ff_reencode(const char * encname, int force)
{
    Encoding * enc = FindOrMakeEncoding(encname);
    if(!enc)
        err("Unknown encoding %s\n", encname);

    ff_do_reencode(enc, force);
}

void ff_reencode_raw(int32 * mapping, int mapping_len, int force)
{
    Encoding * enc = calloc(1, sizeof(Encoding));
    enc->only_1byte = enc->has_1byte = true;
    enc->char_cnt = mapping_len;
    enc->unicode = (int32_t*)malloc(mapping_len * sizeof(int32_t));
    memcpy(enc->unicode, mapping, mapping_len * sizeof(int32_t));
    enc->enc_name = strcopy("");

    ff_do_reencode(enc, force);
}

void ff_reencode_raw2(char ** mapping, int mapping_len, int force)
{
    Encoding * enc = calloc(1, sizeof(Encoding));
    enc->enc_name = strcopy("");
    enc->char_cnt = mapping_len;
    enc->unicode = (int32_t*)malloc(mapping_len * sizeof(int32_t));
    enc->psnames = (char**)calloc(mapping_len, sizeof(char*));
    int i;
    for(i = 0; i < mapping_len; ++i)
    {
        if(mapping[i])
        {
            enc->unicode[i] = UniFromName(mapping[i], ui_none, &custom);
            enc->psnames[i] = strcopy(mapping[i]);
        }
        else
        {
            enc->unicode[i] = -1;
        }
    }

    ff_do_reencode(enc, force);
}

void ff_cidflatten(void)
{
    if(!cur_fv->sf->cidmaster)
        err("Cannot flatten a non-CID font");
    SFFlatten(cur_fv->sf->cidmaster);
}

void ff_save(const char * filename)
{
    char * _filename = strcopy(filename);
    char * _ = strcopy("");

    int r = GenerateScript(cur_fv->sf, _filename
            , _, -1, -1, NULL, NULL, cur_fv->map, NULL, ly_fore);
    
    free(_);
    free(_filename);

    if(!r)
        err("Cannot save font to %s\n", filename);
}

void ff_close(void)
{
    FontViewClose(cur_fv);
    cur_fv = NULL;
}

int ff_get_em_size(void)
{
    return (cur_fv->sf->pfminfo.os2_typoascent - cur_fv->sf->pfminfo.os2_typodescent);
}

int ff_get_max_ascent(void)
{
    return max(cur_fv->sf->pfminfo.os2_winascent,
            max(cur_fv->sf->pfminfo.os2_typoascent,
                cur_fv->sf->pfminfo.hhead_ascent));
}

int ff_get_max_descent(void)
{
    return max(cur_fv->sf->pfminfo.os2_windescent,
            max(-cur_fv->sf->pfminfo.os2_typodescent,
                -cur_fv->sf->pfminfo.hhead_descent));
}

void ff_set_ascent(int a)
{
    cur_fv->sf->pfminfo.os2_winascent = a;
    cur_fv->sf->pfminfo.os2_typoascent = a;
    cur_fv->sf->pfminfo.hhead_ascent = a;
}

void ff_set_descent(int d)
{
    cur_fv->sf->pfminfo.os2_windescent = d;
    cur_fv->sf->pfminfo.os2_typodescent = -d;
    cur_fv->sf->pfminfo.hhead_descent = -d;
}

