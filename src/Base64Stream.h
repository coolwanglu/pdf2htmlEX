/*
 * Base64 Encoding
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef BASE64STREAM_H__
#define BASE64STREAM_H__

#include <iostream>

namespace pdf2htmlEX {

class Base64Stream
{
public:
    Base64Stream(std::istream & in) : in(&in) { }

    std::ostream & dumpto(std::ostream & out);

private:
    std::istream * in;
    static const char * base64_encoding;
};

inline 
std::ostream & operator << (std::ostream & out, Base64Stream bs)
{
    return bs.dumpto(out);
}

} //namespace pdf2htmlEX
#endif //BASE64STREAM_H__
