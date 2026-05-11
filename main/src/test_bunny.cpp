#include "SceneTree.hpp"
#include "Sprite2D.hpp"
#include "Node2D.hpp"
#include "View2D.hpp"
#include "Assets.hpp"
#include <raylib.h>
#include <vector>
#include <string>
#include <cstdlib>

// ============================================================================
// BunnyMark — performance benchmark
//
//  Three modes (press TAB to switch):
//    MODE 0 — Sprite2D nodes inside SceneTree (full engine overhead)
//    MODE 1 — Node2D + manual DrawTexture (node overhead, no Sprite2D logic)
//    MODE 2 — Raw DrawTexture, zero node overhead (baseline)
//
//  Controls:
//    SPACE   — add 500 bunnies
//    BACK    — remove 500 bunnies
//    TAB     — switch mode (resets bunnies)
//    ESC     — quit
// ============================================================================

static constexpr int   SCREEN_W  = 800;
static constexpr int   SCREEN_H  = 600;
static constexpr int   BATCH     = 500;
static const char*     ASSET     = "../assets/wabbit_alpha.png";

// ----------------------------------------------------------------------------
// Shared raw-bunny data (used by all modes)
// ----------------------------------------------------------------------------
struct BunnyData
{
    float x, y;
    float vx, vy;
};

static std::vector<BunnyData> g_data;

static void init_bunny(BunnyData& b, float w, float h)
{
    b.x  = (float)(rand() % SCREEN_W);
    b.y  = (float)(rand() % SCREEN_H);
    b.vx = (float)(rand() % 500 - 250) / 60.0f;
    b.vy = (float)(rand() % 500 - 250) / 60.0f;
    (void)w; (void)h;
}

static void update_bunnies(float dt, float tw, float th)
{
    for (auto& b : g_data)
    {
        b.x += b.vx * dt * 60.0f;
        b.y += b.vy * dt * 60.0f;
        b.vy += 9.8f * dt;            // gravity

        if (b.x + tw > SCREEN_W) { b.vx = -fabsf(b.vx); b.x = SCREEN_W - tw; }
        if (b.x < 0)             { b.vx =  fabsf(b.vx); b.x = 0; }
        if (b.y + th > SCREEN_H) { b.vy = -fabsf(b.vy); b.y = SCREEN_H - th; }
        if (b.y < 0)             { b.vy =  fabsf(b.vy); b.y = 0; }
    }
}

// ============================================================================
// MODE 0 — Sprite2D SceneTree nodes
// ============================================================================
class BunnyNodeScene : public Node2D
{
public:
    int graph_id = -1;
    std::vector<Sprite2D*> sprites;

    BunnyNodeScene() : Node2D("BunnyNodeScene") {}

    void spawn(int n)
    {
        const float tw = 26.0f, th = 37.0f;
        for (int i = 0; i < n; ++i)
        {
            BunnyData bd;
            init_bunny(bd, tw, th);
            g_data.push_back(bd);

            Sprite2D* s = new Sprite2D("B");
            s->graph    = graph_id;
            s->position = Vec2(bd.x, bd.y);
            add_child(s);
            sprites.push_back(s);
        }
    }

    void despawn(int n)
    {
        n = std::min(n, (int)sprites.size());
        for (int i = 0; i < n; ++i)
        {
            Sprite2D* s = sprites.back();
            sprites.pop_back();
            remove_child(s);
            delete s;
            g_data.pop_back();
        }
    }

    void _update(float dt) override
    {
        const float tw = 26.0f, th = 37.0f;
        update_bunnies(dt, tw, th);

        for (size_t i = 0; i < sprites.size(); ++i)
            sprites[i]->position = Vec2(g_data[i].x, g_data[i].y);
    }
};

// ============================================================================
// MODE 1 — Node2D children, DrawTexture manually (no Sprite2D)
// ============================================================================
class BunnyRawNodeScene : public Node2D
{
public:
    Texture2D   tex   = {};
    int         graph_id = -1;
    std::vector<Node2D*> nodes;

    BunnyRawNodeScene() : Node2D("BunnyRawNodeScene") {}

    void spawn(int n)
    {
        const float tw = 26.0f, th = 37.0f;
        for (int i = 0; i < n; ++i)
        {
            BunnyData bd;
            init_bunny(bd, tw, th);
            g_data.push_back(bd);

            Node2D* nd = new Node2D("B");
            nd->position = Vec2(bd.x, bd.y);
            add_child(nd);
            nodes.push_back(nd);
        }
    }

    void despawn(int n)
    {
        n = std::min(n, (int)nodes.size());
        for (int i = 0; i < n; ++i)
        {
            Node2D* nd = nodes.back();
            nodes.pop_back();
            remove_child(nd);
            delete nd;
            g_data.pop_back();
        }
    }

    void _update(float dt) override
    {
        const float tw = 26.0f, th = 37.0f;
        update_bunnies(dt, tw, th);
        for (size_t i = 0; i < nodes.size(); ++i)
            nodes[i]->position = Vec2(g_data[i].x, g_data[i].y);
    }

    void _draw() override
    {
        // Manually draw texture at each node's world position
        if (tex.id == 0) return;
        for (Node2D* nd : nodes)
        {
            const Vec2 p = nd->get_global_position();
            DrawTexture(tex, (int)p.x, (int)p.y, WHITE);
        }
        // Do NOT call Node2D::_draw() — children have no _draw() here
    }
};

