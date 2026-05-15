
#include "pch.hpp"
#include "Assets.hpp"

// ============================================================
// Assets.cpp  —  GraphLib implementation
// ============================================================

// ----------------------------------------------------------
// Private helpers
// ----------------------------------------------------------

int GraphLib::ReadPalette(FILE* fp, Color* out_palette)
{
    uint8_t colors[768];
    if (fread(colors, 1, 768, fp) != 768)
        return 0;

    for (int i = 0; i < 256; i++)
    {
        out_palette[i].r = colors[i * 3 + 0] << 2;
        out_palette[i].g = colors[i * 3 + 1] << 2;
        out_palette[i].b = colors[i * 3 + 2] << 2;
        out_palette[i].a = 255;
    }
    return 1;
}

int GraphLib::ReadPaletteWithGamma(FILE* fp, Color* out_palette)
{
    if (!ReadPalette(fp, out_palette))
        return 0;
    fseek(fp, 576, SEEK_CUR);   // skip gamma correction block
    return 1;
}

// ----------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------

void GraphLib::create()
{
    // Slot 0 = 32x32 black-and-white checker (safe fallback)
    Image img = GenImageChecked(32, 32, 4, 4, WHITE, BLACK);
    Texture2D def = LoadTextureFromImage(img);
    UnloadImage(img);

    Graph g       = {};
    g.id          = 0;
    g.texture     = 0;
    g.width       = def.width;
    g.height      = def.height;
    g.clip        = {0, 0, (float)def.width, (float)def.height};
    g.points.push_back({(float)def.width / 2.0f, (float)def.height / 2.0f});
    strncpy(g.name, "default", MAXNAME - 1);

    graphs.push_back(g);
    textures.push_back(def);
}

void GraphLib::destroy()
{
    for (auto& tex : textures)
        UnloadTexture(tex);

    textures.clear();
    graphs.clear();
}

// ----------------------------------------------------------
// Load single image
// ----------------------------------------------------------

int GraphLib::load(const char* name, const char* texturePath)
{
    Image img = LoadImage(texturePath);
    if (img.data == nullptr)
    {
        TraceLog(LOG_ERROR, "GraphLib::load: cannot open '%s'", texturePath);
        return -1;
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);

    Graph g       = {};
    g.id          = (int)graphs.size();
    g.texture     = (int)textures.size();
    g.width       = tex.width;
    g.height      = tex.height;
    g.clip        = {0, 0, (float)tex.width, (float)tex.height};
    g.points.push_back({(float)tex.width / 2.0f, (float)tex.height / 2.0f});
    strncpy(g.name, name, MAXNAME - 1);
    g.name[MAXNAME - 1] = '\0';

    graphs.push_back(g);
    textures.push_back(tex);

    TraceLog(LOG_INFO, "GraphLib::load: '%s' -> id=%d (%dx%d)",
             name, g.id, g.width, g.height);

    return g.id;
}

// ----------------------------------------------------------
// Load atlas (uniform grid)
// ----------------------------------------------------------

int GraphLib::loadAtlas(const char* name, const char* texturePath,
                        int count_x, int count_y)
{
    Image img = LoadImage(texturePath);
    if (img.data == nullptr)
    {
        TraceLog(LOG_ERROR, "GraphLib::loadAtlas: cannot open '%s'", texturePath);
        return -1;
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);

    int tile_w   = tex.width  / count_x;
    int tile_h   = tex.height / count_y;
    int first_id = (int)graphs.size();
    int tex_idx  = (int)textures.size();

    for (int row = 0; row < count_y; row++)
    {
        for (int col = 0; col < count_x; col++)
        {
            Graph g       = {};
            g.id          = (int)graphs.size();
            g.texture     = tex_idx;          // all frames share the same texture
            g.width       = tile_w;
            g.height      = tile_h;
            g.clip        = {(float)(col * tile_w), (float)(row * tile_h),
                             (float)tile_w,          (float)tile_h};
            g.points.push_back({(float)tile_w / 2.0f, (float)tile_h / 2.0f});
            snprintf(g.name, MAXNAME, "%s_%d_%d", name, col, row);
            g.name[MAXNAME - 1] = '\0';

            graphs.push_back(g);
        }
    }

    textures.push_back(tex);

    TraceLog(LOG_INFO, "GraphLib::loadAtlas: '%s' -> first_id=%d (%d frames, %dx%d each)",
             name, first_id, count_x * count_y, tile_w, tile_h);

    return first_id;
}

