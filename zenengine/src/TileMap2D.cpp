#include "pch.hpp"
#include "TileMap2D.hpp"
#include "Assets.hpp"
#include "render.hpp"
#include <sstream>

TileMap2D::TileMap2D(const std::string& p_name)
    : Node2D(p_name)
{
}

void TileMap2D::init(int w, int h, int tile_w, int tile_h, int graph_id)
{
    width = std::max(0, w);
    height = std::max(0, h);
    tile_width = std::max(1, tile_w);
    tile_height = std::max(1, tile_h);
    atlas_graph = graph_id;
    m_tiles.assign(static_cast<size_t>(width * height), Tile2D{});
}

void TileMap2D::set_tileset_info(int tw, int th, int spacing_px, int margin_px, int cols)
{
    tile_width = std::max(1, tw);
    tile_height = std::max(1, th);
    spacing = std::max(0, spacing_px);
    margin = std::max(0, margin_px);
    columns = std::max(1, cols);
}

void TileMap2D::clear()
{
    for (Tile2D& t : m_tiles)
    {
        t = Tile2D{};
    }
}

void TileMap2D::set_tile(int gx, int gy, const Tile2D& tile)
{
    if (gx < 0 || gy < 0 || gx >= width || gy >= height)
    {
        return;
    }
    m_tiles[static_cast<size_t>(gy * width + gx)] = tile;
}

Tile2D* TileMap2D::get_tile(int gx, int gy)
{
    if (gx < 0 || gy < 0 || gx >= width || gy >= height)
    {
        return nullptr;
    }
    return &m_tiles[static_cast<size_t>(gy * width + gx)];
}

const Tile2D* TileMap2D::get_tile(int gx, int gy) const
{
    if (gx < 0 || gy < 0 || gx >= width || gy >= height)
    {
        return nullptr;
    }
    return &m_tiles[static_cast<size_t>(gy * width + gx)];
}

void TileMap2D::set_tile_solid_by_id(uint16_t tile_id, bool solid)
{
    for (Tile2D& t : m_tiles)
    {
        if (t.id == tile_id)
        {
            t.solid = solid ? 1 : 0;
        }
    }
}

void TileMap2D::paint_rect(int cx, int cy, int radius, uint16_t id, bool solid)
{
    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            set_tile(cx + dx, cy + dy, Tile2D{id, static_cast<uint8_t>(solid ? 1 : 0)});
        }
    }
}

void TileMap2D::erase_rect(int cx, int cy, int radius)
{
    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            set_tile(cx + dx, cy + dy, Tile2D{0, 0});
        }
    }
}

void TileMap2D::fill(int gx, int gy, uint16_t id, bool solid)
{
    Tile2D* start = get_tile(gx, gy);
    if (!start)
    {
        return;
    }
    const uint16_t old_id = start->id;
    if (old_id == id)
    {
        return;
    }

    struct Cell { int x; int y; };
    std::vector<Cell> stack;
    stack.push_back({gx, gy});

    while (!stack.empty())
    {
        const Cell c = stack.back();
        stack.pop_back();

        Tile2D* t = get_tile(c.x, c.y);
        if (!t || t->id != old_id)
        {
            continue;
        }
        *t = Tile2D{id, static_cast<uint8_t>(solid ? 1 : 0)};

        stack.push_back({c.x + 1, c.y});
        stack.push_back({c.x - 1, c.y});
        stack.push_back({c.x, c.y + 1});
        stack.push_back({c.x, c.y - 1});
    }
}

bool TileMap2D::load_from_array(const int* data, int count)
{
    if (!data || width <= 0 || height <= 0)
    {
        return false;
    }
    const int need = width * height;
    if (count < need)
    {
        return false;
    }
    for (int i = 0; i < need; ++i)
    {
        const int v = data[i];
        m_tiles[(size_t)i].id = static_cast<uint16_t>(std::max(0, v));
        m_tiles[(size_t)i].solid = v > 0 ? 1 : 0;
    }
    return true;
}

