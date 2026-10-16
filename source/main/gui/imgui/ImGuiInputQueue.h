// SPDX-License-Identifier: GPL-3.0-only
// Compatibility queue for the polled ImGui 1.73 API. No OIS/Ogre/AppKit dependencies.
#pragma once

#include <array>
#include <cstddef>
#include <deque>

namespace RoR {
class ImGuiInputQueue
{
public:
    static constexpr std::size_t KeyCount = 512;
    static constexpr std::size_t ButtonCount = 5;
    static constexpr std::size_t Capacity = 1024;

    // False means bounded overflow recovery discarded this event and all prior input.
    bool Key(unsigned key, bool down, unsigned text = 0)
    {
        if (key >= KeyCount) return true;
        return Push({Kind::Key, key, down, text, 0, 0, 0});
    }
    bool Button(unsigned button, bool down, float x, float y)
    {
        if (button >= ButtonCount) return true;
        return Push({Kind::Button, button, down, 0, x, y, 0});
    }
    bool Motion(float x, float y, float wheel)
    {
        // Only position-only adjacent events can coalesce. Wheel increments remain ordered.
        if (wheel == 0 && !m_events.empty() && m_events.back().kind == Kind::Motion && m_events.back().wheel == 0)
        {
            m_events.back().x = x;
            m_events.back().y = y;
            return true;
        }
        return Push({Kind::Motion, 0, false, 0, x, y, wheel});
    }
    void Reset()
    {
        m_events.clear();
        m_keys.fill(false);
        m_buttons.fill(false);
        m_x = m_y = 0;
    }
    bool IsKeyDown(unsigned key) const { return key < KeyCount && m_keys[key]; }
    std::size_t Pending() const { return m_events.size(); }

    // Sink implements Key(index, down, text), Button(index, down), Motion(x,y,wheel).
    // Called exactly once immediately before each UI NewFrame, irrespective of simulation dt.
    template<class Sink> void Deliver(Sink& sink)
    {
        std::array<bool, KeyCount> changed_keys{};
        std::array<bool, ButtonCount> changed_buttons{};
        bool button_edge = false;
        while (!m_events.empty())
        {
            const Event& e = m_events.front();
            if (e.kind == Kind::Key)
            {
                if (changed_keys[e.index] && m_keys[e.index] != e.down) break;
                changed_keys[e.index] = changed_keys[e.index] || m_keys[e.index] != e.down;
                m_keys[e.index] = e.down;
                sink.Key(e.index, e.down, e.text);
            }
            else
            {
                // ImGui samples one position per frame: don't move a newly delivered click.
                if (button_edge && (e.x != m_x || e.y != m_y)) break;
                if (e.kind == Kind::Button)
                {
                    if (changed_buttons[e.index] && m_buttons[e.index] != e.down) break;
                    changed_buttons[e.index] = changed_buttons[e.index] || m_buttons[e.index] != e.down;
                    button_edge = button_edge || m_buttons[e.index] != e.down;
                    m_buttons[e.index] = e.down;
                    sink.Button(e.index, e.down);
                }
                m_x = e.x;
                m_y = e.y;
                sink.Motion(e.x, e.y, e.wheel);
            }
            m_events.pop_front();
        }
    }
private:
    enum class Kind { Key, Button, Motion };
    struct Event { Kind kind; unsigned index; bool down; unsigned text; float x, y, wheel; };
    bool Push(Event event)
    {
        if (m_events.size() >= Capacity) { Reset(); return false; }
        m_events.push_back(event);
        return true;
    }
    std::deque<Event> m_events;
    std::array<bool, KeyCount> m_keys{};
    std::array<bool, ButtonCount> m_buttons{};
    float m_x = 0, m_y = 0;
};
} // namespace RoR
