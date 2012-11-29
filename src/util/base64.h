/*
 * Base64 Encoding
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef BASE64_H__
#define BASE64_H__

#include <iostream>

class base64stream
{
public:

    base64stream(std::istream & in) : in(&in) { }
    base64stream(std::istream && in) : in(&in) { }

    std::ostream & dumpto(std::ostream & out)
    {
        unsigned char buf[3];
        while(in->read((char*)buf, 3))
        {
            out << base64_encoding[(buf[0] & 0xfc)>>2]
                << base64_encoding[((buf[0] & 0x03)<<4) | ((buf[1] & 0xf0)>>4)]
                << base64_encoding[((buf[1] & 0x0f)<<2) | ((buf[2] & 0xc0)>>6)]
                << base64_encoding[(buf[2] & 0x3f)];
        } 
        auto cnt = in->gcount();
        if(cnt > 0)
        {
            for(int i = cnt; i < 3; ++i)
                buf[i] = 0;

            out << base64_encoding[(buf[0] & 0xfc)>>2]
                << base64_encoding[((buf[0] & 0x03)<<4) | ((buf[1] & 0xf0)>>4)];

            if(cnt > 1)
            {
                out << base64_encoding[(buf[1] & 0x0f)<<2];
            }
            else
            {
                out <<  '=';
            }
            out << '=';
        }

        return out;
    }

private:
    std::istream * in;
    static const char * base64_encoding;
};

static inline std::ostream & operator << (std::ostream & out, base64stream & bf) { return bf.dumpto(out); }
static inline std::ostream & operator << (std::ostream & out, base64stream && bf) { return bf.dumpto(out); }


#endif //BASE64_H__
