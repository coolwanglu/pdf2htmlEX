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
#include <unordered_map>

#include "util/math.h"
#include "util/css_const.h"

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
        cur_value = imp->default_value();
        cur_id = install(cur_value, &cur_actual_value);
    }

    /*
     * update the current state, which will be installed automatically
     * return if the state has been indeed changed
     */
    bool update(double new_value) {
        if(equal(new_value, cur_value))
            return false;
        cur_value = new_value;
        cur_id = install(cur_value, &cur_actual_value);
        return true;
    }

    // install new_value into the map, but do not update the state
    // return the corresponding id, and set 
    long long install(double new_value, double * actual_value_ptr = nullptr) {
        auto iter = value_map.lower_bound(new_value - eps);
        if((iter != value_map.end()) && (abs(iter->first - new_value) <= eps)) 
        {
            if(actual_value_ptr != nullptr)
                *actual_value_ptr = iter->first;
            return iter->second;
        }

        long long id = value_map.size();
        double v = value_map.insert(std::make_pair(new_value, id)).first->first;
        if(actual_value_ptr != nullptr)
            *actual_value_ptr = v;
        return id;
    }

    // get current state
    long long get_id           (void) const { return cur_id;           }
    double    get_value        (void) const { return cur_value;        }
    double    get_actual_value (void) const { return cur_actual_value; }

    void dump_css(std::ostream & out) {
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << imp->get_css_class_name() << iter->second << "{";
            imp->dump_value(out, iter->first);
            out << "}" << std::endl;
        }
    }

    void dump_print_css(std::ostream & out, double scale) {
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << imp->get_css_class_name() << iter->second << "{";
            imp->dump_print_value(out, iter->first, scale);
            out << "}" << std::endl;
        }
    }

protected:
    double eps;
    Imp * imp;

    long long cur_id;
    double cur_value; // the value we are tracking
    double cur_actual_value; // the value we actually exported to HTML
    std::map<double, long long> value_map;
};

// Be careful about the mixed usage of Matrix and const double *
// the input is usually double *, which might be changed, so we have to copy the content out
// in the map we use Matrix instead of double * such that the array may be automatically release when deconstructign
// since the address of cur_value.m cannot be changed, we can export double * instead of Matrix
template <class Imp>
class StateManager<Matrix, Imp>
{
public:
    StateManager()
        : imp(static_cast<Imp*>(this))
    { }

    void reset(void) {
        memcpy(cur_value.m, imp->default_value(), sizeof(cur_value.m));
        cur_id = install(cur_value);
    }

    // return if changed
    bool update(const double * new_value) {
        // For a transform matrix m
        // m[4] & m[5] have been taken care of
        if(tm_equal(new_value, cur_value.m, 4))
            return false;

        memcpy(cur_value.m, new_value, sizeof(cur_value.m));
        cur_id = install(cur_value);
        return true;
    }

    long long get_id                (void) const { return cur_id;           }
    const double * get_value        (void) const { return cur_value.m;      }

    void dump_css(std::ostream & out) {
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << imp->get_css_class_name() << iter->second << "{";
            imp->dump_value(out, iter->first);
            out << "}" << std::endl;
        }
    }

    void dump_print_css(std::ostream & out, double scale) {}

protected:
    // return id
    long long install(const Matrix & new_value) {
        auto iter = value_map.lower_bound(new_value);
        if((iter != value_map.end()) && (tm_equal(new_value.m, iter->first.m, 4)))
        {
            return iter->second;
        }

        long long id = value_map.size();
        value_map.insert(std::make_pair(new_value, id));
        return id;
    }

    Imp * imp;

    long long cur_id;
    Matrix cur_value;

    class Matrix_less
    {
    public:
        bool operator () (const Matrix & m1, const Matrix & m2) const
        {
            // Note that we only care about the first 4 elements
            for(int i = 0; i < 4; ++i)
            {
                if(m1.m[i] < m2.m[i] - EPS)
                    return true;
                if(m1.m[i] > m2.m[i] + EPS)
                    return false;
            }
            return false;
        }
    };

    std::map<Matrix, long long, Matrix_less> value_map;
};

template <class Imp>
class StateManager<GfxRGB, Imp>
{
public:
    StateManager()
        : imp(static_cast<Imp*>(this))
    { }

