
#include <iostream>

#include "metadata_file.h"
#include "substitution_list.h"


#define STRING_FROM_ARGV(variable)                                             \
    for (int __i = 1; __i + 1 < argc; __i += 2) {                              \
        if (std::string(argv[__i]) == "-" #variable) variable = argv[__i + 1]; \
    }

void panic(const std::string message) {
    std::cerr << "Error: " << message << '\n';
    exit(0);
}

int main(int argc, char** argv) {
    std::string i, o, d, c;
    STRING_FROM_ARGV(i);
    STRING_FROM_ARGV(o);
    STRING_FROM_ARGV(d);
    STRING_FROM_ARGV(c);
    if (i.empty()) {
        panic("no input file");
    }
    if (o.empty()) {
        std::cerr << "default to output file: global-metadata.dat\n";
        o = "global-metadata.dat";
    }
    if ((d.empty()) && (c.empty())) {
        panic("must have -d for direct substitution or -c for config exchange");
    }
    if (!(c.empty() || d.empty())) {
        panic("must not have both -c and -d");
    }
    substitution_list_t substitution_list;
    if (!d.empty()) {
        // direct substitution
        substitution_list.parse_substitution(d);
    } else {
        const std::string old_config_file = argv[argc - 2];
        const std::string new_config_file = argv[argc - 1];
        substitution_list.parse_config_exchange(old_config_file, new_config_file);
    }

    metadata_file_t metadata(i);
    substitution_list.modify(metadata);
    metadata.export_to_file(o);
}