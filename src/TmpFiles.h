#ifndef TMPFILES_H__
#define TMPFILES_H__

#include <string>
#include <set>
#include "Param.h"

namespace pdf2htmlEX {

class TmpFiles
{
public:
    explicit TmpFiles( const Param& param );
    ~TmpFiles();

    void add( const std::string& fn);
    double get_total_size() const;

private:
    void clean();

    const Param& param;
    std::set<std::string> tmp_files;
};

} // namespace pdf2htmlEX

#endif //TMPFILES_H__
