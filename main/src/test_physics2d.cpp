#include "SceneTree.hpp"
#include "RigidBody2D.hpp"
#include "RayCast2D.hpp"
#include "StaticBody2D.hpp"
#include "Collider2D.hpp"
#include "Sprite2D.hpp"
#include "TextNode2D.hpp"
#include "Node2D.hpp"
#include "View2D.hpp"
#include "Assets.hpp"
#include <raylib.h>
#include <string>
#include <cstdlib>

// ============================================================================
// Physics2D Demo
//
// Controls:
//   LMB   — cast ray from screen centre to mouse
//   RMB   — spawn current shape at mouse position
//   SPACE — spawn current shape at centre with random velocity
//   1/2/3 — select Circle / Rectangle / Polygon
//   C     — clear all balls
//   D     — toggle physics debug drawing
// ============================================================================

static constexpr int   W = 800;
static constexpr int   H = 600;
static constexpr float WALL_T = 20.0f;  // wall thickness

// ----------------------------------------------------------------------------
// Helper — create a static wall box
// ----------------------------------------------------------------------------
static StaticBody2D* make_wall(Node* parent, const std::string& name,
                               float x, float y, float w, float h)
{
    auto* body  = new StaticBody2D(name);
    body->position = Vec2(x, y);

    auto* col   = new Collider2D("Shape");
    col->shape  = Collider2D::ShapeType::Rectangle;
    col->points = {Vec2(0,0), Vec2(w,0), Vec2(w,h), Vec2(0,h)};
    col->size   = Vec2(w, h);
    body->add_child(col);
    parent->add_child(body);
    return body;
}

// ----------------------------------------------------------------------------
// Demo scene
// ----------------------------------------------------------------------------
class PhysicsDemo : public Node
{
public:
    enum class SpawnShape { Circle, Rectangle, Polygon };

    Node*       world     = nullptr;   // parent for balls
    RayCast2D*  ray       = nullptr;
    TextNode2D* info      = nullptr;
    bool        debug_draw = true;
    int         ball_count = 0;
    SpawnShape  spawn_shape = SpawnShape::Circle;

    explicit PhysicsDemo(const std::string& n = "PhysicsDemo") : Node(n) {}

    void _ready() override
    {
        // ── Arena walls ───────────────────────────────────────────────────────
        make_wall(this, "Floor",  0,           H - WALL_T, W,      WALL_T);
        make_wall(this, "Ceil",   0,           0,          W,      WALL_T);
        make_wall(this, "Left",   0,           0,          WALL_T, H);
        make_wall(this, "Right",  W - WALL_T,  0,          WALL_T, H);

        // Inner obstacle platform
        make_wall(this, "Plat1", 150, 350, 200, 20);
        make_wall(this, "Plat2", 450, 250, 200, 20);

        // Static segment slopes / rails
        auto* slope_a = new StaticBody2D("SlopeA");
        auto* slope_a_col = new Collider2D("Shape");
        slope_a_col->shape = Collider2D::ShapeType::Segment;
        slope_a_col->points = {Vec2(0.0f, 0.0f), Vec2(180.0f, -90.0f)};
        slope_a->position = Vec2(90.0f, 520.0f);
        slope_a->add_child(slope_a_col);
        add_child(slope_a);

        auto* slope_b = new StaticBody2D("SlopeB");
        auto* slope_b_col = new Collider2D("Shape");
        slope_b_col->shape = Collider2D::ShapeType::Segment;
        slope_b_col->points = {Vec2(0.0f, 0.0f), Vec2(180.0f, 110.0f)};
        slope_b->position = Vec2(470.0f, 380.0f);
        slope_b->add_child(slope_b_col);
        add_child(slope_b);

        auto* poly_obstacle = new StaticBody2D("PolyObstacle");
        auto* poly_col = new Collider2D("Shape");
        poly_col->shape = Collider2D::ShapeType::Polygon;
        poly_col->points = {
            Vec2(-50.0f,  40.0f),
            Vec2(  0.0f, -45.0f),
            Vec2( 55.0f,  15.0f),
            Vec2( 15.0f,  55.0f)
        };
        poly_obstacle->position = Vec2(390.0f, 440.0f);
        poly_obstacle->add_child(poly_col);
        add_child(poly_obstacle);

        // ── Container for balls ───────────────────────────────────────────────
        world = new Node("Balls");
        add_child(world);

        // ── RayCast2D from centre ─────────────────────────────────────────────
        auto* ray_origin = new Node2D("RayOrigin");
        ray_origin->position = Vec2(W * 0.5f, H * 0.5f);
        add_child(ray_origin);

        ray = new RayCast2D("Ray");
        ray->cast_to    = Vec2(400, 0);
        ray->show_debug = true;
        ray_origin->add_child(ray);

        // ── HUD ───────────────────────────────────────────────────────────────
        info = new TextNode2D("Info");
        info->font_size = 16.0f;
        info->color     = WHITE;
        info->position  = Vec2(25, 25);
        info->text      = "";
        // Added to ui_root in main()
    }

