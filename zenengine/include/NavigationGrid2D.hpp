#pragma once

#include "pch.hpp"
#include "Node2D.hpp"
#include <vector>
#include <cstdint>
#include <raylib.h>

class TileMap2D;

// ─── Enums de pathfinding (públicos) ──────────────────────────────────────────
enum class PathHeuristic : uint8_t { Manhattan = 0, Euclidean, Octile, Chebyshev };
enum class PathAlgorithm : uint8_t { AStar = 0, Dijkstra };

// ─────────────────────────────────────────────────────────────────────────────
// NavigationGrid2D
//   Grid de walkability com A* / Dijkstra integrado directamente.
//   Pode ser configurado manualmente ou sincronizado a partir de um TileMap2D.
// ─────────────────────────────────────────────────────────────────────────────
class NavigationGrid2D : public Node2D
{
public:
    explicit NavigationGrid2D(const std::string& p_name = "NavigationGrid2D");
    ~NavigationGrid2D() override;

    NavigationGrid2D(const NavigationGrid2D&)            = delete;
    NavigationGrid2D& operator=(const NavigationGrid2D&) = delete;

    // ── Inicialização ─────────────────────────────────────────────────────────
    void init(int width, int height, int resolution = 1);
    void sync_from_tilemap(const TileMap2D* tilemap);

    // Carrega a grelha a partir de uma imagem:
    //   pixels escuros (brightness < threshold) → livre (walkable)
    //   pixels claros                           → sólido (obstáculo)
    // A imagem é redimensionada para w_ × h_ automaticamente.
    void load_from_image(const char* path, int threshold = 128);

    // ── Gestão da grelha ──────────────────────────────────────────────────────
    void set_cell_solid(int x, int y, bool solid);
    bool is_cell_solid (int x, int y) const;
    void clear_grid    ();

    int  get_width     () const { return w_; }
    int  get_height    () const { return h_; }
    int  get_resolution() const { return res_; }

    // ── Coordenadas ───────────────────────────────────────────────────────────
    void    world_to_grid(float wx, float wy, int& gx, int& gy) const;
    Vector2 grid_to_world(int gx, int gy) const;

    // ── Pathfinding ───────────────────────────────────────────────────────────
    std::vector<Vec2> find_path(
        const Vec2& start_world,
        const Vec2& end_world,
        bool          allow_diagonal = true,
        bool          smooth         = true,
        PathAlgorithm algo           = PathAlgorithm::AStar,
        PathHeuristic heur           = PathHeuristic::Octile);

    // ── Debug / draw ──────────────────────────────────────────────────────────
    void _draw() override;
    bool show_debug = false;

private:
    // ── Estado interno do nó ─────────────────────────────────────────────────
    enum class NodeState : uint8_t { Unvisited = 0, Open, Closed };

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

    // ── Min-heap por f (tie-break por h) ──────────────────────────────────────
    struct OpenListHeap
    {
        GridNode*        nodes = nullptr;
        std::vector<int> heap_;
        std::vector<int> pos_;   // pos_[nodeIdx] = heap position, -1 se ausente

        void init(int capacity, GridNode* n);
        void clear();
        bool empty()              const { return heap_.empty(); }
        bool contains(int idx)    const { return pos_[idx] >= 0; }
        void push  (int idx);
        int  pop   ();
        void update(int idx);

    private:
        bool better   (int a, int b) const;
        void swap_at  (int a, int b);
        void bubble_up(int i);
        void bubble_down(int i);
    };

    // ── Dados ─────────────────────────────────────────────────────────────────
    GridNode*         grid_      = nullptr;
    int               w_         = 0;
    int               h_         = 0;
    int               res_       = 1;
    uint32_t          gen_       = 0;
    OpenListHeap      open_;
    std::vector<Vec2> last_path_;

    static const int DX[8];
    static const int DY[8];

    // ── Helpers internos ──────────────────────────────────────────────────────
    inline int  cell_idx  (int x, int y) const { return y * w_ + x; }
    inline bool in_bounds (int x, int y) const
        { return x >= 0 && x < w_ && y >= 0 && y < h_; }

    bool  is_walkable_cell  (int x, int y) const;
    void  touch             (int i);
    float calc_h            (int dx, int dy, PathHeuristic heur, bool diag) const;
    bool  valid_diagonal    (int cx, int cy, int nx, int ny)                const;
    bool  has_los           (int x0, int y0, int x1, int y1)               const;

    bool run_search(
        int sx, int sy, int ex, int ey,
        bool diag, PathAlgorithm algo, PathHeuristic heur,
        std::vector<Vector2>& out);

    std::vector<Vector2> smooth_path(const std::vector<Vector2>& path) const;
};
