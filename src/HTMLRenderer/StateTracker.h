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

#include <map>

#include <GfxState.h>

namespace pdf2htmlEX {

template<class ValueType, class Imp> class StateTracker {};

template<class Imp>
class StateTracker<double>
{
public:
    // values no father than eps are treated as equal
    StateTracker(const char * css_class_prefix, double eps)
        : css_class_prefix(css_class_prefix)
        , eps(eps)
        , changed(false)
        , imp(static_cast<Imp>(this))
    { }

    // usually called at the beginning of a page
    void reset(void) { imp->reset(); }

    // is called when PDF updates the state
    void update(GfxState * state) { changed = true; }
    /*
     * retrive the new state if update() is called recently, or force == true
     * return if the state has been indeed changed
     */
    bool sync(GfxState * state, bool force)| {
        if(!(changed || force))
            return false;

        changed = false;

        double new_value = imp->get_value(state);
        if(equal(new_value, value))
            return false;

        install(new_value);
        return true;
    }
        

    long long install(double new_value) {
        value = new_value;

        auto iter = value_map.lower_bound(new_value - eps);
        if((iter != value_map.end()) && (abs(iter->first - value) <= eps)) {
            actual_value = iter->first;
            return iter->second;
        }

        actual_value = new_value;
        long long new_id = map.size();
        map.insert(make_pair(new_value, new_id));

        imp->create(new_id, new_value);

        return new_id;
    }
private:
    const char * css_class_prefix;
    const double eps;
    bool changed;
    Imp * imp;


    long long id;
    double value; // the value we are tracking
    double actual_value; // the value we actually exported to HTML
    std::map<double, long long> value_map;
};


} // namespace pdf2htmlEX 

#endif //STATETRACKER_H__
