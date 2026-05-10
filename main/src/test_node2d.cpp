
#include "pch.hpp"
#include "Node.hpp"
#include "Node2D.hpp"
#include "SceneTree.hpp"
#include "math.hpp"

// ============================================================
// Test subclasses
// ============================================================

class Player : public Node2D
{
public:

    explicit Player(const std::string& n) : Node2D(n)
    {
    }

    void _ready() override
    {
        std::cout << "[" << get_path() << "] _ready()\n";
        std::cout << "  local  pos = " << position.x << ", " << position.y << "\n";
        std::cout << "  global pos = " << get_global_position().x
                                       << ", " << get_global_position().y << "\n";
    }
};

class Weapon : public Node2D
{
public:

    explicit Weapon(const std::string& n) : Node2D(n)
    {
    }

    void _ready() override
    {
        std::cout << "[" << get_path() << "] _ready()\n";
        std::cout << "  local  pos = " << position.x << ", " << position.y << "\n";
        std::cout << "  global pos = " << get_global_position().x
                                       << ", " << get_global_position().y << "\n";
    }
};

// ============================================================
// Main
// ============================================================

int main()
{
    std::cout << "=== MiniGodot2D - Node2D test ===\n\n";

    SceneTree tree;

    Node*   root   = new Node("Root");
    Player* player = new Player("Player");
    Weapon* weapon = new Weapon("Weapon");

    // Player at (100, 200), rotated 45 degrees
    player->position = Vec2(100.0f, 200.0f);
    player->rotation = 45.0f;
    player->scale    = Vec2(2.0f, 2.0f);

    // Weapon at (50, 0) relative to player
    weapon->position = Vec2(50.0f, 0.0f);

    player->add_child(weapon);
    root->add_child(player);
    tree.set_root(root);
    tree.start();

    std::cout << "\n--- Transform checks ---\n";

    // Local transform of player
    Matrix2D local = player->get_local_transform();
    std::cout << "Player local  matrix tx=" << local.tx << " ty=" << local.ty << "\n";

    // Global transform of weapon (should include player's rotation + scale)
    Vec2 weapon_global = weapon->get_global_position();
    std::cout << "Weapon global pos = " << weapon_global.x << ", " << weapon_global.y << "\n";

    // to_global / to_local round trip
    Vec2 local_pt(10.0f, 5.0f);
    Vec2 world_pt  = weapon->to_global(local_pt);
    Vec2 back_local = weapon->to_local(world_pt);
    std::cout << "\n--- to_global / to_local round trip ---\n";
    std::cout << "local  = " << local_pt.x  << ", " << local_pt.y  << "\n";
    std::cout << "world  = " << world_pt.x  << ", " << world_pt.y  << "\n";
    std::cout << "back   = " << back_local.x << ", " << back_local.y << " (should match local)\n";

    // look_at
    std::cout << "\n--- look_at ---\n";
    player->look_at(Vec2(200.0f, 200.0f));
    std::cout << "Player rotation after look_at(200,200) = " << player->rotation << " deg\n";

    // translate
    player->translate(Vec2(10.0f, 0.0f));
    std::cout << "Player pos after translate(10,0) = "
              << player->position.x << ", " << player->position.y << "\n";

    std::cout << "\n--- Cascade destruction ---\n";

    return 0;
}
