#include "SceneTree.hpp"
#include "TileMap2D.hpp"
#include "NavigationGrid2D.hpp"
#include "View2D.hpp"
#include <raylib.h>
#include <vector>

class NavTestScene : public Node
{
public:
    TileMap2D* tilemap = nullptr;
    NavigationGrid2D* nav_grid = nullptr;
    Vec2 start_pos = {48, 48};
    Vec2 end_pos   = {320, 320};
    std::vector<Vec2> path;

    NavTestScene() : Node("NavTestScene") {}

    void _ready() override
    {
        const int map_w    = 20;
        const int map_h    = 15;
        const int tile_size = 32;

        // ── TileMap (sem atlas — graph_id = -1) ────────────────────────────
        tilemap = new TileMap2D("WorldMap");
        add_child(tilemap);
        tilemap->init(map_w, map_h, tile_size, tile_size, -1);
        tilemap->show_grid = true;

        // bordas sólidas
        for (int x = 0; x < map_w; ++x)
        {
            tilemap->set_tile(x, 0,          Tile2D{1, 1}); // topo
            tilemap->set_tile(x, map_h - 1,  Tile2D{1, 1}); // fundo
        }
        for (int y = 0; y < map_h; ++y)
        {
            tilemap->set_tile(0,          y, Tile2D{1, 1}); // esquerda
            tilemap->set_tile(map_w - 1,  y, Tile2D{1, 1}); // direita
        }

        // obstáculos internos — usar set_tile, NÃO fill (fill é flood-fill!)
        tilemap->set_tile(5,  5, Tile2D{1, 1});
        tilemap->set_tile(6,  5, Tile2D{1, 1});
        tilemap->set_tile(7,  5, Tile2D{1, 1});
        tilemap->set_tile(5,  6, Tile2D{1, 1});
        tilemap->set_tile(6,  6, Tile2D{1, 1});
        tilemap->set_tile(7,  6, Tile2D{1, 1});

        tilemap->set_tile(12,  8, Tile2D{1, 1});
        tilemap->set_tile(12,  9, Tile2D{1, 1});
        tilemap->set_tile(12, 10, Tile2D{1, 1});
        tilemap->set_tile(12, 11, Tile2D{1, 1});
        tilemap->set_tile(13,  8, Tile2D{1, 1});
        tilemap->set_tile(13,  9, Tile2D{1, 1});

        // ── NavigationGrid ─────────────────────────────────────────────────
        nav_grid = new NavigationGrid2D("NavGrid");
        add_child(nav_grid);
        nav_grid->show_debug = false; // desenhamos nós mesmos abaixo

        nav_grid->init(map_w, map_h, tile_size);
        nav_grid->sync_from_tilemap(tilemap);

        // caminho inicial
        path = nav_grid->find_path(start_pos, end_pos);
    }

    void _update(float dt) override
    {
        (void)dt;

        Vector2 mouse = GetMousePosition();
        Vec2 mouse_world{mouse.x, mouse.y};

        // LMB arrastar → move start
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            start_pos = mouse_world;
            path = nav_grid->find_path(start_pos, end_pos);
        }

        // RMB arrastar → move end
        if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
        {
            end_pos = mouse_world;
            path = nav_grid->find_path(start_pos, end_pos);
        }

        // MMB click → alternar parede
        if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON))
        {
            int gx = 0, gy = 0;
            if (tilemap && tilemap->world_to_grid(mouse_world, gx, gy))
            {
                Tile2D* t = tilemap->get_tile(gx, gy);
                if (t)
                {
                    bool make_solid = (t->solid == 0);
                    *t = Tile2D{static_cast<uint16_t>(make_solid ? 1 : 0),
                                static_cast<uint8_t> (make_solid ? 1 : 0)};
                    nav_grid->set_cell_solid(gx, gy, make_solid);
                    path = nav_grid->find_path(start_pos, end_pos);
                }
            }
        }
    }

    void _draw() override
    {
        const int ts = tilemap ? tilemap->tile_width : 32;

        // ── Desenhar células sólidas a vermelho ────────────────────────────
        if (tilemap)
        {
            for (int y = 0; y < tilemap->height; ++y)
            {
                for (int x = 0; x < tilemap->width; ++x)
                {
                    const Tile2D* t = tilemap->get_tile(x, y);
                    if (t && t->solid)
                    {
                        DrawRectangle(x * ts, y * ts, ts, ts,
                                      Color{180, 60, 60, 200});
                    }
                }
            }
        }

        // ── Caminho ────────────────────────────────────────────────────────
        if (!path.empty())
        {
            const Color path_color = {0, 255, 100, 255};
            const Color node_color = {0, 200,  80, 255};

            if (path.size() > 1)
            {
                for (size_t i = 0; i + 1 < path.size(); ++i)
                {
                    DrawLineEx({path[i].x,   path[i].y},
                               {path[i+1].x, path[i+1].y},
                               3.0f, path_color);
                }
            }
            for (const Vec2& p : path)
                DrawCircleV({p.x, p.y}, 4.0f, node_color);
        }

        // ── Marcadores start/end ───────────────────────────────────────────
        DrawCircleV({start_pos.x, start_pos.y}, 8.0f, BLUE);
        DrawCircleV({end_pos.x,   end_pos.y},   8.0f, RED);

        // ── HUD ────────────────────────────────────────────────────────────
        DrawRectangle(0, 0, 420, 75, Color{0, 0, 0, 160});
        DrawText("LMB (arrastar): mover Start (azul)",    8,  8, 18, WHITE);
        DrawText("RMB (arrastar): mover End (vermelho)",  8, 28, 18, WHITE);
        DrawText("MMB (click): alternar parede",          8, 48, 18, WHITE);

        if (path.empty())
            DrawText("SEM CAMINHO!", 640, 8, 22, RED);
        else
        {
            const std::string s = "Passos: " + std::to_string(path.size());
            DrawText(s.c_str(), 600, 8, 20, GREEN);
        }
    }
};

int main()
{
    InitWindow(800, 600, "ZenEngine - NavigationGrid2D Test");
    SetTargetFPS(60);

    SceneTree tree;
    tree.set_root(new NavTestScene());
    tree.start();

    while (tree.is_running() && !WindowShouldClose())
    {
        tree.process(GetFrameTime());

        BeginDrawing();
        ClearBackground(Color{30, 30, 40, 255});
        tree.draw();
        EndDrawing();
    }

    tree.quit();
    CloseWindow();
    return 0;
}
