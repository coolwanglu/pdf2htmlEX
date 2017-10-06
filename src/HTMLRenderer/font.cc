/*
 * font.cc
 *
 * Font processing
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <iostream>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <unordered_set>

#include <GlobalParams.h>
#include <fofi/FoFiTrueType.h>
#include <CharCodeToUnicode.h>

#include "Param.h"
#include "HTMLRenderer.h"
#include "Base64Stream.h"

#include "pdf2htmlEX-config.h"

#include "util/namespace.h"
#include "util/math.h"
#include "util/misc.h"
#include "util/ffw.h"
#include "util/path.h"
#include "util/unicode.h"
#include "util/css_const.h"

#if ENABLE_SVG
#include <cairo.h>
#include <cairo-ft.h>
#include <cairo-svg.h>
#include "CairoFontEngine.h"
#include "CairoOutputDev.h"
#include <Gfx.h>
#endif

namespace pdf2htmlEX {

using std::min;
using std::unordered_set;
using std::cerr;
using std::endl;

string HTMLRenderer::dump_embedded_font (GfxFont * font, FontInfo & info)
{
    if(info.is_type3)
        return dump_type3_font(font, info);

    Object obj, obj1, obj2;
    Object font_obj, font_obj2, fontdesc_obj;
    string suffix;
    string filepath;

    long long fn_id = info.id;

    try
    {
        // inspired by mupdf 
        string subtype;

        auto * id = font->getID();

        Object ref_obj(id->num, id->gen);
        //ref_obj.initRef(id->num, id->gen);
        font_obj = ref_obj.fetch(xref);
        //ref_obj.free();

        if(!font_obj.isDict())
        {
            cerr << "Font object is not a dictionary" << endl;
            throw 0;
        }

        Dict * dict = font_obj.getDict();
        font_obj2 = dict->lookup("DescendantFonts");
        if(font_obj2.isArray())
        {
            if(font_obj2.arrayGetLength() == 0)
            {
                cerr << "Warning: empty DescendantFonts array" << endl;
            }
            else
            {
                if(font_obj2.arrayGetLength() > 1) {
                    cerr << "TODO: multiple entries in DescendantFonts array" << endl;
                }
                
                obj2 = font_obj2.arrayGet(0);
                if(obj2.isDict())
                {
                    dict = obj2.getDict();
                }
            }
        }

        fontdesc_obj = dict->lookup("FontDescriptor");
        if(!fontdesc_obj.isDict())
        {
            cerr << "Cannot find FontDescriptor " << endl;
            throw 0;
        }

        dict = fontdesc_obj.getDict();
        obj = dict->lookup("FontFile3");
        if(obj.isStream())
        {
            obj1 = obj.streamGetDict()->lookup("Subtype");
            if(obj1.isName())
            {
                subtype = obj1.getName();
                if(subtype == "Type1C")
                {
                    suffix = ".cff";
                }
                else if (subtype == "CIDFontType0C")
                {
                    suffix = ".cid";
                }
                else if (subtype == "OpenType")
                {
                    suffix = ".otf";
                }
                else
                {
                    cerr << "Unknown subtype: " << subtype << endl;
                    throw 0;
                }
            }
            else
            {
                cerr << "Invalid subtype in font descriptor" << endl;
                throw 0;
            }
        } else {
            obj = dict->lookup("FontFile2");
            if (obj.isStream()) {
                suffix = ".ttf";
            } else {
                obj = dict->lookup("FontFile");
                if (obj.isStream()) {
                    suffix = ".pfa";
                } else {
                    cerr << "Cannot find FontFile for dump" << endl;
                    throw 0;
                }
            }
        }

        if(suffix == "")
        {
            cerr << "Font type unrecognized" << endl;
            throw 0;
        }

        obj.streamReset();

        filepath = (char*)str_fmt("%s/f%llx%s", param.tmp_dir.c_str(), fn_id, suffix.c_str());
        tmp_files.add(filepath);

        ofstream outf(filepath, ofstream::binary);
        if(!outf)
            throw string("Cannot open file ") + filepath + " for writing";

        char buf[1024];
        int len;
        while((len = obj.streamGetChars(1024, (Guchar*)buf)) > 0)
        {
            outf.write(buf, len);
        }
        obj.streamClose();
    }
    catch(int) 
    {
        cerr << "Something wrong when trying to dump font " << hex << fn_id << dec << endl;
    }

    //obj2.free();
    //obj1.free();
    //obj.free();

    //fontdesc_obj.free();
    //font_obj2.free();
    //font_obj.free();

    return filepath;
}

string HTMLRenderer::dump_type3_font (GfxFont * font, FontInfo & info)
{
    assert(info.is_type3);

#if ENABLE_SVG
    long long fn_id = info.id;

    FT_Library ft_lib;
    FT_Init_FreeType(&ft_lib);
    CairoFontEngine font_engine(ft_lib); 
    auto * cur_font = font_engine.getFont(font, cur_doc, true, xref);
    auto used_map = preprocessor.get_code_map(hash_ref(font->getID()));

    //calculate transformed metrics
    double * font_bbox = font->getFontBBox();
    double * font_matrix = font->getFontMatrix();
    double transformed_bbox[4];
    memcpy(transformed_bbox, font_bbox, 4 * sizeof(double));
    /*
    // add the origin to the bbox
    if(transformed_bbox[0] > 0) transformed_bbox[0] = 0;
    if(transformed_bbox[1] > 0) transformed_bbox[1] = 0;
    if(transformed_bbox[2] < 0) transformed_bbox[2] = 0;
    if(transformed_bbox[3] < 0) transformed_bbox[3] = 0;
    */
    tm_transform_bbox(font_matrix, transformed_bbox);
    double transformed_bbox_width = transformed_bbox[2] - transformed_bbox[0];
    double transformed_bbox_height = transformed_bbox[3] - transformed_bbox[1];
    info.font_size_scale = std::max(transformed_bbox_width, transformed_bbox_height);

    // we want the glyphs is rendered in a box of size around GLYPH_DUMP_EM_SIZE x GLYPH_DUMP_EM_SIZE
    // for rectangles, the longer edge should be GLYPH_DUMP_EM_SIZE
    const double GLYPH_DUMP_EM_SIZE = 100.0;
    double scale = GLYPH_DUMP_EM_SIZE / info.font_size_scale;

    // we choose ttf as it does not use char names
    // or actually we don't use char names for ttf (see embed_font)
    ffw_new_font();
    // dump each glyph into svg and combine them
    for(int code = 0; code < 256; ++code)
    {
        if(!used_map[code]) continue;

        cairo_surface_t * surface = nullptr;

        string glyph_filename = (char*)str_fmt("%s/f%llx-%x.svg", param.tmp_dir.c_str(), fn_id, code);
        tmp_files.add(glyph_filename);

        surface = cairo_svg_surface_create(glyph_filename.c_str(), transformed_bbox_width * scale, transformed_bbox_height * scale);

        cairo_svg_surface_restrict_to_version(surface, CAIRO_SVG_VERSION_1_2);
        cairo_surface_set_fallback_resolution(surface, param.h_dpi, param.v_dpi);
        cairo_t * cr = cairo_create(surface);

        // track the position of the origin
        double ox, oy;
        ox = oy = 0.0;

        auto glyph_width = ((Gfx8BitFont*)font)->getWidth(code);

#if 1
        {
            // pain the glyph
            cairo_set_font_face(cr, cur_font->getFontFace());

            cairo_matrix_t m1, m2, m3;
            // set up m1
            // m1 shift the bottom-left corner of the glyph bbox to the origin
            // also set font size to scale
            cairo_matrix_init_translate(&m1, -transformed_bbox[0], transformed_bbox[1]);
            cairo_matrix_init_scale(&m2, scale, scale);
            cairo_matrix_multiply(&m1, &m1, &m2);
            cairo_set_font_matrix(cr, &m1);

            cairo_glyph_t glyph;
            glyph.index = cur_font->getGlyph(code, nullptr, 0);
            glyph.x = 0;
            glyph.y = GLYPH_DUMP_EM_SIZE;
            cairo_show_glyphs(cr, &glyph, 1);


            // apply the type 3 font's font matrix before m1
            // such that we got the mapping from type 3 font space to user space, then we will be able to calculate mapped position for ox,oy and glyph_width
            cairo_matrix_init(&m2, font_matrix[0], font_matrix[1], font_matrix[2], font_matrix[3], font_matrix[4], font_matrix[5]);
            cairo_matrix_init_scale(&m3, 1, -1);
            cairo_matrix_multiply(&m2, &m2, &m3);
            cairo_matrix_multiply(&m2, &m2, &m1);

            cairo_matrix_transform_point(&m2, &ox, &oy);
            double dummy = 0;
            cairo_matrix_transform_distance(&m2, &glyph_width, &dummy);
        }
#else
        {
            // manually draw the char to get the metrics
            // adapted from _render_type3_glyph of poppler
            cairo_matrix_t ctm, m, m1;
            cairo_matrix_init_identity(&ctm);

            // apply font-matrix
            cairo_matrix_init(&m, font_matrix[0], font_matrix[1], font_matrix[2], font_matrix[3], font_matrix[4], font_matrix[5]);
            cairo_matrix_multiply(&ctm, &ctm, &m);

            // shift origin
            cairo_matrix_init_translate(&m1, -transformed_bbox[0], -transformed_bbox[1]);
            cairo_matrix_multiply(&ctm, &ctm, &m1);

            // make it upside down since the difference between the glyph coordination and cairo coordination
            cairo_matrix_init_scale(&m1, 1, -1);
            cairo_matrix_multiply(&ctm, &ctm, &m1);
            // save m*m1 to m1 for later use
            cairo_matrix_multiply(&m1, &m, &m1);

            // shift up to the bounding box
            cairo_matrix_init_translate(&m, 0.0, transformed_bbox_height);
            cairo_matrix_multiply(&ctm, &ctm, &m);

            // scale up 
            cairo_matrix_init_scale(&m, scale, scale);
            cairo_matrix_multiply(&ctm, &ctm, &m);

            // set ctm
            cairo_set_matrix(cr, &ctm);

            // calculate the position of origin
            cairo_matrix_transform_point(&ctm, &ox, &oy);
            oy -= transformed_bbox_height * scale;
            // calculate glyph width
            double dummy = 0;
            cairo_matrix_transform_distance(&ctm, &glyph_width, &dummy);

            // draw the glyph
            auto output_dev = new CairoOutputDev();
            output_dev->setCairo(cr);
            output_dev->setPrinting(true);

            PDFRectangle box;
            box.x1 = font_bbox[0];
            box.y1 = font_bbox[1];
            box.x2 = font_bbox[2];
            box.y2 = font_bbox[3];
            auto gfx = new Gfx(cur_doc, output_dev, 
                    ((Gfx8BitFont*)font)->getResources(),
                    &box, nullptr);
            output_dev->startDoc(cur_doc, &font_engine);
            output_dev->startPage(1, gfx->getState(), gfx->getXRef());
            output_dev->setInType3Char(gTrue);
            auto char_procs = ((Gfx8BitFont*)font)->getCharProcs();
            Object char_proc_obj;
            auto glyph_index = cur_font->getGlyph(code, nullptr, 0);
            gfx->display(char_procs->getVal(glyph_index, &char_proc_obj));

            char_proc_obj.free();
            delete gfx;
            delete output_dev;
        }
#endif

        {
            auto status = cairo_status(cr);
            cairo_destroy(cr);
            if(status)
                throw string("Cairo error: ") + cairo_status_to_string(status);
        }
        cairo_surface_finish(surface);
        {
            auto status = cairo_surface_status(surface);
            cairo_surface_destroy(surface);
            surface = nullptr;
            if(status)
                throw string("Error in cairo: ") + cairo_status_to_string(status);
        }

        ffw_import_svg_glyph(code, glyph_filename.c_str(), ox / GLYPH_DUMP_EM_SIZE, -oy / GLYPH_DUMP_EM_SIZE, glyph_width / GLYPH_DUMP_EM_SIZE);
    }

    string font_filename = (char*)str_fmt("%s/f%llx.ttf", param.tmp_dir.c_str(), fn_id);
    tmp_files.add(font_filename);
    ffw_save(font_filename.c_str());
    ffw_close();

    return font_filename;
