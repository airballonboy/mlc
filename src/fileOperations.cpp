#include "fileOperations.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

ML::File ML::openFile(std::string fileName) {
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error: could not open file\n";
		return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[size + 1];

    if (!file.read(buffer, size)) {
        std::cerr << "Error: could not read file\n";
		return {};
	}
	buffer[size] = '\0'; // Null-terminate


	return {buffer, size};
}