bool TileMap2D::load_from_string(const std::string& csv_like, int shift)
{
    if (width <= 0 || height <= 0)
    {
        return false;
    }
    std::stringstream ss(csv_like);
    std::string line;
    int y = 0;

    while (std::getline(ss, line) && y < height)
    {
        std::stringstream ls(line);
        std::string token;
        int x = 0;
        while (std::getline(ls, token, ',') && x < width)
        {
            int v = 0;
            try
            {
                v = std::stoi(token);
            }
            catch (...)
            {
                v = 0;
            }
            v += shift;
            if (v < 0) v = 0;
            set_tile(x, y, Tile2D{static_cast<uint16_t>(v), static_cast<uint8_t>(v > 0 ? 1 : 0)});
            ++x;
        }
        ++y;
    }
    return true;
}

Vec2 TileMap2D::grid_to_world(int gx, int gy) const
{
    Vec2 local;
    switch (grid_type)
    {
        case GridType::ORTHO:
            local = Vec2(static_cast<float>(gx * tile_width),
                         static_cast<float>(gy * tile_height));
            break;

        case GridType::HEXAGON:
        {
            const float offset = (gy % 2) ? (tile_width * 0.5f) : 0.0f;
            local = Vec2(static_cast<float>(gx * tile_width) + offset,
                         static_cast<float>(gy) * (tile_height * 0.75f));
            break;
        }

        case GridType::ISOMETRIC:
        {
            const float half_w = tile_width * 0.5f;
            const float half_h = tile_height * 0.5f;
            local = Vec2((gx - gy) * half_w,
                         (gx + gy) * half_h * iso_compression);
            break;
        }
    }
    return to_global(local);
}

bool TileMap2D::world_to_grid(const Vec2& p, int& gx, int& gy) const
{
    const Vec2 local = to_local(p);
    switch (grid_type)
    {
        case GridType::ORTHO:
            gx = static_cast<int>(std::floor(local.x / static_cast<float>(tile_width)));
            gy = static_cast<int>(std::floor(local.y / static_cast<float>(tile_height)));
            break;

        case GridType::HEXAGON:
        {
            gy = static_cast<int>(std::floor(local.y / (tile_height * 0.75f)));
            const float offset = (gy % 2) ? (tile_width * 0.5f) : 0.0f;
            gx = static_cast<int>(std::floor((local.x - offset) / static_cast<float>(tile_width)));
            break;
        }

        case GridType::ISOMETRIC:
        {
            const float half_w = tile_width * 0.5f;
            const float half_h = tile_height * 0.5f;
            const float gx_f = (local.x / half_w + local.y / (half_h * std::max(0.0001f, iso_compression))) * 0.5f;
            const float gy_f = (local.y / (half_h * std::max(0.0001f, iso_compression)) - local.x / half_w) * 0.5f;
            gx = static_cast<int>(std::floor(gx_f));
            gy = static_cast<int>(std::floor(gy_f));
            break;
        }
    }
    return gx >= 0 && gy >= 0 && gx < width && gy < height;
}

bool TileMap2D::is_solid_world(const Vec2& p) const
{
    int gx = 0;
    int gy = 0;
    if (!world_to_grid(p, gx, gy))
    {
        return false;
    }
    const Tile2D* t = get_tile(gx, gy);
    return t && t->solid != 0;
}

bool TileMap2D::is_solid_cell(int gx, int gy) const
{
    const Tile2D* t = get_tile(gx, gy);
    return t && t->solid != 0;
}

Rectangle TileMap2D::cell_rect_world(int gx, int gy) const
{
    const Vec2 p0 = to_global(Vec2((float)(gx * tile_width), (float)(gy * tile_height)));
    const Vec2 p1 = to_global(Vec2((float)((gx + 1) * tile_width), (float)((gy + 1) * tile_height)));
    const float min_x = std::min(p0.x, p1.x);
    const float min_y = std::min(p0.y, p1.y);
    const float max_x = std::max(p0.x, p1.x);
    const float max_y = std::max(p0.y, p1.y);
    return Rectangle{min_x, min_y, max_x - min_x, max_y - min_y};
}

