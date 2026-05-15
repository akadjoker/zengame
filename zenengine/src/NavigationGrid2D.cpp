#include "NavigationGrid2D.hpp"
#include "TileMap2D.hpp"
#include "SceneTree.hpp"

// ─── Direcções: cardinal primeiro (i<4), depois diagonal (i>=4) ───────────────
const int NavigationGrid2D::DX[8] = {  0, 1,  0, -1, -1,  1, 1, -1 };
const int NavigationGrid2D::DY[8] = { -1, 0,  1,  0, -1, -1, 1,  1 };

static constexpr float SQRT2 = 1.41421356f;

// ═════════════════════════════════════════════════════════════════════════════
//  OpenListHeap
// ═════════════════════════════════════════════════════════════════════════════

void NavigationGrid2D::OpenListHeap::init(int capacity, GridNode* n)
{
    nodes = n;
    pos_.assign(capacity, -1);
    heap_.clear();
    heap_.reserve(capacity);
}

void NavigationGrid2D::OpenListHeap::clear()
{
    for (int i : heap_) pos_[i] = -1;
    heap_.clear();
}

bool NavigationGrid2D::OpenListHeap::better(int a, int b) const
{
    const float fa = nodes[a].f;
    const float fb = nodes[b].f;
    if (fa != fb) return fa < fb;
    return nodes[a].h < nodes[b].h;   // tie-break: menor h primeiro
}

void NavigationGrid2D::OpenListHeap::swap_at(int a, int b)
{
    std::swap(heap_[a], heap_[b]);
    pos_[heap_[a]] = a;
    pos_[heap_[b]] = b;
}

void NavigationGrid2D::OpenListHeap::bubble_up(int i)
{
    while (i > 0)
    {
        const int p = (i - 1) >> 1;
        if (!better(heap_[i], heap_[p])) break;
        swap_at(i, p);
        i = p;
    }
}

void NavigationGrid2D::OpenListHeap::bubble_down(int i)
{
    const int n = (int)heap_.size();
    while (true)
    {
        int best = i;
        const int l = (i << 1) + 1;
        const int r = l + 1;
        if (l < n && better(heap_[l], heap_[best])) best = l;
        if (r < n && better(heap_[r], heap_[best])) best = r;
        if (best == i) break;
        swap_at(i, best);
        i = best;
    }
}

void NavigationGrid2D::OpenListHeap::push(int node_idx)
{
    assert(pos_[node_idx] < 0 && "nó já está no heap");
    pos_[node_idx] = (int)heap_.size();
    heap_.push_back(node_idx);
    bubble_up((int)heap_.size() - 1);
}

int NavigationGrid2D::OpenListHeap::pop()
{
    assert(!heap_.empty());
    const int result = heap_[0];
    pos_[result] = -1;

    if (heap_.size() == 1)
    {
        heap_.clear();
        return result;
    }

    heap_[0] = heap_.back();
    heap_.pop_back();
    pos_[heap_[0]] = 0;
    bubble_down(0);
    return result;
}

void NavigationGrid2D::OpenListHeap::update(int node_idx)
{
    const int p = pos_[node_idx];
    assert(p >= 0 && "update de nó fora do heap");
    bubble_up(p);
    bubble_down(pos_[node_idx]);
}

// ═════════════════════════════════════════════════════════════════════════════
//  NavigationGrid2D — lifecycle
// ═════════════════════════════════════════════════════════════════════════════

NavigationGrid2D::NavigationGrid2D(const std::string& p_name)
    : Node2D(p_name)
{
    node_type = NodeType::NavigationGrid2D;
}

NavigationGrid2D::~NavigationGrid2D()
{
    delete[] grid_;
}

