/*
 * Buffer reusing string formatter
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef STRING_FORMATTER_H__
#define STRING_FORMATTER_H__

namespace pdf2htmlEX {

class string_formatter
{
public:
    class guarded_pointer
    {
    public:
        guarded_pointer(string_formatter * sf) : sf(sf) { ++(sf->buf_cnt); }
        ~guarded_pointer(void) { --(sf->buf_cnt); }
        operator char* () { return &(sf->buf.front()); }
    private:
        string_formatter * sf;
    };

    string_formatter() : buf_cnt(0) { buf.reserve(L_tmpnam); }
    /*
     * Important:
     * there is only one buffer, so new strings will replace old ones
     */
    guarded_pointer operator () (const char * format, ...) {
        assert((buf_cnt == 0) && "string_formatter: buffer is reused!");

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
        return guarded_pointer(this);
    }
private:
    friend class guarded_pointer;
    std::vector<char> buf;
    int buf_cnt;
};

} //namespace pdf2htmlEX
#endif //STRING_FORMATTER_H__
