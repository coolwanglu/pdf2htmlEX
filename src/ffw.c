/*
 * ffw.c: Fontforge wrapper
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

#include <fontforge.h>
#include <baseviews.h>

#include "ffw.h"

static FontViewBase * cur_fv = NULL;
static Encoding * original_enc = NULL;
static Encoding * enc_head = NULL;

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

static void dumb_logwarning(const char * format, ...)
{
}

static void dumb_post_error(const char * title, const char * error, ...)
{
}

void ffw_init(int debug)
{
    InitSimpleStuff();
    if ( default_encoding==NULL )
        default_encoding=FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding==NULL )
        default_encoding=&custom;	/* In case iconv is broken */

    if(!debug)
    {
        //disable error output of Fontforge
        ui_interface->logwarning = &dumb_logwarning;
        ui_interface->post_error = &dumb_post_error;
    }

    original_enc = FindOrMakeEncoding("original");
}

void ffw_fin(void)
{
    while(enc_head)
    {
        Encoding * next = enc_head->next;
        free(enc_head->enc_name);
        free(enc_head->unicode);
        if(enc_head->psnames)
        {
            int i;
            for(i = 0; i < enc_head->char_cnt; ++i)
                free(enc_head->psnames[i]);
            free(enc_head->psnames);
        }
        free(enc_head);
        enc_head = next;
    }
}

void ffw_load_font(const char * filename)
{
    char * _filename = strcopy(filename);
    SplineFont * font = LoadSplineFont(_filename, 1);

    free(_filename);

    if(!font)
        err("Cannot load font %s\n", filename);

    if(!font->fv)
        FVAppend(_FontViewCreate(font));

    assert(font->fv);

    cur_fv = font->fv;
}

static void ffw_do_reencode(Encoding * encoding, int force)
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

void ffw_reencode_glyph_order(void)
{
    ffw_do_reencode(original_enc, 0);
}

void ffw_reencode(const char * encname, int force)
{
    Encoding * enc = FindOrMakeEncoding(encname);
    if(!enc)
        err("Unknown encoding %s\n", encname);

    ffw_do_reencode(enc, force);
}

void ffw_reencode_raw(int32 * mapping, int mapping_len, int force)
{
    Encoding * enc = calloc(1, sizeof(Encoding));
    enc->only_1byte = enc->has_1byte = true;
    enc->char_cnt = mapping_len;
    enc->unicode = (int32_t*)malloc(mapping_len * sizeof(int32_t));
    memcpy(enc->unicode, mapping, mapping_len * sizeof(int32_t));
    enc->enc_name = strcopy("");

    enc->next = enc_head;
    enc_head = enc;

    ffw_do_reencode(enc, force);
}

void ffw_reencode_raw2(char ** mapping, int mapping_len, int force)
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

    enc->next = enc_head;
    enc_head = enc;

    ffw_do_reencode(enc, force);
}

void ffw_cidflatten(void)
{
    if(!cur_fv->sf->cidmaster)
        err("Cannot flatten a non-CID font");
    SFFlatten(cur_fv->sf->cidmaster);
}

void ffw_save(const char * filename)
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

void ffw_close(void)
{
    FontViewClose(cur_fv);
    cur_fv = NULL;
}

void ffw_metric(double * ascent, double * descent)
{
    DBounds bb;
    SplineFont * sf = cur_fv->sf;
    SplineFontFindBounds(sf, &bb);
    struct pfminfo * info = &sf->pfminfo;

    //debug
    printf("bb %lf %lf\n", bb.maxy, bb.miny);
    printf("_ %d %d\n", sf->ascent, sf->descent);
    printf("win %d %d\n", info->os2_winascent, info->os2_windescent);
    printf("%d %d\n", info->winascent_add, info->windescent_add);
    printf("typo %d %d\n", info->os2_typoascent, info->os2_typodescent);
    printf("%d %d\n", info->typoascent_add, info->typodescent_add);
    printf("hhead %d %d\n", info->hhead_ascent, info->hhead_descent);
    printf("%d %d\n", info->hheadascent_add, info->hheaddescent_add);

    int em = sf->ascent + sf->descent;

    if (em > 0)
    {
        *ascent = ((double)bb.maxy) / em;
        *descent = ((double)bb.miny) / em;
    }
    else
    {
        *ascent = *descent = 0;
    }

    info->os2_winascent = *ascent;
    info->os2_typoascent = *ascent;
    info->hhead_ascent = *ascent;
    info->winascent_add = 0;
    info->typoascent_add = 0;
    info->hheadascent_add = 0;

    info->os2_windescent = -*descent;
    info->os2_typodescent = *descent;
    info->hhead_descent = *descent;
    info->windescent_add = 0;
    info->typodescent_add = 0;
    info->hheaddescent_add = 0;

    info->pfmset = 1;
    sf->changed = true;
}

