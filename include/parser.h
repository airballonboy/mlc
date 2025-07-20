#pragma once

#include "lexar.h"
#include "types.h"
#include <vector>
class Parser {
public:
    Parser(Lexar* lexar);
    Program* parse();
    Func parseFunction();

private:
    Lexar* m_currentLexar;
    Program m_program;

};
