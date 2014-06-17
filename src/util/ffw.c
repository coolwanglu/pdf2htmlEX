/*
 * ffw.c: Fontforge wrapper
 *
 * Processing fonts using Fontforge
 *
 * Copyright (C) 2012-2014 Lu Wang <coolwanglu@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

#include <fontforge.h>
#include <baseviews.h>

#include "ffw.h"

static real EPS=1e-6;

static inline int min(int a, int b)
{
    return (a<b)?a:b;
}

static FontViewBase * cur_fv = NULL;
static Encoding * original_enc = NULL;
static Encoding * unicodefull_enc = NULL;
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

static void dumb_logwarning(const char * format, ...) { }

static void dumb_post_error(const char * title, const char * error, ...) { }

void ffw_init(int debug)
{
    InitSimpleStuff();
    if ( default_encoding==NULL )
        default_encoding=FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding==NULL )
        default_encoding=&custom; /* In case iconv is broken */

    if(!debug)
    {
        //disable error output of Fontforge
        ui_interface->logwarning = &dumb_logwarning;
        ui_interface->post_error = &dumb_post_error;
    }

    original_enc = FindOrMakeEncoding("original");
    unicodefull_enc = FindOrMakeEncoding("UnicodeFull");

    {
        Val v;
        v.type = v_int;
        v.u.ival = 1;
        SetPrefs("DetectDiagonalStems", &v, NULL);
    }
}

void ffw_finalize(void)
{
    while(enc_head)
    {
        Encoding * next = enc_head->next;
        free((void*)enc_head->enc_name);
        free(enc_head->unicode);
        if(enc_head->psnames)
        {
            int i;
            for(i = 0; i < enc_head->char_cnt; ++i)
                free((void*)enc_head->psnames[i]);
            free(enc_head->psnames);
        }
        free(enc_head);
        enc_head = next;
    }
}

long ffw_get_version(void)
{
    return FONTFORGE_VERSIONDATE_RAW;
}

void ffw_new_font()
{
    assert((cur_fv == NULL) && "Previous font is not destroyed");
    cur_fv = FVAppend(_FontViewCreate(SplineFontNew()));
}

