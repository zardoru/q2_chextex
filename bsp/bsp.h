// az: this is widely supported.
#pragma once

#include <cstdint>
#include <expected>

class bsp_error;

enum lumptype_t {
    LUMP_ENTITIES = 0,
    LUMP_PLANES = 1,
    LUMP_VERTEXES = 2,
    LUMP_VISIBILITY = 3,
    LUMP_NODES = 4,
    LUMP_TEXINFO = 5,
    LUMP_FACES = 6,
    LUMP_LIGHTING = 7,
    LUMP_LEAFS = 8,
    LUMP_LEAFFACES = 9,
    LUMP_LEAFBRUSHES = 10,
    LUMP_EDGES = 11,
    LUMP_SURFEDGES = 12,
    LUMP_MODELS = 13,
    LUMP_BRUSHES = 14,
    LUMP_BRUSHSIDES = 15,
    LUMP_POP = 16,
    LUMP_AREAS = 17,
    LUMP_AREAPORTALS = 18,
    HEADER_LUMPS = 19
};

// little-endian "IBSP"
constexpr int32_t IDBSPHEADER = (('P' << 24) + ('S' << 16) + ('B' << 8) + 'I');
constexpr int32_t BSPVERSION = 38;

// From qfiles.h
// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
constexpr size_t MAX_MAP_MODELS = 1024;
constexpr size_t MAX_MAP_BRUSHES = 8192;
constexpr size_t MAX_MAP_ENTITIES = 2048;
constexpr size_t MAX_MAP_ENTSTRING = 0x40000; //(262,144)
constexpr size_t MAX_MAP_TEXINFO = 8192;

constexpr size_t MAX_MAP_AREAS = 256;
constexpr size_t MAX_MAP_AREAPORTALS = 1024;
constexpr size_t MAX_MAP_PLANES = 65536;
constexpr size_t MAX_MAP_NODES = 65536;
constexpr size_t MAX_MAP_BRUSHSIDES = 65536;
constexpr size_t MAX_MAP_LEAFS = 65536;
constexpr size_t MAX_MAP_VERTS = 65536;
constexpr size_t MAX_MAP_FACES = 65536;
constexpr size_t MAX_MAP_LEAFFACES = 65536;
constexpr size_t MAX_MAP_LEAFBRUSHES = 65536;
constexpr size_t MAX_MAP_PORTALS = 65536;
constexpr size_t MAX_MAP_EDGES = 128000;
constexpr size_t MAX_MAP_SURFEDGES = 256000;
constexpr size_t MAX_MAP_LIGHTING = 0x200000;
constexpr size_t MAX_MAP_VISIBILITY = 0x100000;

typedef struct lump_s {
    uint32_t fileofs; // file offset of the lump
    uint32_t length; // length of the lump
} lump_t;

typedef struct dheader_s {
    int32_t ident;
    int32_t version;
    lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct texinfo_s {
    float vecs[2][4]; // [s/t][xyz offset]
    int32_t flags; // miptex flags + overrides
    int32_t value; // light emission, etc
    char texture[32]; // texture name (textures/*.wal)
    int32_t nexttexinfo; // for animations, -1 = end of chain
} texinfo_t;

typedef struct csurface_s {
    char name[16];
    int32_t flags;
    int32_t value;
} csurface_t;

typedef struct mapsurface_s // used internally due to name len probs //ZOID
{
    csurface_t c;
    char rname[32];

    std::string texpath() const;
} mapsurface_t;

struct bsp_t {
    dheader_t header;
    std::vector<uint8_t> buf;

    static std::expected<bsp_t, bsp_error> load(std::istream &bsp_path);

    std::expected<std::string, bsp_error> get_entity_string();

    std::expected<std::vector<mapsurface_t>, bsp_error> get_surfaces() const;

    std::expected<std::vector<std::string>, bsp_error> get_textures() const;
};

class bsp_error : public std::runtime_error {
public:
    bsp_error(const std::string &msg) : std::runtime_error(msg) {
    }
};
