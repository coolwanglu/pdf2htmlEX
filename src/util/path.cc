/*
 * Functions manipulating filenames and paths
 *
 * by WangLu
 * 2012.11.29
 */

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

#include "path.h"

#ifdef __MINGW32__
#include "util/mingw.h"
#endif

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

bool sanitize_filename(string & filename)
{
    string sanitized;
    bool format_specifier_found = false;

    for(size_t i = 0; i < filename.size(); i++)
    {
        if('%' == filename[i])
        {
            if(format_specifier_found)
            {
                sanitized.push_back('%');
                sanitized.push_back('%');
            }
            else
            {
                // We haven't found the format specifier yet, so see if we can use this one as a valid formatter
                size_t original_i = i;
                string tmp;
                tmp.push_back('%');
                while(++i < filename.size())
                {
                    tmp.push_back(filename[i]);

                    // If we aren't still in option specifiers, stop looking
                    if(!strchr("0123456789", filename[i]))
                    {
                        break;
                    }
                }

                // Check to see if we yielded a valid format specifier
                if('d' == tmp[tmp.size()-1])
                {
                    // Found a valid integer format
                    sanitized.append(tmp);
                    format_specifier_found = true;
                }
                else
                {
                    // Not a valid format specifier. Just append the protected %
                    // and keep looking from where we left of in the search
                    sanitized.push_back('%');
                    sanitized.push_back('%');
                    i = original_i;
                }
            }
        }
        else
        {
            sanitized.push_back(filename[i]);
        }
    }

    // Only sanitize if it is a valid format.
    if(format_specifier_found)
    {
        filename.assign(sanitized);
    }

    return format_specifier_found;
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
        for(auto & c : s)
            c = tolower(c);
        return s;
    }
}


} //namespace pdf2htmlEX