#else
    return "";
#endif
}

namespace {

void output_map_file_header(std::ostream& out) {
    out << "glyph_code mapped_code unicode" << std::endl;
}

} // namespace

void HTMLRenderer::embed_font(const string & filepath, GfxFont * font, FontInfo & info, bool get_metric_only)
{
    if(param.debug)
    {
        cerr << "Embed font: " << filepath << " " << info.id << endl;
    }

    ffw_load_font(filepath.c_str());
    ffw_prepare_font();

    if(param.debug)
    {
        auto fn = str_fmt("%s/__raw_font_%llx%s", param.tmp_dir.c_str(), info.id, get_suffix(filepath).c_str());
        tmp_files.add((char*)fn);
        ofstream((char*)fn, ofstream::binary) << ifstream(filepath).rdbuf();
    }

    int * code2GID = nullptr;
    int code2GID_len = 0;
    int maxcode = 0;

    Gfx8BitFont * font_8bit = nullptr;
    GfxCIDFont * font_cid = nullptr;

    string suffix = get_suffix(filepath);
    for(auto & c : suffix)
        c = tolower(c);

    /*
     * if parm->tounicode is 0, try the provided tounicode map first
     */
    info.use_tounicode = (param.tounicode >= 0);
    bool has_space = false;

    const char * used_map = nullptr;

    info.em_size = ffw_get_em_size();

    if(param.debug)
    {
        cerr << "em size: " << info.em_size << endl;
    }

    info.space_width = 0;

    if(!font->isCIDFont())
    {
        font_8bit = dynamic_cast<Gfx8BitFont*>(font);
    }
    else
    {
        font_cid = dynamic_cast<GfxCIDFont*>(font);
    }

    if(get_metric_only)
    {
        ffw_fix_metric();
        ffw_get_metric(&info.ascent, &info.descent);
        ffw_close();
        return;
    }

    used_map = preprocessor.get_code_map(hash_ref(font->getID()));

    /*
     * Step 1
     * dump the font file directly from the font descriptor and put the glyphs into the correct slots *
     *
     * for 8bit + nonTrueType
     * re-encoding the font by glyph names
     *
     * for 8bit + TrueType
     * sort the glyphs as the original order, and load the code2GID table
     * later we will map GID (instead of char code) to Unicode
     *
     * for CID + nonTrueType
     * Flatten the font 
     *
     * for CID Truetype
     * same as 8bitTrueType, except for that we have to check 65536 charcodes
     * use the embedded code2GID table if there is, otherwise use the one in the font
     */
    if(font_8bit)
    {
        maxcode = 0xff;
        if(is_truetype_suffix(suffix))
        {
            if(info.is_type3)
            {
                /*
                 * Type 3 fonts are saved and converted into ttf fonts
                 * encoded based on code points instead of GID
                 *
                 * I thought code2GID would work but it never works, and I don't know why
                 * Anyway we can disable code2GID such that the following procedure will be working based on code points instead of GID
                 */
            }
            else
            {
                ffw_reencode_glyph_order();
                if(FoFiTrueType * fftt = FoFiTrueType::load((char*)filepath.c_str()))
                {
                    code2GID = font_8bit->getCodeToGIDMap(fftt);
                    code2GID_len = 256;
                    delete fftt;
                }
            }
        }
        else
        {
            // move the slot such that it's consistent with the encoding seen in PDF
            unordered_set<string> nameset;
            bool name_conflict_warned = false;

            std::fill(cur_mapping2.begin(), cur_mapping2.end(), (char*)nullptr);

            for(int i = 0; i < 256; ++i)
            {
                if(!used_map[i]) continue;

                auto cn = font_8bit->getCharName(i);
                if(cn == nullptr)
                {
                    continue;
                }
                else
                {
                    if(nameset.insert(string(cn)).second)
                    {
                        cur_mapping2[i] = cn;    
                    }
                    else
                    {
                        if(!name_conflict_warned)
                        {
                            name_conflict_warned = true;
                            //TODO: may be resolved using advanced font properties?
                            cerr << "Warning: encoding conflict detected in font: " << hex << info.id << dec << endl;
                        }
                    }
                }
            }

            ffw_reencode_raw2(cur_mapping2.data(), 256, 0);
        }
    }
    else
    {
        maxcode = 0xffff;

        if(is_truetype_suffix(suffix))
        {
            ffw_reencode_glyph_order();

            GfxCIDFont * _font = dynamic_cast<GfxCIDFont*>(font);
            assert(_font != nullptr);

            // To locate CID2GID for the font
            // as in CairoFontEngine.cc
            if((code2GID = _font->getCIDToGID()))
            {
                // use the mapping stored in _font
                code2GID_len = _font->getCIDToGIDLen();
            }
            else
            {
                // use the mapping stored in the file
                if(FoFiTrueType * fftt = FoFiTrueType::load((char*)filepath.c_str()))
                {
                    code2GID = _font->getCodeToGIDMap(fftt, &code2GID_len);
                    delete fftt;
                }
            }
        }
        else
        {
            // TODO: add an option to load the table?
            ffw_cidflatten();
        }
    }

    /*
     * Step 2
     * - map charcode (or GID for CID truetype)
     *
     * -> Always map to Unicode for 8bit TrueType fonts and CID fonts
     *
     * -> For 8bit nonTruetype fonts:
     *   Try to calculate the correct Unicode value from the glyph names, when collision is detected in ToUnicode Map
     * 
     * - Fill in the width_list, and set widths accordingly
     */


    {
        string map_filename;
        ofstream map_outf;
        if(param.debug)
        {
            map_filename = (char*)str_fmt("%s/f%llx.map", param.tmp_dir.c_str(), info.id);
            tmp_files.add(map_filename);
            map_outf.open(map_filename);
            output_map_file_header(map_outf);
        }

        unordered_set<int> codeset;
        bool name_conflict_warned = false;

        auto ctu = font->getToUnicode();
        std::fill(cur_mapping.begin(), cur_mapping.end(), -1);
        std::fill(width_list.begin(), width_list.end(), -1);

        if(code2GID)
            maxcode = min<int>(maxcode, code2GID_len - 1);

        bool is_truetype = is_truetype_suffix(suffix);
        int max_key = maxcode;
        /*
         * Traverse all possible codes
         */
        bool retried = false; // avoid infinite loop
        for(int cur_code = 0; cur_code <= maxcode; ++cur_code)
        {
            if(!used_map[cur_code])
                continue;

            /*
             * Skip glyphs without names (only for non-ttf fonts)
             */
            if(!is_truetype && (font_8bit != nullptr) 
                    && (font_8bit->getCharName(cur_code) == nullptr))
            {
                continue;
            }

            int mapped_code = cur_code;
            if(code2GID)
            {
                // for fonts with GID (e.g. TTF) we need to map GIDs instead of codes
                if((mapped_code = code2GID[cur_code]) == 0) continue;
            }

            if(mapped_code > max_key)
                max_key = mapped_code;

            Unicode u, *pu=&u;
            if(info.use_tounicode)
            {
                int n = ctu ? (ctu->mapToUnicode(cur_code, &pu)) : 0;
                u = check_unicode(pu, n, cur_code, font);
            }
            else
            {
                u = unicode_from_font(cur_code, font);
            }

            if(codeset.insert(u).second)
            {
                cur_mapping[mapped_code] = u;
            }
            else
            {
                // collision detected
                if(param.tounicode == 0)
                {
                    // in auto mode, just drop the tounicode map
                    if(!retried)
                    {
                        cerr << "ToUnicode CMap is not valid and got dropped for font: " << hex << info.id << dec << endl;
                        retried = true;
                        codeset.clear();
                        info.use_tounicode = false;
                        std::fill(cur_mapping.begin(), cur_mapping.end(), -1);
                        std::fill(width_list.begin(), width_list.end(), -1);
                        cur_code = -1;
                        if(param.debug)
                        {
                            map_outf.close();
                            map_outf.open(map_filename);
                            output_map_file_header(map_outf);
                        }
                        continue;
                    }
                }
                if(!name_conflict_warned)
                {
                    name_conflict_warned = true;
                    //TODO: may be resolved using advanced font properties?
                    cerr << "Warning: encoding confliction detected in font: " << hex << info.id << dec << endl;
                }
            }

            {
                double cur_width = 0;
                if(font_8bit)
                {
                    cur_width = font_8bit->getWidth(cur_code);
                }
                else
                {
                    char buf[2];  
                    buf[0] = (cur_code >> 8) & 0xff;
                    buf[1] = (cur_code & 0xff);
                    cur_width = font_cid->getWidth(buf, 2) ;
                }

                cur_width /= info.font_size_scale;

                if(u == ' ')
                {
                    /*
                     * Internet Explorer will ignore `word-spacing` if
                     * the width of the 'space' glyph is 0
                     *
                     * space_width==0 often means no spaces are used in the PDF
                     * so setting it to be 0.001 should be safe
                     */
                    if(equal(cur_width, 0))
                        cur_width = 0.001;

                    info.space_width = cur_width;
                    has_space = true;
                }
                
                width_list[mapped_code] = (int)floor(cur_width * info.em_size + 0.5);
            }

            if(param.debug)
            {
                map_outf << hex << cur_code << ' ' << mapped_code << ' ' << u << endl;
            }
        }

        ffw_set_widths(width_list.data(), max_key + 1, param.stretch_narrow_glyph, param.squeeze_wide_glyph);
        
        ffw_reencode_raw(cur_mapping.data(), max_key + 1, 1);

        // In some space offsets in HTML, we insert a ' ' there in order to improve text copy&paste
        // We need to make sure that ' ' is in the font, otherwise it would be very ugly if you select the text
        // Might be a problem if ' ' is in the font, but not empty
        if(!has_space)
        {
            if(font_8bit)
            {
                info.space_width = font_8bit->getWidth(' ');
            }
            else
            {
                char buf[2] = {0, ' '};
                info.space_width = font_cid->getWidth(buf, 2);
            }
            info.space_width /= info.font_size_scale;

            /* See comments above */
            if(equal(info.space_width,0))
                info.space_width = 0.001;

            ffw_add_empty_char((int32_t)' ', (int)floor(info.space_width * info.em_size + 0.5));
            if(param.debug)
            {
                cerr << "Missing space width in font " << hex << info.id << ": set to " << dec << info.space_width << endl;
            }
        }

        if(param.debug)
        {
            cerr << "space width: " << info.space_width << endl;
        }

        if(ctu)
            ctu->decRefCnt();
    }

    /*
     * Step 3
     * Generate the font as desired
     */

    // Reencode to Unicode Full such that FontForge won't ditch unicode values larger than 0xFFFF
    ffw_reencode_unicode_full();

    // Due to a bug of Fontforge about pfa -> woff conversion
    // we always generate TTF first, instead of the format specified by user
    string cur_tmp_fn = (char*)str_fmt("%s/__tmp_font1.%s", param.tmp_dir.c_str(), "ttf");
    tmp_files.add(cur_tmp_fn);
    string other_tmp_fn = (char*)str_fmt("%s/__tmp_font2.%s", param.tmp_dir.c_str(), "ttf");
    tmp_files.add(other_tmp_fn);

    ffw_save(cur_tmp_fn.c_str());

    ffw_close();

    /*
     * Step 4
     * Font Hinting
     */
    bool hinted = false;

    // Call external hinting program if specified 
    if(param.external_hint_tool != "")
    {
        hinted = (system((char*)str_fmt("%s \"%s\" \"%s\"", param.external_hint_tool.c_str(), cur_tmp_fn.c_str(), other_tmp_fn.c_str())) == 0);
    }

    // Call internal hinting procedure if specified 
    if((!hinted) && (param.auto_hint))
    {
        ffw_load_font(cur_tmp_fn.c_str());
        ffw_auto_hint();
        ffw_save(other_tmp_fn.c_str());
        ffw_close();
        hinted = true;
    }

    if(hinted)
    {
        swap(cur_tmp_fn, other_tmp_fn);
    }

    /* 
     * Step 5 
     * Generate the font, load the metrics and set the embedding bits (fstype)
     *
     * Ascent/Descent are not used in PDF, and the values in PDF may be wrong or inconsistent (there are 3 sets of them)
     * We need to reload in order to retrieve/fix accurate ascent/descent, some info won't be written to the font by fontforge until saved.
     */
    string fn = (char*)str_fmt("%s/f%llx.%s", 
        (param.embed_font ? param.tmp_dir : param.dest_dir).c_str(),
        info.id, param.font_format.c_str());

    if(param.embed_font)
        tmp_files.add(fn);

    ffw_load_font(cur_tmp_fn.c_str());
    ffw_fix_metric();
    ffw_get_metric(&info.ascent, &info.descent);
    if(param.override_fstype)
        ffw_override_fstype();
    ffw_save(fn.c_str());

    ffw_close();
}


