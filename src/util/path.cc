/*
 * Functions manipulating filenames and paths
 *
 * by WangLu
 * 2012.11.29
 */

#include <errno.h>
#include <regex>
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

string sanitize_filename(const string & filename, bool allow_single_format_number)
{
    // First, escape all %'s to make safe for use in printf.
    string sanitized = std::regex_replace(filename, std::regex("%"), "%%");
    
    if(allow_single_format_number)
    {
        // A single %d or %0xd is allowed in the input.
        sanitized = std::regex_replace(sanitized, std::regex("%%([0-9]*)d"), "%$1d", std::regex_constants::format_first_only);
    }
    
    return sanitized;
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
