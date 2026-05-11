#include "SceneTree.hpp"
#include "TileMap2D.hpp"
#include "StaticBody2D.hpp"
#include "Collider2D.hpp"
#include "Node2D.hpp"
#include "View2D.hpp"
#include "Assets.hpp"
#include "TextNode2D.hpp"
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

static const char* TMX_PATH     = "../assets/shooter.tmx";
static const char* TMX_COL_PATH = "../assets/shooter.tmx";

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
    View2D*               camera  = nullptr;
    CollisionDebugOverlay* overlay = nullptr;

    TilemapDemo() : Node("TilemapDemo") {}

    void _ready() override
    {
        // Camera
        camera = new View2D("Camera");
        camera->is_current = true;
        camera->zoom       = 0.5f;
        camera->position   = Vec2(300, 300);
        add_child(camera);

        // Load all tile layers -- each becomes a TileMap2D child node
        const int layers = m_tree->load_tilemap_from_tmx(TMX_PATH, this);
        TraceLog(LOG_INFO, "Loaded %d tile layers", layers);

        // Load collision objects from the objectgroup tmx
        const int col_count = m_tree->load_collision_from_tmx(TMX_COL_PATH, this);
        TraceLog(LOG_INFO, "Loaded %d collision bodies", col_count);

        // Overlay node added LAST so propagate_draw() calls it after all tiles
        overlay = new CollisionDebugOverlay();
        for (size_t i = 0; i < get_child_count(); ++i)
            if (auto* sb = dynamic_cast<StaticBody2D*>(get_child(i)))
                overlay->bodies.push_back(sb);
        add_child(overlay);



        // Uso básico (fonte default)
        auto* label = new TextNode2D("Label");
        label->text      = "Score: 0";
        label->font_size = 24.0f;
        label->color     = YELLOW;
        label->position  = Vec2(100, 50);
        add_child(label);

        // Com fonte TTF
        label->load_font("assets/fonts/arial.ttf", 64); // glyph_size = qualidade
        label->font_size = 32.0f; // tamanho de draw (independente do glyph_size)

        // Pivot / âncora
        label->pivot = Vec2(0.5f, 0.5f); // centrado
        label->pivot = Vec2(0.0f, 0.0f); // top-left (default)

        // Medir tamanho
        Vec2 sz = label->measure(); // {width, height} em pixels

        // Herda todo o transform de Node2D
        label->rotation = 0.0f;
        label->scale    = Vec2(2.0f, 2.0f);

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
    }

    // _draw() of the ROOT runs BEFORE children -- use it only for HUD
    void _draw() override
    {
        DrawRectangle(0, 0, 430, 80, Color{0, 0, 0, 160});
        DrawText("Arrows/WASD: pan   +/-: zoom",      8,  8, 16, WHITE);
        DrawText("G: toggle grid   C: toggle collision", 8, 28, 16, WHITE);
        const std::string info =
            "Zoom: " + std::to_string((int)(camera->zoom * 100)) + "%" +
            "  Bodies: " + std::to_string((int)overlay->bodies.size());
        DrawText(info.c_str(), 8, 52, 16, YELLOW);
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
