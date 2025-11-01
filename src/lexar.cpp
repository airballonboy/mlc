#include "lexar.h"
#include "tools/logger.h"
#include "tools/format.h"
#include "types.h"
#include <cctype>
#include <cmath>
#include <print>
#include <string>
#include <vector>

// Current Character
#define c  m_source[m_currentCharIndex]
// Next Character
#define nc m_source[m_currentCharIndex+1]

bool ishex(char chr) {
    return ((chr > 96 && chr < 103) || (chr > 47 && chr < 58));
}
int64_t hex_string_to_int64(std::string s) {
    int64_t total = 0;
    for (int i = s.size() - 1, j = 0; i >= 0;j++, i--) {
        if (isdigit(s[i])) {
            total += std::stoi(std::string() + s[i]) * std::pow(16, j);
        }else if (s[i] > 96 && s[i] < 103) {
            total += (s[i] - 97 + 10) * std::pow(16, j);
        }else {
            std::println(stderr, "ERROR: cannot convert non hex to int");
            exit(1);
        }
    }
    return total;
}
int64_t string_to_int64(std::string s) {
    int64_t total = 0;
    for (int i = s.size() - 1, j = 0; i >= 0;j++, i--) {
        if (!std::isdigit(s[i])) { std::println(stderr, "ERROR: cannot convert non digit to int (the char is {} {})", j, i); exit(1); }
        total += std::stoi(std::string() + s[i]) * std::pow(10, j);
    }
    return total;
}

Lexar::Lexar(){}
// NOTE:
// Lexing order should be
//  1. Single line comments
//  2. Multi line comments
//  3. Strings and Chars
//  4. Punctuation
//  5. Identifiers but check first if it's a keyword
//  6. Hex
//  7. Int
Lexar::Lexar(const std::string& source, const std::string& path){
    m_source = source;
    m_filePath = path;
    m_currentLoc.inputPath = path;
    for(m_currentCharIndex = 0; m_currentCharIndex < source.size();){
        // Checking Single Line Comments
        if((std::string() + c + nc) == "//"){
            while (c != '\n') {
                m_currentLoc.offset++;
                m_currentCharIndex++;
            }

        // Checking Multi Line comments
        }else if((std::string() + c + nc) == "/*"){
            while((std::string() + c + nc) != "*/") {
                if(c == '\n'){
                    m_currentCharIndex++;
                    m_currentLoc.line++;
                    m_currentLoc.offset = 1;
                }else {
                    m_currentLoc.offset++;
                    m_currentCharIndex++;
                }
            }
            m_currentLoc.offset += 2;
            m_currentCharIndex  += 2;
        // Checking String Lit
        }else if ((m_tokens.size() >= 2) && 
                  (m_tokens.back().type == TokenType::DQoute) &&                 // if the last Token was a Double Qoute
                  (m_tokens.at(m_tokens.size()-2).type != TokenType::StringLit)) // if the Token before The last Token wasn't a stringLit
        {
            
            std::string lit;
            Loc loc = m_currentLoc;
            while(c != '"') {
                if(c == '\n'){
                    m_currentCharIndex++;
                    m_currentLoc.line++;
                    m_currentLoc.offset = 1;
                }else {
                    lit.push_back(c);
                    m_currentLoc.offset++;
                    m_currentCharIndex++;
                }
            }
            m_tokens.push_back({.type = TokenType::StringLit, .loc = loc, .string_value = lit});

        // Checking 2 Char Punctuations
        }else if(PUNCTUATION.contains(std::string() + c + nc)){
            Loc loc = m_currentLoc;
            std::string doublePunct = std::string() + c + nc;
            m_currentLoc.offset += 2;
            m_currentCharIndex  += 2;
            m_tokens.push_back({.type = PUNCTUATION.at(doublePunct), .loc = loc, .string_value = doublePunct});

        // Checking 1 Char Punctuations
        }else if(PUNCTUATION.contains(std::string() + c)){
            Loc loc = m_currentLoc;
            std::string punct = std::string() + c;
            m_currentLoc.offset++;
            m_currentCharIndex++;
            m_tokens.push_back({.type = PUNCTUATION.at(punct), .loc = loc, .string_value = punct});

        // Checking identifiers
        }else if (isIdentStart(c)){
            Loc loc = m_currentLoc;
            std::string identString;
            identString.push_back(c);
            m_currentLoc.offset++;
            m_currentCharIndex++;
            while (isIdent(c)){
                identString.push_back(c);
                m_currentLoc.offset++;
                m_currentCharIndex++;
            }
            // Check Keywords
            if (KEYWORDS.contains(identString)){
                m_tokens.push_back({.type = KEYWORDS.at(identString), .loc = loc, .string_value = identString});
                continue;
            }
            // Check TypeIds
            if (TypeIds.contains(identString)){
                m_tokens.push_back({.type = TokenType::TypeID, .loc = loc, .string_value = identString});
                continue;
            }
            m_tokens.push_back({.type = TokenType::ID, .loc = loc, .string_value = identString});

        // Check Int Lit
        }else if(isdigit(c)){
            Loc loc = m_currentLoc;
            std::string lit;

            if ((std::string() + c + nc) == "0x" || (std::string() + c + nc) == "0X") {
                m_currentLoc.offset += 2;
                m_currentCharIndex  += 2;

                while(ishex(c)){
                    lit.push_back(c);
                    m_currentLoc.offset++;
                    m_currentCharIndex++;
                }

                int64_t intLit = hex_string_to_int64(lit);
                m_tokens.push_back({.type = TokenType::IntLit, .loc = loc, .int_value = intLit});

            }else {
                lit.push_back(c);
                m_currentLoc.offset++;
                m_currentCharIndex++;

                while(isdigit(c)){
                    lit.push_back(c);
                    m_currentLoc.offset++;
                    m_currentCharIndex++;
                }
                int64_t intLit = string_to_int64(lit);
                m_tokens.push_back({.type = TokenType::IntLit, .loc = loc, .int_value = intLit});
            }
        // Skiping Spaces
        }else if(isspace(c)){
            skipSpaces();

        }else {
            m_currentLoc.offset++;
            m_currentCharIndex++;
        }
    }

    m_tokens.push_back({.type = TokenType::EndOfFile, .loc = m_currentLoc});

    currentToken = &m_tokens.at(m_currentTokenIndex);
    //for (const auto& tkn : m_tokens) {
    //    if (tkn.type == TokenType::IntLit)
    //        std::println("{}:{:03}:{:03} {:^10} => {}", tkn.loc.inputPath, tkn.loc.line, tkn.loc.offset, tokenToString(tkn), tkn.int_value);
    //    else 
    //        std::println("{}:{:03}:{:03} {:^10} => {}", tkn.loc.inputPath, tkn.loc.line, tkn.loc.offset, tokenToString(tkn), tkn.string_value);
    //}
}

