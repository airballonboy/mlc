#pragma once

#include "lexar.h"
#include "types.h"
#include <vector>
class Parser {
public:
    Parser(Lexar* lexar);
    Program* parse();
    Func parseFunction();
    Variable parseVariable();

private:
    Lexar* m_currentLexar;
    Program m_program;

    bool module_exist_in_storage(std::string mod_name, ModuleStorage mod_storage);
};