const FontInfo * HTMLRenderer::install_font(GfxFont * font)
{
    assert(sizeof(long long) == 2*sizeof(int));
                
    long long fn_id = (font == nullptr) ? 0 : hash_ref(font->getID());

    auto iter = font_info_map.find(fn_id);
    if(iter != font_info_map.end())
        return &(iter->second);

    long long new_fn_id = font_info_map.size(); 

    auto cur_info_iter = font_info_map.insert(make_pair(fn_id, FontInfo())).first;

    FontInfo & new_font_info = cur_info_iter->second;
    new_font_info.id = new_fn_id;
    new_font_info.use_tounicode = true;
    new_font_info.font_size_scale = 1.0;

    if(font == nullptr)
    {
        new_font_info.em_size = 0;
        new_font_info.space_width = 0;
        new_font_info.ascent = 0;
        new_font_info.descent = 0;
        new_font_info.is_type3 = false;

        export_remote_default_font(new_fn_id);

        return &(new_font_info);
    }

    new_font_info.ascent = font->getAscent();
    new_font_info.descent = font->getDescent();
    new_font_info.is_type3 = (font->getType() == fontType3);

    if(param.debug)
    {
        cerr << "Install font " << hex << new_fn_id << dec
            << ": (" << (font->getID()->num) << ' ' << (font->getID()->gen) << ") " 
            << (font->getName() ? font->getName()->getCString() : "")
            << endl;
    }

    if(new_font_info.is_type3)
    {
#if ENABLE_SVG
        if(param.process_type3)
        {
            install_embedded_font(font, new_font_info);
        }
        else
        {
            export_remote_default_font(new_fn_id);
        }
#else
        cerr << "Type 3 fonts are unsupported and will be rendered as Image" << endl;
        export_remote_default_font(new_fn_id);
#endif
        return &new_font_info;
    }
    if(font->getWMode()) {
        cerr << "Writing mode is unsupported and will be rendered as Image" << endl;
        export_remote_default_font(new_fn_id);
        return &new_font_info;
    }

    /*
     * The 2nd parameter of locateFont should be true only for PS
     * which does not make much sense in our case
     * If we specify gFalse here, font_loc->locType cannot be gfxFontLocResident
     */
    if(auto * font_loc = font->locateFont(xref, nullptr))
    {
        switch(font_loc -> locType)
        {
            case gfxFontLocEmbedded:
                install_embedded_font(font, new_font_info);
                break;
            case gfxFontLocResident:
                std::cerr << "Warning: Base 14 fonts should not be specially handled now. Please report a bug!" << std::endl;
                /* fall through */
            case gfxFontLocExternal:
                install_external_font(font, new_font_info);
                break;
            default:
                cerr << "TODO: other font loc" << endl;
                export_remote_default_font(new_fn_id);
                break;
        }      
        delete font_loc;
    }
    else
    {
        export_remote_default_font(new_fn_id);
    }
      
    return &new_font_info;
}

