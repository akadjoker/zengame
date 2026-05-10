#include "pch.hpp"
#include "Path.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  Direcções: cardeal primeiro, depois diagonal
//  i < 4  → cardinal   (custo 1.0)
//  i >= 4 → diagonal   (custo √2)
// ─────────────────────────────────────────────────────────────────────────────
const int Mask::DX[8] = { 0, 1,  0, -1,  -1,  1, 1, -1 };
const int Mask::DY[8] = {-1, 0,  1,  0,  -1, -1, 1,  1 };

static constexpr float SQRT2 = 1.41421356f;

// ═════════════════════════════════════════════════════════════════════════════
//  OpenListHeap
// ═════════════════════════════════════════════════════════════════════════════

OpenListHeap::OpenListHeap(int capacity)
{
    if (capacity > 0)
    {
        pos_.assign(capacity, -1);
        heap_.reserve(capacity);
    }
}

void OpenListHeap::init(int capacity, GridNode* nodes)
{
    nodes_ = nodes;
    pos_.assign(capacity, -1);
    heap_.clear();
    heap_.reserve(capacity);
}

void OpenListHeap::clear()
{
    for (int i : heap_) pos_[i] = -1;
    heap_.clear();
}

bool OpenListHeap::better(int a, int b) const
{
    const float fa = nodes_[a].f;
    const float fb = nodes_[b].f;
    if (fa != fb) return fa < fb;
    return nodes_[a].h < nodes_[b].h;   // tie-break: menor h primeiro
}

void OpenListHeap::swap_at(int a, int b)
{
    std::swap(heap_[a], heap_[b]);
    pos_[heap_[a]] = a;
    pos_[heap_[b]] = b;
}

void OpenListHeap::bubble_up(int i)
{
    while (i > 0)
    {
        const int p = (i - 1) >> 1;
        if (!better(heap_[i], heap_[p])) break;
        swap_at(i, p);
        i = p;
    }
}

void OpenListHeap::bubble_down(int i)
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

void OpenListHeap::push(int node_idx)
{
    assert(pos_[node_idx] < 0 && "nó já está no heap");
    pos_[node_idx] = (int)heap_.size();
    heap_.push_back(node_idx);
    bubble_up((int)heap_.size() - 1);
}

int OpenListHeap::pop()
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

