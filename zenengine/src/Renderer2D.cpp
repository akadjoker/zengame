#include "pch.hpp"
#include "Renderer2D.hpp"
#include "SceneTree.hpp"
#include "View2D.hpp"
#include "Light2D.hpp"
#include "ShadowCaster2D.hpp"
#include <rlgl.h>
 

// ─────────────────────────────────────────────────────────────────────────────
// Platform shader sources
// ─────────────────────────────────────────────────────────────────────────────

#if defined(PLATFORM_WEB)
#  define GLSL_VERSION      "#version 300 es\nprecision mediump float;\n"
#  define GLSL_FRAG_OUT     "out vec4 finalColor;\n"
#  define GLSL_TEXTURE      "texture"
#  define GLSL_FRAG_COLOR   "finalColor"
#elif defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
#  define GLSL_VERSION      "precision mediump float;\n"
#  define GLSL_FRAG_OUT     ""
#  define GLSL_TEXTURE      "texture2D"
#  define GLSL_FRAG_COLOR   "gl_FragColor"
#else
#  define GLSL_VERSION      "#version 330 core\n"
#  define GLSL_FRAG_OUT     "out vec4 finalColor;\n"
#  define GLSL_TEXTURE      "texture"
#  define GLSL_FRAG_COLOR   "finalColor"
#endif

// ─── Shared vert ─────────────────────────────────────────────────────────────

static const char *PASSTHROUGH_VERT =
    GLSL_VERSION
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    "attribute vec3 vertexPosition;\n"
    "attribute vec2 vertexTexCoord;\n"
    "attribute vec4 vertexColor;\n"
    "uniform   mat4 mvp;\n"
    "varying   vec2 fragUV;\n"
#else
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragUV;\n"
#endif
    "void main(){\n"
    "    fragUV = vertexTexCoord;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

// ─── Light fragment ───────────────────────────────────────────────────────────
// Draws ONE light and samples the shadow mask generated for THAT SAME light.
// Shadow is softened with a tiny 5-tap blur.

static const char *LIGHT_FRAG =
    GLSL_VERSION
    GLSL_FRAG_OUT
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    "varying vec2 fragUV;\n"
    "float atan2(float y, float x){\n"
    "    if(abs(x) < 0.0001) return (y >= 0.0) ? 1.5707963 : -1.5707963;\n"
    "    float a = atan(y / x);\n"
    "    if(x < 0.0) a += (y >= 0.0) ? 3.14159265 : -3.14159265;\n"
    "    return a;\n"
    "}\n"
#else
    "in vec2 fragUV;\n"
#endif
    "uniform vec2  u_lightPos;\n"
    "uniform vec4  u_lightColor;\n"
    "uniform float u_lightRadius;\n"
    "uniform float u_lightIntensity;\n"
    "uniform vec2  u_screenSize;\n"
    "uniform bool  u_isSpot;\n"
    "uniform float u_spotDir;\n"
    "uniform float u_spotAngle;\n"
    "uniform sampler2D u_shadowTex;\n"
    "\n"
    "void main(){\n"
    "    vec2 px = fragUV * u_screenSize;\n"
    "    vec2 delta = px - u_lightPos;\n"
    "    float dist = length(delta);\n"
    "\n"
    "    float radius = max(u_lightRadius, 0.0001);\n"
    "    float t = clamp(dist / radius, 0.0, 1.0);\n"
    "\n"
    "    float falloff = 1.0 - t * t;\n"
    "    falloff *= falloff;\n"
    "\n"
    "    float spot = 1.0;\n"
    "    if(u_isSpot){\n"
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    "        float ang = atan2(delta.y, delta.x);\n"
#else
    "        float ang = atan(delta.y, delta.x);\n"
