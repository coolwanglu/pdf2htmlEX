#ifndef HTMLDEVICE_H__
#define HTMLDEVICE_H__

#include <string>
#include <memory>
#include <fstream>
#include "util.h"

class AnnotBorder;
class AnnotColor;

namespace pdf2htmlEX {

class Param;
class TmpFiles;
class TextState;

class HTMLDevice 
{
public:
    HTMLDevice( Param const& param, TmpFiles& tmp_files );

	void set_default_ctm(double *ctm);

	void document_start();
	void document_end();

	std::string page_start( int pageNum );
	void page_end();
	void page_header( double pageWidth, double pageHeight, int pageNum );
	void page_footer( double const* ctm );


	////////////////////////////////////////////////////
	// export css styles
	////////////////////////////////////////////////////
	/*
	 * remote font: to be retrieved from the web server
	 * local font: to be substituted with a local (client side) font
	 */

	void export_remote_font( 
		FontInfo const& info, 
		std::string const& suffix, 
		std::string const& fontfileformat ) ;
	void export_remote_default_font(long long fn_id) ;
	void export_local_font( 
		FontInfo const& info, 
		std::string const& original_font_name, 
		std::string const& cssfont,
		FontAttrs const& attrs );

	void export_font_size (long long fs_id, double font_size) ;
	void export_transform_matrix (long long tm_id, const double * tm) ;
	void export_letter_space (long long ls_id, double letter_space) ;
	void export_word_space (long long ws_id, double word_space) ;
	void export_color (long long color_id, const GfxRGB * rgb) ;
	void export_whitespace (long long ws_id, double ws_width) ;
	void export_rise (long long rise_id, double rise) ;
	void export_height(long long height_id, double height) ;

	
	void css_draw_rectangle( double x, double y, double w, double h, double scale, long long tm,
							 double * line_width_array, int line_width_count,
							 const GfxRGB * line_color, const GfxRGB * fill_color, 
							 void (*style_function)(void *, std::ostream &), void * style_function_data );

	void link( 
		double x1, double y1, double x2, double y2,
		long long tm,
		double const* ctm,
		std::string const& dest_str, 
		std::string const& dest_detail_str,
		AnnotBorder const* border,
		AnnotColor const* color );

	void text_start( double x, double y, long long tm_id, long long height );
	void text_end();
	bool span_start( TextState const& state, TextState const* prev_state);
	void span_end();
	void span_whitespace( TextState const& state, double target, long long wid );
	void text_data( Unicode const* u, int uLen );

private:
	void render_color( AnnotColor const* color );
	void render_style( int style );

private:
	void fix_stream(std::ostream & out);
	// depending on single-html, to embed the content or add a link to it
	// "type": specify the file type, usually it's the suffix, in which case this parameter could be ""
	// "copy": indicates whether to copy the file into dest_dir, if not embedded
	void embed_file(std::ostream & out, const std::string & path, const std::string & type, bool copy);
		
private:
	// for string formatting
	string_formatter str_fmt;

	double default_ctm[6];

    Param const& param;
	TmpFiles& tmp_files;

	std::ofstream html_fout, css_fout;
	std::string html_path, css_path;

	static const std::string MANIFEST_FILENAME;
	static const char * format_str; // class names for each id
};

} // namespace pdf2htmlEX

#endif //HTMLDEVICE_H__
