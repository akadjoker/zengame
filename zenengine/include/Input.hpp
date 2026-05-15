#pragma once

#include <raylib.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

// ----------------------------------------------------------------------------
// Input
//
// Centralised input manager. Combines keyboard/mouse/gamepad with named
// action bindings and optional touch virtual keys for mobile/web targets.
//
// ── Action map ────────────────────────────────────────────────────────────────
//   Input::map("jump",  {KEY_SPACE, KEY_W});
//   Input::map("fire",  {KEY_Z, MOUSE_BUTTON_LEFT});   // mouse buttons also work
//   Input::map("right", {KEY_RIGHT, KEY_D});
//
//   if (Input::is_action_just_pressed("jump"))  { ... }
//   if (Input::is_action_held("right"))         { ... }
//   float axis = Input::get_axis("left", "right");   // -1..+1
//
// ── Virtual keys (touch / mobile) ────────────────────────────────────────────
//   // Register buttons — call once at startup
//   Input::add_virtual_key(KEY_LEFT,  {  20, 580, 80, 80 });
//   Input::add_virtual_key(KEY_RIGHT, { 110, 580, 80, 80 });
//   Input::add_virtual_key(KEY_SPACE, { 1170, 580, 90, 90 });
//
//   // In game loop (before tree.process):
//   Input::update();
//
//   // is_action_just_pressed / is_action_held transparently include virtual keys.
//
//   // To draw them (call inside BeginDrawing / EndDrawing):
//   Input::draw_virtual_keys();
//
// ── Raw helpers (bypass action map) ──────────────────────────────────────────
//   Vec2 mouse = Input::get_mouse_position();
//   Vec2 dir   = Input::get_dpad_direction();   // 8-dir normalised from arrow keys
//   bool fired = Input::is_mouse_just_pressed(MOUSE_BUTTON_LEFT);
// ----------------------------------------------------------------------------

class Input
{
public:
    // ── Must be called once per frame (before SceneTree::process) ─────────────
    static void update();

    // ── Action map ────────────────────────────────────────────────────────────

    // Bind a named action to one or more keycodes.
    // Mouse buttons: use MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, etc. (add 1000)
    // Gamepad buttons: add 2000 to GAMEPAD_BUTTON_* values.
    // Calling map() again with the same name REPLACES the binding.
    static void map(const std::string& action, std::vector<int> keys);

    // Add extra keys to an existing action (or creates the action).
    static void map_add(const std::string& action, int key);

    // Remove an action entirely.
    static void unmap(const std::string& action);

    // Is the action currently held?
    static bool is_action_held        (const std::string& action);
    // Was the action pressed this frame (first frame only)?
    static bool is_action_just_pressed(const std::string& action);
    // Was the action released this frame?
    static bool is_action_just_released(const std::string& action);

    // Returns -1, 0 or +1 depending on which of the two actions is held.
    static float get_axis(const std::string& neg_action,
                          const std::string& pos_action);

    // Returns a normalised Vec2 from four directional actions.
    // Default names: "left","right","up","down" — override with parameters.
    static Vector2 get_vector(const std::string& left  = "left",
                              const std::string& right = "right",
                              const std::string& up    = "up",
                              const std::string& down  = "down");

    // ── Raw keyboard / mouse / gamepad ────────────────────────────────────────

    // Thin wrappers that also consider virtual-key state.
    static bool is_key_held        (int key_code);
    static bool is_key_just_pressed(int key_code);
    static bool is_key_just_released(int key_code);

    static bool is_mouse_held        (int button = MOUSE_BUTTON_LEFT);
    static bool is_mouse_just_pressed(int button = MOUSE_BUTTON_LEFT);
    static bool is_mouse_just_released(int button = MOUSE_BUTTON_LEFT);

    static Vector2 get_mouse_position();
    static Vector2 get_mouse_delta();
    static float   get_mouse_wheel();

    // Convenience: 8-direction from arrow keys (normalised, length 1 on diagonal).
    static Vector2 get_dpad_direction();

    // ── Virtual keys ─────────────────────────────────────────────────────────

    // Add a touch-screen virtual button that maps to keyCode.
    static void add_virtual_key(int key_code, Rectangle screen_bounds);

    // Remove all virtual keys.
    static void clear_virtual_keys();

    // Show / hide virtual key overlay.
    static void set_virtual_keys_visible(bool visible);
    static bool get_virtual_keys_visible();

    // Draw the virtual key overlay (call inside BeginDrawing / EndDrawing,
    // after tree.draw() so the HUD sits on top).
    static void draw_virtual_keys();

    // ── Touch passthrough ─────────────────────────────────────────────────────

    // True if any touch point appeared this frame that was NOT on a virtual key.
    static bool is_any_touch_pressed_passthrough();

    // World position of touch[index] (raw screen position).
    static Vector2 get_touch_position(int index);
    static int     get_touch_count();

private:
    // ── Internal structs ──────────────────────────────────────────────────────

    struct ActionBinding
    {
        std::vector<int> keys;  // key codes (mouse: +1000, gamepad: +2000)
    };

    struct VirtualKey
    {
        int       key_code;
        Rectangle bounds;
        bool      down     = false;
        bool      pressed  = false;
        bool      released = false;
    };

    // ── State ─────────────────────────────────────────────────────────────────
    static std::unordered_map<std::string, ActionBinding> s_actions;
    static std::vector<VirtualKey>                        s_vkeys;
    static bool                                           s_vkeys_visible;

    static std::vector<int> s_prev_touch_ids;
    static std::vector<int> s_curr_touch_ids;
    static bool             s_any_touch_passthrough_pressed;

    // ── Helpers ───────────────────────────────────────────────────────────────
    static bool raw_is_held        (int key);
    static bool raw_is_just_pressed(int key);
    static bool raw_is_just_released(int key);

    static bool vkey_is_touched(const VirtualKey& vk);
    static bool vkey_is_held        (int key_code);
    static bool vkey_is_just_pressed(int key_code);
    static bool vkey_is_just_released(int key_code);

    static const char* key_label(int key_code);
    static bool contains_id(const std::vector<int>& ids, int value);
};

// ── Offset conventions for action-map keys ────────────────────────────────────
static constexpr int INPUT_MOUSE_OFFSET   = 1000;
static constexpr int INPUT_GAMEPAD_OFFSET = 2000;
