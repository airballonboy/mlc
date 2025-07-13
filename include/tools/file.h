#pragma once
#include <string>
#include <sstream>
#include <fstream>

static std::string readFileToString(std::string filePath) {
    std::string fileContent;
    std::stringstream contents_stream;
	std::fstream input(filePath, std::ios::in);
	contents_stream << input.rdbuf();
	fileContent = contents_stream.str();
	input.close();
	contents_stream.clear();
    return fileContent;
}
