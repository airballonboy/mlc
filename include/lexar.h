#pragma once
#include <string>
#include "types.h"
#include <vector>


//Should add TokenType::typeID
class Lexar {
public:
    Lexar();
    Lexar(const std::string& source, const std::string& path);

    void        getNext();
    void        getAndExpectNext(TokenType tt);
    void        getAndExpectNext(std::vector<TokenType> tt);
    void        expectNext(TokenType tt);
    void        expectNext(std::vector<TokenType> tt);
    void        expectCurrent(TokenType tt);
    void        expectCurrent(std::vector<TokenType> tt);
    std::string tokenToString(Token t);
    Token*      peek();
    void        pushtokensaftercurrent(Lexar* l);
    void        pushTokensAt(size_t index, Lexar* l);
    std::vector<Token> getTokens();

    Token* currentToken;
    
private:
    std::vector<Token> m_tokens;
    std::string        m_source;
    size_t             m_currentCharIndex = 0;
    std::string        m_filePath;
    std::string        m_filePathNoExtension;
    size_t             m_currentTokenIndex = 0;
    Loc                m_currentLoc;

    void        skipSpaces();
    bool        isIdentStart(char Char);
    bool        isIdent(char Char);
    bool        isEOF();


};
