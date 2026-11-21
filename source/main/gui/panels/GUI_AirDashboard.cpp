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

#include "GUI_AirDashboard.h"

#include "Actor.h"
#include "AeroEngine.h"
#include "AppContext.h"
#include "AutoPilot.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GfxActor.h"
#include "Language.h"
#include "SimBuffers.h"
#include "SimConstants.h"

#include <imgui.h>
#include <fmt/format.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

using namespace RoR;
using namespace GUI;

namespace {

const float PI_F = 3.14159265f;
const float DEG2RAD = PI_F / 180.f;
const ImU32 COL_BG      = IM_COL32(29, 32, 37, 255);
const ImU32 COL_RING    = IM_COL32(255, 255, 255, 70);
const ImU32 COL_TICK    = IM_COL32(242, 244, 247, 255);
const ImU32 COL_TEXT    = IM_COL32(230, 233, 238, 255);
const ImU32 COL_MUTED   = IM_COL32(154, 161, 171, 255);
const ImU32 COL_NEEDLE  = IM_COL32(239, 159, 39, 255);
const ImU32 COL_PILL    = IM_COL32(255, 255, 255, 26);
const ImU32 COL_BLUE    = IM_COL32(55, 138, 221, 255);
const ImU32 COL_GREEN   = IM_COL32(29, 158, 117, 255);
const ImU32 COL_RED     = IM_COL32(226, 75, 74, 255);
const ImU32 COL_SKY     = IM_COL32(122, 166, 214, 255);
const ImU32 COL_GROUND  = IM_COL32(138, 90, 60, 255);

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
    void TextL(float x, float cy, const char* s, float size, ImU32 col) const // left aligned
    {
        const ImVec2 ts = Size(s, size);
        const ImVec2 p = P(x, cy);
        dl->AddText(ImGui::GetFont(), size * k, ImVec2(p.x, p.y - ts.y * 0.5f), col, s);
    }
    void TextLR(float cx, float cy, const char* a, const char* b, float size) const // "name value", centered together
    {
        const ImVec2 sa = Size(a, size), sb = Size(b, size);
        const float total = sa.x + sb.x;
        const ImVec2 p = P(cx, cy);
        const float x0 = p.x - total * 0.5f, y0 = p.y - sa.y * 0.5f;
        dl->AddText(ImGui::GetFont(), size * k, ImVec2(x0, y0), COL_MUTED, a);
        dl->AddText(ImGui::GetFont(), size * k, ImVec2(x0 + sa.x, y0), IM_COL32_WHITE, b);
    }
    void Line(float x1, float y1, float x2, float y2, ImU32 col, float w) const { dl->AddLine(P(x1, y1), P(x2, y2), col, w * k); }
};

typedef std::pair<float, const char*> Tick;

// Round gauge (half = left half disc, pivot at the right edge). f maps value -> angle (deg clockwise from top).
template <typename F>
void Dial(Ctx& c, float cx, float cy, float r, bool half, F f, const std::vector<Tick>& ticks, float needle_val, float fs)
{
    if (half)
    {
        c.dl->PathArcTo(c.P(cx, cy), r * c.k, PI_F * 0.5f, PI_F * 1.5f, 32);
        c.dl->PathFillConvex(COL_BG);
        c.dl->PathArcTo(c.P(cx, cy), r * c.k, PI_F * 0.5f, PI_F * 1.5f, 32);
        c.dl->PathStroke(COL_RING, true, 1.f);
    }
    else
    {
        c.dl->AddCircleFilled(c.P(cx, cy), r * c.k, COL_BG, 48);
        c.dl->AddCircle(c.P(cx, cy), r * c.k, COL_RING, 48, 1.f);
    }
    for (const Tick& t : ticks)
    {
        const float a = f(t.first);
        c.dl->AddLine(c.Pol(cx, cy, r - 1.5f, a), c.Pol(cx, cy, r - 7.f, a), COL_TICK, 1.5f * c.k);
        if (t.second[0])
        {
            const ImVec2 lp = c.Pol(cx, cy, r - 15.f, a);
            const ImVec2 ts = c.Size(t.second, fs);
            c.dl->AddText(ImGui::GetFont(), fs * c.k, ImVec2(lp.x - ts.x * 0.5f, lp.y - ts.y * 0.5f), COL_TEXT, t.second);
        }
    }
    const float na = f(needle_val);
    c.dl->AddLine(c.Pol(cx, cy, -r * 0.1f, na), c.Pol(cx, cy, r - 10.f, na), COL_NEEDLE, 3.f * c.k);
    c.dl->AddCircleFilled(c.P(cx, cy), 3.5f * c.k, COL_NEEDLE, 12);
}

} // anonymous namespace

