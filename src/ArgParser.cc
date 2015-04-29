/*
 * A wrapper of getopt
 *
 * by WangLu
 * 2012.09.10
 */

#include <iostream>
#include <unordered_map>
#include <cassert>

#include <getopt.h>

#include "ArgParser.h"

namespace pdf2htmlEX {

using std::ostream;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::unordered_map;
using std::make_pair;
using std::ostringstream;

bool read_value(const char * arg, char * location)
{
    *location = arg[0];
    return (arg[1] == 0);
}

bool read_value(const char * arg, std::string * location)
{
    *location = std::string(arg);
    return true;
}

void dump_value(std::ostream & out, const std::string & v)
{
    out << '"' << v << '"';
}

ArgParser & ArgParser::add(const char * optname, const char * description, ArgParserCallBack callback, bool need_arg)
{
    // ArgEntry does not accept nullptr as optname nor description
    if((!optname) || (!optname[0]))
    {
        // when optname is nullptr or "", it's optional, and description is dropped
        optional_arg_entries.emplace_back(new ArgEntry<string, string>("", "", callback, need_arg));
    }
    else
    {
        arg_entries.emplace_back(new ArgEntry<string, string>(optname, (description ? description : ""), callback, need_arg));
    }

    return *this;
}

void ArgParser::parse(int argc, char ** argv) const
{
    //prepare optstring and longopts
    vector<char> optstring;
    optstring.reserve(2*arg_entries.size() + 1);
    vector<struct option> longopts;
    longopts.reserve(arg_entries.size() + 1);

    unordered_map<int, const ArgEntryBase*> opt_map;

    for(auto iter = arg_entries.begin(); iter != arg_entries.end(); ++iter)
    {
        const auto * p = iter->get();
        if(p->shortname != 0)
        {
            optstring.push_back(p->shortname);
            if(p->need_arg)
                optstring.push_back(':');

            int v = p->shortname;
            if(!(opt_map.insert(make_pair(v, p)).second))
            {
                cerr << "Warning: duplicated shortname: " << v << endl;
            }
        }

        if(p->name != "")
        {
            int v = (256 + (iter - arg_entries.begin()));
            longopts.resize(longopts.size() + 1);
            {
                auto & cur = longopts.back();
                cur.name = p->name.c_str();
                cur.has_arg = ((p->need_arg) ? required_argument : no_argument);
                cur.flag = nullptr;
                cur.val = v;
            }
            if(!(opt_map.insert(make_pair(v, p)).second))
            {
                cerr << "Warning: duplicated long name: " << (p->name) << endl;
            }
        }
    }

    optstring.push_back(0);
    longopts.resize(longopts.size() + 1);
    {
        auto & cur = longopts.back();
        cur.name = 0;
        cur.has_arg = 0;
        cur.flag = 0;
        cur.val = 0;
    }

    {
        opterr = 1;
        int r;
        int idx;
        while(true)
        {
            r = getopt_long(argc, argv, &optstring.front(), &longopts.front(), &idx); 
            if(r == -1)
                break;
            assert(r != ':');
            if(r == '?')
            {
                throw "";
            }    

            auto iter = opt_map.find(r);
            assert(iter != opt_map.end());
            iter->second->parse(optarg);
        }
    }

    {
        auto iter = optional_arg_entries.begin();
        while((optind < argc) && (iter != optional_arg_entries.end())) 
        {
            (*(iter++))->parse(argv[optind++]);
        }
    }
}

void ArgParser::show_usage(ostream & out) const
{
    for(auto & entry : arg_entries)
    {
        entry->show_usage(out);
    }
}

template<> const char * ArgParser::get_type_name<int>    (void) { return "int";    }
template<> const char * ArgParser::get_type_name<double> (void) { return "fp";     }
template<> const char * ArgParser::get_type_name<string> (void) { return "string"; }

ArgParser::ArgEntryBase::ArgEntryBase(const char * name, const char * description, bool need_arg)
    : shortname(0), name(name), description(description), need_arg(need_arg)
{ 
    size_t idx = this->name.rfind(',');
    if(idx != string::npos)
    {
        if(idx+2 == this->name.size())
        {
            shortname = this->name[this->name.size()-1];
            this->name = this->name.substr(0, idx);
        }
        else
        {
            cerr << "Warning: argument '" << this->name << "' cannot be parsed as a short option" << endl;
        }
    }
}

const int ArgParser::arg_col_width = 31;

} // namespace pdf2htmlEX
