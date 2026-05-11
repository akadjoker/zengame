#include "Timer.hpp"

Timer::Timer(const std::string& p_name)
    : Node(p_name)
{
}

// ----------------------------------------------------------------------------
// Node overrides
// ----------------------------------------------------------------------------

void Timer::_ready()
{
    if (autostart)
        start();
}

void Timer::_update(float dt)
{
    if (!m_running || m_paused) return;

    m_time_left -= dt;

    if (m_time_left <= 0.0f)
    {
        m_time_left = 0.0f;

        if (on_timeout) on_timeout();

        if (one_shot)
        {
            m_running = false;
        }
        else
        {
            // Repeating: restart; carry over the overshoot so timing stays accurate.
            const float overshoot = -m_time_left;
            m_time_left = wait_time - overshoot;
            if (m_time_left < 0.0f) m_time_left = 0.0f;
        }
    }
}

// ----------------------------------------------------------------------------
// Control
// ----------------------------------------------------------------------------

void Timer::start(float time)
{
    wait_time   = (time > 0.0f) ? time : wait_time;
    m_time_left = wait_time;
    m_running   = true;
    m_paused    = false;
}

void Timer::stop()
{
    m_running   = false;
    m_paused    = false;
    m_time_left = 0.0f;
}

void Timer::pause()
{
    if (m_running) m_paused = true;
}

void Timer::resume()
{
    m_paused = false;
}

bool Timer::is_stopped() const
{
    return !m_running;
}
