#include "parser.h"
#include "context.h"
#include "lexar.h"
#include "tools/file.h"
#include "tools/logger.h"
#include "types.h"
#include <any>
#include <cstdint>
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
size_t current_offset = 0;
size_t max_locals_offset = 8;
size_t statement_count = 0;
std::string current_module_prefix{};
std::vector<std::string> included_files;

// TODO: fix this and remove it
bool eq = false;

int get_cond_precedence(TokenType tt) {
    switch (tt) {
        case TokenType::Less:
        case TokenType::LessEq:
        case TokenType::Greater:
        case TokenType::GreaterEq:
            return 30; // relational
        case TokenType::EqEq:
        case TokenType::NotEq:
            return 20; // equality
        case TokenType::AndAnd:
            return 10; // logical AND
        case TokenType::OrOr:
            return 5;  // logical OR
        default:
            return -1;
    }
}
Parser::Parser(Lexar* lexar){
    m_currentLexar = lexar;
}
Variable& Parser::parseVariable(VariableStorage& var_store, bool member){
    Variable var{};
    std::string type_name;
    bool strct = false;
    if (TypeIds.find((*tkn)->string_value) == TypeIds.end()) {
        m_currentLexar->getAndExpectNext({TokenType::TypeID, TokenType::ID});
    }
    if (TypeIds.find((*tkn)->string_value) != TypeIds.end() && TypeIds.at((*tkn)->string_value) == Type::Struct_t) {
        type_name = (*tkn)->string_value;
        strct = true;
    } else {
        var.type = TypeIds.at(m_currentLexar->currentToken->string_value);
        var.size = variable_size_bytes(var.type);
    }

    while (m_currentLexar->peek()->type == TokenType::Mul) {
        m_currentLexar->getAndExpectNext(TokenType::Mul);
        var.size = 8;
        var.kind.pointer_count++;
    }
    m_currentLexar->getAndExpectNext(TokenType::ID);
    if (strct) {
        std::string struct_name = (*tkn)->string_value;
        Variable new_var = initStruct(type_name, struct_name);
        new_var.kind = var.kind;
        if (var.kind.pointer_count > 0) new_var.size = 8;
        var = new_var;
    } else if (!member) {
        current_offset += var.size;
    }


    if (variable_exist_in_storage(m_currentLexar->currentToken->string_value, var_store) && var.type != Type::Struct_t) 
        ERROR(m_currentLexar->currentToken->loc, f("redifinition of {}", m_currentLexar->currentToken->string_value));
    else 
        var.name = m_currentLexar->currentToken->string_value;    

    var.offset = current_offset;
    // TODO: support arrays
    if (m_currentLexar->peek()->type == TokenType::OBracket) {
        m_currentLexar->getAndExpectNext(TokenType::OBracket);
        if (m_currentLexar->peek()->type == TokenType::IntLit)
            m_currentLexar->getAndExpectNext(TokenType::IntLit);
        m_currentLexar->getAndExpectNext(TokenType::CBracket);
        TODO("support for arrays");   
    }
    var_store.push_back(var);


    return var_store[var_store.size() - 1];
}
Program* Parser::parse() {
    tkn = &m_currentLexar->currentToken;

    m_currentFuncStorage = &m_program.func_storage;

    while ((*tkn)->type != TokenType::EndOfFile) {
        switch((*tkn)->type) {
            case TokenType::Func:{
                parseFunction();
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
            case TokenType::Struct: {
                parseStructDeclaration();
                m_currentLexar->getNext();
            }break;//TokenType::Struct
            default: {
                std::println(stderr, "unimplemented type of {} at {}:{}:{}", printableToken.at((*tkn)->type), (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset);
                m_currentLexar->getNext();

            }break;
        }
    }
    return &m_program;

}
Struct& Parser::get_struct_from_name(std::string name) {
    for (auto& strct_ : m_program.struct_storage) {
        if (strct_.name == name) return strct_;
    }
    TODO("struct not found");
}
Variable Parser::initStruct(std::string type_name, std::string struct_name) {
    //default
    int strct_offset = 0;
    Struct* strct = nullptr;

    strct = &get_struct_from_name(type_name);
    
    if (strct == nullptr) TODO("trying to access a struct that doesn't exist");
    Variable struct_var = {.type = Type::Struct_t, .name = struct_name, .size = strct->size, ._type_name = type_name};
    current_offset += strct->size;
    for (auto var : strct->var_storage) {
        var.offset = current_offset - strct_offset;
        std::string orig = var.name;
        var.name = struct_name + "___" + orig;
        strct_offset += var.size;
        m_currentFunc->local_variables.push_back(var);
        //std::println("{} {} {} {} {}", (*tkn)->loc.line, var.name, (int)var.type, var.size, var.offset);
    }

    return struct_var;
}
static size_t current_struct_id = 1;
void Parser::parseStructDeclaration() {
    tkn = &m_currentLexar->currentToken;
    std::string struct_name = "";
    size_t struct_index = m_program.struct_storage.size();
    m_program.struct_storage.push_back({});
    Struct& current_struct = m_program.struct_storage[struct_index];
    current_struct.id = current_struct_id++;
    
    m_currentLexar->getAndExpectNext({TokenType::ID, TokenType::OCurly});

    if ((*tkn)->type == TokenType::ID) {
        struct_name = (*tkn)->string_value;
        m_currentLexar->getAndExpectNext(TokenType::OCurly);
    }
    TypeIds.emplace(struct_name, Type::Struct_t);
    current_struct.name = struct_name;
    while ((*tkn)->type != TokenType::CCurly) {
        if ((*tkn)->type == TokenType::Func) {
            size_t func_index = m_program.func_storage.size();
            parseFunction(true, current_struct);
            auto& member_func = m_program.func_storage[func_index];
            member_func.name = struct_name + "___" + member_func.name;
            Variable this_pointer = {
                .type       = Type::Struct_t,
                .name       = "this", .offset = 8,
                .size       = 8,
                ._type_name = struct_name,
                .kind       = {
                    .pointer_count=1
                }
            };
            member_func.arguments.emplace(member_func.arguments.begin(), this_pointer);
            member_func.arguments_count++;

            m_currentLexar->getNext();
            continue;
        }
        auto& var = parseVariable(current_struct.var_storage, true);
        var.offset = current_struct.size;
        current_struct.size += var.size;
        m_currentLexar->getAndExpectNext(TokenType::SemiColon);
        m_currentLexar->getNext();

#if 0 // TODO: setting the default value is currently unsupported should be added later
        if (var.type == Type::String_t) {
            m_currentFunc->local_variables.push_back(var);
            m_currentFunc->body.push_back({Op::INIT_STRING, {var}});
        }
        if (m_currentLexar->peek()->type == TokenType::Eq) {
            m_currentLexar->getAndExpectNext(TokenType::Eq);
            m_currentLexar->getNext();
            auto var2 = parseExpression();
            m_currentFunc->body.push_back({Op::STORE_VAR, {var2, var}});
        } else if (var.type != Type::String_t) {
            Variable default_val;
            default_val.type = Type::Int_lit;

            default_val.name = "def_value";
            default_val.value = variable_default_value(var.type);
            m_currentFunc->body.push_back({Op::STORE_VAR, {default_val, var}});
        }
#endif
    }
    
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
            func.local_variables.push_back(parseVariable(func.arguments));
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
Func Parser::parseFunction(bool member, Struct parent){
    tkn = &m_currentLexar->currentToken;
    current_offset = 0;
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

    if (member) {
        Variable this_pointer = {
            .type       = Type::Struct_t,
            .name       = "this", .offset = 8,
            .size       = 8,
            ._type_name = parent.name,
            .kind       = {
                .pointer_count=1
            }
        };
        func.local_variables.push_back(this_pointer);
        current_offset += 8;
    }
    m_currentLexar->getAndExpectNext(TokenType::OParen);
    VariableStorage temp_var_storage{};
    while (m_currentLexar->peek()->type != TokenType::CParen && (*tkn)->type != TokenType::EndOfFile && m_currentLexar->peek()->type != TokenType::EndOfFile) {
        parseVariable(temp_var_storage);
        if(m_currentLexar->peek()->type != TokenType::CParen) {
            m_currentLexar->expectNext(TokenType::Comma);
            m_currentLexar->getNext();
        }
    }
    for (auto var : temp_var_storage) {
        if (var.type == Type::Struct_t) {
            if (var.kind.pointer_count > 0) {
                func.arguments.push_back(var);
                func.local_variables.push_back(var);
                func.arguments_count++;
                continue;
            }
            if (var.size <= 8) {
                func.arguments.push_back(var);
                func.local_variables.push_back(var);
                func.arguments_count++;
            } else if (var.size <= 16) {
                size_t base_offset = var.offset;
                auto var1 = var;
                auto var2 = var;
                var1.size = 8;
                var2.offset -= 8;
                var2.size = var.size - 8;
                func.arguments.push_back(var1);
                func.local_variables.push_back(var1);
                func.arguments.push_back(var2);
                func.local_variables.push_back(var2);
                func.arguments_count += 2;
            } else {
                auto var_ptr = var;
                var_ptr.kind.pointer_count = 1;
                var_ptr.size = 8;
                func.arguments.push_back(var_ptr);
                func.local_variables.push_back(var);
                var_ptr.deref_count = 1;
                func.body.push_back({Op::STORE_VAR, {var_ptr, var}});
                func.arguments_count++;
            }
        } else {
            func.arguments.push_back(var);
            func.local_variables.push_back(var);
            func.arguments_count++;
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
                
    m_currentLexar->getAndExpectNext(TokenType::OCurly);

    // For recursion
    auto func_index = m_program.func_storage.size();
    m_program.func_storage.push_back(func);

    parseBlock();

    func.stack_size = max_locals_offset;
    max_locals_offset = 8;
    m_program.func_storage[func_index] = func;

    return func;
}
size_t temp_offset = 0;
Variable make_temp_var(Type type, size_t size, Variable old = {}) {
    Variable var;
    var.type = type;
    var.size = size;
    var.offset = current_offset + temp_offset + size;
    temp_offset += size;
    var.name = "temporery variable";
    //var.deref_count = old.deref_count;
    //var.value = old.value;
    return var;
}
void delete_temp_vars() {
    temp_offset = 0;
}

void Parser::parseStatement(){
    statement_count++;
    switch ((*tkn)->type) {
        case TokenType::SemiColon: { }break;
        case TokenType::OCurly: {
            parseBlock();
        }break;//TokenType::OCurly
        case TokenType::Return: {
            Variable return_value;
            m_currentLexar->getNext();

            if ((*tkn)->type == TokenType::SemiColon) {
                if (m_currentFunc->return_type != Type::Void_t) TODO("error on no return");
                return_value = {.type = Type::Int_lit, .name = "IntLit", .value = (int64_t)0, .size = 4};
                m_currentFunc->body.push_back({Op::RETURN, {return_value}});
                break;
            }
            eq = true;
            return_value = std::get<0>(parseExpression());
            eq = false;
            m_currentFunc->body.push_back({Op::RETURN, {return_value}});
            m_currentLexar->getAndExpectNext(TokenType::SemiColon);

        }break;//TokenType::Return
        case TokenType::If: {
            m_currentLexar->getAndExpectNext(TokenType::OParen);
            m_currentLexar->getNext();
            eq = true;
            auto expr = std::get<0>(parseExpression());
            eq = false;
            m_currentLexar->getAndExpectNext(TokenType::CParen);

            delete_temp_vars();

            m_currentLexar->getNext();
            size_t jmp_if_not = m_currentFunc->body.size();
            m_currentFunc->body.push_back({Op::JUMP_IF_NOT, {"", expr}});
            parseStatement();
            std::string label = std::format("{:06x}", statement_count++);
            m_currentFunc->body.push_back({Op::LABEL, {label}});
            m_currentFunc->body[jmp_if_not].args[0] = label;
        }break;//TokenType::If
        case TokenType::While: {
            m_currentLexar->getAndExpectNext(TokenType::OParen);
            m_currentLexar->getNext();
            std::string pre_label = std::format("{:06x}", statement_count++);
            m_currentFunc->body.push_back({Op::LABEL, {pre_label}});
            eq = true;
            auto expr = std::get<0>(parseExpression());
            eq = false;
            m_currentLexar->getAndExpectNext(TokenType::CParen);

            delete_temp_vars();

            m_currentLexar->getNext();
            size_t jmp_if_not = m_currentFunc->body.size();
            m_currentFunc->body.push_back({Op::JUMP_IF_NOT, {"", expr}});
            parseStatement();
            m_currentFunc->body.push_back({Op::JUMP, {pre_label}});
            std::string label = std::format("{:06x}", statement_count++);
            m_currentFunc->body.push_back({Op::LABEL, {label}});
            m_currentFunc->body[jmp_if_not].args[0] = label;
        }break;//TokenType::While
        case TokenType::ID:
        case TokenType::TypeID: {
            if (TypeIds.find((*tkn)->string_value) == TypeIds.end()) goto defau;
            auto var = parseVariable(m_currentFunc->local_variables);

            // TODO: factor out to a function
            if (var.type == Type::String_t) {
                m_currentFunc->local_variables.push_back(var);
                m_currentFunc->body.push_back({Op::INIT_STRING, {var}});
            }
            if (m_currentLexar->peek()->type == TokenType::Eq) {
                m_currentLexar->getAndExpectNext(TokenType::Eq);
                m_currentLexar->getNext();
                eq = true;
                auto var2 = std::get<0>(parseExpression());
                eq = false;
                m_currentFunc->body.push_back({Op::STORE_VAR, {var2, var}});
            } else if (var.type != Type::String_t && var.type != Type::Struct_t) {
                Variable default_val;
                default_val.type = Type::Int_lit;
                default_val.size = 4;

                default_val.name = "def_value";
                default_val.value = variable_default_value(var.type);
                m_currentFunc->body.push_back({Op::STORE_VAR, {default_val, var}});
            }

            m_currentLexar->getAndExpectNext(TokenType::SemiColon);
        }break;//TokenType::TypeID
        default: {
        defau:
            auto data = parsePrimaryExpression();
            auto &[lhs, lvalue] = data;
            auto peek_type = m_currentLexar->peek()->type;

            if (lvalue) {
                if (peek_type == TokenType::Eq || peek_type == TokenType::PlusEq || peek_type == TokenType::MinusEq || peek_type == TokenType::MulEq) {
                    m_currentLexar->getNext();
                    eq = true;
                    m_currentLexar->getNext();
                    auto rhs = std::get<0>(parseExpression());

                    switch (peek_type) {
                    case TokenType::Eq:
                        m_currentFunc->body.push_back({Op::STORE_VAR, {rhs, lhs}});
                        break;
                    case TokenType::PlusEq:
                        m_currentFunc->body.push_back({Op::ADD, {lhs, rhs, lhs}});
                        break;
                    case TokenType::MinusEq:
                        m_currentFunc->body.push_back({Op::SUB, {lhs, rhs, lhs}});
                        break;
                    case TokenType::MulEq:
                        m_currentFunc->body.push_back({Op::MUL, {lhs, rhs, lhs}});
                        break;
                    default: TODO("unsupported Token found");
                    }

                    eq = false;
                }
            }

            m_currentLexar->getAndExpectNext(TokenType::SemiColon);
        
            //ERROR(m_currentLexar->currentToken->loc, "unsupported token");
            //exit(1);
        }
    }
    delete_temp_vars();
}

void Parser::parseBlock(){
    size_t offset = current_offset;
    auto storage = m_currentFunc->local_variables;

    while (m_currentLexar->peek()->type != TokenType::CCurly) {
        m_currentLexar->getNext();
        parseStatement();
    }
    m_currentLexar->getAndExpectNext(TokenType::CCurly);

    if (current_offset > max_locals_offset) max_locals_offset = current_offset;

    m_currentFunc->local_variables = storage;
    current_offset = offset;

}
std::tuple<Variable, bool> Parser::parsePrimaryExpression() {
    // will need it later
    std::tuple<Variable, bool> data;
    tkn = &m_currentLexar->currentToken;
    Variable var;
    bool ret_lvalue = false;

    //m_currentLexar->getAndExpectNext({TokenType::And, TokenType::Mul, TokenType::DQoute, TokenType::IntLit, TokenType::ID});

    if((*tkn)->type == TokenType::DQoute) {
        m_currentLexar->getAndExpectNext(TokenType::StringLit);
        if((*tkn)->type == TokenType::StringLit) {
            var = {.type = Type::String_lit, .name = f("string_literal_{}", stringLiteralCount++), .value = (*tkn)->string_value, .size = 8};
            m_program.var_storage.push_back(var);
            m_currentLexar->getAndExpectNext(TokenType::DQoute);
        }
        return {var, ret_lvalue};
    }

    if ((*tkn)->type == TokenType::IntLit) {
        var = {.type = Type::Int_lit, .name = "Int_Lit", .value = (*tkn)->int_value, .size = 4};
        return {var, ret_lvalue};
    }
    
    if ((*tkn)->type == TokenType::PlusPlus) {
        auto loc = (*tkn)->loc;
        m_currentLexar->getNext();
        size_t add_loc = m_currentFunc->body.size();
        m_currentFunc->body.push_back({Op::ADD, {}});
        data = parsePrimaryExpression();
        auto &[var, lvalue] = data;
        if (!lvalue) ERROR(loc, "cannot pre-increment a non lvalue");
        Variable amount = {.type = Type::Int_lit, .name = "Int_literal", .value = (int64_t)1, .size = 4};
        m_currentFunc->body[add_loc].args = {var, amount, var};
        ret_lvalue = true;
        return {var, ret_lvalue};
    }
    if ((*tkn)->type == TokenType::MinusMinus) {
        auto loc = (*tkn)->loc;
        m_currentLexar->getNext();
        size_t add_loc = m_currentFunc->body.size();
        m_currentFunc->body.push_back({Op::SUB, {}});
        data = parsePrimaryExpression();
        auto &[var, lvalue] = data;
        if (!lvalue) ERROR(loc, "cannot pre-decrement a non lvalue");
        Variable amount = {.type = Type::Int_lit, .name = "Int_literal", .value = (int64_t)1, .size = 4};
        m_currentFunc->body[add_loc].args = {var, amount, var};
        ret_lvalue = true;
        return {var, ret_lvalue};
    }
    if ((*tkn)->type == TokenType::Mul) { 
        m_currentLexar->getNext();
        data = parsePrimaryExpression();
        auto &[var, lvalue] = data;
        var.deref_count++;
        ret_lvalue = true;
        return {var, ret_lvalue};
    }
    if ((*tkn)->type == TokenType::And) {
        Loc loc = (*tkn)->loc;
        m_currentLexar->getNext();
        data = parsePrimaryExpression();
        auto &[var, lvalue] = data;
        if (!lvalue) ERROR(loc, "cannot refernce a non lvalue");
        var.deref_count = -1;
        return {var, ret_lvalue};
    }
    current_module_prefix = "";
    if (m_currentLexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_currentLexar->getAndExpectNext(TokenType::ID);
    }
    Variable this_ptr = {.type = Type::Void_t};
    std::string struct_prefix{};
    std::string struct_func_prefix{};
    while (m_currentLexar->peek()->type == TokenType::Dot) {
        struct_prefix += (*tkn)->string_value;
        struct_prefix += "___";
        auto strct = get_var_from_name((*tkn)->string_value, m_currentFunc->local_variables);
        struct_func_prefix += strct._type_name;
        struct_func_prefix += "___";
        //asm("int3");
        this_ptr = strct;
        this_ptr.deref_count = -1;
        this_ptr.kind.pointer_count += 1;

        m_currentLexar->getAndExpectNext(TokenType::Dot);
        m_currentLexar->getAndExpectNext(TokenType::ID);
    }
    std::string name      = current_module_prefix + struct_prefix      + m_currentLexar->currentToken->string_value;
    std::string func_name = current_module_prefix + struct_func_prefix + m_currentLexar->currentToken->string_value;
    if ((*tkn)->type == TokenType::ID) {
        if (m_currentLexar->peek()->type == TokenType::OParen) {
            if (!function_exist_in_storage(func_name, m_program.func_storage)) {std::println("{}", func_name);TODO("func doesn't exist");}
            auto& func = get_func_from_name(func_name, m_program.func_storage);
            parseFuncCall(func, this_ptr);
            var = make_temp_var(func.return_type, variable_size_bytes(func.return_type));
            if (eq)
                m_currentFunc->body.push_back({Op::STORE_RET, {var}});
            return {var, ret_lvalue};
        }
        if (variable_exist_in_storage((*tkn)->string_value, m_program.var_storage)) {
            std::println("{}",(*tkn)->string_value);
            TODO(f("check Global variables at {}:{}:{}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset));
            return {var, true};

        }else if (variable_exist_in_storage(name, m_currentFunc->local_variables)) {
            var = get_var_from_name(name, m_currentFunc->local_variables);
            ret_lvalue = true;
            return {var, ret_lvalue};
        }else if (this_ptr.type != Type::Void_t) {
            auto strct = get_struct_from_name(this_ptr._type_name);
            auto var_ = get_var_from_name((*tkn)->string_value, strct.var_storage);
            this_ptr.size = var_.size;
            this_ptr.type = var_.type;
            this_ptr.name = var_.name;
            this_ptr.deref_count = 0;
            this_ptr.kind.deref_offset = var_.offset;
            this_ptr.kind.pointer_count -= 1;

            return {this_ptr, true};

            //auto strct = get_struct_from_name(this_ptr._type_name);
            //auto var_ = get_var_from_name((*tkn)->string_value, strct.var_storage);
            //auto temp = make_temp_var(var_.type, var_.size);
            //m_currentFunc->body.push_back({Op::DEREF_OFFSET, {var_.offset, this_ptr, temp}});
            //return {temp, false};
        } else {
            TODO(f("ERROR: variable `{}` at {}:{}:{} wasn't found", name, (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset));
        }
    }

    // Paranthesis
    if ((*tkn)->type == TokenType::OParen) {
        m_currentLexar->getNext(); 
        data = parseExpression();
        auto &[var, lvalue] = data;
        m_currentLexar->getAndExpectNext(TokenType::CParen);
        return {var, ret_lvalue};
    }

    ERROR((*tkn)->loc, f("unexpected token of type {}", printableToken.at((*tkn)->type)));

    return {var, false};
}
std::tuple<Variable, bool> Parser::parseUnaryExpression(){
    tkn = &m_currentLexar->currentToken;

    if ((*tkn)->type == TokenType::Minus) {
        m_currentLexar->getNext();
        auto rhs = std::get<0>(parseUnaryExpression());
        auto type = (rhs.type == Type::Int_lit ? Type::Int32_t : rhs.type);
        Variable result = make_temp_var(type, variable_size_bytes(type));
        Variable zero   = {.type = Type::Int_lit, .name = "Int_lit", .value = (int64_t)0, .size = 4};
        m_currentFunc->body.push_back({Op::SUB, {zero, rhs, result}});
        return {result, false};
    }
    if ((*tkn)->type == TokenType::Not) {
        m_currentLexar->getNext();
        auto rhs = std::get<0>(parseUnaryExpression());
        auto type = Type::Bool_t;
        Variable result = make_temp_var(type, variable_size_bytes(type));
        Variable zero   = {.type = Type::Int_lit, .name = "Int_lit", .value = (int64_t)0, .size = 4};
        m_currentFunc->body.push_back({Op::EQ, {rhs, zero, result}});
        return {result, false};
    }

    return parsePrimaryExpression();
}
std::tuple<Variable, bool> Parser::parseMultiplicativeExpression(){
    tkn = &m_currentLexar->currentToken;
    // TODO: fix this it should return lvalue not false
    auto lhs = std::get<0>(parseUnaryExpression());

    while ((*tkn)->type != TokenType::EndOfFile) {
        auto peek = m_currentLexar->peek()->type;
        if (peek != TokenType::Mul && peek != TokenType::Div && peek != TokenType::Mod) break;

        m_currentLexar->getNext();
        auto op_type = (*tkn)->type;
        m_currentLexar->getNext();
        auto rhs = std::get<0>(parseUnaryExpression());

        auto type = (lhs.type == Type::Int_lit ? Type::Int32_t : lhs.type);
        Variable result = make_temp_var(type, variable_size_bytes(type));

        if (op_type == TokenType::Mul) {
            m_currentFunc->body.push_back({Op::MUL, {lhs, rhs, result}});
        }else if (op_type == TokenType::Div) {
            m_currentFunc->body.push_back({Op::DIV, {lhs, rhs, result}});
        }else if (op_type == TokenType::Mod) {
            m_currentFunc->body.push_back({Op::MOD, {lhs, rhs, result}});
        }

        lhs = result;
    }

    return {lhs, false};
}
std::tuple<Variable, bool> Parser::parseAdditiveExpression(){
    tkn = &m_currentLexar->currentToken;
    auto lhs = std::get<0>(parseMultiplicativeExpression());

    while ((*tkn)->type != TokenType::EndOfFile) {
        TokenType peek = m_currentLexar->peek()->type;
        if (peek != TokenType::Plus && peek != TokenType::Minus) break;

        m_currentLexar->getNext();
        TokenType op_type = (*tkn)->type;
        m_currentLexar->getNext();
        auto rhs = std::get<0>(parseMultiplicativeExpression());


        auto type = (lhs.type == Type::Int_lit ? Type::Int32_t : lhs.type);
        Variable result = make_temp_var(type, variable_size_bytes(type));
        if (op_type == TokenType::Plus) {
            m_currentFunc->body.push_back({Op::ADD, {lhs, rhs, result}});
        }else {
            m_currentFunc->body.push_back({Op::SUB, {lhs, rhs, result}});
        }

        lhs = result;
    }

    return {lhs, false};
}
std::tuple<Variable, bool> Parser::parseCondition(int min_prec){
    tkn = &m_currentLexar->currentToken;
    auto data = parseAdditiveExpression();
    auto &[lhs, lvalue] = data;

    while ((*tkn)->type != TokenType::EndOfFile) {
        auto peek = m_currentLexar->peek()->type;
        int prec = get_cond_precedence(peek);
        if (prec < min_prec) 
            break;

        Op op;
        switch (peek) {
            case TokenType::EqEq:      op = Op::EQ;   break;
            case TokenType::NotEq:     op = Op::NE;   break;
            case TokenType::Less:      op = Op::LT;   break;
            case TokenType::LessEq:    op = Op::LE;   break;
            case TokenType::Greater:   op = Op::GT;  break;
            case TokenType::GreaterEq: op = Op::GE; break;
            case TokenType::AndAnd:    op = Op::LAND; break;
            case TokenType::OrOr:      op = Op::LOR;  break;
            default:
                return {lhs, lvalue};
        }
        // consume operator
        m_currentLexar->getNext();
        m_currentLexar->getNext();

        Variable rhs = std::get<0>(parseCondition(prec + 1));

        Variable result = make_temp_var(Type::Bool_t, variable_size_bytes(Type::Bool_t));

        m_currentFunc->body.push_back({ op, { lhs, rhs, result } });

        lhs = result;

    }

    return {lhs, false};
}
std::tuple<Variable, bool> Parser::parseExpression(){
    return parseCondition(0);
}

void Parser::parseFuncCall(Func func, Variable this_ptr){
    VariableStorage args{};
    m_currentLexar->getAndExpectNext(TokenType::OParen);
    if (this_ptr.type != Type::Void_t) args.push_back(this_ptr);
    while (m_currentLexar->peek()->type != TokenType::CParen) {
        m_currentLexar->getNext();
        if (eq) {
            args.push_back(std::get<0>(parseExpression()));
        } else {
            eq = true;
            args.push_back(std::get<0>(parseExpression()));
            eq = false;
        }
        if (m_currentLexar->peek()->type != TokenType::CParen) {
            m_currentLexar->getAndExpectNext(TokenType::Comma);
        }
    }
    m_currentLexar->getAndExpectNext(TokenType::CParen);
    m_currentFunc->body.push_back({Op::CALL, {func.name, args}});

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
        case Type::Int64_t:  return (int64_t)0; break;
        case Type::Float_t:  return 0         ; break;

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
        case Type::Int_lit:  return 4; break;
        case Type::Int64_t:  return 8; break;
        case Type::Size_t:   return 8; break;
        case Type::Float_t:  return 4; break;

        case Type::String_t:   return 8; break;
        case Type::String_lit: return 8; break;
        case Type::Void_t:   return 0; break;

        default: 
            TODO(f("type {} doesn't have size", (int)t));
    }
    return 0;
}
