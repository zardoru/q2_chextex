#include <print>
#include <filesystem>

#include <map>
#include <fstream>
#include <iostream>

#include "fs/pak.h"

#include <boost/program_options.hpp>

#include "fs/q2fs.h"

#include <boost/algorithm/string.hpp>

filelist_t make_filelist(std::istream &src) {
    filelist_t filelist;

    for (std::string line; std::getline(src, line);) {
        boost::trim(line);
        boost::to_lower(line);

        // skip entdump output
        if (line.find("opening ") == 0) {
            continue;
        }

        // empty lines skip them
        if (line.empty())
            continue;

        filelist.insert(line);
    }

    return filelist;
}

int main(int argc, char *argv[]) {
    std::filesystem::path q2dir, mod_dir;
    std::vector<std::filesystem::path> map_lists;
    std::optional<std::filesystem::path> out_pak_name;
    std::optional<std::filesystem::path> out_res_name;

    std::println(stderr, "q2_chextex by zardoru (wyrmin.xyz)");
    std::println(stderr, "a tool to verify file integrity and packaging for quake 2 mods");

    auto po = boost::program_options::options_description("options");
    po.add_options()
            ("help,h", "show this help message and exit")
            ("version,v", "show version information and exit")
            ("q2dir,q2", boost::program_options::value<std::filesystem::path>(), "path to q2 directory")
            ("moddir,mod", boost::program_options::value<std::filesystem::path>(), "path to mod directory")
            ("file-list,l", boost::program_options::value<std::filesystem::path>(),
             "path to file list text file to verify (newline separated)")
            ("map-lists,m", boost::program_options::value<std::vector<std::filesystem::path>>()->multitoken(),
                "paths to map lists to verify (newline separated), no maps/ prefix")
            ("out-pak", boost::program_options::value<std::filesystem::path>(),
             "path to output pak file with loose files")
            ("out-resolutions", boost::program_options::value<std::filesystem::path>(), "path to output resolved file list")
    ;


    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, po), vm);
        boost::program_options::notify(vm);
    } catch (boost::program_options::error &e) {
        std::println(stderr, "error: {}", e.what());
        return 1;
    }

    if (vm.contains("help")) {
        std::cerr << po << "\n";
        return 0;
    }

    if (vm.contains("version")) {
        std::println(stderr, "q2_chextex version 1.0");
        return 0;
    }

    std::optional<std::ifstream> filelist_file;
    if (vm.contains("file-list")) {
        filelist_file = std::ifstream(vm["file-list"].as<std::filesystem::path>());
        if (!filelist_file.value().is_open()) {
            std::println(stderr, "error: file list file not found or could not be opened.");
            return 1;
        }
    }

    if (vm.contains("q2dir")) {
        q2dir = vm["q2dir"].as<std::filesystem::path>();
        if (vm.contains("moddir")) {
            mod_dir = vm["moddir"].as<std::filesystem::path>();
        }
    } else {
        std::println(stderr, "error: q2dir not specified.");
        return 1;
    }

    bool q2dir_valid = std::filesystem::is_directory(q2dir);
    if (!q2dir_valid) {
        std::println(stderr, "error: the q2 path indicated is not a directory. or wasn't found...");
        return 1;
    }

    if (!mod_dir.empty()) {
        if (!std::filesystem::is_directory(mod_dir)) {
            std::println(stderr, "error: the mod path indicated is not a directory. or wasn't found...");
            return 1;
        }
    }

    if (vm.contains("map-lists")) {
        map_lists.append_range(vm["map-lists"].as<std::vector<std::filesystem::path>>());
    }

    if (vm.contains("out-pak")) {
        out_pak_name = vm["out-pak"].as<std::filesystem::path>();
    }

    if (vm.contains("out-resolutions")) {
        out_res_name = vm["out-resolutions"].as<std::filesystem::path>();
    }

    try {
        q2fs_t fs;
        filelist_t filelist;

        fs.set_paths(q2dir, mod_dir);

        //
        if (!map_lists.empty()) {
            for (const auto &map_list: map_lists) {
                std::ifstream map_list_in(map_list, std::ios::in);
                if (!map_list_in.is_open()) {
                    std::println(stderr, "error: map list file {} not found or could not be opened.",
                                 map_list.string());
                    return 1;
                }
                auto _maps = make_filelist(map_list_in);

                filelist.insert_range(_maps);
            }
        }

        if (filelist_file.has_value()) {
            auto input_resolve_requests = make_filelist(filelist_file.value());
            filelist.insert_range(input_resolve_requests);
        }

        auto mappings = fs.resolve_mappings(filelist);
        if (!mappings) {
            std::println(stderr, "error: {}", mappings.error().what());
            return 1;
        }

        if (out_res_name.has_value()) {
            std::ofstream out(out_res_name.value(), std::ios::out);

            if (!out.is_open()) {
                std::println(stderr, "error: could not open output file {}", out_res_name.value().string());
                return 1;
            }

            out << mappings->serialized();
        }


        if (out_pak_name.has_value() && !mappings->loose_file_locations.empty()) {
            auto pak = pak_t::make_pak(mappings->loose_file_locations);
            std::ofstream out(out_pak_name.value(), std::ios::out | std::ios::binary);

            if (!out.is_open()) {
                std::println(stderr, "error: could not open output pak file {}", out_pak_name.value().string());
                return 1;
            }

            pak.write(mappings->loose_file_locations, out);
        }
    } catch (std::exception &e) {
        // std::expected doesn't cover _all_ the bases
        std::println(stderr, "error: {}", e.what());
        return 1;
    }

    return 0;
}