void HTMLRenderer::install_embedded_font(GfxFont * font, FontInfo & info)
{
    auto path = dump_embedded_font(font, info);

    if(path != "")
    {
        embed_font(path, font, info);
        export_remote_font(info, param.font_format, font);
    }
    else
    {
        export_remote_default_font(info.id);
    }
}

void HTMLRenderer::install_external_font(GfxFont * font, FontInfo & info)
{
    string fontname(font->getName()->getCString());

    // resolve bad encodings in GB
    auto iter = GB_ENCODED_FONT_NAME_MAP.find(fontname); 
    if(iter != GB_ENCODED_FONT_NAME_MAP.end())
    {
        fontname = iter->second;
        cerr << "Warning: workaround for font names in bad encodings." << endl;
    }

    GfxFontLoc * localfontloc = font->locateFont(xref, nullptr);

    if(param.embed_external_font)
    {
        if(localfontloc != nullptr)
        {
            embed_font(string(localfontloc->path->getCString()), font, info);
            export_remote_font(info, param.font_format, font);
            delete localfontloc;
            return;
        }
        else
        {
            cerr << "Cannot embed external font: f" << hex << info.id << dec << ' ' << fontname << endl;
            // fallback to exporting by name
        }
    }

    // still try to get an idea of read ascent/descent
    if(localfontloc != nullptr)
    {
        // fill in ascent/descent only, do not embed
        embed_font(string(localfontloc->path->getCString()), font, info, true);
        delete localfontloc;
    }
    else
    {
        info.ascent = font->getAscent();
        info.descent = font->getDescent();
    }

    export_local_font(info, font, fontname, "");
}

