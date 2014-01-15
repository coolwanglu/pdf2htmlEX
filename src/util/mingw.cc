/*
 * Win32 specific functions
 *
 * by MarcSanfacon
 * 2014.01.13
 */

#ifdef __MINGW32__

#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <limits.h>
#include <libgen.h>

#include "mingw.h"

using namespace std;

char* mkdtemp(char* temp)
{
    char *filename = nullptr;
    if (temp != nullptr) {
        filename = mktemp(temp);
        if (filename != nullptr) {
            if (_mkdir(temp) != 0) {
                filename = nullptr;
            }
        }
    }

    return filename;
}

namespace pdf2htmlEX {
string get_exec_dir(char *dir)
{
    // Under Windows, the default data_dir is under /data in the pdf2htmlEX directory
    string s = dirname(dir);
    if (s == ".") {
        char* wd(getcwd(nullptr, PATH_MAX));
        s = wd;
        free(wd);
    }
    s += "/data";
    return s;
}

string get_tmp_dir()
{
    // Under Windows, the temp path is not under /tmp, find it.
    char *tmp = getenv("TMP");
    if (tmp == nullptr) {
        tmp = getenv("TEMP");
    }

    return tmp != nullptr ? string(tmp) + "/" : "/";
}

} // namespace pdf2htmlEX;

#endif //__MINGW32__

