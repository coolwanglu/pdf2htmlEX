/*
 * TmpFiles.cc
 *
 * Collect and clean-up temporary files
 *
 * implemented by WangLu
 * split off by Filodej <philodej@gmail.com>
 */

#include <iostream>
#include <cstdio>
#include <sys/stat.h>

#include "TmpFiles.h"
#include "Param.h"

using namespace std;

#ifndef _WIN32
#   define STAT stat
#   define RMDIR rmdir
#else
#   include <direct.h>
#   define STAT _stat
#   define RMDIR _rmdir
#endif

namespace pdf2htmlEX {


TmpFiles::TmpFiles( const Param& param )
    : param( param )
{ }

TmpFiles::~TmpFiles()
{
    clean();
}

void TmpFiles::add( const string & fn)
{
    if(!param.clean_tmp)
        return;

    if(tmp_files.insert(fn).second && param.debug)
        cerr << "Add new temporary file: " << fn << endl;
}

// Return the total size of the temporary files in bytes
double TmpFiles::get_total_size() const
{
    double total_size = 0;
    struct STAT st;
    for(auto iter = tmp_files.begin(); iter != tmp_files.end(); ++iter) {
        STAT(iter->c_str(), &st);
        total_size += st.st_size;
    }

    return total_size;
}


void TmpFiles::clean()
{
    if(!param.clean_tmp)
        return;

    for(auto iter = tmp_files.begin(); iter != tmp_files.end(); ++iter)
    {
        const string & fn = *iter;
        remove(fn.c_str());
        if(param.debug)
            cerr << "Remove temporary file: " << fn << endl;
    }

    RMDIR(param.tmp_dir.c_str());
    if(param.debug)
        cerr << "Remove temporary directory: " << param.tmp_dir << endl;
}

} // namespace pdf2htmlEX

