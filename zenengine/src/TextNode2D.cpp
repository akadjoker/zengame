#include "TextNode2D.hpp"
#include <raylib.h>
#include <raymath.h>

// ----------------------------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------------------------

TextNode2D::TextNode2D(const std::string& p_name)
    : Node2D(p_name)
{
    node_type = NodeType::TextNode2D;
    // Default: use raylib's built-in font (no file required)
    m_font      = GetFontDefault();
    m_owns_font = false;
}

TextNode2D::~TextNode2D()
{
    if (m_owns_font)
    {
        UnloadFont(m_font);
        m_owns_font = false;
    }
}

// ----------------------------------------------------------------------------
// Font management
// ----------------------------------------------------------------------------

bool TextNode2D::load_font(const char* path, int glyph_size)
{
    Font f = LoadFontEx(path, glyph_size, nullptr, 0);
    if (f.texture.id == 0)
    {
        TraceLog(LOG_ERROR, "TextNode2D::load_font: failed to load '%s'", path);
        return false;
    }

    if (m_owns_font)
        UnloadFont(m_font);

    m_font      = f;
    m_owns_font = true;
    return true;
}

void TextNode2D::use_default_font()
{
    if (m_owns_font)
    {
        UnloadFont(m_font);
        m_owns_font = false;
    }
    m_font = GetFontDefault();
}

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

Vec2 TextNode2D::measure() const
{
    if (text.empty()) return {0.0f, 0.0f};
    const Vector2 sz = MeasureTextEx(m_font, text.c_str(), font_size, spacing);
    return {sz.x, sz.y};
}

// ----------------------------------------------------------------------------
// Draw
// ----------------------------------------------------------------------------

void TextNode2D::_draw()
{
    // Draw children first (consistent with Node2D convention)
    Node2D::_draw();

    if (text.empty()) return;

    // Measure text to apply pivot offset
    const Vector2 sz = MeasureTextEx(m_font, text.c_str(), font_size, spacing);

    // Pivot offset in local space: pivot {0,0}=top-left, {0.5,0.5}=centre
    const float ox = pivot.x * sz.x;
    const float oy = pivot.y * sz.y;

    // World transform
    const Matrix2D xf  = get_global_transform();
    const Vec2     pos = xf.TransformCoords(Vec2(-ox, -oy));

    // Rotation in degrees from the 2D transform (atan2 of the first column)
    const float angle = atan2f(xf.b, xf.a) * (180.0f / 3.14159265f);

    // Scale from transform
    const float sx = sqrtf(xf.a * xf.a + xf.b * xf.b);
    const float sy = sqrtf(xf.c * xf.c + xf.d * xf.d);
    const float draw_size = font_size * std::max(sx, sy);
    const float draw_spacing = spacing * std::max(sx, sy);

    // DrawTextPro handles rotation and origin correctly
    DrawTextPro(
        m_font,
        text.c_str(),
        Vector2{pos.x, pos.y},
        Vector2{0.0f, 0.0f},   // origin already baked into pos
        angle,
        draw_size,
        draw_spacing,
        color
    );
}
