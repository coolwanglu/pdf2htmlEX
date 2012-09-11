/*
 * A wrapper of getopt
 *
 * by WangLu
 * 2012.09.10
 */


#ifndef ARGPARSER_H__
#define ARGPARSER_H__

#include <string>
#include <vector>
#include <ostream>
#include <sstream>

#ifndef nullptr
#define nullptr (NULL)
#endif

namespace pdf2htmlEX {

//helper
template<class T>
bool read_value(const char * arg, T * location)
{
    std::istringstream sin(arg);
    return ((sin >> (*location)) && (sin.eof()));
}

extern bool read_value(const char * arg, char * location);
extern bool read_value(const char * arg, std::string * location);

template<class T>
void dump_value(std::ostream & out, const T & v)
{
    out << v;
}

extern void dump_value(std::ostream & out, const std::string & v);


class ArgParser
{
    public:
        ~ArgParser(void);

        typedef void (*ArgParserCallBack) (const char * arg);

        /*
         * optname: name of the argment, should be provided as --optname
         * description: if description is "", the argument won't be shown in show_usage()
         */

        ArgParser & add(const char * optname, const char * description, ArgParserCallBack callback = nullptr);

        template <class T, class Tv>
            ArgParser & add(const char * optname, T * location, const Tv & default_value, const char * description, ArgParserCallBack callback = nullptr);

        void parse(int argc, char ** argv) const;
        void show_usage(std::ostream & out) const;

    private:
        class ArgEntryBase
        {
            public:
                ArgEntryBase(const char * name, const char * description, bool need_arg);
                virtual ~ArgEntryBase() { }
                char shortname;
                std::string name;
                std::string description;
                bool need_arg;
                virtual void parse (const char * arg) const = 0;
                virtual void show_usage (std::ostream & out) const = 0;
        };

        template <class T, class Tv>
        class ArgEntry : public ArgEntryBase
        {
            public:
                ArgEntry(const char * name, T * location, const Tv & deafult_value, ArgParserCallBack callback, const char * description);

                virtual void parse (const char * arg) const;
                virtual void show_usage (std::ostream & out) const;

            private:
                T * location;
                T default_value;
                ArgParserCallBack callback;
        };

        std::vector<ArgEntryBase *> arg_entries, optional_arg_entries;
        static const int arg_col_width;
};

template<class T, class Tv>
ArgParser & ArgParser::add(const char * optname, T * location, const Tv & default_value, const char * description, ArgParserCallBack callback)
{
    // use "" in case nullptr is provided
    if((!optname) || (!optname[0]))
        optional_arg_entries.push_back(new ArgEntry<T, Tv>("", location, default_value, callback, ""));
    else
        arg_entries.push_back(new ArgEntry<T, Tv>(optname, location, default_value, callback, description));

    return *this;
}

template<class T, class Tv>
ArgParser::ArgEntry<T, Tv>::ArgEntry(const char * name, T * location, const Tv & default_value,  ArgParserCallBack callback, const char * description)
    : ArgEntryBase(name, description, (location != nullptr))
      , location(location)
      , default_value(default_value)
      , callback(callback)
{ 
    if(need_arg)
        *location = T(default_value);
}

template<class T, class Tv>
void ArgParser::ArgEntry<T, Tv>::parse(const char * arg) const
{ 
    if(need_arg)
    { 
        if(!arg)
            throw std::string("Missing argument of option: --") + name;

        if(!read_value(arg, location))
            throw std::string("Invalid argument: ") + arg;
    }

    if(callback)
        (*callback)(arg); 
}

template<class T, class Tv>
void ArgParser::ArgEntry<T, Tv>::show_usage(std::ostream & out) const
{ 
    if(description == "")
        return;

    std::ostringstream sout;
    sout << "  ";

    if(shortname != 0)
    {
        sout << "-" << shortname;
    }

    if(name != "")
    {
        if(shortname != 0)
            sout << ",";
        sout << "--" << name;
    }

    if(need_arg)
    {
        sout << " <arg> (=";
        dump_value(sout, default_value);
        sout << ")";
    }

    std::string s = sout.str();
    out << s;

    for(int i = s.size(); i < arg_col_width; ++i)
        out << ' ';

    out << " " << description << std::endl;
}

} // namespace ArgParser

#endif //ARGPARSER_H__
