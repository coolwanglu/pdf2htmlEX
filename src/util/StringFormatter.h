/*
 * Buffer reusing string formatter
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef STRINGFORMATTER_H__
#define STRINGFORMATTER_H__

namespace pdf2htmlEX {

class StringFormatter
{
public:
    class GuardedPointer
    {
    public:
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
    GuardedPointer operator () (const char * format, ...) {
        assert((buf_cnt == 0) && "StringFormatter: buffer is reused!");

        va_list vlist;
        va_start(vlist, format);
        int l = vsnprintf(&buf.front(), buf.capacity(), format, vlist);
        va_end(vlist);
        if(l >= (int)buf.capacity()) 
        {
            buf.reserve(std::max<long>((long)(l+1), (long)buf.capacity() * 2));
            va_start(vlist, format);
            l = vsnprintf(&buf.front(), buf.capacity(), format, vlist);
            va_end(vlist);
        }
        assert(l >= 0); // we should fail when vsnprintf fail
        assert(l < (int)buf.capacity());
        return GuardedPointer(this);
    }
private:
    friend class GuardedPointer;
    std::vector<char> buf;
    int buf_cnt;
};

} //namespace pdf2htmlEX
#endif //STRINGFORMATTER_H__
