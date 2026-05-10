#include "pch.hpp"
#include <raylib.h>

#include "SceneTree.hpp"
#include "View2D.hpp"
#include "PolyMesh2D.hpp"
#include "CharacterBody2D.hpp"
#include "Collider2D.hpp"
#include "Assets.hpp"

static void setup_polygon(PolyMesh2D* mesh)
{
    mesh->clear();
    mesh->position = Vec2(180.0f, 180.0f);
    mesh->rotation = 0.0f;
    mesh->tint = Color{200, 170, 120, 255};
    mesh->show_wire = true;
    mesh->wire_color = GREEN;
    mesh->debug_collision_mesh = true;
    mesh->debug_collision_color = RED;
    mesh->debug_collision_fill = false;
    mesh->auto_collision = true;
    mesh->auto_collision_visible = true;

    mesh->add_point(0, 0);
    mesh->add_point(420, 20);
    mesh->add_point(500, 120);
    mesh->add_point(460, 260);
    mesh->add_point(200, 320);
    mesh->add_point(40, 260);
    mesh->build_polygon();
}

static void setup_track(PolyMesh2D* mesh)
{
    mesh->clear();
    mesh->position = Vec2(130.0f, 140.0f);
    mesh->rotation = 0.0f;
    mesh->tint = Color{180, 210, 160, 255};
    mesh->show_wire = true;
    mesh->wire_color = YELLOW;
    mesh->debug_collision_mesh = true;
    mesh->debug_collision_color = RED;
    mesh->debug_collision_fill = true;
    mesh->auto_collision = true;
    mesh->auto_collision_visible = true;

    mesh->add_point(0, 140);
    mesh->add_point(90, 120);
    mesh->add_point(180, 150);
    mesh->add_point(280, 200);
    mesh->add_point(380, 190);
    mesh->add_point(500, 240);
    mesh->add_point(680, 220);

    mesh->build_track_layered(220.0f, 34.0f, 0.02f, 0.03f, 1.0f, 1.0f);
}

int main()
{
    InitWindow(1200, 760, "MiniGodot2D - PolyMesh AutoCollision Test");
    SetTargetFPS(60);
    GraphLib::Instance().create();

    SceneTree tree;
    Node2D* root = new Node2D("Root");

    View2D* cam = new View2D("Camera");
    cam->is_current = true;
    root->add_child(cam);

    int mesh_body_graph = GraphLib::Instance().load("mesh_body", "../assets/tile3.png");
    int mesh_edge_graph = GraphLib::Instance().load("mesh_edge", "../assets/tile2.png");
    if (mesh_body_graph < 0) mesh_body_graph = 0;
    if (mesh_edge_graph < 0) mesh_edge_graph = 0;

    PolyMesh2D* mesh = new PolyMesh2D("DemoPolyMesh");
    mesh->collision_layer = (1u << 1);
    mesh->collision_mask = (1u << 0);
    mesh->set_body_texture_graph(mesh_body_graph);
    mesh->set_edge_texture_graph(mesh_edge_graph);
    mesh->set_bottom_scale(0.12f, 1.0f);
    mesh->set_top_scale(0.20f, 1.0f);
    root->add_child(mesh);

    CharacterBody2D* player = new CharacterBody2D("Player");
    player->collision_layer = (1u << 0);
    player->collision_mask = (1u << 1);
    player->position = Vec2(120.0f, 120.0f);
    root->add_child(player);

    Collider2D* player_col = new Collider2D("PlayerCollider");
    player_col->shape = Collider2D::ShapeType::Rectangle;
    player_col->size = Vec2(28.0f, 28.0f);
    player->add_child(player_col);

    setup_polygon(mesh);

    tree.set_root(root);
    tree.start();
    tree.set_debug_draw_flags(SceneTree::DEBUG_PIVOT | SceneTree::DEBUG_NAMES | SceneTree::DEBUG_SHAPES |
                              SceneTree::DEBUG_CONTACTS | SceneTree::DEBUG_SPATIAL);

    bool is_track_mode = false;
    cam->follow_target = player;
    cam->follow_speed = 6.0f;
    cam->position = player->position;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_F6)) mesh->debug_collision_mesh = !mesh->debug_collision_mesh;
        if (IsKeyPressed(KEY_F7)) mesh->debug_collision_fill = !mesh->debug_collision_fill;

        if (IsKeyPressed(KEY_ONE))
        {
            setup_polygon(mesh);
            is_track_mode = false;
        }
        if (IsKeyPressed(KEY_TWO))
        {
            setup_track(mesh);
            is_track_mode = true;
        }

        Vec2 input(0.0f, 0.0f);
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.x += 220.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) input.x -= 220.0f;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) input.y += 220.0f;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) input.y -= 220.0f;
        player->move_topdown(input, GetFrameTime());

        if (IsKeyDown(KEY_Q)) mesh->rotation -= 90.0f * GetFrameTime();
        if (IsKeyDown(KEY_E)) mesh->rotation += 90.0f * GetFrameTime();

        tree.process(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);
        tree.draw();
        DrawText("1 polygon  2 track layered", 12, 12, 20, WHITE);
        DrawText("WASD/Arrows move player | Q/E rotate mesh", 12, 36, 20, WHITE);
        DrawText(is_track_mode ? "Mode: TRACK" : "Mode: POLYGON", 12, 60, 20, is_track_mode ? YELLOW : GREEN);
        DrawText(TextFormat("F6 collision mesh: %s   F7 fill: %s",
                            mesh->debug_collision_mesh ? "ON" : "OFF",
                            mesh->debug_collision_fill ? "ON" : "OFF"),
                 12, 84, 20, LIGHTGRAY);
        EndDrawing();
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
