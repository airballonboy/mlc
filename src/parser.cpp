#include "parser.h"
#include "context.h"
#include "lexar.h"
#include "tools/file.h"
#include "tools/logger.h"
#include "types.h"
#include <any>
#include <print>
#include <string_view>
#include <vector>

#define ERROR(loc, massage) \
    do { \
        mlog::log(mlog::Red,     \
                   "ERROR:\n", \
                 mlog::Cyan,    \
                 f("  {}:{}:{} {}", (loc).inputPath, (loc).line, (loc).offset, (massage)).c_str()); \
        exit(1); \
    } while(0)

int stringLiteralCount = 0;
int stringCount = 0;
size_t current_locals_count = 1;
size_t max_locals_count = 1;
std::string current_module_prefix{};
std::vector<std::string> included_files;

Parser::Parser(Lexar* lexar){
    m_currentLexar = lexar;
}
Variable Parser::parseVariable(){
    Variable var;
    if (m_currentLexar->currentToken->type != TokenType::TypeID)
        m_currentLexar->getAndExpectNext(TokenType::TypeID);
    var.type = TypeIds.at(m_currentLexar->currentToken->string_value);

    while (m_currentLexar->peek()->type == TokenType::Mul)
        m_currentLexar->getAndExpectNext(TokenType::Mul);

    m_currentLexar->getAndExpectNext(TokenType::ID);

    if (variable_exist_in_storage(m_currentLexar->currentToken->string_value, m_currentFunc->local_variables)) 
        ERROR(m_currentLexar->currentToken->loc, f("redifinition of {}", m_currentLexar->currentToken->string_value));
    else 
        var.name = m_currentLexar->currentToken->string_value;    

    if(var.type == Type::String_t)
        var.offset = stringCount++;
    else 
        var.offset = current_locals_count++*8;
    // TODO: support arrays
    if (m_currentLexar->peek()->type == TokenType::OBracket) {
        m_currentLexar->getAndExpectNext(TokenType::OBracket);
        if (m_currentLexar->peek()->type == TokenType::IntLit)
            m_currentLexar->getAndExpectNext(TokenType::IntLit);
        m_currentLexar->getAndExpectNext(TokenType::CBracket);
        TODO("support for arrays");   
    }
    m_currentFunc->local_variables.push_back(var);


    return var;
}
Program* Parser::parse() {
    tkn = &m_currentLexar->currentToken;

    m_currentFuncStorage = &m_program.func_storage;

    while ((*tkn)->type != TokenType::EndOfFile) {
        switch((*tkn)->type) {
            case TokenType::Func:{
                m_program.func_storage.push_back(parseFunction());
                m_currentLexar->getNext();
                
            }break;//TokenType::Func
            case TokenType::Module: {
                parseModuleDeclaration();
                m_currentLexar->getNext();
            }break;//TokenType::Module
            case TokenType::Hash: {
                parseHash();
                m_currentLexar->getNext();
            }break;//TokenType::Hash
            default: {
                std::println(stderr, "unimplemented type of {} at {}:{}:{}", printableToken.at((*tkn)->type), (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset);
                m_currentLexar->getNext();

            }break;
        }
    }
    return &m_program;

}
void Parser::parseModuleDeclaration() {
    m_currentLexar->getAndExpectNext(TokenType::ID);

    ModuleStorage* current_mod = &m_program.module_storage;

    if (m_program.module_storage.contains((*tkn)->string_value)) {
        if (m_currentLexar->peek()->type == TokenType::ColonColon) {
            current_mod = &current_mod->at((*tkn)->string_value).module_storage;
            m_currentLexar->getAndExpectNext(TokenType::ColonColon);
            m_currentLexar->getAndExpectNext(TokenType::ID);
        }else {
            TODO("redeclare module");
        }
    }

    current_mod->emplace((*tkn)->string_value, Module{});
    m_currentLexar->getAndExpectNext(TokenType::SemiColon);
}
void Parser::parseHash() {
    m_currentLexar->getAndExpectNext({TokenType::Import, TokenType::Include, TokenType::Extern});
    switch ((*tkn)->type ) {
        case TokenType::Include: {
            if (m_currentLexar->peek()->loc.line == (*tkn)->loc.line)
                m_currentLexar->getAndExpectNext(TokenType::Less);
            else 
                TODO("no file provided to include");

            std::string file_name{};
            while (m_currentLexar->peek()->loc.line == (*tkn)->loc.line && m_currentLexar->peek()->type != TokenType::Greater) {
                m_currentLexar->getNext();
                file_name += (*tkn)->string_value;
            }
            m_currentLexar->getNext();
            for (std::string& file : included_files) if (file == file_name) goto end_of_include;
            if (fileExistsInPaths(file_name, ctx.includePaths)) {
                std::string file_path = getFilePathFromPaths(file_name, ctx.includePaths);

                Lexar l(readFileToString(file_path), file_path);

                m_currentLexar->pushtokensaftercurrent(&l);
                included_files.push_back(file_name);
            }else {
                TODO("file not found");
            };
        end_of_include:;
        }break;//TokenType::Include
        case TokenType::Extern: {
            parseExtern();
        }break;//TokenType::Extern
        case TokenType::Import: {
            TODO("handle imports");
        }break;//TokenType::Import
    }
}
void Parser::parseExtern(){
    m_currentLexar->getAndExpectNext(TokenType::Func);

    Func func;
    m_currentFunc = &func;
    func.external = true;

    // Func name
    m_currentLexar->getAndExpectNext(TokenType::ID);
    current_module_prefix = "";
    if (m_currentLexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_currentLexar->getAndExpectNext(TokenType::ID);
    }
    func.name = current_module_prefix + m_currentLexar->currentToken->string_value;
    func.link_name = func.name;                
    current_module_prefix = "";

    m_currentLexar->getAndExpectNext(TokenType::OParen);
    while (m_currentLexar->peek()->type != TokenType::CParen && (*tkn)->type != TokenType::EndOfFile) {
        if(m_currentLexar->peek()->type == TokenType::Dot) {
            
            m_currentLexar->getAndExpectNext(TokenType::Dot);
            m_currentLexar->getAndExpectNext(TokenType::Dot);
            m_currentLexar->getAndExpectNext(TokenType::Dot);
            func.arguments_count = 1000;
            
            break;
        }else {
            func.arguments.push_back(parseVariable());
            func.arguments_count++;
        }
        // Process parameter(Local Variables)
        if(m_currentLexar->peek()->type != TokenType::CParen) {
            m_currentLexar->expectNext(TokenType::Comma);
            m_currentLexar->getNext();
        }
    }

    m_currentLexar->getAndExpectNext(TokenType::CParen);

    // Returns
    if (m_currentLexar->peek()->type == TokenType::Arrow) {
        m_currentLexar->getAndExpectNext(TokenType::Arrow);
        m_currentLexar->getAndExpectNext(TokenType::TypeID);
        func.return_type = TypeIds.at((*tkn)->string_value);
    }
    if (m_currentLexar->peek()->type == TokenType::OBracket) {
        m_currentLexar->getAndExpectNext(TokenType::OBracket);
        do {
            m_currentLexar->getAndExpectNext(TokenType::ID);
            std::string s = (*tkn)->string_value;
            m_currentLexar->getAndExpectNext(TokenType::Eq);
            m_currentLexar->getAndExpectNext(TokenType::DQoute);
            m_currentLexar->getAndExpectNext(TokenType::StringLit);
            std::string seq = (*tkn)->string_value;
            m_currentLexar->getAndExpectNext(TokenType::DQoute);

            if (s == "link_name")
                func.link_name = seq;                
            else if (s == "lib")
                func.lib = seq;                
            else if (s == "search_path")
                func.search_path = seq;                
            else 
                TODO("ERROR: UNREACHABLE");
            
            if(m_currentLexar->peek()->type != TokenType::CBracket) {
                m_currentLexar->getAndExpectNext(TokenType::Comma);
            }else {
                m_currentLexar->getAndExpectNext(TokenType::CBracket);
            }
        }while((*tkn)->type != TokenType::CBracket);
    }

    m_currentLexar->getAndExpectNext(TokenType::SemiColon);

    m_program.func_storage.push_back(func);

}
void Parser::parseModulePrefix(){
    auto current_module_storage = &m_program.module_storage;

    //if(!current_module_storage->contains((*tkn)->string_value)) TODO("error");
    if(!current_module_storage->contains((*tkn)->string_value)) { 
        std::println("{}:{}:{} tkn = {}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset, (*tkn)->string_value);
        TODO("error");
    }

    m_currentFuncStorage   = &current_module_storage->at((*tkn)->string_value).func_storage;
    current_module_storage = &m_program.module_storage.at((*tkn)->string_value).module_storage;
    current_module_prefix += (*tkn)->string_value + "__";

    m_currentLexar->getAndExpectNext(TokenType::ColonColon);

    while((m_currentLexar->peek() + 1)->type == TokenType::ColonColon){
        m_currentLexar->getAndExpectNext(TokenType::ID);
        if(!current_module_storage->contains((*tkn)->string_value)) { 
            std::println("{}:{}:{} tkn = {}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset, (*tkn)->string_value);
            TODO("error");
        }

        m_currentFuncStorage   = &current_module_storage->at((*tkn)->string_value).func_storage;
        current_module_storage = &current_module_storage->at((*tkn)->string_value).module_storage;
        current_module_prefix += (*tkn)->string_value + "__";

        m_currentLexar->getAndExpectNext(TokenType::ColonColon);
    }
}
Func Parser::parseFunction(){
    tkn = &m_currentLexar->currentToken;
    current_locals_count = 1;
    Func func;
    m_currentFunc = &func;
    m_currentLexar->getAndExpectNext(TokenType::ID);
    current_module_prefix = "";
    if (m_currentLexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_currentLexar->getAndExpectNext(TokenType::ID);
    }
    func.name = current_module_prefix + m_currentLexar->currentToken->string_value;
    current_module_prefix = "";

    m_currentLexar->getAndExpectNext(TokenType::OParen);
    while (m_currentLexar->peek()->type != TokenType::CParen && (*tkn)->type != TokenType::EndOfFile && m_currentLexar->peek()->type != TokenType::EndOfFile) {
        func.arguments.push_back(parseVariable());
        func.arguments_count++;

        // Process parameter(Local Variables)
        if(m_currentLexar->peek()->type != TokenType::CParen) {
            m_currentLexar->expectNext(TokenType::Comma);
            m_currentLexar->getNext();
        }
    }

    m_currentLexar->getAndExpectNext(TokenType::CParen);
    m_currentLexar->expectNext({TokenType::Arrow, TokenType::OCurly});

    // Returns
    if (m_currentLexar->peek()->type == TokenType::Arrow) {
        m_currentLexar->getAndExpectNext(TokenType::Arrow);
        m_currentLexar->getAndExpectNext(TokenType::TypeID);
        func.return_type = TypeIds.at((*tkn)->string_value);
    }
                

    for (auto& var : func.arguments) {
        Variable default_val;
        if (var.type == Type::String_t)
            default_val.type = Type::String_lit;
        else 
            default_val.type = Type::Int_lit;
        default_val.name = "def_value";
        default_val.value = variable_default_value(var.type);
        if (default_val.type == Type::String_lit) {
            std::string var_name = f("{}_{}", var.name, var.offset);
            m_program.var_storage.push_back({Type::String_t, var_name, std::string("")});
        } else 
            m_currentFunc->body.push_back({Op::STORE_VAR, {default_val, var}});
    }

    m_currentLexar->getAndExpectNext(TokenType::OCurly);



    parseBlock();

    //while (m_currentLexar->peek()->type != TokenType::CCurly && (*tkn)->type != TokenType::EndOfFile && m_currentLexar->peek()->type != TokenType::EndOfFile) {
    //}

    func.stack_size = max_locals_count*8;

    return func;

}
void Parser::parseStatement(){
    //m_currentLexar->getAndExpectNext({TokenType::ID, TokenType::TypeID, TokenType::OCurly, TokenType::Return});
    switch ((*tkn)->type) {
        case TokenType::SemiColon: { }break;
        case TokenType::OCurly: {
            parseBlock();
        }break;//TokenType::OCurly
        case TokenType::ID: {
            if (m_currentLexar->peek()->type == TokenType::OParen) {
                parseFuncCall();
                m_currentLexar->getAndExpectNext(TokenType::SemiColon);                
            }else if (m_currentLexar->peek()->type == TokenType::ColonColon) {
                parseModulePrefix();
            }else if (m_currentLexar->peek()->type == TokenType::Eq) {
                auto var1 = get_var_from_name((*tkn)->string_value, m_currentFunc->local_variables);
                m_currentLexar->getAndExpectNext(TokenType::Eq);
                auto var2 = parseExpression();
                m_currentFunc->body.push_back({Op::STORE_VAR, {var2, var1}});
                m_currentLexar->getAndExpectNext(TokenType::SemiColon);
            }
        
        }break;//TokenType::ID
        case TokenType::Return: {
            m_currentLexar->getAndExpectNext({TokenType::IntLit, TokenType::ID, TokenType::SemiColon});
            Variable return_value;
            if ((*tkn)->type == TokenType::SemiColon) {
                if (m_currentFunc->return_type != Type::Void_t) TODO("error on no return");
                return_value = {Type::Int_lit, "IntLit", 0};
                m_currentLexar->currentToken--;
            }else if ((*tkn)->type == TokenType::IntLit) {
                // TODO: type checker
                // it should have a map to functions called cast and take and give the type expected and
                //  if the current type has a cast to the other type then they are compatible types
                if (m_currentFunc->return_type == Type::Int8_t  || 
                    m_currentFunc->return_type == Type::Int16_t || 
                    m_currentFunc->return_type == Type::Int32_t || 
                    m_currentFunc->return_type == Type::Int64_t 
                )
                    return_value = {Type::Int_lit, "IntLit", (*tkn)->int_value};
                else if(m_currentFunc->return_type == Type::Void_t) {
                    ERROR((*tkn)->loc, "void can't return");
                }
            }else if ((*tkn)->type == TokenType::ID) {
                if (variable_exist_in_storage((*tkn)->string_value, m_currentFunc->local_variables)) {
                    return_value = get_var_from_name((*tkn)->string_value, m_currentFunc->local_variables);
                }
            }
            m_currentFunc->body.push_back({Op::RETURN, {return_value}});
            m_currentLexar->getAndExpectNext(TokenType::SemiColon);
        }break;//TokenType::Return
        case TokenType::TypeID: {
            auto var = parseVariable();

            if (m_currentLexar->peek()->type == TokenType::Eq) {
                m_currentLexar->getAndExpectNext(TokenType::Eq);
                auto var2 = parseExpression();
                m_currentFunc->body.push_back({Op::STORE_VAR, {var2, var}});
            } else {
                Variable default_val;
                if (var.type == Type::String_t) default_val.type = Type::String_lit;
                else default_val.type = Type::Int_lit;

                default_val.name = "def_value";
                default_val.value = variable_default_value(var.type);
                if (default_val.type == Type::String_lit) {
                    std::string var_name = f("{}_{}", var.name, var.offset);
                    m_program.var_storage.push_back({Type::String_t, var_name, std::string("")});
                } else {
                    m_currentFunc->body.push_back({Op::STORE_VAR, {default_val, var}});
                }
            }


            m_currentLexar->getAndExpectNext(TokenType::SemiColon);
        }break;//TokenType::TypeID
    }
}

void Parser::parseBlock(){
    size_t locals_count = current_locals_count;
    auto storage = m_currentFunc->local_variables;

    while (m_currentLexar->peek()->type != TokenType::CCurly) {
        m_currentLexar->getNext();
        parseStatement();
    }
    m_currentLexar->getAndExpectNext(TokenType::CCurly);

    if (current_locals_count > max_locals_count) max_locals_count = current_locals_count;

    m_currentFunc->local_variables = storage;
    current_locals_count = locals_count;

}

Variable Parser::parseExpression(){
    Variable var;
    tkn = &m_currentLexar->currentToken;
    int64_t deref = 0;

    m_currentLexar->getAndExpectNext({TokenType::And, TokenType::Mul, TokenType::DQoute, TokenType::IntLit, TokenType::ID});

    // Storing a function return
    if (m_currentLexar->peek()->type == TokenType::OParen) {
        parseFuncCall();
        m_currentFunc->body.push_back({Op::STORE_RET, {var}});
        m_currentLexar->getAndExpectNext(TokenType::SemiColon);
        return var;
    }
    
    while ((*tkn)->type == TokenType::Mul) { 
        m_currentLexar->getNext();
        deref++;
    }
    if((*tkn)->type == TokenType::DQoute) {
        m_currentLexar->getAndExpectNext(TokenType::StringLit);
        if((*tkn)->type == TokenType::StringLit) {
            var = {Type::String_lit, f("string_literal_{}", stringLiteralCount), (*tkn)->string_value};
            m_program.var_storage.push_back({Type::String_lit, f("string_literal_{}", stringLiteralCount++), (*tkn)->string_value});
            m_currentLexar->getAndExpectNext(TokenType::DQoute);
        }
        return var;
    }
    if ((*tkn)->type == TokenType::IntLit) {
        var = {Type::Int_lit, "Int_Lit", (*tkn)->int_value};
        return var;
    }
    if ((*tkn)->type == TokenType::And) {
        deref = -1;
        m_currentLexar->getNext();
    }
    current_module_prefix = "";
    if (m_currentLexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_currentLexar->getAndExpectNext(TokenType::ID);
    }
    std::string name = current_module_prefix + m_currentLexar->currentToken->string_value;
    if ((*tkn)->type == TokenType::ID) {
        if (m_currentLexar->peek()->type == TokenType::OParen) {
            auto& func = get_func_from_name(name, m_program.func_storage);
            parseFuncCall();
            var.offset = current_locals_count++*8;
            var.type = func.return_type;
            //std::println("{}", int(arg.type));
            m_currentFunc->body.push_back({Op::STORE_RET, {var}});
            return var;
        }
        if (variable_exist_in_storage((*tkn)->string_value, m_program.var_storage))
            TODO(f("check Global variables at {}:{}:{}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset));
        else if (variable_exist_in_storage((*tkn)->string_value, m_currentFunc->local_variables)) {
            var = get_var_from_name((*tkn)->string_value, m_currentFunc->local_variables);
            var.deref_count = deref;
        } else {
            TODO(f("ERROR: variable `{}` at {}:{}:{} wasn't found", (*tkn)->string_value, (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset));
        }
    }

    current_module_prefix = "";
    return var;
}

void Parser::parseFuncCall(){
    std::string func_name = current_module_prefix + m_currentLexar->currentToken->string_value;

    // TODO: currently uncommented but not fully working because i don't have variadic functions
    if (!function_exist_in_storage(func_name, m_program.func_storage)) {std::println("{}", func_name);TODO("func doesn't exist");}

    VariableStorage args{};
    m_currentLexar->getAndExpectNext(TokenType::OParen);
    while (m_currentLexar->peek()->type != TokenType::CParen) {
        args.push_back(parseExpression());
        if (m_currentLexar->peek()->type != TokenType::CParen) {
            m_currentLexar->getAndExpectNext(TokenType::Comma);
        }
    }
    m_currentLexar->getAndExpectNext(TokenType::CParen);
    m_currentFunc->body.push_back({Op::CALL, {func_name, args}});

    m_currentFuncStorage = &m_program.func_storage;
    current_module_prefix = "";
}

bool Parser::function_exist_in_storage(std::string_view func_name, const FunctionStorage& func_storage) {
    for (auto& func : func_storage) {
        if (func.name == func_name) return true;
    }
    return false;
}
Func& Parser::get_func_from_name(std::string_view name, FunctionStorage& func_storage) {

    if (!function_exist_in_storage(name, func_storage)) {std::println("{}", name); TODO("func doesn't exist");}

    for (auto& func : func_storage) {
        if (func.name == name) return func;
    }
    std::println("func {} was not found", name);
    TODO("func doesn't exist");
}
// OUTDATED:
bool Parser::variable_exist_in_storage(std::string_view var_name, const VariableStorage& var_storage) {
    for (const auto& var : var_storage) {
        if (var.name == var_name) return true;
    }
    return false;
}
Variable& Parser::get_var_from_name(std::string_view name, VariableStorage& var_storage) {
    for (auto& var : var_storage) {
        if (var.name == name) return var;
    }
    TODO("var doesn't exist");
}
Variable Parser::parseArgument() {
    return {};
}
std::any Parser::variable_default_value(Type t) {
    switch (t) {
        case Type::Size_t:
        case Type::Int8_t:
        case Type::Int16_t:
        case Type::Int32_t:
        case Type::Int64_t:
        case Type::Float_t:  return 0    ; break;

        case Type::String_t: return ""   ; break;
        case Type::Char_t:   return ""   ; break;
        case Type::Bool_t:   return false; break;
        case Type::Void_t:   return 0    ; break;

        default: 
            TODO(f("type {} doesn't have default", (int)t));
    }
    return 0;
}
// UNNEEDED
size_t Parser::variable_size_bytes(Type t) {
    switch (t) {
        case Type::Bool_t:   return 1; break;
        case Type::Char_t:   return 1; break;
        case Type::Int8_t:   return 1; break;
        case Type::Int16_t:  return 2; break;
        case Type::Int32_t:  return 4; break;
        case Type::Int64_t:  return 8; break;
        case Type::Size_t:   return 8; break;
        case Type::Float_t:  return 4; break;

        case Type::String_t: return 0; break;
        case Type::Void_t:   return 0; break;

        default: 
            TODO(f("type {} doesn't have default", (int)t));
    }
    return 0;
}
