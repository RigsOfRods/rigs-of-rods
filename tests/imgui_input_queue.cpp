// SPDX-License-Identifier: GPL-3.0-only
// Headless regression fixture: production queue, no application, renderer or live input.
#include "ImGuiInputQueue.h"
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

struct Sink
{
    std::array<bool, 512> keys{};
    std::array<bool, 5> buttons{};
    std::vector<unsigned> text;
    float x = 0, y = 0, wheel = 0;
    void Key(unsigned key, bool down, unsigned character)
    {
        keys[key] = down;
        if (character) text.push_back(character);
    }
    void Button(unsigned button, bool down) { buttons[button] = down; }
    void Motion(float px, float py, float delta) { x = px; y = py; wheel += delta; }
};

int main()
{
    using RoR::ImGuiInputQueue;
    {
        ImGuiInputQueue q; Sink s;
        q.Key(30, true, 'a'); q.Key(30, false);
        q.Deliver(s); assert(s.keys[30] && s.text.size() == 1 && q.Pending() == 1);
        q.Deliver(s); assert(!s.keys[30] && s.text.size() == 1 && q.Pending() == 0);
        q.Deliver(s); assert(s.text.size() == 1);
    }
    {
        ImGuiInputQueue q; Sink s;
        q.Key(30, true, 'a'); q.Key(30, true, 'a'); q.Key(30, false);
        q.Deliver(s); assert(s.text.size() == 2 && s.keys[30]);
        q.Deliver(s); assert(s.text.size() == 2 && !s.keys[30]);
    }
    {
        ImGuiInputQueue q; Sink s;
        // Independent controls and modifier chord share a frame. Release remains queued.
        q.Key(29, true, 0, true); q.Key(30, true, 'a'); q.Button(1, true, 10, 20); q.Key(29, false, 0, true); q.Key(30, false);
        q.Deliver(s); assert(q.IsKeyDown(29) && s.keys[30] && s.buttons[1]);
        q.Deliver(s); assert(!q.IsKeyDown(29) && !s.keys[30]);
    }
    {
        ImGuiInputQueue q; Sink s;
        // A modifier held in a previous frame still belongs to the shortcut edge.
        q.Key(29, true, 0, true); q.Deliver(s);
        q.Key(30, true, 'a'); q.Key(29, false, 0, true); q.Key(30, false);
        q.Deliver(s); assert(q.IsKeyDown(29) && s.keys[30] && q.Pending() == 2);
        q.Deliver(s); assert(!q.IsKeyDown(29) && !s.keys[30] && q.Pending() == 0);
    }
    {
        ImGuiInputQueue q; Sink s;
        // A later modifier press must not retroactively modify an earlier action.
        q.Key(30, true, 'a'); q.Key(48, true, 'b'); q.Key(29, true, 0, true); q.Key(30, false);
        q.Deliver(s); assert(s.keys[30] && s.keys[48] && !q.IsKeyDown(29) && q.Pending() == 2);
        q.Deliver(s); assert(q.IsKeyDown(29) && !s.keys[30] && q.Pending() == 0);
    }
    {
        ImGuiInputQueue q; Sink s;
        q.Key(29, true, 0, true); q.Deliver(s);
        q.Button(0, true, 10, 20); q.Key(29, false, 0, true);
        q.Deliver(s); assert(s.buttons[0] && q.IsKeyDown(29) && q.Pending() == 1);
        q.Deliver(s); assert(!q.IsKeyDown(29));
        q.Motion(10, 20, .25f); q.Key(29, true, 0, true);
        q.Deliver(s); assert(s.wheel == .25f && !q.IsKeyDown(29) && q.Pending() == 1);
        q.Deliver(s); assert(q.IsKeyDown(29));
    }
    {
        ImGuiInputQueue q; Sink s;
        q.Motion(5, 6, 0); q.Motion(10, 20, 0);
        assert(q.Pending() == 1);
        q.Button(0, true, 10, 20); q.Motion(90, 100, 0); q.Button(0, false, 90, 100);
        q.Deliver(s); assert(s.buttons[0] && s.x == 10 && s.y == 20);
        q.Deliver(s); assert(!s.buttons[0] && s.x == 90 && s.y == 100);
        q.Button(0, true, 90, 100); q.Button(0, false, 90, 100);
        q.Button(0, true, 90, 100); q.Button(0, false, 90, 100);
        for (bool down : {true, false, true, false}) { q.Deliver(s); assert(s.buttons[0] == down); }
        assert(q.Pending() == 0);
    }
    {
        ImGuiInputQueue q; Sink s;
        q.Motion(1, 2, .25f); q.Motion(1, 2, .5f); q.Deliver(s);
        assert(s.wheel == .75f && q.Pending() == 0);
    }
    {
        ImGuiInputQueue q; Sink s;
        // Wheel delivery pins its position until the UI has sampled that increment.
        q.Motion(10, 20, .25f); q.Motion(900, 800, 0);
        q.Deliver(s); assert(s.x == 10 && s.y == 20 && s.wheel == .25f && q.Pending() == 1);
        s.wheel = 0; q.Deliver(s); assert(s.x == 900 && s.y == 800 && s.wheel == 0 && q.Pending() == 0);
    }
    {
        ImGuiInputQueue q; Sink s;
        q.Motion(10, 20, .25f); q.Motion(10, 20, .5f); q.Motion(900, 800, 1);
        q.Deliver(s); assert(s.x == 10 && s.y == 20 && s.wheel == .75f && q.Pending() == 1);
        s.wheel = 0; q.Deliver(s); assert(s.x == 900 && s.y == 800 && s.wheel == 1 && q.Pending() == 0);
    }
    {
        ImGuiInputQueue q; Sink s;
        q.Key(1, true); q.Deliver(s); q.Key(1, false);
        q.Reset(); assert(!q.IsKeyDown(1) && q.Pending() == 0);
        q.Reset(); q.Key(2, true, 'b'); q.Reset(); q.Deliver(s);
        assert(q.Pending() == 0 && s.text.empty());
        // Recreation starts clear, and queue delivery has no simulation-dt condition.
        q = ImGuiInputQueue(); q.Key(2, true, 'b'); q.Deliver(s);
        assert(s.keys[2] && s.text.size() == 1);
    }
    {
        ImGuiInputQueue q; Sink s;
        q.Key(4, true); q.Deliver(s);
        for (std::size_t i = 0; i < ImGuiInputQueue::Capacity; ++i) assert(q.Key(5, true, 'x'));
        assert(!q.Key(4, false));
        assert(q.Pending() == 0 && !q.IsKeyDown(4));
        q.Deliver(s); assert(s.text.empty());
        q.Key(4, true); q.Key(4, false); q.Deliver(s); assert(q.IsKeyDown(4));
        q.Deliver(s); assert(!q.IsKeyDown(4));
        assert(q.Key(512, true) && q.Button(5, true, 0, 0) && q.Pending() == 0);
    }
    std::cout << "ImGui 1.73 input queue: all headless assertions passed\n";
}
