// Dear emacs, this is -*- c++ -*-
#ifndef X2P_STRING_HELPERS_H
#define X2P_STRING_HELPERS_H

/*
  String manipulation utility functions

  Mostly useful to parse text files.

  davide.gerbaudo@gmail.com
  June 2014
*/

#include <string>
#include <vector>

namespace Susy{
namespace utils{

typedef std::vector< std::string > vstring_t;

/// return a copy of the str where the leading and trailing space+tab have been dropped
std::string rmLeadingTrailingWhitespaces(const std::string &str);
/// given str, return a copy where contiguous whitespaces have been replaced by single whitespaces
std::string multipleSpaces2singleSpace(std::string str);
/// return a string reproducting the command being executed 
std::string commandLineArguments(int argc, char **argv);
/// given str, return a copy where tabs have been replaced with whitespaces
std::string tab2space(std::string str);
/// whether str contains substr
bool contains(const std::string &str, const std::string &substr);
/// whether str ends with ending
bool endswith(const std::string &str, const std::string &ending);
/// split inputString in a vector of tokens
std::vector< std::string > tokenizeString(const std::string &inputString, char separator);
/// convert a string to a double
double string2double(const std::string &s);
/// convert a string to a double accounting for '*' multiplicative factors
double multiply(const std::string &str);
/// whether a string can be safely converted to an int
bool isInt(const std::string& s);
/// given a directory, return the list of its files
/**
   This function could be in another header.
 */
std::vector<std::string> filesFromDir(const std::string &dirname);

} // utils
} // susy
#endif

