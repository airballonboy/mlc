#pragma once 
#include <functional>
#include <string>
#include <typeindex>
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
    UScore,
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
    Func, From, Include,
    Module, 
};
enum class Type : int {
    Void_t = 0, 
    Int8_t, Int16_t, 
    Int32_t, Int64_t, 
    String_t, Char_t,
    Float_t, Size_t,
    Bool_t,
    String_lit, Int_lit
};
enum class Op {
    LOAD_CONST,
    // stores variable1 into variable2
    // STORE_VAR(variable1, variable2)
    STORE_VAR,
    // stores the return of the last function called
    // STORE_RET(variable)
    STORE_RET,
    LOAD_VAR,
    ADD,
    SUB,
    MUL,
    DIV,
    // RETURN(variable)
    RETURN,
    // CALL(func_name, args)
    CALL,
};


inline std::unordered_map<std::string, Type> TypeIds = {
    {"void"  , Type::Void_t},
    {"int16" , Type::Int16_t},
    {"int"   , Type::Int32_t},
    {"int32" , Type::Int32_t},
    {"int64" , Type::Int64_t},
    {"long"  , Type::Int64_t},
    {"string", Type::String_t},
    {"float" , Type::Float_t},
    {"usize" , Type::Size_t},
    {"bool"  , Type::Bool_t}
};


inline std::unordered_map<Type, std::function<std::string(std::any)>> TypeToString = {
    {Type::Int8_t    , [](const std::any& val){ return std::to_string(std::any_cast<int8_t> (val)); }},
    {Type::Int16_t   , [](const std::any& val){ return std::to_string(std::any_cast<int16_t>(val)); }},
    {Type::Int_lit   , [](const std::any& val){ return std::to_string(std::any_cast<int32_t>(val)); }},
    {Type::Int32_t   , [](const std::any& val){ return std::to_string(std::any_cast<int32_t>(val)); }},
    {Type::Int64_t   , [](const std::any& val){ return std::to_string(std::any_cast<int64_t>(val)); }},
    {Type::String_t  , [](const std::any& val){ return std::any_cast<std::string>(val);             }},
    {Type::String_lit, [](const std::any& val){ return std::any_cast<std::string>(val);             }},
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
    {TokenType::Include    ,"keyword `include`"},
    {TokenType::Module     ,"keyword `module`"},
};

static const std::unordered_map<std::string, TokenType> PUNCTUATION = {
    {"_" ,  TokenType::UScore},
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
    {"extern" , TokenType::Extern},
    {"case"   , TokenType::Case},
    {"if"     , TokenType::If},
    {"else"   , TokenType::Else},
    {"while"  , TokenType::While},
    {"switch" , TokenType::Switch},
    {"goto"   , TokenType::Goto},
    {"return" , TokenType::Return},
    {"import" , TokenType::Import},
    {"func"   , TokenType::Func},
    {"from"   , TokenType::From},
    {"include", TokenType::Include},
    {"module" , TokenType::Module},
};


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

struct Instruction {
    Op op;
    std::vector<std::any> args;
};
struct Variable {
    Type        type;
    std::string name;
    std::any    value;
    size_t      offset;
};

typedef std::vector<Variable> VariableStorage;

struct Func {
    Type return_type = Type::Void_t;

    int arguments_count = 0;
    VariableStorage arguments{};

    std::string name{};
    
    VariableStorage local_variables{};
    size_t stack_size = 0;

    bool external = false;
    std::string link_name{};
    std::string lib{};
    std::string search_path{};

    std::vector<Instruction> body{};
};

typedef std::vector<Func> FunctionStorage;

struct Module {
    std::string name{};
    std::unordered_map<std::string, Module> module_storage;
    FunctionStorage func_storage;
    VariableStorage var_storage;
};
//typedef std::vector<Module> ModuleStorage;
typedef std::unordered_map<std::string, Module> ModuleStorage;

struct Program {
    ModuleStorage   module_storage;
    FunctionStorage func_storage;
    VariableStorage var_storage;
};

inline std::string input_no_extention;
inline std::string input_path;
inline std::string build_path;