#endif
    "        float PI = acos(-1.0);\n"
    "        float diff = abs(mod(ang - u_spotDir + PI, 2.0 * PI) - PI);\n"
    "        spot = 1.0 - smoothstep(u_spotAngle * 0.8, u_spotAngle, diff);\n"
    "    }\n"
    "\n"
    "    vec2 shadowUV = vec2(fragUV.x, 1.0 - fragUV.y);\n"
    "    vec2 texel = 1.0 / max(u_screenSize, vec2(1.0));\n"
    "\n"
    "    float blur = 2.0;\n"
    "    float shadow = 0.0;\n"
    "    shadow += " GLSL_TEXTURE "(u_shadowTex, shadowUV).a * 0.40;\n"
    "    shadow += " GLSL_TEXTURE "(u_shadowTex, shadowUV + texel * vec2( blur,  0.0)).a * 0.15;\n"
    "    shadow += " GLSL_TEXTURE "(u_shadowTex, shadowUV + texel * vec2(-blur,  0.0)).a * 0.15;\n"
    "    shadow += " GLSL_TEXTURE "(u_shadowTex, shadowUV + texel * vec2( 0.0,  blur)).a * 0.15;\n"
    "    shadow += " GLSL_TEXTURE "(u_shadowTex, shadowUV + texel * vec2( 0.0, -blur)).a * 0.15;\n"
    "    shadow = clamp(shadow, 0.0, 1.0);\n"
    "\n"
    "    float shadowStrength = 0.85;\n"
    "    float amount = falloff * spot * u_lightIntensity * (1.0 - shadow * shadowStrength);\n"
    "\n"
    "    vec4 c = u_lightColor * amount;\n"
    "    c.a = clamp(c.a, 0.0, 1.0);\n"
    "\n"
    "    " GLSL_FRAG_COLOR " = c;\n"
    "}\n";

// ─── Shadow fragment ──────────────────────────────────────────────────────────
// Draws alpha only where the pixel lies inside the shadow trapezoid of one edge.

static const char *SHADOW_FRAG =
    GLSL_VERSION
    GLSL_FRAG_OUT
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    "varying vec2 fragUV;\n"
#else
    "in vec2 fragUV;\n"
#endif
    "uniform vec2  u_lightPos;\n"
    "uniform float u_lightRadius;\n"
    "uniform vec2  u_segA;\n"
    "uniform vec2  u_segB;\n"
    "uniform vec2  u_screenSize;\n"
    "\n"
    "float edge(vec2 a, vec2 b, vec2 p){\n"
    "    return sign((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x));\n"
    "}\n"
    "\n"
    "void main(){\n"
    "    vec2 px = fragUV * u_screenSize;\n"
    "\n"
    "    float radius = max(u_lightRadius, 0.0001);\n"
    "    float ext = radius * 3.0;\n"
    "\n"
    "    vec2 dirA = u_segA - u_lightPos;\n"
    "    vec2 dirB = u_segB - u_lightPos;\n"
    "\n"
    "    if(length(dirA) < 0.0001 || length(dirB) < 0.0001){\n"
    "        " GLSL_FRAG_COLOR " = vec4(0.0, 0.0, 0.0, 0.0);\n"
    "        return;\n"
    "    }\n"
    "\n"
    "    vec2 dA = normalize(dirA) * ext;\n"
    "    vec2 dB = normalize(dirB) * ext;\n"
    "\n"
    "    vec2 farA = u_segA + dA;\n"
    "    vec2 farB = u_segB + dB;\n"
    "\n"
    "    float s0 = edge(u_segA, u_segB, px);\n"
    "    float s1 = edge(u_segB, farB,   px);\n"
    "    float s2 = edge(farB,   farA,   px);\n"
    "    float s3 = edge(farA,   u_segA, px);\n"
    "\n"
    "    bool inside = (s0 >= 0.0 && s1 >= 0.0 && s2 >= 0.0 && s3 >= 0.0)\n"
    "               || (s0 <= 0.0 && s1 <= 0.0 && s2 <= 0.0 && s3 <= 0.0);\n"
    "\n"
    "    float dist = length(px - u_lightPos);\n"
    "    float fade = 1.0 - clamp(dist / radius, 0.0, 1.0);\n"
    "\n"
    "    float shadowDarkness = 0.85;\n"
    "    float alpha = inside ? fade * shadowDarkness : 0.0;\n"
    "\n"
    "    " GLSL_FRAG_COLOR " = vec4(0.0, 0.0, 0.0, alpha);\n"
    "}\n";

