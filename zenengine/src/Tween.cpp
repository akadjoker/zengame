#include "Tween.hpp"
#include <raylib.h>
#include <cmath>
#include <algorithm>

// ----------------------------------------------------------------------------
// Easing functions  (t in [0..1] → [0..1])
// ----------------------------------------------------------------------------

static constexpr float TW_PI = 3.14159265f;

static float ease_in_quad   (float t) { return t * t; }
static float ease_out_quad  (float t) { return t * (2.0f - t); }
static float ease_inout_quad(float t)
    { return (t < 0.5f) ? 2.0f*t*t : -1.0f + (4.0f - 2.0f*t)*t; }

static float ease_in_cubic   (float t) { return t*t*t; }
static float ease_out_cubic  (float t) { float s=1.0f-t; return 1.0f - s*s*s; }
static float ease_inout_cubic(float t)
    { return (t < 0.5f) ? 4.0f*t*t*t : 1.0f - powf(-2.0f*t+2.0f,3)*0.5f; }

static float ease_in_quart   (float t) { return t*t*t*t; }
static float ease_out_quart  (float t) { float s=1.0f-t; return 1.0f - s*s*s*s; }
static float ease_inout_quart(float t)
    { return (t < 0.5f) ? 8.0f*t*t*t*t : 1.0f - powf(-2.0f*t+2.0f,4)*0.5f; }

static float ease_in_sine    (float t) { return 1.0f - cosf(t * TW_PI * 0.5f); }
static float ease_out_sine   (float t) { return sinf(t * TW_PI * 0.5f); }
static float ease_inout_sine (float t) { return -(cosf(TW_PI*t)-1.0f)*0.5f; }

static float ease_in_back    (float t)
    { constexpr float c1=1.70158f, c3=c1+1.0f; return c3*t*t*t - c1*t*t; }
static float ease_out_back   (float t)
{
    constexpr float c1=1.70158f, c3=c1+1.0f;
    float s = t - 1.0f;
    return 1.0f + c3*s*s*s + c1*s*s;
}
static float ease_inout_back (float t)
{
    constexpr float c1=1.70158f, c2=c1*1.525f;
    if (t < 0.5f)
        return (powf(2.0f*t,2)*((c2+1.0f)*2.0f*t - c2)) * 0.5f;
    float s = 2.0f*t - 2.0f;
    return (powf(s,2)*((c2+1.0f)*s + c2) + 2.0f) * 0.5f;
}

static float ease_out_bounce(float t)
{
    constexpr float n=7.5625f, d=2.75f;
    if (t < 1.0f/d)       return n*t*t;
    if (t < 2.0f/d)       { t -= 1.5f/d;   return n*t*t + 0.75f; }
    if (t < 2.5f/d)       { t -= 2.25f/d;  return n*t*t + 0.9375f; }
                             t -= 2.625f/d; return n*t*t + 0.984375f;
}
static float ease_in_bounce    (float t) { return 1.0f - ease_out_bounce(1.0f-t); }
static float ease_inout_bounce (float t)
    { return (t < 0.5f) ? (1.0f-ease_out_bounce(1.0f-2.0f*t))*0.5f
                         : (1.0f+ease_out_bounce(2.0f*t-1.0f))*0.5f; }

static float ease_in_elastic(float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    constexpr float c4 = (2.0f*TW_PI)/3.0f;
    return -powf(2.0f, 10.0f*t-10.0f) * sinf((t*10.0f-10.75f)*c4);
}
static float ease_out_elastic(float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    constexpr float c4 = (2.0f*TW_PI)/3.0f;
    return powf(2.0f,-10.0f*t) * sinf((t*10.0f-0.75f)*c4) + 1.0f;
}
static float ease_inout_elastic(float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    constexpr float c5 = (2.0f*TW_PI)/4.5f;
    return (t < 0.5f)
        ? -(powf(2.0f, 20.0f*t-10.0f) * sinf((20.0f*t-11.125f)*c5)) * 0.5f
        :  (powf(2.0f,-20.0f*t+10.0f) * sinf((20.0f*t-11.125f)*c5)) * 0.5f + 1.0f;
}

// ----------------------------------------------------------------------------
// apply_ease
// ----------------------------------------------------------------------------

