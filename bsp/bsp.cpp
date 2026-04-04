#include <cstdio>
#include <print>
#include <vector>
#include "bsp.h"

#include <cstring>
#include <istream>
#include <unordered_set>

std::string mapsurface_s::texpath() const {
    char fpath[sizeof rname + 1];
    std::strncpy(fpath, rname, sizeof fpath);
    fpath[sizeof fpath - 1] = '\0';

    return std::format("textures/{}.wal", fpath);
}

std::expected<bsp_t, bsp_error> bsp_t::load(std::istream &in) {
    bsp_t bsp;

    in.seekg(0, std::ios::end);
    const size_t len = in.tellg();
    in.seekg(0, std::ios::beg);

    if (len < sizeof(dheader_t)) {
        return std::unexpected(bsp_error("bsp file is too small to be valid"));
    }

    bsp.buf.resize(len);
    in.read(reinterpret_cast<char *>(&bsp.buf[0]), len);

    //map header structs onto the buffer
    bsp.header = *reinterpret_cast<dheader_t *>(&bsp.buf[0]);

    //r1: check header pointers point within allocated data
    for (int i = 0; i < HEADER_LUMPS; i++) {
        //for some reason there are unused lumps with invalid values
        if (i == LUMP_POP)
            continue;

        if (bsp.header.lumps[i].fileofs < 0 || bsp.header.lumps[i].length < 0 ||
            bsp.header.lumps[i].fileofs + bsp.header.lumps[i].length > len) {
            return std::unexpected(bsp_error(std::format(
                "{}: lump {} offset {} of size {} is out of bounds\n"
                "map is probably truncated or otherwise corrupted",
                __func__, i, bsp.header.lumps[i].fileofs,
                bsp.header.lumps[i].length)));
        }
    }

    if (bsp.header.version != BSPVERSION) {
        return std::unexpected(bsp_error(std::format(
            "map is not a valid BSP file.")));
    }

    return bsp;
}

// az: cppified from entdump
std::expected<std::string, bsp_error> bsp_t::get_entity_string() {
    auto lump = header.lumps[LUMP_ENTITIES];

    if (lump.length > MAX_MAP_ENTSTRING) {
        return std::unexpected(bsp_error(std::format("Map has too large entity lump {} > {}",
                  lump.length, MAX_MAP_ENTSTRING)));
    }
    if (lump.fileofs + lump.length > buf.size()) {
        return std::unexpected(bsp_error(std::format("Entity lump parameter error in bsp\n"
                  "lump offset {} + length {} exceeds filesize {}\n"
                  "the file is truncated or otherwise corrupted.\n",
                  lump.fileofs, lump.length, buf.size())));
    }

    auto ptr = reinterpret_cast<char *>(&buf[0]) + lump.fileofs;
    std::string ent_string(ptr, lump.length);

    // remove newline at end of lump string if present.
    if (!strcmp(&ent_string[ent_string.size() - 1], "\n"))
        ent_string.resize(ent_string.size() - 1);

    return ent_string;
}

/*
=================
print_bsp_textures
=================
//QW// pulled this from quake2 engine source and modified it
to list textures used and to flag the missing ones.

az: cppified
*/
std::expected<std::vector<mapsurface_t>, bsp_error> bsp_t::get_surfaces() const {
    auto lump = header.lumps[LUMP_TEXINFO];
    std::vector<mapsurface_t> map_surfaces;

    auto in = (texinfo_t *)(buf.data() + lump.fileofs);
    if (lump.length % sizeof(*in)) {
        return std::unexpected(bsp_error("funny lump size"));
    }

    const int count = lump.length / sizeof(*in);
    if (count < 1) {
        return std::unexpected(bsp_error("Map with no surfaces:"));
    }
    if (count > MAX_MAP_TEXINFO) {
        return std::unexpected(bsp_error("Map has too many surfaces"));
    }

    std::unordered_set<std::string> seen;
    for (int i = 0; i < count; i++, in++) {
        std::string texture_name(in->texture, sizeof in->texture);

        if (texture_name.empty())
            continue;

        if (seen.contains(texture_name))
            continue;

        seen.insert(texture_name);

        mapsurface_t surf;
        memcpy(surf.c.name, in->texture, sizeof surf.c.name);
        surf.c.flags = in->flags;
        surf.c.value = in->value;
        memcpy(surf.rname, in->texture, sizeof surf.rname);

        map_surfaces.emplace_back(surf);
    }

    return map_surfaces;
}

std::expected<std::vector<std::string>, bsp_error> bsp_t::get_textures() const {
    std::vector<std::string> textures;
    auto surfs = get_surfaces();
    if (!surfs.has_value()) {
        return std::unexpected(surfs.error());
    }

    for (auto& surf : *surfs) {
        textures.push_back(surf.texpath());
    }
    return textures;
}
