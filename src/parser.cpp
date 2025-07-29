#include "parser.h"
#include "tools/logger.h"
#include "types.h"
#include <any>
#include <print>
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
                while (m_currentLexar->peek()->loc.line == (*tkn)->loc.line)
                    m_currentLexar->getNext();
                m_currentLexar->getNext();
                std::println("# passed but not implemented");
            }break;
            default: {
                std::println("unimplemented type of {}", printableToken.at((*tkn)->type));
                m_currentLexar->getNext();

            }break;
        }
    }
    return &m_program;

}
// should be instuctions not statments
Func Parser::parseFunction(){
    auto tkn = &m_currentLexar->currentToken;
    Func func;
    m_currentLexar->getAndExpectNext(TokenType::ID);
    func.name = m_currentLexar->currentToken->string_value;
    m_currentLexar->getAndExpectNext(TokenType::OParen);
    while (m_currentLexar->peek()->type != TokenType::CParen && m_currentLexar->peek()->type != TokenType::EndOfFile) {
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

    while (m_currentLexar->peek()->type != TokenType::CCurly && m_currentLexar->peek()->type != TokenType::EndOfFile) {
        m_currentLexar->getAndExpectNext({TokenType::ID, TokenType::TypeID, TokenType::OCurly, TokenType::Return});
        switch ((*tkn)->type) {
            case TokenType::ID: {
                if (m_currentLexar->peek()->type == TokenType::OParen) {
                    auto func_name = (*tkn)->string_value;
                    VariableStorage args{};
                    m_currentLexar->getAndExpectNext(TokenType::OParen);
                    while (m_currentLexar->peek()->type != TokenType::CParen) {
                        m_currentLexar->getAndExpectNext({TokenType::DQoute, TokenType::IntLit, TokenType::ID});
                        if((*tkn)->type == TokenType::DQoute) m_currentLexar->getAndExpectNext(TokenType::StringLit);
                        if((*tkn)->type == TokenType::StringLit) {
                            // TODO: make string literals stored in local variable not public
                            args.push_back({Type::String_t, f("string_literal_{}", stringLiteralCount), (*tkn)->string_value});
                            m_program.var_storage.push_back({Type::String_t, f("string_literal_{}", stringLiteralCount++), (*tkn)->string_value});
                            m_currentLexar->getAndExpectNext(TokenType::DQoute);
                        }
                        if (m_currentLexar->peek()->type != TokenType::CParen)
                            m_currentLexar->getAndExpectNext(TokenType::Comma);
                    }
                    m_currentLexar->getAndExpectNext(TokenType::CParen);
                    func.body.push_back({Op::CALL, {func_name, args}});
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
                    TODO("Check Assignment");
                }
            }break;
            case TokenType::Return: {
                m_currentLexar->getAndExpectNext({TokenType::IntLit, TokenType::ID, TokenType::SemiColon});
                int return_value;
                if ((*tkn)->type == TokenType::SemiColon) {
                    if (func.return_type != Type::Void_t) TODO("error on no return");
                    return_value = 0;
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
                        return_value = (*tkn)->int_value;
                    else if(func.return_type == Type::Void_t) {
                        ERROR((*tkn)->loc, "void can't return");
                    }
                }else if ((*tkn)->type == TokenType::ID)
                    TODO("check if is variable and get it's value");
                func.body.push_back({Op::RETURN, {(int)func.return_type, return_value}});
                m_currentLexar->getAndExpectNext(TokenType::SemiColon);
            }break;
            case TokenType::TypeID: {
                TODO("Add Variables");
            }break;
        }
        //while ((*tkn)->type != TokenType::SemiColon) m_currentLexar->getNext();
    }

    m_currentLexar->getAndExpectNext(TokenType::CCurly);

    return func;

}
bool Parser::module_exist_in_storage(std::string mod_name, ModuleStorage mod_storage) {
    //for (const auto& mod : mod_storage) {
    //    if (mod.name == mod_name) return true;
    //}
    return false;
}
