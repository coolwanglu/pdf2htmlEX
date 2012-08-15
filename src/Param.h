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

    double h_dpi, v_dpi;
    double h_dpi2, v_dpi2;
    double h_eps, v_eps;

    int process_nontext;
    int single_html;

    int debug;
    int clean_tmp;
};


#endif //PARAM_h__