void HTMLRenderer::export_remote_font(const FontInfo & info, const string & format, GfxFont * font)
{
    string css_font_format;
    if(format == "ttf")
    {
        css_font_format = "truetype";
    }
    else if(format == "otf")
    {
        css_font_format = "opentype";
    }
    else if(format == "woff")
    {
        css_font_format = "woff";
    }
    else if(format == "eot")
    {
        css_font_format = "embedded-opentype";
    }
    else if(format == "svg")
    {
        css_font_format = "svg";
    }
    else
    {
        throw string("Warning: unknown font format: ") + format;
    }
    auto iter = FORMAT_MIME_TYPE_MAP.find(format);
    if(iter == FORMAT_MIME_TYPE_MAP.end())
    {
        throw string("Warning: unknown font format: ") + format;
    }
    string mime_type = iter->second;

    f_css.fs << "@font-face{"
             << "font-family:" << CSS::FONT_FAMILY_CN << info.id << ";"
             << "src:url(";

    {
        auto fn = str_fmt("f%llx.%s", info.id, format.c_str());
        if(param.embed_font)
        {
            auto path = param.tmp_dir + "/" + (char*)fn;
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw "Cannot locate font file: " + path;
            f_css.fs << "'data:" + mime_type + ";base64," << Base64Stream(fin) << "'";
        }
        else
        {
            f_css.fs << (char*)fn;
        }
    }

    f_css.fs << ")"
             << "format(\"" << css_font_format << "\");"
             << "}" // end of @font-face
             << "." << CSS::FONT_FAMILY_CN << info.id << "{"
             << "font-family:" << CSS::FONT_FAMILY_CN << info.id << ";"
             << "line-height:" << round(info.ascent - info.descent) << ";"
             << "font-style:normal;"
             << "font-weight:normal;"
             << "visibility:visible;"
             << "}" 
             << endl;
}

