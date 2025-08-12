#pragma once

#include <string_view>
#include <vector>
struct CONTEXT {
    std::vector<std::string_view> includePaths;
};

inline CONTEXT ctx;
