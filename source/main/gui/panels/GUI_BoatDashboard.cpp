/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/


#include "GUI_BoatDashboard.h"

#include "Actor.h"
#include "Application.h"
#include "Collisions.h"
#include "DashBoardManager.h"
#include "GameContext.h"
#include "GfxActor.h"
#include "GUIManager.h"
#include "Language.h"
#include "ScrewProp.h"
#include "SimBuffers.h"
#include "SimConstants.h"
#include "SimData.h"
#include "Terrain.h"

#include <imgui.h>
#include <fmt/format.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>

using namespace RoR;
using namespace GUI;

namespace {

const float PI_F = 3.14159265f;
const float DEG2RAD = PI_F / 180.f;
const float MS_TO_KNOTS = 1.9438f;
const float MS_TO_KMH = 3.6f;
const int   DEPTH_SAMPLES = 40;        //!< Echo sounder history length
const float DEPTH_INTERVAL = 0.5f;     //!< Seconds between echo sounder readings
const float DEPTH_ALARM = 2.f;         //!< Shallow water warning [m]

const ImU32 COL_CARD    = IM_COL32(22, 27, 34, 255);
const ImU32 COL_CARD_BR = IM_COL32(42, 49, 58, 255);
const ImU32 COL_INSET   = IM_COL32(11, 16, 22, 255);
const ImU32 COL_TEXT    = IM_COL32(230, 237, 243, 255);
const ImU32 COL_MUTED   = IM_COL32(157, 167, 179, 255);
const ImU32 COL_LINE    = IM_COL32(58, 68, 80, 255);
const ImU32 COL_ACCENT  = IM_COL32(63, 184, 232, 255);
const ImU32 COL_WARN    = IM_COL32(240, 160, 32, 255);
const ImU32 COL_PORT    = IM_COL32(229, 83, 75, 255);   // red = port (backboard)
const ImU32 COL_STBD    = IM_COL32(63, 185, 80, 255);   // green = starboard
const ImU32 COL_BAR     = IM_COL32(47, 111, 143, 255);
const ImU32 COL_BTN     = IM_COL32(33, 38, 45, 255);
const ImU32 COL_BTN_HOV = IM_COL32(48, 55, 64, 255);

/// Drawing in design units, scaled by k (same scheme as GUI_AirDashboard)
struct Ctx
{
    ImDrawList* dl;
    ImVec2 o;   // window origin (screen px)
    float k;    // px per design unit
    ImVec2 P(float x, float y) const { return ImVec2(o.x + x * k, o.y + y * k); }
    ImVec2 Pol(float cx, float cy, float r, float deg) const // deg clockwise from top
    {
        const float t = deg * DEG2RAD;
        return P(cx + r * sinf(t), cy - r * cosf(t));
    }
    ImVec2 Size(const char* s, float size) const { return ImGui::GetFont()->CalcTextSizeA(size * k, FLT_MAX, 0.f, s); }
    void Text(float cx, float cy, const char* s, float size, ImU32 col) const // centered
    {
        const ImVec2 ts = Size(s, size);
        const ImVec2 p = P(cx, cy);
        dl->AddText(ImGui::GetFont(), size * k, ImVec2(p.x - ts.x * 0.5f, p.y - ts.y * 0.5f), col, s);
    }
    void TextL(float x, float cy, const char* s, float size, ImU32 col) const // left aligned, vertically centered
    {
        const ImVec2 ts = Size(s, size);
        const ImVec2 p = P(x, cy);
        dl->AddText(ImGui::GetFont(), size * k, ImVec2(p.x, p.y - ts.y * 0.5f), col, s);
    }
    void TextR(float x, float cy, const char* s, float size, ImU32 col) const // right aligned
    {
        const ImVec2 ts = Size(s, size);
        const ImVec2 p = P(x, cy);
        dl->AddText(ImGui::GetFont(), size * k, ImVec2(p.x - ts.x, p.y - ts.y * 0.5f), col, s);
    }
    void Line(float x1, float y1, float x2, float y2, ImU32 col, float w) const { dl->AddLine(P(x1, y1), P(x2, y2), col, w * k); }
    void Card(float x, float w, const char* title) const
    {
        dl->AddRectFilled(P(x, 20.f), P(x + w, 160.f), COL_CARD, 6.f * k);
        dl->AddRect(P(x, 20.f), P(x + w, 160.f), COL_CARD_BR, 6.f * k, 0, 1.f);
        TextL(x + 7.f, 28.f, title, 7.5f, COL_MUTED);
    }
};

/// Arc stroke, angles in degrees clockwise from top
void Arc(Ctx& c, float cx, float cy, float r, float a0, float a1, ImU32 col, float w)
{
    // ImGui: radians, 0 = +x (right), clockwise on screen (y down)
    c.dl->PathArcTo(c.P(cx, cy), r * c.k, (a0 - 90.f) * DEG2RAD, (a1 - 90.f) * DEG2RAD, 32);
    c.dl->PathStroke(col, false, w * c.k);
}

// Compass point abbreviations (own context: "S" = south is not "S" = starboard)
std::string CompassPoint(int idx)
{
    switch (idx & 7)
    {
    case 0:  return _LC("BoatDashboardCompass", "N");
    case 1:  return _LC("BoatDashboardCompass", "NE");
    case 2:  return _LC("BoatDashboardCompass", "E");
    case 3:  return _LC("BoatDashboardCompass", "SE");
    case 4:  return _LC("BoatDashboardCompass", "S");
    case 5:  return _LC("BoatDashboardCompass", "SW");
    case 6:  return _LC("BoatDashboardCompass", "W");
    default: return _LC("BoatDashboardCompass", "NW");
    }
}

std::string HeadingName(float hdg)
{
    return CompassPoint((int)std::floor((hdg + 22.5f) / 45.f));
}

// Port/starboard abbreviations
std::string Port() { return _LC("BoatDashboardSide", "P"); }
std::string Stbd() { return _LC("BoatDashboardSide", "S"); }

} // anonymous namespace

