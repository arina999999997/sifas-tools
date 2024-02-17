#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

std::string strip(std::string s) {
    while ((!s.empty()) && isspace(s.back())) s.pop_back();
    size_t pos = 0;
    while ((pos < s.size()) && isspace(s[pos])) pos++;
    return s.substr(pos);
}

std::string remove_space(std::string s) {
    std::string res;
    for (auto&& c : s)
        if (!isspace(c)) res += c;
    return res;
}

std::vector<std::string> split(std::string s, std::string delim) {
    std::vector<std::string> res;
    size_t start = 0;
    while (true) {
        size_t end = s.find(delim, start);
        if (end == s.npos) {
            res.push_back(s.substr(start, end - start));
            break;
        } else {
            res.push_back(s.substr(start, end - start));
            start = end + delim.size();
        }
    }
    return res;
}

std::string byte_to_hex(int byte) {
    static const std::string hex_chars = "0123456789abcdef";
    if (byte < 0) byte += 256;
    std::string res;
    res += hex_chars[byte / 16];
    res += hex_chars[byte % 16];
    return res;
}

std::string to_hex(size_t address) {
    std::string res;
    while (address > 0) {
        res = byte_to_hex(address % 256) + res;
        address /= 256;
    }
    while (res.size() < 8) res = "00" + res;
    return res;
}

void crash(int line_id, const std::string message) {
    std::cerr << "Error at line " << line_id << ": " << message << '\n';
    exit(0);
}

std::map<size_t, char> assign_map;

void assign(size_t address, char byte) {
    if (assign_map.count(address)) {
        std::cerr << "Warning: address " << to_hex(address) << " is overwritten twice, " << byte_to_hex(assign_map[address])
                  << " -> " << byte_to_hex(byte) << '\n';
    }
    assign_map[address] = byte;
}

class value_t {
public:
    std::string bytes;
    const int line_id;

    void crash(std::string message) const { ::crash(line_id, message); }

    value_t(const int line_id, std::string s) : line_id(line_id) {
        if (s.find('"') == s.npos) {  // read as hex
            bytes = parse_hex(s);
        } else {  // qouted string
            bytes = parse_quote(s);
        }
    }

    std::string parse_hex(std::string s) const {
        s = remove_space(s);
        if ((s.find("0x") == 0) || (s.find("0X") == 0)) s = s.substr(2);
        if (s.empty()) crash("empty hex");
        if (s.size() % 2) crash("bad hex (must have even number of hex char)");
        std::string res;
        for (int i = 0; i < s.size(); i += 2) {
            std::string t = s.substr(i, 2);
            char byte = std::stoi(t, nullptr, 16);
            res += byte;
        }
        return res;
    }

    std::string parse_quote(std::string s) const {
        s = strip(s);
        if (s[0] != '"') crash("invalid quote value");
        s = s.substr(1);
        size_t pos = s.rfind('"');
        if (pos == s.npos) crash("invalid quote value");
        std::string quote_type = s.substr(pos + 1);
        s = s.substr(pos);
        pos = s.find("\"\"");
        {
            std::vector<int> qoute_pos;
            while (pos != s.npos) {
                qoute_pos.push_back(pos);
                s = s.replace(pos, 2, " ");
                pos = s.find("\"\"", pos);
            }
            for (auto&& x : qoute_pos) s[x] = '"';
        }
        if (quote_type == "") {  // string, just take thing
            return s;
        }
        crash("invalid qoute type: " + quote_type);
        return "";
    }

    size_t size() const { return bytes.size(); }

    void write_to(size_t start) const {
        for (auto&& x : bytes) assign(start++, x);
    }
};

class address_t {
public:
    enum ADDRESS_TYPE {
        SINGLE,
        LIST,
        SLICE,
    };
    
    const int line_id;
    ADDRESS_TYPE type;
    size_t start;
    size_t end;
    std::vector<address_t> member;

    void crash(std::string message) const { ::crash(line_id, message); }

    size_t read_address(std::string& s) const { return stoull(s, nullptr, 16); }

