#include "Input.hpp"

// ── Static storage ─────────────────────────────────────────────────────────────
std::unordered_map<std::string, Input::ActionBinding> Input::s_actions;
std::vector<Input::VirtualKey>                        Input::s_vkeys;
bool                                                  Input::s_vkeys_visible = true;
std::vector<int>                                      Input::s_prev_touch_ids;
std::vector<int>                                      Input::s_curr_touch_ids;
bool                                                  Input::s_any_touch_passthrough_pressed = false;

// ── update ─────────────────────────────────────────────────────────────────────
void Input::update()
{
    // Track touch point IDs for passthrough detection
    s_prev_touch_ids = s_curr_touch_ids;
    s_curr_touch_ids.clear();
    const int touch_count = GetTouchPointCount();
    for (int i = 0; i < touch_count; ++i)
    {
        const int id = GetTouchPointId(i);
        if (id >= 0) s_curr_touch_ids.push_back(id);
    }

    // Update each virtual key
    for (VirtualKey& vk : s_vkeys)
    {
        const bool was_down = vk.down;
        const bool now_down = vkey_is_touched(vk);
        vk.down     = now_down;
        vk.pressed  = (!was_down &&  now_down);
        vk.released = ( was_down && !now_down);
    }

    // Passthrough: a new touch that did not land on any virtual key
    s_any_touch_passthrough_pressed = false;
    for (int i = 0; i < touch_count; ++i)
    {
        const int id = GetTouchPointId(i);
        if (contains_id(s_prev_touch_ids, id)) continue;  // not new
        // New touch — check if it hits any virtual key
        const Vector2 tp = GetTouchPosition(i);
        bool on_vkey = false;
        for (const VirtualKey& vk : s_vkeys)
        {
            if (CheckCollisionPointRec(tp, vk.bounds)) { on_vkey = true; break; }
        }
        if (!on_vkey) { s_any_touch_passthrough_pressed = true; break; }
    }
}

// ── Action map ─────────────────────────────────────────────────────────────────
void Input::map(const std::string& action, std::vector<int> keys)
{
    s_actions[action].keys = std::move(keys);
}

void Input::map_add(const std::string& action, int key)
{
    s_actions[action].keys.push_back(key);
}

void Input::unmap(const std::string& action)
{
    s_actions.erase(action);
}

bool Input::is_action_held(const std::string& action)
{
    auto it = s_actions.find(action);
    if (it == s_actions.end()) return false;
    for (int k : it->second.keys)
        if (raw_is_held(k) || vkey_is_held(k)) return true;
    return false;
}

bool Input::is_action_just_pressed(const std::string& action)
{
    auto it = s_actions.find(action);
    if (it == s_actions.end()) return false;
    for (int k : it->second.keys)
        if (raw_is_just_pressed(k) || vkey_is_just_pressed(k)) return true;
    return false;
}

bool Input::is_action_just_released(const std::string& action)
{
    auto it = s_actions.find(action);
    if (it == s_actions.end()) return false;
    for (int k : it->second.keys)
        if (raw_is_just_released(k) || vkey_is_just_released(k)) return true;
    return false;
}

float Input::get_axis(const std::string& neg_action, const std::string& pos_action)
{
    return (is_action_held(pos_action) ? 1.0f : 0.0f)
         - (is_action_held(neg_action) ? 1.0f : 0.0f);
}

Vector2 Input::get_vector(const std::string& left, const std::string& right,
                          const std::string& up,   const std::string& down)
{
    float x = get_axis(left, right);
    float y = get_axis(up,   down);
    const float len = sqrtf(x * x + y * y);
    if (len > 1.0f) { x /= len; y /= len; }
    return Vector2{x, y};
}

// ── Raw key/mouse helpers ──────────────────────────────────────────────────────
bool Input::is_key_held        (int k) { return raw_is_held(k)         || vkey_is_held(k); }
bool Input::is_key_just_pressed(int k) { return raw_is_just_pressed(k) || vkey_is_just_pressed(k); }
bool Input::is_key_just_released(int k){ return raw_is_just_released(k)|| vkey_is_just_released(k); }

