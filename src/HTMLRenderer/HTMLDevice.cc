#include "HTMLDevice.h"
#include "Param.h"
#include "TmpFiles.h"
#include "TextState.h"

#include <Annot.h>

using namespace std;

namespace pdf2htmlEX {

const string HTMLDevice::MANIFEST_FILENAME = "manifest";
const char * HTMLDevice::format_str = "fsclwr";

HTMLDevice::HTMLDevice( Param const& param_, TmpFiles& tmp_files_ )
	: param( param_ )
	, tmp_files( tmp_files_ )
{
}

void HTMLDevice::text_start( double x, double y, long long tm_id, long long height )
{
    html_fout << "<div style=\"left:" 
			  << _round(x) << "px;bottom:" 
			  << _round(y) << "px;"
			  << "\""
			  << " class=\"l t" << tm_id 
			  << " h" << height
			  << "\">";
}

void HTMLDevice::text_end()
{
    html_fout << "</div>";
}

bool HTMLDevice::span_start( TextState const& state, TextState const* prev_state)
{
    bool first = true;
    for(int i = 0; i < TextState::ID_COUNT; ++i)
    {
        if(prev_state && (prev_state->ids[i] == state.ids[i]))
            continue;

        if(first)
        { 
            html_fout << "<span class=\"";
            first = false;
        }
        else
        {
            html_fout << ' ';
        }

        // out should has set hex
        html_fout << format_str[i] << state.ids[i];
    }

    if(first)
		return false;
	html_fout << "\">";
	return true;
}

void HTMLDevice::span_end()
{
	html_fout << "</span>";
}

void HTMLDevice::span_whitespace( TextState const& state, double target, long long wid )
{
	double threshold = state.draw_font_size * (state.ascent - state.descent) * (param.space_threshold);
	
	html_fout << "<span class=\"_ _" << wid << "\">" << (target > (threshold - EPS) ? " " : "") << "</span>";
}

void HTMLDevice::text_data( Unicode const* u, int uLen )
{
	outputUnicodes( html_fout, u, uLen );
}
/* ===================================================================== */

void HTMLDevice::document_start()
{
    // we may output utf8 characters, so always use binary
    {
        /*
         * If single-html && !split-pages
         * we have to keep the generated css file into a temporary place
         * and embed it into the main html later
         *
         *
         * If single-html && split-page
         * as there's no place to embed the css file, just leave it alone (into param->dest_dir)
         *
         * If !single-html
         * leave it in param.dest_dir
         */

        auto fn = (param.single_html && (!param.split_pages))
            ? str_fmt("%s/__css", param.tmp_dir.c_str())
            : str_fmt("%s/%s", param.dest_dir.c_str(), param.css_filename.c_str());

        if(param.single_html && (!param.split_pages))
            tmp_files.add((char*)fn);

        css_path = (char*)fn,
			css_fout.open(css_path, ofstream::binary);
        if(!css_fout)
            throw string("Cannot open ") + (char*)fn + " for writing";
        fix_stream(css_fout);
    }

    // if split-pages is specified, open & close the file in the process loop
    // if not, open the file here:
    if(!param.split_pages)
    {
        /*
         * If single-html
         * we have to keep the html file (for page) into a temporary place
         * because we'll have to embed css before it
         *
         * Otherwise just generate it 
         */
        auto fn = str_fmt("%s/__pages", param.tmp_dir.c_str());
        tmp_files.add((char*)fn);

        html_path = (char*)fn;
        html_fout.open(html_path, ofstream::binary); 
        if(!html_fout)
            throw string("Cannot open ") + (char*)fn + " for writing";
        fix_stream(html_fout);
    }

}

void HTMLDevice::document_end()
{
    // close files
    html_fout.close(); 
    css_fout.close();

    //only when split-page, do we have some work left to do
    if(param.split_pages)
        return;

    ofstream output;
    {
        auto fn = str_fmt("%s/%s", param.dest_dir.c_str(), param.output_filename.c_str());
        output.open((char*)fn, ofstream::binary);
        if(!output)
            throw string("Cannot open ") + (char*)fn + " for writing";
        fix_stream(output);
    }

    // apply manifest
    ifstream manifest_fin((char*)str_fmt("%s/%s", param.data_dir.c_str(), MANIFEST_FILENAME.c_str()), ifstream::binary);
    if(!manifest_fin)
        throw "Cannot open the manifest file";

    bool embed_string = false;
    string line;
    while(getline(manifest_fin, line))
    {
        if(line == "\"\"\"")
        {
            embed_string = !embed_string;
            continue;
        }

        if(embed_string)
        {
            output << line << endl;
            continue;
        }

        if(line.empty() || line[0] == '#')
            continue;


        if(line[0] == '@')
        {
            embed_file(output, param.data_dir + "/" + line.substr(1), "", true);
            continue;
        }

        if(line[0] == '$')
        {
            if(line == "$css")
            {
                embed_file(output, css_path, ".css", false);
            }
            else if (line == "$pages")
            {
                ifstream fin(html_path, ifstream::binary);
                if(!fin)
                    throw "Cannot open read the pages";
                output << fin.rdbuf();
            }
            else
            {
                cerr << "Warning: unknown line in manifest: " << line << endl;
            }
            continue;
        }

        cerr << "Warning: unknown line in manifest: " << line << endl;
    }

}

string HTMLDevice::page_start( int pageNum )
{
	if(param.split_pages)
	{
		auto page_fn = str_fmt("%s/%s%d.page", param.dest_dir.c_str(), param.output_filename.c_str(), pageNum );
		html_fout.open((char*)page_fn, ofstream::binary); 
		if(!html_fout)
			throw string("Cannot open ") + (char*)page_fn + " for writing";
		fix_stream(html_fout);
	}

	if(param.process_nontext)
	{
		auto fn = str_fmt("%s/p%x.png", (param.single_html ? param.tmp_dir : param.dest_dir).c_str(), pageNum);
		if(param.single_html)
			tmp_files.add((char*)fn);

		return (char*)fn;
	}
	return string();
}


void HTMLDevice::page_header( double pageWidth, double pageHeight, int pageNum )
{
    html_fout 
        << "<div class=\"d\" style=\"width:" 
            << (pageWidth) << "px;height:" 
            << (pageHeight) << "px;\">"
        << "<div id=\"p" << pageNum << "\" data-page-no=\"" << pageNum << "\" class=\"p\">"
        << "<div class=\"b\" style=\"";

    if(param.process_nontext)
    {
        html_fout << "background-image:url(";

        {
            if(param.single_html)
            {
                auto path = str_fmt("%s/p%x.png", param.tmp_dir.c_str(), pageNum);
                ifstream fin((char*)path, ifstream::binary);
                if(!fin)
                    throw string("Cannot read background image ") + (char*)path;
                html_fout << "'data:image/png;base64," << base64stream(fin) << "'";
            }
            else
            {
                html_fout << str_fmt("p%x.png", pageNum);
            }
        }

        html_fout << ");background-position:0 0;background-size:" << pageWidth << "px " << pageHeight << "px;background-repeat:no-repeat;";
    }
    html_fout << "\">";
}

void HTMLDevice::page_footer( double const* ctm )
{
    // close box
    html_fout << "</div>";

    // dump info for js
    // TODO: create a function for this
    // BE CAREFUL WITH ESCAPES
    html_fout << "<div class=\"j\" data-data='{";
    
    //default CTM
    html_fout << "\"ctm\":[";
    for(int i = 0; i < 6; ++i)
    {
        if(i > 0) html_fout << ",";
        html_fout << _round( ctm[i] );
    }
    html_fout << "]";

    html_fout << "}'></div>";
    
    // close page
    html_fout << "</div></div>" << endl;
}

void HTMLDevice::page_end()
{
	if(param.split_pages)
	{
		html_fout.close();
	}
}

/* ====================================================================== */

void HTMLDevice::export_remote_font(const FontInfo & info, const string & suffix, const string & fontfileformat ) 
{
    css_fout << "@font-face{font-family:f" << info.id << ";src:url(";

    {
        auto fn = str_fmt("f%llx%s", info.id, suffix.c_str());
        if(param.single_html)
        {
            auto path = param.tmp_dir + "/" + (char*)fn;
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw "Cannot locate font file: " + path;
            css_fout << "'data:font/" + fontfileformat + ";base64," << base64stream(fin) << "'";
        }
        else
        {
            css_fout << (char*)fn;
        }
    }

    css_fout << ")format(\"" << fontfileformat 
        << "\");}.f" << info.id 
        << "{font-family:f" << info.id 
        << ";line-height:" << _round(info.ascent - info.descent) 
        << ";font-style:normal;font-weight:normal;}";

    css_fout << endl;
}


// TODO: this function is called when some font is unable to process, may use the name there as a hint
void HTMLDevice::export_remote_default_font(long long fn_id) 
{
    css_fout << ".f" << fn_id << "{font-family:sans-serif;color:transparent;visibility:hidden;}" << endl;
}

static string general_font_family( FontAttrs const& attrs )
{
    if( attrs.fixed  )
        return "monospace";
    else if ( attrs.serif )
        return "serif";
    else
        return "sans-serif";
}

void HTMLDevice::export_local_font( const FontInfo & info, 
									const string & original_font_name, 
									const string & cssfont,
									FontAttrs const& attrs )
{
    css_fout << ".f" << info.id << "{";
    css_fout << "font-family:" << ((cssfont == "") ? original_font_name + "," + general_font_family( attrs ) : cssfont) << ";";

    string fn = original_font_name;
    for(auto iter = fn.begin(); iter != fn.end(); ++iter)
        *iter = tolower(*iter);

   if( attrs.bold || (fn.find("bold") != string::npos))
        css_fout << "font-weight:bold;";
    else
        css_fout << "font-weight:normal;";

    if(fn.find("oblique") != string::npos)
        css_fout << "font-style:oblique;";
    else if( attrs.italic || (fn.find("italic") != string::npos))
        css_fout << "font-style:italic;";
    else
        css_fout << "font-style:normal;";

    css_fout << "line-height:" << _round(info.ascent - info.descent) << ";";

    css_fout << "}" << endl;
}

void HTMLDevice::export_font_size (long long fs_id, double font_size) 
{
    css_fout << ".s" << fs_id << "{font-size:" << _round(font_size) << "px;}" << endl;
}

void HTMLDevice::export_transform_matrix (long long tm_id, const double * tm) 
{
    css_fout << ".t" << tm_id << "{";

    // always ignore tm[4] and tm[5] because
    // we have already shifted the origin
    
    // TODO: recognize common matices
    if(_tm_equal(tm, id_matrix, 4))
    {
        auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
        for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
            css_fout << *iter << "transform:none;";
    }
    else
    {
        auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
        for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
        {
            // PDF use a different coordinate system from Web
            css_fout << *iter << "transform:matrix("
                << _round(tm[0]) << ','
                << _round(-tm[1]) << ','
                << _round(-tm[2]) << ','
                << _round(tm[3]) << ',';

            css_fout << "0,0);";
        }
    }
    css_fout << "}" << endl;
}

void HTMLDevice::export_letter_space (long long ls_id, double letter_space) 
{
    css_fout << ".l" << ls_id << "{letter-spacing:" << _round(letter_space) << "px;}" << endl;
}

void HTMLDevice::export_word_space (long long ws_id, double word_space) 
{
    css_fout << ".w" << ws_id << "{word-spacing:" << _round(word_space) << "px;}" << endl;
}

void HTMLDevice::export_color (long long color_id, const GfxRGB * rgb) 
{
    css_fout << ".c" << color_id << "{color:" << (*rgb) << ";}" << endl;
}

void HTMLDevice::export_whitespace (long long ws_id, double ws_width) 
{
    if(ws_width > 0)
        css_fout << "._" << ws_id << "{display:inline-block;width:" << _round(ws_width) << "px;}" << endl;
    else
        css_fout << "._" << ws_id << "{display:inline;margin-left:" << _round(ws_width) << "px;}" << endl;
}

void HTMLDevice::export_rise (long long rise_id, double rise) 
{
    css_fout << ".r" << rise_id << "{top:" << _round(-rise) << "px;}" << endl;
}

void HTMLDevice::export_height(long long height_id, double height) 
{
    css_fout << ".h" << height_id << "{height:" << _round(height) << "px;}" << endl;
}

void HTMLDevice::css_draw_rectangle( double x, double y, double w, double h, double scale, long long tm,
									 double * line_width_array, int line_width_count,
									 const GfxRGB * line_color, const GfxRGB * fill_color, 
									 void (*style_function)(void *, ostream &), void * style_function_data )
{
    html_fout << "<div class=\"Cd t" << tm << "\" style=\"";

    if(line_color)
    {
        html_fout << "border-color:" << *line_color << ";";

        html_fout << "border-width:";
        for(int i = 0; i < line_width_count; ++i)
        {
            if(i > 0) html_fout << ' ';

            double lw = line_width_array[i] * scale;
            html_fout << _round(lw);
            if(_is_positive(lw)) html_fout << "px";
        }
        html_fout << ";";
    }
    else
    {
        html_fout << "border:none;";
    }

    if(fill_color)
    {
        html_fout << "background-color:" << (*fill_color) << ";";
    }
    else
    {
        html_fout << "background-color:transparent;";
    }

    if(style_function)
    { 
        style_function(style_function_data, html_fout);
    }

    html_fout << "bottom:" << _round(y) << "px;"
        << "left:" << _round(x) << "px;"
        << "width:" << _round(w * scale) << "px;"
        << "height:" << _round(h * scale) << "px;";

    html_fout << "\"></div>";
}

void HTMLDevice::render_style( int style )
{
	switch(style)
	{
	case AnnotBorder::borderSolid:
		html_fout << "border-style:solid;";
		break;
	case AnnotBorder::borderDashed:
		html_fout << "border-style:dashed;";
		break;
	case AnnotBorder::borderBeveled:
		html_fout << "border-style:outset;";
		break;
	case AnnotBorder::borderInset:
		html_fout << "border-style:inset;";
		break;
	case AnnotBorder::borderUnderlined:
		html_fout << "border-style:none;border-bottom-style:solid;";
		break;
	default:
		cerr << "Warning:Unknown annotation border style: " << style << endl;
		html_fout << "border-style:solid;";
	}
}

void HTMLDevice::render_color( AnnotColor const* color )
{
	double r,g,b;
	if(color && (color->getSpace() == AnnotColor::colorRGB))
	{
		const double * v = color->getValues();
		r = v[0];
		g = v[1];
		b = v[2];
	}
	else
	{
		r = g = b = 0;
	}

	html_fout << "border-color:rgb("
			  << dec << (int)dblToByte(r) << "," << (int)dblToByte(g) << "," << (int)dblToByte(b) << hex
			  << ");";
}

void HTMLDevice::link( 
	double x1, double y1, double x2, double y2,
	long long tm,
	double const* ctm,
	string const& dest_str, 
	string const& dest_detail_str,
	AnnotBorder const* border,
	AnnotColor const* color )
{
    if(dest_str != "")
    {
        html_fout << "<a class=\"a\" href=\"" << dest_str << "\"";

        if(dest_detail_str != "")
            html_fout << " data-dest-detail='" << dest_detail_str << "'";

        html_fout << ">";
    }

    html_fout << "<div class=\"Cd t" << tm << "\" style=\"";

    double x,y,w,h;
    x = min<double>(x1, x2);
    y = min<double>(y1, y2);
    w = max<double>(x1, x2) - x;
    h = max<double>(y1, y2) - y;
    
    double border_width = 0; 
    double border_top_bottom_width = 0;
    double border_left_right_width = 0;
    if(border)
    {
        border_width = border->getWidth();
        if(border_width > 0)
        {
            {
                css_fix_rectangle_border_width(x1, y1, x2, y2, border_width, 
                        x, y, w, h,
                        border_top_bottom_width, border_left_right_width);

                if(abs(border_top_bottom_width - border_left_right_width) < EPS)
                    html_fout << "border-width:" << _round(border_top_bottom_width) << "px;";
                else
                    html_fout << "border-width:" << _round(border_top_bottom_width) << "px " << _round(border_left_right_width) << "px;";
            }
            render_style( border->getStyle() );

            render_color( color );
        }
        else
        {
            html_fout << "border-style:none;";
        }
    }
    else
    {
        html_fout << "border-style:none;";
    }

    _tm_transform( ctm, x, y);

    html_fout << "position:absolute;"
        << "left:" << _round(x) << "px;"
        << "bottom:" << _round(y) << "px;"
        << "width:" << _round(w) << "px;"
        << "height:" << _round(h) << "px;";

    // fix for IE
    html_fout << "background-color:rgba(255,255,255,0.000001);";

    html_fout << "\"></div>";

    if(dest_str != "")
    {
        html_fout << "</a>";
    }
}

/* ============================================================== */

void HTMLDevice::fix_stream (ostream & out)
{
    // we output all ID's in hex
    // browsers are not happy with scientific notations
    out << hex << fixed;
}

void HTMLDevice::embed_file(ostream & out, const string & path, const string & type, bool copy)
{
    string fn = get_filename(path);
    string suffix = (type == "") ? get_suffix(fn) : type; 
    
    auto iter = EMBED_STRING_MAP.find(make_pair(suffix, (bool)param.single_html));
    if(iter == EMBED_STRING_MAP.end())
    {
        cerr << "Warning: unknown suffix: " << suffix << endl;
        return;
    }
    
    if(param.single_html)
    {
        ifstream fin(path, ifstream::binary);
        if(!fin)
            throw string("Cannot open file ") + path + " for embedding";
        out << iter->second.first << endl
            << fin.rdbuf()
            << iter->second.second << endl;
    }
    else
    {
        out << iter->second.first
            << fn
            << iter->second.second << endl;

        if(copy)
        {
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw string("Cannot copy file: ") + path;
            auto out_path = param.dest_dir + "/" + fn;
            ofstream out(out_path, ofstream::binary);
            if(!out)
                throw string("Cannot open file ") + path + " for embedding";
            out << fin.rdbuf();
        }
    }
}


} // namespace pdf2htmlEX
