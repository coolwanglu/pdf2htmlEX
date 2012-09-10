/*
 * A wrapper of getopt
 *
 * by WangLu
 * 2012.09.10
 */

#include <getopt.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cassert>

#include "ArgParser.h"

using std::ostream;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::unordered_map;
using std::make_pair;
using std::ostringstream;

ArgParser::~ArgParser(void)
{
    for(auto iter = arg_entries.begin(); iter != arg_entries.end(); ++iter)
        delete (*iter);
    for(auto iter = optional_arg_entries.begin(); iter != optional_arg_entries.end(); ++iter)
        delete (*iter);
}

ArgParser & ArgParser::add(const char * optname, const char * description, ArgParserCallBack callback)
{
    return add<char>(optname, nullptr, 0, description, callback);
}

void ArgParser::parse(int argc, char ** argv) const
{
    //prepare optstring and longopts
    vector<char> optstring;
    optstring.reserve(arg_entries.size() + 1);
    vector<struct option> longopts;
    longopts.reserve(arg_entries.size() + 1);

    unordered_map<int, const ArgEntryBase*> opt_map;

    for(auto iter = arg_entries.begin(); iter != arg_entries.end(); ++iter)
    {
        const ArgEntryBase * p = *iter;
        if(p->shortname != 0)
        {
            optstring.push_back(p->shortname);
            if(p->need_arg)
                optstring.push_back(':');

            int v = p->shortname;
            if(!(opt_map.insert(make_pair(v, p)).second))
            {
                cerr << "Warning: duplicated shortname '" << v << "' used by -" << (char)(p->shortname) << " and -" << (char)(opt_map[p->shortname]->shortname) << endl;
            }
        }

        if(p->name != "")
        {
            int v = (256 + (iter - arg_entries.begin()));
            longopts.push_back({p->name.c_str(), ((p->need_arg) ? required_argument : no_argument), nullptr, v});           
            if(!(opt_map.insert(make_pair(v, p)).second))
            {
                cerr << "Warning: duplicated shortname '" << v << "' used by --" << (p->name) << " and --" << (opt_map[p->shortname]->name) << endl;
            }
        }
    }

    optstring.push_back(0);
    longopts.push_back({0,0,0,0});

    {
        int r;
        int idx;
        opterr = 0;
        while(true)
        {
            r = getopt_long(argc, argv, &optstring.front(), &longopts.front(), &idx); 
            if(r == -1)
                break;
            assert(r != ':');
            if(r == '?')
            {
                ostringstream sout;
                assert(optopt < 256);
                throw string() + ((opt_map.find(optopt) == opt_map.end()) ? "Unknown option: -" : "Missing argument for option -") + (char)optopt;
            }    

            auto iter = opt_map.find(r);
            assert(iter != opt_map.end());
            iter->second->parse(optarg);
        }
    }

    {
        int i = optind;
        auto iter = optional_arg_entries.begin();
        while((i < argc) && (iter != optional_arg_entries.end())) 
        {
            (*(iter++))->parse(argv[i++]);
        }
    }
}

void ArgParser::show_usage(ostream & out) const
{
    for(auto iter = arg_entries.begin(); iter != arg_entries.end(); ++iter)
    {
        (*iter)->show_usage(out);
    }
}

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
            cerr << "Warning: argument '" << this->name << "' may not be parsed correctly" << endl;
        }
    }
}


void dump_default_value(std::ostream & out, const std::string & v)
{
    out << '"' << v << '"';
}

const int ArgParser::arg_col_width = 40;
