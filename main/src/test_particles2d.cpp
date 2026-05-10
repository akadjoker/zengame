#include "pch.hpp"
#include <raylib.h>

#include "Assets.hpp"
#include "Node2D.hpp"
#include "ParticleEmitter2D.hpp"
#include "ParticleEffectFactory.hpp"
#include "Renderer2D.hpp"
#include "SceneTree.hpp"
#include "Sprite2D.hpp"
#include "View2D.hpp"

class ParticleHudNode : public Node
{
public:
    explicit ParticleHudNode(SceneTree* tree)
        : Node("ParticleHud")
        , m_tree(tree)
    {
    }

    void _draw() override
    {
        int child_count = 0;
        if (m_tree && m_tree->get_root())
        {
            child_count = (int)m_tree->get_root()->get_child_count();
        }

        DrawText("SPACE explosion (one-shot auto remove)", 16, 16, 20, WHITE);
        DrawText("F toggle fire  S toggle smoke  C shake  V trauma  Z zoom  X flash", 16, 40, 20, WHITE);
        DrawText(TextFormat("Root children: %d", child_count), 16, 64, 20, YELLOW);
    }

private:
    SceneTree* m_tree = nullptr;
};

int main()
{
    InitWindow(1280, 720, "MiniGodot2D - ParticleEmitter2D Test");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    const int fire_graph = GraphLib::Instance().load("fire", "../assets/fire.png");
    const int smoke_graph = GraphLib::Instance().load("smoke", "../assets/snow.png");

    SceneTree tree;
    Renderer2D renderer;
    renderer.set_clear_color(Color{18, 18, 26, 255});
    Node2D* root = new Node2D("Root");

    View2D* cam = new View2D("Camera");
    cam->is_current = true;
    cam->position = Vec2(640.0f, 360.0f);
    cam->set_trauma_profile(14.0f, 10.0f, 20.0f, 1.8f);
    root->add_child(cam);

    Sprite2D* marker = new Sprite2D("Marker");
    marker->graph = fire_graph >= 0 ? fire_graph : 0;
    marker->position = Vec2(640.0f, 520.0f);
    marker->color = Color{255, 255, 255, 80};
    root->add_child(marker);

    ParticleEmitter2D* fire = ParticleEffectFactory::create_fire("Fire", fire_graph >= 0 ? fire_graph : 0);
    fire->position = Vec2(640.0f, 520.0f);
    root->add_child(fire);

    ParticleEmitter2D* smoke = ParticleEffectFactory::create_smoke("Smoke", smoke_graph >= 0 ? smoke_graph : 0);
    smoke->position = Vec2(700.0f, 540.0f);
    root->add_child(smoke);

    tree.set_root(root);
    tree.set_ui_root(new ParticleHudNode(&tree));
    tree.start();
    renderer.fade_in(0.35f);

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F))
        {
            fire->emitting = !fire->emitting;
        }

        if (IsKeyPressed(KEY_S))
        {
            smoke->emitting = !smoke->emitting;
        }

        if (IsKeyPressed(KEY_SPACE))
        {
            ParticleEmitter2D* explosion = ParticleEffectFactory::create_explosion("Explosion", fire_graph >= 0 ? fire_graph : 0);
            explosion->position = Vec2((float)GetMouseX(), (float)GetMouseY());
            root->add_child(explosion);
            explosion->play();
            cam->start_shake(12.0f, 8.0f, 20.0f, 8.0f);
            cam->add_trauma(0.45f);
            cam->start_zoom_punch(0.08f, 0.18f);
            renderer.flash(Color{255, 240, 180, 255}, 0.12f, 120.0f);
        }

        if (IsKeyPressed(KEY_C))
        {
            cam->start_shake(16.0f, 10.0f, 18.0f, 10.0f);
        }

        if (IsKeyPressed(KEY_V))
        {
            cam->add_trauma(0.65f);
        }

        if (IsKeyPressed(KEY_Z))
        {
            cam->start_zoom_punch(0.10f, 0.22f);
        }

        if (IsKeyPressed(KEY_X))
        {
            renderer.flash(WHITE, 0.15f, 100.0f);
        }

        tree.process(dt);
        renderer.update(dt);
        renderer.render(tree);
    }

    renderer.shutdown();
    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