// ----------------------------------------------------------
// Add sub-graph (zero-copy clip of existing graph)
// ----------------------------------------------------------

int GraphLib::addSubGraph(int parentId, const char* name,
                          int x, int y, int w, int h)
{
    if (parentId < 0 || parentId >= (int)graphs.size())
    {
        TraceLog(LOG_WARNING, "GraphLib::addSubGraph: invalid parentId %d", parentId);
        return -1;
    }

    const Graph& parent = graphs[parentId];

    Graph g    = {};
    g.id       = (int)graphs.size();
    g.texture  = parent.texture;
    g.clip     = {(float)x, (float)y, (float)w, (float)h};
    g.width    = w;
    g.height   = h;
    g.points.push_back({(float)w / 2.0f, (float)h / 2.0f});
    strncpy(g.name, name, MAXNAME - 1);
    g.name[MAXNAME - 1] = '\0';

    graphs.push_back(g);
    return g.id;
}

// ----------------------------------------------------------
// Load DIV / FPG packed file  (8 / 16 / 32 bpp)
// ----------------------------------------------------------

int GraphLib::loadDIV(const char* filename)
{
    has_palette = false;

    FILE* fp = fopen(filename, "rb");
    if (!fp)
    {
        TraceLog(LOG_ERROR, "GraphLib::loadDIV: cannot open '%s'", filename);
        return -1;
    }

    // Validate magic
    char header[8];
    if (fread(header, 1, 8, fp) != 8)
    {
        fclose(fp);
        TraceLog(LOG_ERROR, "GraphLib::loadDIV: cannot read header");
        return -1;
    }

    int bpp = 0;
    if      (memcmp(header, F32_MAGIC, 7) == 0) bpp = 32;
    else if (memcmp(header, F16_MAGIC, 7) == 0) bpp = 16;
    else if (memcmp(header, FPG_MAGIC, 7) == 0) bpp = 8;
    else if (memcmp(header, F01_MAGIC, 7) == 0) bpp = 1;
    else
    {
        fclose(fp);
        TraceLog(LOG_ERROR, "GraphLib::loadDIV: invalid magic in '%s'", filename);
        return -1;
    }

    if (bpp == 8)
    {
        if (!ReadPaletteWithGamma(fp, palette))
        {
            fclose(fp);
            TraceLog(LOG_ERROR, "GraphLib::loadDIV: cannot read palette");
            return -1;
        }
        has_palette     = true;
        palette[0].a    = 0;    // colour index 0 = transparent
    }

    int loaded = 0;

    while (!feof(fp))
    {
        // Chunk header
        struct {
            uint32_t code;
            uint32_t regsize;
            char     name[32];
            char     fpname[12];
            uint32_t width;
            uint32_t height;
            uint32_t flags;   // number of control points
        } chunk;

        if (fread(&chunk, 1, sizeof(chunk), fp) != sizeof(chunk))
            break;

        ARRANGE_DWORD(&chunk.code);
        ARRANGE_DWORD(&chunk.regsize);
        ARRANGE_DWORD(&chunk.width);
        ARRANGE_DWORD(&chunk.height);
        ARRANGE_DWORD(&chunk.flags);

        Graph g       = {};
        g.id          = (int)graphs.size();
        g.texture     = (int)textures.size();
        g.width       = (int)chunk.width;
        g.height      = (int)chunk.height;
        g.clip        = {0, 0, (float)chunk.width, (float)chunk.height};
        strncpy(g.name, chunk.name, MAXNAME - 1);

        // Control points
        int npoints = (int)chunk.flags;
        for (int c = 0; c < npoints; c++)
        {
            int16_t px, py;
            fread(&px, sizeof(int16_t), 1, fp);
            fread(&py, sizeof(int16_t), 1, fp);
            ARRANGE_WORD(&px);
            ARRANGE_WORD(&py);

            if (px == -1 && py == -1)
                g.points.push_back({(float)g.width  / 2.0f,
                                    (float)g.height / 2.0f});
            else
                g.points.push_back({(float)px, (float)py});
        }

        if (g.points.empty())
            g.points.push_back({(float)g.width  / 2.0f,
                                (float)g.height / 2.0f});

        // Pixel data
        int widthb    = (chunk.width * bpp + 7) / 8;
        int pixelSize = 4;

        int format;
        switch (bpp)
        {
            case 32: format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8; pixelSize = 4; break;
            case 16: format = PIXELFORMAT_UNCOMPRESSED_R5G6B5;   pixelSize = 2; break;
            default: format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8; pixelSize = 4; break;
        }

        Image image   = {};
        image.width   = (int)chunk.width;
        image.height  = (int)chunk.height;
        image.format  = format;
        image.mipmaps = 1;

        if (bpp == 8 || bpp == 1)
        {
            image.data = malloc(chunk.width * chunk.height * 4);
            uint8_t* dest = (uint8_t*)image.data;

            for (int row = 0; row < (int)chunk.height; row++)
            {
                uint8_t* line = (uint8_t*)malloc(widthb);
                fread(line, 1, widthb, fp);

                for (int col = 0; col < (int)chunk.width; col++)
                {
                    uint8_t idx;
                    if (bpp == 1)
                    {
                        int byte_pos = col / 8;
                        int bit_pos  = 7 - (col % 8);
                        idx = (~line[byte_pos] >> bit_pos) & 1;
                    }
                    else
                    {
                        idx = line[col];
                    }

                    int dp = (row * chunk.width + col) * 4;
                    if (has_palette)
                    {
                        dest[dp + 0] = palette[idx].r;
                        dest[dp + 1] = palette[idx].g;
                        dest[dp + 2] = palette[idx].b;
                        dest[dp + 3] = palette[idx].a;
                    }
                    else
                    {
                        dest[dp + 0] = idx;
                        dest[dp + 1] = idx;
                        dest[dp + 2] = idx;
                        dest[dp + 3] = 255;
                    }
                }
                free(line);
            }
        }
        else
        {
            image.data = malloc(chunk.width * chunk.height * pixelSize);

            for (int row = 0; row < (int)chunk.height; row++)
            {
                uint8_t* dest = (uint8_t*)image.data + row * chunk.width * pixelSize;
                fread(dest, pixelSize, chunk.width, fp);

                if (bpp == 16)
                {
                    uint16_t* px_row = (uint16_t*)dest;
                    for (int col = 0; col < (int)chunk.width; col++)
                        ARRANGE_WORD(&px_row[col]);
                }
                else if (bpp == 32)
                {
                    uint32_t* px_row = (uint32_t*)dest;
                    for (int col = 0; col < (int)chunk.width; col++)
                        ARRANGE_DWORD(&px_row[col]);
                }
            }
        }

        Texture2D tex = LoadTextureFromImage(image);
        UnloadImage(image);

        graphs.push_back(g);
        textures.push_back(tex);
        loaded++;

        TraceLog(LOG_INFO, "GraphLib::loadDIV: [%u] '%s' (%dx%d)",
                 chunk.code, g.name, g.width, g.height);
    }

    fclose(fp);
    return loaded;
}