float Tween::apply_ease(float t, TweenEase ease)
{
    t = std::max(0.0f, std::min(1.0f, t));

    switch (ease)
    {
    case TweenEase::Linear:       return t;
    case TweenEase::InQuad:       return ease_in_quad(t);
    case TweenEase::OutQuad:      return ease_out_quad(t);
    case TweenEase::InOutQuad:    return ease_inout_quad(t);
    case TweenEase::InCubic:      return ease_in_cubic(t);
    case TweenEase::OutCubic:     return ease_out_cubic(t);
    case TweenEase::InOutCubic:   return ease_inout_cubic(t);
    case TweenEase::InQuart:      return ease_in_quart(t);
    case TweenEase::OutQuart:     return ease_out_quart(t);
    case TweenEase::InOutQuart:   return ease_inout_quart(t);
    case TweenEase::InSine:       return ease_in_sine(t);
    case TweenEase::OutSine:      return ease_out_sine(t);
    case TweenEase::InOutSine:    return ease_inout_sine(t);
    case TweenEase::InBack:       return ease_in_back(t);
    case TweenEase::OutBack:      return ease_out_back(t);
    case TweenEase::InOutBack:    return ease_inout_back(t);
    case TweenEase::InBounce:     return ease_in_bounce(t);
    case TweenEase::OutBounce:    return ease_out_bounce(t);
    case TweenEase::InOutBounce:  return ease_inout_bounce(t);
    case TweenEase::InElastic:    return ease_in_elastic(t);
    case TweenEase::OutElastic:   return ease_out_elastic(t);
    case TweenEase::InOutElastic: return ease_inout_elastic(t);
    }
    return t;
}

// ----------------------------------------------------------------------------
// Tween
// ----------------------------------------------------------------------------

Tween::Tween(const std::string& p_name) : Node(p_name) {}

void Tween::recalc_duration()
{
    m_duration = 0.0f;
    for (const auto& tw : m_tweeners)
        m_duration = std::max(m_duration, tw.duration);
}

// ── Tweener builders ──────────────────────────────────────────────────────────

Tween& Tween::tween_float(float* target, float from, float to,
                           float duration, TweenEase ease)
{
    m_tweeners.push_back({
        [target, from, to](float t) { *target = from + (to - from) * t; },
        duration, ease
    });
    recalc_duration();
    return *this;
}

Tween& Tween::tween_vec2(Vec2* target, Vec2 from, Vec2 to,
                          float duration, TweenEase ease)
{
    m_tweeners.push_back({
        [target, from, to](float t) {
            target->x = from.x + (to.x - from.x) * t;
            target->y = from.y + (to.y - from.y) * t;
        },
        duration, ease
    });
    recalc_duration();
    return *this;
}

Tween& Tween::tween_color(Color* target, Color from, Color to,
                           float duration, TweenEase ease)
{
    m_tweeners.push_back({
        [target, from, to](float t) {
            target->r = (uint8_t)(from.r + (to.r - from.r) * t);
            target->g = (uint8_t)(from.g + (to.g - from.g) * t);
            target->b = (uint8_t)(from.b + (to.b - from.b) * t);
            target->a = (uint8_t)(from.a + (to.a - from.a) * t);
        },
        duration, ease
    });
    recalc_duration();
    return *this;
}

Tween& Tween::tween_lambda(std::function<void(float)> setter,
                            float duration, TweenEase ease)
{
    m_tweeners.push_back({ std::move(setter), duration, ease });
    recalc_duration();
    return *this;
}

// ── Control ───────────────────────────────────────────────────────────────────

void Tween::start()
{
    m_elapsed = 0.0f;
    m_running = true;
    m_paused  = false;
    m_reverse = false;
}

void Tween::stop()
{
    m_running = false;
    m_paused  = false;
}

void Tween::pause()  { if (m_running) m_paused = true; }
void Tween::resume() { m_paused = false; }

void Tween::kill()
{
    m_running = false;
    m_paused  = false;
    m_tweeners.clear();
    m_elapsed  = 0.0f;
    m_duration = 0.0f;
}

// ── Update ────────────────────────────────────────────────────────────────────

void Tween::_update(float dt)
{
    if (!m_running || m_paused || m_tweeners.empty()) return;

    m_elapsed += m_reverse ? -dt : dt;

    // Clamp and apply all tweeners
    const float clamped = std::max(0.0f, std::min(m_duration, m_elapsed));
    for (const auto& tw : m_tweeners)
    {
        const float local_t = (tw.duration > 0.0f)
            ? std::min(clamped / tw.duration, 1.0f) : 1.0f;
        tw.setter(apply_ease(local_t, tw.ease));
    }

    if (on_step) on_step();

    // Check completion
    const bool forward_done = !m_reverse && (m_elapsed >= m_duration);
    const bool reverse_done =  m_reverse && (m_elapsed <= 0.0f);

    if (forward_done || reverse_done)
    {
        switch (loop_mode)
        {
        case TweenLoop::None:
            m_running = false;
            if (on_finished) on_finished();
            break;

        case TweenLoop::Repeat:
            m_elapsed = 0.0f;
            break;

        case TweenLoop::PingPong:
            m_reverse = !m_reverse;
            m_elapsed = m_reverse ? m_duration : 0.0f;
            break;
        }
    }
}