bool BoatDashboard::IsUsedBy(const ActorPtr& actor)
{
    return actor && actor->ar_driveable == BOAT
        && !(actor->ar_dashboard && actor->ar_dashboard->wasDashboardHudLoaded()) // the boat's own dashboard wins
        && App::ui_default_boat_dash->getStr() == DASHBOARD_FILE;
}

void BoatDashboard::Draw(GfxActor* gfx_actor)
{
    if (!m_visible || gfx_actor == nullptr)
        return;
    ActorPtr actor = App::GetGameContext()->GetPlayerActor();
    if (!actor || actor->ar_driveable != BOAT || actor->ar_num_screwprops == 0)
        return;

    ActorSB& sb = gfx_actor->GetSimDataBuffer();
    const std::vector<NodeSB>& nodes = sb.simbuf_nodes;
    if (nodes.empty())
        return;

    // ---- Readings (from the sim buffer, i.e. the last finished physics step) ----
    const Ogre::Vector3 dir = sb.simbuf_direction;
    const float speed_ms = dir.dotProduct(sb.simbuf_node0_velo);
    const float knots = speed_ms * MS_TO_KNOTS;
    while (std::abs(knots) > m_speed_max * 0.9f && m_speed_max < 80.f)
        m_speed_max += 20.f;

    float heading = std::atan2(dir.x, -dir.z) / DEG2RAD; // 0 = north (-Z), clockwise
    if (heading < 0.f)
        heading += 360.f;

    // Heel and trim from the camera reference nodes: forward = direction, side = pos -> roll node
    float heel = 0.f, trim = 0.f;
    {
        trim = std::asin(std::max(-1.f, std::min(1.f, dir.y))) / DEG2RAD; // > 0 = bow up
        const NodeNum_t pos_node = sb.simbuf_camera0_pos_node;
        const NodeNum_t roll_node = sb.simbuf_camera0_roll_node;
        if (pos_node < nodes.size() && roll_node < nodes.size() && pos_node != roll_node)
        {
            Ogre::Vector3 side = (nodes[pos_node].AbsPosition - nodes[roll_node].AbsPosition).normalisedCopy();
            const Ogre::Vector3 stbd = dir.crossProduct(Ogre::Vector3::UNIT_Y).normalisedCopy();
            if (side.dotProduct(stbd) < 0.f)
                side = -side; // make it point to starboard
            heel = -std::asin(std::max(-1.f, std::min(1.f, side.y))) / DEG2RAD; // > 0 = heeling to starboard
        }
    }

    // Echo sounder: distance of the lowest hull point to the ground below it, a few times per second
    if (m_actor_id != actor->ar_instance_id)
    {
        m_actor_id = actor->ar_instance_id;
        m_depth_history.clear();
        m_depth = -1.f;
        m_depth_timer = 0.f;
        m_speed_max = 20.f;
    }
    m_depth_timer -= ImGui::GetIO().DeltaTime;
    if (m_depth_timer <= 0.f && App::GetGameContext()->GetTerrain())
    {
        m_depth_timer = DEPTH_INTERVAL;
        Collisions* col = App::GetGameContext()->GetTerrain()->GetCollisions();
        float agl = FLT_MAX;
        for (const NodeSB& n : nodes)
            agl = std::min(agl, n.AbsPosition.y - col->getSurfaceHeight(n.AbsPosition.x, n.AbsPosition.z));
        m_depth = (agl > 0.05f && agl < 999.f) ? agl : -1.f;
        m_depth_history.push_back(std::max(0.f, m_depth));
        if ((int)m_depth_history.size() > DEPTH_SAMPLES)
            m_depth_history.erase(m_depth_history.begin());
    }

    const int n_props = std::min<int>((int)sb.simbuf_screwprops.size(), actor->ar_num_screwprops);
    if (n_props == 0)
        return;
    const float rudder = sb.simbuf_screwprops[0].simbuf_sp_rudder; // -1..1, > 0 = port (keyboard 'left')

    // ---- Layout (design units) ----
    const float GAP = 6.f;
    const float W_SPEED = 116.f;
    const float W_LEVER = std::max(2, n_props) * 30.f + 46.f;
    const float W_COURSE = 170.f;
    const float W_DEPTH = 140.f;
    const float W_ATT = 150.f;
    const float x_speed = 6.f;
    const float x_lever = x_speed + W_SPEED + GAP;
    const float x_course = x_lever + W_LEVER + GAP;
    const float x_depth = x_course + W_COURSE + GAP;
    const float x_att = x_depth + W_DEPTH + GAP;
    const float DW = x_att + W_ATT + 6.f;
    const float DH = 166.f;

    const ImVec2 scr = ImGui::GetIO().DisplaySize;
    const float k = std::max(0.5f, std::min((scr.x * 0.5f / 640.f) * m_scale_mul, scr.x * 0.97f / DW));
    const ImVec2 wsize(DW * k, DH * k);

    ImGui::SetNextWindowSize(wsize);
    ImGui::SetNextWindowPos(ImVec2((scr.x - wsize.x) * 0.5f, scr.y - wsize.y - 8.f),
                            m_reset_pos ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    m_reset_pos = false;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, m_transparent ? 0.f : 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f * k);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.07f, 0.09f, m_transparent ? 0.f : 0.92f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.12f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("BoatDashboard", nullptr, flags))
    {
        Ctx c;
        c.dl = ImGui::GetWindowDrawList();
        c.o = ImGui::GetWindowPos();
        c.k = k;
        char buf[64];
        int idc = 0;

        // Clickable area; returns the item's 'active' state via out param
        auto Area = [&](float x, float y, float w, float h, bool* active = nullptr) -> bool
        {
            ImGui::SetCursorScreenPos(c.P(x, y));
            ImGui::PushID(idc++);
            ImGui::InvisibleButton("a", ImVec2(w * k, h * k));
            ImGui::PopID();
            if (active)
                *active = ImGui::IsItemActive();
            return ImGui::IsItemClicked();
        };
        auto Button = [&](float x, float y, float w, float h, const char* label, bool on, const char* tooltip) -> bool
        {
            const bool clicked = Area(x, y, w, h);
            const bool hovered = ImGui::IsItemHovered();
            c.dl->AddRectFilled(c.P(x, y), c.P(x + w, y + h), on ? COL_ACCENT : (hovered ? COL_BTN_HOV : COL_BTN), 4.f * k);
            c.dl->AddRect(c.P(x, y), c.P(x + w, y + h), on ? COL_ACCENT : COL_CARD_BR, 4.f * k, 0, 1.f);
            c.Text(x + w * 0.5f, y + h * 0.5f, label, 7.5f, on ? COL_INSET : COL_TEXT);
            if (hovered && tooltip)
                ImGui::SetTooltip("%s", tooltip);
            return clicked;
        };

        // ---- Title strip ----
        c.TextL(10.f, 10.f, actor->getTruckName().c_str(), 7.5f, COL_TEXT);
        if (Area(DW - 22.f, 3.f, 16.f, 14.f))
            ImGui::OpenPopup("BoatDashboardSettings");
        {
            const bool hov = ImGui::IsItemHovered();
            const ImVec2 gc = c.P(DW - 14.f, 10.f);
            c.dl->AddCircle(gc, 3.6f * k, hov ? COL_TEXT : COL_MUTED, 12, 1.4f * k);
            for (int i = 0; i < 8; ++i)
            {
                const float a = i * 45.f;
                c.dl->AddLine(c.Pol(DW - 14.f, 10.f, 4.2f, a), c.Pol(DW - 14.f, 10.f, 6.f, a), hov ? COL_TEXT : COL_MUTED, 1.6f * k);
            }
            if (hov)
                ImGui::SetTooltip("%s", _LC("BoatDashboard", "Dashboard settings"));
        }
        // Background drag moves the window (the instruments are separate items)
        Area(0.f, 0.f, DW - 26.f, 18.f);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
        {
            const ImVec2 d = ImGui::GetIO().MouseDelta;
            ImGui::SetWindowPos(ImVec2(ImGui::GetWindowPos().x + d.x, ImGui::GetWindowPos().y + d.y));
        }

        // ---- Speed ----
        c.Card(x_speed, W_SPEED, _LC("BoatDashboard", "Speed"));
        {
            const float cx = x_speed + W_SPEED * 0.5f, cy = 92.f, r = 44.f;
            auto ang = [&](float v) { return -135.f + std::max(0.f, std::min(v, m_speed_max)) / m_speed_max * 270.f; };
            Arc(c, cx, cy, r, -135.f, 135.f, COL_LINE, 3.f);
            if (knots > 0.f)
                Arc(c, cx, cy, r, -135.f, ang(knots), COL_ACCENT, 3.f);
            const int steps = 8;
            for (int i = 0; i <= steps; ++i)
            {
                const float v = m_speed_max * i / steps;
                const float a = ang(v);
                c.dl->AddLine(c.Pol(cx, cy, r - 7.f, a), c.Pol(cx, cy, r - 2.f, a), COL_MUTED, 1.2f * k);
                if (i % 2 == 0)
                {
                    snprintf(buf, sizeof(buf), "%d", (int)v);
                    const ImVec2 lp = c.Pol(cx, cy, r - 14.f, a);
                    const ImVec2 ts = c.Size(buf, 7.f);
                    c.dl->AddText(ImGui::GetFont(), 7.f * k, ImVec2(lp.x - ts.x * 0.5f, lp.y - ts.y * 0.5f), COL_MUTED, buf);
                }
            }
            const float na = ang(std::abs(knots));
            c.dl->AddLine(c.P(cx, cy), c.Pol(cx, cy, r - 9.f, na), COL_TEXT, 2.f * k);
            c.dl->AddCircleFilled(c.P(cx, cy), 3.f * k, COL_TEXT, 12);
            snprintf(buf, sizeof(buf), "%.1f", knots);
            c.Text(cx, cy + 22.f, buf, 13.f, COL_TEXT);
            const std::string unit = fmt::format("{} · {:.0f} {}", _LC("BoatDashboard", "kn"), speed_ms * MS_TO_KMH, _LC("BoatDashboard", "km/h"));
            c.Text(cx, cy + 36.f, unit.c_str(), 7.f, COL_MUTED);
        }

        // ---- Throttle levers, one per screwprop (drag; Ctrl = all levers; double click = stop) ----
        c.Card(x_lever, W_LEVER, _LC("BoatDashboard", "Throttle"));
        {
            const float top = 44.f, bot = 120.f;
            auto yv = [&](float v) { return top + (1.f - (v + 1.f) * 0.5f) * (bot - top); };
            const float x0 = x_lever + 40.f;
            // scale labels
            c.TextR(x0 - 6.f, yv(1.f), _LC("BoatDashboard", "Ahead"), 6.5f, COL_MUTED);
            c.TextR(x0 - 6.f, yv(0.f), _LC("BoatDashboard", "Stop"), 6.5f, COL_TEXT);
            c.TextR(x0 - 6.f, yv(-1.f), _LC("BoatDashboard", "Astern"), 6.5f, COL_MUTED);
            for (int i = 0; i < n_props; ++i)
            {
                const float lx = x0 + i * 30.f + 8.f;
                const float v = sb.simbuf_screwprops[i].simbuf_sp_throttle;
                for (float m : { 1.f, 0.5f, 0.f, -0.5f, -1.f })
                    c.Line(lx - 9.f, yv(m), lx + 9.f, yv(m), m == 0.f ? COL_TEXT : COL_LINE, m == 0.f ? 1.4f : 1.f);
                c.dl->AddRectFilled(c.P(lx - 3.f, top), c.P(lx + 3.f, bot), COL_INSET, 3.f * k);
                c.dl->AddRectFilled(c.P(lx - 3.f, std::min(yv(v), yv(0.f))), c.P(lx + 3.f, std::max(yv(v), yv(0.f))), v >= 0.f ? COL_ACCENT : COL_WARN, 2.f * k);
                c.dl->AddRectFilled(c.P(lx - 10.f, yv(v) - 4.f), c.P(lx + 10.f, yv(v) + 4.f), COL_TEXT, 2.f * k);

                bool active = false;
                Area(lx - 12.f, top - 6.f, 24.f, bot - top + 12.f, &active);
                const bool hovered = ImGui::IsItemHovered();
                if (hovered && ImGui::IsMouseDoubleClicked(0))
                {
                    actor->ar_screwprops[i]->setThrottle(0.f);
                }
                else if (active)
                {
                    float nv = 1.f - 2.f * (ImGui::GetIO().MousePos.y - c.P(0.f, top).y) / ((bot - top) * k);
                    nv = std::max(-1.f, std::min(1.f, nv));
                    if (std::abs(nv) < 0.04f)
                        nv = 0.f; // snap to stop
                    if (ImGui::GetIO().KeyCtrl)
                    {
                        for (int j = 0; j < actor->ar_num_screwprops; ++j)
                            actor->ar_screwprops[j]->setThrottle(nv);
                    }
                    else
                    {
                        actor->ar_screwprops[i]->setThrottle(nv);
                    }
                }
                if (hovered && !active)
                    ImGui::SetTooltip("%s", _LC("BoatDashboard", "Drag to set the throttle, Ctrl: all levers, double click: stop"));

                const std::string name = (n_props == 2) ? (i == 0 ? Port() : Stbd()) : fmt::format("{}", i + 1);
                snprintf(buf, sizeof(buf), "%d%%", (int)std::round(v * 100.f));
                const std::string label = fmt::format("{} {}", name, buf);
                c.Text(lx, 128.f, label.c_str(), 6.5f, COL_TEXT);

                // Engine switch (like the airplane dashboard's start buttons)
                const bool on = actor->ar_screwprops[i]->IsEngineOn();
                const std::string sw_label = on ? std::string(_LC("BoatDashboard", "On")) : std::string(_LC("BoatDashboard", "Off"));
                const std::string sw_tip = on ? std::string(_LC("BoatDashboard", "Engine running - click to switch off"))
                                              : std::string(_LC("BoatDashboard", "Engine off - click to start"));
                if (Button(lx - 13.f, 136.f, 26.f, 16.f, sw_label.c_str(), on, sw_tip.c_str()))
                    actor->ar_screwprops[i]->SetEngineOn(!on);
            }
        }

        // ---- Heading and rudder ----
        c.Card(x_course, W_COURSE, _LC("BoatDashboard", "Heading & rudder"));
        {
            snprintf(buf, sizeof(buf), "%03d°", (int)std::round(heading) % 360);
            c.TextL(x_course + 8.f, 44.f, buf, 15.f, COL_TEXT);
            const std::string hn = HeadingName(heading);
            c.TextL(x_course + 60.f, 46.f, hn.c_str(), 8.f, COL_MUTED);

            // compass tape
            const float tx0 = x_course + 8.f, tx1 = x_course + W_COURSE - 8.f, ty0 = 56.f, ty1 = 76.f;
            const float tcx = (tx0 + tx1) * 0.5f, ppd = 2.4f; // units per degree
            c.dl->AddRectFilled(c.P(tx0, ty0), c.P(tx1, ty1), COL_INSET, 4.f * k);
            c.dl->PushClipRect(c.P(tx0, ty0), c.P(tx1, ty1), true);
            const int first = (int)std::floor(heading - 35.f);
            for (int d = first; d <= first + 70; ++d)
            {
                const float x = tcx + (d - heading) * ppd;
                const int dd = ((d % 360) + 360) % 360;
                if (dd % 10 == 0)
                {
                    c.Line(x, ty1 - 6.f, x, ty1, COL_MUTED, 1.2f);
                    if (dd % 90 == 0)
                    {
                        const std::string cs = CompassPoint(dd / 45);
                        c.Text(x, ty0 + 6.f, cs.c_str(), 8.f, COL_TEXT);
                    }
                    else if (dd % 30 == 0)
                    {
                        snprintf(buf, sizeof(buf), "%03d", dd);
                        c.Text(x, ty0 + 6.f, buf, 6.5f, COL_MUTED);
                    }
                }
                else if (dd % 5 == 0)
                {
                    c.Line(x, ty1 - 3.f, x, ty1, COL_LINE, 1.f);
                }
            }
            c.dl->PopClipRect();
            c.dl->AddTriangleFilled(c.P(tcx - 4.f, ty1), c.P(tcx + 4.f, ty1), c.P(tcx, ty1 - 6.f), COL_ACCENT);

            // rudder half dial, port left / starboard right
            const float rcx = x_course + 66.f, rcy = 152.f, rr = 58.f;
            Arc(c, rcx, rcy, rr, -90.f, 0.f, COL_PORT, 4.f);
            Arc(c, rcx, rcy, rr, 0.f, 90.f, COL_STBD, 4.f);
            for (int a = -90; a <= 90; a += 15) // dial +-90 = rudder +-45
            {
                const bool major = (a % 30 == 0);
                c.dl->AddLine(c.Pol(rcx, rcy, rr - (major ? 8.f : 5.f), (float)a), c.Pol(rcx, rcy, rr - 1.f, (float)a), COL_MUTED, (major ? 1.4f : 1.f) * k);
                if (major)
                {
                    snprintf(buf, sizeof(buf), "%d", std::abs(a) / 2);
                    const ImVec2 lp = c.Pol(rcx, rcy, rr - 17.f, (float)a);
                    const ImVec2 ts = c.Size(buf, 7.5f);
                    c.dl->AddText(ImGui::GetFont(), 7.5f * k, ImVec2(lp.x - ts.x * 0.5f, lp.y - ts.y * 0.5f), COL_MUTED, buf);
                }
            }
            const float rdeg = rudder * 45.f; // 'rudder' in degrees, > 0 = port
            c.dl->AddLine(c.P(rcx, rcy), c.Pol(rcx, rcy, rr - 8.f, -rdeg * 2.f), COL_TEXT, 2.5f * k);
            c.dl->AddCircleFilled(c.P(rcx, rcy), 4.f * k, COL_TEXT, 12);
            c.TextL(rcx - rr + 4.f, rcy - 6.f, Port().c_str(), 7.5f, COL_PORT);
            c.TextR(rcx + rr - 4.f, rcy - 6.f, Stbd().c_str(), 7.5f, COL_STBD);

            const std::string rval = (std::abs(rdeg) < 0.5f)
                ? std::string("0°")
                : fmt::format("{:.0f}° {}", std::abs(rdeg), rdeg > 0.f ? Port() : Stbd());
            c.TextL(x_course + 134.f, 100.f, _LC("BoatDashboard", "Rudder"), 7.5f, COL_MUTED);
            c.TextL(x_course + 134.f, 112.f, rval.c_str(), 9.f, COL_TEXT);
            if (Button(x_course + 134.f, 128.f, 28.f, 18.f, "0°", false, _LC("BoatDashboard", "Center the rudder")))
            {
                for (int j = 0; j < actor->ar_num_screwprops; ++j)
                    actor->ar_screwprops[j]->setRudder(0.f);
            }
        }

        // ---- Echo sounder ----
        c.Card(x_depth, W_DEPTH, _LC("BoatDashboard", "Echo sounder"));
        {
            const bool shallow = (m_depth >= 0.f && m_depth < DEPTH_ALARM * 2.f);
            if (m_depth >= 0.f)
                snprintf(buf, sizeof(buf), "%.1f m", m_depth);
            else
                snprintf(buf, sizeof(buf), "--.- m");
            c.TextL(x_depth + 8.f, 44.f, buf, 15.f, shallow ? COL_WARN : COL_TEXT);
            c.TextR(x_depth + W_DEPTH - 8.f, 46.f, _LC("BoatDashboard", "below keel"), 6.5f, COL_MUTED);

            const float gx0 = x_depth + 8.f, gx1 = x_depth + W_DEPTH - 8.f, gy0 = 56.f, gy1 = 146.f;
            c.dl->AddRectFilled(c.P(gx0, gy0), c.P(gx1, gy1), COL_INSET, 4.f * k);
            float dmax = 10.f;
            for (float d : m_depth_history)
                dmax = std::max(dmax, d * 1.2f);
            const float bw = (gx1 - gx0) / DEPTH_SAMPLES;
            const int offset = DEPTH_SAMPLES - (int)m_depth_history.size();
            for (int i = 0; i < (int)m_depth_history.size(); ++i)
            {
                const float d = m_depth_history[i];
                const float bx = gx0 + (offset + i) * bw;
                const float h = std::min(d / dmax, 1.f) * (gy1 - gy0 - 4.f);
                c.dl->AddRectFilled(c.P(bx + 0.4f, gy0 + 2.f), c.P(bx + bw - 0.4f, gy0 + 2.f + h), d < DEPTH_ALARM * 2.f ? COL_WARN : COL_BAR, 1.f * k);
            }
            const float ya = gy0 + 2.f + DEPTH_ALARM / dmax * (gy1 - gy0 - 4.f);
            c.Line(gx0, ya, gx1, ya, COL_PORT, 1.f);
            const std::string alarm = fmt::format("{:.0f} m", DEPTH_ALARM);
            c.TextR(gx1 - 2.f, ya + 5.f, alarm.c_str(), 6.f, COL_PORT);
            const std::string hist = fmt::format(fmt::runtime(_LC("BoatDashboard", "last {} s")), (int)(DEPTH_SAMPLES * DEPTH_INTERVAL));
            c.TextL(x_depth + 8.f, 154.f, hist.c_str(), 6.5f, COL_MUTED);
        }

        // ---- Attitude and switches ----
        c.Card(x_att, W_ATT, _LC("BoatDashboard", "Attitude & switches"));
        {
            const float icx = x_att + 40.f, icy = 96.f, ir = 36.f;
            Arc(c, icx, icy, ir, -60.f, 60.f, COL_LINE, 4.f); // upper arc, the marker leans to the side the boat heels to
            for (int a = -30; a <= 30; a += 15)
                c.dl->AddLine(c.Pol(icx, icy, ir - 5.f, (float)a), c.Pol(icx, icy, ir + 3.f, (float)a), a == 0 ? COL_TEXT : COL_MUTED, 1.3f * k);
            for (int a : { -30, 30 })
            {
                const ImVec2 lp = c.Pol(icx, icy, ir + 10.f, (float)a);
                const ImVec2 ts = c.Size("30", 6.f);
                c.dl->AddText(ImGui::GetFont(), 6.f * k, ImVec2(lp.x - ts.x * 0.5f, lp.y - ts.y * 0.5f), COL_MUTED, "30");
            }
            const float hb = std::max(-55.f, std::min(55.f, heel));
            c.dl->AddCircleFilled(c.Pol(icx, icy, ir, hb), 5.f * k, std::abs(heel) > 20.f ? COL_WARN : COL_ACCENT, 12);

            const std::string heel_s = (std::abs(heel) < 0.5f)
                ? std::string("0°")
                : fmt::format("{:.0f}° {}", std::abs(heel), heel > 0.f ? Stbd() : Port());
            const std::string trim_s = fmt::format("{:+.0f}°", trim);
            c.TextL(x_att + 86.f, 62.f, _LC("BoatDashboard", "Heel"), 7.5f, COL_MUTED);
            c.TextL(x_att + 86.f, 74.f, heel_s.c_str(), 11.f, COL_TEXT);
            c.TextL(x_att + 86.f, 90.f, _LC("BoatDashboard", "Trim"), 7.5f, COL_MUTED);
            c.TextL(x_att + 86.f, 102.f, trim_s.c_str(), 11.f, COL_TEXT);

            // Switches: navigation lights (head/side lights), beacons, custom lights 1..n
            struct Sw { std::string label; bool on; int kind; int n; }; // kind 0 lights, 1 beacons, 2 custom
            std::vector<Sw> sw;
            if (actor->countFlaresByType(FlareType::HEADLIGHT) + actor->countFlaresByType(FlareType::SIDELIGHT) > 0)
                sw.push_back({ _LC("BoatDashboard", "Lights"), actor->getHeadlightsVisible(), 0, 0 });
            bool has_beacon = false;
            for (const Prop& p : gfx_actor->getProps())
                has_beacon = has_beacon || (p.pp_beacon_type != 0);
            if (has_beacon)
                sw.push_back({ _LC("BoatDashboard", "Beacons"), actor->getBeaconMode(), 1, 0 });
            for (int n = 0; n < MAX_CLIGHTS && sw.size() < 4; ++n)
            {
                if (actor->countCustomLights(n) > 0)
                    sw.push_back({ fmt::format("{} {}", _LC("BoatDashboard", "Light"), n + 1), actor->getCustomLightVisible(n), 2, n });
            }
            for (size_t i = 0; i < sw.size(); ++i)
            {
                const float bx = x_att + 8.f + (i % 2) * 68.f;
                const float by = 114.f + (i / 2) * 22.f;
                if (Button(bx, by, 64.f, 18.f, sw[i].label.c_str(), sw[i].on, nullptr))
                {
                    if (sw[i].kind == 0)
                    {
                        actor->setHeadlightsVisible(!actor->getHeadlightsVisible());
                        actor->setSideLightsVisible(actor->getHeadlightsVisible());
                    }
                    else if (sw[i].kind == 1)
                    {
                        actor->beaconsToggle();
                    }
                    else
                    {
                        actor->setCustomLightVisible(sw[i].n, !actor->getCustomLightVisible(sw[i].n));
                    }
                }
            }
        }

        // ---- Settings popup ----
        if (ImGui::BeginPopup("BoatDashboardSettings"))
        {
            ImGui::TextDisabled("%s", _LC("BoatDashboard", "Boat dashboard"));
            ImGui::PushItemWidth(160.f);
            ImGui::SliderFloat(_LC("BoatDashboard", "Size"), &m_scale_mul, 0.5f, 1.6f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::Checkbox(_LC("BoatDashboard", "Transparent background"), &m_transparent);
            if (ImGui::Button(_LC("BoatDashboard", "Reset position")))
                m_reset_pos = true;
            ImGui::Separator();
            ImGui::TextDisabled("%s", _LC("BoatDashboard", "Other dashboards: Settings > UI > Default boat dashboard"));
            ImGui::EndPopup();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}
