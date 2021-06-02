#ifndef MyTool_hpp
#define MyTool_hpp 1

#include <string>
#include <vector>

class MyTool
{
 public:
  static std::vector<std::string> split(const std::string str,
                                        const std::string delimiter = " ")
  {
    std::vector<std::string> v;
    if (str.size() == 0) return v;

    auto start = str.find_first_not_of(delimiter);
    auto stop = str.find_first_of(delimiter, start);
    while (start != std::string::npos) {
      v.push_back(str.substr(start, stop - start));
      start = str.find_first_not_of(delimiter, stop);
      stop = str.find_first_of(delimiter, start);
    }

    return v;
  };
};

#endif