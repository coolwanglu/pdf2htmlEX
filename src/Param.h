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
    int first_page, last_page;

    double h_dpi, v_dpi;
    double h_eps, v_eps;

    int readable;
};


#endif //PARAM_h__