    void reset(void) {
        cur_is_transparent = true;
        cur_id = -1;
    }

    bool update(const GfxRGB & new_value) {
        if((!cur_is_transparent) && gfxrgb_equal_obj(new_value, cur_value))
            return false;
        cur_value = new_value;
        cur_is_transparent = false;
        cur_id = install(cur_value);
        return true;
    }

    bool update_transparent (void) { 
        if(cur_is_transparent)
            return false;
        cur_is_transparent = true;
        cur_id = -1;
        return true;
    }


    long long get_id                (void) const { return cur_id;             }
    const GfxRGB & get_value        (void) const { return cur_value;          }
    bool get_is_transparent         (void) const { return cur_is_transparent; }

    void dump_css(std::ostream & out) {
        out << "." << imp->get_css_class_name() << CSS::INVALID_ID << "{";
        imp->dump_transparent(out);
        out << "}" << std::endl;

        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << imp->get_css_class_name() << iter->second << "{";
            imp->dump_value(out, iter->first);
            out << "}" << std::endl;
        }
    }

    void dump_print_css(std::ostream & out, double scale) {}

protected:
    long long install(const GfxRGB & new_value) { 
        auto iter = value_map.find(new_value);
        if(iter != value_map.end())
        {
            return iter->second;
        }

        long long id = value_map.size();
        value_map.insert(std::make_pair(new_value, id));
        return id;
    }

    Imp * imp;

    long long cur_id;
    GfxRGB cur_value;
    bool cur_is_transparent;

    class GfxRGB_hash 
    {
    public:
        size_t operator () (const GfxRGB & rgb) const
        {
            return ( (((size_t)colToByte(rgb.r)) << 16) 
                   | (((size_t)colToByte(rgb.g)) << 8) 
                   | ((size_t)colToByte(rgb.b))
                   );
        }
    };

    class GfxRGB_equal
    { 
    public:
        bool operator ()(const GfxRGB & rgb1, const GfxRGB & rgb2) const
        {
            return ((rgb1.r == rgb2.r) && (rgb1.g == rgb2.g) && (rgb1.b == rgb2.b));
        }
    };

    GfxRGB_equal gfxrgb_equal_obj;
    std::unordered_map<GfxRGB, long long, GfxRGB_hash, GfxRGB_equal> value_map;
};

/////////////////////////////////////
// Specific state managers

class FontSizeManager : public StateManager<double, FontSizeManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::FONT_SIZE_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "font-size:" << round(value) << "px;"; }
    void dump_print_value(std::ostream & out, double value, double scale) { out << "font-size:" << round(value*scale) << "pt;"; }
};

class LetterSpaceManager : public StateManager<double,  LetterSpaceManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::LETTER_SPACE_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "letter-spacing:" << round(value) << "px;"; }
    void dump_print_value(std::ostream & out, double value, double scale) { out << "letter-spacing:" << round(value*scale) << "pt;"; }
};

class WordSpaceManager : public StateManager<double, WordSpaceManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::WORD_SPACE_CN;}
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "word-spacing:" << round(value) << "px;"; }
    void dump_print_value(std::ostream & out, double value, double scale) { out << "word-spacing:" << round(value*scale) << "pt;"; }
};

class RiseManager : public StateManager<double, RiseManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::RISE_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "top:" << round(-value) << "px;"; }
    void dump_print_value(std::ostream & out, double value, double scale) { out << "top:" << round(-value*scale) << "pt;"; }
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
    void dump_print_value(std::ostream & out, double value, double scale) 
    {
        value *= scale;
        out << ((value > 0) ? "display:inline-block;width:" 
                            : "display:inline;margin-left:")
            << round(value) << "pt;";
    }
};

class WidthManager : public StateManager<double, WidthManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::WIDTH_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "width:" << round(value) << "px;"; }
    void dump_print_value(std::ostream & out, double value, double scale) { out << "width:" << round(value*scale) << "pt;"; }
};

class BottomManager : public StateManager<double, BottomManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::BOTTOM_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "bottom:" << round(value) << "px;"; }
    void dump_print_value(std::ostream & out, double value, double scale) { out << "bottom:" << round(value*scale) << "pt;"; }
};

