/*
 * ff.h
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
extern "C" {
#else
#include <stdint.h>
#endif

void ff_init(void);
void ff_load_font(const char * filename);
void ff_reencode_glyph_order(void);
void ff_reencode_raw(int32_t * mapping, int mapping_len, int force);
void ff_reencode_raw2(char ** mapping, int mapping_len, int force);
void ff_cidflatten(void);
void ff_save(const char * filename);
void ff_close(void);
int ff_get_em_size(void);
int ff_get_max_ascent(void);
int ff_get_max_descent(void);
void ff_set_ascent(int a);
void ff_set_descent(int d);

#ifdef __cplusplus
}
#endif
