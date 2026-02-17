#pragma once
#include <string_view>
#include <unordered_set>
#include <vector>
#include <filesystem>

struct CONTEXT {
    std::vector<std::string_view> includePaths;
    std::unordered_set<std::string_view> libs;
    std::unordered_set<std::string_view> search_paths;
    bool lib    = false;
    bool shared = false;
};

namespace fs = std::filesystem;
inline fs::path input_path;
inline fs::path input_file;
inline fs::path build_path;

inline CONTEXT ctx;
