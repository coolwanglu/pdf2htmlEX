/*
 * Buffer reusing string formatter
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef STRINGFORMATTER_H__
#define STRINGFORMATTER_H__

#include <vector>
#include <cstdio>

namespace pdf2htmlEX {

class StringFormatter
{
public:
    struct GuardedPointer
    {
        GuardedPointer(StringFormatter * sf) : sf(sf) { ++(sf->buf_cnt); }
        GuardedPointer(const GuardedPointer & gp) : sf(gp.sf) { ++(sf->buf_cnt); }
        ~GuardedPointer(void) { --(sf->buf_cnt); }
        operator char* () const { return &(sf->buf.front()); }
    private:
        StringFormatter * sf;
    };

    StringFormatter() : buf_cnt(0) { buf.reserve(L_tmpnam); }
    /*
     * Important:
     * there is only one buffer, so new strings will replace old ones
     */
    GuardedPointer operator () (const char * format, ...);

private:
    friend class GuardedPointer;
    std::vector<char> buf;
    int buf_cnt;
};

} //namespace pdf2htmlEX
#endif //STRINGFORMATTER_H__
