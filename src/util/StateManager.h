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
        _install(imp->default_value());
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

    void dump_print_css(std::ostream & out, double scale) {
        //debug
        std::cout << imp->get_css_class_name << ' ' << scale << std::endl;

        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << imp->get_css_class_name() << iter->second << "{";
            imp->dump_print_value(out, iter->first, scale);
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

        id = value_map.size();
        actual_value = value_map.insert(std::make_pair(new_value, id)).first->first;
        return true;
    }

    double eps;
    Imp * imp;

    long long id;
    double value; // the value we are tracking
    double actual_value; // the value we actually exported to HTML
    std::map<double, long long> value_map;
};

// Be careful about the mixed usage of Matrix and const double *
template <class Imp>
class StateManager<Matrix, Imp>
{
public:
    StateManager()
        : imp(static_cast<Imp*>(this))
    { }

    void reset(void) {
        _install(imp->default_value());
    }

    // return if changed
    bool install(const double * new_value) {
        // For a transform matrix m
        // m[4] & m[5] have been taken care of
        if(tm_equal(new_value, value.m, 4))
            return false;
        _install(new_value);
        return true;
    }

    long long get_id                (void) const { return id;           }
    const Matrix & get_value        (void) const { return value;        }

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
    // return if a new entry has been created
    bool _install(const double * new_value) {
        memcpy(value.m, new_value, sizeof(value.m));

        auto iter = value_map.lower_bound(value);
        if((iter != value_map.end()) && (tm_equal(value.m, iter->first.m, 4)))
        {
            id = iter->second;
            return false;
        }

        id = value_map.size();
        value_map.insert(std::make_pair(value, id));
        return true;
    }

    Imp * imp;

    long long id;
    Matrix value;

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
        is_transparent = true;
        id = -1;
    }

    bool install(const GfxRGB & new_value) {
        if((!is_transparent) && gfxrgb_equal_obj(new_value, value))
            return false;
        _install(new_value);
        return true;
    }

    bool install_transparent (void) { 
        if(is_transparent)
            return false;
        _install_transparent();
        return true;
    }


    long long get_id                (void) const { return id;             }
    const GfxRGB & get_value        (void) const { return value;          }
    bool get_is_transparent         (void) const { return is_transparent; }

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
    bool _install(const GfxRGB & new_value) { 
        is_transparent = false;
        value = new_value;
        auto iter = value_map.find(new_value);
        if(iter != value_map.end())
        {
            id = iter->second;
            return false;
        }

        id = value_map.size();
        value_map.insert(std::make_pair(value, id));
        return true;
    }

    bool _install_transparent(void) {
        is_transparent = true;
        id = -1;
        return false;
    }

    Imp * imp;

    long long id;
    GfxRGB value;
    bool is_transparent;

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
        // normal CSS
        out << "." << get_css_class_name() << CSS::INVALID_ID << "{color:white;}" << std::endl;
        for(auto iter = value_map.begin(); iter != value_map.end(); ++iter)
        {
            out << "." << get_css_class_name() << iter->second 
                << "{color:" << iter->first << ";}" << std::endl;
        }
        // webkit
        out << CSS::WEBKIT_ONLY << "{" << std::endl;
        out << "." << get_css_class_name() << CSS::INVALID_ID << "{color:transparent;}" << std::endl;
        out << "}" << std::endl;
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

} // namespace pdf2htmlEX 

#endif //STATEMANAGER_H__