static string general_font_family(GfxFont * font)
{
    if(font->isFixedWidth())
        return "monospace";
    else if (font->isSerif())
        return "serif";
    else
        return "sans-serif";
}

// TODO: this function is called when some font is unable to process, may use the name there as a hint
void HTMLRenderer::export_remote_default_font(long long fn_id) 
{
    f_css.fs << "." << CSS::FONT_FAMILY_CN << fn_id << "{font-family:sans-serif;visibility:hidden;}" << endl;
}

void HTMLRenderer::export_local_font(const FontInfo & info, GfxFont * font, const string & original_font_name, const string & cssfont) 
{
    f_css.fs << "." << CSS::FONT_FAMILY_CN << info.id << "{";
    f_css.fs << "font-family:" << ((cssfont == "") ? (original_font_name + "," + general_font_family(font)) : cssfont) << ";";

    string fn = original_font_name;
    for(auto & c : fn)
        c = tolower(c);

    if(font->isBold() || (fn.find("bold") != string::npos))
        f_css.fs << "font-weight:bold;";
    else
        f_css.fs << "font-weight:normal;";

    if(fn.find("oblique") != string::npos)
        f_css.fs << "font-style:oblique;";
    else if(font->isItalic() || (fn.find("italic") != string::npos))
        f_css.fs << "font-style:italic;";
    else
        f_css.fs << "font-style:normal;";

    f_css.fs << "line-height:" << round(info.ascent - info.descent) << ";";

    f_css.fs << "visibility:visible;";

    f_css.fs << "}" << endl;
}

} //namespace pdf2htmlEX
