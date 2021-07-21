#pragma once
#include <string>

// https://stackoverflow.com/a/29752943/10771848
static std::string ReplaceAll(const std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());  // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while (std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    return std::move(newString);
}

static std::string JoinString(const std::vector<std::string>& strs)
{
    std::string output;

    output.resize(11 * strs.size()); // TODO: check if it is worth it to check the exact size before assigning

    for (auto& str : strs)
        output += str + "\n";

    return std::move(output);
}
