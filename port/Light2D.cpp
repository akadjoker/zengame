#include "Light2D.hpp"
#include <rlgl.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <cassert>

// ─────────────────────────────────────────────────────────────────────────────
//  GLSL — light accumulation
//  Desenha um quad sobre o ecrã; calcula falloff por fragmento.
// ─────────────────────────────────────────────────────────────────────────────
static const char* LIGHT_VERT = R"glsl(
#version 330 core
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

uniform mat4 mvp;
out vec2 fragUV;

void main()
{
    fragUV      = vertexTexCoord;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)glsl";

static const char* LIGHT_FRAG = R"glsl(
#version 330 core
in vec2 fragUV;
out vec4 finalColor;

uniform vec2  u_lightPos;       // pixels, origem top-left
uniform vec4  u_lightColor;     // rgba [0,1]
uniform float u_lightRadius;
uniform float u_lightIntensity;
uniform vec2  u_screenSize;

// Spot
uniform bool  u_isSpot;
uniform float u_spotDir;        // radianos
uniform float u_spotAngle;      // meio-ângulo, radianos

const float PI = 3.14159265;

void main()
{
    // UV → pixel coords (Y não precisa de flip porque vamos usar
    // DrawTexturePro com flip na composição)
    vec2 pixelPos = fragUV * u_screenSize;
    vec2 delta    = pixelPos - u_lightPos;
    float dist    = length(delta);

    // Falloff quadrático suave
    float t       = clamp(dist / u_lightRadius, 0.0, 1.0);
    float falloff = 1.0 - t * t;
    falloff       = falloff * falloff;   // quartic = mais suave nas bordas

    // Spot mask
    float spot_mask = 1.0;
    if (u_isSpot)
    {
        float ang   = atan(delta.y, delta.x);
        float diff  = abs(mod(ang - u_spotDir + PI, 2.0 * PI) - PI);
        spot_mask   = 1.0 - smoothstep(u_spotAngle * 0.8, u_spotAngle, diff);
    }

    finalColor = u_lightColor * (falloff * spot_mask * u_lightIntensity);
    finalColor.a = clamp(finalColor.a, 0.0, 1.0);
}
)glsl";

// ─────────────────────────────────────────────────────────────────────────────
//  GLSL — composite  (scene * (ambient + lights))
// ─────────────────────────────────────────────────────────────────────────────
static const char* COMPOSITE_VERT = R"glsl(
#version 330 core
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

uniform mat4 mvp;
out vec2 fragUV;

void main()
{
    fragUV      = vertexTexCoord;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)glsl";

static const char* COMPOSITE_FRAG = R"glsl(
#version 330 core
in vec2 fragUV;
out vec4 finalColor;

uniform sampler2D u_sceneTex;
uniform sampler2D u_lightTex;
uniform vec4      u_ambient;    // [0,1]

void main()
{
    // RenderTexture tem Y invertido no Raylib
    vec2 uvScene = vec2(fragUV.x, 1.0 - fragUV.y);
    vec2 uvLight = vec2(fragUV.x, 1.0 - fragUV.y);

    vec4 scene = texture(u_sceneTex, uvScene);
    vec4 light = texture(u_lightTex, uvLight);

    // Multiplicativo: escurece onde não há luz
    vec4 total_light = clamp(u_ambient + light, 0.0, 1.0);
    finalColor = vec4(scene.rgb * total_light.rgb, scene.a);
}
)glsl";

// ─────────────────────────────────────────────────────────────────────────────

LightSystem2D& LightSystem2D::Instance()
{
    static LightSystem2D inst;
    return inst;
}

