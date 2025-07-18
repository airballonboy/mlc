#pragma once 
#include <string>
#include <vector>
#include <any>
#include <unordered_map>

enum class TokenType {
    // Terminal
    EndOfFile,
    ParseError,
    // Values
    ID, StringLit,
    CharLit, IntLit,
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
    //Int8_t, Int16_t, 
    //Int32_t, Int64_t, 
    //String_t, Char_t,
    //Float_t, Size_t,
    TypeID,
    // Keywords
    Extern, Case, If,
    Else, While, Switch,
    Goto, Return, Import,
    Func, From, 
};
enum class Type : int{
    Int8_t = 0, Int16_t, 
    Int32_t, Int64_t, 
    String_t, Char_t,
    Float_t, Size_t,
};

const std::unordered_map<std::string, Type> TypeIds = {
    {"int16", Type::Int16_t},
    {"int"  , Type::Int32_t},
    {"int32", Type::Int32_t},
    {"int64", Type::Int64_t},
    {"long" , Type::Int64_t},
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
    {TokenType::Plus       ,"`+`"},
    {TokenType::PlusPlus   ,"`++`"},
    {TokenType::Minus      ,"`-`"},
    {TokenType::MinusMinus ,"`--`"},
    {TokenType::Less       ,"`<`"},
    {TokenType::LessEq     ,"`<=`"},
    {TokenType::Greater    ,"`>`"},
    {TokenType::GreaterEq  ,"`>=`"},
    {TokenType::Or         ,"`|`"},
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
    {TokenType::Case       ,"keyword `case`"},
    {TokenType::If         ,"keyword `if`"},
    {TokenType::Else       ,"keyword `else`"},
    {TokenType::While      ,"keyword `while`"},
    {TokenType::Switch     ,"keyword `switch`"},
    {TokenType::Goto       ,"keyword `goto`"},
    {TokenType::Return     ,"keyword `return`"},
    {TokenType::Import     ,"keyword `import`"},
    {TokenType::Func       ,"keyword `func`"},
    {TokenType::From       ,"keyword `from`"},
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
    {"|"  , TokenType::Or},
    {"&=" , TokenType::AndEq},
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

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"extern", TokenType::Extern},
    {"case"  , TokenType::Case},
    {"if"    , TokenType::If},
    {"else"  , TokenType::Else},
    {"while" , TokenType::While},
    {"switch", TokenType::Switch},
    {"goto"  , TokenType::Goto},
    {"return", TokenType::Return},
    {"import", TokenType::Import},
    {"func"  , TokenType::Func},
    {"from"  , TokenType::From},
};

typedef struct {
    
}statment;

typedef struct {
    std::string name;
    //typeid returnType;
    //std::vector<var> args;
    std::string id;
    std::vector<statment> body;
}func;


typedef struct LocStruct{
    size_t line   = 1;
    size_t offset = 1;
    std::string inputPath;
}Loc;

typedef struct {
    TokenType type;
    Loc loc;
    std::string string_value;
    int         int_value;
}Token;

enum class Op {
    LOAD_CONST,
    STORE_VAR,
    LOAD_VAR,
    ADD,
    SUB,
    MUL,
    DIV,
    // RETURN(return_type, return_value)
    RETURN,
    CALL,
};

struct Instruction {
    Op op;
    std::vector<std::any> args;
};

typedef std::vector<Instruction> Program;       
