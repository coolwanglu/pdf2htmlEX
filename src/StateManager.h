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

#include "Color.h"

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
    { }

    // values no farther than eps are treated as equal
    void set_eps (double eps) { 
        this->eps = eps; 
    }

    double get_eps (void) const {
        return eps;
    }

    // install new_value into the map
    // return the corresponding id
    long long install(double new_value, double * actual_value_ptr = nullptr) {
        auto iter = value_map.lower_bound(new_value - eps);
        if((iter != value_map.end()) && (std::abs(iter->first - new_value) <= eps))
        {
            if(actual_value_ptr != nullptr)
                *actual_value_ptr = iter->first;
            return iter->second;
        }

        long long id = value_map.size();
        double v = value_map.insert(iter, std::make_pair(new_value, id))->first;
        if(actual_value_ptr != nullptr)
            *actual_value_ptr = v;
        return id;
    }

    void dump_css(std::ostream & out) {
        for(auto & p : value_map)
        {
            out << "." << imp->get_css_class_name() << p.second << "{";
            imp->dump_value(out, p.first);
            out << "}" << std::endl;
        }
    }

    void dump_print_css(std::ostream & out, double scale) {
        for(auto & p : value_map)
        {
            out << "." << imp->get_css_class_name() << p.second << "{";
            imp->dump_print_value(out, p.first, scale);
            out << "}" << std::endl;
        }
    }

protected:
    double eps;
    Imp * imp;
    std::map<double, long long> value_map;
};

// Be careful about the mixed usage of Matrix and const double *
// the input is usually double *, which might be changed, so we have to copy the content out
// in the map we use Matrix instead of double * such that the array may be automatically release when destructing
template <class Imp>
class StateManager<Matrix, Imp>
{
public:
    StateManager()
        : imp(static_cast<Imp*>(this))
    { }

    // return id
    long long install(const double * new_value) {
        Matrix m;
        memcpy(m.m, new_value, 4 * sizeof(double));
        auto iter = value_map.lower_bound(m);
        if((iter != value_map.end()) && (tm_equal(m.m, iter->first.m, 4)))
        {
            return iter->second;
        }

        long long id = value_map.size();
        value_map.insert(iter, std::make_pair(m, id));
        return id;
    }

    void dump_css(std::ostream & out) {
        for(auto & p : value_map)
        {
            out << "." << imp->get_css_class_name() << p.second << "{";
            imp->dump_value(out, p.first);
            out << "}" << std::endl;
        }
    }

    void dump_print_css(std::ostream & out, double scale) {}

protected:
    Imp * imp;

    struct Matrix_less
    {
        bool operator () (const Matrix & m1, const Matrix & m2) const
        {
            // Note that we only care about the first 4 elements
            for(int i = 0; i < 4; ++i)
            {
                if(m1.m[i] < m2.m[i])
                    return true;
                if(m1.m[i] > m2.m[i])
                    return false;
            }
            return false;
        }
    };

    std::map<Matrix, long long, Matrix_less> value_map;
};

template <class Imp>
class StateManager<Color, Imp>
{
public:
    StateManager()
        : imp(static_cast<Imp*>(this))
    { }

    long long install(const Color & new_value) { 
        auto iter = value_map.find(new_value);
        if(iter != value_map.end())
        {
            return iter->second;
        }

        long long id = value_map.size();
        value_map.insert(std::make_pair(new_value, id));
        return id;
    }

    void dump_css(std::ostream & out) {
        out << "." << imp->get_css_class_name() << CSS::INVALID_ID << "{";
        imp->dump_transparent(out);
        out << "}" << std::endl;

        for(auto & p : value_map)
        {
            out << "." << imp->get_css_class_name() << p.second << "{";
            imp->dump_value(out, p.first);
            out << "}" << std::endl;
        }
    }

    void dump_print_css(std::ostream & out, double scale) {}

protected:
    Imp * imp;

    struct Color_hash 
    {
        size_t operator () (const Color & color) const
        {
            if(color.transparent)
            {
                return (~((size_t)0));
            }
            else
            {
                return ( ((((size_t)colToByte(color.rgb.r)) & 0xff) << 16) 
                        | ((((size_t)colToByte(color.rgb.g)) & 0xff) << 8) 
                        | (((size_t)colToByte(color.rgb.b)) & 0xff)
                        );
            }
        }
    };

    std::unordered_map<Color, long long, Color_hash> value_map;
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

class VerticalAlignManager : public StateManager<double, VerticalAlignManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::VERTICAL_ALIGN_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { out << "vertical-align:" << round(value) << "px;"; }
    void dump_print_value(std::ostream & out, double value, double scale) { out << "vertical-align:" << round(value*scale) << "pt;"; }
};

