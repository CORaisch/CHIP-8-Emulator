#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <set>

std::vector<std::string> splitpath(const std::string& str, const std::set<char> delimiters)
{
    std::vector<std::string> result;

    char const* pch = str.c_str();
    char const* start = pch;
    for(; *pch; ++pch)
    {
        if(delimiters.find(*pch) != delimiters.end())
        {
            if(start != pch)
            {
                std::string str(start, pch);
                result.push_back(str);
            }
            else
            {
                result.push_back("");
            }
            start = pch+1;
        }
    }
    result.push_back(start);

    return result;
}

#endif
