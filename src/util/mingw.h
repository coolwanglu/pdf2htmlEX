/*
 * Win32 specific functions
 *
 * by MarcSanfacon
 * 2014.01.13
 */

#ifndef MINGW_H__
#define MINGW_H__

#ifdef __MINGW32__

#include <io.h>

char *mkdtemp(char *temp);

#include <direct.h>
#define mkdir(A, B) _mkdir(A)
#define stat _stat

namespace pdf2htmlEX {
    std::string     get_exec_dir(char *dir);
    std::string     get_tmp_dir();
} // namespace pdf2htmlEX

#endif //__MINGW32__

#endif //MINGW_H__

