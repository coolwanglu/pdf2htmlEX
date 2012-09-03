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

#include <fontforge.h>
#include <baseviews.h>

#include "ff.h"

SplineFont * cur_font = NULL;

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

void ff_init(void)
{
    InitSimpleStuff();
    if ( default_encoding==NULL )
        default_encoding=FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding==NULL )
        default_encoding=&custom;	/* In case iconv is broken */
}
void ff_load_font(const char * filename)
{
    char * _filename = strcopy(filename);
    cur_font = LoadSplineFont(_filename, 1);
    free(_filename);

    if(!cur_font)
        err("Cannot load font %s\n", filename);

    if(!cur_font->fv)
        FVAppend(_FontViewCreate(cur_font));
}

void ff_load_encoding(const char * filename, const char * encname)
{
    char * _filename = strcopy(filename);
    char * _encname = strcopy(encname);
    ParseEncodingFile(_filename, _encname);
    free(_encname);
    free(_filename);
}

void ff_reencode(const char * encname, int force)
{
    Encoding * enc = FindOrMakeEncoding(encname);
    if(!enc)
        err("Unknown encoding %s\n", encname);

    if(force)
    {
        SFForceEncoding(cur_font, cur_font->fv->map, enc);
    }
    else
    {
        EncMapFree(cur_font->fv->map);
        cur_font->fv->map= EncMapFromEncoding(cur_font, enc);
    }

    SFReplaceEncodingBDFProps(cur_font, cur_font->fv->map);
}

void ff_cidflatten(void)
{
    printf("cid flatten\n");

    if(!cur_font->cidmaster)
        err("Cannot flatten a non-CID font");
    SFFlatten(cur_font->cidmaster);
}

void ff_save(const char * filename)
{
    char * _filename = strcopy(filename);
    char * _ = strcopy("");

    int r = GenerateScript(cur_font, _filename
            , _, -1, -1, NULL, NULL, cur_font->fv->map, NULL, ly_fore);
    
    free(_);
    free(_filename);

    if(!r)
        err("Cannot save font to %s\n", filename);
}

void ff_close(void)
{
    FontViewClose(cur_font->fv);
    cur_font = NULL;
}

int ff_get_em_size(void)
{
    return (cur_font->pfminfo.os2_typoascent - cur_font->pfminfo.os2_typodescent);
}

int ff_get_max_ascent(void)
{
    return max(cur_font->pfminfo.os2_winascent,
            max(cur_font->pfminfo.os2_typoascent,
                cur_font->pfminfo.hhead_ascent));
}

int ff_get_max_descent(void)
{
    return max(cur_font->pfminfo.os2_windescent,
            max(-cur_font->pfminfo.os2_typodescent,
                -cur_font->pfminfo.hhead_descent));
}

void ff_set_ascent(int a)
{
    cur_font->pfminfo.os2_winascent = a;
    cur_font->pfminfo.os2_typoascent = a;
    cur_font->pfminfo.hhead_ascent = a;
}

void ff_set_descent(int d)
{
    cur_font->pfminfo.os2_windescent = d;
    cur_font->pfminfo.os2_typodescent = -d;
    cur_font->pfminfo.hhead_descent = -d;
}