bool Input::is_mouse_held        (int b) { return IsMouseButtonDown    (b); }
bool Input::is_mouse_just_pressed(int b) { return IsMouseButtonPressed (b); }
bool Input::is_mouse_just_released(int b){ return IsMouseButtonReleased(b); }

Vector2 Input::get_mouse_position() { return GetMousePosition(); }
Vector2 Input::get_mouse_delta()    { return GetMouseDelta();    }
float   Input::get_mouse_wheel()    { return GetMouseWheelMove(); }

Vector2 Input::get_dpad_direction()
{
    float x = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D) ? 1.0f : 0.0f)
            - (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A) ? 1.0f : 0.0f);
    float y = (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S) ? 1.0f : 0.0f)
            - (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W) ? 1.0f : 0.0f);

    // Also consider virtual keys
    if (vkey_is_held(KEY_RIGHT)) x += 1.0f;
    if (vkey_is_held(KEY_LEFT))  x -= 1.0f;
    if (vkey_is_held(KEY_DOWN))  y += 1.0f;
    if (vkey_is_held(KEY_UP))    y -= 1.0f;

    const float len = sqrtf(x * x + y * y);
    if (len > 1.0f) { x /= len; y /= len; }
    return Vector2{x, y};
}

// ── Virtual keys ──────────────────────────────────────────────────────────────
void Input::add_virtual_key(int key_code, Rectangle screen_bounds)
{
    VirtualKey vk;
    vk.key_code = key_code;
    vk.bounds   = screen_bounds;
    s_vkeys.push_back(vk);
}

void Input::clear_virtual_keys()              { s_vkeys.clear(); }
void Input::set_virtual_keys_visible(bool v)  { s_vkeys_visible = v; }
bool Input::get_virtual_keys_visible()        { return s_vkeys_visible; }

void Input::draw_virtual_keys()
{
    if (!s_vkeys_visible) return;
    for (const VirtualKey& vk : s_vkeys)
    {
        const Color fill   = vk.down ? Color{255,140,56,170}  : Color{28,36,56,118};
        const Color border = vk.down ? Color{255,220,130,230} : Color{170,200,255,196};
        const Color tcol   = vk.down ? Color{255,245,220,255} : Color{225,238,255,230};

        DrawRectangleRounded(vk.bounds, 0.24f, 8, fill);
        DrawRectangleRoundedLinesEx(vk.bounds, 0.24f, 8, 2.0f, border);

        // Inner gloss
        Rectangle inner = {
            vk.bounds.x + 6.0f, vk.bounds.y + 6.0f,
            std::max(0.0f, vk.bounds.width  - 12.0f),
            std::max(0.0f, vk.bounds.height - 12.0f)
        };
        if (inner.width > 1.0f && inner.height > 1.0f)
        {
            const Color gloss = vk.down ? Color{255,210,120,44} : Color{200,220,255,30};
            DrawRectangleRounded(inner, 0.20f, 6, gloss);
        }

        const char* lbl = key_label(vk.key_code);
        if (lbl && lbl[0] != '\0')
        {
            const int fs = (int)std::max(14.0f, std::min(vk.bounds.height * 0.28f, 24.0f));
            const int tw = MeasureText(lbl, fs);
            DrawText(lbl,
                     (int)(vk.bounds.x + (vk.bounds.width  - tw) * 0.5f),
                     (int)(vk.bounds.y + (vk.bounds.height - fs) * 0.5f),
                     fs, tcol);
        }
    }
}

// ── Touch passthrough ──────────────────────────────────────────────────────────
bool    Input::is_any_touch_pressed_passthrough() { return s_any_touch_passthrough_pressed; }
Vector2 Input::get_touch_position(int index)      { return GetTouchPosition(index); }
int     Input::get_touch_count()                  { return GetTouchPointCount(); }

