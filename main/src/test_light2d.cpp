#include "pch.hpp"
#include <raylib.h>

#include "Assets.hpp"
#include "Light2D.hpp"
#include "Node2D.hpp"
#include "ParallaxLayer2D.hpp"
#include "Renderer2D.hpp"
#include "SceneTree.hpp"
#include "Sprite2D.hpp"
#include "ShadowCaster2D.hpp"
#include "View2D.hpp"


static ShadowCaster2D* add_rect_shadow_caster(
    Node2D* parent,
    const std::string& name,
    const Vec2& size,
    const Vec2& local_offset = Vec2(0.0f, 0.0f)
)
{
    ShadowCaster2D* caster = new ShadowCaster2D(name);
    caster->use_aabb = true;
    caster->size = size;
    caster->position = local_offset;
    parent->add_child(caster);
    return caster;
}

static ShadowCaster2D* add_polygon_shadow_caster(
    Node2D* parent,
    const std::string& name,
    const std::vector<Vec2>& polygon_points,
    const Vec2& local_offset = Vec2(0.0f, 0.0f)
)
{
    ShadowCaster2D* caster = new ShadowCaster2D(name);
    caster->use_aabb = false;
    caster->position = local_offset;

    const size_t n = polygon_points.size();
    if (n >= 2)
    {
        caster->segments.reserve(n * 2);

        for (size_t i = 0; i < n; ++i)
        {
            const Vec2& a = polygon_points[i];
            const Vec2& b = polygon_points[(i + 1) % n];

            caster->segments.push_back(a);
            caster->segments.push_back(b);
        }
    }

    parent->add_child(caster);
    return caster;
}

static std::string make_light_bg_image()
{
    const std::string path = "/tmp/zengame_light_bg.png";
    Image img = GenImageColor(512, 512, Color{28, 26, 34, 255});

    for (int y = 0; y < img.height; ++y)
    {
        Color c = {
            (unsigned char)(28 + y * 12 / img.height),
            (unsigned char)(26 + y * 10 / img.height),
            (unsigned char)(34 + y * 18 / img.height),
            255
        };
        ImageDrawLine(&img, 0, y, img.width - 1, y, c);
    }

    for (int i = 0; i < 40; ++i)
    {
        int x = (i * 73) % img.width;
        int y = (i * 37) % img.height;
        ImageDrawRectangle(&img, x, y, 18, 18, Color{40, 44, 52, 255});
    }

    ExportImage(img, path.c_str());
    UnloadImage(img);
    return path;
}

class HudNode : public Node
{
public:
    Renderer2D *renderer = nullptr;

    void _draw() override
    {
        DrawText("Mouse = point light   Q/E rotate spot", 16, 16, 20, WHITE);
        DrawText("1 toggle point   2 toggle spot   L toggle lights", 16, 40, 20, WHITE);
        DrawText("S toggle shadows   D debug lights   X debug shadows", 16, 64, 20, WHITE);

        if (renderer)
        {
            const char *sh = renderer->shadows_enabled ? "ON" : "OFF";
            const char *li = renderer->lights_enabled  ? "ON" : "OFF";
            DrawText(TextFormat("Lights: %s   Shadows: %s", li, sh), 16, 92, 20, YELLOW);
        }
    }
};

