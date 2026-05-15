#pragma once

#include "pch.hpp"

class SceneTree;
class Light2D;
class ShadowCaster2D;
class View2D;

class Renderer2D
{
public:
    Color clear_color   = BLACK;
    Color ambient_color = Color{200, 200, 200, 255};

    bool lights_enabled  = true;
    bool shadows_enabled = true;
    bool debug_lights    = false;
    bool debug_shadows   = false;

    Renderer2D();
    ~Renderer2D();

    void set_clear_color(Color color);
    void set_ambient_color(Color color);
    void shutdown();

    void fade_in(float duration);
    void fade_out(float duration);
    void flash(Color color, float duration, float alpha = 255.0f);
    void clear_fx();
    void update(float dt);

    void begin_frame() const;
    void end_frame() const;
    void render(SceneTree &tree);

    void register_light(Light2D *l);
    void unregister_light(Light2D *l);

    void register_shadow_caster(ShadowCaster2D *c);
    void unregister_shadow_caster(ShadowCaster2D *c);

private:
    // ── Render targets ────────────────────────────────────────────────────────
    RenderTexture2D m_scene_rt  = {};
    RenderTexture2D m_light_rt  = {};
    RenderTexture2D m_shadow_rt = {};

    Shader m_light_shader     = {};
    Shader m_composite_shader = {};
    Shader m_shadow_shader    = {};

    int  m_rt_width  = 0;
    int  m_rt_height = 0;
    bool m_rt_ready  = false;

    // ── Uniform locations — light shader ─────────────────────────────────────
    int m_u_light_pos        = -1;
    int m_u_light_color      = -1;
    int m_u_light_radius     = -1;
    int m_u_light_intensity  = -1;
    int m_u_screen_size      = -1;
    int m_u_is_spot          = -1;
    int m_u_spot_dir         = -1;
    int m_u_spot_angle       = -1;
    int m_u_light_shadow_tex = -1;

    // ── Uniform locations — composite shader ─────────────────────────────────
    int m_u_scene_tex = -1;
    int m_u_light_tex = -1;
    int m_u_ambient   = -1;

    // ── Uniform locations — shadow shader ────────────────────────────────────
    int m_u_sh_light_pos    = -1;
    int m_u_sh_light_radius = -1;
    int m_u_sh_seg_a        = -1;
    int m_u_sh_seg_b        = -1;
    int m_u_sh_screen_size  = -1;

    // ── Scene objects ─────────────────────────────────────────────────────────
    std::vector<Light2D *>        m_lights;
    std::vector<ShadowCaster2D *> m_casters;

    // ── FX state ──────────────────────────────────────────────────────────────
    float m_fade_alpha        = 0.0f;
    float m_fade_start_alpha  = 0.0f;
    float m_fade_target_alpha = 0.0f;
    float m_fade_duration     = 0.0f;
    float m_fade_elapsed      = 0.0f;
    Color m_fade_color        = BLACK;

    float m_flash_alpha       = 0.0f;
    float m_flash_start_alpha = 0.0f;
    float m_flash_duration    = 0.0f;
    float m_flash_elapsed     = 0.0f;
    Color m_flash_color       = WHITE;

    // ── Internals ─────────────────────────────────────────────────────────────
    void ensure_targets();
    void shutdown_targets();
    void build_light_shader();
    void build_shadow_shader();

    void render_light_pass(SceneTree &tree);
    void render_shadow_pass_for_light(View2D* cam, Light2D* light);
    void render_composite_pass();
    void draw_fullscreen_overlay(Color color, float alpha) const;
};
