#pragma once
#include <string>
#include <vector>
#include "token.h"


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
    size_t currentTokenIndex = 0;
    
private:
    std::vector<Token> m_tokens;
    std::string        m_source;
    size_t             m_currentCharIndex = 0;
    std::string        m_filePath;
    std::string        m_filePathNoExtension;
    Loc                m_currentLoc;

    void        skipSpaces();
    bool        isIdentStart(char Char);
    bool        isIdent(char Char);
    bool        isEOF();


};
