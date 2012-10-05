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


void ffw_init(int debug);
void ffw_fin(void);
void ffw_load_font(const char * filename);
void ffw_reencode_glyph_order(void);
void ffw_reencode_raw(int32_t * mapping, int mapping_len, int force);
void ffw_reencode_raw2(char ** mapping, int mapping_len, int force);
void ffw_cidflatten(void);
void ffw_save(const char * filename);
void ffw_close(void);

int ffw_get_em_size(void);
// fix metrics and get them
void ffw_metric(double * ascent, double * descent);

void ffw_set_widths(int * width_list, int mapping_len, 
        int stretch_narrow, int squeeze_wide, 
        int remove_unused);

void ffw_auto_hint(void);

#ifdef __cplusplus
}
}
#endif
