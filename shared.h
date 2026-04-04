#pragma once

#include <vector>
#include <filesystem>
#include <map>
#include <unordered_set>
#include <fstream>
#include <unordered_map>

typedef std::unordered_set<std::filesystem::path> filelist_t;

typedef std::map<std::string, std::filesystem::path> file_locations_t;

struct fs_mappings_t {
    filelist_t pak_files;

    file_locations_t pak_file_locations;
    file_locations_t loose_file_locations;

    // missing
    filelist_t missing_files;

    std::string serialized(const std::optional<const std::unordered_map<std::filesystem::path, filelist_t>> &referencers) const;
};

