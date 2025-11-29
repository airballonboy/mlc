#pragma once

#include "lexar.h"
#include "types.h"
#include <any>
#include <vector>
class Parser {
public:
    Parser(Lexar* lexar);
    Program* parse();
    Func parseFunction(bool member = false, Struct parent = {});
    Variable& parseVariable(VariableStorage& var_store, bool member = false);
    Variable parseArgument();
    void     parseModuleDeclaration();
    void     parseStructDeclaration();
    void     parseModulePrefix();
    void     parseExtern();
    void     parseStatement();
    void     parseBlock();
    Variable initStruct(VariableStorage& var_store, std::string type_name, std::string struct_name, bool member = false);
    std::tuple<Variable, bool> parsePrimaryExpression();
    std::tuple<Variable, bool> parseUnaryExpression();
    std::tuple<Variable, bool> parseMultiplicativeExpression();
    std::tuple<Variable, bool> parseAdditiveExpression();
    std::tuple<Variable, bool> parseCondition(int min_prec);
    std::tuple<Variable, bool> parseExpression();
    void     parseFuncCall(Func func, Variable this_ptr = {Type::Void_t});
    void     parseHash();

    Token** tkn;
private:
    Lexar* m_currentLexar;
    Program m_program;
    Func*   m_currentFunc;
    VariableStorage* m_current_var_store;
    FunctionStorage* m_currentFuncStorage;

    bool variable_exist_in_storage(std::string_view varName, const VariableStorage&);
    bool function_exist_in_storage(std::string_view funcName, const FunctionStorage&);
    Func& get_func_from_name(std::string_view name, FunctionStorage& func_storage);
    std::any  variable_default_value(Type t);
    size_t    variable_size_bytes(Type t);
    Variable& get_var_from_name(std::string_view name, VariableStorage& var_storage);
    Struct& get_struct_from_name(std::string& name);
};