int main()
{
    InitWindow(1280, 720, "MiniGodot2D - Shadow Test");
    SetTargetFPS(60);

    GraphLib::Instance().create();
    const std::string bg_path   = make_light_bg_image();
    const int bg_graph   = GraphLib::Instance().load("light_bg", bg_path.c_str());
    const int ship_graph = GraphLib::Instance().load("ship", "../assets/tile3.png");

    SceneTree  tree;
    Renderer2D renderer;
    renderer.set_clear_color(BLACK);
    renderer.set_ambient_color(Color{20, 20, 36, 255});

    Node2D *root = new Node2D("Root");

    View2D *cam = new View2D("Camera");
    cam->is_current = true;
    cam->position   = Vec2(640.0f, 360.0f);
    root->add_child(cam);

    ParallaxLayer2D *bg = new ParallaxLayer2D("BG");
    bg->graph            = bg_graph >= 0 ? bg_graph : 0;
    bg->mode             = ParallaxLayer2D::TILE_X | ParallaxLayer2D::TILE_Y;
    bg->scroll_factor_x  = 1.0f;
    bg->scroll_factor_y  = 1.0f;
    root->add_child(bg);

    // ── Ships + shadow casters ────────────────────────────────────────────────
    // Each ship gets a ShadowCaster2D child whose AABB matches the sprite size.
    // Adjust `size` to match playerShip1_orange.png actual dimensions.
    for (int i = 0; i < 6; ++i)
    {
        Sprite2D *ship = new Sprite2D("Ship");
        ship->graph    = ship_graph >= 0 ? ship_graph : 0;
        ship->position = Vec2(180.0f + i * 160.0f, 260.0f + (i % 2) * 140.0f);
        //ship->scale    = Vec2(1.4f, 1.4f);
        root->add_child(ship);

        // Shadow caster — AABB in local space, scaled with the sprite
        // playerShip1_orange.png is 99×75 px; multiply by scale (1.4)
        ShadowCaster2D *caster = new ShadowCaster2D("Shadow");
        caster->use_aabb = true;
        caster->size = Vec2(128,128);
        caster->position = Vec2(128*0.5f, 128*0.5f);  // center of the sprite
        ship->add_child(caster);  // child of ship so it follows position

        renderer.register_shadow_caster(caster);
    }

    // ── Point light (follows mouse) ───────────────────────────────────────────
    Light2D *mouse_light     = new Light2D("MouseLight");
    mouse_light->radius      = 320.0f;
    mouse_light->intensity   = 1.7f;
    mouse_light->color       = Color{255, 235, 180, 255};
    mouse_light->cast_shadows = true;
    mouse_light->position    = cam->screen_to_world(Vec2(640.0f, 360.0f));
    root->add_child(mouse_light);
    renderer.register_light(mouse_light);

    // ── Spot light (static, rotatable) ───────────────────────────────────────
    Light2D *spot       = new Light2D("Spot");
    spot->type          = Light2D::Type::Spot;
    spot->position      = Vec2(640.0f, 50.0f);
    spot->radius        = 1600.0f;
    spot->intensity     = 0.95f;
    spot->spot_angle    = 22.0f;
    spot->spot_dir      = 90.0f;
    spot->color         = Color{110, 180, 255, 255};
    spot->cast_shadows  = true;
    root->add_child(spot);
    renderer.register_light(spot);

    // ── HUD ───────────────────────────────────────────────────────────────────
    HudNode *hud    = new HudNode();
    hud->renderer   = &renderer;
    tree.set_ui_root(hud);

    tree.set_root(root);
    tree.start();

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        // Point light follows mouse in world space
        mouse_light->position = cam->screen_to_world(
            Vec2((float)GetMouseX(), (float)GetMouseY()));

        // Rotate spot
        if (IsKeyDown(KEY_Q)) spot->spot_dir -= 90.0f * dt;
        if (IsKeyDown(KEY_E)) spot->spot_dir += 90.0f * dt;

        // Toggles
        if (IsKeyPressed(KEY_ONE)) mouse_light->enabled      = !mouse_light->enabled;
        if (IsKeyPressed(KEY_TWO)) spot->enabled             = !spot->enabled;
        if (IsKeyPressed(KEY_L))   renderer.lights_enabled   = !renderer.lights_enabled;
        if (IsKeyPressed(KEY_S))   renderer.shadows_enabled  = !renderer.shadows_enabled;
        if (IsKeyPressed(KEY_D))   renderer.debug_lights     = !renderer.debug_lights;
        if (IsKeyPressed(KEY_X))   renderer.debug_shadows    = !renderer.debug_shadows;

        tree.process(dt);
        renderer.update(dt);
        renderer.render(tree);
    }

    renderer.shutdown();
    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}