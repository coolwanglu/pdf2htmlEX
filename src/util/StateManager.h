/*
 * StateManager.h
 *
 * manage reusable CSS classes
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef STATEMANAGER_H__
#define STATEMANAGER_H__

#include <iostream>
#include <map>

#include "util/math.h"
#include "util/CSSClassNames.h"

namespace pdf2htmlEX {

template<class ValueType, class Imp> class StateManager {};

template<class Imp>
class StateManager<double, Imp>
{
public:
    StateManager()
        : eps(0)
        , imp(static_cast<Imp*>(this))
    { 
        reset();
    }

    // values no farther than eps are treated as equal
    void set_eps (double eps) { 
        this->eps = eps; 
    }

    // usually called at the beginning of a page
    void reset(void) { 
        value = imp->default_value(); 
        _install(value);
    }

    /*
     * install new_value if changed (equal() should be faster than map::lower_bound)
     * return if the state has been indeed changed
     */
    bool install(double new_value) {
        if(equal(new_value, value))
            return false;
        _install(new_value);
        return true;
    }

    long long get_id           (void) const { return id;           }
    double    get_value        (void) const { return value;        }
    double    get_actual_value (void) const { return actual_value; }

    void dump_css(std::ostream & out) {
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << imp->get_css_class_name() << iter->second << "{";
            imp->dump_value(out, iter->first);
            out << "}" << std::endl;
        }
    }

protected:
    // this version of install does not check if value has been updated
    // return if a new entry has been created
    bool _install(double new_value) {
        value = new_value;

        auto iter = value_map.lower_bound(new_value - eps);
        if((iter != value_map.end()) && (abs(iter->first - value) <= eps)) 
        {
            actual_value = iter->first;
            id = iter->second;
            return false;
        }

        actual_value = new_value;
        id = value_map.size();
        value_map.insert(std::make_pair(new_value, id));
        return true;
    }

    double eps;
    Imp * imp;

    long long id;
    double value; // the value we are tracking
    double actual_value; // the value we actually exported to HTML
    std::map<double, long long> value_map;
};

class FontSizeManager : public StateManager<double, FontSizeManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::FONT_SIZE_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "font-size:" << round(value) << "px;"; }
};

class LetterSpaceManager : public StateManager<double,  LetterSpaceManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::LETTER_SPACE_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "letter-spacing:" << round(value) << "px;"; }
};

class WordSpaceManager : public StateManager<double, WordSpaceManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::WORD_SPACE_CN;}
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "word-spacing:" << round(value) << "px;"; }
};

class RiseManager : public StateManager<double, RiseManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::RISE_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "top:" << round(-value) << "px;"; }
};

class WhitespaceManager : public StateManager<double, WhitespaceManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::WHITESPACE_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { 
        out << ((value > 0) ? "display:inline-block;width:" 
                            : "display:inline;margin-left:")
            << round(value) << "px;";
    }
};

class HeightManager : public StateManager<double, HeightManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::HEIGHT_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "height:" << round(value) << "px;"; }
};

class LeftManager : public StateManager<double, LeftManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::LEFT_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "left:" << round(value) << "px;"; }
};

} // namespace pdf2htmlEX 

#endif //STATEMANAGER_H__