    const char* get_shape_name() const
    {
        switch (spawn_shape)
        {
            case SpawnShape::Circle:    return "Circle";
            case SpawnShape::Rectangle: return "Rectangle";
            case SpawnShape::Polygon:   return "Polygon";
        }
        return "Unknown";
    }

    void spawn_body(float x, float y, float vx, float vy)
    {
        auto* body = new RigidBody2D("Ball" + std::to_string(ball_count++));
        body->position       = Vec2(x, y);
        body->linear_velocity = Vec2(vx, vy);
        body->gravity_scale  = 1.0f;
        body->bounciness     = 0.5f + (float)(rand() % 40) / 100.0f;
        body->friction       = 0.05f;

        auto* col   = new Collider2D("Shape");
        switch (spawn_shape)
        {
            case SpawnShape::Circle:
                col->shape  = Collider2D::ShapeType::Circle;
                col->radius = 12.0f + (float)(rand() % 8);
                break;

            case SpawnShape::Rectangle:
                col->shape = Collider2D::ShapeType::Rectangle;
                col->size = Vec2(22.0f + (float)(rand() % 16),
                                 22.0f + (float)(rand() % 16));
                break;

            case SpawnShape::Polygon:
                col->shape = Collider2D::ShapeType::Polygon;
                col->points = {
                    Vec2(-18.0f,  16.0f),
                    Vec2(  0.0f, -20.0f),
                    Vec2( 18.0f,  16.0f)
                };
                break;
        }
        body->add_child(col);

        world->add_child(body);
    }

    void _update(float dt) override
    {
        (void)dt;

        const Vector2 mouse = GetMousePosition();

        // Aim ray toward mouse
        if (ray)
        {
            auto* origin = dynamic_cast<Node2D*>(ray->get_parent());
            if (origin)
            {
                const Vec2 op = origin->get_global_position();
                const float dx = mouse.x - op.x;
                const float dy = mouse.y - op.y;
                const float len = sqrtf(dx*dx + dy*dy);
                if (len > 1.0f)
                    ray->cast_to = Vec2(dx / len * 600.0f, dy / len * 600.0f);
            }
        }

        // RMB: spawn ball at mouse
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        {
            const float vx = (float)(rand() % 400 - 200);
            const float vy = (float)(rand() % 200 - 300);
            spawn_body(mouse.x, mouse.y, vx, vy);
        }

        // SPACE: spawn ball at centre
        if (IsKeyPressed(KEY_SPACE))
        {
            const float vx = (float)(rand() % 600 - 300);
            const float vy = (float)(rand() % 400 - 500);
            spawn_body(W * 0.5f, H * 0.3f, vx, vy);
        }

        if (IsKeyPressed(KEY_ONE))   spawn_shape = SpawnShape::Circle;
        if (IsKeyPressed(KEY_TWO))   spawn_shape = SpawnShape::Rectangle;
        if (IsKeyPressed(KEY_THREE)) spawn_shape = SpawnShape::Polygon;

        // C: clear balls
        if (IsKeyPressed(KEY_C))
        {
            while (world->get_child_count() > 0)
            {
                Node* child = world->get_child(0);
                world->remove_child(child);
                delete child;
            }
            ball_count = 0;
        }

        // D: toggle debug
        if (IsKeyPressed(KEY_D))
        {
            debug_draw = !debug_draw;
            if (ray) ray->show_debug = debug_draw;
        }

        // Update HUD
        if (info)
        {
            std::string hit_info = ray && ray->is_colliding()
                ? "HIT frac=" + std::to_string((int)(ray->get_hit_fraction() * 100)) + "%"
                  + (ray->get_collider()
                       ? " body=" + ray->get_collider()->name
                       : " tilemap")
                : "no hit";

            info->text =
                "Balls: " + std::to_string(world ? world->get_child_count() : 0) +
                "\nSpawn: " + get_shape_name() +
                "\nRay: " + hit_info +
                "\n\nRMB: spawn shape   SPACE: random shape"
                "\n1/2/3: Circle Rect Poly"
                "\nC: clear   D: toggle debug";
        }
    }

