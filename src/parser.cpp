#include "parser.h"
#include "tools/logger.h"
#include "types.h"
#include <print>
#include <vector>
Parser::Parser(Lexar* lexar){
    m_currentLexar = lexar;
}
void Parser::parse() {
    auto tkn = &m_currentLexar->currentToken;

    while ((*tkn)->type != TokenType::EndOfFile) {
        switch((*tkn)->type) {
            case TokenType::Func:{
                func fn;
                m_currentLexar->getAndExpectNext(TokenType::ID);
                fn.id = (*tkn)->string_value;
                m_currentLexar->getAndExpectNext(TokenType::OParen);
                while (m_currentLexar->peek()->type != TokenType::CParen && m_currentLexar->peek()->type != TokenType::EndOfFile) {
                    m_currentLexar->getAndExpectNext(TokenType::TypeID);
                    m_currentLexar->getAndExpectNext(TokenType::ID);
                    // Process Local Variables
                    if(m_currentLexar->peek()->type != TokenType::CParen)
                        m_currentLexar->expectNext(TokenType::Comma);
                }

                if (m_currentLexar->peek()->type == TokenType::EndOfFile) return;

                m_currentLexar->getAndExpectNext(TokenType::CParen);
                m_currentLexar->expectNext({TokenType::Arrow, TokenType::OCurly});

                // Returns
                if (m_currentLexar->peek()->type == TokenType::Arrow) {
                    m_currentLexar->getAndExpectNext(TokenType::Arrow);
                    m_currentLexar->getAndExpectNext(TokenType::TypeID);
                }

                parseBody();
                m_currentLexar->getNext();
                
            }break;//TokenType::Func
            case TokenType::ID: {

                std::println("unimplemented type of {}", printableToken.at((*tkn)->type));
                m_currentLexar->getNext();
            }break;
            default: {
                std::println("unimplemented type of {}", printableToken.at((*tkn)->type));
                m_currentLexar->getNext();

            }break;
        }
    }

}
std::vector<statment> Parser::parseBody(){
    std::vector<statment> body;

    m_currentLexar->getAndExpectNext(TokenType::OCurly);

    TODO("parse Body");
    while (m_currentLexar->peek()->type != TokenType::CCurly && m_currentLexar->peek()->type != TokenType::EndOfFile) {
        m_currentLexar->getNext();        
    }

    m_currentLexar->getAndExpectNext(TokenType::CCurly);

    return body;

}
