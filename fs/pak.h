#pragma once

#include "../shared.h"

// src: https://quakewiki.org/wiki/.pak
struct pak_header_t
{
    char id[4];
    int offset;
    int size;

    pak_header_t();
};

struct pak_file_t
{
    char name[56];
    int offset;
    int size;

    pak_file_t();

    operator std::string() const;
};

class pak_stream_t : public std::streambuf {
public:
    pak_stream_t(pak_file_t file, const std::filesystem::path& pak_path);

private:
    std::ifstream in;
    pak_file_t file;
    char buf[1024]{};
    off_t last_offset;
    off_t end;
    size_t advance;

protected:
    int_type underflow();
};

struct pak_t
{
    pak_header_t header = {};
    std::vector<pak_file_t> files;

    static pak_t load(std::filesystem::path pak_path);

    static pak_t make_pak(const file_locations_t& files);

    void write(const file_locations_t &map, std::ostream &out) const;
};

class not_a_pak : public std::exception {};