// ----------------------------------------------------------
// Access
// ----------------------------------------------------------

Graph* GraphLib::getGraph(int id)
{
    if (graphs.empty())
        return nullptr;
    if (id < 0 || id >= (int)graphs.size())
        return &graphs[0];  // fallback to default checker
    return &graphs[id];
}

Texture2D* GraphLib::getTexture(int index)
{
    if (index < 0 || index >= (int)textures.size())
        return nullptr;
    return &textures[index];
}

void GraphLib::DrawGraph(int id, float x, float y, Color tint)
{
    Graph*     g   = getGraph(id);
    Texture2D* tex = getTexture(g->texture);
    if (tex)
        DrawTextureRec(*tex, g->clip, {x, y}, tint);
}

// ----------------------------------------------------------
// PAK  —  compiled asset bundle
// ----------------------------------------------------------

bool GraphLib::savePak(const char* pakFile)
{
    FILE* f = fopen(pakFile, "wb");
    if (!f)
    {
        TraceLog(LOG_ERROR, "GraphLib::savePak: cannot create '%s'", pakFile);
        return false;
    }

    PakHeader hdr;
    memcpy(hdr.magic, PAK_MAGIC, sizeof(hdr.magic));
    hdr.version      = PAK_VERSION;
    hdr.textureCount = (int)textures.size();
    hdr.graphCount   = (int)graphs.size();

    if (fwrite(&hdr, sizeof(PakHeader), 1, f) != 1)
    {
        fclose(f);
        return false;
    }

    // Write unique textures (read pixels back from VRAM)
    for (size_t i = 0; i < textures.size(); i++)
    {
        Image img = LoadImageFromTexture(textures[i]);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        PakTextureHeader th;
        snprintf(th.name, MAXNAME, "tex_%zu", i);
        th.width  = img.width;
        th.height = img.height;
        th.size   = img.width * img.height * 4;

        if (fwrite(&th, sizeof(PakTextureHeader), 1, f) != 1 ||
            fwrite(img.data, 1, th.size, f)              != (size_t)th.size)
        {
            UnloadImage(img);
            fclose(f);
            return false;
        }
        UnloadImage(img);
    }

    // Write graphs
    for (const auto& g : graphs)
    {
        PakGraphHeader gh;
        strncpy(gh.name, g.name, MAXNAME - 1);
        gh.name[MAXNAME - 1] = '\0';
        gh.texture = g.texture;
        gh.clip_x  = g.clip.x;
        gh.clip_y  = g.clip.y;
        gh.clip_w  = g.clip.width;
        gh.clip_h  = g.clip.height;
        gh.point_count = (int)g.points.size();

        if (fwrite(&gh, sizeof(PakGraphHeader), 1, f) != 1)
        {
            fclose(f);
            return false;
        }

        for (const auto& pt : g.points)
        {
            if (fwrite(&pt, sizeof(Vec2), 1, f) != 1)
            {
                fclose(f);
                return false;
            }
        }
    }

    fclose(f);
    TraceLog(LOG_INFO, "GraphLib::savePak: saved '%s' (%d textures, %d graphs)",
             pakFile, (int)textures.size(), (int)graphs.size());
    return true;
}

