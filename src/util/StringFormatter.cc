#include <cstdarg>
#include <algorithm>
#include <cassert>

#include "StringFormatter.h"

namespace pdf2htmlEX {

StringFormatter::GuardedPointer StringFormatter::operator () (const char * format, ...) 
{
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

} //namespace pdf2htmlEX

