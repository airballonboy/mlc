#include "parser.h"
#include "tools/logger.h"
#include "types.h"
#include <any>
#include <print>
#include <vector>


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
        func.return_type = TypeIds.at(m_currentLexar->currentToken->string_value);
    }
                

    auto tkn = &m_currentLexar->currentToken;

    m_currentLexar->getAndExpectNext(TokenType::OCurly);

    while (m_currentLexar->peek()->type != TokenType::CCurly && m_currentLexar->peek()->type != TokenType::EndOfFile) {
        m_currentLexar->getAndExpectNext({TokenType::ID, TokenType::TypeID, TokenType::OCurly, TokenType::Return});
        switch ((*tkn)->type) {
            case TokenType::ID: {
                if (m_currentLexar->peek()->type == TokenType::OParen) {
                    TODO("Check Functions");
                }else if (m_currentLexar->peek()->type == TokenType::ColonColon) {
                    TODO("Check Module or Class");
                    m_currentLexar->getAndExpectNext(TokenType::ColonColon);
                    while((m_currentLexar->peek() + 1)->type == TokenType::ColonColon){
                        TODO("Check Module or Class");
                        m_currentLexar->getAndExpectNext(TokenType::ID);
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
                }else if ((*tkn)->type == TokenType::IntLit)
                    return_value = (*tkn)->int_value;
                else if ((*tkn)->type == TokenType::ID)
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
