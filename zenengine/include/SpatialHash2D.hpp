#pragma once

#include "pch.hpp"
#include <raylib.h>

struct SpatialBody2D
{
    int      id = -1;
    void*    user_data = nullptr;
    Rectangle aabb = {};
    uint32_t layer = 1u;
    uint32_t mask  = 0xFFFFFFFFu;
};

class SpatialHash2D
{
public:

    explicit SpatialHash2D(float cell_size = 128.0f);

    void  set_cell_size(float cell_size);
    float get_cell_size() const;

    void clear();
    bool upsert(const SpatialBody2D& body);
    bool remove(int id);

    bool get_body(int id, SpatialBody2D& out_body) const;
    void query_aabb(const Rectangle& area, std::vector<int>& out_ids) const;
    void query_pairs(std::vector<std::pair<int, int>>& out_pairs) const;

    void debug_draw_cells(Color color = Color{30, 220, 60, 120}) const;
    void debug_draw_bodies(Color color = Color{250, 200, 40, 255}) const;

private:

    static int64_t pack_cell(int x, int y);
    void raster_aabb(const Rectangle& aabb, std::vector<int64_t>& out_cells) const;

    float m_cell_size;
    std::unordered_map<int, SpatialBody2D> m_bodies;
    std::unordered_map<int64_t, std::vector<int>> m_cells;
};

