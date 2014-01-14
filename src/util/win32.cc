/*
 * Win32 specific functions
 *
 * by MarcSanfacon
 * 2014.01.13
 */

#ifdef _WIN32

#include <string>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <sstream>
#include <windows.h>

#include "win32.h"

char* mkdtemp(char* temp)
{
    if (temp != nullptr) {
        bool created = false;
        char value[30];

        srand((unsigned)time(0));
        while (!created) {
            int rand_value = (int)((rand() / ((double)RAND_MAX+1.0)) * 1e6);
            sprintf(value, "%06d", rand_value);
            sprintf(temp + strlen(temp) - 6, "%6.6s", value);
            created = _mkdir(temp) == 0;
        }
    }

    return temp;
}

namespace pdf2htmlEX {
std::string get_data_dir(char *dir)
{
    // Under Windows, the default data_dir is under /data in the pdf2htmlEX directory
    std::stringstream ss;
    ss << dirname(dir) << "/data";
    return ss.str();
}

std::string get_tmp_dir()
{
    // Under Windows, the temp path is not under /tmp, find it.
    char temppath[MAX_PATH];
    ::GetTempPath(MAX_PATH, temppath);
    return temppath;
}

} // namespace pdf2htmlEX;

#endif //_WIN32