    void _draw() override
    {
        // Draw arena outline
        DrawRectangleLinesEx({WALL_T, WALL_T, W - WALL_T*2, H - WALL_T*2},
                              2.0f, Color{80, 80, 120, 255});

        // Draw walls (filled)
        const Color wall_col = Color{60, 60, 90, 255};
        DrawRectangle(0, (int)(H - WALL_T), W, (int)WALL_T, wall_col);
        DrawRectangle(0, 0, W, (int)WALL_T, wall_col);
        DrawRectangle(0, 0, (int)WALL_T, H, wall_col);
        DrawRectangle((int)(W - WALL_T), 0, (int)WALL_T, H, wall_col);
        DrawRectangle(150, 350, 200, 20, wall_col);
        DrawRectangle(450, 250, 200, 20, wall_col);

        // Draw balls
        if (world && debug_draw)
        {
            for (size_t i = 0; i < world->get_child_count(); ++i)
            {
                auto* body = dynamic_cast<RigidBody2D*>(world->get_child(i));
                if (!body) continue;

                Collider2D* col = body->get_collider();
                if (!col) continue;

                const Vec2 pos = body->get_global_position();

                if (col->shape == Collider2D::ShapeType::Circle)
                {
                    const float r = col->get_world_radius();
                    const Color c = body->is_on_floor()
                        ? Color{100, 200, 100, 200}
                        : Color{200, 120, 60, 200};
                    DrawCircleV({pos.x, pos.y}, r, c);
                    DrawCircleLines((int)pos.x, (int)pos.y, r,
                                    Color{255, 200, 100, 255});
                }
                else if (col->shape == Collider2D::ShapeType::Segment)
                {
                    Vec2 a, b;
                    col->get_world_segment(a, b);
                    DrawLineEx({a.x, a.y}, {b.x, b.y}, 3.0f, Color{220, 220, 120, 255});
                }
                else
                {
                    std::vector<Vec2> poly;
                    col->get_world_polygon(poly);
                    if (poly.size() >= 2)
                    {
                        const Color fill = body->is_on_floor()
                            ? Color{100, 200, 100, 160}
                            : Color{120, 160, 220, 160};
                        if (poly.size() == 3)
                        {
                            DrawTriangle({poly[0].x, poly[0].y},
                                         {poly[1].x, poly[1].y},
                                         {poly[2].x, poly[2].y},
                                         fill);
                        }
                        else if (poly.size() == 4)
                        {
                            DrawTriangle({poly[0].x, poly[0].y},
                                         {poly[1].x, poly[1].y},
                                         {poly[2].x, poly[2].y},
                                         fill);
                            DrawTriangle({poly[0].x, poly[0].y},
                                         {poly[2].x, poly[2].y},
                                         {poly[3].x, poly[3].y},
                                         fill);
                        }
                        for (size_t j = 0; j < poly.size(); ++j)
                        {
                            const Vec2& a = poly[j];
                            const Vec2& b = poly[(j + 1) % poly.size()];
                            DrawLineEx({a.x, a.y}, {b.x, b.y}, 2.0f, Color{255, 220, 120, 255});
                        }
                    }
                }
            }
        }
    }
};

// ============================================================================
int main()
{
    InitWindow(W, H, "ZenEngine - RigidBody2D + RayCast2D Demo");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    SceneTree tree;

    auto* scene = new PhysicsDemo();
    tree.set_root(scene);

    // HUD via ui_root so it's always on top
    auto* ui_root = new Node("UIRoot");
    scene->info->get_parent();   // already parented in _ready to `this`
    // Move info label to a dedicated ui node
    // (simpler: just re-parent after start)
    tree.set_ui_root(ui_root);

    tree.start();

    // Now re-parent the info label to the ui_root
    if (scene->info)
    {
        scene->remove_child(scene->info);
        ui_root->add_child(scene->info);
    }

    while (tree.is_running() && !WindowShouldClose())
    {
        tree.process(GetFrameTime());
        BeginDrawing();
        ClearBackground(Color{20, 20, 30, 255});
        tree.draw();
        EndDrawing();
    }

    tree.quit();
    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
