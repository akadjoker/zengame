
#pragma once

#include "pch.hpp"
#include <raylib.h>
#include "math.hpp"

// ============================================================
// Assets.hpp  —  GraphLib: texture + graph (sprite frame) manager
//
// Concepts:
//   Texture2D  — raw GPU texture (lives in VRAM, shared)
//   Graph      — a named sub-region of a texture with control points
//                graph_id == 0 is always the built-in 32x32 checker (fallback)
//
// Typical usage:
//   GraphLib assets;
//   assets.create();                              // init + default texture
//   int id  = assets.load("player", "player.png");
//   int id2 = assets.loadAtlas("coin", "coin.png", 4, 1);  // 4-frame strip
//   ...
//   assets.destroy();                             // unload all GPU memory
// ============================================================

// ----------------------------------------------------------
// Constants
// ----------------------------------------------------------

constexpr int  MAXNAME      = 64;
constexpr char PAK_MAGIC[4] = {'M','G','P','K'};
constexpr int  PAK_VERSION  = 1;

// Endian helpers (little-endian source files from DIV format)
#if defined(__BIG_ENDIAN__) || defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define ARRANGE_WORD(x)  (*(uint16_t*)(x) = (uint16_t)(((*(uint16_t*)(x) & 0x00FF) << 8) | ((*(uint16_t*)(x) & 0xFF00) >> 8)))
  #define ARRANGE_DWORD(x) (*(uint32_t*)(x) = (uint32_t)(((*(uint32_t*)(x) & 0x000000FF) << 24) | ((*(uint32_t*)(x) & 0x0000FF00) << 8) | ((*(uint32_t*)(x) & 0x00FF0000) >> 8) | ((*(uint32_t*)(x) & 0xFF000000) >> 24)))
#else
  #define ARRANGE_WORD(x)  ((void)0)
  #define ARRANGE_DWORD(x) ((void)0)
#endif

// DIV/FPG format magic numbers
constexpr char FPG_MAGIC[7] = {'f','p','g',0x1a,0x0d,0x0a,0x00};
constexpr char F16_MAGIC[7] = {'f','1','6',0x1a,0x0d,0x0a,0x00};
constexpr char F32_MAGIC[7] = {'f','3','2',0x1a,0x0d,0x0a,0x00};
constexpr char F01_MAGIC[7] = {'f','0','1',0x1a,0x0d,0x0a,0x00};

// ----------------------------------------------------------
// Graph  —  one named sub-region inside a Texture2D
// ----------------------------------------------------------

struct Graph
{
    int            id;               // index in GraphLib::graphs
    int            texture;          // index in GraphLib::textures (shared)
    int            width;
    int            height;
    Rectangle      clip;             // source rect inside the texture
    char           name[MAXNAME];
    std::vector<Vec2> points;        // control points; [0] = default pivot
};

// ----------------------------------------------------------
// PAK binary format headers (for savePak / loadPak)
// ----------------------------------------------------------

#pragma pack(push, 1)

struct PakHeader
{
    char magic[4];
    int  version;
    int  textureCount;
    int  graphCount;
};

struct PakTextureHeader
{
    char name[MAXNAME];
    int  width;
    int  height;
    int  size;   // bytes = width * height * 4  (RGBA8)
};

struct PakGraphHeader
{
    char  name[MAXNAME];
    int   texture;
    float clip_x, clip_y, clip_w, clip_h;
    int   point_count;
};

#pragma pack(pop)

// ----------------------------------------------------------
// GraphLib
// ----------------------------------------------------------

class GraphLib
{
public:

    // ------ Lifecycle ----------------------------------------

    // Must be called before any load*(). Creates the fallback texture (id=0).
    void create();

    // Unloads all GPU textures and clears all graphs.
    void destroy();

    // ------ Loading ------------------------------------------

    // Load a single image file. Returns graph id, or -1 on error.
    int load(const char* name, const char* texturePath);

    // Load an image as a uniform grid atlas.
    // Returns the id of the FIRST sub-graph; ids are contiguous.
    int loadAtlas(const char* name, const char* texturePath,
                  int count_x, int count_y);

    // Load a DIV/FPG packed file (8 / 16 / 32 bpp).
    // Returns number of graphics loaded, or -1 on error.
    int loadDIV(const char* filename);

    // Add a named sub-region of an existing graph (zero-copy — shares texture).
    // Returns new graph id, or -1 on invalid parentId.
    int addSubGraph(int parentId, const char* name,
                    int x, int y, int w, int h);

    // ------ PAK (compiled asset bundle) ----------------------

    bool savePak(const char* pakFile);
    bool loadPak(const char* pakFile);

    // ------ Access -------------------------------------------

    // Returns graph by id. Falls back to id=0 (default checker) on invalid id.
    Graph*     getGraph(int id);

    // Returns Texture2D pointer by raw texture index. Returns nullptr on invalid.
    Texture2D* getTexture(int index);

    // Draw graph directly (no Node2D transform — for debug / HUD use)
    void DrawGraph(int id, float x, float y, Color tint = WHITE);

    // ------ Public data --------------------------------------

    std::vector<Graph>     graphs;
    std::vector<Texture2D> textures;

    static GraphLib* InstancePtr()
    {
        static GraphLib instance;
        return &instance;
    }
    static GraphLib& Instance()
    {
        return *InstancePtr();
    }

private:

    Color   palette[256];
    bool    has_palette = false;

    // Reads a raw 6-bit DIV palette (768 bytes) from fp.
    int  ReadPalette(FILE* fp, Color* out_palette);

    // Like ReadPalette but also skips the 576-byte gamma block.
    int  ReadPaletteWithGamma(FILE* fp, Color* out_palette);
};