void AirDashboard::Draw(GfxActor* gfx_actor)
{
    if (!m_visible || gfx_actor == nullptr)
        return;
    ActorPtr actor = App::GetGameContext()->GetPlayerActor();
    if (!actor || actor->ar_driveable != AIRPLANE)
        return;
    ActorSB& sb = gfx_actor->GetSimDataBuffer();
    NodeSB* nodes = gfx_actor->GetSimNodeBuffer();
    if (sb.simbuf_aeroengines.empty())
        return;
    Autopilot* ap = actor->ar_autopilot.GetRef();

    const int n = std::min<int>((int)sb.simbuf_aeroengines.size(), MAX_AEROENGINES);

    // ---- Layout (design units) ----
    const float FULL_W = 120.f, HALF_W = 64.f;
    const float slot_w[6] = { FULL_W, FULL_W, HALF_W, FULL_W, HALF_W, FULL_W };
    float left_w = 8.f;
    for (int i = 0; i < 6; ++i)
        if (m_show[i]) left_w += slot_w[i];
    const bool has_dials = m_show[6] || m_show[7] || m_show[8];
    const float col_w = has_dials ? 62.f : 36.f;
    const float STRIP_END = 630.f;
    const float eng_x = std::max(left_w, STRIP_END - 4.f) + 6.f;
    const float DW = eng_x + col_w * n + 8.f;
    const float DH = 200.f;

    const ImVec2 scr = ImGui::GetIO().DisplaySize;
    const float k = std::max(0.5f, std::min((scr.x * 0.5f / 770.f) * m_scale_mul, scr.x * 0.97f / DW));
    const ImVec2 wsize(DW * k, DH * k);

    ImGui::SetNextWindowSize(wsize);
    ImGui::SetNextWindowPos(ImVec2((scr.x - wsize.x) * 0.5f, scr.y - wsize.y - 8.f),
                            m_reset_pos ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    m_reset_pos = false;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, m_transparent ? 0.f : 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f * k);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.09f, m_transparent ? 0.f : 0.88f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.12f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;
    const bool open = ImGui::Begin("AirDashboard", nullptr, flags);
    if (open)
    {
        Ctx c;
        c.dl = ImGui::GetWindowDrawList();
        c.o = ImGui::GetWindowPos();
        c.k = k;

        int idc = 0;
        auto Hit = [&](float x, float y, float w, float h) -> bool
        {
            ImGui::SetCursorScreenPos(c.P(x, y));
            ImGui::PushID(idc++);
            ImGui::InvisibleButton("b", ImVec2(w * k, h * k));
            ImGui::PopID();
            return ImGui::IsItemClicked();
        };
        auto Repeat = [&](float x, float y, float w, float h) -> bool
        {
            ImGui::SetCursorScreenPos(c.P(x, y));
            ImGui::PushID(idc++);
            ImGui::InvisibleButton("b", ImVec2(w * k, h * k));
            ImGui::PopID();
            if (ImGui::IsItemClicked())
            {
                m_hold_timer = 0.35f;
                return true;
            }
            if (ImGui::IsItemActive())
            {
                m_hold_timer -= ImGui::GetIO().DeltaTime;
                if (m_hold_timer <= 0.f)
                {
                    m_hold_timer = 0.08f;
                    return true;
                }
            }
            return false;
        };
        auto Pill = [&](float x, float w, const char* txt, bool active, ImU32 col) -> bool
        {
            c.dl->AddRectFilled(c.P(x, 12.f), c.P(x + w, 28.f), active ? col : COL_PILL, 8.f * k);
            c.Text(x + w * 0.5f, 20.f, txt, 8.f, active ? IM_COL32_WHITE : COL_TEXT);
            return Hit(x, 12.f, w, 16.f);
        };
        // label, value, up/down arrows. Returns +1/-1 while an arrow is pressed.
        auto ApGroup = [&](float lx, const char* label, float vx, const char* val, float ax) -> int
        {
            c.TextL(lx, 20.f, label, 9.f, COL_MUTED);
            c.TextL(vx, 20.f, val, 11.f, IM_COL32_WHITE);
            c.dl->AddTriangleFilled(c.P(ax + 4.f, 12.f), c.P(ax, 18.f), c.P(ax + 8.f, 18.f), COL_TEXT);
            c.dl->AddTriangleFilled(c.P(ax, 22.f), c.P(ax + 8.f, 22.f), c.P(ax + 4.f, 28.f), COL_TEXT);
            int res = 0;
            if (Repeat(ax - 1.f, 11.f, 10.f, 9.f)) res = 1;
            if (Repeat(ax - 1.f, 20.f, 10.f, 9.f)) res = -1;
            return res;
        };

        // Click a value to type it in with the keyboard (Enter applies, Esc/click elsewhere cancels).
        // Returns true and the typed number when the user confirmed.
        auto EditValue = [&](int field, float vx, float vw, int current, int& typed) -> bool
        {
            bool done = false;
            ImGui::SetCursorScreenPos(c.P(vx - 2.f, 11.f));
            if (m_edit_field != field)
            {
                ImGui::PushID(idc++);
                ImGui::InvisibleButton("v", ImVec2((vw + 4.f) * k, 18.f * k));
                ImGui::PopID();
                // start on button release, so the mouse button is no longer held by this widget
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
                {
                    m_edit_field = field;
                    m_edit_focus = true;
                    m_edit_was_active = false;
                    snprintf(m_edit_buf, sizeof(m_edit_buf), "%d", current);
                }
                return false;
            }
            idc++; // keep the ids of the following buttons stable
            App::GetGuiManager()->RequestGuiCaptureKeyboard(true); // game keys must not react while typing
            ImGui::SetCursorScreenPos(c.P(vx - 3.f, 11.f));
            ImGui::PushItemWidth((vw + 6.f) * k);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f * k, 1.f * k));
            ImGui::SetWindowFontScale(11.f * k / ImGui::GetFontSize());
            if (m_edit_focus)
                ImGui::SetKeyboardFocusHere(); // repeated until the field really is active
            ImGui::PushID(2000 + field);
            const ImGuiInputTextFlags itf = ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue
                | ImGuiInputTextFlags_AutoSelectAll;
            if (ImGui::InputText("##edit", m_edit_buf, sizeof(m_edit_buf), itf))
            {
                typed = atoi(m_edit_buf);
                done = true;
                m_edit_field = -1;
            }
            else
            {
                if (ImGui::IsItemActive())
                {
                    m_edit_was_active = true;
                    m_edit_focus = false;
                }
                const bool esc = ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Escape]);
                const bool click_outside = ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered();
                if (esc || click_outside || (m_edit_was_active && !ImGui::IsItemActive()))
                    m_edit_field = -1; // cancelled
            }
            ImGui::PopID();
            ImGui::SetWindowFontScale(1.f);
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();
            return done;
        };

        // ---- Autopilot strip ----
        c.dl->AddRectFilled(c.P(8.f, 8.f), c.P(STRIP_END, 32.f), COL_PILL, 12.f * k);
        char buf[48];
        int r;
        snprintf(buf, sizeof(buf), "%03d", sb.simbuf_ap_heading_value);
        r = ApGroup(18.f, "HDG", 40.f, buf, 64.f);
        if (ap) { if (r == 1) ap->adjHDG(1); else if (r == -1) ap->adjHDG(-1); }
        int typed = 0;
        if (EditValue(0, 40.f, 22.f, sb.simbuf_ap_heading_value, typed) && ap)
            ap->adjHDG(((typed % 360) + 360) % 360 - ap->GetHeadingValue());
        if (Pill(82.f, 28.f, "HDG", sb.simbuf_ap_heading_mode == Autopilot::HEADING_FIXED, COL_BLUE) && ap) ap->toggleHeading(Autopilot::HEADING_FIXED);
        if (Pill(114.f, 28.f, "WLV", sb.simbuf_ap_heading_mode == Autopilot::HEADING_WLV, COL_BLUE) && ap) ap->toggleHeading(Autopilot::HEADING_WLV);
        if (Pill(146.f, 28.f, "NAV", sb.simbuf_ap_heading_mode == Autopilot::HEADING_NAV, COL_BLUE) && ap) ap->toggleHeading(Autopilot::HEADING_NAV);

        snprintf(buf, sizeof(buf), "%d00", sb.simbuf_ap_alt_value / 100);
        r = ApGroup(186.f, "ALT", 206.f, buf, 240.f);
        if (ap) { if (r == 1) ap->adjALT(100); else if (r == -1) ap->adjALT(-100); }
        if (EditValue(1, 206.f, 32.f, sb.simbuf_ap_alt_value, typed) && ap)
            ap->adjALT(std::max(0, typed) - ap->GetAltValue());
        if (Pill(256.f, 32.f, "HOLD", sb.simbuf_ap_alt_mode == Autopilot::ALT_FIXED, COL_BLUE) && ap) ap->toggleAlt(Autopilot::ALT_FIXED);

        if (sb.simbuf_ap_vs_value == 0) snprintf(buf, sizeof(buf), "000");
        else snprintf(buf, sizeof(buf), "%+d00", sb.simbuf_ap_vs_value / 100);
        r = ApGroup(298.f, "VS", 312.f, buf, 338.f);
        if (ap) { if (r == 1) ap->adjVS(100); else if (r == -1) ap->adjVS(-100); }
        if (EditValue(2, 312.f, 24.f, sb.simbuf_ap_vs_value, typed) && ap)
            ap->adjVS(typed - ap->GetVsValue());
        if (Pill(354.f, 24.f, "VS", sb.simbuf_ap_alt_mode == Autopilot::ALT_VS, COL_BLUE) && ap) ap->toggleAlt(Autopilot::ALT_VS);

        snprintf(buf, sizeof(buf), "%03d", sb.simbuf_ap_ias_value);
        r = ApGroup(388.f, "IAS", 408.f, buf, 434.f);
        if (ap) { if (r == 1) ap->adjIAS(1); else if (r == -1) ap->adjIAS(-1); }
        if (EditValue(3, 408.f, 24.f, sb.simbuf_ap_ias_value, typed) && ap)
            ap->adjIAS(typed - ap->GetIasValue());
        if (Pill(450.f, 30.f, "ATHR", sb.simbuf_ap_ias_mode, COL_BLUE) && ap) ap->toggleIAS();
        if (Pill(486.f, 32.f, "GPWS", sb.simbuf_ap_gpws_mode, COL_GREEN) && ap) ap->toggleGPWS();
        if (Pill(522.f, 32.f, "BRKS", sb.simbuf_parking_brake, COL_RED)) actor->parkingbrakeToggle();

        // ---- Flight data ----
        const float altitude = nodes[0].AbsPosition.y;
        const float alt_ft = altitude * 3.28084f;
        const float ground_speed_kt = sb.simbuf_node0_velo.length() * 1.9438f;
        const float airpressure = 101325.f * powf(1.f - 0.0065f * altitude / 288.15f, 5.24947f);
        const float kt = ground_speed_kt * sqrtf(airpressure * 0.0000120896f / 1.225f);

        // Outside air temperature (ISA standard atmosphere, 15 C at sea level, -6.5 C per 1000 m)
        if (m_show_temp)
        {
            const float oat = 15.f - 0.0065f * std::min(altitude, 11000.f);
            snprintf(buf, sizeof(buf), "OAT %d C", (int)floorf(oat + 0.5f));
            c.dl->AddRectFilled(c.P(560.f, 12.f), c.P(608.f, 28.f), IM_COL32(55, 138, 221, 46), 8.f * k);
            c.Text(584.f, 20.f, buf, 8.f, IM_COL32_WHITE);
        }
        // Settings gear
        {
            const float gx = 618.f, gy = 20.f;
            c.dl->AddCircle(c.P(gx, gy), 3.5f * k, COL_TEXT, 16, 1.6f * k);
            for (int i = 0; i < 8; ++i)
                c.dl->AddLine(c.Pol(gx, gy, 3.5f, i * 45.f), c.Pol(gx, gy, 6.f, i * 45.f), COL_TEXT, 2.f * k);
            if (Hit(gx - 9.f, gy - 9.f, 18.f, 18.f))
                ImGui::OpenPopup("AirDashSettings");
        }

        const float cy = 104.f;
        const float R = 58.f, RH = 52.f;
        float x = 8.f; // next slot left edge
        auto Under = [&](float cx, const char* name, const char* value)
        {
            c.Text(cx, 177.f, name, 8.f, COL_MUTED);
            c.Text(cx, 191.f, value, 11.f, IM_COL32_WHITE);
        };

        // Airspeed
        if (m_show[0])
        {
            const float cx = x + FULL_W * 0.5f;
            auto f = [](float v) { return -140.f + std::max(0.f, std::min(320.f, v)) / 320.f * 280.f; };
            std::vector<Tick> t = { {0,"0"},{50,"50"},{100,"100"},{150,"150"},{200,"200"},{250,"250"},{300,"300"} };
            Dial(c, cx, cy, R, false, f, t, kt, 10.f);
            snprintf(buf, sizeof(buf), "%d kt", (int)kt);
            Under(cx, _LC("AirDashboard", "AIRSPEED"), buf);
            x += FULL_W;
        }
        // Altimeter (one revolution = 1000 ft, scale in hundreds of feet)
        if (m_show[1])
        {
            const float cx = x + FULL_W * 0.5f;
            auto f = [](float v) { return v * 36.f; };
            std::vector<Tick> t = { {0,"0"},{1,"1"},{2,"2"},{3,"3"},{4,"4"},{5,"5"},{6,"6"},{7,"7"},{8,"8"},{9,"9"} };
            const float hundreds = fmodf(fmodf(alt_ft, 1000.f) + 1000.f, 1000.f) / 100.f;
            Dial(c, cx, cy, R, false, f, t, hundreds, 10.f);
            snprintf(buf, sizeof(buf), "%d ft", (int)alt_ft);
            Under(cx, _LC("AirDashboard", "ALTITUDE x100"), buf);
            x += FULL_W;
        }
        // Vertical speed (half dial, 0 at 9 o'clock, scale in 1000 ft/min)
        if (m_show[2])
        {
            const float px = x + HALF_W - 6.f;
            auto f = [](float v) {
                const float s = v < 0.f ? -1.f : 1.f, a = std::min(6.5f, fabsf(v));
                const float deg = (a <= 4.f) ? a * 12.5f : 50.f + (a - 4.f) * 10.f;
                return -90.f + s * deg;
            };
            std::vector<Tick> t = { {-6,"6"},{-4,"4"},{-2,"2"},{0,"0"},{2,"2"},{4,"4"},{6,"6"} };
            const float vvi = sb.simbuf_node0_velo.y * 196.85f;
            Dial(c, px, cy, RH, true, f, t, vvi / 1000.f, 9.f);
            snprintf(buf, sizeof(buf), "%+d fpm", (int)vvi);
            Under(px - 22.f, _LC("AirDashboard", "VERT SPEED"), buf);
            x += HALF_W;
        }
        // Attitude indicator
        if (m_show[3])
        {
            const float cx = x + FULL_W * 0.5f;
            Ogre::Vector3 rollv = nodes[sb.simbuf_camera0_pos_node].AbsPosition - nodes[sb.simbuf_camera0_roll_node].AbsPosition;
            rollv.normalise();
            float roll = asinf(std::max(-1.f, std::min(1.f, rollv.dotProduct(Ogre::Vector3::UNIT_Y))));
            const Ogre::Vector3 dirv = sb.simbuf_direction;
            const float pitch = asinf(std::max(-1.f, std::min(1.f, dirv.dotProduct(Ogre::Vector3::UNIT_Y))));
            const Ogre::Vector3 upv = dirv.crossProduct(-rollv);
            if (upv.y < 0)
                roll = 3.14159f - roll;
            const float cr = cosf(roll), sr = sinf(roll);
            auto Rot = [&](float px, float py) { return c.P(cx + px * cr - py * sr, cy + px * sr + py * cr); };
            const float pitch_deg = pitch / DEG2RAD;
            const float ar = R - 8.f;
            const float ppd = 1.1f; // design units per degree of pitch
            const float d = std::max(-ar * 1.5f, std::min(ar * 1.5f, pitch_deg * ppd));
            c.dl->AddCircleFilled(c.P(cx, cy), R * k, COL_BG, 48);
            c.dl->AddCircleFilled(c.P(cx, cy), ar * k, COL_SKY, 48);
            if (d <= -ar)
                c.dl->AddCircleFilled(c.P(cx, cy), ar * k, COL_GROUND, 48);
            else if (d < ar)
            {
                const float phi = acosf(d / ar);
                std::vector<ImVec2> pts;
                for (int i = 0; i <= 28; ++i)
                {
                    const float tt = phi - 2.f * phi * i / 28.f; // clockwise on screen
                    pts.push_back(Rot(ar * sinf(tt), ar * cosf(tt)));
                }
                c.dl->AddConvexPolyFilled(pts.data(), (int)pts.size(), COL_GROUND);
            }
            // pitch ladder with degree numbers
            for (int deg = -40; deg <= 40; deg += 10)
            {
                const float y = d - deg * ppd;
                if (deg == 0 || fabsf(y) > ar - 7.f) continue;
                const bool major = (deg % 20 == 0);
                const float hw = major ? 12.f : 6.f;
                c.dl->AddLine(Rot(-hw, y), Rot(hw, y), IM_COL32_WHITE, 1.2f * k);
                if (major)
                {
                    char s[8]; snprintf(s, sizeof(s), "%d", abs(deg));
                    const ImVec2 ts = c.Size(s, 8.f);
                    const ImVec2 pl = Rot(-hw - 8.f, y), pr = Rot(hw + 8.f, y);
                    c.dl->AddText(ImGui::GetFont(), 8.f * k, ImVec2(pl.x - ts.x * 0.5f, pl.y - ts.y * 0.5f), IM_COL32_WHITE, s);
                    c.dl->AddText(ImGui::GetFont(), 8.f * k, ImVec2(pr.x - ts.x * 0.5f, pr.y - ts.y * 0.5f), IM_COL32_WHITE, s);
                }
            }
            if (fabsf(d) < ar)
                c.dl->AddLine(Rot(-ar, d), Rot(ar, d), IM_COL32_WHITE, 1.3f * k);
            // roll scale on the outer ring (fixed): 0, 30, 60
            c.dl->AddCircle(c.P(cx, cy), R * k, COL_RING, 48, 1.f);
            const float roll_marks[5] = { -60.f, -30.f, 0.f, 30.f, 60.f };
            for (int i = 0; i < 5; ++i)
            {
                c.dl->AddLine(c.Pol(cx, cy, R - 1.f, roll_marks[i]), c.Pol(cx, cy, R - 6.f, roll_marks[i]), COL_TICK, 1.3f * k);
                if (roll_marks[i] != 0.f)
                {
                    char s[8]; snprintf(s, sizeof(s), "%d", (int)fabsf(roll_marks[i]));
                    const ImVec2 lp = c.Pol(cx, cy, R + 7.f, roll_marks[i]);
                    const ImVec2 ts = c.Size(s, 7.f);
                    c.dl->AddText(ImGui::GetFont(), 7.f * k, ImVec2(lp.x - ts.x * 0.5f, lp.y - ts.y * 0.5f), COL_MUTED, s);
                }
            }
            // roll pointer (moves with the horizon) and fixed aircraft reference
            c.dl->AddTriangleFilled(Rot(0.f, -ar + 1.f), Rot(-3.f, -ar + 8.f), Rot(3.f, -ar + 8.f), IM_COL32_WHITE);
            c.dl->AddTriangleFilled(c.P(cx, cy - R + 1.f), c.P(cx - 3.f, cy - R - 5.f), c.P(cx + 3.f, cy - R - 5.f), COL_NEEDLE);
            c.Line(cx - 36.f, cy, cx - 11.f, cy, COL_NEEDLE, 3.f);
            c.Line(cx + 11.f, cy, cx + 36.f, cy, COL_NEEDLE, 3.f);
            c.dl->AddCircleFilled(c.P(cx, cy), 3.f * k, COL_NEEDLE, 10);
            float rd = roll / DEG2RAD;
            while (rd > 180.f) rd -= 360.f;
            while (rd < -180.f) rd += 360.f;
            snprintf(buf, sizeof(buf), "P %+d  R %+d", (int)pitch_deg, (int)rd);
            Under(cx, _LC("AirDashboard", "ATTITUDE"), buf);
            x += FULL_W;
        }
        // AOA (half dial)
        if (m_show[4])
        {
            const float px = x + HALF_W - 6.f;
            float a = sb.simbuf_wing4_aoa;
            if (kt < 10.f) a = 0.f;
            auto f = [](float v) { return -90.f + std::max(-24.f, std::min(24.f, v)) * 3.5f; };
            std::vector<Tick> t = { {-20,"-20"},{-10,"-10"},{0,"0"},{10,"10"},{20,"20"} };
            Dial(c, px, cy, RH, true, f, t, a, 9.f);
            snprintf(buf, sizeof(buf), "%.1f", a);
            Under(px - 22.f, "AOA", buf);
            x += HALF_W;
        }
        // HSI
        if (m_show[5])
        {
            const float cx = x + FULL_W * 0.5f;
            const Ogre::Vector3 dirv = sb.simbuf_direction;
            const float hdg = atan2f(dirv.dotProduct(Ogre::Vector3::UNIT_X), dirv.dotProduct(-Ogre::Vector3::UNIT_Z)) / DEG2RAD;
            c.dl->AddCircleFilled(c.P(cx, cy), R * k, COL_BG, 48);
            c.dl->AddCircle(c.P(cx, cy), R * k, COL_RING, 48, 1.f);
            const char* names[12] = { "N", "3", "6", "E", "12", "15", "S", "21", "24", "W", "30", "33" };
            for (int i = 0; i < 12; ++i)
            {
                const float a = i * 30.f - hdg;
                c.dl->AddLine(c.Pol(cx, cy, R - 1.5f, a), c.Pol(cx, cy, R - 7.f, a), COL_TICK, 1.5f * k);
                const ImVec2 lp = c.Pol(cx, cy, R - 15.f, a);
                const ImVec2 ts = c.Size(names[i], 10.f);
                c.dl->AddText(ImGui::GetFont(), 10.f * k, ImVec2(lp.x - ts.x * 0.5f, lp.y - ts.y * 0.5f), (i % 3 == 0) ? COL_NEEDLE : COL_TEXT, names[i]);
            }
            const float ba = (float)sb.simbuf_ap_heading_value - hdg;
            c.dl->AddTriangleFilled(c.Pol(cx, cy, R - 1.f, ba), c.Pol(cx, cy, R - 9.f, ba - 6.f), c.Pol(cx, cy, R - 9.f, ba + 6.f), COL_BLUE);
            c.dl->AddTriangleFilled(c.P(cx, cy - 18.f), c.P(cx - 5.f, cy - 6.f), c.P(cx + 5.f, cy - 6.f), COL_NEEDLE);
            c.Line(cx, cy - 6.f, cx, cy + 18.f, COL_TICK, 2.f);
            if (sb.simbuf_ap_ils_available)
            {
                const float hd = std::max(-15.f, std::min(15.f, sb.simbuf_ap_ils_hdev)) / 15.f * 28.f;
                const float vd = std::max(-15.f, std::min(15.f, sb.simbuf_ap_ils_vdev)) / 15.f * 28.f;
                c.Line(cx + hd, cy - 28.f, cx + hd, cy + 28.f, COL_GREEN, 1.5f);
                c.Line(cx - 28.f, cy + vd, cx + 28.f, cy + vd, COL_GREEN, 1.5f);
            }
            const float hd360 = fmodf(hdg + 360.f, 360.f);
            snprintf(buf, sizeof(buf), "%03d", (int)hd360);
            Under(cx, _LC("AirDashboard", "HEADING"), buf);
            x += FULL_W;
        }

        // ---- Engines: one column per engine ----
        for (int i = 0; i < n; ++i)
        {
            const AeroEngineSB& ae = sb.simbuf_aeroengines[i];
            const bool is_prop = (ae.simbuf_ae_type == AeroEngineType::AE_XPROP);
            const float x0 = eng_x + col_w * i;
            const float ex = x0 + 22.f;
            auto frpm = [](float v) { return -130.f + std::max(0.f, std::min(110.f, v)) / 110.f * 260.f; };
            std::vector<Tick> t_rpm = { {0,"0"},{60,"60"},{110,"110"} };
            int row = 0;
            auto RowY = [&](int rr) { return 30.f + 58.f * rr; };
            if (m_show[6])
            {
                const float ey = RowY(row++);
                Dial(c, ex, ey, 22.f, false, frpm, t_rpm, ae.simbuf_ae_rpmpc, 7.f);
                snprintf(buf, sizeof(buf), "%d%%", (int)ae.simbuf_ae_rpmpc);
                c.TextLR(ex, ey + 32.f, "RPM ", buf, 8.f);
            }
            if (is_prop && m_show[7])
            {
                const float ey = RowY(row++);
                auto fp = [](float v) { return std::max(-65.f, std::min(65.f, v)) * 2.f; };
                std::vector<Tick> t_p = { {-60,"-"},{0,"0"},{60,"+"} };
                Dial(c, ex, ey, 22.f, false, fp, t_p, ae.simbuf_tp_aepitch, 7.f);
                snprintf(buf, sizeof(buf), "%d", (int)ae.simbuf_tp_aepitch);
                c.TextLR(ex, ey + 32.f, "PITCH ", buf, 8.f);
            }
            if (is_prop && m_show[8])
            {
                const float ey = RowY(row++);
                Dial(c, ex, ey, 22.f, false, frpm, t_rpm, ae.simbuf_tp_aetorque, 7.f);
                snprintf(buf, sizeof(buf), "%d%%", (int)ae.simbuf_tp_aetorque);
                c.TextLR(ex, ey + 32.f, "TORQ ", buf, 8.f);
            }
            // Throttle lever
            const float tx = x0 + (has_dials ? 50.f : 14.f), ty = 12.f, th = 160.f;
            c.dl->AddRectFilled(c.P(tx, ty), c.P(tx + 8.f, ty + th), COL_PILL, 4.f * k);
            const float thr = std::max(0.f, std::min(1.f, ae.simbuf_ae_throttle));
            const float hy = ty + (1.f - thr) * (th - 8.f);
            c.dl->AddRectFilled(c.P(tx - 3.f, hy), c.P(tx + 11.f, hy + 8.f), COL_NEEDLE, 2.f * k);
            ImGui::SetCursorScreenPos(c.P(tx - 6.f, ty - 4.f));
            ImGui::PushID(1000 + i);
            ImGui::InvisibleButton("thr", ImVec2(20.f * k, (th + 8.f) * k));
            ImGui::PopID();
            if (ImGui::IsItemActive() && actor->ar_aeroengines[i])
            {
                const float v = 1.f - ((ImGui::GetIO().MousePos.y - c.P(0.f, ty).y - 4.f * k) / ((th - 8.f) * k));
                actor->ar_aeroengines[i]->setThrottle(std::max(0.f, std::min(1.f, v)));
            }
            // Start button, centered under the lever (turns red with FIRE on engine failure)
            const float px0 = tx + 4.f - 14.f;
            const ImU32 col = ae.simbuf_ae_failed ? COL_RED : (ae.simbuf_ae_ignition ? COL_GREEN : COL_PILL);
            c.dl->AddRectFilled(c.P(px0, 183.f), c.P(px0 + 28.f, 195.f), col, 6.f * k);
            c.Text(px0 + 14.f, 189.f, ae.simbuf_ae_failed ? "FIRE" : (ae.simbuf_ae_ignition ? "ON" : "OFF"), 8.f, IM_COL32_WHITE);
            if (Hit(px0, 183.f, 28.f, 12.f) && actor->ar_aeroengines[i])
                actor->ar_aeroengines[i]->flipStart();
        }

        // ---- Settings popup ----
        if (ImGui::BeginPopup("AirDashSettings"))
        {
            // Translated each frame; std::string keeps the text alive while ImGui uses it
            const std::string names[9] = {
                _LC("AirDashboard", "Airspeed"), _LC("AirDashboard", "Altitude"), _LC("AirDashboard", "Vertical speed"),
                _LC("AirDashboard", "Attitude"), _LC("AirDashboard", "AOA"), _LC("AirDashboard", "Heading (HSI)"),
                _LC("AirDashboard", "Engine RPM"), _LC("AirDashboard", "Engine pitch"), _LC("AirDashboard", "Engine torque") };
            for (int i = 0; i < 9; ++i)
                ImGui::Checkbox(fmt::format("{}##show{}", names[i], i).c_str(), &m_show[i]);
            ImGui::Checkbox(fmt::format("{}##showtemp", _LC("AirDashboard", "Outside temperature")).c_str(), &m_show_temp);
            ImGui::Separator();
            ImGui::Checkbox(fmt::format("{}##transparent", _LC("AirDashboard", "Transparent background")).c_str(), &m_transparent);
            ImGui::SliderFloat(fmt::format("{}##scale", _LC("AirDashboard", "Scale")).c_str(), &m_scale_mul, 0.6f, 2.0f, "%.2f");
            if (ImGui::Button(fmt::format("{}##resetpos", _LC("AirDashboard", "Reset position")).c_str()))
                m_reset_pos = true;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}
