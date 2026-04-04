//
// Created by Diego Ahumada on 03-04-26.
//

#pragma once
#include <expected>
#include <filesystem>
#include <map>
#include <optional>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include "pak.h"
#include "../shared.h"

struct pak_t;

/* this function returns file names in lower case. react accordingly */
typedef std::unordered_map<std::string, pak_t> files_per_pak_t;

class resolution_error : public std::runtime_error {
public:
    resolution_error(const std::string& message) : std::runtime_error(message) {}
};

typedef std::unordered_map<std::filesystem::path, filelist_t> file_referencers_t;

struct q2fs_t {
    std::filesystem::path moddir;

    files_per_pak_t files_per_pak;
    file_locations_t pak_file_locations;
    file_locations_t loose_file_locations_mod;
    file_locations_t loose_file_locations_base;

    file_referencers_t file_referencers;

    std::vector<pak_t> pak_files;
    std::filesystem::path baseq2dir;

    void read_paks(const std::filesystem::path &pakdir);

    static file_locations_t read_loose(std::filesystem::path path);

    void set_paths(const std::filesystem::path &q2dir, const std::filesystem::path &moddir);

    bool resolve_file(fs_mappings_t& final_locations, std::string filename);

    void resolve_list(fs_mappings_t &final_locations, const filelist_t &files, std::unordered_set<std::filesystem::path> seen_files);

    std::expected<fs_mappings_t, resolution_error> resolve_mappings(const std::optional<filelist_t> &file_list);

};