class WhitespaceManager : public StateManager<double, WhitespaceManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::WHITESPACE_CN; }
    double default_value(void) { return 0; }
    void dump_value(std::ostream & out, double value) { 
        out << ((value > 0) ? "width:"
                            : "margin-left:")
            << round(value) << "px;";
    }
    void dump_print_value(std::ostream & out, double value, double scale) 
    {
        value *= scale;
        out << ((value > 0) ? "width:"
                            : "margin-left:")
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
        // TODO: recognize common matrices
        const auto & m = matrix.m;
        auto prefixes = {"", "-ms-", "-webkit-"};
        if(tm_equal(m, ID_MATRIX, 4))
        {
            for(auto & s : prefixes)
                out << s << "transform:none;";
        }
        else
        {
            for(auto & s : prefixes)
            {
                // PDF use a different coordinate system from Web
                out << s << "transform:matrix("
                    << round(m[0]) << ','
                    << round(-m[1]) << ','
                    << round(-m[2]) << ','
                    << round(m[3]) << ',';
                out << "0,0);";
            }
        }
    }
};

class FillColorManager : public StateManager<Color, FillColorManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::FILL_COLOR_CN; }
    /* override base's method, as we need some workaround in CSS */ 
    void dump_css(std::ostream & out) { 
        for(auto & p : value_map)
        {
            out << "." << get_css_class_name() << p.second 
                << "{color:" << p.first << ";}" << std::endl;
        }
    }
};

class StrokeColorManager : public StateManager<Color, StrokeColorManager>
{
public:
    static const char * get_css_class_name (void) { return CSS::STROKE_COLOR_CN; }
    /* override base's method, as we need some workaround in CSS */ 
    void dump_css(std::ostream & out) { 
        // normal CSS
        out << "." << get_css_class_name() << CSS::INVALID_ID << "{text-shadow:none;}" << std::endl;
        for(auto & p : value_map)
        {
            // TODO: take the stroke width from the graphics state,
            //       currently using 0.015em as a good default
            out << "." << get_css_class_name() << p.second << "{text-shadow:" 
                << "-0.015em 0 "  << p.first << "," 
                << "0 0.015em "   << p.first << ","
                << "0.015em 0 "   << p.first << ","
                << "0 -0.015em  " << p.first << ";"
                << "}" << std::endl;
        }
        // webkit
        out << CSS::WEBKIT_ONLY << "{" << std::endl;
        out << "." << get_css_class_name() << CSS::INVALID_ID << "{-webkit-text-stroke:0px transparent;}" << std::endl;
        for(auto & p : value_map)
        {
            out << "." << get_css_class_name() << p.second 
                << "{-webkit-text-stroke:0.015em " << p.first << ";text-shadow:none;}" << std::endl;
        }
        out << "}" << std::endl;
    }
};

/////////////////////////////////////
/*
 * Manage the background image sizes
 *
 * We don't merge similar values, since they are bound with PAGE_CONTENT_BOX_number
 */
class BGImageSizeManager
{
public:
    void install(int page_no, double width, double height){
        value_map.insert(std::make_pair(page_no, std::make_pair(width, height)));
    }

    void dump_css(std::ostream & out) {
        for(auto & p : value_map)
        {
            const auto & s = p.second;
            out << "." << CSS::PAGE_CONTENT_BOX_CN << p.first << "{";
            out << "background-size:" << round(s.first) << "px " << round(s.second) << "px;";
            out << "}" << std::endl;
        }
    }

    void dump_print_css(std::ostream & out, double scale) {
        for(auto & p : value_map)
        {
            const auto & s = p.second;
            out << "." << CSS::PAGE_CONTENT_BOX_CN << p.first << "{";
            out << "background-size:" << round(s.first * scale) << "pt " << round(s.second * scale) << "pt;";
            out << "}" << std::endl;
        }
    }

private:
    std::unordered_map<int, std::pair<double,double>> value_map; 
};

struct AllStateManager
{
    TransformMatrixManager transform_matrix;
    VerticalAlignManager     vertical_align;
    StrokeColorManager         stroke_color;
    LetterSpaceManager         letter_space;
    WhitespaceManager            whitespace;
    WordSpaceManager             word_space;
    FillColorManager             fill_color;
    FontSizeManager               font_size;
    BottomManager                    bottom;
    HeightManager                    height;
    WidthManager                      width;
    LeftManager                        left;
    BGImageSizeManager         bgimage_size;
};

} // namespace pdf2htmlEX 

#endif //STATEMANAGER_H__