// ─── Composite fragment ───────────────────────────────────────────────────────
// Shadow is already baked into light_rt. Composite only applies ambient + light.

static const char *COMPOSITE_FRAG =
    GLSL_VERSION
    GLSL_FRAG_OUT
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    "varying vec2 fragUV;\n"
#else
    "in vec2 fragUV;\n"
#endif
    "uniform sampler2D u_sceneTex;\n"
    "uniform sampler2D u_lightTex;\n"
    "uniform vec4      u_ambient;\n"
    "\n"
    "void main(){\n"
    "    vec2 uv = vec2(fragUV.x, 1.0 - fragUV.y);\n"
    "\n"
    "    vec4 scene = " GLSL_TEXTURE "(u_sceneTex, uv);\n"
    "    vec4 light = " GLSL_TEXTURE "(u_lightTex, uv);\n"
    "\n"
    "    vec4 totalLight = clamp(u_ambient + light, 0.0, 1.0);\n"
    "\n"
    "    " GLSL_FRAG_COLOR " = vec4(scene.rgb * totalLight.rgb, scene.a);\n"
    "}\n";

// ─────────────────────────────────────────────────────────────────────────────
// Renderer2D
// ─────────────────────────────────────────────────────────────────────────────

Renderer2D::Renderer2D()  = default;
Renderer2D::~Renderer2D() = default;

// ── Registration ──────────────────────────────────────────────────────────────

void Renderer2D::register_light(Light2D *l)
{
    if (l) m_lights.push_back(l);
}

void Renderer2D::unregister_light(Light2D *l)
{
    m_lights.erase(std::remove(m_lights.begin(), m_lights.end(), l), m_lights.end());
}

void Renderer2D::register_shadow_caster(ShadowCaster2D *c)
{
    if (c) m_casters.push_back(c);
}

void Renderer2D::unregister_shadow_caster(ShadowCaster2D *c)
{
    m_casters.erase(std::remove(m_casters.begin(), m_casters.end(), c), m_casters.end());
}

// ── Color helpers ─────────────────────────────────────────────────────────────

void Renderer2D::set_clear_color(Color color)   { clear_color   = color; }
void Renderer2D::set_ambient_color(Color color) { ambient_color = color; }

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void Renderer2D::shutdown()
{
    shutdown_targets();
}

// ── FX ────────────────────────────────────────────────────────────────────────

void Renderer2D::fade_in(float duration)
{
    if (duration <= 0.0f)
    {
        m_fade_alpha = m_fade_target_alpha = 0.0f;
        m_fade_duration = m_fade_elapsed = 0.0f;
        return;
    }

    m_fade_start_alpha  = 255.0f;
    m_fade_alpha        = 255.0f;
    m_fade_target_alpha = 0.0f;
    m_fade_duration     = duration;
    m_fade_elapsed      = 0.0f;
    m_fade_color        = BLACK;
}

void Renderer2D::fade_out(float duration)
{
    if (duration <= 0.0f)
    {
        m_fade_alpha = m_fade_target_alpha = 255.0f;
        m_fade_duration = m_fade_elapsed = 0.0f;
        return;
    }

    m_fade_start_alpha  = m_fade_alpha;
    m_fade_target_alpha = 255.0f;
    m_fade_duration     = duration;
    m_fade_elapsed      = 0.0f;
    m_fade_color        = BLACK;
}

