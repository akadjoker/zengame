#pragma once
#include <vector>
#include <cstdint>
#include <raylib.h>

// ─────────────────────────────────────────────────────────────────────────────

enum class PathHeuristic : uint8_t
{
    Manhattan = 0,
    Euclidean,
    Octile,
    Chebyshev
};

enum class PathAlgorithm : uint8_t
{
    AStar     = 0,
    Dijkstra
};

enum class NodeState : uint8_t
{
    Unvisited = 0,
    Open,
    Closed
};

// ─────────────────────────────────────────────────────────────────────────────

struct GridNode
{
    float     f       = 0.0f;
    float     g       = 0.0f;
    float     h       = 0.0f;
    int       parent  = -1;
    NodeState state   = NodeState::Unvisited;
    uint8_t   walkable= 1;
    uint32_t  stamp   = 0;   // geração — evita reset O(W×H)
};

// ─────────────────────────────────────────────────────────────────────────────
//  Min-heap por f (tie-break por h).
//  pos[nodeIndex] = posição no heap; -1 = fora.
// ─────────────────────────────────────────────────────────────────────────────
class OpenListHeap
{
public:
    explicit OpenListHeap(int capacity = 0);

    void init(int capacity, GridNode* nodes);
    void clear();
    bool empty() const { return heap_.empty(); }

    void push(int idx);
    int  pop();
    void update(int idx);       // chama após baixar g de um nó Open
    bool contains(int idx) const { return pos_[idx] >= 0; }

private:
    bool better(int a, int b) const;
    void swap_at(int a, int b);
    void bubble_up(int i);
    void bubble_down(int i);

    GridNode*        nodes_ = nullptr;
    std::vector<int> heap_;
    std::vector<int> pos_;      // pos_[nodeIdx] = heap position, -1 se ausente
};

// ─────────────────────────────────────────────────────────────────────────────
//  Mask  —  grid de walkability + A* / Dijkstra
// ─────────────────────────────────────────────────────────────────────────────
class Mask
{
public:
    Mask(int w, int h, int resolution = 1);
    ~Mask();

    Mask(const Mask&)            = delete;
    Mask& operator=(const Mask&) = delete;

    // ── grid ─────────────────────────────────────────────────────────────────
    void set_occupied(int x, int y);
    void set_free    (int x, int y);
    void clear_all   ();

    bool is_occupied(int x, int y) const;   // out-of-bounds → true
    bool is_walkable(int x, int y) const;   // out-of-bounds → false

    int get_width()      const { return w_; }
    int get_height()     const { return h_; }
    int get_resolution() const { return res_; }

    // ── coordenadas ──────────────────────────────────────────────────────────
    // world → célula grid (trunca)
    void world_to_grid(Vector2 world, int& gx, int& gy) const;
    // célula grid → centro em world
    Vector2 grid_to_world(int gx, int gy) const;

    // ── carregamento ─────────────────────────────────────────────────────────
    void load_from_image(const char* path, int threshold = 128);

    // ── pathfinding ──────────────────────────────────────────────────────────
    // Retorna vector de pontos em world-space (centro de cada célula).
    // Vazio se sem caminho.
    std::vector<Vector2> find_path(
        int sx, int sy, int ex, int ey,
        bool           allow_diagonal = true,
        PathAlgorithm  algo           = PathAlgorithm::AStar,
        PathHeuristic  heur           = PathHeuristic::Octile);

    // Versão que guarda resultado interno (acesso por scripting / BuLang).
    bool find_path_ex(
        int sx, int sy, int ex, int ey,
        bool           allow_diagonal = true,
        PathAlgorithm  algo           = PathAlgorithm::AStar,
        PathHeuristic  heur           = PathHeuristic::Octile);

    int     result_count()     const { return (int)result_.size(); }
    Vector2 result_point(int i) const;

    // ── pós-processamento ────────────────────────────────────────────────────
    // String-pulling: remove waypoints intermédios com LOS directo.
    std::vector<Vector2> smooth_path(const std::vector<Vector2>& path) const;

private:
    // Implementação única do search — findPath e findPathEx delegam aqui.
    bool run_search(
        int sx, int sy, int ex, int ey,
        bool diag, PathAlgorithm algo, PathHeuristic heur,
        std::vector<Vector2>& out);

    // Garante que o nó está inicializado para a geração actual.
    inline void touch(int idx);

    float calc_h(int dx, int dy, PathHeuristic heur, bool diag) const;
    bool  valid_diagonal(int cx, int cy, int nx, int ny) const;
    bool  has_los(int x0, int y0, int x1, int y1) const;

    inline int  idx(int x, int y) const { return y * w_ + x; }
    inline bool in_bounds(int x, int y) const
        { return x >= 0 && x < w_ && y >= 0 && y < h_; }

    GridNode*   grid_  = nullptr;
    int         w_     = 0;
    int         h_     = 0;
    int         res_   = 1;
    uint32_t    gen_   = 0;     // geração actual

    OpenListHeap         open_;
    std::vector<Vector2> result_;

    static const int DX[8];
    static const int DY[8];
};