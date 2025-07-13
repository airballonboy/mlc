#pragma once

#include "lexar.h"
#include "types.h"
#include <vector>
class Parser {
public:
    Parser(Lexar* lexar);
    void parse();
    std::vector<statment> parseBody();

private:
    Lexar* m_currentLexar;
    std::vector<func> funcsi;

};