void OpenListHeap::update(int node_idx)
{
    const int p = pos_[node_idx];
    assert(p >= 0 && "update de nó fora do heap");
    bubble_up(p);
    bubble_down(pos_[node_idx]);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Mask —  
// ═════════════════════════════════════════════════════════════════════════════

Mask::Mask(int w, int h, int resolution)
    : w_(w), h_(h), res_(resolution)
{
    assert(w > 0 && h > 0 && resolution > 0);
    grid_ = new GridNode[w_ * h_];
    open_.init(w_ * h_, grid_);
}

Mask::~Mask()
{
    delete[] grid_;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Grid
// ─────────────────────────────────────────────────────────────────────────────

void Mask::set_occupied(int x, int y)
{
    if (in_bounds(x, y)) grid_[idx(x, y)].walkable = 0;
}

void Mask::set_free(int x, int y)
{
    if (in_bounds(x, y)) grid_[idx(x, y)].walkable = 1;
}

void Mask::clear_all()
{
    for (int i = 0; i < w_ * h_; ++i) grid_[i].walkable = 1;
}

bool Mask::is_occupied(int x, int y) const
{
    if (!in_bounds(x, y)) return true;
    return grid_[idx(x, y)].walkable == 0;
}

bool Mask::is_walkable(int x, int y) const
{
    if (!in_bounds(x, y)) return false;
    return grid_[idx(x, y)].walkable == 1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Coordenadas
// ─────────────────────────────────────────────────────────────────────────────

void Mask::world_to_grid(Vector2 world, int& gx, int& gy) const
{
    gx = (int)(world.x / (float)res_);
    gy = (int)(world.y / (float)res_);
}

Vector2 Mask::grid_to_world(int gx, int gy) const
{
    // centro da célula
    return { (gx + 0.5f) * (float)res_, (gy + 0.5f) * (float)res_ };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Carregamento de imagem
// ─────────────────────────────────────────────────────────────────────────────

void Mask::load_from_image(const char* path, int threshold)
{
    clear_all();
    Image img = LoadImage(path);
    ImageResize(&img, w_, h_);
    Color* px = LoadImageColors(img);

    for (int y = 0; y < h_; ++y)
        for (int x = 0; x < w_; ++x)
        {
            const Color& c = px[y * w_ + x];
            if ((c.r + c.g + c.b) / 3 < threshold)
                set_occupied(x, y);
        }

    UnloadImageColors(px);
    UnloadImage(img);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resultado interno (BuLang / scripting)
// ─────────────────────────────────────────────────────────────────────────────

Vector2 Mask::result_point(int i) const
{
    if (i < 0 || i >= (int)result_.size()) return {0.0f, 0.0f};
    return result_[i];
}

// ═════════════════════════════════════════════════════════════════════════════
//  Pathfinding — internals
// ═════════════════════════════════════════════════════════════════════════════

inline void Mask::touch(int i)
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

float Mask::calc_h(int dx, int dy, PathHeuristic heur, bool diag) const
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
        // com diagonais, octile é admissível; sem diagonais, manhattan
        if (diag)
        {
            const float F = SQRT2 - 1.0f;
            return (dx < dy) ? F * (float)dx + (float)dy
                             : F * (float)dy + (float)dx;
        }
        return (float)(dx + dy);
    }
}

bool Mask::valid_diagonal(int cx, int cy, int nx, int ny) const
{
    // Movimento diagonal: os dois vizinhos cardinais têm de ser walkable
    if (nx != cx && ny != cy)
        return is_walkable(cx, ny) && is_walkable(nx, cy);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  run_search — implementação única de A* / Dijkstra
// ─────────────────────────────────────────────────────────────────────────────

bool Mask::run_search(int sx, int sy, int ex, int ey,
                      bool diag, PathAlgorithm algo, PathHeuristic heur,
                      std::vector<Vector2>& out)
{
    out.clear();

    if (!is_walkable(sx, sy) || !is_walkable(ex, ey))
        return false;

    ++gen_;                 // invalida todos os stamps → sem loop de reset
    open_.clear();

    const int start = idx(sx, sy);
    const int end   = idx(ex, ey);

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
            // reconstrução do caminho
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

            if (!is_walkable(nx, ny))          continue;
            if (!valid_diagonal(cx, cy, nx, ny)) continue;

            const int ni = idx(nx, ny);
            touch(ni);
            GridNode& nb = grid_[ni];

            if (nb.state == NodeState::Closed)  continue;

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
//  API pública de pathfinding
// ─────────────────────────────────────────────────────────────────────────────

std::vector<Vector2> Mask::find_path(int sx, int sy, int ex, int ey,
                                     bool diag, PathAlgorithm algo, PathHeuristic heur)
{
    std::vector<Vector2> path;
    run_search(sx, sy, ex, ey, diag, algo, heur, path);
    return path;
}

bool Mask::find_path_ex(int sx, int sy, int ex, int ey,
                        bool diag, PathAlgorithm algo, PathHeuristic heur)
{
    return run_search(sx, sy, ex, ey, diag, algo, heur, result_);
}

// ═════════════════════════════════════════════════════════════════════════════
//  String pulling  —  line-of-sight Bresenham
// ═════════════════════════════════════════════════════════════════════════════

bool Mask::has_los(int x0, int y0, int x1, int y1) const
{
    const int dx  = std::abs(x1 - x0);
    const int dy  = std::abs(y1 - y0);
    const int sx  = x0 < x1 ? 1 : -1;
    const int sy  = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        if (!is_walkable(x0, y0)) return false;
        if (x0 == x1 && y0 == y1) break;

        const int e2 = err << 1;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
    return true;
}

std::vector<Vector2> Mask::smooth_path(const std::vector<Vector2>& path) const
{
    if (path.size() <= 2) return path;

    std::vector<Vector2> out;
    out.reserve(path.size());
    out.push_back(path[0]);

    size_t anchor = 0;
    while (anchor < path.size() - 1)
    {
        // tenta saltar o máximo possível a partir de anchor
        size_t reach = anchor + 1;
        for (size_t j = path.size() - 1; j > anchor + 1; --j)
        {
            int ax, ay, bx, by;
            world_to_grid(path[anchor], ax, ay);
            world_to_grid(path[j],      bx, by);
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