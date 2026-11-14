#pragma once
#include <any>
#include "lexar.h"
#include "type_system/variable.h"
#include "type_system/struct.h"
#include "type_system/func.h"
#include "program.h"

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
    void     parseFuncCall(Func& func, Variable this_ptr = {type_infos.at("void")}, Variable return_address = {type_infos.at("void")});
    void     parseHash();
    static size_t align(size_t current_offset, Type type, std::string type_name = "");

    Token** tkn;
private:
    Lexar* m_currentLexar;
    inline static Program m_program;
    Func*   m_currentFunc;
    VariableStorage* m_current_var_store;
    FunctionStorage* m_currentFuncStorage;

    Func make_type_info_func(Struct s);
};
