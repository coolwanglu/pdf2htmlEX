/*
 * Parameters
 *
 * Wang Lu
 * 2012.08.03
 */


#ifndef PARAM_H__
#define PARAM_H__

#include <string>

struct Param
{
    std::string owner_password, user_password;
    std::string input_filename, output_filename;

    std::string dest_dir, tmp_dir;

    int first_page, last_page;

    double zoom;
    double font_size_multiplier;
    double h_dpi, v_dpi;
    double h_eps, v_eps;

    int process_nontext;
    int single_html;

    int debug;
    int clean_tmp;
    int only_metadata;
};


#endif //PARAM_h__
