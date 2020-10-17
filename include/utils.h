#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <set>

void splitpath(const std::string& str, std::vector<std::string>& out, const std::set<char>& delimiters)
{
    out.clear();
    char const* pch = str.c_str();
    char const* start = pch;
    for(; *pch; ++pch)
    {
        if(delimiters.find(*pch) != delimiters.end())
        {
            if(start != pch)
            {
                std::string str(start, pch);
                out.push_back(str);
            }
            else
            {
                out.push_back("");
            }
            start = pch+1;
        }
    }
    out.push_back(start);
}

#endif