void ffw_load_font(const char * filename)
{
    assert((cur_fv == NULL) && "Previous font is not destroyed");

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

/*
 * Fight again dirty stuffs
 */
void ffw_prepare_font(void)
{
    memset(cur_fv->selected, 1, cur_fv->map->enccount);
    // remove kern
    FVRemoveKerns(cur_fv);
    FVRemoveVKerns(cur_fv);

    /*
     * Remove Alternate Unicodes
     * We never use them because we will do a force encoding
     */
    int i;
    SplineFont * sf = cur_fv->sf;
    for(i = 0; i < sf->glyphcnt; ++i)
    {
        SplineChar * sc = sf->glyphs[i];
        if(sc)
        {
            struct altuni * p = sc->altuni;
            if(p)
            {
                AltUniFree(p);
                sc->altuni = NULL;
            }
        }
    }
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

    free(cur_fv->selected);
    cur_fv->selected = calloc(cur_fv->map->enccount, sizeof(char));
}

void ffw_reencode_glyph_order(void)
{
    ffw_do_reencode(original_enc, 0);
}

void ffw_reencode_unicode_full(void)
{
    ffw_do_reencode(unicodefull_enc, 0);
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

    int len = (mapping_len < 256) ? 256 : mapping_len;
    enc->char_cnt = len;
    enc->unicode = (int32_t*)malloc(len * sizeof(int32_t));
    memcpy(enc->unicode, mapping, mapping_len * sizeof(int32_t));
    if(mapping_len < 256)
    {
        int i;
        for(i = mapping_len; i < 256; ++i)
            enc->unicode[i] = -1;
    }

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
    enc->psnames = (const char**)calloc(mapping_len, sizeof(char*));
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

/*
 * There is no check if a glyph with the same unicode exists!
 * TODO: let FontForge fill in the standard glyph name <- or maybe this might cause collision?
 */
void ffw_add_empty_char(int32_t unicode, int width)
{
    SplineChar * sc = SFMakeChar(cur_fv->sf, cur_fv->map, cur_fv->map->enccount);
    char buffer[400];
    SCSetMetaData(sc,
        strcopy(StdGlyphName(buffer, unicode,
                cur_fv->sf->uni_interp, cur_fv->sf->for_new_glyphs)),
        unicode, sc->comment);
    SCSynchronizeWidth(sc, width, sc->width, cur_fv);
}

int ffw_get_em_size(void)
{
    return cur_fv->sf->ascent + cur_fv->sf->descent;
}

void ffw_fix_metric()
{
    double ascent, descent;
    ffw_get_metric(&ascent, &descent);
    ffw_set_metric(ascent, descent);
}

void ffw_get_metric(double * ascent, double * descent)
{
    SplineFont * sf = cur_fv->sf;

    DBounds bb;
    SplineFontFindBounds(sf, &bb);

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
}

void ffw_set_metric(double ascent, double descent)
{
    SplineFont * sf = cur_fv->sf;
    struct pfminfo * info = &sf->pfminfo;

    SFDefaultOS2Info(info, sf, sf->fontname);
    info->pfmset = 1;
    sf->changed = 1;

    int em = sf->ascent + sf->descent;
    int a = floor(ascent * em + 0.5);
    int d = floor(descent * em + 0.5);

    if(a < 0) a = 0;
    if(d > 0) d = 0;

    /*
    sf->ascent = min(a, em);
    sf->descent = em - bb.maxy;
    */

    /*
     * The embedded fonts are likely to have inconsistent values for the 3 sets of ascent/descent
     * PDF viewers don't care, since they don't even use these values
     * But have to unify them, for different browsers on different platforms
     * Things may become easier when there are CSS rules for baseline-based positioning.
     */
    info->os2_winascent = a;
    info->os2_typoascent = a;
    info->hhead_ascent = a;
    info->winascent_add = 0;
    info->typoascent_add = 0;
    info->hheadascent_add = 0;

    info->os2_windescent = -d;
    info->os2_typodescent = d;
    info->hhead_descent = d;
    info->windescent_add = 0;
    info->typodescent_add = 0;
    info->hheaddescent_add = 0;

    info->os2_typolinegap = 0;
    info->linegap = 0;
}

/*
 * TODO:bitmap, reference have not been considered in this function
 */
void ffw_set_widths(int * width_list, int mapping_len,
        int stretch_narrow, int squeeze_wide)
{
    SplineFont * sf = cur_fv->sf;

    if(sf->onlybitmaps
            && cur_fv->active_bitmap != NULL
            && sf->bitmaps != NULL)
    {
        printf("TODO: width vs bitmap\n");
    }

    EncMap * map = cur_fv->map;
    int i;
    int imax = min(mapping_len, map->enccount);
    for(i = 0; i < imax; ++i)
    {
        /*
         * Don't mess with it if the glyphs is not used.
         */
        if(width_list[i] == -1)
        {
            continue;
        }

        int j = map->map[i];
        if(j == -1) continue;

        SplineChar * sc = sf->glyphs[j];
        if(sc == NULL)
        {
            sc = SFMakeChar(cur_fv->sf, cur_fv->map, j);
        }
        else if(((sc->width > EPS)
                && (((sc->width > width_list[i] + EPS) && (squeeze_wide))
                    || ((sc->width < width_list[i] - EPS) && (stretch_narrow)))))
        {
            real transform[6];
            transform[0] = ((double)width_list[i]) / (sc->width);
            transform[3] = 1.0;
            transform[1] = transform[2] = transform[4] = transform[5] = 0;
            FVTrans(cur_fv, sc, transform, NULL, fvt_alllayers | fvt_dontmovewidth);
        }

        SCSynchronizeWidth(sc, width_list[i], sc->width, cur_fv);
    }
}

void ffw_import_svg_glyph(int code, const char * filename, double ox, double oy, double width)
{
    int enc = SFFindSlot(cur_fv->sf, cur_fv->map, code, "");
    if(enc == -1)
        return;

    SplineChar * sc = SFMakeChar(cur_fv->sf, cur_fv->map, enc);

    memset(cur_fv->selected, 0, cur_fv->map->enccount);
    cur_fv->selected[enc] = 1;
    int ok = FVImportImages(cur_fv, (char*)filename, fv_svg, 0, -1);
    if(!ok)
        err("Import SVG glyph failed");

    // correct origin and width
    {
        int a = cur_fv->sf->ascent;
        int d = cur_fv->sf->descent;
        real transform[6];
        transform[0] = 1.0;
        transform[3] = 1.0;
        transform[1] = transform[2] = 0.0;
        transform[4] = -ox * (a+d);
        transform[5] = -oy * (a+d) + d;
        FVTrans(cur_fv, sc, transform, NULL, fvt_alllayers | fvt_dontmovewidth);

        SCSynchronizeWidth(sc, floor(width * (a+d) + 0.5), sc->width, cur_fv);
    }
}

void ffw_auto_hint(void)
{
    // convert to quadratic
    if(!(cur_fv->sf->layers[ly_fore].order2))
    {
        SFCloseAllInstrs(cur_fv->sf);
        SFConvertToOrder2(cur_fv->sf);
    }
    memset(cur_fv->selected, 1, cur_fv->map->enccount);
    FVAutoHint(cur_fv);
    FVAutoInstr(cur_fv);
}

void ffw_override_fstype(void)
{
    *(int16 *)(&cur_fv->sf->pfminfo.fstype) = 0;
    cur_fv->sf->pfminfo.pfmset = true;
    cur_fv->sf->changed = true;
}
