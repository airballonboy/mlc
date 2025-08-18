#pragma once

#include <string_view>
#include <unordered_set>
#include <vector>
struct CONTEXT {
    std::vector<std::string_view> includePaths;
    std::unordered_set<std::string_view> libs;
    std::unordered_set<std::string_view> search_paths;
};

inline CONTEXT ctx;
