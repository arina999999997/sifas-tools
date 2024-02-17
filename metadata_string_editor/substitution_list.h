#pragma once

#include <iostream>
#include <map>
#include <sstream>

#include "metadata_file.h"

static std::string read_until_separator(std::ifstream& f, const std::string& separator) {
    std::string result;
    std::string line;
    bool good = false;
    while (getline(f, line)) {
        std::cerr << "Parsing line: " << line << '\n';
        if ((line.size() >= separator.size()) && (line.substr(line.size() - separator.size(), separator.size()) == separator)) {
            // end
            result += line.substr(0, line.size() - separator.size());
            good = true;
            break;
        } else {  // this is not the end, get the line + the LF
            result += line;
            result += '\n';
        }
    }
    if (!good) {
        std::cerr << "unclosed separator: " << separator << '\n';
    }
    return result;
}

class substitution_t {
public:
    bool is_good;
    bool is_id;
    std::size_t id;
    std::string name;
    std::string original;
    std::string replaced;
    std::string separator;

    substitution_t(){}
    
    substitution_t(std::ifstream& f)
            : is_good(false), is_id(false), id(0), name(""), original(""), replaced(""), separator("") {
        std::string s;
        while (getline(f, s)) {
            if (s.empty()) continue;
            else break;
        }
        if (s.empty()) return;
        std::cerr << "parsing line: " << s << '\n';
        if (s.substr(0, 2) == "id") {
            is_id = true;
        } else {
            assert(s.substr(0, 3) == "str");
            is_id = false;
        }
        if (is_id) {
            std::stringstream ss(s);
            ss >> s >> id >> separator;
        } else {
            std::stringstream ss(s);
            ss >> s >> separator;
            original = read_until_separator(f, separator);
        }
        replaced = read_until_separator(f, separator);
        is_good = true;
    }

    substitution_t(std::ifstream& f, bool is_original)
            : is_good(false), is_id(false), id(0), name(""), original(""), replaced(""), separator("") {
        std::string s;
        while (getline(f, s)) {
            if (s.empty()) continue;
            else break;
        }
        if (s.empty()) return;
        std::cerr << "parsing line: " << s << '\n';
        std::stringstream ss(s);
        ss >> name;
        if (is_original) {
            std::string type;
            ss >> type;
            if (type == "str") {  // string, need to read the original value
                ss >> separator;
                original = read_until_separator(f, separator);
            } else {
                assert(type == "id");
                ss >> id;
            }
        } else {
            ss >> separator;
            replaced = read_until_separator(f, separator);
        }
        is_good = true;
    }

    substitution_t(const substitution_t& old_config, const substitution_t* new_config) {
        (*this) = old_config;
        is_good = new_config != nullptr;
        if (is_good) {
            assert(new_config->name == old_config.name);
            replaced = new_config->replaced;
        }
    }

    operator bool() const { return is_good; }

    void modify(metadata_file_t& file) {
        std::vector<size_t> ids;
        if (is_id) ids.push_back(id);
        else ids = file.search(original);
        if (ids.empty()) {
            std::cerr << "Error: string isn't in metadata file: " << original << '\n';
            exit(-1);
        }
        if (!name.empty()) std::cerr << "Name: " << name << ", ";
        original = file.get(ids[0]);
        std::cerr << "Original: " << original;
        if (is_id) {
            std::cerr << ", Id: " << id << '\n';
        } else {
            std::cerr << ", Found ids:\n";
            for (auto&& id : ids) {
                std::cerr << id << ' ';
            }
            std::cerr << '\n';
        }

        if (!is_good) {
            // this is a config file run
            std::cerr << "Keep original value\n";
        } else {
            for (auto&& id : ids) {
                file.update(id, replaced);
            }
        }
    }
};

class substitution_list_t {
public:
    std::vector<substitution_t> items;

    void parse_substitution(const std::string& file) {
        std::ifstream f(file);
        while (true) {
            substitution_t s(f);
            if (s) items.push_back(s);
            else break;
        }
    }

    void parse_config_exchange(const std::string& old_file, const std::string& new_file) {
        std::map<std::string, substitution_t> original_configs;
        std::map<std::string, substitution_t> replaced_configs;

        std::ifstream of(old_file);
        while (true) {
            substitution_t s(of, true);
            if (s) {
                assert(original_configs.count(s.name) == 0);
                original_configs[s.name] = s;
            } else {
                break;
            }
        }
        std::ifstream nf(new_file);
        while (true) {
            substitution_t s(nf, false);
            if (s) {
                assert(replaced_configs.count(s.name) == 0);
                assert(original_configs.count(s.name));
                replaced_configs[s.name] = s;
            } else {
                break;
            }
        }
        for (auto&& [name, config] : original_configs) {
            auto it = replaced_configs.find(name);
            if (it != replaced_configs.end()) items.emplace_back(config, &it->second);
            else items.emplace_back(config, nullptr);
        }
    }

    void modify(metadata_file_t& file) {
        for (auto&& s : items) {
            s.modify(file);
        }
    }
};