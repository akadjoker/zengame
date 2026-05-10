#pragma once
#include <raylib.h>
#include <vector>
#include <string>

struct Vec2;

// ─────────────────────────────────────────────
//  Light2D  —  nó de luz, registas no LightSystem2D
// ─────────────────────────────────────────────
struct Light2D
{
    enum class Type { Point, Spot };

    std::string name;
    Type        type      = Type::Point;
    bool        enabled   = true;

    Vector2     position  = {0.0f, 0.0f};
    Color       color     = WHITE;
    float       intensity = 1.0f;
    float       radius    = 256.0f;

    // Spot-only
    float       spot_angle  = 45.0f;   // graus, meio-ângulo do cone
    float       spot_dir    = 0.0f;    // graus, 0 = direita

    Light2D() = default;
    explicit Light2D(const std::string& p_name) : name(p_name) {}
};

// ─────────────────────────────────────────────
//  LightSystem2D  —  singleton de renderização
// ─────────────────────────────────────────────
class LightSystem2D
{
public:
    static LightSystem2D& Instance();

    // Chama uma vez ao inicio, com a resolução do ecrã
    void init(int width, int height);
    void shutdown();

    // Registo de luzes
    void register_light(Light2D* l);
    void unregister_light(Light2D* l);

    // ── pipeline ──────────────────────────────
    // 1) Desenha a tua cena aqui (antes de chamar begin_scene)
    //    depois chama begin_scene / end_scene para capturar.

    void begin_scene();      // BeginTextureMode(scene_buf)
    void end_scene();        // EndTextureMode

    // 2) Chama render() a seguir — acumula luzes e faz composite
    void render(Color ambient = {20, 20, 40, 255});

    // ── configuração ──────────────────────────
    Color ambient_color = {20, 20, 40, 255};

    bool debug_lights = false; // desenha círculos de debug

private:
    LightSystem2D() = default;

    void build_shaders();
    void draw_point_light(const Light2D& l);
    void draw_spot_light(const Light2D& l);

    int width_  = 0;
    int height_ = 0;

    RenderTexture2D scene_buf_  = {};
    RenderTexture2D light_buf_  = {};

    Shader light_shader_     = {};
    Shader composite_shader_ = {};

    // uniform locations – light shader
    int u_light_pos_      = -1;
    int u_light_color_    = -1;
    int u_light_radius_   = -1;
    int u_light_intensity_= -1;
    int u_screen_size_    = -1;
    int u_is_spot_        = -1;
    int u_spot_dir_       = -1;
    int u_spot_angle_     = -1;

    // uniform locations – composite shader
    int u_scene_tex_   = -1;
    int u_light_tex_   = -1;
    int u_ambient_     = -1;

    bool ready_ = false;

    std::vector<Light2D*> lights_;
};