void LightSystem2D::init(int width, int height)
{
    width_  = width;
    height_ = height;

    scene_buf_ = LoadRenderTexture(width, height);
    light_buf_ = LoadRenderTexture(width, height);

    SetTextureFilter(scene_buf_.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(light_buf_.texture, TEXTURE_FILTER_BILINEAR);

    build_shaders();
    ready_ = true;
}

void LightSystem2D::shutdown()
{
    if (!ready_) return;
    UnloadRenderTexture(scene_buf_);
    UnloadRenderTexture(light_buf_);
    UnloadShader(light_shader_);
    UnloadShader(composite_shader_);
    ready_ = false;
}

void LightSystem2D::build_shaders()
{
    light_shader_     = LoadShaderFromMemory(LIGHT_VERT, LIGHT_FRAG);
    composite_shader_ = LoadShaderFromMemory(COMPOSITE_VERT, COMPOSITE_FRAG);

    // — light uniforms —
    u_light_pos_       = GetShaderLocation(light_shader_, "u_lightPos");
    u_light_color_     = GetShaderLocation(light_shader_, "u_lightColor");
    u_light_radius_    = GetShaderLocation(light_shader_, "u_lightRadius");
    u_light_intensity_ = GetShaderLocation(light_shader_, "u_lightIntensity");
    u_screen_size_     = GetShaderLocation(light_shader_, "u_screenSize");
    u_is_spot_         = GetShaderLocation(light_shader_, "u_isSpot");
    u_spot_dir_        = GetShaderLocation(light_shader_, "u_spotDir");
    u_spot_angle_      = GetShaderLocation(light_shader_, "u_spotAngle");

    // — composite uniforms —
    u_scene_tex_ = GetShaderLocation(composite_shader_, "u_sceneTex");
    u_light_tex_ = GetShaderLocation(composite_shader_, "u_lightTex");
    u_ambient_   = GetShaderLocation(composite_shader_, "u_ambient");
}

void LightSystem2D::register_light(Light2D* l)
{
    if (!l) return;
    if (std::find(lights_.begin(), lights_.end(), l) == lights_.end())
        lights_.push_back(l);
}

void LightSystem2D::unregister_light(Light2D* l)
{
    lights_.erase(std::remove(lights_.begin(), lights_.end(), l), lights_.end());
}

// ─────────────────────────────────────────────────────────────────────────────

void LightSystem2D::begin_scene()
{
    assert(ready_);
    BeginTextureMode(scene_buf_);
    ClearBackground(BLANK);
}

void LightSystem2D::end_scene()
{
    EndTextureMode();
}

// ─────────────────────────────────────────────────────────────────────────────

void LightSystem2D::draw_point_light(const Light2D& l)
{
    const float sw = (float)width_;
    const float sh = (float)height_;

    // Quad centrado na luz com raio como metade do tamanho
    // (podíamos fazer um quad de ecrã inteiro mas é desperdício)
    const float r  = l.radius;
    const float x0 = l.position.x - r;
    const float y0 = l.position.y - r;
    const float x1 = l.position.x + r;
    const float y1 = l.position.y + r;

    // UV correspondentes (para o shader saber a posição em pixels)
    const float u0 = x0 / sw;
    const float v0 = y0 / sh;
    const float u1 = x1 / sw;
    const float v1 = y1 / sh;

    float pos[2]   = {l.position.x, l.position.y};
    float col[4]   = {(float)l.color.r / 255.0f,
                      (float)l.color.g / 255.0f,
                      (float)l.color.b / 255.0f,
                      (float)l.color.a / 255.0f};
    float size[2]  = {sw, sh};
    int   is_spot  = 0;

    SetShaderValue(light_shader_, u_light_pos_,       pos,           SHADER_UNIFORM_VEC2);
    SetShaderValue(light_shader_, u_light_color_,     col,           SHADER_UNIFORM_VEC4);
    SetShaderValue(light_shader_, u_light_radius_,    &l.radius,     SHADER_UNIFORM_FLOAT);
    SetShaderValue(light_shader_, u_light_intensity_, &l.intensity,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(light_shader_, u_screen_size_,     size,          SHADER_UNIFORM_VEC2);
    SetShaderValue(light_shader_, u_is_spot_,         &is_spot,      SHADER_UNIFORM_INT);

    // Quad manual com UV customizados (rlgl directo)
    rlBegin(RL_QUADS);
        rlTexCoord2f(u0, v0); rlVertex3f(x0, y0, 0.0f);
        rlTexCoord2f(u0, v1); rlVertex3f(x0, y1, 0.0f);
        rlTexCoord2f(u1, v1); rlVertex3f(x1, y1, 0.0f);
        rlTexCoord2f(u1, v0); rlVertex3f(x1, y0, 0.0f);
    rlEnd();
}

void LightSystem2D::draw_spot_light(const Light2D& l)
{
    const float sw  = (float)width_;
    const float sh  = (float)height_;
    const float r   = l.radius;
    const float x0  = l.position.x - r;
    const float y0  = l.position.y - r;
    const float x1  = l.position.x + r;
    const float y1  = l.position.y + r;
    const float u0  = x0 / sw, v0 = y0 / sh;
    const float u1  = x1 / sw, v1 = y1 / sh;

    const float DEG2RAD = 0.0174532925f;
    float pos[2]    = {l.position.x, l.position.y};
    float col[4]    = {(float)l.color.r / 255.0f,
                       (float)l.color.g / 255.0f,
                       (float)l.color.b / 255.0f,
                       (float)l.color.a / 255.0f};
    float size[2]   = {sw, sh};
    int   is_spot   = 1;
    float dir_rad   = l.spot_dir   * DEG2RAD;
    float angle_rad = l.spot_angle * DEG2RAD;

    SetShaderValue(light_shader_, u_light_pos_,       pos,           SHADER_UNIFORM_VEC2);
    SetShaderValue(light_shader_, u_light_color_,     col,           SHADER_UNIFORM_VEC4);
    SetShaderValue(light_shader_, u_light_radius_,    &l.radius,     SHADER_UNIFORM_FLOAT);
    SetShaderValue(light_shader_, u_light_intensity_, &l.intensity,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(light_shader_, u_screen_size_,     size,          SHADER_UNIFORM_VEC2);
    SetShaderValue(light_shader_, u_is_spot_,         &is_spot,      SHADER_UNIFORM_INT);
    SetShaderValue(light_shader_, u_spot_dir_,        &dir_rad,      SHADER_UNIFORM_FLOAT);
    SetShaderValue(light_shader_, u_spot_angle_,      &angle_rad,    SHADER_UNIFORM_FLOAT);

    rlBegin(RL_QUADS);
        rlTexCoord2f(u0, v0); rlVertex3f(x0, y0, 0.0f);
        rlTexCoord2f(u0, v1); rlVertex3f(x0, y1, 0.0f);
        rlTexCoord2f(u1, v1); rlVertex3f(x1, y1, 0.0f);
        rlTexCoord2f(u1, v0); rlVertex3f(x1, y0, 0.0f);
    rlEnd();
}

// ─────────────────────────────────────────────────────────────────────────────

void LightSystem2D::render(Color ambient)
{
    if (!ready_) return;

    const float sw = (float)width_;
    const float sh = (float)height_;

    // ── 1. Acumular luzes no light_buf_ ──────────────────────────────────────
    BeginTextureMode(light_buf_);
    ClearBackground(BLANK);

    BeginShaderMode(light_shader_);
    rlSetBlendMode(BLEND_ADDITIVE);   // luzes somam-se

    for (Light2D* l : lights_)
    {
        if (!l || !l->enabled) continue;
        if (l->type == Light2D::Type::Point) draw_point_light(*l);
        else                                 draw_spot_light(*l);
    }

    rlSetBlendMode(BLEND_ALPHA);
    EndShaderMode();
    EndTextureMode();

    // ── 2. Composite: scene * (ambient + lights) ──────────────────────────────
    // ambient normalizado
    float amb[4] = {
        (float)ambient.r / 255.0f,
        (float)ambient.g / 255.0f,
        (float)ambient.b / 255.0f,
        (float)ambient.a / 255.0f
    };

    BeginShaderMode(composite_shader_);

    // bind textures
    SetShaderValueTexture(composite_shader_, u_scene_tex_, scene_buf_.texture);
    SetShaderValueTexture(composite_shader_, u_light_tex_, light_buf_.texture);
    SetShaderValue(composite_shader_, u_ambient_, amb, SHADER_UNIFORM_VEC4);

    // full-screen quad
    DrawRectangle(0, 0, width_, height_, WHITE);

    EndShaderMode();

    // ── 3. Debug ──────────────────────────────────────────────────────────────
    if (debug_lights)
    {
        for (const Light2D* l : lights_)
        {
            if (!l || !l->enabled) continue;
            DrawCircleLinesV(l->position, l->radius, ColorAlpha(l->color, 0.4f));
            DrawCircleV(l->position, 4.0f, l->color);

            if (l->type == Light2D::Type::Spot)
            {
                const float DEG2RAD = 0.0174532925f;
                const float d = l->spot_dir * DEG2RAD;
                const float a = l->spot_angle * DEG2RAD;
                Vector2 p  = l->position;
                Vector2 d1 = {p.x + cosf(d - a) * l->radius, p.y + sinf(d - a) * l->radius};
                Vector2 d2 = {p.x + cosf(d + a) * l->radius, p.y + sinf(d + a) * l->radius};
                DrawLineV(p, d1, ColorAlpha(l->color, 0.6f));
                DrawLineV(p, d2, ColorAlpha(l->color, 0.6f));
            }
        }
    }
}
