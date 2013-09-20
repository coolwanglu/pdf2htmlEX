/*
 * ffw.h : Fontforge Wrapper
 *
 * Processing fonts using Fontforge
 *
 * fontforge.h cannot be included in C++
 * So this wrapper in C publishes several functions we need
 *
 * by WangLu
 * 2012.09.03
 */


#ifdef __cplusplus
#include <cstdint>
namespace pdf2htmlEX {
extern "C" {
#else
#include <stdint.h>
#endif

////////////////////////
// global
void ffw_init(int debug);
void ffw_finalize(void);
long ffw_get_version(void);

////////////////////////
// load & save
void ffw_new_font();
void ffw_load_font(const char * filename);
void ffw_prepare_font(void);

void ffw_save(const char * filename);
void ffw_close(void);

////////////////////////
// encoding
void ffw_reencode_glyph_order(void);
void ffw_reencode_unicode_full(void);
void ffw_reencode_raw(int32_t * mapping, int mapping_len, int force);
void ffw_reencode_raw2(char ** mapping, int mapping_len, int force);

void ffw_cidflatten(void);
// add a new empty char into the font
void ffw_add_empty_char(int32_t unicode, int width);

////////////////////////
// metrics
int ffw_get_em_size(void);
// manipulate ascent and descent
// asscent is between 0 and 1
// descent is between -1 and 0
void ffw_fix_metric();
// get ascent/descent based on the shape
void ffw_get_metric(double * ascent, double * descent);
// set corresponding fields
void ffw_set_metric(double ascent, double descent);

void ffw_set_widths(int * width_list, int mapping_len, 
        int stretch_narrow, int squeeze_wide);

////////////////////////
// others
// (ox,oy) is the position of the true origin, fractions related to em_size
void ffw_import_svg_glyph(int code, const char * filename, double ox, double oy);
void ffw_auto_hint(void);
void ffw_override_fstype(void);

#ifdef __cplusplus
}
}
#endif
