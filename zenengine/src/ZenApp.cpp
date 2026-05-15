#include "ZenApp.hpp"
#include "Assets.hpp"

ZenApp::ZenApp(int width, int height, const char* title, int fps, Color clear)
    : m_width(width), m_height(height), m_title(title), m_fps(fps), m_clear(clear)
{}

int ZenApp::run()
{
    InitWindow(m_width, m_height, m_title);
    SetTargetFPS(m_fps);
    GraphLib::Instance().create();

    tree.start();

    while (!WindowShouldClose() && tree.is_running())
    {
        const float dt = GetFrameTime();
        tree.process(dt);

        BeginDrawing();
        ClearBackground(m_clear);
        tree.draw();
        EndDrawing();
    }

    tree.clean();                    // free nodes before GPU assets
    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}

void ZenApp::quit()
{
    tree.quit();
}
