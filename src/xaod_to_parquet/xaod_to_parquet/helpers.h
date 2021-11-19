#ifndef X2P_MISC_HELPERS_H
#define X2P_MISC_HELPERS_H

#include <string>

std::string computeMethodName(const std::string& function, const std::string& prettyFunction);
#define __x2pfunc__ computeMethodName(__FUNCTION__,__PRETTY_FUNCTION__).c_str()

#endif