void Lexar::skipSpaces(){
    while(isspace(c)){
        if(c == '\n'){
            m_currentCharIndex++;
            m_currentLoc.line++;
            m_currentLoc.offset = 1;
        }else if(c == '\t') {
            m_currentCharIndex++;
            m_currentLoc.offset += 4;
        }else {
            m_currentCharIndex++;
            m_currentLoc.offset++;
        }
    }
}

void Lexar::pushtokensaftercurrent(Lexar* l){
    if (l->m_tokens.back().type == TokenType::EndOfFile)
        l->m_tokens.pop_back();
    this->pushTokensAt(m_currentTokenIndex+1, l);    
}
void Lexar::pushTokensAt(size_t index, Lexar* l) {
    if (index > this->m_tokens.size()) TODO("ERROR: trying to push a vector of tokens at invalid index");
    std::vector<Token> ts;
    for (int i = 0; i < index; i++) ts.push_back(m_tokens[i]); 
    for (auto& t : l->m_tokens) ts.push_back(t); 
    for (;index < m_tokens.size(); index++) ts.push_back(m_tokens[index]); 
    m_tokens = ts;
    currentToken = &m_tokens.at(m_currentTokenIndex);
}
std::vector<Token> Lexar::getTokens() {
    return m_tokens;
}

void Lexar::getNext(){
    if (currentToken->type == TokenType::EndOfFile) {
        std::println("EndOfFile REACHED");
        return;
    }
    m_currentTokenIndex++;
    currentToken = &m_tokens.at(m_currentTokenIndex);// = &m_tokens.at(++m_currentTokenIndex);
}
void Lexar::expectNext(TokenType tt){
    if (peek()->type == tt) {
        return;
    }

    auto l = peek()->loc;
    mlog::error(f("{}:{}:{} ", l.inputPath, l.line, l.offset).c_str(),
                  f("Lexing error Expected {} but got {}", printableToken.at(tt), printableToken.at(peek()->type)).c_str());

}
void Lexar::expectNext(std::vector<TokenType> tts){
    std::string string_tts = "[ ";

    for(const auto& tt : tts) {
        string_tts += printableToken.at(tt);
        if (tt != tts.back())
            string_tts += ", ";

        if (peek()->type == tt) {
            return;
        }
    }
    string_tts += " ]";
    auto l = peek()->loc;
    mlog::error(f("{}:{}:{} ", l.inputPath, l.line, l.offset).c_str(),
                  f("Lexing error Expected {} but got {}", string_tts, printableToken.at(peek()->type)).c_str());

}
void Lexar::expectCurrent(TokenType tt){
    if (currentToken->type == tt) {
        return;
    }

    auto l = currentToken->loc;
    mlog::error(f("{}:{}:{} ", l.inputPath, l.line, l.offset).c_str(),
                  f("Lexing error Expected {} but got {}", printableToken.at(tt), printableToken.at(currentToken->type)).c_str());

}
void Lexar::expectCurrent(std::vector<TokenType> tts){
    std::string string_tts = "[ ";

    for(const auto& tt : tts) {
        string_tts += printableToken.at(tt);
        if (tt != tts.back())
            string_tts += ", ";

        if (currentToken->type == tt) {
            return;
        }
    }
    string_tts += " ]";
    auto l = currentToken->loc;
    mlog::error(f("{}:{}:{} ", l.inputPath, l.line, l.offset).c_str(),
                  f("Lexing error Expected {} but got {}", string_tts, printableToken.at(currentToken->type)).c_str());

}
void Lexar::getAndExpectNext(TokenType tt){
    getNext();
    
    expectCurrent(tt);
}
void Lexar::getAndExpectNext(std::vector<TokenType> tts){
    getNext();

    expectCurrent(tts);
}

Token* Lexar::peek(){
    if (currentToken->type == TokenType::EndOfFile) {
        std::println("EndOfFile REACHED");
        return &m_tokens[m_currentTokenIndex];
    }
    return &m_tokens[m_currentTokenIndex+1];
}


bool Lexar::isIdentStart(char Char){
    return (isalpha(Char) || (Char == '_'));
}
bool Lexar::isIdent(char Char){
    return (isalnum(Char) || (Char == '_'));
}
bool Lexar::isEOF(){
    return (c == EOF);
}
std::string Lexar::tokenToString(Token t){
    return printableToken.at(t.type);
}

