#pragma once

#include "pch.hpp"
#include "Node2D.hpp"
#include <raylib.h>

struct Tile2D
{
    uint16_t id = 0;
    uint8_t solid = 0;
};

class TileMap2D : public Node2D
{
public:
    enum class GridType
    {
        ORTHO = 0,
        HEXAGON = 1,
        ISOMETRIC = 2
    };

    explicit TileMap2D(const std::string& p_name = "TileMap2D");

    int atlas_graph = -1;
    int width = 0;
    int height = 0;
    int tile_width = 16;
    int tile_height = 16;
    int spacing = 0;
    int margin = 0;
    int columns = 1;
    GridType grid_type = GridType::ORTHO;
    float iso_compression = 1.0f;
    Color tint = WHITE;
    bool show_grid = false;
    bool show_ids = false;

    void init(int w, int h, int tile_w, int tile_h, int graph_id);
    void set_tileset_info(int tw, int th, int spacing_px, int margin_px, int cols);
    void clear();
    void set_tile(int gx, int gy, const Tile2D& tile);
    Tile2D* get_tile(int gx, int gy);
    const Tile2D* get_tile(int gx, int gy) const;
    void set_tile_solid_by_id(uint16_t tile_id, bool solid);
    void paint_rect(int cx, int cy, int radius, uint16_t id, bool solid = true);
    void erase_rect(int cx, int cy, int radius);
    void fill(int gx, int gy, uint16_t id, bool solid = true);
    bool load_from_array(const int* data, int count);
    bool load_from_string(const std::string& csv_like, int shift = 0);

    Vec2 grid_to_world(int gx, int gy) const;
    bool world_to_grid(const Vec2& p, int& gx, int& gy) const;
    bool is_solid_world(const Vec2& p) const;
    bool is_solid_cell(int gx, int gy) const;
    Rectangle cell_rect_world(int gx, int gy) const;

    void _draw() override;
    void debug();
    void render_grid();

private:

    std::vector<Tile2D> m_tiles;
};
