#pragma once

#include "lexar.h"
#include "types.h"
#include <any>
typedef std::tuple<Variable, bool> ExprResult;
class Parser {
public:
    Parser(Lexar* lexar);
    Program* parse();
    Func parseFunction(bool member = false, Struct parent = {});
    Variable& parseVariable(VariableStorage& var_store, bool member = false);
    void     parseModuleDeclaration();
    void     parseStructDeclaration();
    Variable parseConstant();
    void     parseModulePrefix();
    void     parseExtern();
    void     parseStatement();
    void     parseBlock();
    Variable initStruct(std::string type_name, std::string struct_name, bool member = false, bool save_defaults = true);
    ExprResult parsePrimaryExpression(Variable this_ptr = {type_infos.at("void")}, Variable this_ = {type_infos.at("void")}, std::string func_prefix = {});
    ExprResult parseDotExpression(Variable this_ptr = {type_infos.at("void")}, Variable this_ = {type_infos.at("void")}, std::string func_prefix = {});
    ExprResult parseUnaryExpression();
    ExprResult parseMultiplicativeExpression();
    ExprResult parseAdditiveExpression();
    ExprResult parseCondition(int min_prec);
    ExprResult parseExpression();
    void     parseFuncCall(Func func, Variable this_ptr = {type_infos.at("void")}, Variable return_address = {type_infos.at("void")});
    void     parseHash();
    static size_t align(size_t current_offset, Type type, std::string type_name = "");

    Token** tkn;
private:
    Lexar* m_currentLexar;
    inline static Program m_program;
    Func*   m_currentFunc;
    VariableStorage* m_current_var_store;
    FunctionStorage* m_currentFuncStorage;

    bool variable_exist_in_storage(std::string_view varName, const VariableStorage&);
    bool function_exist_in_storage(std::string_view funcName, const FunctionStorage&);
    Func& get_func_from_name(std::string_view name, FunctionStorage& func_storage);
    Variable& get_var_from_name(std::string_view name, VariableStorage& var_storage);
    static std::any  variable_default_value(Type t);
    static size_t    variable_size_bytes(Type t);
    static Struct& get_struct_from_name(std::string& name);
};
