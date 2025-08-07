#include "parser.h"
#include "lexar.h"
#include "tools/file.h"
#include "tools/logger.h"
#include "types.h"
#include <any>
#include <functional>
#include <print>
#include <string_view>
#include <vector>

#define ERROR(loc, massage) std::println("{}:{}:{} {}", (loc).inputPath, (loc).line, (loc).offset, massage);

int stringLiteralCount = 0;

Parser::Parser(Lexar* lexar){
    m_currentLexar = lexar;
}
Variable Parser::parseVariable(){
    Variable var;
    m_currentLexar->getAndExpectNext(TokenType::TypeID);
    var.type = TypeIds.at(m_currentLexar->currentToken->string_value);
    m_currentLexar->getAndExpectNext(TokenType::ID);
    var.name = m_currentLexar->currentToken->string_value;    
    // TODO: support arrays
    if (m_currentLexar->peek()->type == TokenType::OBracket) {
        m_currentLexar->getAndExpectNext(TokenType::OBracket);
        if (m_currentLexar->peek()->type == TokenType::IntLit)
            m_currentLexar->getAndExpectNext(TokenType::IntLit);
        m_currentLexar->getAndExpectNext(TokenType::CBracket);
        TODO("support for arrays");   
    }
    return var;
}
Program* Parser::parse() {
    auto tkn = &m_currentLexar->currentToken;

    while ((*tkn)->type != TokenType::EndOfFile) {
        switch((*tkn)->type) {
            case TokenType::Func:{
                m_program.func_storage.push_back(parseFunction());
                m_currentLexar->getNext();
                
            }break;//TokenType::Func
            case TokenType::Hash: {
                m_currentLexar->getAndExpectNext({TokenType::Import, TokenType::Include});
                if ((*tkn)->type == TokenType::Include) {
                    if (m_currentLexar->peek()->loc.line == (*tkn)->loc.line)
                        m_currentLexar->getAndExpectNext(TokenType::Less);
                    std::string file_name{};
                    while (m_currentLexar->peek()->loc.line == (*tkn)->loc.line && m_currentLexar->peek()->type != TokenType::Greater) {
                        m_currentLexar->getNext();
                        file_name += (*tkn)->string_value;
                        //std::println("{:10} => {}", (*tkn)->string_value, printableToken.at((*tkn)->type));
                    }
                    std::println("{}", file_name);
                    Lexar l(readFileToString(file_name), file_name);

                    m_currentLexar->getNext();

                    m_currentLexar->pushtokensaftercurrent(&l);

                    m_currentLexar->getNext();

                }
                //std::println("# passed but not implemented");
            }break;
            default: {
                std::println("unimplemented type of {} at {}:{}:{}", printableToken.at((*tkn)->type), (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset);
                m_currentLexar->getNext();

            }break;
        }
    }
    return &m_program;

}
size_t current_locals_count = 1;
// should be instuctions not statments
Func Parser::parseFunction(){
    auto tkn = &m_currentLexar->currentToken;
    current_locals_count = 1;
    Func func;
    m_currentFunc = &func;
    m_currentLexar->getAndExpectNext(TokenType::ID);
    func.name = m_currentLexar->currentToken->string_value;
    m_currentLexar->getAndExpectNext(TokenType::OParen);
    while (m_currentLexar->peek()->type != TokenType::CParen && (*tkn)->type != TokenType::EndOfFile && m_currentLexar->peek()->type != TokenType::EndOfFile) {
        Variable var = parseVariable();

        // Process parameter(Local Variables)
        if(m_currentLexar->peek()->type != TokenType::CParen)
            m_currentLexar->expectNext(TokenType::Comma);
    }

    m_currentLexar->getAndExpectNext(TokenType::CParen);
    m_currentLexar->expectNext({TokenType::Arrow, TokenType::OCurly});

    // Returns
    if (m_currentLexar->peek()->type == TokenType::Arrow) {
        m_currentLexar->getAndExpectNext(TokenType::Arrow);
        m_currentLexar->getAndExpectNext(TokenType::TypeID);
        func.return_type = TypeIds.at((*tkn)->string_value);
    }
                


    m_currentLexar->getAndExpectNext(TokenType::OCurly);

    while (m_currentLexar->peek()->type != TokenType::CCurly && (*tkn)->type != TokenType::EndOfFile && m_currentLexar->peek()->type != TokenType::EndOfFile) {
        m_currentLexar->getAndExpectNext({TokenType::ID, TokenType::TypeID, TokenType::OCurly, TokenType::Return});
        switch ((*tkn)->type) {
            case TokenType::ID: {
                if (m_currentLexar->peek()->type == TokenType::OParen) {
                    parseFuncCall();
                    m_currentLexar->getAndExpectNext(TokenType::SemiColon);
                }else if (m_currentLexar->peek()->type == TokenType::ColonColon) {
                    auto current_module_storage = m_program.module_storage;
                    if(!current_module_storage.contains((*tkn)->string_value)) TODO("error");
                    m_currentLexar->getAndExpectNext(TokenType::ColonColon);
                    while((m_currentLexar->peek() + 1)->type == TokenType::ColonColon){
                        m_currentLexar->getAndExpectNext(TokenType::ID);
                        if(!current_module_storage.contains((*tkn)->string_value)) TODO("error");
                        current_module_storage = current_module_storage.at((*tkn)->string_value).module_storage;
                        m_currentLexar->getAndExpectNext(TokenType::ColonColon);
                    }
                }else if (m_currentLexar->peek()->type == TokenType::Eq) {
                    auto var1 = get_var_from_name((*tkn)->string_value, func.local_variables);
                    m_currentLexar->getAndExpectNext(TokenType::Eq);
                    m_currentLexar->getAndExpectNext({TokenType::DQoute, TokenType::IntLit, TokenType::ID});
                    // Storing a function return
                    if (m_currentLexar->peek()->type == TokenType::OParen) {
                        parseFuncCall();
                        func.body.push_back({Op::STORE_RET, {var1}});
                        m_currentLexar->getAndExpectNext(TokenType::SemiColon);
                        break;
                    }
                    auto var2 = parseArgument();
                    func.body.push_back({Op::STORE_VAR, {var2, var1}});
                    m_currentLexar->getAndExpectNext(TokenType::SemiColon);
                }
            }break;
            case TokenType::Return: {
                m_currentLexar->getAndExpectNext({TokenType::IntLit, TokenType::ID, TokenType::SemiColon});
                Variable return_value;
                if ((*tkn)->type == TokenType::SemiColon) {
                    if (func.return_type != Type::Void_t) TODO("error on no return");
                    return_value = {Type::Int_lit, "IntLit", 0};
                    m_currentLexar->currentToken--;
                }else if ((*tkn)->type == TokenType::IntLit) {
                    // TODO: type checker
                    // it should have a map to functions called cast and take and give the type expected and
                    //  if the current type has a cast to the other type then they are compatible types
                    if (func.return_type == Type::Int8_t  || 
                        func.return_type == Type::Int16_t || 
                        func.return_type == Type::Int32_t || 
                        func.return_type == Type::Int64_t 
                    )
                        return_value = {Type::Int_lit, "IntLit", (*tkn)->int_value};
                    else if(func.return_type == Type::Void_t) {
                        ERROR((*tkn)->loc, "void can't return");
                    }
                }else if ((*tkn)->type == TokenType::ID) {
                    if (variable_exist_in_storage((*tkn)->string_value, m_currentFunc->local_variables)) {
                        return_value = get_var_from_name((*tkn)->string_value, m_currentFunc->local_variables);
                    }
                }
                func.body.push_back({Op::RETURN, {return_value}});
                m_currentLexar->getAndExpectNext(TokenType::SemiColon);
            }break;
            case TokenType::TypeID: {
                std::string id;
                Type t = TypeIds.at((*tkn)->string_value);
                m_currentLexar->getAndExpectNext({TokenType::ID, TokenType::UScore});
                if ((*tkn)->type == TokenType::ID)
                    id = (*tkn)->string_value;
                else 
                    continue;
                m_currentLexar->getAndExpectNext({TokenType::Eq, TokenType::SemiColon});

                Variable v = {t, id, variable_default_value(t), current_locals_count++*8};
                Type def_t;
                if (t == Type::String_t)
                    def_t = Type::String_lit;
                else 
                    def_t = Type::Int_lit;
                Variable def = {def_t, "def_value", variable_default_value(t)};
                func.body.push_back({Op::STORE_VAR, {def, v}});
                func.local_variables.push_back(v);
            }break;
        }
        //while ((*tkn)->type != TokenType::SemiColon) m_currentLexar->getNext();
    }

    m_currentLexar->getAndExpectNext(TokenType::CCurly);
    func.stack_size = current_locals_count*8;

    return func;

}


void Parser::parseFuncCall(){
    auto func_name = m_currentLexar->currentToken->string_value;

    //TODO: uncomment because it now can't finc c std fuctions
    //if (!function_exist_in_storage(func_name, m_program.func_storage)) {std::println("{}", func_name);TODO("func doesn't exist");}

    VariableStorage args{};
    m_currentLexar->getAndExpectNext(TokenType::OParen);
    while (m_currentLexar->peek()->type != TokenType::CParen) {
        m_currentLexar->getAndExpectNext({TokenType::DQoute, TokenType::IntLit, TokenType::ID});
        args.push_back(parseArgument());
        if (m_currentLexar->peek()->type != TokenType::CParen) {
            m_currentLexar->getAndExpectNext(TokenType::Comma);
        }
    }
    m_currentLexar->getAndExpectNext(TokenType::CParen);
    m_currentFunc->body.push_back({Op::CALL, {func_name, args}});
}

bool Parser::function_exist_in_storage(std::string_view func_name, const FunctionStorage& func_storage) {
    for (auto& func : func_storage) {
        if (func.name == func_name) return true;
    }
    return false;
}
Func& Parser::get_func_from_name(std::string_view name, FunctionStorage& func_storage) {

    if (!function_exist_in_storage(name, func_storage)) {std::println("{}", name); TODO("func doesn't exist");}

    for (auto& func : func_storage) {
        if (func.name == name) return func;
    }
    std::println("{}", name);
    TODO("func doesn't exist");
}
// OUTDATED:
bool Parser::variable_exist_in_storage(std::string_view var_name, const VariableStorage& var_storage) {
    for (const auto& var : var_storage) {
        if (var.name == var_name) return true;
    }
    return false;
}
Variable& Parser::get_var_from_name(std::string_view name, VariableStorage& var_storage) {
    for (auto& var : var_storage) {
        if (var.name == name) return var;
    }
    TODO("var doesn't exist");
}
Variable Parser::parseArgument() {
    auto tkn = &m_currentLexar->currentToken;
    Variable arg;
    if((*tkn)->type == TokenType::DQoute) m_currentLexar->getAndExpectNext(TokenType::StringLit);
    if((*tkn)->type == TokenType::StringLit) {
        // TODO: make string literals stored in local variable not public
        arg = {Type::String_lit, f("string_literal_{}", stringLiteralCount), (*tkn)->string_value};
        m_program.var_storage.push_back({Type::String_lit, f("string_literal_{}", stringLiteralCount++), (*tkn)->string_value});
        m_currentLexar->getAndExpectNext(TokenType::DQoute);
    }
    if ((*tkn)->type == TokenType::IntLit) {
        arg = {Type::Int_lit, "Int_Lit", (*tkn)->int_value};
    }
    if ((*tkn)->type == TokenType::ID) {
        if (m_currentLexar->peek()->type == TokenType::OParen) {
            auto& func = get_func_from_name((*tkn)->string_value, m_program.func_storage);
            parseFuncCall();
            arg.offset = current_locals_count++*8;
            arg.type = func.return_type;
            std::println("{}", int(arg.type));
            m_currentFunc->body.push_back({Op::STORE_RET, {arg}});
        }
        if (variable_exist_in_storage((*tkn)->string_value, m_program.var_storage))
            TODO("check Global variables");
        else {
            if (variable_exist_in_storage((*tkn)->string_value, m_currentFunc->local_variables)) {
                arg = get_var_from_name((*tkn)->string_value, m_currentFunc->local_variables);
            }
        }
    }

    return arg;
}
std::any Parser::variable_default_value(Type t) {
    switch (t) {
        case Type::Size_t:
        case Type::Int8_t:
        case Type::Int16_t:
        case Type::Int32_t:
        case Type::Int64_t:
        case Type::Float_t:  return 0    ; break;

        case Type::String_t: return ""   ; break;
        case Type::Char_t:   return ""   ; break;
        case Type::Bool_t:   return false; break;
        case Type::Void_t:   return 0    ; break;

        default: 
            TODO(f("type {} doesn't have default", (int)t));
    }
    return 0;
}
// UNNEEDED
size_t Parser::variable_size_bytes(Type t) {
    switch (t) {
        case Type::Bool_t:   return 1; break;
        case Type::Char_t:   return 1; break;
        case Type::Int8_t:   return 1; break;
        case Type::Int16_t:  return 2; break;
        case Type::Int32_t:  return 4; break;
        case Type::Int64_t:  return 8; break;
        case Type::Size_t:   return 8; break;
        case Type::Float_t:  return 4; break;

        case Type::String_t: return 0; break;
        case Type::Void_t:   return 0; break;

        default: 
            TODO(f("type {} doesn't have default", (int)t));
    }
    return 0;
}
