#pragma once
#include <unordered_map>
#include <string>

enum class TokenType {
    // Terminal
    EndOfFile,
    ParseError,
    // Values
    ID, StringLit,
    CharLit, IntLit,
    DoubleLit,
    // Puncts
    OCurly, CCurly, 
    OParen, CParen,
    OBracket, CBracket,
    Not, Mul, Div,
    Mod, And, Plus,
    PlusPlus, Minus,
    MinusMinus, Less,
    LessEq, Greater,
    GreaterEq, Or, Eq,
    OrOr, AndAnd,
    EqEq, NotEq, Shl,
    ShlEq, Shr, ShrEq,
    ModEq, OrEq, AndEq,
    PlusEq, MinusEq, MulEq,
    DivEq, Question,
    Colon, ColonColon,
    SemiColon, Comma,
    Hash, BSlash,
    SQoute, DQoute,
    Dot, Arrow,
    // Types
    TypeID,
    // Keywords
    Extern, If, Else,
    While, Return, Import,
    Func, From, Include,
    Module, Struct, Const,
    Static,
};

class Loc {
public:
    size_t line   = 1;
    size_t offset = 1;
    std::string inputPath;
};

class Token {
public:
    TokenType type;
    Loc loc;
    std::string string_value;
    int64_t     int_value;
    double      double_value;
};

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"extern" , TokenType::Extern},
    {"if"     , TokenType::If},
    {"else"   , TokenType::Else},
    {"while"  , TokenType::While},
    {"return" , TokenType::Return},
    {"import" , TokenType::Import},
    {"func"   , TokenType::Func},
    {"from"   , TokenType::From},
    {"include", TokenType::Include},
    {"module" , TokenType::Module},
    {"struct" , TokenType::Struct},
    {"const"  , TokenType::Const},
    {"static" , TokenType::Static},
};

static const std::unordered_map<std::string, TokenType> PUNCTUATION = {
    {"\'" , TokenType::SQoute},
    {"\"" , TokenType::DQoute},
    {"."  , TokenType::Dot},
    {"->" , TokenType::Arrow},
    {"#"  , TokenType::Hash},
    {"\\" , TokenType::BSlash},
    {"?"  , TokenType::Question},
    {"{"  , TokenType::OCurly},
    {"}"  , TokenType::CCurly},
    {"("  , TokenType::OParen},
    {")"  , TokenType::CParen},
    {"["  , TokenType::OBracket},
    {"]"  , TokenType::CBracket},
    {";"  , TokenType::SemiColon},
    {"::" , TokenType::ColonColon},
    {":"  , TokenType::Colon},
    {","  , TokenType::Comma},
    {"--" , TokenType::MinusMinus},
    {"-=" , TokenType::MinusEq},
    {"-"  , TokenType::Minus},
    {"++" , TokenType::PlusPlus},
    {"+=" , TokenType::PlusEq},
    {"+"  , TokenType::Plus},
    {"*=" , TokenType::MulEq},
    {"*"  , TokenType::Mul},
    {"%=" , TokenType::ModEq},
    {"%"  , TokenType::Mod},
    {"/=" , TokenType::DivEq},
    {"/"  , TokenType::Div},
    {"|=" , TokenType::OrEq},
    {"||" , TokenType::OrOr},
    {"|"  , TokenType::Or},
    {"&=" , TokenType::AndEq},
    {"&&" , TokenType::AndAnd},
    {"&"  , TokenType::And},
    {"==" , TokenType::EqEq},
    {"="  , TokenType::Eq},
    {"!=" , TokenType::NotEq},
    {"!"  , TokenType::Not},
    {"<<=", TokenType::ShlEq},
    {"<<" , TokenType::Shl},
    {"<=" , TokenType::LessEq},
    {"<"  , TokenType::Less},
    {">>=", TokenType::ShrEq},
    {">>" , TokenType::Shr},
    {">=" , TokenType::GreaterEq},
    {">"  , TokenType::Greater},
};

static const std::unordered_map<TokenType, std::string> printableToken = {
    // Terminal
    {TokenType::EndOfFile  ,"end of file"},
    {TokenType::ParseError ,"parse error"},
    // Values
    {TokenType::ID         ,"identifier"},
    {TokenType::StringLit  ,"string"},
    {TokenType::CharLit    ,"character"},
    {TokenType::IntLit     ,"integer literal"},
    {TokenType::DoubleLit  ,"double literal"},
    // Puncts
    {TokenType::OCurly     ,"`{`"},
    {TokenType::CCurly     ,"`}`"},
    {TokenType::OParen     ,"`(`"},
    {TokenType::CParen     ,"`)`"},
    {TokenType::OBracket   ,"`[`"},
    {TokenType::CBracket   ,"`]`"},
    {TokenType::Not        ,"`!`"},
    {TokenType::Mul        ,"`*`"},
    {TokenType::Div        ,"`/`"},
    {TokenType::Mod        ,"`%`"},
    {TokenType::And        ,"`&`"},
    {TokenType::AndAnd     ,"`&&`"},
    {TokenType::Plus       ,"`+`"},
    {TokenType::PlusPlus   ,"`++`"},
    {TokenType::Minus      ,"`-`"},
    {TokenType::MinusMinus ,"`--`"},
    {TokenType::Less       ,"`<`"},
    {TokenType::LessEq     ,"`<=`"},
    {TokenType::Greater    ,"`>`"},
    {TokenType::GreaterEq  ,"`>=`"},
    {TokenType::Or         ,"`|`"},
    {TokenType::OrOr       ,"`||`"},
    {TokenType::NotEq      ,"`!=`"},
    {TokenType::Eq         ,"`=`"},
    {TokenType::EqEq       ,"`==`"},
    {TokenType::Shl        ,"`<<`"},
    {TokenType::ShlEq      ,"`<<=`"},
    {TokenType::Shr        ,"`>>`"},
    {TokenType::ShrEq      ,"`>>=`"},
    {TokenType::ModEq      ,"`%=`"},
    {TokenType::OrEq       ,"`|=`"},
    {TokenType::AndEq      ,"`&=`"},
    {TokenType::PlusEq     ,"`+=`"},
    {TokenType::MinusEq    ,"`-=`"},
    {TokenType::MulEq      ,"`*=`"},
    {TokenType::DivEq      ,"`/=`"},
    {TokenType::Question   ,"`?`"},
    {TokenType::Colon      ,"`:`"},
    {TokenType::ColonColon ,"`::`"},
    {TokenType::SemiColon  ,"`;`"},
    {TokenType::Comma      ,"`,`"},
    {TokenType::Hash       ,"`#`"},
    {TokenType::BSlash     ,"`\\`"},
    {TokenType::SQoute     ,"`\'`"},
    {TokenType::DQoute     ,"`\"`"},
    {TokenType::Dot        ,"`.`"},
    {TokenType::Arrow      ,"`->`"},
    // Keyword
    {TokenType::TypeID     ,"typeid"},
    {TokenType::Extern     ,"keyword `extern`"},
    {TokenType::If         ,"keyword `if`"},
    {TokenType::Else       ,"keyword `else`"},
    {TokenType::While      ,"keyword `while`"},
    {TokenType::Return     ,"keyword `return`"},
    {TokenType::Import     ,"keyword `import`"},
    {TokenType::Func       ,"keyword `func`"},
    {TokenType::From       ,"keyword `from`"},
    {TokenType::Include    ,"keyword `include`"},
    {TokenType::Module     ,"keyword `module`"},
    {TokenType::Struct     ,"keyword `struct`"},
    {TokenType::Const      ,"keyword `const`"},
    {TokenType::Static     ,"keyword `static`"},
};
