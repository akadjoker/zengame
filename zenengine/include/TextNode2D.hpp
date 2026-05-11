#pragma once

#include "Node2D.hpp"
#include <raylib.h>
#include <string>

// ----------------------------------------------------------------------------
// TextNode2D
//
// Renders a text string in world space using a raylib Font.
// Inherits Node2D, so position/rotation/scale all apply.
//
// Usage:
//   auto* label = new TextNode2D("Score");
//   label->text      = "Score: 0";
//   label->font_size = 24.0f;
//   label->color     = WHITE;
//   label->load_font("assets/fonts/my_font.ttf", 64);
//   add_child(label);
// ----------------------------------------------------------------------------
class TextNode2D : public Node2D
{
public:
    explicit TextNode2D(const std::string& p_name = "TextNode2D");
    ~TextNode2D() override;

    TextNode2D(const TextNode2D&)            = delete;
    TextNode2D& operator=(const TextNode2D&) = delete;

    // ── Content ───────────────────────────────────────────────────────────────
    std::string text;

    // ── Style ─────────────────────────────────────────────────────────────────
    float font_size = 16.0f;
    float spacing   = 1.0f;   // pixels between glyphs
    Color color     = WHITE;

    // Local pivot/origin offset (applied before the world transform).
    // {0,0} = top-left,  {0.5, 0.5} = center,  {1,1} = bottom-right
    // Values are in [0..1] relative to the text bounding box.
    Vec2 pivot = {0.0f, 0.0f};

    // ── Font management ───────────────────────────────────────────────────────

    // Load a TTF/OTF font at `glyph_size` pixels (affects quality, not draw size).
    // Returns true on success. Falls back to default font on failure.
    bool load_font(const char* path, int glyph_size = 64);

    // Revert to raylib's built-in bitmap font (always available, no file needed).
    void use_default_font();

    // ── Helpers ───────────────────────────────────────────────────────────────

    // Returns the rendered text size in pixels at the current font_size.
    Vec2 measure() const;

    // ── Node2D override ───────────────────────────────────────────────────────
    void _draw() override;

private:
    Font m_font      = {};
    bool m_owns_font = false;  // true if we loaded the font (must unload)
};
