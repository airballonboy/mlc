#include "parser.h"
#include "context.h"
#include "lexar.h"
#include "tools/file.h"
#include "tools/logger.h"
#include "types.h"
#include <algorithm>
#include <any>
#include <bit>
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
    } while (0)

int64_t literal_count = 0;
size_t current_offset = 0;
size_t max_locals_offset = 8;
size_t statement_count = 0;
Func default_func = {};
std::string current_module_prefix{};
std::vector<std::string> included_files;
uint8_t size_of_signed_int (int64_t i) {
    if (i <= 127)
        return 1;
    if (i <= 32767)
        return 2;
    if (i <= 2147483647l)
        return 4;
    if (i <= 9223372036854775807ll)
        return 8;
    return 0;
}
uint8_t size_of_unsigned_int (uint64_t i) {
    if (i <= 255)
        return 1;
    if (i <= 65535)
        return 2;
    if (i <= 4294967295ul)
        return 4;
    if (i <= 18446744073709551615ull)
        return 8;
    return 0;
}
TypeInfo sized_int_type(int8_t size) {
    if (size == 1)
        return type_infos.at("int8");
    if (size == 2)
        return type_infos.at("int16");
    if (size == 4)
        return type_infos.at("int32");
    if (size == 8)
        return type_infos.at("int64");
    return type_infos.at("void");
}

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
size_t Parser::align(size_t current_offset, Type type, std::string type_name) {
    size_t alignment = 0;
    if (type != Type::Struct_t) {
        if (type == Type::Void_t) return current_offset;
        alignment = variable_size_bytes(type);
    } else {
        alignment = get_struct_from_name(type_name).alignment;
    }
    return (current_offset + alignment - 1) & ~(alignment-1);
}
Parser::Parser(Lexar* lexar) {
    m_currentLexar = lexar;
}
Variable& Parser::parseVariable(VariableStorage& var_store, bool member) {
    Variable var{};
    std::string type_name{};
    bool strct = false;
    if ((*tkn)->type != TokenType::ID && (*tkn)->type != TokenType::TypeID) {
        m_currentLexar->getAndExpectNext({TokenType::TypeID, TokenType::ID});
    }
    if (m_currentLexar->peek()->type == TokenType::ColonColon) {
        current_module_prefix = "";
        parseModulePrefix();
        m_currentLexar->getAndExpectNext(TokenType::ID);
        type_name = current_module_prefix + m_currentLexar->currentToken->string_value;
        current_module_prefix = "";
    } else {
        type_name = (*tkn)->string_value;
    }
    if (TypeIds.contains(type_name) && TypeIds.at(type_name) == Type::Struct_t) {
        strct = true;
        //var.type_name = type_name;
        var.type_info = type_infos.at(type_name);
    } else if (TypeIds.contains((*tkn)->string_value)) {
        type_name = printableTypeIds.at(TypeIds.at(type_name));
        var.type_info = type_infos.at(type_name);
        var.size = variable_size_bytes(var.type_info.type);
    } else {
        ERROR((*tkn)->loc, "type was not found");
    }

    while (m_currentLexar->peek()->type == TokenType::Mul) {
        m_currentLexar->getAndExpectNext(TokenType::Mul);
        var.size = 8;
        var.kind.pointer_count++;
    }
    m_currentLexar->getAndExpectNext(TokenType::ID);
    if (strct) {
        std::string struct_name = (*tkn)->string_value;
        Func *orig_func;
        if (var.kind.pointer_count != 0) {
            orig_func = m_currentFunc;
            m_currentFunc = &default_func;
        }
        Variable new_var{};
        new_var = initStruct(type_name, struct_name, member);
        new_var.kind = var.kind;
        if (var.kind.pointer_count != 0) {
            current_offset -= new_var.size - 8;
            new_var.size = 8;
        }
        if (var.kind.pointer_count != 0) {
            m_currentFunc = orig_func;
        }
        for (auto& v : new_var.members) {
            v.parent->kind = new_var.kind;
            v.parent->size = new_var.size;
        }
        var = new_var;
    } else {
        current_offset += var.size;
    }


    if (variable_exist_in_storage(m_currentLexar->currentToken->string_value, var_store))
        ERROR(m_currentLexar->currentToken->loc, f("redifinition of {}", m_currentLexar->currentToken->string_value));
    else 
        var.name = m_currentLexar->currentToken->string_value;    

    current_offset = align(current_offset, var.type_info.type, type_name);
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
    m_currentFunc = &default_func;

    while ((*tkn)->type != TokenType::EndOfFile) {
        switch((*tkn)->type) {
            case TokenType::Func:{
                parseFunction();
                m_currentLexar->getNext();
                m_currentFunc = &default_func;
                
            }break;//TokenType::Func
            case TokenType::Module: {
                parseModuleDeclaration();
                m_currentLexar->getNext();
            }break;//TokenType::Module
            case TokenType::Const: {
                auto var = parseConstant();
                var.kind.global = true;
                m_program.var_storage.push_back(var);
                m_currentLexar->getNext();
            }break;//TokenType::Hash
            case TokenType::Hash: {
                parseHash();
                m_currentLexar->getNext();
            }break;//TokenType::Hash
            case TokenType::Struct: {
                parseStructDeclaration();
                m_currentLexar->getNext();
            }break;//TokenType::Struct
            default: {
                std::println(stderr, "unimplemented type of {} at {}:{}:{}",
                             printableToken.at((*tkn)->type), (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset);
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
Variable Parser::initStruct(std::string type_name, std::string struct_name, bool member, bool save_defaults) {
    //default
    int strct_offset = 0;
    Struct* strct = nullptr;

    strct = &get_struct_from_name(type_name); 

    if (strct == nullptr) TODO("trying to access a struct that doesn't exist");
    // maybe shared pointers??
    Variable* struct_var = new Variable;
    if (!member) {
        current_offset = align(current_offset, Type::Struct_t, type_name);
        current_offset += strct->size;
        *struct_var = {.type_info = type_infos.at(type_name), .name = struct_name, .offset = current_offset, .size = strct->size};
    } else {
        current_offset = align(current_offset, Type::Struct_t, type_name);
        *struct_var = {.type_info = type_infos.at(type_name), .name = struct_name, .offset = current_offset, .size = strct->size};
        current_offset += strct->size;
    }


    for (size_t i = 0; i < strct->var_storage.size(); i++) {
        auto var = strct->var_storage[i];
        var.parent = struct_var;

        // I think this should be removed
        if (var.type_info.type == Type::Struct_t) {
            size_t temp_offset = current_offset;
            Variable strct_;
            if (var.kind.pointer_count > 0) {
                strct_ = initStruct(var.type_info.name, var.name, true, false);
            } else {
                strct_ = initStruct(var.type_info.name, var.name, true, !strct->defaults.contains(i));
            }
            current_offset = temp_offset;


            strct_.parent = struct_var;
            strct_.kind = var.kind;
            strct_.offset = var.offset;
            strct_.size = var.size;
            if (strct_.members[0].parent)
                *strct_.members[0].parent = strct_;
            struct_var->members.push_back(strct_);
            var = strct_;
        } else
            struct_var->members.push_back(var);

        if (strct->defaults.contains(i) && save_defaults) {
            auto def = strct->defaults.at(i);
            if (var.type_info.type == Type::String_t) {
                m_currentFunc->body.push_back({Op::INIT_STRING, {var}});
            }
            if (def.type_info.type != Type::Void_t && var.type_info.type != Type::Struct_t) {
                m_currentFunc->body.push_back({Op::STORE_VAR, {def, var}});
            }
            if (def.type_info.type == Type::Struct_t) {
                for (size_t j = 0; j < def.members.size() && j < var.members.size(); j++) {
                    def.members[j].kind.literal = true;
                    if (def.members[j].type_info.type == Type::Float_t) {
                        def.members[j].type_info = type_infos.at("double");
                        def.members[j].size = 8;
                    }
                    if (def.members[j].type_info.type == Type::String_t) {
                        m_currentFunc->body.push_back({Op::INIT_STRING, {var.members[j]}});
                    }
                    if (def.members[j].type_info.type != Type::Void_t && def.members[j].type_info.type != Type::Struct_t) {
                        m_currentFunc->body.push_back({Op::STORE_VAR, {def.members[j], var.members[j]}});
                    }
                }
            }
        }
    }
    if (member)
        current_offset += strct->size;

    return *struct_var;
}
static size_t current_struct_id = 1;
void Parser::parseStructDeclaration() {
    m_currentLexar->getAndExpectNext(TokenType::ID);
    tkn = &m_currentLexar->currentToken;

    current_module_prefix = "";
    if (m_currentLexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_currentLexar->getAndExpectNext(TokenType::ID);
    }
    current_offset = 0;
    std::string struct_name = current_module_prefix + m_currentLexar->currentToken->string_value;
    current_module_prefix = "";

    size_t struct_index = m_program.struct_storage.size();
    m_program.struct_storage.push_back({});
    Struct& current_struct = m_program.struct_storage[struct_index];
    current_struct.id = current_struct_id++;
    current_struct.is_float_only = true;

    TypeIds.emplace(struct_name, Type::Struct_t);
    size_t struct_info_index = type_infos.size();
    type_infos.emplace(struct_name, TypeInfo{.id = current_typeid_max++, .type = Type::Struct_t, .size = 0, .name = struct_name});

    size_t type_info_func_index = m_program.func_storage.size();
    m_program.func_storage.push_back({.return_type = type_infos.at("TypeInfo"), .name = struct_name + "___" +"type_info", .is_static = true});
    
    m_currentLexar->getAndExpectNext(TokenType::OCurly);

    current_struct.name = struct_name;
    while ((*tkn)->type != TokenType::CCurly) {
        if ((*tkn)->type == TokenType::Func) {
            size_t temp_offset = current_offset;
            size_t func_index = m_program.func_storage.size();
            parseFunction(true, current_struct);

            m_currentLexar->getNext();
            current_offset = temp_offset;
            continue;
        }
        Func* last_func = m_currentFunc;
        m_currentFunc = &default_func;
        auto& var = parseVariable(current_struct.var_storage, true);
        int var_index = current_struct.var_storage.size()-1;
        current_struct.size = align(current_struct.size, var.type_info.type, var.type_info.name);
        if (var.type_info.type == Type::Struct_t) {
            current_struct.alignment = std::max((int)current_struct.alignment, (int)get_struct_from_name(var.type_info.name).alignment);
        } else {
            current_struct.alignment = std::max((int)current_struct.alignment, (int)var.size);
        }
        var.offset = current_struct.size;
        current_struct.size += var.size;
        m_currentFunc = last_func;
        if (var.type_info.type != Type::Double_t && var.type_info.type != Type::Float_t)
            current_struct.is_float_only = false;

        // Defaults
        if (m_currentLexar->peek()->type == TokenType::Eq) {
            m_currentLexar->getAndExpectNext(TokenType::Eq);
            m_currentLexar->getNext();
            auto var2 = std::get<0>(parseExpression());
            current_struct.defaults.emplace(var_index, var2);
        } else if (var.type_info.type != Type::String_t && var.type_info.type != Type::Struct_t) {
            Variable default_val;
            default_val.name = "def_value";
            default_val.kind.literal  = true;
            default_val.kind.constant = true;
            if (var.type_info.type == Type::Double_t || var.type_info.type == Type::Float_t) {
                default_val.type_info = type_infos.at("double");
                default_val.size = 8;
                default_val.value = std::any_cast<double>(variable_default_value(default_val.type_info.type));
                if(!variable_exist_in_storage(default_val.name, m_program.var_storage))
                    m_program.var_storage.push_back(default_val);
            } else {
                default_val.type_info = type_infos.at("int8");
                default_val.value = std::any_cast<int64_t>(variable_default_value(default_val.type_info.type));
                default_val.size = 1;
            }

            current_struct.defaults.emplace(var_index, default_val);
        } else {
            current_struct.defaults.emplace(var_index, Variable{ type_infos.at("void") });
        }
        m_currentLexar->getAndExpectNext(TokenType::SemiColon);
        m_currentLexar->getNext();
    }
    type_infos.at(struct_name).size = current_struct.size;
    m_program.func_storage[type_info_func_index] = make_type_info_func(current_struct);
    current_offset = 0;
}
Variable Parser::parseConstant() {
    Variable var{};
    // TODO:m_currentLexar->getAndExpectNext({TokenType::ID, TokenType::TypeID});
    m_currentLexar->getAndExpectNext(TokenType::ID);   
    var.name = (*tkn)->string_value;
    m_currentLexar->getAndExpectNext(TokenType::Eq);   
    m_currentLexar->getNext();   
    auto [rhs, lvalue] = parseExpression();
    var.type_info = rhs.type_info;
    var.size    = rhs.size;
    if (var.type_info.type != Type::Struct_t)
        var.value   = rhs.value;
    var.members = rhs.members;
    var.parent  = rhs.parent;
    var.kind    = rhs.kind;
    var.kind.constant = true;

    m_currentLexar->getAndExpectNext(TokenType::SemiColon);   
    return var;
}
void Parser::parseModuleDeclaration() {
    m_currentLexar->getAndExpectNext(TokenType::ID);

    ModuleStorage* current_mod = &m_program.module_storage;

    if (m_program.module_storage.contains((*tkn)->string_value)) {
        if (m_currentLexar->peek()->type == TokenType::ColonColon) {
            current_mod = &current_mod->at((*tkn)->string_value).module_storage;
            m_currentLexar->getAndExpectNext(TokenType::ColonColon);
            m_currentLexar->getAndExpectNext(TokenType::ID);
        } else {
            TODO("redeclare module");
        }
    }

    current_mod->emplace((*tkn)->string_value, Module{});
    m_currentLexar->getAndExpectNext(TokenType::SemiColon);
}
void Parser::parseHash() {
    m_currentLexar->getAndExpectNext({TokenType::Import, TokenType::Include, TokenType::Extern});
    switch ((*tkn)->type) {
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
            if (file_name == (*tkn)->loc.inputPath)
                TODO("cannot include the current file");
            if (fileExistsInPaths(file_name, ctx.includePaths)) {
                std::string file_path = getFilePathFromPaths(file_name, ctx.includePaths);

                Lexar l(readFileToString(file_path), file_path);

                m_currentLexar->pushtokensaftercurrent(&l);
                included_files.push_back(file_name);
            } else {
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
void Parser::parseExtern() {
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
        if (m_currentLexar->peek()->type == TokenType::Dot) {
            m_currentLexar->getAndExpectNext(TokenType::Dot);
            m_currentLexar->getAndExpectNext(TokenType::Dot);
            m_currentLexar->getAndExpectNext(TokenType::Dot);
            func.c_variadic = true;
            
            break;
        } else {
            func.local_variables.push_back(parseVariable(func.arguments));
            func.arguments_count++;
        }
        if (m_currentLexar->peek()->type != TokenType::CParen) {
            m_currentLexar->getAndExpectNext(TokenType::Comma);
            if (m_currentLexar->peek()->type == TokenType::CParen) {
                ERROR((*tkn)->loc, "cannot do trailing commas in function calls");
            }
        }
    }

    m_currentLexar->getAndExpectNext(TokenType::CParen);

    // Returns
    if (m_currentLexar->peek()->type == TokenType::Arrow) {
        m_currentLexar->getAndExpectNext(TokenType::Arrow);
        m_currentLexar->getAndExpectNext({TokenType::TypeID, TokenType::ID});
        current_module_prefix = "";
        if (m_currentLexar->peek()->type == TokenType::ColonColon) {
            parseModulePrefix(); 
            m_currentLexar->getNext();
        }
        func.return_type = type_infos.at(current_module_prefix + (*tkn)->string_value);
        current_module_prefix = "";
    }
    if (func.return_type.size > 16) {
        Variable ret = {
            .type_info = TypeInfo(func.return_type),
            .name   = "return_register",
            .offset = 8,
            .size   = 8,
            .kind   = {
                .pointer_count = 1
            }
        };
        func.arguments.emplace(func.arguments.begin(), ret);
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
            
            if (m_currentLexar->peek()->type != TokenType::CBracket) {
                m_currentLexar->getAndExpectNext(TokenType::Comma);
            } else {
                m_currentLexar->getAndExpectNext(TokenType::CBracket);
            }
        } while ((*tkn)->type != TokenType::CBracket);
    }

    m_currentLexar->getAndExpectNext(TokenType::SemiColon);

    m_program.func_storage.push_back(func);
    m_currentFunc = &default_func;

}
void Parser::parseModulePrefix() {
    auto current_module_storage = &m_program.module_storage;

    //if (!current_module_storage->contains((*tkn)->string_value)) TODO("error");
    if (!current_module_storage->contains((*tkn)->string_value)) { 
        std::println("{}:{}:{} tkn = {}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset, (*tkn)->string_value);
        TODO("error");
    }

    m_currentFuncStorage   = &current_module_storage->at((*tkn)->string_value).func_storage;
    current_module_storage = &m_program.module_storage.at((*tkn)->string_value).module_storage;
    current_module_prefix += (*tkn)->string_value + "__";

    m_currentLexar->getAndExpectNext(TokenType::ColonColon);

    while ((m_currentLexar->peek() + 1)->type == TokenType::ColonColon) {
        m_currentLexar->getAndExpectNext(TokenType::ID);
        if (!current_module_storage->contains((*tkn)->string_value)) { 
            std::println("{}:{}:{} tkn = {}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset, (*tkn)->string_value);
            TODO("error");
        }

        m_currentFuncStorage   = &current_module_storage->at((*tkn)->string_value).func_storage;
        current_module_storage = &current_module_storage->at((*tkn)->string_value).module_storage;
        current_module_prefix += (*tkn)->string_value + "__";

        m_currentLexar->getAndExpectNext(TokenType::ColonColon);
    }
}
Func Parser::parseFunction(bool member, Struct parent) {
    tkn = &m_currentLexar->currentToken;
    current_offset = 0;
    Func func = {};
    std::string name{};
    m_currentFunc = &func;
    if (m_currentLexar->peek()->type == TokenType::Static) {
        m_currentLexar->getAndExpectNext(TokenType::Static);
        func.is_static = true;
    }
    m_currentLexar->getAndExpectNext({TokenType::ID, TokenType::TypeID});
    current_module_prefix = "";
    if (m_currentLexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_currentLexar->getAndExpectNext(TokenType::ID);
    }
    if (member) {
        name = parent.name + "___" + current_module_prefix + (*tkn)->string_value;
    } else if (!member && m_currentLexar->peek()->type == TokenType::Dot) {
        std::string type_name = current_module_prefix + (*tkn)->string_value;
        if (type_infos.contains(type_name)) {
            auto type = type_infos.at(type_name);
            if (type.type == Type::Struct_t)
                parent = get_struct_from_name(type_name);
            else 
                parent.name = type_name;
        }
        member = true;
        m_currentLexar->getAndExpectNext(TokenType::Dot);
        m_currentLexar->getAndExpectNext(TokenType::ID);
        name = type_name + "___" + current_module_prefix + (*tkn)->string_value;
    } else {
        name = current_module_prefix + (*tkn)->string_value;
    }
    if (func.is_static && !member) 
        TODO("cannot have static non member functions");
    func.name = name;
    current_module_prefix = "";

    if (member && !func.is_static) {
        Variable this_pointer = {
            .type_info  = type_infos.at(parent.name),
            .name       = "this", .offset = 8,
            .size       = 8,
            .members    = parent.var_storage,
            .kind       = {
                .pointer_count=1
            }
        };
        func.local_variables.push_back(this_pointer);
        func.arguments.push_back(this_pointer);
        func.arguments_count++;
        current_offset += 8;
    }
    m_currentLexar->getAndExpectNext(TokenType::OParen);
    VariableStorage args_temp_storage{};
    m_currentFunc = &default_func;
    while (m_currentLexar->peek()->type != TokenType::CParen && (*tkn)->type != TokenType::EndOfFile && m_currentLexar->peek()->type != TokenType::EndOfFile) {
        parseVariable(args_temp_storage);
        if (m_currentLexar->peek()->type != TokenType::CParen) {
            m_currentLexar->expectNext(TokenType::Comma);
            m_currentLexar->getNext();
        }
    }
    m_currentFunc = &func;
    m_currentLexar->getAndExpectNext(TokenType::CParen);
    m_currentLexar->expectNext({TokenType::Arrow, TokenType::OCurly});

    // Returns
    if (m_currentLexar->peek()->type == TokenType::Arrow) {
        m_currentLexar->getAndExpectNext(TokenType::Arrow);
        m_currentLexar->getAndExpectNext({TokenType::TypeID, TokenType::ID});
        current_module_prefix = "";
        if (m_currentLexar->peek()->type == TokenType::ColonColon) {
            parseModulePrefix();
            m_currentLexar->getNext();
        }
        if (type_infos.contains(current_module_prefix + (*tkn)->string_value))
            func.return_type = type_infos.at(current_module_prefix + (*tkn)->string_value);
        else 
            TODO("Type does not exist");
        if (m_currentLexar->peek()->type == TokenType::OBracket) {
            m_currentLexar->getNext();
            m_currentLexar->getAndExpectNext(TokenType::CBracket);
            TODO("add arrays return");
        }
        while (m_currentLexar->peek()->type == TokenType::Mul) {
            m_currentLexar->getNext();
            func.return_kind.pointer_count += 1;
        }
        current_module_prefix = "";
    } else {
        func.return_type = type_infos.at("void");
    }
                
    if (func.return_type.size > 16 && func.return_kind.pointer_count == 0) {
        current_offset += 8;
        Variable ret = {
            .type_info = TypeInfo(func.return_type),
            .name = "return_register",
            .offset = current_offset,
            .size = 8,
            .kind = {
                .pointer_count = 1
            }
        };
        func.arguments_count++;
        func.arguments.emplace(func.arguments.begin(), ret);
    }
    for (int i = 0; i < args_temp_storage.size(); i++) {
        auto var = args_temp_storage[i];
        if (var.type_info.type == Type::Struct_t) {
            if (var.kind.pointer_count > 0) {
                func.arguments.push_back(var);
                func.local_variables.push_back(var);
                func.arguments_count++;
                continue;
            }
            if (var.size <= 8) {
                //i -= get_struct_from_name(var._type_name).var_storage.size();
                func.arguments.push_back(var);
                func.local_variables.push_back(var);
                func.arguments_count++;
            } else if (var.size <= 16) {
                i -= get_struct_from_name(var.type_info.name).var_storage.size();
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
                //i -= get_struct_from_name(var._type_name).var_storage.size();
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
Variable make_temp_var(TypeInfo type, Kind kind = {}) {
    Variable var;
    var.type_info = type;
    var.kind = kind;
    if (kind.pointer_count > 0)
        var.size = 8;
    else 
        var.size = type.size;

    current_offset = Parser::align(current_offset, type.type, type.name);
    temp_offset = Parser::align(temp_offset, type.type, type.name);
    var.offset = current_offset + temp_offset + var.size;
    if (max_locals_offset < current_offset + temp_offset) {
        max_locals_offset = current_offset + temp_offset;
    }
    temp_offset += var.size;
    var.name = "temporary variable";
    //var.deref_count = old.deref_count;
    //var.value = old.value;
    return var;
}
Variable make_temp_var(Type type, size_t size, Variable old = {}) {
    Variable var;
    var.type_info = type_infos.at(printableTypeIds.at(type));
    var.size = size;
    current_offset = Parser::align(current_offset, type);
    temp_offset = Parser::align(temp_offset, type);
    var.offset = current_offset + temp_offset + size;
    if (max_locals_offset < current_offset + temp_offset) {
        max_locals_offset = current_offset + temp_offset;
    }
    temp_offset += size;
    var.name = "temporary variable";
    //var.deref_count = old.deref_count;
    //var.value = old.value;
    return var;
}
void delete_temp_vars() {
    temp_offset = 0;
}

void Parser::parseStatement() {
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
                if (m_currentFunc->return_type.name != "void") TODO("error on no return");
                return_value = {
                    .type_info = type_infos.at("int8"),
                    .name = "IntLit",
                    .value = (int64_t)0,
                    .size = 1,
                    .kind = {
                        .constant = true,
                        .literal = true,
                    }
                };
                m_currentFunc->body.push_back({Op::RETURN, {return_value}});
                break;
            }
            return_value = std::get<0>(parseExpression());
            m_currentFunc->body.push_back({Op::RETURN, {return_value}});
            m_currentLexar->getAndExpectNext(TokenType::SemiColon);

        }break;//TokenType::Return
        case TokenType::If: {
            m_currentLexar->getAndExpectNext(TokenType::OParen);
            m_currentLexar->getNext();
            auto expr = std::get<0>(parseExpression());
            m_currentLexar->getAndExpectNext(TokenType::CParen);

            delete_temp_vars();

            m_currentLexar->getNext();
            size_t jmp_if_not = m_currentFunc->body.size();
            m_currentFunc->body.push_back({Op::JUMP_IF_NOT, {"", expr}});
            parseStatement();
            if (m_currentLexar->peek()->type == TokenType::Else) {
                size_t jmp_else = m_currentFunc->body.size();
                m_currentFunc->body.push_back({Op::JUMP, {""}});
                std::string label = std::format("{:06x}", statement_count++);
                m_currentFunc->body.push_back({Op::LABEL, {label}});
                m_currentFunc->body[jmp_if_not].args[0] = label;
                m_currentLexar->getAndExpectNext(TokenType::Else);
                m_currentLexar->getNext();
                parseStatement();
                label = std::format("{:06x}", statement_count++);
                m_currentFunc->body.push_back({Op::LABEL, {label}});
                m_currentFunc->body[jmp_else].args[0] = label;
            } else {
                std::string label = std::format("{:06x}", statement_count++);
                m_currentFunc->body.push_back({Op::LABEL, {label}});
                m_currentFunc->body[jmp_if_not].args[0] = label;
            }
        }break;//TokenType::If
        case TokenType::While: {
            m_currentLexar->getAndExpectNext(TokenType::OParen);
            m_currentLexar->getNext();
            std::string pre_label = std::format("{:06x}", statement_count++);
            m_currentFunc->body.push_back({Op::LABEL, {pre_label}});
            auto expr = std::get<0>(parseExpression());
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
            std::string type_name = (*tkn)->string_value;
            size_t current_point = m_currentLexar->currentTokenIndex;
            if (m_currentLexar->peek()->type == TokenType::ColonColon) {
                current_module_prefix = "";
                parseModulePrefix();
                m_currentLexar->getAndExpectNext(TokenType::ID);
                type_name = current_module_prefix + m_currentLexar->currentToken->string_value;
                current_module_prefix = "";
            }
            auto peek = m_currentLexar->peek();
            m_currentLexar->currentTokenIndex = current_point - 1;
            m_currentLexar->getNext();
            if (!TypeIds.contains(type_name)) {
                goto defau;
            } else if (peek->type == TokenType::Dot) {
                goto defau;
            }
            auto saved_body = m_currentFunc->body;
            auto var = parseVariable(m_currentFunc->local_variables);

            // TODO: factor out to a function
            if (var.type_info.type == Type::String_t) {
                m_currentFunc->local_variables.push_back(var);
                m_currentFunc->body.push_back({Op::INIT_STRING, {var}});
            }
            if (m_currentLexar->peek()->type == TokenType::Eq) {
                m_currentLexar->getAndExpectNext(TokenType::Eq);
                m_currentLexar->getNext();
                m_currentFunc->body = saved_body;
                auto var2 = std::get<0>(parseExpression());
                m_currentFunc->body.push_back({Op::STORE_VAR, {var2, var}});
            } else if (var.type_info.type != Type::String_t && var.type_info.type != Type::Struct_t) {
                Variable default_val;
                default_val.kind.literal = true;
                default_val.kind.constant = true;
                default_val.name = "def_value";
                if (var.type_info.type == Type::Double_t || var.type_info.type == Type::Float_t) {
                    default_val.type_info = type_infos.at("double");
                    default_val.size = 8;
                    default_val.value = std::any_cast<double>(variable_default_value(var.type_info.type));
                    if(!variable_exist_in_storage(default_val.name, m_program.var_storage))
                        m_program.var_storage.push_back(default_val);
                } else {
                    default_val.type_info = type_infos.at("int8");
                    default_val.size = 1;
                    default_val.value = std::any_cast<int64_t>(variable_default_value(var.type_info.type));
                }

                m_currentFunc->body.push_back({Op::STORE_VAR, {default_val, var}});
            } else if (var.kind.pointer_count > 0) {
                Variable default_val;
                default_val.type_info = type_infos.at("int64");
                default_val.kind.literal = true;
                default_val.kind.constant = true;
                default_val.size = 1;

                default_val.name = "def_value";
                default_val.value = std::any_cast<int64_t>(variable_default_value(default_val.type_info.type));
                m_currentFunc->body.push_back({Op::STORE_VAR, {default_val, var}});
            }

            m_currentLexar->getAndExpectNext(TokenType::SemiColon);
        }break;//TokenType::TypeID
        default: {
        defau:
            auto data = parseDotExpression();
            auto &[lhs, lvalue] = data;
            auto peek_type = m_currentLexar->peek()->type;

            if (lvalue) {
                if (peek_type == TokenType::Eq || peek_type == TokenType::PlusEq || peek_type == TokenType::MinusEq || peek_type == TokenType::MulEq) {
                    m_currentLexar->getNext();
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

                }
            }

            m_currentLexar->getAndExpectNext(TokenType::SemiColon);
        
            //ERROR(m_currentLexar->currentToken->loc, "unsupported token");
            //exit(1);
        }
    }
    delete_temp_vars();
}

void Parser::parseBlock() {
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
void print_var(Variable var) {
    std::println("name: {}", var.name);
    std::println("offset: {}, size: {}", var.offset, var.size);
    std::println("type: {}, type_name: {}", (int)var.type_info.type, var.type_info.name);
    std::println("deref_count: {}", var.deref_count);
    std::println("kind: {{ ");
    std::println("  pointer_count: {}", var.kind.pointer_count);
    std::println("  deref_offset:  {}", var.kind.deref_offset);
    std::println("}} ");
}
ExprResult Parser::parsePrimaryExpression(Variable this_ptr, Variable this_, std::string func_prefix) {
    // will need it later
    ExprResult data;
    tkn = &m_currentLexar->currentToken;
    Variable var;
    bool ret_lvalue = false;


    if ((*tkn)->type == TokenType::DQoute) {
        m_currentLexar->getAndExpectNext(TokenType::StringLit);
        var = {
            .type_info = type_infos.at("string"),
            .name = f("literal_{}", literal_count++),
            .value = (*tkn)->string_value,
            .size = 8,
            .kind = {
                .constant = true,
                .literal = true,
            },
        };
        m_program.var_storage.push_back(var);
        m_currentLexar->getAndExpectNext(TokenType::DQoute);
        return {var, ret_lvalue};
    }

    if ((*tkn)->type == TokenType::IntLit) {
        int64_t value = (*tkn)->int_value;
        size_t size = size_of_signed_int(value);
        var = {
            .type_info = sized_int_type(size),
            .name  = "Int_Lit",
            .value = value,
            .size  = size,
            .kind  = {
                .constant = true,
                .literal = true,
            },
        };
        return {var, ret_lvalue};
    }
    if ((*tkn)->type == TokenType::DoubleLit) {
        var = {
            .type_info = type_infos.at("double"),
            .name  = f("literal_{}", literal_count++),
            .value = (*tkn)->double_value,
            .size  = 8,
            .kind  = {
                .constant = true,
                .literal = true,
            },
        };
        m_program.var_storage.push_back(var);
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
        Variable amount = {
            .type_info = type_infos.at("int8"),
            .name = "Int_Lit",
            .value = (int64_t)1,
            .size = 1,
            .kind = {
                .constant = true,
                .literal = true,
            },
        };
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
        Variable amount = {
            .type_info = type_infos.at("int8"),
            .name = "Int_Lit",
            .value = (int64_t)1,
            .size = 1,
            .kind = {
                .constant = true,
                .literal = true,
            },
        };
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

    std::string name      = current_module_prefix + m_currentLexar->currentToken->string_value;
    std::string func_name = current_module_prefix + func_prefix + m_currentLexar->currentToken->string_value;
    if ((*tkn)->type == TokenType::ID || (*tkn)->type == TokenType::TypeID) {
        if (m_currentLexar->peek()->type == TokenType::OParen) {
            if (!function_exist_in_storage(func_name, m_program.func_storage)) 
                TODO(f("func {} doesn't exist", func_name));
            auto& func = get_func_from_name(func_name, m_program.func_storage);
            var = make_temp_var(func.return_type, func.return_kind);
            if (var.kind.pointer_count > 0)
                var.size = 8;
            if (var.type_info.type == Type::Struct_t) {
                var.members = get_struct_from_name(var.type_info.name).var_storage;
            }
            if(!func.is_static && this_.type_info.type == Type::Typeid_t)
                TODO("cannot use Typeid to call a non static function");
            if (!func.is_static)
                parseFuncCall(func, this_ptr, var);
            else 
                parseFuncCall(func, {type_infos.at("void")}, var);
            var.parent = nullptr;
            return {var, ret_lvalue};
        }
        if (type_infos.contains(name)) {
            TypeInfo type = type_infos.at(name);
            //Struct_literal
            if (type.type == Type::Struct_t) {
                VariableStorage v{};
                if (m_currentLexar->peek()->type == TokenType::OCurly) {
                    m_currentLexar->getNext();
                    while (m_currentLexar->peek()->type != TokenType::CCurly) {
                        m_currentLexar->getNext();
                        v.push_back(std::get<0>(parseExpression()));
                        if (m_currentLexar->peek()->type == TokenType::Comma)
                            m_currentLexar->getAndExpectNext(TokenType::Comma);
                    }
                    m_currentLexar->getAndExpectNext(TokenType::CCurly);
                    // saving offset to make struct literal temporary
                    auto save_off  = current_offset;
                    auto save_func = m_currentFunc;
                    auto strct = get_struct_from_name(name);
                    m_currentFunc = &default_func;
                    var = initStruct(name, "struct literal");
                    m_currentFunc  = save_func;
                    for (size_t i = 0; i < var.members.size(); i++) {
                        var.members[i].name = v[i].name;
                        if (i < v.size()) {
                            var.members[i].value = v[i].value;
                        } else if (strct.defaults.contains(i)) {
                            v.push_back(strct.defaults.at(i));
                            var.members[i].value = v[i].value;
                        } else {
                            v.push_back({.type_info = type_infos.at("void")});
                        }
                        m_currentFunc->body.push_back({Op::STORE_VAR, {v[i], var.members[i]}});
                    }
                    current_offset = save_off;
                    return {var, false};
                }
            }
            var.name = type.name;
            var.size = 8;
            var.type_info = type_infos.at("typeid");
            var.kind.constant = true,
            var.kind.literal = true;
            var.value = (int64_t)type.id;
            return {var, false};
        }
        if (this_ptr.type_info.type != Type::Void_t) {
            auto var_ = get_var_from_name(name, this_.members);
            var_.parent = new Variable;
            *var_.parent = this_;
            return {var_, true};
        } else if (variable_exist_in_storage((*tkn)->string_value, m_program.var_storage)) {
            var = get_var_from_name((*tkn)->string_value, m_program.var_storage);
            return {var, true};
        } else if (variable_exist_in_storage(name, m_currentFunc->local_variables)) {
            var = get_var_from_name(name, m_currentFunc->local_variables);
            ret_lvalue = true;
            return {var, ret_lvalue};
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
ExprResult Parser::parseDotExpression(Variable this_ptr, Variable this_, std::string func_prefix) {
    tkn = &m_currentLexar->currentToken;
    auto lhs = parsePrimaryExpression(this_ptr, this_, func_prefix);

    if (m_currentLexar->peek()->type == TokenType::Dot) {
        m_currentLexar->getAndExpectNext(TokenType::Dot);
        m_currentLexar->getNext();

        Variable lhs_var = std::get<0>(lhs);
        if (lhs_var.parent == nullptr) {
            this_ptr = lhs_var;
            this_    = lhs_var;
            this_ptr.deref_count = this_ptr.kind.pointer_count - 1;
            this_ptr.kind.pointer_count = 1;
            this_ptr.size = 8;
        } else {
            lhs_var.parent = new Variable;
            *lhs_var.parent = this_;
            this_    = lhs_var;
            this_ptr = lhs_var;
            this_ptr.deref_count = this_ptr.kind.pointer_count - 1;
            this_ptr.kind.pointer_count = 1;
            this_ptr.size = 8;
        }
        if (lhs_var.type_info.type == Type::Typeid_t)
            func_prefix = lhs_var.name;
        else
            func_prefix = lhs_var.type_info.name;
        func_prefix += "___";

        auto rhs = parseDotExpression(this_ptr, this_, func_prefix);
        return rhs;
    }
    return lhs;
}
ExprResult Parser::parseUnaryExpression() {
    tkn = &m_currentLexar->currentToken;

    if ((*tkn)->type == TokenType::Minus) {
        m_currentLexar->getNext();
        auto rhs = std::get<0>(parseUnaryExpression());
        Variable result = make_temp_var(rhs.type_info.type, 8);
        if (rhs.kind.literal) {
            if (rhs.type_info.type == Type::Double_t || rhs.type_info.type == Type::Float_t)
                rhs.value = -std::any_cast<double>(rhs.value);
            else
                rhs.value = -std::any_cast<int64_t>(rhs.value);
            m_currentFunc->body.push_back({Op::STORE_VAR, {rhs, result}});
        } else {
            Variable zero   = {
                .type_info = type_infos.at("int8"),
                .name = "Int_Lit",
                .value = (int64_t)0,
                .size = 1,
                .kind = {
                    .constant = true,
                    .literal = true,
                },
            };
            m_currentFunc->body.push_back({Op::SUB, {zero, rhs, result}});
        }
        return {result, false};
    }
    if ((*tkn)->type == TokenType::Not) {
        m_currentLexar->getNext();
        auto rhs = std::get<0>(parseUnaryExpression());
        auto type = Type::Bool_t;
        Variable result = make_temp_var(type, variable_size_bytes(type));
        Variable zero   = {
            .type_info = type_infos.at("int8"),
            .name = "Int_Lit",
            .value = (int64_t)0,
            .size = 1,
            .kind = {
                .constant = true,
                .literal = true,
            },
        };
        m_currentFunc->body.push_back({Op::EQ, {rhs, zero, result}});
        return {result, false};
    }

    return parseDotExpression();
}
ExprResult Parser::parseMultiplicativeExpression() {
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

        Variable result = make_temp_var(lhs.type_info.type, variable_size_bytes(lhs.type_info.type));

        if (op_type == TokenType::Mul) {
            m_currentFunc->body.push_back({Op::MUL, {lhs, rhs, result}});
        } else if (op_type == TokenType::Div) {
            m_currentFunc->body.push_back({Op::DIV, {lhs, rhs, result}});
        } else if (op_type == TokenType::Mod) {
            m_currentFunc->body.push_back({Op::MOD, {lhs, rhs, result}});
        }

        lhs = result;
    }

    return {lhs, false};
}
ExprResult Parser::parseAdditiveExpression() {
    tkn = &m_currentLexar->currentToken;
    auto lhs = std::get<0>(parseMultiplicativeExpression());

    while ((*tkn)->type != TokenType::EndOfFile) {
        TokenType peek = m_currentLexar->peek()->type;
        if (peek != TokenType::Plus && peek != TokenType::Minus) break;

        m_currentLexar->getNext();
        TokenType op_type = (*tkn)->type;
        m_currentLexar->getNext();
        auto rhs = std::get<0>(parseMultiplicativeExpression());


        Variable result = make_temp_var(lhs.type_info.type, variable_size_bytes(lhs.type_info.type));
        if (op_type == TokenType::Plus) {
            m_currentFunc->body.push_back({Op::ADD, {lhs, rhs, result}});
        } else {
            m_currentFunc->body.push_back({Op::SUB, {lhs, rhs, result}});
        }

        lhs = result;
    }

    return {lhs, false};
}
ExprResult Parser::parseCondition(int min_prec) {
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
ExprResult Parser::parseExpression() {
    return parseCondition(0);
}

void Parser::parseFuncCall(Func func, Variable this_ptr, Variable return_address) {
    std::string loc = std::format("{}:{}:{}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset);
    VariableStorage args{};
    m_currentLexar->getAndExpectNext(TokenType::OParen);
    if (func.return_type.size > 16) {
        return_address.deref_count -= 1;
        args.push_back(return_address);
        return_address.deref_count += 1;
    }
    if (this_ptr.type_info.type != Type::Void_t) args.push_back(this_ptr);
    while (m_currentLexar->peek()->type != TokenType::CParen) {
        m_currentLexar->getNext();
        args.push_back(std::get<0>(parseExpression()));
        if (m_currentLexar->peek()->type != TokenType::CParen) {
            m_currentLexar->getAndExpectNext(TokenType::Comma);
            if (m_currentLexar->peek()->type == TokenType::CParen) {
                ERROR((*tkn)->loc, "cannot do trailing commas in function calls");
            }
        }
    }
    m_currentLexar->getAndExpectNext(TokenType::CParen);
    if (!func.variadic && !func.c_variadic) {
        if (func.arguments_count != args.size()) {
            for (auto& arg : args)
                std::println("{} {}", arg.name, arg.type_info.name);
            std::println("----------------------");
            for (auto& arg : func.arguments)
                std::println("{} {}", arg.name, arg.type_info.name);
            TODO(f("\n"
                   "{} incorrect amount of function arguments got {} but expected {}",
                   loc, args.size(), func.arguments_count)
                 );
        }
        // TODO: check every argument
    }
    m_currentFunc->body.push_back({Op::CALL, {func, args, return_address}});

    m_currentFuncStorage = &m_program.func_storage;
    current_module_prefix = "";
}
Func Parser::make_type_info_func(Struct s) {
    auto save = current_offset;
    auto orig = m_currentFunc;
    Kind literal = {
        .constant = true,
        .literal = true,
    };
    Func fn;
    m_currentFunc = &fn;
    fn.name = s.name + "___" + "type_info";
    fn.return_type = type_infos.at("TypeInfo");
    fn.is_static = true;
    Variable typeinfo;
    typeinfo = initStruct("TypeInfo", "struct literal");
    Variable id   = {.type_info = type_infos.at("int64") , .value = (int64_t)type_infos.at(s.name).id      , .kind = literal};
    Variable type = {.type_info = type_infos.at("int32") , .value = (int64_t)type_infos.at(s.name).type        , .kind = literal};
    Variable size = {.type_info = type_infos.at("int64") , .value = (int64_t)type_infos.at(s.name).size    , .kind = literal};
    Variable name = {
        .type_info = type_infos.at("string"),
        .name  = f("literal_{}", literal_count++),
        .value = (std::string)type_infos.at(s.name).name,
        .kind  = literal
    };
    fn.arguments = {Variable{.type_info = type_infos.at("TypeInfo"), .offset = 8, .size = 8, .kind = {.pointer_count = 1}}};
    fn.arguments_count = 1;
    fn.stack_size = 48;
    m_program.var_storage.push_back(name);
    fn.body = {
        {Op::INIT_STRING, {typeinfo.members[3]}},
        {Op::STORE_VAR, {id  , typeinfo.members[0]}},
        {Op::STORE_VAR, {type, typeinfo.members[1]}},
        {Op::STORE_VAR, {size, typeinfo.members[2]}},
        {Op::STORE_VAR, {name, typeinfo.members[3]}},
        {Op::RETURN   , {typeinfo}}
    };
    current_offset = save;
    m_currentFunc = orig;
    return fn;
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
    Variable& get_var_from_name(std::string_view name, VariableStorage& var_storage);
    for (auto& var : var_storage) {
        if (var.name == name) return var;
    }
    std::println("{} was not found", name);
    TODO("var doesn't exist");
    ERROR((*tkn)->loc, "");
}
std::any Parser::variable_default_value(Type t) {
    switch (t) {
        case Type::Size_t:
        case Type::Int8_t:
        case Type::Int16_t:
        case Type::Int32_t:
        case Type::Char_t:
        case Type::Bool_t:
        case Type::Typeid_t:
        case Type::Int64_t: return (int64_t)0; break;
        case Type::Double_t:
        case Type::Float_t: return (double)0.0; break;

        case Type::String_t: return ""   ; break;
        case Type::Void_t:   return (int64_t)0    ; break;

        default: 
            TODO(f("type {} doesn't have default", (int)t));
    }
    return 0;
}
size_t Parser::variable_size_bytes(Type t) {
    switch (t) {
        case Type::Bool_t:   return 1; break;
        case Type::Char_t:   return 1; break;
        case Type::Int8_t:   return 1; break;
        case Type::Int16_t:  return 2; break;
        case Type::Int32_t:  return 4; break;
        case Type::Int64_t:  return 8; break;
        case Type::Typeid_t: return 8; break;
        case Type::Ptr_t:    return 8; break;
        case Type::Size_t:   return 8; break;
        case Type::Float_t:  return 4; break;
        case Type::Double_t: return 8; break;

        case Type::String_t: return 8; break;
        case Type::Void_t:   return 0; break;

        default: 
            TODO(f("type {} doesn't have default size", (int)t));
    }
    return 0;
}
