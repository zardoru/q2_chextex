#include "pak.h"

#include <cstring>
#include <format>
#include <fstream>
#include <iostream>

pak_header_t::pak_header_t() {
    memcpy(id, "PACK", 4);
    offset = 0;
    size = 0;
}

pak_file_t::pak_file_t() {
    memset(this, 0, sizeof(pak_file_t));
}

pak_file_t::operator std::string() const {
    return std::string(name);
}

pak_stream_t::pak_stream_t(pak_file_t file, const std::filesystem::path& pak_path) :
    in(pak_path, std::ios::in | std::ios::binary),
    file(file)
{
    in.seekg(file.offset, std::ios::beg);

    setg(
        buf,
        buf,
        std::min(buf + file.size, buf + sizeof buf)
    );

    last_offset = file.offset;
    end = last_offset + file.size;

    advance = std::min(static_cast<size_t>(file.size), sizeof buf);
    in.read(buf, advance);

    last_offset += advance;
}

std::streambuf::int_type pak_stream_t::underflow() {
    if (gptr() < egptr()) {
        return traits_type::to_int_type(*gptr());
    }

    advance = std::min(static_cast<size_t>(file.size - last_offset), sizeof buf);
    if (last_offset < end) {
        in.read(buf, advance);
        last_offset += advance;
        setg(buf, buf, std::min(buf + advance, buf + sizeof buf));
        return traits_type::to_int_type(*gptr());
    }

    return traits_type::eof();
}


pak_t pak_t::load(std::filesystem::path pak_path) {
    std::ifstream pak_in(pak_path, std::ios::in | std::ios::binary);
    pak_t pak;
    pak_in.read(reinterpret_cast<char *>(&pak.header), sizeof pak.header);

    if (memcmp(pak.header.id, "PACK", 4) != 0)
        throw not_a_pak();

    pak_in.seekg(pak.header.offset, std::ios::beg);

    size_t entry_count = pak.header.size / sizeof(pak_file_t);
    std::vector<pak_file_t> file_list;
    pak_file_t pak_file = {};

    file_list.reserve(entry_count);

    for (size_t i = 0; i < entry_count; i++) {
        pak_in.read(reinterpret_cast<char *>(&pak_file), sizeof pak_file);

        for (char &j: pak_file.name) {
            j = static_cast<char>(tolower(j));
        }

        file_list.push_back(pak_file);
    }

    pak.files = file_list;

    return pak;
}

pak_t pak_t::make_pak(const file_locations_t &files) {
    pak_t pak;

    auto offs = sizeof(pak_header_t) + sizeof(pak_file_t) * files.size();
    pak.header.offset = sizeof(pak_header_t);
    pak.header.size = files.size() * sizeof(pak_file_t);

    for (const auto &[fs_name, phys_path]: files) {
        pak_file_t pak_file;

        strncpy(pak_file.name, fs_name.c_str(), sizeof(pak_file.name) - 1);
        pak_file.name[sizeof(pak_file.name) - 1] = '\0';
        pak_file.offset = offs;

        auto size = std::filesystem::file_size(phys_path);
        pak_file.size = size;
        offs += size;

        pak.files.push_back(pak_file);
    }

    return pak;
}


void pak_t::write(const file_locations_t& map, std::ostream &out) const {
    out.write(reinterpret_cast<const char *>(&header), sizeof(pak_header_t));

    for (const auto &file: files) {
        if (auto it = map.find(file); it == map.end()) {
            std::println(std::cerr, "warn: file {} not found in map", file.name);
            continue;
        }

        out.write(reinterpret_cast<const char *>(&file), sizeof(pak_file_t));
    }

    for (const auto &file: files) {
        auto it = map.find(file);
        if (it == map.end()) {
            continue;
        }
        std::ifstream in(it->second, std::ios::in | std::ios::binary);
        if (!in.is_open()) {
            throw std::runtime_error(std::format("could not open '{}'", it->second.string()));
        }

        out << in.rdbuf();
        in.close();
    }
}
