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

#include <GfxState.h>

namespace pdf2htmlEX {

template<class ValueType, class Imp> class StateTracker {};

template<class Imp>
class StateTracker<double, Imp>
{
public:
    StateTracker()
        : eps(0)
        , changed(false)
        , imp(static_cast<Imp>(this))
    { 
        reset();
    }

    // values no father than eps are treated as equal
    void set_param (const char * css_class_name, double eps) { 
        this->css_class_name = css_class_name;
        this->eps = eps; 
    }

    // usually called at the beginning of a page
    void reset(void) { value = imp->default_value(); }

    // is called when PDF updates the state
    void update(GfxState * state) { changed = true; }
    /*
     * retrive the new state if update() is called recently, or force == true
     * return if the state has been indeed changed
     */
    bool sync(GfxState * state, bool force) {
        if(!(changed || force))
            return false;

        changed = false;

        double new_value = imp->get_value(state);
        if(equal(new_value, value))
            return false;

        install(new_value);
        return true;
    }
    
    double get_actual_value (void) const { return actual_value; }
    
    long long install(double new_value) {
        value = new_value;

        auto iter = value_map.lower_bound(new_value - eps);
        if((iter != value_map.end()) && (abs(iter->first - value) <= eps)) 
        {
            actual_value = iter->first;
            return iter->second;
        }

        actual_value = new_value;
        long long new_id = map.size();
        map.insert(make_pair(new_value, new_id));

        return new_id;
    }

    void dump_css(std::ostream & out) {
        for(auto iter = map.begin(); iter != map.end(); ++iter)
        {
            out << "." << css_class_name << iter->second << "{";
            imp->dump_value(out, iter->first);
            out << "}" << std::endl;
        }
    }

protected:
    const char * css_class_name;
    double eps;
    bool changed;
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
    void dump_value(std::ostream & out, double value) { out "font-size:" << round(value) << "px;"; }
    }
};

class LetterSpaceTracker : public StateTracker<double, LetterSpaceTracker>
{
public:
    double default_value(void) { return 0; }
    double get_value(GfxState * state) { return state->getCharSpace(); }
    void dump_value(std::ostream & out, double value) { out "font-size:" << round(value) << "px;"; }
    }
};



        std::map<Matrix, long long, Matrix_less> transform_matrix_map;
        std::map<double, long long> letter_space_map;
        std::map<double, long long> word_space_map;
        std::unordered_map<GfxRGB, long long, GfxRGB_hash, GfxRGB_equal> fill_color_map, stroke_color_map; 
        std::map<double, long long> whitespace_map;
        std::map<double, long long> rise_map;
        std::map<double, long long> height_map;
        std::map<double, long long> left_map;


} // namespace pdf2htmlEX 

#endif //STATETRACKER_H__