void NavigationGrid2D::init(int width, int height, int resolution)
{
    assert(width > 0 && height > 0 && resolution > 0);

    delete[] grid_;

    w_   = width;
    h_   = height;
    res_ = resolution;
    gen_ = 0;

    grid_ = new GridNode[w_ * h_]();   // zero-init
    open_.init(w_ * h_, grid_);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Grid
// ═════════════════════════════════════════════════════════════════════════════

void NavigationGrid2D::set_cell_solid(int x, int y, bool solid)
{
    if (!grid_ || !in_bounds(x, y)) return;
    grid_[cell_idx(x, y)].walkable = solid ? 0 : 1;
}

bool NavigationGrid2D::is_cell_solid(int x, int y) const
{
    if (!grid_ || !in_bounds(x, y)) return false;
    return grid_[cell_idx(x, y)].walkable == 0;
}

void NavigationGrid2D::clear_grid()
{
    if (!grid_) return;
    for (int i = 0; i < w_ * h_; ++i) grid_[i].walkable = 1;
}

bool NavigationGrid2D::is_walkable_cell(int x, int y) const
{
    if (!in_bounds(x, y)) return false;
    return grid_[cell_idx(x, y)].walkable == 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Coordenadas
// ═════════════════════════════════════════════════════════════════════════════

void NavigationGrid2D::world_to_grid(float wx, float wy, int& gx, int& gy) const
{
    gx = (int)(wx / (float)res_);
    gy = (int)(wy / (float)res_);
}

Vector2 NavigationGrid2D::grid_to_world(int gx, int gy) const
{
    return { (gx + 0.5f) * (float)res_, (gy + 0.5f) * (float)res_ };
}

// ═════════════════════════════════════════════════════════════════════════════
//  Sync from TileMap
// ═════════════════════════════════════════════════════════════════════════════

void NavigationGrid2D::sync_from_tilemap(const TileMap2D* tilemap)
{
    if (!tilemap || !grid_) return;

    for (int y = 0; y < tilemap->height; ++y)
    {
        for (int x = 0; x < tilemap->width; ++x)
        {
            if (in_bounds(x, y))
                grid_[cell_idx(x, y)].walkable = tilemap->is_solid_cell(x, y) ? 0 : 1;
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Load from image
//   Preto (brightness < threshold) → livre
//   Outra cor                      → sólido
// ═════════════════════════════════════════════════════════════════════════════

void NavigationGrid2D::load_from_image(const char* path, int threshold)
{
    if (!grid_) return;

    Image img = LoadImage(path);
    if (img.data == nullptr)
    {
        TraceLog(LOG_ERROR, "NavigationGrid2D::load_from_image: cannot open '%s'", path);
        return;
    }

    // Redimensiona para w_ × h_ (a grelha define a resolução)
    ImageResize(&img, w_, h_);

    Color* px = LoadImageColors(img);

    for (int y = 0; y < h_; ++y)
    {
        for (int x = 0; x < w_; ++x)
        {
            const Color& c = px[y * w_ + x];
            // brightness média simples
            const int brightness = ((int)c.r + (int)c.g + (int)c.b) / 3;
            grid_[cell_idx(x, y)].walkable = (brightness < threshold) ? 1 : 0;
        }
    }

    UnloadImageColors(px);
    UnloadImage(img);

    TraceLog(LOG_INFO, "NavigationGrid2D::load_from_image: '%s' (%dx%d, threshold=%d)",
             path, w_, h_, threshold);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Pathfinding — helpers
// ═════════════════════════════════════════════════════════════════════════════

void NavigationGrid2D::touch(int i)
{
    GridNode& n = grid_[i];
    if (n.stamp != gen_)
    {
        n.stamp  = gen_;
        n.f      = 0.0f;
        n.g      = 0.0f;
        n.h      = 0.0f;
        n.parent = -1;
        n.state  = NodeState::Unvisited;
    }
}

float NavigationGrid2D::calc_h(int dx, int dy, PathHeuristic heur, bool diag) const
{
    switch (heur)
    {
    case PathHeuristic::Euclidean:
        return std::sqrt((float)(dx * dx + dy * dy));

    case PathHeuristic::Chebyshev:
        return (float)std::max(dx, dy);

    case PathHeuristic::Octile:
    {
        const float F = SQRT2 - 1.0f;
        return (dx < dy) ? F * (float)dx + (float)dy
                         : F * (float)dy + (float)dx;
    }

    case PathHeuristic::Manhattan:
    default:
        if (diag)
        {
            const float F = SQRT2 - 1.0f;
            return (dx < dy) ? F * (float)dx + (float)dy
                             : F * (float)dy + (float)dx;
        }
        return (float)(dx + dy);
    }
}

bool NavigationGrid2D::valid_diagonal(int cx, int cy, int nx, int ny) const
{
    if (nx != cx && ny != cy)
        return is_walkable_cell(cx, ny) && is_walkable_cell(nx, cy);
    return true;
}

bool NavigationGrid2D::has_los(int x0, int y0, int x1, int y1) const
{
    const int dx  = std::abs(x1 - x0);
    const int dy  = std::abs(y1 - y0);
    const int sx  = x0 < x1 ? 1 : -1;
    const int sy  = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        if (!is_walkable_cell(x0, y0)) return false;
        if (x0 == x1 && y0 == y1) break;

        const int e2 = err * 2;   // NOTE: não usar << — err pode ser negativo (UB)
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  A* / Dijkstra
// ─────────────────────────────────────────────────────────────────────────────

bool NavigationGrid2D::run_search(int sx, int sy, int ex, int ey,
                                   bool diag, PathAlgorithm algo, PathHeuristic heur,
                                   std::vector<Vector2>& out)
{
    out.clear();
    if (!grid_) return false;
    if (!is_walkable_cell(sx, sy) || !is_walkable_cell(ex, ey)) return false;

    ++gen_;
    open_.clear();

    const int start = cell_idx(sx, sy);
    const int end   = cell_idx(ex, ey);

    touch(start);
    GridNode& s = grid_[start];
    s.g     = 0.0f;
    s.h     = (algo == PathAlgorithm::AStar)
                ? calc_h(std::abs(sx - ex), std::abs(sy - ey), heur, diag)
                : 0.0f;
    s.f     = s.h;
    s.state = NodeState::Open;
    open_.push(start);

    const int dirs = diag ? 8 : 4;

    while (!open_.empty())
    {
        const int curr = open_.pop();
        grid_[curr].state = NodeState::Closed;

        if (curr == end)
        {
            int i = end;
            while (i != -1)
            {
                out.push_back(grid_to_world(i % w_, i / w_));
                i = grid_[i].parent;
            }
            std::reverse(out.begin(), out.end());
            return true;
        }

        const int cx = curr % w_;
        const int cy = curr / w_;

        for (int d = 0; d < dirs; ++d)
        {
            const int nx = cx + DX[d];
            const int ny = cy + DY[d];

            if (!is_walkable_cell(nx, ny))           continue;
            if (!valid_diagonal(cx, cy, nx, ny))     continue;

            const int ni = cell_idx(nx, ny);
            touch(ni);
            GridNode& nb = grid_[ni];

            if (nb.state == NodeState::Closed) continue;

            const float cost = (d >= 4) ? SQRT2 : 1.0f;
            const float ng   = grid_[curr].g + cost;

            if (nb.state == NodeState::Unvisited || ng < nb.g)
            {
                nb.g      = ng;
                nb.parent = curr;
                nb.h      = (algo == PathAlgorithm::AStar)
                              ? calc_h(std::abs(nx - ex), std::abs(ny - ey), heur, diag)
                              : 0.0f;
                nb.f      = nb.g + nb.h;

                if (nb.state == NodeState::Unvisited)
                {
                    nb.state = NodeState::Open;
                    open_.push(ni);
                }
                else
                {
                    open_.update(ni);
                }
            }
        }
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  String-pulling (LOS Bresenham)
// ─────────────────────────────────────────────────────────────────────────────

std::vector<Vector2> NavigationGrid2D::smooth_path(const std::vector<Vector2>& path) const
{
    if (path.size() <= 2) return path;

    std::vector<Vector2> out;
    out.reserve(path.size());
    out.push_back(path[0]);

    size_t anchor = 0;
    while (anchor < path.size() - 1)
    {
        size_t reach = anchor + 1;
        for (size_t j = path.size() - 1; j > anchor + 1; --j)
        {
            int ax, ay, bx, by;
            world_to_grid(path[anchor].x, path[anchor].y, ax, ay);
            world_to_grid(path[j].x,      path[j].y,      bx, by);
            if (has_los(ax, ay, bx, by))
            {
                reach = j;
                break;
            }
        }
        anchor = reach;
        out.push_back(path[anchor]);
    }

    return out;
}

// ═════════════════════════════════════════════════════════════════════════════
//  API pública de pathfinding
// ═════════════════════════════════════════════════════════════════════════════

std::vector<Vec2> NavigationGrid2D::find_path(
    const Vec2& start_world, const Vec2& end_world,
    bool allow_diagonal, bool smooth,
    PathAlgorithm algo, PathHeuristic heur)
{
    last_path_.clear();
    if (!grid_) return last_path_;

    int sx, sy, ex, ey;
    world_to_grid(start_world.x, start_world.y, sx, sy);
    world_to_grid(end_world.x,   end_world.y,   ex, ey);

    std::vector<Vector2> r_path;
    run_search(sx, sy, ex, ey, allow_diagonal, algo, heur, r_path);

    if (smooth && r_path.size() > 2)
        r_path = smooth_path(r_path);

    last_path_.reserve(r_path.size());
    for (const auto& p : r_path)
        last_path_.emplace_back(p.x, p.y);

    return last_path_;
}

Vec2 NavigationGrid2D::get_closest_walkable(const Vec2& world_pos) const
{
    if (!grid_ || w_ <= 0 || h_ <= 0) return world_pos;

    int gx, gy;
    world_to_grid(world_pos.x, world_pos.y, gx, gy);

    // If already walkable, return as-is
    if (in_bounds(gx, gy) && is_walkable_cell(gx, gy))
        return Vec2((float)(grid_to_world(gx, gy).x), (float)(grid_to_world(gx, gy).y));

    // BFS outward from (gx,gy) to find nearest walkable cell
    const int max_r = std::max(w_, h_);
    for (int r = 1; r <= max_r; ++r)
    {
        for (int dx = -r; dx <= r; ++dx)
        {
            for (int dy = -r; dy <= r; ++dy)
            {
                if (abs(dx) != r && abs(dy) != r) continue; // only perimeter
                const int nx = gx + dx, ny = gy + dy;
                if (in_bounds(nx, ny) && is_walkable_cell(nx, ny))
                {
                    const Vector2 wp = grid_to_world(nx, ny);
                    return Vec2(wp.x, wp.y);
                }
            }
        }
    }
    return world_pos; // no walkable cell found
}

float NavigationGrid2D::get_last_path_length() const
{
    float len = 0.0f;
    for (size_t i = 1; i < last_path_.size(); ++i)
        len += last_path_[i].distance(last_path_[i - 1]);
    return len;
}

bool NavigationGrid2D::is_walkable(const Vec2& world_pos) const
{
    if (!grid_) return false;
    int gx, gy;
    world_to_grid(world_pos.x, world_pos.y, gx, gy);
    return in_bounds(gx, gy) && is_walkable_cell(gx, gy);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Debug draw
// ═════════════════════════════════════════════════════════════════════════════

void NavigationGrid2D::_draw()
{
    Node2D::_draw();

    bool draw_debug = show_debug;
    if (m_tree && m_tree->has_debug_draw_flag(SceneTree::DEBUG_SPATIAL))
        draw_debug = true;

    if (!draw_debug || !grid_) return;

    const Color block_color = { 255, 0, 0, 100 };

    for (int y = 0; y < h_; ++y)
    {
        for (int x = 0; x < w_; ++x)
        {
            if (grid_[cell_idx(x, y)].walkable == 0)
            {
                const Vector2 c = grid_to_world(x, y);
                DrawRectangle(
                    (int)(c.x - res_ / 2.0f),
                    (int)(c.y - res_ / 2.0f),
                    res_, res_, block_color);
            }
        }
    }

    if (!last_path_.empty())
    {
        const Color path_color = { 0, 255, 0, 255 };
        const Color node_color = { 0, 200, 0, 255 };

        if (last_path_.size() > 1)
        {
            for (size_t i = 0; i < last_path_.size() - 1; ++i)
            {
                DrawLineEx({ last_path_[i].x,   last_path_[i].y   },
                           { last_path_[i+1].x, last_path_[i+1].y },
                           3.0f, path_color);
            }
        }
        for (const auto& p : last_path_)
            DrawCircleV({ p.x, p.y }, 4.0f, node_color);
    }
}
