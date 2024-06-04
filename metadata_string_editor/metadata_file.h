#pragma once

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "endian.h"

class metadata_file_t {
private:
    uint32_t sanity;
    uint32_t version;
    uint32_t string_literal_offset;  // string data for managed code
    uint32_t string_literal_size;
    uint32_t string_literal_data_offset;
    uint32_t string_literal_data_size;

    std::string string_buffer;
    char* buffer;
    size_t cursor;
    size_t string_literal_data_info_offset;

    std::fstream file;

    bool is_reversed_order;

    template <typename T>
    void read(T& x) {
        std::copy(buffer + cursor, buffer + cursor + sizeof(T), reinterpret_cast<char*>(&x));
        if (is_reversed_order) x = reverse_bytes(x);
        cursor += sizeof(T);
    }

    void read(char* dest, size_t size) {
        std::copy(buffer + cursor, buffer + cursor + size, dest);
        cursor += size;
    }

    void grow_buffer(const size_t& at_least_size) {
        if (string_buffer.size() < at_least_size) {
            string_buffer.resize(at_least_size);
            buffer = &string_buffer[0];  // resize might invalidate the old pointer
        }
    }

    template <typename T>
    void write(T x) {
        if (is_reversed_order) x = reverse_bytes(x);
        grow_buffer(cursor + sizeof(T));
        std::copy(reinterpret_cast<char*>(&x), reinterpret_cast<char*>(&x) + sizeof(T), buffer + cursor);
        cursor += sizeof(T);
    }

    void write(char* source, size_t size) {
        grow_buffer(cursor + sizeof(size));
        std::copy(source, source + size, buffer + cursor);
        cursor += size;
    }

    class string_literal_t {
    public:
        uint32_t length;
        uint32_t offset;
        std::string data;
    };

public:
    std::vector<string_literal_t> string_literals;

    metadata_file_t(const std::string& path) : file(path, std::ios::in | std::ios::binary) {
        {
            file.seekg(0, file.end);
            if (!file) {
                std::cerr << "failed to get file size: " << path << '\n';
                exit(-1);
            }
            string_buffer.resize(file.tellg());
            file.seekg(0, file.beg);
            buffer = &string_buffer[0];
            file.read(buffer, string_buffer.size());
            assert(file);
            file.close();
            cursor = 0;
        }
        is_reversed_order = false;
        read(sanity);  // 0
        is_reversed_order = (sanity != 0xFAB11BAF);
        if (is_reversed_order) sanity = reverse_bytes(sanity);
        assert(sanity == 0xFAB11BAF);

        read(version);                // 4
        read(string_literal_offset);  // 8
        read(string_literal_size);    // c
        assert(string_literal_size % 8 == 0);
        string_literal_data_info_offset = cursor;
        read(string_literal_data_offset);  // 10
        read(string_literal_data_size);    // 14

        string_literals.reserve(string_literal_size / 8);
        string_literals.resize(string_literal_size / 8);
        cursor = string_literal_offset;
        for (auto&& [length, offset, data] : string_literals) {
            read(length);
            read(offset);
            data.resize(length);
        }
        for (auto&& [length, offset, data] : string_literals) {
            cursor = string_literal_data_offset + offset;
            read(&data[0], length);
        }
    }

    std::vector<size_t> search(const std::string& s) {
        // search for strings that match s and then return the index
        std::vector<size_t> result;
        for (size_t i = 0; i < string_literals.size(); i++) {
            auto&& [length, offset, data] = string_literals[i];
            if (data == s) result.push_back(i);
        }
        return result;
    }

    void update(const size_t index, const std::string& value) {
        string_literals[index].data = value;
        string_literals[index].length = value.size();
    }

    const std::string& get(const size_t index) { return string_literals[index].data; }

    void export_to_file(const std::string& path) {
        file = std::fstream(path, std::ios::out | std::ios::binary);
        if (!file) {
            std::cerr << "failed to write to file: " << path << '\n';
            return;
        }

        cursor = string_literal_offset;
        size_t total_size = 0;
        for (size_t i = 0; i < string_literals.size(); i++) {
            auto&& [length, offset, data] = string_literals[i];
            offset = total_size;
            total_size += length;
            write(length);
            write(offset);
        }

        // alignment
        size_t tmp = (string_literal_data_offset + total_size) % 4;
        if (tmp != 0) total_size += 4 - tmp;
        if (total_size > string_literal_data_size) {  // can't grow in place
            if (string_literal_data_offset + string_literal_data_size < string_buffer.size()) {
                // we are not at the end so we move the string value to the end of the metadata
                // this works for the most part, but there will be a chunk of unused data in the middle
                // to resolve that, we would have to understand all the data that is going on and move everything accordingly.
                string_literal_data_offset = string_buffer.size();
            } else {
                // we are at the end of the file already, so we can just directly expand
            }
        }
        string_literal_data_size = total_size;
        cursor = string_literal_data_offset;
        for (size_t i = 0; i < string_literals.size(); i++) {
            auto&& [length, offset, data] = string_literals[i];
            write(&data[0], length);
        }

        cursor = string_literal_data_info_offset;
        write(string_literal_data_offset);
        write(string_literal_data_size);
        std::cerr << string_buffer.size() << '\n';
        file.write(buffer, string_buffer.size());
        if (!file) {
            std::cerr << "failed to write to file: " << path << '\n';
            return;
        }
        file.close();
    }

    void dump_to_text(std::string path) const {
        std::ofstream f(path);
        for (size_t i = 0; i < string_literals.size(); i++) {
            auto&& [length, offset, data] = string_literals[i];
            f << i << '\t' << length << '\t' << data << '\n';
        }
    }
};