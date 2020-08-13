#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <fstream>
#include "pak.h"

/* this function returns file names in lower case. react accordingly */
std::vector<std::string> get_pak_file_list(std::filesystem::path pak_path);

typedef std::unordered_map<std::string, std::vector<std::string> > files_per_pak_t;
typedef std::map<std::string, std::filesystem::path> file_locations_t;

void read_paks(std::filesystem::path pakdir, files_per_pak_t &files_per_pak, file_locations_t &file_locations) {
    for (auto &pak_path: std::filesystem::directory_iterator(pakdir)) {
        if (pak_path.path().extension() != ".pak")
            continue;

        try {
            auto list = get_pak_file_list(pak_path);
            files_per_pak[pak_path.path().string()] = list;
            for (auto &p: list) {
                file_locations[p] = pak_path;
            }
        } catch (not_a_pak &e) {
            std::cerr << "warn: " << pak_path.path().string() << " is not a pak file.\n";
        }
    }
}

int main(int argc, char* argv[]) {
    std::filesystem::path q2dir, moddir;

    std::cerr << "q2_chextex by zardoru (wyrmin.xyz)\n";

    if (argc == 2) { // q2 path
        q2dir = argv[1];
    } else if (argc >= 3) { // q2 path and mod path
        q2dir = argv[1];
        moddir = q2dir / argv[2];
    } else { // invalid usage
        std::cerr << "usage: q2_chextex q2dir moddir < file_list.txt\n";
        std::cerr << "file_list.txt must contain one filename per line.\n";
        return 1;
    }

    bool q2dir_valid = std::filesystem::is_directory(q2dir);
    if (!q2dir_valid) {
        std::cerr << "error: the q2 path indicated is not a directory. or wasn't found...\n";
        return 1;
    }

    if (!moddir.empty()) {
        if (!std::filesystem::is_directory(moddir)) {
            std::cerr << "error: the mod path indicated is not a directory. or wasn't found...\n";
        }
    }


    std::cerr << "reading pak files...\n";
    files_per_pak_t files_per_pak;
    file_locations_t file_locations;

    std::filesystem::path baseq2dir = q2dir / "baseq2";

    if (!std::filesystem::exists(baseq2dir)) {
        std::cerr << "baseq2 not found in the quake2 dir... aborting.\n";
        return 1;
    }

    // read pak files from q2 dir
    read_paks(baseq2dir, files_per_pak, file_locations);

    // read from mod dir
    if (!moddir.empty()) {
        read_paks(moddir, files_per_pak, file_locations);
    }

    std::cerr << "read " << file_locations.size() << " files from " << files_per_pak.size() << " pak files\n";

    std::unordered_set<std::string> seen_files;

    // now start checking those texture files
    for (std::string line; std::getline(std::cin, line); ) {
        // skip entdump output
        if (line.find("opening ") == 0) {
            continue;
        }

        // empty lines skip them
        if (line.length() == 0)
            continue;

        std::string line_lower = line;
        std::transform(line_lower.begin(), line_lower.end(), line_lower.begin(), tolower);

        // don't check files more than once
        if (seen_files.find(line_lower) != seen_files.end())
            continue;

        seen_files.emplace(line_lower);

        // go and find it, champ
        // not case sensitive here
        auto it = file_locations.find(line_lower);
        if (it != file_locations.end()) {
            std::cout << line << "@" << it->second.string() << std::endl;
        } else {
            // try modpath first. case sensitive depending on OS, so we don't use lower-case version.
            if (!moddir.empty()) {
                std::filesystem::path filepath = moddir / line;
                if (std::filesystem::exists(filepath)) {
                    std::cout << line << "@" << filepath.string() << std::endl;
                    continue;
                }
            }

            // not found. try baseq2
            std::filesystem::path filepath = baseq2dir / line;
            if (std::filesystem::exists(filepath)) {
                std::cout << line << "@" << filepath.string() << std::endl;
            } else {
                std::cout << line << "@" << "NOT FOUND" << std::endl;
            }
        }
    }

    return 0;
}

std::vector<std::string> get_pak_file_list(std::filesystem::path pak_path) {
    std::ifstream pak_in(pak_path, std::ios::in | std::ios::binary);
    pak_header_t header = {0};
    pak_in.read(reinterpret_cast<char*>(&header), sizeof (pak_header_t));

    if (memcmp(header.id, "PACK", 4) != 0)
        throw not_a_pak();

    pak_in.seekg(header.offset, std::ios::beg);

    size_t entry_count = header.size / sizeof(pak_file_t);
    std::vector<std::string> file_list;
    pak_file_t pak_file = {0};

    file_list.reserve(entry_count);

    for (int i = 0; i < entry_count; i++) {
        pak_in.read(reinterpret_cast<char*>(&pak_file), sizeof(pak_file_t));

        std::string lower = pak_file.name;
        std::transform(lower.begin(), lower.end(), lower.begin(), tolower);
        file_list.emplace_back(lower);
    }

    return file_list;
}