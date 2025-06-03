#ifndef FILEOPERATIONS
#define FILEOPERATIONS

#include <fstream>

namespace ML {

typedef struct {
	char* buffer;
	std::streamsize size;
}File;

ML::File openFile(std::string fileName);


}

#endif //FILEOPERATIONS
