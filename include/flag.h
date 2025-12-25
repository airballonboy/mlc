#pragma once 
#include <cstdio>
#include <print>
#include <string_view>
#include <type_traits>
#include <vector>

class FLAG_BASE {
public:
    FLAG_BASE()  = default;

    static std::vector<std::string_view> parse_flags(int &argc, char** &argv) {
        for (size_t i = 0; i < argc; i++)
            m_args.push_back(argv[i]);

        current_iteratior = 0;
        auto max = argc;
        while (current_iteratior < max) {
            bool is_flag = false;
            for (auto& flag : flags) {
                if(flag->parse_flag(m_args[current_iteratior], argc)) { 
                    not_ret.push_back(m_args[current_iteratior]);
                    is_flag = true;
                    break;
                }
            }
            if (!is_flag && m_args[current_iteratior].starts_with("-")) {
                std::println(stderr, "flag {} was not found", m_args[current_iteratior]);
                exit(1);
            }
            current_iteratior++;
        }
        for (auto& a : m_args) {
            for (auto& b : not_ret) {
                if (a == b) goto cont;
            }
            ret.push_back(a);
            cont: // continue
		    (void)0;
        }
        return ret;
    }
    virtual bool parse_flag(std::string _flag, int& argc) { return false; };

    std::string name;
    std::string desc;
    std::vector<const char*> names{""};
    inline static std::vector<FLAG_BASE*> flags;
protected:
    inline static std::vector<std::string_view> not_ret;
    inline static std::vector<std::string_view> ret;
    inline static std::vector<std::string> m_args;
    inline static size_t current_iteratior;
};
template <typename FlagType>
class Flag : public FLAG_BASE {
public:
    Flag() = default;
    Flag(std::vector<const char*> _names, std::string_view _desc) {
        names = _names;
        desc = _desc;
        flags.push_back(this);
    }
    Flag(const char* _name, std::string_view _desc) {
        name = std::string(_name);
        desc = _desc;
        flags.push_back(this);
    }
    ~Flag() {
        if(value) {
            delete value;
        }
    }
    bool exists     = false;
    FlagType* value = nullptr;
    
    bool parse_flag(std::string _flag, int& argc) override {
        if (strcmp(names[0], "") != 0) {
            bool found = false;
            for (auto& flag_name : names) {
                if (strcmp(flag_name, _flag.c_str()) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        } else if (_flag != name) {
            return false;
        } else {
            std::println("name {}, flag {}", name, _flag);
        }

        if (!exists) {
            value = new FlagType;
        }
        exists = true;

        if constexpr (std::is_same_v<FlagType, bool>) {
            *value = true;
            argc -= 1;
        } else if constexpr (std::is_same_v<FlagType, int>) {
            if (current_iteratior+1 >= m_args.size()) {
                std::println(stderr, "flag {} expects to have an integer value after it", name);
                exit(1);
            }
            not_ret.push_back(m_args[current_iteratior]);
            *value = std::stoi(m_args[++current_iteratior]);
            argc -= 2;
        } else if constexpr (std::is_same_v<FlagType, std::string>) {
            if (current_iteratior+1 >= m_args.size()) {
                std::println(stderr, "flag {} expects to have a string value after it", name);
                exit(1);
            }
            not_ret.push_back(m_args[current_iteratior]);
            *value = m_args[++current_iteratior];            
            argc -= 2;
        } else if constexpr (std::is_same_v<FlagType, std::vector<std::string>>) {
            if (current_iteratior+1 >= m_args.size()) {
                std::println(stderr, "flag {} expects to have a string value after it", name);
                exit(1);
            }
            not_ret.push_back(m_args[current_iteratior]);
            value->push_back(m_args[++current_iteratior]);            
            argc -= 2;
        } else {
            std::println(stderr, "not supported flag type");
            exit(1);
        }
        return true;
    }
};
