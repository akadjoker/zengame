#pragma once

#include "SceneTree.hpp"
#include <raylib.h>

// ----------------------------------------------------------------------------
// ZenApp — window + game loop.
//
// The script (zenpy) drives everything via ScriptHost on nodes.
// C++ just calls run().
//
//   int main() {
//       ZenApp app(1280, 720, "Game");
//       // set root, load scripts...
//       return app.run();
//   }
// ----------------------------------------------------------------------------

class ZenApp
{
public:
    ZenApp(int width, int height, const char* title,
           int fps = 60, Color clear = {20, 20, 30, 255});

    int  run();
    void quit();

    SceneTree tree;

private:
    int         m_width;
    int         m_height;
    const char* m_title;
    int         m_fps;
    Color       m_clear;
};