void Renderer2D::flash(Color color, float duration, float alpha)
{
    m_flash_color       = color;
    m_flash_alpha       = alpha;
    m_flash_start_alpha = alpha;
    m_flash_duration    = duration;
    m_flash_elapsed     = 0.0f;
}

void Renderer2D::clear_fx()
{
    m_fade_alpha = m_fade_start_alpha = m_fade_target_alpha = 0.0f;
    m_fade_duration = m_fade_elapsed = 0.0f;

    m_flash_alpha = m_flash_start_alpha = 0.0f;
    m_flash_duration = m_flash_elapsed = 0.0f;
}

void Renderer2D::update(float dt)
{
    if (m_fade_duration > 0.0f && m_fade_alpha != m_fade_target_alpha)
    {
        m_fade_elapsed += dt;
        const float t = std::min(m_fade_elapsed / m_fade_duration, 1.0f);
        m_fade_alpha  = m_fade_start_alpha + (m_fade_target_alpha - m_fade_start_alpha) * t;
    }

    if (m_flash_alpha > 0.0f)
    {
        if (m_flash_duration <= 0.0f)
        {
            m_flash_alpha = 0.0f;
        }
        else
        {
            m_flash_elapsed += dt;
            const float t = std::min(m_flash_elapsed / m_flash_duration, 1.0f);
            m_flash_alpha = (1.0f - t) * m_flash_start_alpha;
        }
    }
}

// ── Frame ─────────────────────────────────────────────────────────────────────

void Renderer2D::begin_frame() const
{
    BeginDrawing();
    ClearBackground(clear_color);
}

void Renderer2D::end_frame() const
{
    EndDrawing();
}

void Renderer2D::render(SceneTree &tree)
{
    ensure_targets();
    begin_frame();

    const bool do_lights = lights_enabled && m_rt_ready && !m_lights.empty();

    if (do_lights)
    {
        BeginTextureMode(m_scene_rt);
        ClearBackground(clear_color);
        tree.draw_world_pass();
        EndTextureMode();

        render_light_pass(tree);
        render_composite_pass();
    }
    else
    {
        tree.draw_world_pass();
    }

    tree.draw_ui_pass();
    tree.draw_overlay_pass();

    draw_fullscreen_overlay(m_flash_color, m_flash_alpha);
    draw_fullscreen_overlay(m_fade_color,  m_fade_alpha);

    end_frame();
}

void Renderer2D::draw_fullscreen_overlay(Color color, float alpha) const
{
    if (alpha <= 0.0f) return;

    color.a = static_cast<unsigned char>(std::min(alpha, 255.0f));
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), color);
}

// ── Render target management ──────────────────────────────────────────────────

