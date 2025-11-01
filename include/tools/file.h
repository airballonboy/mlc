#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <string_view>
#include <vector>

static std::string readFileToString(std::string filePath) {
    std::string fileContent;
    std::stringstream contents_stream;
	std::fstream input(filePath, std::ios::in);
	contents_stream << input.rdbuf();
	fileContent = contents_stream.str();
	input.flush();
	input.close();
	contents_stream.clear();
    return fileContent;
}
static bool fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
}
static bool fileExistsInPaths(const std::string& filename, const std::vector<std::string_view>& paths) {
    for (const auto& path : paths) {
        if (fileExists(std::string(path) + "/" + filename)) {
            return true;
        }
    }
    return false;
}
static std::string getFilePathFromPaths(const std::string& filename, const std::vector<std::string_view>& paths) {
    for (const auto& path : paths) {
        if (fileExists(std::string(path) + "/" + filename)) {
            return (std::string(path) + "/" + filename);
        }
    }
    throw "error couldn't find file";
    return "";
}
