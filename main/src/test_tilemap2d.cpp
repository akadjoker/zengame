#include "SceneTree.hpp"
#include "TileMap2D.hpp"
#include "StaticBody2D.hpp"
#include "Collider2D.hpp"
#include "Node2D.hpp"
#include "View2D.hpp"
#include "Assets.hpp"
#include <raylib.h>

// ----------------------------------------------------------------------------
// Demo: load a .tmx map via SceneTree helpers.
//
// Controls:
//   Arrow keys / WASD  -- pan camera
//   +/-                -- zoom in/out
//   G                  -- toggle tile grid
//   C                  -- toggle collision shapes debug
// ----------------------------------------------------------------------------

static const char* TMX_MAPS[] = {
    "../assets/TileMaps/tile_iso_offset.tmx",
    "../assets/TileMaps/orthogonal-test3.tmx",
    "../assets/TileMaps/ortho-objects.tmx",
    "../assets/TileMaps/iso-test2.tmx",
};
static const int TMX_MAP_COUNT = 4;

// ----------------------------------------------------------------------------
// CollisionDebugOverlay
//   Node2D with z_index = 100 so it draws ON TOP of all tile layers.
//   Holds refs to StaticBody2D nodes and renders their Collider2D shapes.
// ----------------------------------------------------------------------------
class CollisionDebugOverlay : public Node2D
{
public:
    bool show_col = true;
    std::vector<StaticBody2D*> bodies;

    CollisionDebugOverlay() : Node2D("CollisionDebugOverlay")
    {
        z_index = 100;
    }

    void _draw() override
    {
        if (!show_col) return;

        for (StaticBody2D* sb : bodies)
        {
            if (!sb) continue;
            for (size_t ci = 0; ci < sb->get_child_count(); ++ci)
            {
                auto* col = dynamic_cast<Collider2D*>(sb->get_child(ci));
                if (!col) continue;

                const Rectangle aabb = col->get_world_aabb();
                DrawRectangleLinesEx(aabb, 2.0f, Color{0, 200, 255, 220});

                if (col->shape == Collider2D::ShapeType::Polygon)
                {
                    std::vector<Vec2> pts;
                    col->get_world_polygon(pts);
                    for (size_t pi = 0; pi + 1 < pts.size(); ++pi)
                        DrawLineEx({pts[pi].x,   pts[pi].y},
                                   {pts[pi+1].x, pts[pi+1].y},
                                   2.0f, Color{255, 165, 0, 220});
                    if (pts.size() > 2)
                        DrawLineEx({pts.back().x, pts.back().y},
                                   {pts[0].x, pts[0].y},
                                   2.0f, Color{255, 165, 0, 220});
                }
            }
        }
    }
};

// ----------------------------------------------------------------------------
// TilemapDemo
// ----------------------------------------------------------------------------
class TilemapDemo : public Node
{
public:
    View2D*               camera     = nullptr;
    CollisionDebugOverlay* overlay   = nullptr;
    int                   map_index  = 0;

    TilemapDemo() : Node("TilemapDemo") {}

    void load_map(int index)
    {
        // Remove old tile layers and bodies (keep camera and overlay)
        std::vector<Node*> to_remove;
        for (size_t i = 0; i < get_child_count(); ++i)
        {
            Node* ch = get_child(i);
            if (ch != camera && ch != overlay)
                to_remove.push_back(ch);
        }
        for (Node* n : to_remove)
        {
            remove_child(n);
            delete n;
        }
        overlay->bodies.clear();

        const char* path = TMX_MAPS[index];
        const int layers = m_tree->load_tilemap_from_tmx(path, this);
        TraceLog(LOG_INFO, "[%d] %s — %d layers", index, path, layers);

        // Force show_ids so we can always see tile data even if texture fails
        for (size_t i = 0; i < get_child_count(); ++i)
            if (auto* tm = dynamic_cast<TileMap2D*>(get_child(i)))
                tm->show_ids = true;

        const int col_count = m_tree->load_collision_from_tmx(path, this);
        TraceLog(LOG_INFO, "Loaded %d collision bodies", col_count);

        for (size_t i = 0; i < get_child_count(); ++i)
            if (auto* sb = dynamic_cast<StaticBody2D*>(get_child(i)))
                overlay->bodies.push_back(sb);

        // Move overlay to end so it draws on top
        remove_child(overlay);
        add_child(overlay);
    }

    void _ready() override
    {
        // Camera
        camera = new View2D("Camera");
        camera->is_current = true;
        camera->zoom       = 0.5f;
        camera->position   = Vec2(300, 300);
        add_child(camera);

        overlay = new CollisionDebugOverlay();
        add_child(overlay);

        load_map(map_index);
    }

    void _update(float dt) override
    {
        const float spd = 300.0f * dt / camera->zoom;

        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) camera->position.x += spd;
        if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) camera->position.x -= spd;
        if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) camera->position.y += spd;
        if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) camera->position.y -= spd;

        if (IsKeyDown(KEY_EQUAL)) camera->zoom += 0.5f * dt;
        if (IsKeyDown(KEY_MINUS)) camera->zoom -= 0.5f * dt;
        camera->zoom = std::max(0.1f, camera->zoom);

        if (IsKeyPressed(KEY_G))
            for (size_t i = 0; i < get_child_count(); ++i)
                if (auto* tm = dynamic_cast<TileMap2D*>(get_child(i)))
                    tm->show_grid = !tm->show_grid;

        if (IsKeyPressed(KEY_C))
            overlay->show_col = !overlay->show_col;

        if (IsKeyPressed(KEY_TAB))
        {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
                map_index = (map_index - 1 + TMX_MAP_COUNT) % TMX_MAP_COUNT;
            else
                map_index = (map_index + 1) % TMX_MAP_COUNT;
            load_map(map_index);
        }
    }

    // _draw() of the ROOT runs BEFORE children -- use it only for HUD
    void _draw() override
    {
        DrawRectangle(0, 0, 480, 96, Color{0, 0, 0, 160});
        DrawText("Arrows/WASD: pan   +/-: zoom   G: grid   C: col", 8,  8, 16, WHITE);
        DrawText("Tab: next map   Shift+Tab: prev map",              8, 28, 16, WHITE);
        const std::string map_name = TMX_MAPS[map_index];
        DrawText(("Map: " + map_name.substr(map_name.rfind('/') + 1)).c_str(), 8, 48, 16, Color{0, 255, 255, 255});
        const std::string info =
            "Zoom: " + std::to_string((int)(camera->zoom * 100)) + "%" +
            "  Bodies: " + std::to_string((int)overlay->bodies.size());
        DrawText(info.c_str(), 8, 72, 16, YELLOW);
    }
};

int main()
{
    InitWindow(800, 600, "ZenEngine - TileMap2D TMX Demo");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    SceneTree tree;
    tree.set_root(new TilemapDemo());
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
    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