void Renderer2D::ensure_targets()
{
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    if (sw <= 0 || sh <= 0)
        return;

    if (m_rt_ready && sw == m_rt_width && sh == m_rt_height)
        return;

    shutdown_targets();

    m_rt_width  = sw;
    m_rt_height = sh;

    m_scene_rt  = LoadRenderTexture(sw, sh);
    m_light_rt  = LoadRenderTexture(sw, sh);
    m_shadow_rt = LoadRenderTexture(sw, sh);

    SetTextureFilter(m_scene_rt.texture,  TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(m_light_rt.texture,  TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(m_shadow_rt.texture, TEXTURE_FILTER_BILINEAR);

    build_light_shader();
    build_shadow_shader();

    m_rt_ready = true;
}

void Renderer2D::build_light_shader()
{
    m_light_shader     = LoadShaderFromMemory(PASSTHROUGH_VERT, LIGHT_FRAG);
    m_composite_shader = LoadShaderFromMemory(PASSTHROUGH_VERT, COMPOSITE_FRAG);

    m_u_light_pos        = GetShaderLocation(m_light_shader, "u_lightPos");
    m_u_light_color      = GetShaderLocation(m_light_shader, "u_lightColor");
    m_u_light_radius     = GetShaderLocation(m_light_shader, "u_lightRadius");
    m_u_light_intensity  = GetShaderLocation(m_light_shader, "u_lightIntensity");
    m_u_screen_size      = GetShaderLocation(m_light_shader, "u_screenSize");
    m_u_is_spot          = GetShaderLocation(m_light_shader, "u_isSpot");
    m_u_spot_dir         = GetShaderLocation(m_light_shader, "u_spotDir");
    m_u_spot_angle       = GetShaderLocation(m_light_shader, "u_spotAngle");
    m_u_light_shadow_tex = GetShaderLocation(m_light_shader, "u_shadowTex");

    m_u_scene_tex = GetShaderLocation(m_composite_shader, "u_sceneTex");
    m_u_light_tex = GetShaderLocation(m_composite_shader, "u_lightTex");
    m_u_ambient   = GetShaderLocation(m_composite_shader, "u_ambient");
}

void Renderer2D::build_shadow_shader()
{
    m_shadow_shader = LoadShaderFromMemory(PASSTHROUGH_VERT, SHADOW_FRAG);

    m_u_sh_light_pos    = GetShaderLocation(m_shadow_shader, "u_lightPos");
    m_u_sh_light_radius = GetShaderLocation(m_shadow_shader, "u_lightRadius");
    m_u_sh_seg_a        = GetShaderLocation(m_shadow_shader, "u_segA");
    m_u_sh_seg_b        = GetShaderLocation(m_shadow_shader, "u_segB");
    m_u_sh_screen_size  = GetShaderLocation(m_shadow_shader, "u_screenSize");
}

void Renderer2D::shutdown_targets()
{
    if (m_scene_rt.id != 0)
    {
        UnloadRenderTexture(m_scene_rt);
        m_scene_rt = {};
    }

    if (m_light_rt.id != 0)
    {
        UnloadRenderTexture(m_light_rt);
        m_light_rt = {};
    }

    if (m_shadow_rt.id != 0)
    {
        UnloadRenderTexture(m_shadow_rt);
        m_shadow_rt = {};
    }

    if (m_light_shader.id != 0)
    {
        UnloadShader(m_light_shader);
        m_light_shader = {};
    }

    if (m_composite_shader.id != 0)
    {
        UnloadShader(m_composite_shader);
        m_composite_shader = {};
    }

    if (m_shadow_shader.id != 0)
    {
        UnloadShader(m_shadow_shader);
        m_shadow_shader = {};
    }

    m_rt_ready = false;
}

// ── Light pass ────────────────────────────────────────────────────────────────
// Correct pipeline:
//   clear light_rt once
//   for each light:
//       clear shadow_rt
//       render shadows for this light only
//       render this light with this shadow texture into light_rt additively

void Renderer2D::render_light_pass(SceneTree &tree)
{
    View2D *cam = tree.get_current_camera();

    const float zoom = cam ? cam->zoom : 1.0f;
    const float sw   = static_cast<float>(m_rt_width);
    const float sh   = static_cast<float>(m_rt_height);

    const float screen_size[2] = { sw, sh };

    BeginTextureMode(m_light_rt);
    ClearBackground(BLANK);
    EndTextureMode();

    for (Light2D *light : m_lights)
    {
        if (!light || !light->enabled || !light->visible || !light->active)
            continue;

        BeginTextureMode(m_shadow_rt);
        ClearBackground(BLANK);
        EndTextureMode();

        if (shadows_enabled && light->cast_shadows && !m_casters.empty())
            render_shadow_pass_for_light(tree, light);

        const Vec2  wpos   = light->get_global_position();
        const Vec2  spos   = cam ? cam->world_to_screen(wpos) : wpos;
        const float radius = light->radius * zoom;

        if (radius <= 0.0f)
            continue;

        const float pos[2] = { spos.x, spos.y };

        const float col[4] = {
            light->color.r / 255.0f,
            light->color.g / 255.0f,
            light->color.b / 255.0f,
            light->color.a / 255.0f
        };

        int   is_spot   = (light->type == Light2D::Type::Spot) ? 1 : 0;
        float intensity = light->intensity;
        float dir_rad   = (light->spot_dir + light->get_global_rotation()) * DEG_TO_RAD;
        float angle_rad = light->spot_angle * DEG_TO_RAD;

        const float x0 = spos.x - radius;
        const float y0 = spos.y - radius;
        const float x1 = spos.x + radius;
        const float y1 = spos.y + radius;

        const float u0 = x0 / sw;
        const float v0 = y0 / sh;
        const float u1 = x1 / sw;
        const float v1 = y1 / sh;

        BeginTextureMode(m_light_rt);
        BeginShaderMode(m_light_shader);
        rlSetBlendMode(BLEND_ADDITIVE);

        SetShaderValue(m_light_shader, m_u_light_pos,        pos,         SHADER_UNIFORM_VEC2);
        SetShaderValue(m_light_shader, m_u_light_color,      col,         SHADER_UNIFORM_VEC4);
        SetShaderValue(m_light_shader, m_u_light_radius,     &radius,     SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_light_shader, m_u_light_intensity,  &intensity,  SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_light_shader, m_u_screen_size,      screen_size, SHADER_UNIFORM_VEC2);
        SetShaderValue(m_light_shader, m_u_is_spot,          &is_spot,    SHADER_UNIFORM_INT);
        SetShaderValue(m_light_shader, m_u_spot_dir,         &dir_rad,    SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_light_shader, m_u_spot_angle,       &angle_rad,  SHADER_UNIFORM_FLOAT);
        SetShaderValueTexture(m_light_shader, m_u_light_shadow_tex, m_shadow_rt.texture);

        rlBegin(RL_QUADS);
            rlTexCoord2f(u0, v0); rlVertex3f(x0, y0, 0.0f);
            rlTexCoord2f(u0, v1); rlVertex3f(x0, y1, 0.0f);
            rlTexCoord2f(u1, v1); rlVertex3f(x1, y1, 0.0f);
            rlTexCoord2f(u1, v0); rlVertex3f(x1, y0, 0.0f);
        rlEnd();

        rlDrawRenderBatchActive();

        rlSetBlendMode(BLEND_ALPHA);
        EndShaderMode();
        EndTextureMode();
    }
}

// ── Shadow pass for one light ────────────────────────────────────────────────

void Renderer2D::render_shadow_pass_for_light(SceneTree &tree, Light2D *light)
{
    if (!light)
        return;

    View2D *cam = tree.get_current_camera();

    const float zoom = cam ? cam->zoom : 1.0f;
    const float sw   = static_cast<float>(m_rt_width);
    const float sh   = static_cast<float>(m_rt_height);

    const float screen_size[2] = { sw, sh };

    const Vec2  light_world = light->get_global_position();
    const Vec2  light_pos   = cam ? cam->world_to_screen(light_world) : light_world;
    const float radius      = light->radius * zoom;

    if (radius <= 0.0f)
        return;

    const float lpos[2] = { light_pos.x, light_pos.y };

    BeginTextureMode(m_shadow_rt);
    BeginShaderMode(m_shadow_shader);
    rlSetBlendMode(BLEND_ALPHA);

    SetShaderValue(m_shadow_shader, m_u_sh_light_pos,    lpos,        SHADER_UNIFORM_VEC2);
    SetShaderValue(m_shadow_shader, m_u_sh_light_radius, &radius,     SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shadow_shader, m_u_sh_screen_size,  screen_size, SHADER_UNIFORM_VEC2);

    for (ShadowCaster2D *caster : m_casters)
    {
        if (!caster || !caster->enabled || !caster->visible || !caster->active)
            continue;

        auto world_segments = caster->get_world_segments();

        for (size_t i = 0; i + 1 < world_segments.size(); i += 2)
        {
            const Vec2 sA = cam ? cam->world_to_screen(world_segments[i])     : world_segments[i];
            const Vec2 sB = cam ? cam->world_to_screen(world_segments[i + 1]) : world_segments[i + 1];

            const Vec2 toA = sA - light_pos;
            const Vec2 toB = sB - light_pos;

            const float lenA2 = toA.x * toA.x + toA.y * toA.y;
            const float lenB2 = toB.x * toB.x + toB.y * toB.y;

            if (lenA2 < 0.0001f || lenB2 < 0.0001f)
                continue;

            // Silhouette culling. If shadows vanish, flip this sign or comment it.
            const float cx = (sB.x - sA.x) * (light_pos.y - sA.y)
                           - (sB.y - sA.y) * (light_pos.x - sA.x);

            if (cx < 0.0f)
                continue;

            const float ext = radius * 3.0f;

            const Vec2 dA   = Vec2::Normalize(sA - light_pos) * ext;
            const Vec2 dB   = Vec2::Normalize(sB - light_pos) * ext;
            const Vec2 farA = sA + dA;
            const Vec2 farB = sB + dB;

            const float sa[2] = { sA.x, sA.y };
            const float sb[2] = { sB.x, sB.y };

            SetShaderValue(m_shadow_shader, m_u_sh_seg_a, sa, SHADER_UNIFORM_VEC2);
            SetShaderValue(m_shadow_shader, m_u_sh_seg_b, sb, SHADER_UNIFORM_VEC2);

            const float u0 = sA.x   / sw;
            const float v0 = sA.y   / sh;
            const float u1 = sB.x   / sw;
            const float v1 = sB.y   / sh;
            const float u2 = farB.x / sw;
            const float v2 = farB.y / sh;
            const float u3 = farA.x / sw;
            const float v3 = farA.y / sh;

            rlBegin(RL_QUADS);
                rlTexCoord2f(u0, v0); rlVertex3f(sA.x,   sA.y,   0.0f);
                rlTexCoord2f(u1, v1); rlVertex3f(sB.x,   sB.y,   0.0f);
                rlTexCoord2f(u2, v2); rlVertex3f(farB.x, farB.y, 0.0f);
                rlTexCoord2f(u3, v3); rlVertex3f(farA.x, farA.y, 0.0f);
            rlEnd();

            rlDrawRenderBatchActive();
        }
    }

    rlSetBlendMode(BLEND_ALPHA);
    EndShaderMode();
    EndTextureMode();
}

// ── Composite pass ────────────────────────────────────────────────────────────

void Renderer2D::render_composite_pass()
{
    const float ambient[4] = {
        ambient_color.r / 255.0f,
        ambient_color.g / 255.0f,
        ambient_color.b / 255.0f,
        ambient_color.a / 255.0f
    };

    BeginShaderMode(m_composite_shader);

    SetShaderValueTexture(m_composite_shader, m_u_scene_tex, m_scene_rt.texture);
    SetShaderValueTexture(m_composite_shader, m_u_light_tex, m_light_rt.texture);
    SetShaderValue(m_composite_shader, m_u_ambient, ambient, SHADER_UNIFORM_VEC4);

    rlBegin(RL_QUADS);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(0.0f,              0.0f,               0.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(0.0f,              (float)m_rt_height, 0.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f((float)m_rt_width, (float)m_rt_height, 0.0f);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f((float)m_rt_width, 0.0f,               0.0f);
    rlEnd();

    EndShaderMode();

    if (debug_lights)
    {
        DrawTextureRec(
            m_light_rt.texture,
            Rectangle{0, 0, (float)m_rt_width, -(float)m_rt_height},
            Vector2{0, 0},
            Color{255, 255, 255, 100}
        );
    }

    if (debug_shadows)
    {
        DrawTextureRec(
            m_shadow_rt.texture,
            Rectangle{0, 0, (float)m_rt_width, -(float)m_rt_height},
            Vector2{0, 0},
            Color{255, 100, 100, 160}
        );
    }
}
