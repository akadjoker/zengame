#include "pch.hpp"
#include <unordered_set>
#include "SpatialHash2D.hpp"
#include "Collision2D.hpp"

SpatialHash2D::SpatialHash2D(float cell_size)
    : m_cell_size(std::max(1.0f, cell_size))
{
}

void SpatialHash2D::set_cell_size(float cell_size)
{
    m_cell_size = std::max(1.0f, cell_size);

    std::vector<SpatialBody2D> bodies;
    bodies.reserve(m_bodies.size());
    for (const auto& it : m_bodies)
    {
        bodies.push_back(it.second);
    }

    clear();
    for (const SpatialBody2D& body : bodies)
    {
        upsert(body);
    }
}

float SpatialHash2D::get_cell_size() const
{
    return m_cell_size;
}

void SpatialHash2D::clear()
{
    m_bodies.clear();
    m_cells.clear();
}

bool SpatialHash2D::upsert(const SpatialBody2D& body)
{
    if (body.id < 0)
    {
        return false;
    }

    m_bodies[body.id] = body;
    m_cells.clear();

    std::vector<int64_t> cells;
    for (const auto& it : m_bodies)
    {
        cells.clear();
        raster_aabb(it.second.aabb, cells);
        for (int64_t key : cells)
        {
            m_cells[key].push_back(it.first);
        }
    }

    return true;
}

bool SpatialHash2D::remove(int id)
{
    auto it = m_bodies.find(id);
    if (it == m_bodies.end())
    {
        return false;
    }

    m_bodies.erase(it);
    m_cells.clear();

    std::vector<int64_t> cells;
    for (const auto& kv : m_bodies)
    {
        cells.clear();
        raster_aabb(kv.second.aabb, cells);
        for (int64_t key : cells)
        {
            m_cells[key].push_back(kv.first);
        }
    }

    return true;
}

bool SpatialHash2D::get_body(int id, SpatialBody2D& out_body) const
{
    auto it = m_bodies.find(id);
    if (it == m_bodies.end())
    {
        return false;
    }

    out_body = it->second;
    return true;
}

void SpatialHash2D::query_aabb(const Rectangle& area, std::vector<int>& out_ids) const
{
    out_ids.clear();
    std::vector<int64_t> cells;
    raster_aabb(area, cells);

    std::unordered_set<int> unique_ids;
    for (int64_t key : cells)
    {
        auto it = m_cells.find(key);
        if (it == m_cells.end())
        {
            continue;
        }
        for (int id : it->second)
        {
            unique_ids.insert(id);
        }
    }

    out_ids.reserve(unique_ids.size());
    for (int id : unique_ids)
    {
        out_ids.push_back(id);
    }
}

void SpatialHash2D::query_pairs(std::vector<std::pair<int, int>>& out_pairs) const
{
    out_pairs.clear();
    std::unordered_set<uint64_t> emitted;

    for (const auto& bucket : m_cells)
    {
        const std::vector<int>& ids = bucket.second;
        const size_t n = ids.size();
        for (size_t i = 0; i < n; ++i)
        {
            for (size_t j = i + 1; j < n; ++j)
            {
                int a = ids[i];
                int b = ids[j];
                if (a == b)
                {
                    continue;
                }
                if (a > b)
                {
                    std::swap(a, b);
                }

                const uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(a)) << 32) |
                                     static_cast<uint32_t>(b);
                if (!emitted.insert(key).second)
                {
                    continue;
                }

                const auto ita = m_bodies.find(a);
                const auto itb = m_bodies.find(b);
                if (ita == m_bodies.end() || itb == m_bodies.end())
                {
                    continue;
                }
                if (!Collision2D::CanCollide(ita->second.layer, ita->second.mask, itb->second.layer, itb->second.mask))
                {
                    continue;
                }
                if (!CheckCollisionRecs(ita->second.aabb, itb->second.aabb))
                {
                    continue;
                }

                out_pairs.push_back({a, b});
            }
        }
    }
}

void SpatialHash2D::debug_draw_cells(Color color) const
{
    for (const auto& bucket : m_cells)
    {
        const uint64_t key = static_cast<uint64_t>(bucket.first);
        const int32_t cx = static_cast<int32_t>(key >> 32);
        const int32_t cy = static_cast<int32_t>(key & 0xFFFFFFFFu);
        const float x = static_cast<float>(cx) * m_cell_size;
        const float y = static_cast<float>(cy) * m_cell_size;
        DrawRectangleLines(static_cast<int>(x), static_cast<int>(y),
                           static_cast<int>(m_cell_size), static_cast<int>(m_cell_size), color);
    }
}

void SpatialHash2D::debug_draw_bodies(Color color) const
{
    for (const auto& it : m_bodies)
    {
        const Rectangle& r = it.second.aabb;
        DrawRectangleLines(static_cast<int>(r.x), static_cast<int>(r.y),
                           static_cast<int>(r.width), static_cast<int>(r.height), color);
    }
}

int64_t SpatialHash2D::pack_cell(int x, int y)
{
    const uint64_t ux = static_cast<uint64_t>(static_cast<uint32_t>(x));
    const uint64_t uy = static_cast<uint64_t>(static_cast<uint32_t>(y));
    return static_cast<int64_t>((ux << 32) | uy);
}

void SpatialHash2D::raster_aabb(const Rectangle& aabb, std::vector<int64_t>& out_cells) const
{
    out_cells.clear();

    const int min_x = static_cast<int>(std::floor(aabb.x / m_cell_size));
    const int min_y = static_cast<int>(std::floor(aabb.y / m_cell_size));
    const int max_x = static_cast<int>(std::floor((aabb.x + aabb.width) / m_cell_size));
    const int max_y = static_cast<int>(std::floor((aabb.y + aabb.height) / m_cell_size));

    out_cells.reserve(static_cast<size_t>((max_x - min_x + 1) * (max_y - min_y + 1)));
    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            out_cells.push_back(pack_cell(x, y));
        }
    }
}
