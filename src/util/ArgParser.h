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
         * The 1st is for arg without arguments (i.e. flags), and the 2nd is for general args.
         * optname:
         *  - if not nullptr, it should be the name of the arg, should be in the format of "<long name>[,<short char>]", e.g. "help,h"
         *  - if nullptr, it denotes an optional arg, and description will be ignored
         * description:
         *  - if description is nullptr or "", the argument won't be shown in show_usage()
         */
        ArgParser & add(const char * optname, const char * description, ArgParserCallBack callback = nullptr);
        template <class T, class Tv>
        ArgParser & add(const char * optname, T * location, const Tv & default_value, const char * description, ArgParserCallBack callback = nullptr, bool dont_show_default = false);

        void parse(int argc, char ** argv) const;
        void show_usage(std::ostream & out) const;

    private:
        // type names helper
        template<class> 
        static const char * get_type_name(void) { return "unknown"; }

        class ArgEntryBase
        {
            public:
                /* name or description cannot be nullptr */
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
                ArgEntry(const char * name, 
                        T * location, const Tv & deafult_value, 
                        ArgParserCallBack callback, 
                        const char * description, bool dont_show_default);
                

                virtual void parse (const char * arg) const;
                virtual void show_usage (std::ostream & out) const;

            private:
                T * location;
                T default_value;
                ArgParserCallBack callback;
                bool dont_show_default;
        };

        std::vector<ArgEntryBase *> arg_entries, optional_arg_entries;
        static const int arg_col_width;
};

template<class T, class Tv>
ArgParser & ArgParser::add(const char * optname, T * location, const Tv & default_value, const char * description, ArgParserCallBack callback, bool dont_show_default)
{
    // ArgEntry does not accept nullptr as optname nor description
    if((!optname) || (!optname[0]))
    {
        // when optname is nullptr or "", it's optional, and description is dropped
        optional_arg_entries.push_back(new ArgEntry<T, Tv>("", location, default_value, callback, "", dont_show_default));
    }
    else
    {
        arg_entries.push_back(new ArgEntry<T, Tv>(optname, location, default_value, callback, (description ? description : ""), dont_show_default));
    }

    return *this;
}

// Known types
template<> const char * ArgParser::get_type_name<int>         (void);
template<> const char * ArgParser::get_type_name<double>      (void);
template<> const char * ArgParser::get_type_name<std::string> (void);

template<class T, class Tv>
ArgParser::ArgEntry<T, Tv>::ArgEntry(const char * name, T * location, const Tv & default_value,  ArgParserCallBack callback, const char * description, bool dont_show_default)
    : ArgEntryBase(name, description, (location != nullptr))
      , location(location)
      , default_value(default_value)
      , callback(callback)
      , dont_show_default(dont_show_default)
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
    if(description.empty())
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
        sout << " <" << get_type_name<T>() << ">";
    }

    std::string s = sout.str();
    out << s;

    for(int i = s.size(); i < arg_col_width; ++i)
        out << ' ';
    
    out << " " << description;
    
    if(need_arg && !dont_show_default)
    {
        out << " (default: ";
        dump_value(out, default_value);
        out << ")";	
    }
    
    out << std::endl;
}

} // namespace ArgParser

#endif //ARGPARSER_H__