void TileMap2D::_draw()
{
    if (!visible)
    {
        return;
    }

    if (atlas_graph < 0 || width <= 0 || height <= 0)
    {
        if (show_ids)
        {
            debug();
        }
        return;
    }

    Graph* atlas = GraphLib::Instance().getGraph(atlas_graph);
    if (!atlas)
    {
        if (show_ids)
        {
            debug();
        }
        return;
    }
    Texture2D* tex = GraphLib::Instance().getTexture(atlas->texture);
    if (!tex)
    {
        if (show_ids)
        {
            debug();
        }
        return;
    }

    if (columns <= 0)
    {
        columns = std::max(1, atlas->width / std::max(1, tile_width + spacing));
    }

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const Tile2D& t = m_tiles[static_cast<size_t>(y * width + x)];
            if (t.id == 0)
            {
                continue;
            }

            const int tile_index = static_cast<int>(t.id - 1);
            const int sx = margin + (tile_index % columns) * (tile_width + spacing);
            const int sy = margin + (tile_index / columns) * (tile_height + spacing);

            Rectangle src{
                atlas->clip.x + static_cast<float>(sx),
                atlas->clip.y + static_cast<float>(sy),
                static_cast<float>(tile_width),
                static_cast<float>(tile_height)
            };
            const Vec2 wpos = grid_to_world(x, y);
            const float dx = wpos.x;
            const float dy = wpos.y;

            RenderClipSize(
                *tex,
                dx, dy,
                static_cast<float>(tile_width),
                static_cast<float>(tile_height),
                src,
                false, false,
                tint,
                0
            );
        }
    }

    if (show_grid)
    {
        render_grid();
    }
    if (show_ids)
    {
        debug();
    }
}

void TileMap2D::debug()
{
    const Vec2 origin = get_global_position();
    for (int gy = 0; gy < height; ++gy)
    {
        for (int gx = 0; gx < width; ++gx)
        {
            const Tile2D* t = get_tile(gx, gy);
            if (!t || t->id == 0)
            {
                continue;
            }
            const float x = origin.x + static_cast<float>(gx * tile_width);
            const float y = origin.y + static_cast<float>(gy * tile_height);
            DrawRectangleLines((int)x, (int)y, tile_width, tile_height, GRAY);
            DrawText(TextFormat("%d", t->id), (int)x + 4, (int)y + 4, 10, WHITE);
        }
    }
}

void TileMap2D::render_grid()
{
    const Color c{50, 50, 50, 200};
    if (grid_type == GridType::ORTHO)
    {
        const Vec2 origin = get_global_position();
        const int map_w = width * tile_width;
        const int map_h = height * tile_height;

        for (int x = 0; x <= map_w; x += tile_width)
        {
            DrawLine((int)(origin.x + x), (int)origin.y,
                     (int)(origin.x + x), (int)(origin.y + map_h), c);
        }
        for (int y = 0; y <= map_h; y += tile_height)
        {
            DrawLine((int)origin.x, (int)(origin.y + y),
                     (int)(origin.x + map_w), (int)(origin.y + y), c);
        }
        return;
    }

    for (int gy = 0; gy < height; ++gy)
    {
        for (int gx = 0; gx < width; ++gx)
        {
            Vec2 p = grid_to_world(gx, gy);
            if (grid_type == GridType::ISOMETRIC)
            {
                const float hw = tile_width * 0.5f;
                const float hh = tile_height * 0.5f * iso_compression;
                Vector2 a{p.x, p.y + hh};
                Vector2 b{p.x + hw, p.y};
                Vector2 d{p.x + hw, p.y + 2 * hh};
                Vector2 c2{p.x + 2 * hw, p.y + hh};
                DrawLineV(a, b, c);
                DrawLineV(b, c2, c);
                DrawLineV(c2, d, c);
                DrawLineV(d, a, c);
            }
            else
            {
                const float w = (float)tile_width;
                const float h = (float)tile_height;
                DrawLine((int)p.x, (int)p.y, (int)(p.x + w), (int)p.y, c);
                DrawLine((int)(p.x + w), (int)p.y, (int)(p.x + w), (int)(p.y + h), c);
                DrawLine((int)(p.x + w), (int)(p.y + h), (int)p.x, (int)(p.y + h), c);
                DrawLine((int)p.x, (int)(p.y + h), (int)p.x, (int)p.y, c);
            }
        }
    }
}
