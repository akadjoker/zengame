#include "pch.hpp"
#include "Node.hpp"
#include "SceneTree.hpp"

// ============================================================
// Test subclasses
// ============================================================

class Player : public Node
{
public:

    int hp = 100;

    explicit Player(const std::string& n) : Node(n)
    {
    }

    void _ready() override
    {
        std::cout << "[" << get_path() << "] _ready() hp=" << hp << "\n";
    }

    void _update(float dt) override
    {
        std::cout << "[" << name << "] _update(dt=" << dt << ")\n";
    }

    void _on_destroy() override
    {
        std::cout << "[" << name << "] _on_destroy()\n";
    }
};

class Sprite : public Node
{
public:

    explicit Sprite(const std::string& n) : Node(n)
    {
    }

    void _ready() override
    {
        std::cout << "[" << get_path() << "] _ready() sprite ready\n";
    }

    void _draw() override
    {
        std::cout << "[" << name << "] _draw()\n";
    }

    void _on_destroy() override
    {
        std::cout << "[" << name << "] _on_destroy()\n";
    }
};

// ============================================================
// Main
// ============================================================

int main()
{
    std::cout << "=== MiniGodot2D - Node test ===\n\n";

    SceneTree tree;

    Node*   root   = new Node("Root");
    Player* player = new Player("Player");
    Sprite* sprite = new Sprite("Sprite");

    player->add_child(sprite);
    root->add_child(player);
    tree.set_root(root);
    tree.start();

    std::cout << "\n--- Frame (dt=0.016) ---\n";
    tree.process(0.016f);
    tree.draw();

    std::cout << "\n--- Paths ---\n";
    std::cout << "Player : " << player->get_path() << "\n";
    std::cout << "Sprite : " << sprite->get_path() << "\n";

    std::cout << "\n--- find_node ---\n";
    Node* found = root->find_node("Sprite");
    if (found)
    {
        std::cout << "Found: " << found->get_path() << "\n";
    }

    std::cout << "\n--- remove_child (no destroy) ---\n";
    Node* orphan = player->remove_child(sprite);
    std::cout << "Has parent? " << (orphan->get_parent() ? "yes" : "no") << "\n";
    delete orphan;

    std::cout << "\n--- Cascade destruction ---\n";

    return 0;
}
