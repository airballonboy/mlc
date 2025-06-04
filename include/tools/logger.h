#ifndef MASTERLOGGER
#define MASTERLOGGER

#include <iostream>


namespace logger {

enum color{
	NoColor = 0,
	Black	= 30,
	Red 	= 31,
	Green	= 32,
	Yellow	= 33,
	Blue	= 34,
	Magenta	= 35,
	Cyan	= 36,
	White	= 37,
};

inline void log(const color color, const char* text)
{
	std::cout << "\033["<< color << "m" << text << "\033[0m\n";
}
inline void log(const char* prefix, const color color, const char* text)
{
	std::cout << "\033[36m" << prefix 
		<< "\033["<< color << "m" << text << "\033[0m\n";
}

inline void success(const char* text)
{
	std::cout << "\033[32m" << text << "\033[0m\n";
}
inline void success(const char* prefix, const char* successMassage)
{
	std::cout << "\033[36m" << prefix << "\033[32m" << successMassage << "\033[0m\n";
}


inline void error(const char* error)
{
	std::cout << "\033[31m" << error << "\033[0m\n";
}
inline void error(const char* prefix, const char* errorMassage)
{
	std::cout << "\033[36m" << prefix << "\033[31m" << errorMassage << "\033[0m\n";
}


}

#endif
