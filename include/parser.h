#pragma once

#include "lexar.h"
#include "types.h"
#include <any>
#include <vector>
class Parser {
public:
    Parser(Lexar* lexar);
    Program* parse();
    Func parseFunction();
    Variable parseVariable();
    Variable parseArgument();

private:
    Lexar* m_currentLexar;
    Program m_program;
    Func*   m_currentFunc;

    bool variable_exist_in_storage(std::string_view varName, const VariableStorage&);
    std::any variable_default_value(Type t);
    size_t   variable_size_bytes(Type t);
};
