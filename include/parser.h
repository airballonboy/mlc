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
    void     parseModuleDeclaration();
    void     parseModulePrefix();
    void     parseFuncCall();
    void     parseHash();

    Token** tkn;
private:
    Lexar* m_currentLexar;
    Program m_program;
    Func*   m_currentFunc;
    FunctionStorage* m_currentFuncStorage;

    bool variable_exist_in_storage(std::string_view varName, const VariableStorage&);
    bool function_exist_in_storage(std::string_view funcName, const FunctionStorage&);
    Func& get_func_from_name(std::string_view name, FunctionStorage& func_storage);
    std::any  variable_default_value(Type t);
    size_t    variable_size_bytes(Type t);
    Variable& get_var_from_name(std::string_view name, VariableStorage& var_storage);
};