bool GraphLib::loadPak(const char* pakFile)
{
    FILE* f = fopen(pakFile, "rb");
    if (!f)
    {
        TraceLog(LOG_ERROR, "GraphLib::loadPak: cannot open '%s'", pakFile);
        return false;
    }

    PakHeader hdr;
    if (fread(&hdr, sizeof(PakHeader), 1, f) != 1 ||
        memcmp(hdr.magic, PAK_MAGIC, sizeof(hdr.magic)) != 0 ||
        hdr.version != PAK_VERSION)
    {
        fclose(f);
        TraceLog(LOG_ERROR, "GraphLib::loadPak: invalid or corrupt '%s'", pakFile);
        return false;
    }

    destroy();  // clear existing assets

    // Load textures
    for (int i = 0; i < hdr.textureCount; i++)
    {
        PakTextureHeader th;
        if (fread(&th, sizeof(PakTextureHeader), 1, f) != 1)
        {
            fclose(f);
            return false;
        }

        std::vector<unsigned char> pixels(th.size);
        if (fread(pixels.data(), 1, th.size, f) != (size_t)th.size)
        {
            fclose(f);
            return false;
        }

        Image img    = {};
        img.data     = pixels.data();
        img.width    = th.width;
        img.height   = th.height;
        img.mipmaps  = 1;
        img.format   = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

        textures.push_back(LoadTextureFromImage(img));
        // pixels freed by vector dtor
    }

    // Load graphs
    for (int i = 0; i < hdr.graphCount; i++)
    {
        PakGraphHeader gh;
        if (fread(&gh, sizeof(PakGraphHeader), 1, f) != 1)
        {
            fclose(f);
            return false;
        }

        Graph g   = {};
        g.id      = (int)graphs.size();
        g.texture = gh.texture;
        g.width   = (int)gh.clip_w;
        g.height  = (int)gh.clip_h;
        g.clip    = {gh.clip_x, gh.clip_y, gh.clip_w, gh.clip_h};
        strncpy(g.name, gh.name, MAXNAME - 1);
        g.name[MAXNAME - 1] = '\0';

        for (int p = 0; p < gh.point_count; p++)
        {
            Vec2 pt;
            if (fread(&pt, sizeof(Vec2), 1, f) != 1)
            {
                fclose(f);
                return false;
            }
            g.points.push_back(pt);
        }

        graphs.push_back(g);
    }

    fclose(f);
    TraceLog(LOG_INFO, "GraphLib::loadPak: loaded '%s' (%d textures, %d graphs)",
             pakFile, (int)textures.size(), (int)graphs.size());
    return true;
}
