// test_atlas_clip.cpp — test mouth-smile using OUR RenderTransformFlipClipOffset
// Atlas: rotate:true, xy:992,257, size:47,30
//   => original display: 47w x 30h
//   => packed in texture: 30w x 47h (rotated 90° CW)

#include "pch.hpp"
#include <raylib.h>
#include <rlgl.h>
#include "Assets.hpp"
#include "render.hpp"
#include "math.hpp"

static Matrix2D make_matrix(float tx, float ty, float rot_deg = 0.f, float sx = 1.f, float sy = 1.f)
{
    float r = rot_deg * DEG2RAD;
    float c = cosf(r), s = sinf(r);
    Matrix2D m;
    m.a  = sx * c;
    m.b  = sx * s;
    m.c  = sy * -s;
    m.d  = sy * c;
    m.tx = tx;
    m.ty = ty;
    return m;
}

int main()
{
    InitWindow(900, 650, "Atlas clip test - our RenderTransformFlipClipOffset");
    SetTargetFPS(60);
    rlDisableBackfaceCulling();

    // Bootstrap GraphLib (creates default graph at slot 0)
    GraphLib::Instance().create();

    // Load raptor atlas as graph 1
    int atlas_id = GraphLib::Instance().load("raptor", "../assets/spine/raptor.png");

    // mouth-smile: rotate:true, xy:992,257, size:47,30
    // packed in texture as 30w x 47h
    float S = 4.f;   // zoom scale so we can see clearly

    Texture2D* tex = GraphLib::Instance().getTexture(
        GraphLib::Instance().getGraph(atlas_id)->texture);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        // ── A) Raw packed clip, draw as 30×47 (what the atlas actually stored)
        {
            Rectangle clip = {992, 257, 30, 47};
            Matrix2D m = make_matrix(80, 80);
            RenderTransformFlipClipOffset(*tex, {0,0}, 30*S, 47*S, clip, false, false, WHITE, &m, 0);
            DrawText("A: packed 30x47", 80, 80 + 47*S + 5, 18, YELLOW);
        }

        // ── B) Packed clip, draw as 47×30 (stretch to original dims — no rotation)
        {
            Rectangle clip = {992, 257, 30, 47};
            Matrix2D m = make_matrix(80, 300);
            RenderTransformFlipClipOffset(*tex, {0,0}, 47*S, 30*S, clip, false, false, WHITE, &m, 0);
            DrawText("B: packed->47x30 stretch", 80, 300 + 30*S + 5, 18, YELLOW);
        }

        // ── C) Packed clip, draw 30×47 but sprite rotated -90°
        {
            Rectangle clip = {992, 257, 30, 47};
            float pw = 30*S, ph = 47*S;
            // pivot = centre of packed rect
            Matrix2D m = make_matrix(300 + pw*0.5f, 80 + ph*0.5f, -90.f);
            RenderTransformFlipClipOffset(*tex, {-pw*0.5f, -ph*0.5f}, pw, ph, clip, true, true, WHITE, &m, 0);
            DrawText("C: packed rot -90", 300, 80 + ph + 5, 18, YELLOW);
        }

        // ── D) Spine UV trick: remap UVs manually for rotated region
        //    setUVs(rotate=true): UL=(u2,v2) UR=(u,v2) BR=(u,v) BL=(u2,v)
        //    maps packed rect onto a 47×30 quad
        {
            float texW = (float)tex->width, texH = (float)tex->height;
            float u  = 992.f / texW, v  = 257.f / texH;
            float u2 = (992.f+30.f) / texW, v2 = (257.f+47.f) / texH;

            float x1 = 300.f, y1 = 300.f;
            float x2 = x1 + 47*S, y2 = y1 + 30*S;

            rQuad q;
            q.tex   = *tex;
            q.blend = 0;
            // spine setUVs rotate=true: UL=(u2,v2) UR=(u,v2) BR=(u,v) BL=(u2,v)
            q.v[1] = {x1, y1, 0, WHITE, u2, v2}; // TL
            q.v[0] = {x1, y2, 0, WHITE, u2, v };  // BL
            q.v[3] = {x2, y2, 0, WHITE, u,  v };  // BR
            q.v[2] = {x2, y1, 0, WHITE, u,  v2};  // TR
            RenderQuad(&q);
            DrawText("D: spine UV trick 47x30", 300, y2 + 5, 18, YELLOW);
        }

        DrawText("ESC = exit", 10, 625, 18, LIGHTGRAY);
        EndDrawing();
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
