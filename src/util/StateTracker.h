/*
 * StateTracker.h
 *
 * track specific PDF state
 * manage reusable CSS classes
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef STATETRACKER_H__
#define STATETRACKER_H__

#include <iostream>
#include <map>

#include "util/math.h"

namespace pdf2htmlEX {

template<class ValueType, class Imp> class StateTracker {};

template<class Imp>
class StateTracker<double, Imp>
{
public:
    StateTracker()
        : eps(0)
        , imp(static_cast<Imp*>(this))
    { 
        reset();
    }

    // values no father than eps are treated as equal
    void set_param (const char * css_class_name, double eps) { 
        this->css_class_name = css_class_name;
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
        return _install(new_value);
    }

    long long get_id (void) const { return id; }
    double get_actual_value (void) const { return actual_value; }

    void dump_css(std::ostream & out) {
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << css_class_name << iter->second << "{";
            imp->dump_value(out, iter->first);
            out << "}" << std::endl;
        }
    }

protected:
    // this version of install does not check if value has been updated
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

    const char * css_class_name;
    double eps;
    Imp * imp;

    long long id;
    double value; // the value we are tracking
    double actual_value; // the value we actually exported to HTML
    std::map<double, long long> value_map;
};

class FontSizeTracker : public StateTracker<double, FontSizeTracker>
{
public:
    double default_value(void) { return 0; }
    double get_value(GfxState * state) { return state->getFontSize(); }
    void dump_value(std::ostream & out, double value) { out << "font-size:" << round(value) << "px;"; }
};

class LetterSpaceTracker : public StateTracker<double, LetterSpaceTracker>
{
public:
    double default_value(void) { return 0; }
    double get_value(GfxState * state) { return state->getCharSpace(); }
    void dump_value(std::ostream & out, double value) { out << "letter-spacing:" << round(value) << "px;"; }
};

} // namespace pdf2htmlEX 

#endif //STATETRACKER_H__
