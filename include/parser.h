#pragma once

#include "lexar.h"
#include "types.h"
#include <vector>
class Parser {
public:
    Parser(Lexar* lexar);
    Program* parse();
    std::vector<statment> parseBody();

private:
    Lexar* m_currentLexar;
    Program m_program;
    std::vector<func> funcsi;

};
