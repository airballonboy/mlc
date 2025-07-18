#include "parser.h"
#include "tools/logger.h"
#include "types.h"
#include <any>
#include <print>
#include <vector>

/* TODO:
*  make this a real thing
*   enum class Op {
*       LOAD_CONST,
*       STORE_VAR,
*       LOAD_VAR,
*       ADD,
*       SUB,
*       MUL,
*       DIV,
*       // RETURN(return_type, return_value)
*       RETURN,
*       CALL,
*   };
*   
*   struct Instruction {
*       Op op;
*       std::vector<std::any> args;
*   };
*   
*   std::vector<Instruction> program;       
*/

Parser::Parser(Lexar* lexar){
    m_currentLexar = lexar;
}
Program* Parser::parse() {
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
                    // Process parameter(Local Variables)
                    if(m_currentLexar->peek()->type != TokenType::CParen)
                        m_currentLexar->expectNext(TokenType::Comma);
                }

                if (m_currentLexar->peek()->type == TokenType::EndOfFile) return &m_program;

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
                TODO(f("unimplemented type of {}", printableToken.at((*tkn)->type)));
                m_currentLexar->getNext();
            }break;
            default: {
                std::println("unimplemented type of {}", printableToken.at((*tkn)->type));
                m_currentLexar->getNext();

            }break;
        }
    }
    return &m_program;

/* TODO: 
*  make this into "codegen/ir"  
*   for (auto& inst : program) {
*       switch (inst.op) {
*           case Op::RETURN: {
*               auto arg = std::any_cast<int>(inst.args[0]);
*               std::println("ret({})", arg);
*           }break;
*       }
*   }
*/
}
// should be instuctions not statments
std::vector<statment> Parser::parseBody(){
    std::vector<statment> body;
    auto tkn = &m_currentLexar->currentToken;

    m_currentLexar->getAndExpectNext(TokenType::OCurly);

    while (m_currentLexar->peek()->type != TokenType::CCurly && m_currentLexar->peek()->type != TokenType::EndOfFile) {
        m_currentLexar->getAndExpectNext({TokenType::ID, TokenType::TypeID, TokenType::OCurly, TokenType::Return});
        switch ((*tkn)->type) {
            case TokenType::ID: {
                if (m_currentLexar->peek()->type == TokenType::OParen) {
                    TODO("Check Functions");
                }else if (m_currentLexar->peek()->type == TokenType::Eq) {
                    TODO("Check Assignment");
                }
            }break;
            case TokenType::Return: {
                m_currentLexar->getAndExpectNext({TokenType::IntLit, TokenType::ID});
                int return_value;
                if ((*tkn)->type == TokenType::IntLit)
                    return_value = (*tkn)->int_value;
                else if ((*tkn)->type == TokenType::ID)
                    TODO("check if is variable and get it's value");
                m_program.push_back({Op::RETURN, {(int)Type::Int32_t, return_value}});
            }break;
            case TokenType::TypeID: {
                TODO("Add Variables");
            }break;
        }
        while ((*tkn)->type != TokenType::SemiColon) m_currentLexar->getNext();
    }

    m_currentLexar->getAndExpectNext(TokenType::CCurly);

    return body;

}
