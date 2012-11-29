#include <sys/stat.h>
#include <sys/types.h>

#include "path.h"

using std::string;

namespace pdf2htmlEX {

void create_directories(const string & path)
{
    if(path.empty()) return;

    size_t idx = path.rfind('/');
    if(idx != string::npos)
    {
        create_directories(path.substr(0, idx));
    }
            
    int r = mkdir(path.c_str(), S_IRWXU);
    if(r != 0)
    {
        if(errno == EEXIST)
        {
            struct stat stat_buf;
            if((stat(path.c_str(), &stat_buf) == 0) && S_ISDIR(stat_buf.st_mode))
                return;
        }

        throw string("Cannot create directory: ") + path;
    }
}

bool is_truetype_suffix(const string & suffix)
{
    return (suffix == ".ttf") || (suffix == ".ttc") || (suffix == ".otf");
}

string get_filename (const string & path)
{
    size_t idx = path.rfind('/');
    if(idx == string::npos) 
        return path;
    else if (idx == path.size() - 1)
        return "";
    return path.substr(idx + 1);
}

string get_suffix(const string & path)
{
    string fn = get_filename(path);
    size_t idx = fn.rfind('.');
    if(idx == string::npos)
        return "";
    else
    {
        string s = fn.substr(idx);
        for(auto iter = s.begin(); iter != s.end(); ++iter)
            *iter = tolower(*iter);
        return s;
    }
}


} //namespace pdf2htmlEX 
