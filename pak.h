#ifndef Q2_CHEXTEX_PAK_H
#define Q2_CHEXTEX_PAK_H

#include <vector>
#include <string>
#include <filesystem>

// src: https://quakewiki.org/wiki/.pak
struct pak_header_t
{
    char id[4];
    int offset;
    int size;
};

struct pak_file_t
{
    char name[56];
    int offset;
    int size;
};

class not_a_pak : std::exception {};

#endif //Q2_CHEXTEX_PAK_H
