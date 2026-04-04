#include "q2fs.h"

#include <fstream>

#include "../bsp/bsp.h"

#include <iostream>
#include <print>
#include <ranges>

#include <boost/algorithm/string.hpp>
#include <boost/json/src.hpp>

bool q2fs_t::resolve_file(fs_mappings_t &final_locations, std::string filename) {
    // go and find it, champ
    // not case sensitive here
    auto it = pak_file_locations.find(filename);
    if (it == pak_file_locations.end()) {
        //
        // try modpath first. case sensitive depending on OS, so we don't use lower-case version.
        if (!moddir.empty()) {
            if (loose_file_locations_mod.contains(filename) && std::filesystem::exists(
                    loose_file_locations_mod[filename])) {
                final_locations.loose_file_locations[filename] = loose_file_locations_mod[filename];
                return true;
            }
        }

        // not found. try baseq2
        if (loose_file_locations_base.contains(filename) &&
            std::filesystem::exists(loose_file_locations_base[filename])) {
            final_locations.loose_file_locations[filename] = loose_file_locations_base[filename];
            return true;
        }

        final_locations.missing_files.insert(filename);
    } else {
        final_locations.pak_file_locations[filename] = it->second;
        return true;
    }
    return false;
}

void q2fs_t::resolve_list(fs_mappings_t &final_locations, const filelist_t &files,
                          std::unordered_set<std::filesystem::path> seen_files) {
    for (const auto &line: files) {
        std::string line_lower = line;
        std::ranges::transform(line_lower, line_lower.begin(), tolower);

        // don't check files more than once
        if (seen_files.contains(line_lower))
            continue;

        seen_files.emplace(line_lower);

        resolve_file(final_locations, line_lower);
    }
}

std::expected<fs_mappings_t, resolution_error> q2fs_t::resolve_mappings(const std::optional<filelist_t> &file_list) {
    std::println(stderr, "reading pak files...\n");
    fs_mappings_t final_locations;

    std::println(stderr, "read {} files from {} pak files\n",
                 pak_file_locations.size(), files_per_pak.size());

    for (const auto &key: pak_file_locations | std::views::keys) {
        final_locations.pak_files.insert(key);
    }

    filelist_t seen_files;
    filelist_t map_files;

    if (file_list.has_value()) {
        resolve_list(final_locations, file_list.value(), seen_files);

        for (const auto &file: file_list.value()) {
            if (file.extension() == ".bsp") {
                map_files.insert(file);
            }
        }
    }

    // now start checking those texture files

    // get all textures (and maybe sounds) from map
    filelist_t textures;
    for (const auto &map: map_files) {
        auto mapfile = final_locations.loose_file_locations.find(map);
        if (mapfile == final_locations.loose_file_locations.end()) {
            if (final_locations.pak_file_locations.contains(map)) {
                continue;
            }

            return std::unexpected(std::format("bsp file {} not found", map.string()));
        }

        std::ifstream in(mapfile->second, std::ios::in);

        if (!in.is_open()) {
            return std::unexpected(std::format("could not open bsp file {}", mapfile->second.string()));
        }

        auto bsp = bsp_t::load(in);
        if (!bsp) {
            return std::unexpected(
                resolution_error(std::format(
                        "could not open bsp file {}: {}",
                        mapfile->second.string(),
                        bsp.error().what())
                )
            );
        }

        auto tex_path = bsp->get_textures();
        if (!tex_path) {
            return std::unexpected(
                resolution_error(std::format(
                    "could not get textures from bsp file {}: {}",
                    mapfile->second.string(),
                    tex_path.error().what()
                ))
            );
        }

        for (const auto &texture: *tex_path) {
            textures.insert(texture);
            file_referencers[texture].insert(mapfile->second.string());
        }
    }

    // resolve all map references
    resolve_list(final_locations, textures, seen_files);

    return final_locations;
}


void q2fs_t::read_paks(const std::filesystem::path &pakdir) {
    for (auto &pak_path: std::filesystem::directory_iterator(pakdir)) {
        if (pak_path.path().extension() != ".pak")
            continue;

        try {
            auto pak = pak_t::load(pak_path);
            files_per_pak[pak_path.path().string()] = pak;
            for (auto &p: pak.files) {
                pak_file_locations[p] = pak_path;
            }
        } catch ([[maybe_unused]] not_a_pak &e) {
            std::println(stderr, "warn: {} is not a pak file.", pak_path.path().string());
        }
    }
}

file_locations_t q2fs_t::read_loose(std::filesystem::path path) {
    file_locations_t locs;
    for (auto &file: std::filesystem::recursive_directory_iterator(path)) {
        if (file.path().extension() == ".pak")
            continue;

        if (file.is_directory())
            continue;

        auto fpath = relative(file, path);
        std::string path = fpath.string();
        std::ranges::transform(path, path.begin(), tolower);

        locs[path] = file.path();
    }

    return locs;
}

void q2fs_t::set_paths(const std::filesystem::path &q2dir, const std::filesystem::path &moddir) {
    this->moddir = moddir;

    files_per_pak.clear();
    pak_file_locations.clear();

    baseq2dir = q2dir / "baseq2";

    if (!std::filesystem::exists(baseq2dir)) {
        throw std::runtime_error("baseq2 not found in the quake2 dir... aborting.");
    }

    read_paks(baseq2dir);
    loose_file_locations_base = read_loose(baseq2dir);

    if (!moddir.empty()) {
        auto final_mod_dir = q2dir / moddir;

        if (!std::filesystem::exists(final_mod_dir)) {
            throw std::runtime_error(std::format("{} not found in the quake2 dir... aborting.",
                                                 final_mod_dir.string()));
        }

        read_paks(q2dir / moddir);
        loose_file_locations_mod = read_loose(q2dir / moddir);
    }
}

boost::json::object serialize_file_locset(const file_locations_t &locset) {
    boost::json::object locset_json;
    for (const auto &loc: locset) {
        locset_json[loc.first] = loc.second.string();
    }
    return locset_json;
}

boost::json::array serialize_filelist(const filelist_t &locset) {
    boost::json::array locset_json;
    for (const auto &loc: locset) {
        locset_json.emplace_back(loc.string());
    }
    return locset_json;
}

std::optional<boost::json::object> serialize_referencers(const filelist_t &files,
                                                         const std::optional<const file_referencers_t> &referencers) {
    if (!referencers.has_value())
        return std::nullopt;

    boost::json::object referencers_json;
    for (const auto &file: files) {
        if (!referencers->contains(file)) {
            std::println(stderr, "warn: file {} not found in referencers", file.string());
            continue;
        }

        auto list = referencers->at(file);
        referencers_json[file.string()] = serialize_filelist(list);
    }
    return referencers_json;
}

std::string fs_mappings_t::serialized(const std::optional<const file_referencers_t> &referencers) const {
    // output the mappings with boost::json
    boost::json::object pak_mappings = serialize_file_locset(pak_file_locations);
    boost::json::object loose_mappings = serialize_file_locset(loose_file_locations);
    boost::json::array missing_mappings = serialize_filelist(missing_files);
    auto missing_referencers = serialize_referencers(missing_files, referencers);

    boost::json::object obj{
        {"pak_files", pak_mappings},
        {"loose_files", loose_mappings},
        {"missing_files", missing_mappings},
    };

    if (missing_referencers.has_value()) {
        obj["missing_referencers"] = missing_referencers.value();
    }

    return boost::json::serialize(obj);
}
