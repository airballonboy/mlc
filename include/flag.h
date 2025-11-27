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
            for (auto& flag : flags) {
                if(flag->parse_flag(m_args[current_iteratior], argc)) { 
                    not_ret.push_back(m_args[current_iteratior]);
                    break;
                }
            }
            current_iteratior++;
        }
        for (auto& a : m_args) {
            for (auto& b : not_ret) {
                if (a == b) goto cont;
            }
            ret.push_back(a);
            cont: // continue
        }
        return ret;
    }
    virtual bool parse_flag(std::string _flag, int& argc) { return false; };

    std::string_view name;
    std::string_view desc;
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
    Flag(std::string_view _name, std::string_view _desc) {
        name = _name;
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
        if (_flag != name) return false;
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
