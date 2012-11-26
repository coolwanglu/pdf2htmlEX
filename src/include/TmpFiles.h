#ifndef TMPFILES_H__
#define TMPFILES_H__

#include <string>
#include <set>
#include "Param.h"

namespace pdf2htmlEX {

class TmpFiles 
{
public:
    explicit TmpFiles( Param const& param );
    virtual ~TmpFiles();

	void add(std::string const& fn);

private:
	void clean();
		
private:
    Param const& param;
	std::set<std::string> tmp_files;

};

} // namespace pdf2htmlEX

#endif //TMPFILES_H__
