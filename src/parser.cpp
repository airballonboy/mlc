#include "parser.h"
#include "ast/all.h"
#include "codegen/base.h"
#include "context.h"
#include "lexar.h"
#include "token.h"
#include "tools/file.h"
#include "tools/format.h"
#include "tools/logger.h"
#include "type_system/kind.h"
#include "type_system/type.h"
#include "type_system/type_info.h"
#include "type_system/typeid.h"
#include "type_system/func.h"
#include "type_system/variable.h"
#include "ast/load_ast.h"
#include <algorithm>
#include <any>
#include <cassert>
#include <cstdint>
#include <vector>

#define ERROR(loc, massage) \
    do { \
        mlog::log(mlog::Red,     \
                   "ERROR:\n", \
                 mlog::Cyan,    \
                 mlog::format("  {}:{}:{} {}", (loc).inputPath, (loc).line, (loc).offset, (massage)).c_str()); \
        exit(1); \
    } while (0)

int64_t literal_count = 0;
size_t  temp_count = 0;
int64_t current_offset = 0;
size_t max_locals_offset = 8;
size_t statement_count = 0;
Func default_func = Func();
std::string current_module_prefix{};
std::vector<std::string> included_files;
// Copies var1 value into var2
void copy_val(const Variable& var1, Variable& var2) {
    var2.Int_val = var1.Int_val;
    var2.Double_val = var1.Double_val;
    var2.String_val = var1.String_val;
}
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
    if (type_name == "") type_name = type.info.name;
    if (type.info.kind != Kind::Struct) {
        if (type.info.kind == Kind::Void) return current_offset;
        alignment = type.info.size;
    } else {
        alignment = Struct::get_from_name(type_name, m_program.struct_storage).alignment;
    }
    return (current_offset + alignment - 1) & ~(alignment-1);
}
Parser::Parser(Lexar* lexar) {
    m_lexar = lexar;
}
Variable& Parser::parseVariable(VariableStorage& var_store, bool member) {
    Variable var{};
    std::string type_name{};
    bool strct = false;
    if ((*tkn)->type != TokenType::TypeID && (*tkn)->type != TokenType::ID) {
        m_lexar->getAndExpectNext({TokenType::ID, TokenType::TypeID});
    }
    auto type = std::get<0>(parsePrimaryExpression());
    if (auto type_node = dynamic_cast<Load_Ast*>(type.get())) {
        if (type_node->var.type.info.name == "typeid") {
            type_name = TypeInfo::get_from_id(type_node->var.Int_val).name;
        } else {
            ERROR((*tkn)->loc, "expected typeid in variable declaration");
        }
    } else {
        ERROR((*tkn)->loc, "expected typeid in variable declaration");
    }
    if (TypeIds.contains(type_name) && TypeIds.at(type_name) > TypeId::BasicTypesCount) {
        strct = true;
        var.type = type_infos.at(type_name);
        var.size = var.type.info.size;
    } else if (TypeIds.contains(type_name)) {
        type_name = printableTypeIds.at(TypeIds.at(type_name));
        var.type = type_infos.at(type_name);
        var.size = var.type.info.size;
    } else {
        ERROR((*tkn)->loc, "type was not found");
    }

    while (m_lexar->peek()->type == TokenType::Mul) {
        m_lexar->getAndExpectNext(TokenType::Mul);
        var.size = 8;
        var.type = make_ptr(var.type);
    }
    m_lexar->getAndExpectNext(TokenType::ID);
    if (strct) {
        std::string struct_name = (*tkn)->string_value;
        Func *orig_func;
        if (var.type.info.kind == Kind::Pointer) {
            orig_func = m_func;
            m_func = &default_func;
        }
        Variable new_var{};
        new_var = initStruct(type_name, struct_name, member);
        new_var.type = set_ptr_count(new_var.type, get_ptr_count(var.type));
        if (var.type.info.kind == Kind::Pointer) {
            current_offset -= new_var.size - 8;
            new_var.size = 8;
        }
        if (var.type.info.kind == Kind::Pointer) {
            m_func = orig_func;
        }
        for (auto& v : new_var.members) {
            v.parent->type = set_ptr_count(v.parent->type, get_ptr_count(new_var.type));
            v.parent->size = new_var.size;
        }
        var = new_var;
    } else {
        current_offset += var.size;
    }


    if (Variable::is_in_storage(m_lexar->currentToken->string_value, var_store))
        ERROR(m_lexar->currentToken->loc, mlog::format("redifinition of {}", m_lexar->currentToken->string_value));
    else 
        var.name = m_lexar->currentToken->string_value;    

    current_offset = align(current_offset, var.type, type_name);
    var.offset = current_offset;
    // TODO: support arrays
    if (m_lexar->peek()->type == TokenType::OBracket) {
        m_lexar->getAndExpectNext(TokenType::OBracket);
        if (m_lexar->peek()->type == TokenType::IntLit)
            m_lexar->getAndExpectNext(TokenType::IntLit);
        m_lexar->getAndExpectNext(TokenType::CBracket);
        TODO("support for arrays");   
    }
    var_store.push_back(var);


    return var_store[var_store.size() - 1];
}
Program* Parser::parse() {
    tkn = &m_lexar->currentToken;

    m_currentFuncStorage = &m_program.func_storage;
    m_func = &default_func;

    while ((*tkn)->type != TokenType::EndOfFile) {
        switch((*tkn)->type) {
            case TokenType::Func:{
                parseFunction();
                m_lexar->getNext();
                m_func = &default_func;
                
            }break;//TokenType::Func
            case TokenType::Module: {
                parseModuleDeclaration();
                m_lexar->getNext();
            }break;//TokenType::Module
            case TokenType::Const: {
                auto var = parseConstant();
                var.type.qualifiers |= Qualifier::global;
                m_program.var_storage.push_back(var);
                m_lexar->getNext();
            }break;//TokenType::Hash
            case TokenType::Hash: {
                parseHash();
                m_lexar->getNext();
            }break;//TokenType::Hash
            case TokenType::Struct: {
                parseStructDeclaration();
                m_lexar->getNext();
            }break;//TokenType::Struct
            default: {
                mlog::println(stderr, "unimplemented type of {} at {}:{}:{}",
                             printableToken.at((*tkn)->type), (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset);
                m_lexar->getNext();

            }break;
        }
    }
    return &m_program;

}
Variable Parser::initStruct(std::string type_name, std::string struct_name, bool member, bool save_defaults) {
    //default
    int strct_offset = 0;
    Struct* strct = nullptr;

    strct = &Struct::get_from_name(type_name, m_program.struct_storage); 

    if (strct == nullptr) TODO("trying to access a struct that doesn't exist");
    // maybe shared pointers??
    Variable* struct_var = new Variable;
    if (!member) {
        current_offset = align(current_offset, Kind::Struct, type_name);
        current_offset += strct->size;
        *struct_var = {.type = type_infos.at(type_name), .name = struct_name, .offset = current_offset, .size = strct->size};
    } else {
        current_offset = align(current_offset, Kind::Struct, type_name);
        *struct_var = {.type = type_infos.at(type_name), .name = struct_name, .offset = current_offset, .size = strct->size};
        current_offset += strct->size;
    }


    for (size_t i = 0; i < strct->var_storage.size(); i++) {
        auto var = strct->var_storage[i];
        var.parent = struct_var;

        // I think this should be removed
        if (var.type.info.kind == Kind::Struct) {
            size_t temp_offset = current_offset;
            Variable strct_;
            if (var.type.info.kind == Kind::Pointer) {
                strct_ = initStruct(var.type.info.name, var.name, true, false);
            } else {
                strct_ = initStruct(var.type.info.name, var.name, true, !strct->defaults.contains(i));
            }
            if (current_offset > max_locals_offset) max_locals_offset = current_offset;
            current_offset = temp_offset;


            strct_.parent = struct_var;
            strct_.type = set_ptr_count(var.type, get_ptr_count(var.type));
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
            if (var.type.info.id == TypeId::String) {
                //ERROR((*tkn)->loc, "init str");
                //TODO("m_currentFunc->body.push_back({Op::INIT_STRING, {var}})");
            }
            if (def.type.info.kind != Kind::Void && var.type.info.kind != Kind::Struct) {
                m_body->body.push_back(Store_Ast::make_node(var, Load_Ast::make_node(def)));
            }
            if (def.type.info.kind == Kind::Struct) {
                for (size_t j = 0; j < def.members.size() && j < var.members.size(); j++) {
                    def.members[j].type.qualifiers |= Qualifier::literal;
                    if (def.members[j].type.info.id == TypeId::Float) {
                        def.members[j].type = type_infos.at("double");
                        def.members[j].size = 8;
                    }
                    if (def.members[j].type.info.id == TypeId::String) {
                        //ERROR((*tkn)->loc, "init str");
                        //TODO("m_currentFunc->body.push_back({Op::INIT_STRING, {var.members[j]}})");
                    }
                    if (def.members[j].type.info.kind != Kind::Void && def.members[j].type.info.kind != Kind::Struct) {
                        m_body->body.push_back(Store_Ast::make_node(var.members[j], Load_Ast::make_node(def.members[j])));
                    }
                }
            }
        }
    }
    if (member)
        current_offset += strct->size;

    return *struct_var;
}
void Parser::parseStructDeclaration() {
    m_lexar->getAndExpectNext(TokenType::ID);
    tkn = &m_lexar->currentToken;

    current_module_prefix = "";
    if (m_lexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_lexar->getAndExpectNext(TokenType::ID);
    }
    current_offset = 0;
    std::string struct_name = current_module_prefix + m_lexar->currentToken->string_value;
    current_module_prefix = "";

    size_t struct_index = m_program.struct_storage.size();
    m_program.struct_storage.push_back({});
    Struct& current_struct = m_program.struct_storage[struct_index];
    current_struct.id = current_typeid_max++;
    current_struct.is_float_only = true;

    TypeIds.emplace(struct_name, current_struct.id);
    size_t struct_info_index = type_infos.size();
    type_infos.emplace(struct_name, TypeInfo{.id = current_struct.id, .kind = Kind::Struct, .size = 0, .name = struct_name});

    size_t type_info_func_index = m_program.func_storage.size();
    {
        Func f = Func();
        *f.type.func_data->return_type = type_infos.at("TypeInfo");
        f.name = struct_name + "___" + "type_info";
        f.is_static = true;
        m_program.func_storage.push_back(std::move(f));
    }

    m_lexar->getAndExpectNext(TokenType::OCurly);

    current_struct.name = struct_name;
    while ((*tkn)->type != TokenType::CCurly) {
        if ((*tkn)->type == TokenType::Func) {
            size_t temp_offset = current_offset;
            size_t func_index = m_program.func_storage.size();
            parseFunction(true, current_struct);

            m_lexar->getNext();
            current_offset = temp_offset;
            continue;
        }
        Func* last_func = m_func;
        m_func = &default_func;
        auto& var = parseVariable(current_struct.var_storage, true);
        int var_index = current_struct.var_storage.size()-1;
        current_struct.size = align(current_struct.size, var.type, var.type.info.name);
        if (var.type.info.kind == Kind::Struct) {
            current_struct.alignment = std::max((int)current_struct.alignment, (int)Struct::get_from_name(var.type.info.name, m_program.struct_storage).alignment);
        } else {
            current_struct.alignment = std::max((int)current_struct.alignment, (int)var.size);
        }
        var.offset = current_struct.size;
        current_struct.size += var.size;
        m_func = last_func;
        if (var.type.info.kind != Kind::Float)
            current_struct.is_float_only = false;

        // Defaults
        if (m_lexar->peek()->type == TokenType::Eq) {
            m_lexar->getAndExpectNext(TokenType::Eq);
            m_lexar->getNext();
            auto var2 = std::get<0>(parseExpression());
            TODO("default");
            //current_struct.defaults.emplace(var_index, var2);
        } else if (var.type.info.kind != Kind::String && var.type.info.kind != Kind::Struct) {
            Variable default_val;
            default_val.name = "def_value";
            default_val.type.qualifiers |= Qualifier::literal;
            default_val.type.qualifiers |= Qualifier::constant;
            if (var.type.info.kind == Kind::Float) {
                default_val.type = type_infos.at("double");
                default_val.size = 8;
                default_val.Double_val = std::any_cast<double>(Variable::default_value(default_val.type));
                if (!Variable::is_in_storage(default_val.name, m_program.var_storage))
                    m_program.var_storage.push_back(default_val);
            } else {
                default_val.type = type_infos.at("int8");
                default_val.Int_val = std::any_cast<int64_t>(Variable::default_value(default_val.type));
                default_val.size = 1;
            }

            current_struct.defaults.emplace(var_index, default_val);
        } else {
            current_struct.defaults.emplace(var_index, Variable{ type_infos.at("void") });
        }
        m_lexar->getAndExpectNext(TokenType::SemiColon);
        m_lexar->getNext();
    }
    type_infos.at(struct_name).size = current_struct.size;
    m_program.func_storage[type_info_func_index] = make_type_info_func(current_struct);
    current_offset = 0;
}
Variable Parser::parseConstant() {
    Variable var{};
    m_lexar->getAndExpectNext(TokenType::ID);   
    var.name = (*tkn)->string_value;
    m_lexar->getAndExpectNext(TokenType::Eq);   
    m_lexar->getNext();   
    auto [rhs_expr, lvalue] = parseExpression();
    if (auto val = dynamic_cast<Load_Ast*>(rhs_expr.get())) {
        auto rhs = val->var;
        assert(rhs.type.qualifiers & Qualifier::constant);
        var.type = rhs.type;
        var.size    = rhs.size;
        copy_val(rhs, var);
        var.members = rhs.members;
        var.parent  = rhs.parent;
        var.type = set_ptr_count(var.type, get_ptr_count(rhs.type));
    } else {
        TODO("constants");
    }

    m_lexar->getAndExpectNext(TokenType::SemiColon);   
    return var;
}
void Parser::parseModuleDeclaration() {
    m_lexar->getAndExpectNext(TokenType::ID);

    ModuleStorage* current_mod = &m_program.module_storage;

    if (m_program.module_storage.contains((*tkn)->string_value)) {
        if (m_lexar->peek()->type == TokenType::ColonColon) {
            current_mod = &current_mod->at((*tkn)->string_value).module_storage;
            m_lexar->getAndExpectNext(TokenType::ColonColon);
            m_lexar->getAndExpectNext(TokenType::ID);
        } else {
            TODO("redeclare module");
        }
    }

    current_mod->emplace((*tkn)->string_value, Module{});
    m_lexar->getAndExpectNext(TokenType::SemiColon);
}
void Parser::parseHash() {
    m_lexar->getAndExpectNext({TokenType::Import, TokenType::Include, TokenType::Extern, TokenType::Not});
    switch ((*tkn)->type) {
        case TokenType::Not: {
            // Ignore shebangs
            auto line = (*tkn)->loc.line;
            while (m_lexar->peek()->loc.line == line) {
                m_lexar->getNext();
            }
        }break;//TokenType::Not
        case TokenType::Include: {
            if (m_lexar->peek()->loc.line == (*tkn)->loc.line)
                m_lexar->getAndExpectNext(TokenType::Less);
            else 
                TODO("no file provided to include");

            std::string file_name{};
            while (m_lexar->peek()->loc.line == (*tkn)->loc.line && m_lexar->peek()->type != TokenType::Greater) {
                m_lexar->getNext();
                file_name += (*tkn)->string_value;
            }
            m_lexar->getNext();
            for (std::string& file : included_files) if (file == file_name) goto end_of_include;
            if (file_name == (*tkn)->loc.inputPath)
                TODO("cannot include the current file");
            if (fileExistsInPaths(file_name, ctx.includePaths)) {
                std::string file_path = getFilePathFromPaths(file_name, ctx.includePaths);

                Lexar l(readFileToString(file_path), file_path);

                m_lexar->pushtokensaftercurrent(&l);
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
        default:
            TODO("unexpected");
    }
}
void Parser::parseExtern() {
    m_lexar->getAndExpectNext(TokenType::Func);

    Func func = Func();
    m_func = &func;
    func.external = true;

    // Func name
    m_lexar->getAndExpectNext(TokenType::ID);
    current_module_prefix = "";
    if (m_lexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_lexar->getAndExpectNext(TokenType::ID);
    }
    func.name = current_module_prefix + m_lexar->currentToken->string_value;
    func.link_name = func.name;                
    current_module_prefix = "";

    m_lexar->getAndExpectNext(TokenType::OParen);
    while (m_lexar->peek()->type != TokenType::CParen && (*tkn)->type != TokenType::EndOfFile) {
        if (m_lexar->peek()->type == TokenType::Dot) {
            m_lexar->getAndExpectNext(TokenType::Dot);
            m_lexar->getAndExpectNext(TokenType::Dot);
            m_lexar->getAndExpectNext(TokenType::Dot);
            func.c_variadic = true;
            
            break;
        } else {
            func.local_variables.push_back(parseVariable(func.arguments));
            func.arguments_count++;
        }
        if (m_lexar->peek()->type != TokenType::CParen) {
            m_lexar->getAndExpectNext(TokenType::Comma);
            if (m_lexar->peek()->type == TokenType::CParen) {
                ERROR((*tkn)->loc, "cannot do trailing commas in function calls");
            }
        }
    }

    m_lexar->getAndExpectNext(TokenType::CParen);

    // Returns
    if (m_lexar->peek()->type == TokenType::Arrow) {
        m_lexar->getAndExpectNext(TokenType::Arrow);
        m_lexar->getAndExpectNext({TokenType::TypeID, TokenType::ID});
        current_module_prefix = "";
        if (m_lexar->peek()->type == TokenType::ColonColon) {
            parseModulePrefix(); 
            m_lexar->getNext();
        }
        *func.type.func_data->return_type = type_infos.at(current_module_prefix + (*tkn)->string_value);
        current_module_prefix = "";
    }
    if (func.type.func_data->return_type->info.size > 16) {
        Variable ret = {
            .type = make_ptr(*func.type.func_data->return_type),
            .name   = "return_register",
            .offset = 8,
            .size   = 8,
        };
        func.arguments.emplace(func.arguments.begin(), ret);
    }
    if (m_lexar->peek()->type == TokenType::OBracket) {
        m_lexar->getAndExpectNext(TokenType::OBracket);
        do {
            m_lexar->getAndExpectNext(TokenType::ID);
            std::string s = (*tkn)->string_value;
            m_lexar->getAndExpectNext(TokenType::Eq);
            m_lexar->getAndExpectNext(TokenType::DQoute);
            m_lexar->getAndExpectNext(TokenType::StringLit);
            std::string seq = (*tkn)->string_value;
            m_lexar->getAndExpectNext(TokenType::DQoute);

            if (s == "link_name")
                func.link_name = seq;                
            else if (s == "lib")
                func.lib = seq;                
            else if (s == "search_path")
                func.search_path = seq;                
            else 
                TODO("ERROR: UNREACHABLE");
            
            if (m_lexar->peek()->type != TokenType::CBracket) {
                m_lexar->getAndExpectNext(TokenType::Comma);
            } else {
                m_lexar->getAndExpectNext(TokenType::CBracket);
            }
        } while ((*tkn)->type != TokenType::CBracket);
    }

    m_lexar->getAndExpectNext(TokenType::SemiColon);

    m_program.func_storage.push_back(std::move(func));
    m_func = &default_func;

}
void Parser::parseModulePrefix() {
    auto current_module_storage = &m_program.module_storage;

    if (!current_module_storage->contains((*tkn)->string_value)) { 
        mlog::println("{}:{}:{} tkn = {}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset, (*tkn)->string_value);
        TODO("error");
    }

    m_currentFuncStorage   = &current_module_storage->at((*tkn)->string_value).func_storage;
    current_module_storage = &m_program.module_storage.at((*tkn)->string_value).module_storage;
    current_module_prefix += (*tkn)->string_value + "__";

    m_lexar->getAndExpectNext(TokenType::ColonColon);

    while ((m_lexar->peek() + 1)->type == TokenType::ColonColon) {
        m_lexar->getAndExpectNext(TokenType::ID);
        if (!current_module_storage->contains((*tkn)->string_value)) { 
            mlog::println("{}:{}:{} tkn = {}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset, (*tkn)->string_value);
            TODO("error");
        }

        m_currentFuncStorage   = &current_module_storage->at((*tkn)->string_value).func_storage;
        current_module_storage = &current_module_storage->at((*tkn)->string_value).module_storage;
        current_module_prefix += (*tkn)->string_value + "__";

        m_lexar->getAndExpectNext(TokenType::ColonColon);
    }
}
void Parser::parseFunction(bool member, Struct parent) {
    tkn = &m_lexar->currentToken;
    current_offset = 0;
    Func func = Func();
    std::string name{};
    m_func = &func;
    if (m_lexar->peek()->type == TokenType::Static) {
        m_lexar->getAndExpectNext(TokenType::Static);
        func.is_static = true;
    }
    m_lexar->getAndExpectNext({TokenType::ID, TokenType::TypeID});
    current_module_prefix = "";
    if (m_lexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_lexar->getAndExpectNext(TokenType::ID);
    }
    if (member) {
        name = parent.name + "___" + current_module_prefix + (*tkn)->string_value;
    } else if (!member && m_lexar->peek()->type == TokenType::Dot) {
        std::string type_name = current_module_prefix + (*tkn)->string_value;
        if (type_infos.contains(type_name)) {
            auto type = type_infos.at(type_name);
            if (type.kind == Kind::Struct)
                parent = Struct::get_from_name(type_name, m_program.struct_storage);
            else 
                parent.name = type_name;
        }
        member = true;
        m_lexar->getAndExpectNext(TokenType::Dot);
        m_lexar->getAndExpectNext(TokenType::ID);
        current_module_prefix = "";
        if (m_lexar->peek()->type == TokenType::ColonColon) {
            parseModulePrefix();
            m_lexar->getAndExpectNext(TokenType::ID);
        }
        name = type_name + "___" + current_module_prefix + (*tkn)->string_value;
    } else {
        name = current_module_prefix + (*tkn)->string_value;
    }
    if (func.is_static && !member) 
        TODO("cannot have static non member functions");
    func.name = name;
    current_module_prefix = "";
    if (name == "main") 
        func.is_used = true;

    if (member && !func.is_static) {
        current_offset += 8;
        Variable this_pointer = {
            .type    = make_ptr(TypeInfo::get_from_id(parent.id)),
            .name    = "this",
            .offset  = current_offset,
            .size    = 8,
            .members = parent.var_storage,
        };
        func.local_variables.push_back(this_pointer);
        func.arguments.push_back(this_pointer);
    }
    m_lexar->getAndExpectNext(TokenType::OParen);
    VariableStorage args_temp_storage{};
    m_func = &default_func;
    while (m_lexar->peek()->type != TokenType::CParen && (*tkn)->type != TokenType::EndOfFile && m_lexar->peek()->type != TokenType::EndOfFile) {
        parseVariable(args_temp_storage);
        if (m_lexar->peek()->type != TokenType::CParen) {
            m_lexar->expectNext(TokenType::Comma);
            m_lexar->getNext();
        }
    }
    m_lexar->getAndExpectNext(TokenType::CParen);
    m_lexar->expectNext({TokenType::Arrow, TokenType::OCurly});

    // Returns
    if (m_lexar->peek()->type == TokenType::Arrow) {
        m_lexar->getAndExpectNext(TokenType::Arrow);
        m_lexar->getAndExpectNext({TokenType::TypeID, TokenType::ID});
        current_module_prefix = "";
        if (m_lexar->peek()->type == TokenType::ColonColon) {
            parseModulePrefix();
            m_lexar->getNext();
        }
        if (type_infos.contains(current_module_prefix + (*tkn)->string_value))
            *func.type.func_data->return_type = type_infos.at(current_module_prefix + (*tkn)->string_value);
        else 
            ERROR((*tkn)->loc, mlog::format("Type {} does not exist", current_module_prefix + (*tkn)->string_value));
        if (m_lexar->peek()->type == TokenType::OBracket) {
            m_lexar->getNext();
            m_lexar->getAndExpectNext(TokenType::CBracket);
            TODO("add arrays return");
        }
        while (m_lexar->peek()->type == TokenType::Mul) {
            m_lexar->getNext();
            *func.type.func_data->return_type = make_ptr(*func.type.func_data->return_type);
        }
        current_module_prefix = "";
    } else {
        *func.type.func_data->return_type = type_infos.at("void");
    }
                

    for (int i = 0; i < args_temp_storage.size(); i++) {
        auto var = args_temp_storage[i];
        if (var.type.info.kind == Kind::Struct) {
            func.arguments.push_back(var);
            func.local_variables.push_back(var);
            func.arguments_count++; 
        } else {
            func.arguments.push_back(var);
            func.local_variables.push_back(var);
            func.arguments_count++;
        }
    }

    m_lexar->getAndExpectNext(TokenType::OCurly);

    auto func_index = m_program.func_storage.size();
    m_program.func_storage.push_back(std::move(func));
    m_func = &m_program.func_storage[func_index];

    m_func->body = parseBlock();

    m_func->stack_size = max_locals_offset;
    max_locals_offset = 8;
}
size_t temp_offset = 0;
Variable make_temp_var(Type type) {
    Variable var;
    var.type = type.info;
    var.size = type.info.size;
    var.name = mlog::format("%%temp%%{}", temp_count++);
    current_offset = Parser::align(current_offset, type);
    temp_offset = Parser::align(temp_offset, type);
    var.offset = current_offset + temp_offset + type.info.size;
    if (max_locals_offset < current_offset + temp_offset) {
        max_locals_offset = current_offset + temp_offset;
    }
    temp_offset += type.info.size;
    if (temp_offset > max_locals_offset) max_locals_offset = temp_offset;
    var.name = "temporary variable";
    return var;
}
void delete_temp_vars() {
    temp_offset = 0;
}

StmtNode Parser::parseStatement() {
    StmtNode stmt;
    statement_count++;
    switch ((*tkn)->type) {
        case TokenType::SemiColon: { }break;
        case TokenType::OCurly: {
            parseBlock();
        }break;//TokenType::OCurly
        case TokenType::Return: {
            Node return_value;
            m_lexar->getNext();

            if ((*tkn)->type == TokenType::SemiColon) {
                if (m_func->type.func_data->return_type->info.kind == Kind::Void) TODO("error on no return");
                return_value = std::make_unique<Load_Ast>(Variable{
                    .type    = Type(
                        type_infos.at("int8"),
                        Qualifier::literal | Qualifier::constant
                    ),
                    .name    = "IntLit",
                    .Int_val = (int64_t)0,
                    .size    = 1,
                });
            } else {
                return_value = std::get<0>(parseExpression());
            }
            stmt = Return_Ast::make_node(std::move(return_value));
            m_lexar->getAndExpectNext(TokenType::SemiColon);

        }break;//TokenType::Return
        case TokenType::If: {
            TODO("if");
            /*
            m_currentLexar->getAndExpectNext(TokenType::OParen);
            m_currentLexar->getNext();
            auto expr = std::get<0>(parseExpression());
            m_currentLexar->getAndExpectNext(TokenType::CParen);

            delete_temp_vars();

            m_currentLexar->getNext();
            size_t jmp_if_not = m_currentFunc->body->body.size();
            m_currentFunc->body.push_back({Op::JUMP_IF_NOT, {"", expr}});
            parseStatement();
            if (m_currentLexar->peek()->type == TokenType::Else) {
                size_t jmp_else = m_currentFunc->body.size();
                m_currentFunc->body.push_back({Op::JUMP, {""}});
                std::string label = mlog::format("{:06x}", statement_count++);
                m_currentFunc->body.push_back({Op::LABEL, {label}});
                m_currentFunc->body[jmp_if_not].args[0] = label;
                m_currentLexar->getAndExpectNext(TokenType::Else);
                m_currentLexar->getNext();
                parseStatement();
                label = mlog::format("{:06x}", statement_count++);
                m_currentFunc->body.push_back({Op::LABEL, {label}});
                m_currentFunc->body[jmp_else].args[0] = label;
            } else {
                std::string label = mlog::format("{:06x}", statement_count++);
                m_currentFunc->body.push_back({Op::LABEL, {label}});
                m_currentFunc->body[jmp_if_not].args[0] = label;
            }
            */
        }break;//TokenType::If
        case TokenType::While: {
            TODO("while");
            /*
            m_currentLexar->getAndExpectNext(TokenType::OParen);
            m_currentLexar->getNext();
            std::string pre_label = mlog::format("{:06x}", statement_count++);
            m_currentFunc->body.push_back({Op::LABEL, {pre_label}});
            auto expr = std::get<0>(parseExpression());
            m_currentLexar->getAndExpectNext(TokenType::CParen);

            delete_temp_vars();

            m_currentLexar->getNext();
            size_t jmp_if_not = m_currentFunc->body.size();
            m_currentFunc->body.push_back({Op::JUMP_IF_NOT, {"", expr}});
            parseStatement();
            m_currentFunc->body.push_back({Op::JUMP, {pre_label}});
            std::string label = mlog::format("{:06x}", statement_count++);
            m_currentFunc->body.push_back({Op::LABEL, {label}});
            m_currentFunc->body[jmp_if_not].args[0] = label;
            */
        }break;//TokenType::While
        case TokenType::ID:
        case TokenType::TypeID: {
            std::string type_name = (*tkn)->string_value;
            size_t current_point = m_lexar->currentTokenIndex;
            auto current_body_size = m_body->body.size();
            auto node = std::get<0>(parsePrimaryExpression());
            auto peek = m_lexar->peek();
            m_lexar->currentTokenIndex = current_point - 1;
            m_lexar->getNext();
            if (auto type_node = dynamic_cast<Load_Ast*>(node.get())) {
                if (type_node->var.type.info.id != TypeId::Typeid) {
                    m_body->body.resize(current_body_size);
                    goto defau;
                }
                type_name = TypeInfo::get_from_id(type_node->var.Int_val).name;
            } else {
                m_body->body.resize(current_body_size);
                goto defau;
            }
            current_body_size = m_body->body.size();
            auto var = parseVariable(m_func->local_variables);

            // TODO: factor out to a function
            if (var.type.info.id == TypeId::String) {
                m_func->local_variables.push_back(var);
                //m_body->body.push_back({Op::INIT_STRING, {var}});
            }
            if (m_lexar->peek()->type == TokenType::Eq) {
                m_lexar->getAndExpectNext(TokenType::Eq);
                m_lexar->getNext();
                m_func->body->body.resize(current_body_size);
                auto var2 = std::get<0>(parseExpression());
                stmt = Store_Ast::make_node(var, std::move(var2));
            } else if (var.type.info.kind != Kind::String && var.type.info.kind != Kind::Struct) {
                Variable default_val;
                default_val.type.qualifiers = Qualifier::literal | Qualifier::constant;
                default_val.name = "def_value";
                if (var.type.info.id == TypeId::Double || var.type.info.id == TypeId::Float) {
                    default_val.type = type_infos.at("double");
                    default_val.size = 8;
                    default_val.Double_val = std::any_cast<double>(Variable::default_value(var.type));
                    if (!Variable::is_in_storage(default_val.name, m_program.var_storage))
                        m_program.var_storage.push_back(default_val);
                } else {
                    default_val.type = type_infos.at("int8");
                    default_val.size = 1;
                    default_val.Int_val = std::any_cast<int64_t>(Variable::default_value(var.type));
                }

                stmt = Store_Ast::make_node(var, Load_Ast::make_node(default_val));
            } else if (var.type.info.kind == Kind::Pointer) {
                Variable default_val;
                default_val.type = type_infos.at("int64");
                default_val.type.qualifiers = Qualifier::literal | Qualifier::constant;
                default_val.size = 1;

                default_val.name = "def_value";
                default_val.Int_val = std::any_cast<int64_t>(Variable::default_value(default_val.type));
                stmt = Store_Ast::make_node(var, Load_Ast::make_node(default_val));
            }

            m_lexar->getAndExpectNext(TokenType::SemiColon);
        }break;//TokenType::TypeID
        default: {
        defau:
            auto [lhs, lvalue] = parseDotExpression();
            auto peek_type = m_lexar->peek()->type;

            if (lvalue) {
                if (peek_type == TokenType::Eq || peek_type == TokenType::PlusEq || peek_type == TokenType::MinusEq || peek_type == TokenType::MulEq) {
                    m_lexar->getNext();
                    m_lexar->getNext();
                    auto rhs = std::get<0>(parseExpression());

                    switch (peek_type) {
                    case TokenType::Eq:
                        stmt = Store_Ast::make_node(std::move(rhs), std::move(lhs));
                        break;
                    case TokenType::PlusEq:
                        stmt = Store_Ast::make_node(BinOp_Ast::make_node(std::move(lhs), std::move(rhs), BinOp::ADD), std::move(lhs));
                        break;
                    case TokenType::MinusEq:
                        stmt = Store_Ast::make_node(BinOp_Ast::make_node(std::move(lhs), std::move(rhs), BinOp::SUB), std::move(lhs));
                        break;
                    case TokenType::MulEq:
                        stmt = Store_Ast::make_node(BinOp_Ast::make_node(std::move(lhs), std::move(rhs), BinOp::MUL), std::move(lhs));
                        break;
                    default: TODO("unsupported Token found");
                    }

                }
            } else {
                stmt = std::move(lhs);
            }

            m_lexar->getAndExpectNext(TokenType::SemiColon);
        }
    }
    delete_temp_vars();
    stmt->loc_end = (*tkn)->loc;
    return stmt;
}

BodyNode Parser::parseBlock() {
    size_t offset = current_offset;
    auto storage = m_func->local_variables;
    BodyNode body = std::make_unique<Body_Ast>();
    m_body = body.get();
    m_body->loc_start = (*tkn)->loc;

    while (m_lexar->peek()->type != TokenType::CCurly) {
        m_lexar->getNext();
        m_body->body.push_back(parseStatement());
    }
    m_lexar->getAndExpectNext(TokenType::CCurly);
    m_body->loc_end = (*tkn)->loc;

    if (current_offset > max_locals_offset) max_locals_offset = current_offset;

    m_func->local_variables = storage;
    current_offset = offset;

    return body;
}
void print_var(Variable var) {
    mlog::println("name: {}", var.name);
    mlog::println("offset: {}, size: {}", var.offset, var.size);
    mlog::println("deref_count: {}", var.deref_count);
    mlog::println("type: {{");
    mlog::println("  info: {{");
    mlog::println("    id:   {}", var.type.info.id);
    mlog::println("    name: {}", var.type.info.name);
    mlog::println("    kind: {}", (int)var.type.info.kind);
    mlog::println("  }}");
    mlog::println("}}");
}
ExprResult Parser::parsePrimaryExpression(Variable this_ptr, Variable this_, std::string func_prefix) {
    ExprResult data;
    tkn = &m_lexar->currentToken;
    Variable var;
    bool ret_lvalue = false;


    if ((*tkn)->type == TokenType::DQoute) {
        m_lexar->getAndExpectNext(TokenType::StringLit);
        var = {
            .type = Type(
                type_infos.at("string"),
                Qualifier::literal | Qualifier::constant
            ),
            .name = mlog::format("literal_{}", literal_count++),
            .String_val = (*tkn)->string_value,
            .size = 8,
        };
        m_program.var_storage.push_back(var);
        m_lexar->getAndExpectNext(TokenType::DQoute);
        return {Load_Ast::make_node(var), ret_lvalue};
    }

    if ((*tkn)->type == TokenType::IntLit) {
        int64_t value = (*tkn)->int_value;
        size_t size = size_of_signed_int(value);
        var = {
            .type = Type(
                sized_int_type(size),
                Qualifier::literal | Qualifier::constant
            ),
            .name  = "Int_Lit",
            .Int_val = value,
            .size  = size,
        };
        return {Load_Ast::make_node(var), ret_lvalue};
    }
    if ((*tkn)->type == TokenType::DoubleLit) {
        var = {
            .type = Type(
                type_infos.at("double"),
                Qualifier::literal | Qualifier::constant
            ),
            .name  = mlog::format("literal_{}", literal_count++),
            .Double_val = (*tkn)->double_value,
            .size  = 8,
        };
        m_program.var_storage.push_back(var);
        return {Load_Ast::make_node(var), ret_lvalue};
    }
    
    if ((*tkn)->type == TokenType::PlusPlus) {
        TODO("unimplemented");
        /*
        auto loc = (*tkn)->loc;
        m_currentLexar->getNext();
        size_t add_loc = m_currentFunc->body.size();
        m_currentFunc->body.push_back({Op::BIN_OP, {}});
        data = parsePrimaryExpression();
        auto &[var, lvalue] = data;
        if (!lvalue) ERROR(loc, "cannot pre-increment a non lvalue");
        Variable amount = {
            .type = Type(
                type_infos.at("int8"),
                Qualifier::literal | Qualifier::constant
            ),
            .name = "Int_Lit",
            .Int_val = (int64_t)1,
            .size = 1,
        };
        m_currentFunc->body[add_loc].args = {BinOp::ADD, var, amount, var};
        ret_lvalue = true;
        return {var, ret_lvalue};
        */
    }
    if ((*tkn)->type == TokenType::MinusMinus) {
        TODO("unimplemented");
        /*
        auto loc = (*tkn)->loc;
        m_currentLexar->getNext();
        size_t add_loc = m_currentFunc->body.size();
        m_currentFunc->body.push_back({Op::BIN_OP, {}});
        data = parsePrimaryExpression();
        auto &[var, lvalue] = data;
        if (!lvalue) ERROR(loc, "cannot pre-decrement a non lvalue");
        Variable amount = {
            .type = Type(
                type_infos.at("int8"),
                Qualifier::literal | Qualifier::constant
            ),
            .name = "Int_Lit",
            .Int_val = (int64_t)1,
            .size = 1,
        };
        m_currentFunc->body[add_loc].args = {BinOp::SUB, var, amount, var};
        ret_lvalue = true;
        return {var, ret_lvalue};
        */
    }
    if ((*tkn)->type == TokenType::Mul) { 
        m_lexar->getNext();
        auto [var, lvalue] = parsePrimaryExpression();
        var = Deref_Ast::make_node(std::move(var));
        ret_lvalue = true;
        return {std::move(var), ret_lvalue};
    }
    if ((*tkn)->type == TokenType::And) {
        Loc loc = (*tkn)->loc;
        m_lexar->getNext();
        auto [var, lvalue] = parsePrimaryExpression();
        auto var_ = dynamic_cast<Load_Ast*>(var.get());
        if (!lvalue) ERROR(loc, "cannot refernce a non lvalue");
        var = Ref_Ast::make_node(var_->var);
        return {std::move(var), ret_lvalue};
    }

    current_module_prefix = "";
    if (m_lexar->peek()->type == TokenType::ColonColon) {
        parseModulePrefix();
        m_lexar->getAndExpectNext(TokenType::ID);
    }

    std::string name      = current_module_prefix + m_lexar->currentToken->string_value;
    std::string func_name = current_module_prefix + func_prefix + m_lexar->currentToken->string_value;
    if ((*tkn)->type == TokenType::ID || (*tkn)->type == TokenType::TypeID) {
        if (m_lexar->peek()->type == TokenType::OParen) {
            if (!Func::is_in_storage(func_name, m_program.func_storage)) {
                mlog::println("functions:");
                for (auto& func : m_program.func_storage) {
                    mlog::println("  {}", func.name);
                }
                TODO(mlog::format("func {} doesn't exist", func_name));
            }
            auto& func = Func::get_from_name(func_name, m_program.func_storage);
            var = make_temp_var(*func.type.func_data->return_type);
            if (var.type.info.kind == Kind::Pointer)
                var.size = 8;
            if (var.type.info.kind == Kind::Struct) {
                var.members = Struct::get_from_name(var.type.info.name, m_program.struct_storage).var_storage;
            }
            if (!func.is_static && this_.type.info.id == TypeId::Typeid)
                TODO("cannot use Typeid to call a non static function");
            ExprNode result;
            if (!func.is_static)
                result = parseFuncCall(func, this_ptr, var);
            else 
                result = parseFuncCall(func, {type_infos.at("void")}, var);
            var.parent = nullptr;
            return {std::move(result), ret_lvalue};
        }
        if (type_infos.contains(name)) {
            TypeInfo type = type_infos.at(name);
            //Struct_literal
            if (type.kind == Kind::Struct) {
                VariableStorage v{};
                if (m_lexar->peek()->type == TokenType::OCurly) {
                    m_lexar->getNext();
                    while (m_lexar->peek()->type != TokenType::CCurly) {
                        m_lexar->getNext();
                        TODO("unimplemented");
                        //v.push_back(std::get<0>(parseExpression()));
                        if (m_lexar->peek()->type == TokenType::Comma)
                            m_lexar->getAndExpectNext(TokenType::Comma);
                    }
                    m_lexar->getAndExpectNext(TokenType::CCurly);
                    // saving offset to make struct literal temporary
                    auto save_off  = current_offset;
                    auto save_func = m_func;
                    auto strct = Struct::get_from_name(name, m_program.struct_storage);
                    m_func = &default_func;
                    var = initStruct(name, mlog::format("%%tmp%%{}", temp_count++));
                    m_func  = save_func;
                    //for (size_t i = 0; i < var.members.size(); i++) {
                    //    var.members[i].name = v[i].name;
                    //    if (i < v.size()) {
                    //        copy_val(v[i], var.members[i]);
                    //    } else if (strct.defaults.contains(i)) {
                    //        v.push_back(strct.defaults.at(i));
                    //        copy_val(v[i], var.members[i]);
                    //    } else {
                    //        v.push_back({.type = type_infos.at("void")});
                    //    }
                    //    m_currentFunc->body.push_back({Op::STORE_VAR, {v[i], var.members[i]}});
                    //}
                    if (current_offset > max_locals_offset) max_locals_offset = current_offset;
                    current_offset = save_off;
                    return {Load_Ast::make_node(var), false};
                }
            }
            var.name = type.name;
            var.size = 8;
            var.type = type_infos.at("typeid");
            var.type.qualifiers = Qualifier::literal | Qualifier::constant;
            var.Int_val = (int64_t)type.id;
            return {Load_Ast::make_node(var), false};
        }
        if (this_ptr.type.info.kind != Kind::Void) {
            auto var_ = Variable::get_from_name(name, this_.members);
            var_.parent = new Variable;
            *var_.parent = this_;
            return {Load_Ast::make_node(var_), true};
        } else if (Variable::is_in_storage((*tkn)->string_value, m_program.var_storage)) {
            var = Variable::get_from_name((*tkn)->string_value, m_program.var_storage);
            return {Load_Ast::make_node(var), true};
        } else if (Variable::is_in_storage(name, m_func->local_variables)) {
            var = Variable::get_from_name(name, m_func->local_variables);
            ret_lvalue = true;
            return {Load_Ast::make_node(var), ret_lvalue};
        } else {
            TODO(mlog::format("ERROR: variable `{}` at {}:{}:{} wasn't found", name, (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset));
        }
    }

    // Paranthesis
    if ((*tkn)->type == TokenType::OParen) {
        m_lexar->getNext(); 
        data = parseExpression();
        auto &[var, lvalue] = data;
        m_lexar->getAndExpectNext(TokenType::CParen);
        //return {var, ret_lvalue};
    }

    ERROR((*tkn)->loc, mlog::format("unexpected token of type {}", printableToken.at((*tkn)->type)));
}
ExprResult Parser::parseDotExpression(Variable this_ptr, Variable this_, std::string func_prefix) {
    tkn = &m_lexar->currentToken;
    auto [lhs, lvalue] = parsePrimaryExpression(this_ptr, this_, func_prefix);

    if (m_lexar->peek()->type == TokenType::Dot) {
        m_lexar->getAndExpectNext(TokenType::Dot);
        m_lexar->getNext();
        
        Variable lhs_var;
        if (auto lhs_node = dynamic_cast<Load_Ast*>(lhs.get())) {
            lhs_var = lhs_node->var;
        } else {
            TODO("cannot do dot on expressions currently");
        }
        if (lhs_var.parent == nullptr) {
            this_ptr = lhs_var;
            this_    = lhs_var;
            this_ptr.deref_count = get_ptr_count(lhs_var.type) - 1;
            this_ptr.type = set_ptr_count(get_base_type(lhs_var.type), 1);
            this_ptr.size = 8;
        } else {
            lhs_var.parent = new Variable;
            *lhs_var.parent = this_;
            this_    = lhs_var;
            this_ptr = lhs_var;
            this_ptr.deref_count = get_ptr_count(this_ptr.type) - 1;
            this_ptr.type = set_ptr_count(get_base_type(this_ptr.type), 1);
            this_ptr.size = 8;
        }
        if (lhs_var.type.info.id == TypeId::Typeid)
            func_prefix = lhs_var.name;
        else
            func_prefix = get_base_type(lhs_var.type).info.name;
        func_prefix += "___";

        auto rhs = parseDotExpression(this_ptr, this_, func_prefix);
        return rhs;
    }
    return {std::move(lhs), lvalue};
}
ExprResult Parser::parseUnaryExpression() {
    tkn = &m_lexar->currentToken;

    if ((*tkn)->type == TokenType::Minus) {
        m_lexar->getNext();
        auto rhs = std::get<0>(parseUnaryExpression());
        if (auto rhs_node = dynamic_cast<Load_Ast*>(rhs.get())) {
            auto rhs_var = rhs_node->var;
            if (rhs_var.type.qualifiers & Qualifier::literal) {
                if (rhs_var.type.info.kind == Kind::Float) {
                    Variable::get_from_name(rhs_var.name, m_program.var_storage).Double_val = -rhs_var.Double_val;
                    return {Load_Ast::make_node(rhs_var), false};
                } else {
                    rhs_var.Int_val = -rhs_var.Int_val;
                    return {Load_Ast::make_node(rhs_var), false};
                }
            }
        } else {
            Variable zero   = {
                .type = Type(
                    type_infos.at("int8"),
                    Qualifier::literal | Qualifier::constant
                ),
                .name = "Int_Lit",
                .Int_val = (int64_t)0,
                .size = 1,
            };
            return {BinOp_Ast::make_node(Load_Ast::make_node(zero), std::move(rhs), BinOp::SUB), false};
        }
    }
    if ((*tkn)->type == TokenType::Not) {
        m_lexar->getNext();
        auto rhs = std::get<0>(parseUnaryExpression());
        auto type = Type(TypeInfo::get_from_id(TypeId::Bool));
        Variable result = make_temp_var(type);
        Variable zero   = {
            .type = Type(
                type_infos.at("int8"),
                Qualifier::literal | Qualifier::constant
            ),
            .name = "Int_Lit",
            .Int_val = 0,
            .size = 1,
        };
        return {BinOp_Ast::make_node(std::move(rhs), Load_Ast::make_node(zero), BinOp::EQ), false};
    }

    return parseDotExpression();
}
ExprResult Parser::parseMultiplicativeExpression() {
    tkn = &m_lexar->currentToken;
    auto [lhs, lvalue] = parseUnaryExpression();

    while ((*tkn)->type != TokenType::EndOfFile) {
        auto peek = m_lexar->peek()->type;
        if (peek != TokenType::Mul && peek != TokenType::Div && peek != TokenType::Mod) break;

        m_lexar->getNext();
        auto op_type = (*tkn)->type;
        m_lexar->getNext();
        auto rhs = std::get<0>(parseUnaryExpression());
        lvalue = false;

        if (op_type == TokenType::Mul) {
            lhs = BinOp_Ast::make_node(std::move(lhs), std::move(rhs), BinOp::MUL);
        } else if (op_type == TokenType::Div) {
            lhs = BinOp_Ast::make_node(std::move(lhs), std::move(rhs), BinOp::DIV);
        } else if (op_type == TokenType::Mod) {
            lhs = BinOp_Ast::make_node(std::move(lhs), std::move(rhs), BinOp::MOD);
        }
    }

    return {std::move(lhs), lvalue};
}
ExprResult Parser::parseAdditiveExpression() {
    tkn = &m_lexar->currentToken;
    auto [lhs, lvalue] = parseMultiplicativeExpression();

    while ((*tkn)->type != TokenType::EndOfFile) {
        TokenType peek = m_lexar->peek()->type;
        if (peek != TokenType::Plus && peek != TokenType::Minus) break;

        m_lexar->getNext();
        TokenType op_type = (*tkn)->type;
        m_lexar->getNext();
        auto rhs = std::get<0>(parseMultiplicativeExpression());
        lvalue = false;

        if (op_type == TokenType::Plus) {
            lhs = BinOp_Ast::make_node(std::move(lhs), std::move(rhs), BinOp::ADD);
        } else {
            lhs = BinOp_Ast::make_node(std::move(lhs), std::move(rhs), BinOp::SUB);
        }
    }

    return {std::move(lhs), lvalue};
}
ExprResult Parser::parseCondition(int min_prec) {
    tkn = &m_lexar->currentToken;
    auto [lhs, lvalue] = parseAdditiveExpression();

    while ((*tkn)->type != TokenType::EndOfFile) {
        auto peek = m_lexar->peek()->type;
        int prec = get_cond_precedence(peek);
        if (prec < min_prec) 
            break;

        BinOp op;
        switch (peek) {
            case TokenType::EqEq:      op = BinOp::EQ;   break;
            case TokenType::NotEq:     op = BinOp::NE;   break;
            case TokenType::Less:      op = BinOp::LT;   break;
            case TokenType::LessEq:    op = BinOp::LE;   break;
            case TokenType::Greater:   op = BinOp::GT;  break;
            case TokenType::GreaterEq: op = BinOp::GE; break;
            case TokenType::AndAnd:    op = BinOp::LAND; break;
            case TokenType::OrOr:      op = BinOp::LOR;  break;
            default:
                return {std::move(lhs), lvalue};
        }
        // consume operator
        m_lexar->getNext();
        m_lexar->getNext();

        auto rhs = std::get<0>(parseCondition(prec + 1));
        lhs = BinOp_Ast::make_node(std::move(lhs), std::move(rhs), op);
    }

    return {std::move(lhs), false};
}
ExprResult Parser::parseExpression() {
    return parseCondition(0);
}

ExprNode Parser::parseFuncCall(Func& func, Variable this_ptr, Variable return_address) {
    std::string loc = mlog::format("{}:{}:{}", (*tkn)->loc.inputPath, (*tkn)->loc.line, (*tkn)->loc.offset);
    std::vector<Node> args{};
    size_t given_args_count = 0;
    m_lexar->getAndExpectNext(TokenType::OParen);
    if (this_ptr.type.info.kind != Kind::Void) args.push_back(Load_Ast::make_node(this_ptr));
    while (m_lexar->peek()->type != TokenType::CParen) {
        m_lexar->getNext();
        //TODO("call func");
        args.push_back(std::get<0>(parseExpression()));
        if (m_lexar->peek()->type != TokenType::CParen) {
            m_lexar->getAndExpectNext(TokenType::Comma);
            if (m_lexar->peek()->type == TokenType::CParen) {
                ERROR((*tkn)->loc, "cannot do trailing commas in function calls");
            }
        }
        given_args_count++;
    }
    m_lexar->getAndExpectNext(TokenType::CParen);
    if (!func.variadic && !func.c_variadic) {
        if (func.arguments_count != given_args_count) {
            mlog::println("----------------------");
            for (auto& arg : func.arguments)
                mlog::println("{} {}", arg.name, arg.type.info.name);
            TODO(mlog::format("\n"
                   "{} incorrect amount of function arguments got {} but expected {}",
                   loc, given_args_count, func.arguments_count)
                 );
        }
        // TODO: check every argument
    }
    func.is_used = true;

    m_currentFuncStorage = &m_program.func_storage;
    current_module_prefix = "";
    return Call_Ast::make_node(func, std::move(args));
}
Func Parser::make_type_info_func(Struct s) {
    auto save = current_offset;
    auto orig = m_func;
    uint32_t literal = Qualifier::literal | Qualifier::constant;
    Func fn = Func();
    m_func = &fn;
    m_body = fn.body.get();
    fn.name = s.name + "___" + "type_info";
    *fn.type.func_data->return_type = Type(type_infos.at("TypeInfo"));
    fn.is_static = true;
    Variable typeinfo;
    typeinfo = initStruct("TypeInfo", "struct literal");
    Variable id   = {.type = Type(type_infos.at("int64"), literal), .Int_val = (int64_t)type_infos.at(s.name).id};
    Variable type = {.type = Type(type_infos.at("int32"), literal), .Int_val = (int64_t)type_infos.at(s.name).kind};
    Variable size = {.type = Type(type_infos.at("int64"), literal), .Int_val = (int64_t)type_infos.at(s.name).size};
    Variable name = {
        .type = Type(type_infos.at("string"), literal),
        .name  = mlog::format("literal_{}", literal_count++),
        .String_val = type_infos.at(s.name).name,
    };
    fn.arguments = {{.type = make_ptr(type_infos.at("TypeInfo")), .offset = 8, .size = 8}};
    fn.arguments_count = 0;
    fn.stack_size = 48;
    m_program.var_storage.push_back(name);
    
    fn.body->body.push_back(Store_Ast::make_node(typeinfo.members[0], Load_Ast::make_node(id)));
    fn.body->body.push_back(Store_Ast::make_node(typeinfo.members[1], Load_Ast::make_node(type)));
    fn.body->body.push_back(Store_Ast::make_node(typeinfo.members[2], Load_Ast::make_node(size)));
    fn.body->body.push_back(Store_Ast::make_node(typeinfo.members[3], Load_Ast::make_node(name)));
    fn.body->body.push_back(Return_Ast::make_node(Load_Ast::make_node(typeinfo)));
    current_offset = save;
    m_func = orig;
    return fn;
}