// ── Private helpers ────────────────────────────────────────────────────────────
bool Input::raw_is_held(int key)
{
    if (key >= INPUT_MOUSE_OFFSET && key < INPUT_GAMEPAD_OFFSET)
        return IsMouseButtonDown(key - INPUT_MOUSE_OFFSET);
    if (key >= INPUT_GAMEPAD_OFFSET)
        return IsGamepadButtonDown(0, key - INPUT_GAMEPAD_OFFSET);
    return IsKeyDown(key);
}

bool Input::raw_is_just_pressed(int key)
{
    if (key >= INPUT_MOUSE_OFFSET && key < INPUT_GAMEPAD_OFFSET)
        return IsMouseButtonPressed(key - INPUT_MOUSE_OFFSET);
    if (key >= INPUT_GAMEPAD_OFFSET)
        return IsGamepadButtonPressed(0, key - INPUT_GAMEPAD_OFFSET);
    return IsKeyPressed(key);
}

bool Input::raw_is_just_released(int key)
{
    if (key >= INPUT_MOUSE_OFFSET && key < INPUT_GAMEPAD_OFFSET)
        return IsMouseButtonReleased(key - INPUT_MOUSE_OFFSET);
    if (key >= INPUT_GAMEPAD_OFFSET)
        return IsGamepadButtonReleased(0, key - INPUT_GAMEPAD_OFFSET);
    return IsKeyReleased(key);
}

bool Input::vkey_is_touched(const VirtualKey& vk)
{
    const int count = GetTouchPointCount();
    for (int i = 0; i < count; ++i)
        if (CheckCollisionPointRec(GetTouchPosition(i), vk.bounds)) return true;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        if (CheckCollisionPointRec(GetMousePosition(), vk.bounds)) return true;
    return false;
}

bool Input::vkey_is_held(int key_code)
{
    for (const VirtualKey& vk : s_vkeys)
        if (vk.key_code == key_code && vk.down) return true;
    return false;
}

bool Input::vkey_is_just_pressed(int key_code)
{
    for (const VirtualKey& vk : s_vkeys)
        if (vk.key_code == key_code && vk.pressed) return true;
    return false;
}

bool Input::vkey_is_just_released(int key_code)
{
    for (const VirtualKey& vk : s_vkeys)
        if (vk.key_code == key_code && vk.released) return true;
    return false;
}

bool Input::contains_id(const std::vector<int>& ids, int value)
{
    for (int id : ids) if (id == value) return true;
    return false;
}

const char* Input::key_label(int key_code)
{
    switch (key_code)
    {
    case KEY_UP:            return "UP";
    case KEY_DOWN:          return "DN";
    case KEY_LEFT:          return "LT";
    case KEY_RIGHT:         return "RT";
    case KEY_ESCAPE:        return "ESC";
    case KEY_ENTER:         return "OK";
    case KEY_SPACE:         return "SPC";
    case KEY_TAB:           return "TAB";
    case KEY_BACKSPACE:     return "DEL";
    case KEY_LEFT_SHIFT:
    case KEY_RIGHT_SHIFT:   return "SHIFT";
    case KEY_LEFT_CONTROL:
    case KEY_RIGHT_CONTROL: return "CTRL";
    case KEY_F1:  return "F1";  case KEY_F2:  return "F2";
    case KEY_F3:  return "F3";  case KEY_F4:  return "F4";
    case KEY_F5:  return "F5";  case KEY_F6:  return "F6";
    case KEY_F7:  return "F7";  case KEY_F8:  return "F8";
    case KEY_F9:  return "F9";  case KEY_F10: return "F10";
    case KEY_F11: return "F11"; case KEY_F12: return "F12";
    default: break;
    }
    static char one[2] = {0, 0};
    if (key_code >= 'A' && key_code <= 'Z') { one[0] = (char)key_code; return one; }
    if (key_code >= '0' && key_code <= '9') { one[0] = (char)key_code; return one; }
    return "";
}