// ============================================================================
// MODE 2 — Pure raw DrawTexture, zero node overhead
// ============================================================================
class BunnyRawScene : public Node2D
{
public:
    Texture2D tex = {};

    BunnyRawScene() : Node2D("BunnyRawScene") {}

    void spawn(int n)
    {
        const float tw = 26.0f, th = 37.0f;
        for (int i = 0; i < n; ++i)
        {
            BunnyData bd;
            init_bunny(bd, tw, th);
            g_data.push_back(bd);
        }
    }

    void despawn(int n)
    {
        n = std::min(n, (int)g_data.size());
        g_data.resize(g_data.size() - n);
    }

    void _update(float dt) override
    {
        update_bunnies(dt, 26.0f, 37.0f);
    }

    void _draw() override
    {
        if (tex.id == 0) return;
        for (const auto& b : g_data)
            DrawTexture(tex, (int)b.x, (int)b.y, WHITE);
    }
};

// ============================================================================
// HUD overlay
// ============================================================================
class HUD : public Node
{
public:
    int   mode    = 0;
    int*  p_mode  = nullptr;

    HUD() : Node("HUD") {}

    void _draw() override
    {
        const int   count = (int)g_data.size();
        const float fps   = GetFPS();
        const float ms    = GetFrameTime() * 1000.0f;

        static const char* mode_names[] = {
            "MODE 0: Sprite2D nodes (full engine)",
            "MODE 1: Node2D + DrawTexture (no Sprite2D)",
            "MODE 2: Raw DrawTexture (zero nodes)"
        };

        DrawRectangle(0, 0, SCREEN_W, 80, Color{0,0,0,180});
        DrawText(mode_names[p_mode ? *p_mode : 0], 8, 6, 16, YELLOW);

        const std::string stat =
            "Bunnies: " + std::to_string(count) +
            "   FPS: " + std::to_string((int)fps) +
            "   " + std::to_string((int)ms) + "ms";
        DrawText(stat.c_str(), 8, 28, 18, WHITE);
        DrawText("SPACE: +500   BACK: -500   TAB: switch mode", 8, 52, 14, LIGHTGRAY);
    }
};

// ============================================================================
// Main
// ============================================================================
int main()
{
    InitWindow(SCREEN_W, SCREEN_H, "ZenEngine - BunnyMark");
    SetTargetFPS(0);   // unlimited FPS for benchmark

    GraphLib::Instance().create();
    const int graph_id = GraphLib::Instance().load("wabbit", ASSET);

    Texture2D raw_tex = {};
    if (Graph* g = GraphLib::Instance().getGraph(graph_id))
        if (Texture2D* t = GraphLib::Instance().getTexture(g->texture))
            raw_tex = *t;

    int mode = 0;
    SceneTree tree;

    // Build world root + ui_root (HUD always on top via set_ui_root)
    auto build = [&]()
    {
        g_data.clear();
        Node* world = new Node("World");

        if (mode == 0)
        {
            auto* s = new BunnyNodeScene();
            s->graph_id = graph_id;
            s->spawn(BATCH);
            world->add_child(s);
        }
        else if (mode == 1)
        {
            auto* s = new BunnyRawNodeScene();
            s->tex      = raw_tex;
            s->graph_id = graph_id;
            s->spawn(BATCH);
            world->add_child(s);
        }
        else
        {
            auto* s = new BunnyRawScene();
            s->tex = raw_tex;
            s->spawn(BATCH);
            world->add_child(s);
        }

        HUD* hud = new HUD();
        hud->p_mode = &mode;

        tree.set_root(world);
        tree.set_ui_root(hud);   // drawn AFTER world pass — always on top
    };

    build();
    tree.start();

    const bool space_down_prev   = false;
    const bool back_down_prev    = false;
    (void)space_down_prev;
    (void)back_down_prev;

    while (tree.is_running() && !WindowShouldClose())
    {
        const float dt = GetFrameTime();

        if (IsKeyPressed(KEY_TAB))
        {
            // Switch mode: rebuild entire scene
            mode = (mode + 1) % 3;
            tree.quit();
            build();
            tree.start();
        }
        else
        {
            // Spawn / despawn via world root's first child (the bunny scene)
            Node* world = tree.get_root();
            if (world && world->get_child_count() > 0)
            {
                Node* scene = world->get_child(0);
                if (IsKeyPressed(KEY_SPACE))
                {
                    if (auto* s = dynamic_cast<BunnyNodeScene*>(scene))    s->spawn(BATCH);
                    if (auto* s = dynamic_cast<BunnyRawNodeScene*>(scene)) s->spawn(BATCH);
                    if (auto* s = dynamic_cast<BunnyRawScene*>(scene))     s->spawn(BATCH);
                }
                if (IsKeyPressed(KEY_BACKSPACE))
                {
                    if (auto* s = dynamic_cast<BunnyNodeScene*>(scene))    s->despawn(BATCH);
                    if (auto* s = dynamic_cast<BunnyRawNodeScene*>(scene)) s->despawn(BATCH);
                    if (auto* s = dynamic_cast<BunnyRawScene*>(scene))     s->despawn(BATCH);
                }
            }

            tree.process(dt);
        }

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