    address_t(const int line_id, std::string s) : line_id(line_id) {
        s = strip(s);
        if (s[0] == '[') {
            if (s.back() != ']') crash("bad address: " + s);
            s = s.substr(1);
            s.pop_back();
            s = strip(s);
        }
        if (s.empty()) crash("address is empty");
        if (s.find(",") != s.npos) {
            type = LIST;
            auto addresses = split(s, ",");
            for (auto&& address : addresses) {
                member.emplace_back(line_id, address);
            }
        } else if (s.find("..") != s.npos) {
            type = SLICE;
            auto addresses = split(s, "..");
            if (addresses.size() != 2) crash("wrong slice format (need a..b)");
            start = read_address(addresses[0]);
            end = read_address(addresses[1]);
            if (start > end) crash("invalid slice (start > end)");
        } else {
            type = SINGLE;
            start = read_address(s);
        }
    }

    void assign(const value_t& v) const {
        if (type == SINGLE) {
            v.write_to(start);
        } else if (type == SLICE) {
            if (v.size() != 1) crash("assigned non-byte to slice");
            for (int i = start; i <= end; i++) v.write_to(i);
        } else {
            for (auto&& a : member) a.assign(v);
        }
    }
};

class command_t {
public:
    int line_id;
    std::string address;
};

class line_t {
public:
    const int line_id;

    void crash(std::string message) const { ::crash(line_id, message); }

    line_t(const int line_id, std::string content) : line_id(line_id) {
        std::cerr << line_id << ", " << content << '\n';
        size_t pos = content.find("#");
        if (pos != content.npos) content = content.substr(0, pos);
        content = strip(content);
        auto commands = split(content, ";");
        for (auto&& c : commands) {
            c = strip(c);
            if (c.empty()) continue;

            pos = c.find('=');
            if (pos == c.npos) crash("bad command: " + c);
            address_t a(line_id, c.substr(0, pos));
            value_t v(line_id, c.substr(pos + 1));
            a.assign(v);
        }
    }
};

void parse_commands(std::string commands) {
    auto lines = split(commands, "\n");
    std::vector<line_t> v;
    for (int i = 1; i <= lines.size(); i++) v.emplace_back(i, lines[i - 1]);
    // parsing done, now to write the file
}

std::string read_text(std::string file) {
    std::fstream f(file);
    std::string line;
    std::string content;
    while (std::getline(f, line)) {
        content += line;
        content += "\n";
    }
    return content;
}
void write_output(std::string in, std::string out) {
    char* buffer = nullptr;
    size_t length = 0;
    {
        std::fstream f(in, std::ios::in | std::ios::binary);
        if (!f) {
            std::cerr << "Error in reading file!\n";
            exit(-1);
        }
        f.seekg(0, f.end);
        length = f.tellg();
        buffer = new char[length];
        f.seekg(0, f.beg);
        f.read(buffer, length);
        if (!f) {
            std::cerr << "Error in reading file!\n";
            exit(-1);
        }
        f.close();
    }
    for (auto&& [address, byte] : assign_map) {
        assert(address < length);
        buffer[address] = byte;
    }
    {
        std::fstream f(out, std::ios::out | std::ios::binary);
        if (!f) {
            std::cerr << "Error in writing file!\n";
            exit(-1);
        }
        f.write(buffer, length);
        f.close();
    }
}

#define STRING_FROM_ARGV(variable)                                             \
    for (int __i = 1; __i + 1 < argc; __i += 2) {                              \
        if (std::string(argv[__i]) == "-" #variable) variable = argv[__i + 1]; \
    }

int main(int argc, char** argv) {
    std::string i, o, s, c;
    STRING_FROM_ARGV(i);
    STRING_FROM_ARGV(o);
    STRING_FROM_ARGV(s);
    STRING_FROM_ARGV(c);
    if (i.empty()) {
        std::cerr << "No input file provided!";
        return -1;
    }
    if (o.empty()) {
        std::cerr << "No output file provided, writing to input file!\n";
        o = i;
    }
    if (s.empty()) {
        if (c.empty()) {
            std::cerr << "No script of command provided, use -s or -c";
            return -1;
        }
        parse_commands(c);
    } else {
        parse_commands(read_text(s));
    }
    write_output(i, o);
}