class HeightManager : public StateManager<double, HeightManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::HEIGHT_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "height:" << round(value) << "px;"; }
    void dump_print_value(std::ostream & out, double value, double scale) { out << "height:" << round(value*scale) << "pt;"; }
};

class LeftManager : public StateManager<double, LeftManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::LEFT_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "left:" << round(value) << "px;"; }
    void dump_print_value(std::ostream & out, double value, double scale) { out << "left:" << round(value*scale) << "pt;"; }
};

class TransformMatrixManager : public StateManager<Matrix, TransformMatrixManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::TRANSFORM_MATRIX_CN; }
    const double * default_value(void) { return ID_MATRIX; }
    void dump_value(std::ostream & out, const Matrix & matrix) { 
        // always ignore tm[4] and tm[5] because
        // we have already shifted the origin
        // TODO: recognize common matices
        const auto & m = matrix.m;
        if(tm_equal(m, ID_MATRIX, 4))
        {
            auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
            for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
                out << *iter << "transform:none;";
        }
        else
        {
            auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
            for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
            {
                // PDF use a different coordinate system from Web
                out << *iter << "transform:matrix("
                    << round(m[0]) << ','
                    << round(-m[1]) << ','
                    << round(-m[2]) << ','
                    << round(m[3]) << ',';
                out << "0,0);";
            }
        }
    }
};

class FillColorManager : public StateManager<GfxRGB, FillColorManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::FILL_COLOR_CN; }
    /* override base's method, as we need some workaround in CSS */ 
    void dump_css(std::ostream & out) { 
        out << "." << get_css_class_name() << CSS::INVALID_ID << "{color:transparent;}" << std::endl;
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << get_css_class_name() << iter->second 
                << "{color:" << iter->first << ";}" << std::endl;
        }
    }
};

class StrokeColorManager : public StateManager<GfxRGB, StrokeColorManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::STROKE_COLOR_CN; }
    /* override base's method, as we need some workaround in CSS */ 
    void dump_css(std::ostream & out) { 
        // normal CSS
        out << "." << get_css_class_name() << CSS::INVALID_ID << "{text-shadow:none;}" << std::endl;
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            // TODO: take the stroke width from the graphics state,
            //       currently using 0.015em as a good default
            out << "." << get_css_class_name() << iter->second << "{text-shadow:" 
                << "-0.015em 0 "  << iter->first << "," 
                << "0 0.015em "   << iter->first << ","
                << "0.015em 0 "   << iter->first << ","
                << "0 -0.015em  " << iter->first << ";"
                << "}" << std::endl;
        }
        // webkit
        out << CSS::WEBKIT_ONLY << "{" << std::endl;
        out << "." << get_css_class_name() << CSS::INVALID_ID << "{-webkit-text-stroke:0px transparent;}" << std::endl;
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << get_css_class_name() << iter->second 
                << "{-webkit-text-stroke:0.015em " << iter->first << ";text-shadow:none;}" << std::endl;
        }
        out << "}" << std::endl;
    }
};

/////////////////////////////////////
/*
 * Manage the background image sizes
 * Kind of similar with StateManager, but not exactly the same
 * anyway temporarly leave it here
 */
class BGImageSizeManager
{
public:
    void install(int page_no, double width, double height){
        value_map.insert(std::make_pair(page_no, std::make_pair(width, height)));
    }

    void dump_css(std::ostream & out) {
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            const auto & s = iter->second;
            out << "." << CSS::PAGE_CONTENT_BOX_CN << iter->first << "{";
            out << "background-size:" << round(s.first) << "px " << round(s.second) << "px;";
            out << "}" << std::endl;
        }
    }

    void dump_print_css(std::ostream & out, double scale) {
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            const auto & s = iter->second;
            out << "." << CSS::PAGE_CONTENT_BOX_CN << iter->first << "{";
            out << "background-size:" << round(s.first * scale) << "pt " << round(s.second * scale) << "pt;";
            out << "}" << std::endl;
        }
    }

private:
    std::unordered_map<int, std::pair<double,double>> value_map; 
};

} // namespace pdf2htmlEX 

#endif //STATEMANAGER_H__
