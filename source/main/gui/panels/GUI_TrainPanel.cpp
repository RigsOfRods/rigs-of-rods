/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

#include "GUI_TrainPanel.h"

#include "Actor.h"
#include "Application.h"
#include "CacheSystem.h"
#include "Engine.h"
#include "GameContext.h"
#include "GfxActor.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "Language.h"
#include "SimData.h"

#include <fmt/format.h>
#include <algorithm>
#include <cmath>

using namespace RoR;
using namespace GUI;

static const ImU32 COL_BAR_BG      = IM_COL32( 18,  19,  21, 255);
static const ImU32 COL_THROTTLE    = IM_COL32( 55, 138, 221, 255);
static const ImU32 COL_BRAKE       = IM_COL32(226,  75,  74, 255);
static const ImU32 COL_GRAB        = IM_COL32(208, 211, 216, 255);
static const ImU32 COL_GRAB_EDGE_T = IM_COL32(133, 183, 235, 255);
static const ImU32 COL_GRAB_EDGE_B = IM_COL32(240, 149, 149, 255);
static const ImU32 COL_DIAL_BG     = IM_COL32( 18,  19,  21, 255);
static const ImU32 COL_DIAL_RIM    = IM_COL32( 90,  93,  99, 255);
static const ImU32 COL_DIAL_TRACK  = IM_COL32( 60,  63,  68, 255);
static const ImU32 COL_DIAL_OK     = IM_COL32( 79, 160,  90, 255);
static const ImU32 COL_DIAL_OVER   = IM_COL32(226,  75,  74, 255);
static const ImU32 COL_TEXT        = IM_COL32(230, 230, 230, 255);
static const ImU32 COL_MUTED       = IM_COL32(154, 157, 163, 255);
static const ImU32 COL_BTN_BLUE    = IM_COL32( 43,  79, 128, 255);
static const ImU32 COL_BTN_RED     = IM_COL32(138,  48,  48, 255);
static const ImU32 COL_BTN_GREY    = IM_COL32( 58,  61,  66, 255);
static const ImU32 COL_BTN_ON      = IM_COL32( 46, 107,  58, 255);

static const float BASE_WIDTH = 300.f; // px at ui_scale_factor 1.0

static ImVec4 Muted() { return ImGui::ColorConvertU32ToFloat4(COL_MUTED); }

static ImU32 Lighten(ImU32 c, int d)
{
    const int r = std::min(255, (int)((c >> IM_COL32_R_SHIFT) & 0xFF) + d);
    const int g = std::min(255, (int)((c >> IM_COL32_G_SHIFT) & 0xFF) + d);
    const int b = std::min(255, (int)((c >> IM_COL32_B_SHIFT) & 0xFF) + d);
    return IM_COL32(r, g, b, (c >> IM_COL32_A_SHIFT) & 0xFF);
}

static events CommandEvent(int key) // commandkeys are indexed 1-MAX_COMMANDS
{
    return (events)(EV_COMMANDS_01 + (key - 1));
}

static bool IsUsable(const ActorPtr& actor)
{
    return actor && actor->ar_state != ActorState::DISPOSED;
}

// -------------------------------------------------------------------------------------------------

bool TrainPanel::IsTrain(const ActorPtr& actor)
{
    if (!IsUsable(actor) || !actor->ar_engine)
        return false;

    const CacheEntryPtr& entry = actor->getUsedActorEntry();
    if (!entry)
        return false;

    std::string ext = entry->fext;
    Ogre::StringUtil::toLowerCase(ext);
    return ext == "train";
}

void TrainPanel::Detach()
{
    if (IsUsable(m_actor) && (m_actor->ar_state == ActorState::LOCAL_SIMULATED || m_actor->ar_state == ActorState::LOCAL_SLEEPING))
    {
        m_actor->resetEventSimulatedValue(EV_TRUCK_ACCELERATE);
        m_actor->resetEventSimulatedValue(EV_TRUCK_BRAKE);
        for (int ev : m_held)
            m_actor->resetEventSimulatedValue(ev);
        for (events ev : m_pulses)
            m_actor->resetEventSimulatedValue(ev);
    }
    m_actor = nullptr;
    m_throttle = 0.f;
    m_train_brake = 0.f;
    m_held.clear();
    m_pulses.clear();
}

void TrainPanel::Draw(RoR::GfxActor* player)
{
    ActorPtr actor = (player != nullptr) ? player->GetActor() : ActorPtr();
    const bool is_train = IsTrain(actor);

    // Player left the locomotive or switched vehicles: give the controls back.
    if (m_actor && (!is_train || m_actor != actor))
        this->Detach();
    if (!is_train || (actor->ar_state != ActorState::LOCAL_SIMULATED && actor->ar_state != ActorState::LOCAL_SLEEPING))
        return;
    m_actor = actor;

    // Toggles from the previous frame are released now.
    for (events ev : m_pulses)
        actor->resetEventSimulatedValue(ev);
    m_pulses.clear();

    if (m_closed_for == actor->ar_instance_id)
    {
        m_throttle = 0.f;
        m_train_brake = 0.f;
        this->ApplyLevers(actor);
        return;
    }

    this->ApplyLevers(actor);

    const float scale = App::GetGuiManager()->GetUiScaleFactor();
    ImGui::SetNextWindowPos(ImVec2(20.f, 200.f), ImGuiCond_FirstUseEver);
    // Minimum width only - the window grows with its content (long names, big fonts)
    ImGui::SetNextWindowSizeConstraints(ImVec2(BASE_WIDTH * scale, 0.f), ImVec2(FLT_MAX, FLT_MAX));
    bool open = true;
    const std::string title = fmt::format("{}###TrainPanel", actor->getTruckName());
    if (ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
    {
        m_held_now.clear();

        this->DrawStatus(actor);
        ImGui::Dummy(ImVec2(0.f, 4.f * scale));

        // Show the keyboard value while a key is held (keyboard wins, see ApplyLevers()).
        const float key_accel = App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE);
        const float key_brake = App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE);
        this->DrawBar("##throttle", _LC("TrainPanel", "Throttle"), m_throttle, std::max(m_throttle, key_accel), COL_THROTTLE, COL_GRAB_EDGE_T);
        ImGui::Dummy(ImVec2(0.f, 2.f * scale));
        this->DrawBar("##trainbrake", _LC("TrainPanel", "Train brake"), m_train_brake, std::max(m_train_brake, key_brake), COL_BRAKE, COL_GRAB_EDGE_B);

        ImGui::Separator();
        this->DrawSwitches(actor);
        this->DrawCouplings(actor);
        this->DrawCommands(actor);

        // Buttons released this frame: hand the event back to the keyboard.
        for (int ev : m_held)
        {
            if (m_held_now.count(ev) == 0)
                actor->resetEventSimulatedValue(ev);
        }
        m_held = m_held_now;
    }
    ImGui::End();

    if (!open)
    {
        m_closed_for = actor->ar_instance_id;
        m_throttle = 0.f;
        m_train_brake = 0.f;
        this->ApplyLevers(actor);
    }
}

// -------------------------------------------------------------------------------------------------

void TrainPanel::ApplyLevers(const ActorPtr& actor)
{
    // Keyboard/joystick wins: as soon as it is used, the lever drops back to 0.
    if (App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE) > 0.02f)
        m_throttle = 0.f;
    if (App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE) > 0.02f)
        m_train_brake = 0.f;

    if (m_throttle > 0.f)
        actor->setEventSimulatedValue(EV_TRUCK_ACCELERATE, m_throttle);
    else
        actor->resetEventSimulatedValue(EV_TRUCK_ACCELERATE);

    if (m_train_brake > 0.f)
        actor->setEventSimulatedValue(EV_TRUCK_BRAKE, m_train_brake);
    else
        actor->resetEventSimulatedValue(EV_TRUCK_BRAKE);
}

void TrainPanel::DrawStatus(const ActorPtr& actor)
{
    const float kmh = actor->getSpeed() * 3.6f;
    const float limit_kmh = actor->sl_enabled ? actor->sl_speed_limit * 3.6f : 0.f;

    this->DrawTacho(kmh, limit_kmh);
    ImGui::SameLine();

    ImGui::BeginGroup();
    std::string dir = _LC("TrainPanel", "neutral");
    float rpm = 0.f;
    if (actor->ar_engine)
    {
        const int gear = actor->ar_engine->getGear();
        if (gear > 0)
            dir = _LC("TrainPanel", "forward");
        else if (gear < 0)
            dir = _LC("TrainPanel", "reverse");
        rpm = actor->ar_engine->getRPM();
    }
    const std::string lbl_dir = _LC("TrainPanel", "Direction");
    const std::string lbl_engine = _LC("TrainPanel", "Engine");
    const std::string lbl_limit = _LC("TrainPanel", "Limit");
    // Value column right of the widest label
    const float label_w = std::max({ ImGui::CalcTextSize(lbl_dir.c_str()).x, ImGui::CalcTextSize(lbl_engine.c_str()).x, ImGui::CalcTextSize(lbl_limit.c_str()).x });
    const float col2 = ImGui::GetCursorPosX() + label_w + ImGui::GetStyle().ItemSpacing.x * 2.f;
    ImGui::TextColored(Muted(), "%s", lbl_dir.c_str());    ImGui::SameLine(col2); ImGui::Text("%s", dir.c_str());
    ImGui::TextColored(Muted(), "%s", lbl_engine.c_str()); ImGui::SameLine(col2); ImGui::Text("%.0f %s", rpm, _LC("TrainPanel", "rpm"));
    ImGui::TextColored(Muted(), "%s", lbl_limit.c_str());  ImGui::SameLine(col2);
    if (limit_kmh > 0.f)
        ImGui::Text("%.0f km/h", limit_kmh);
    else
        ImGui::Text("%s", _LC("TrainPanel", "off"));
    ImGui::EndGroup();
}

void TrainPanel::DrawTacho(float kmh, float limit_kmh)
{
    const float scale = App::GetGuiManager()->GetUiScaleFactor();
    const float size = 92.f * scale;
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(size, size));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Scale end: next multiple of 20 above 1.2x the limit (or 120 without limit)
    float max_kmh = (limit_kmh > 0.f) ? std::ceil(limit_kmh * 1.2f / 20.f) * 20.f : 120.f;
    max_kmh = std::max(max_kmh, std::ceil(kmh / 20.f) * 20.f);
    max_kmh = std::max(max_kmh, 20.f);

    const ImVec2 c(p0.x + size * 0.5f, p0.y + size * 0.5f);
    const float r = size * 0.45f;
    dl->AddCircleFilled(c, r, COL_DIAL_BG, 32);
    dl->AddCircle(c, r, COL_DIAL_RIM, 32, 2.f);

    // 270 degree arc from bottom-left (135 deg) clockwise to bottom-right (405 deg), screen coords
    const float a0 = 135.f * IM_PI / 180.f;
    const float sweep = 270.f * IM_PI / 180.f;
    const float frac = ImClamp(kmh / max_kmh, 0.f, 1.f);
    const float track_r = r - 6.f * scale;
    const float thick = 5.f * scale;

    dl->PathArcTo(c, track_r, a0, a0 + sweep, 32);
    dl->PathStroke(COL_DIAL_TRACK, false, thick);
    if (frac > 0.f)
    {
        const bool over = (limit_kmh > 0.f) && (kmh > limit_kmh);
        dl->PathArcTo(c, track_r, a0, a0 + sweep * frac, 32);
        dl->PathStroke(over ? COL_DIAL_OVER : COL_DIAL_OK, false, thick);
    }
    if (limit_kmh > 0.f) // limit tick
    {
        const float al = a0 + sweep * ImClamp(limit_kmh / max_kmh, 0.f, 1.f);
        dl->AddLine(ImVec2(c.x + cosf(al) * (r - 12.f * scale), c.y + sinf(al) * (r - 12.f * scale)),
                    ImVec2(c.x + cosf(al) * r, c.y + sinf(al) * r), COL_DIAL_OVER, 2.f);
    }

    const float an = a0 + sweep * frac;
    dl->AddLine(c, ImVec2(c.x + cosf(an) * (r - 10.f * scale), c.y + sinf(an) * (r - 10.f * scale)), COL_DIAL_OVER, 3.f * scale);
    dl->AddCircleFilled(c, 4.f * scale, COL_TEXT, 12);

    const std::string val = fmt::format("{:.0f}", kmh);
    const ImVec2 vs = ImGui::CalcTextSize(val.c_str());
    dl->AddText(ImVec2(c.x - vs.x * 0.5f, c.y + 10.f * scale), COL_TEXT, val.c_str());
    const ImVec2 us = ImGui::CalcTextSize("km/h");
    dl->AddText(ImVec2(c.x - us.x * 0.5f, c.y + 10.f * scale + vs.y), COL_MUTED, "km/h");
}

void TrainPanel::DrawBar(const char* id, const char* label, float& value, float shown, ImU32 fill, ImU32 grab_edge)
{
    const float scale = App::GetGuiManager()->GetUiScaleFactor();

    ImGui::TextColored(Muted(), "%s", label);
    const std::string pct = fmt::format("{:.0f} %", shown * 100.f);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(pct.c_str()).x);
    ImGui::Text("%s", pct.c_str());

    const float w = ImGui::GetContentRegionAvail().x;
    const float h = 10.f * scale;
    const float grab_w = 8.f * scale;
    const float pad = 4.f * scale;
    const ImVec2 cur = ImGui::GetCursorScreenPos();
    const ImVec2 p0(cur.x, cur.y + pad);
    const ImVec2 p1(p0.x + w, p0.y + h);

    // Hit area slightly taller than the bar so the grab is easy to catch
    ImGui::InvisibleButton(id, ImVec2(w, h + 2.f * pad));
    if (ImGui::IsItemActive())
    {
        float v = ImClamp((ImGui::GetIO().MousePos.x - p0.x) / w, 0.f, 1.f);
        if (v < 0.01f)
            v = 0.f; // far left snaps to "off"
        value = v;
        shown = v;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", _LC("TrainPanel", "Drag the slider with the mouse. Far left = off, then the keyboard takes over again."));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, p1, COL_BAR_BG, 2.f);
    if (shown > 0.f)
        dl->AddRectFilled(p0, ImVec2(p0.x + w * shown, p1.y), fill, 2.f);

    const float gx = ImClamp(p0.x + w * shown - grab_w * 0.5f, p0.x, p1.x - grab_w);
    const ImVec2 g0(gx, p0.y - pad);
    const ImVec2 g1(gx + grab_w, p1.y + pad);
    dl->AddRectFilled(g0, g1, COL_GRAB, 2.f);
    dl->AddRect(g0, g1, grab_edge, 2.f);
}

// -------------------------------------------------------------------------------------------------

bool TrainPanel::ColoredButton(const char* label, ImVec2 size, ImU32 col)
{
    ImGui::PushStyleColor(ImGuiCol_Button, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Lighten(col, 20));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Lighten(col, 38));
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return pressed;
}

void TrainPanel::HoldButton(const ActorPtr& actor, const char* label, events ev, ImVec2 size, ImU32 col)
{
    this->ColoredButton(label, size, col);
    if (ImGui::IsItemActive())
    {
        actor->setEventSimulatedValue(ev, 1.f);
        m_held_now.insert((int)ev);
    }
}

void TrainPanel::Pulse(const ActorPtr& actor, events ev)
{
    actor->setEventSimulatedValue(ev, 1.f);
    m_pulses.push_back(ev);
}

void TrainPanel::DrawSwitches(const ActorPtr& actor)
{
    const float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    const ImVec2 half_size(half, 0.f);

    this->HoldButton(actor, _LC("TrainPanel", "Horn"), EV_TRUCK_HORN, half_size, COL_BTN_GREY);
    ImGui::SameLine();
    const bool pb = actor->getParkingBrake();
    if (this->ColoredButton(pb ? _LC("TrainPanel", "Parking brake on") : _LC("TrainPanel", "Parking brake off"), half_size, pb ? COL_BTN_RED : COL_BTN_GREY))
        actor->parkingbrakeToggle();

    // Switchable lights/props: custom light flares (u-type) and toggle-props on TRUCK_LIGHTTOGGLE01..10
    struct Switch { int number; bool on; };
    std::vector<Switch> switches;
    for (int n = 0; n < MAX_CLIGHTS; n++)
    {
        const events ev = (events)(EV_TRUCK_LIGHTTOGGLE01 + n);
        bool used = false;
        bool on = false;
        if (actor->countCustomLights(n) > 0)
        {
            used = true;
            on = actor->getCustomLightVisible(n);
        }
        for (const PropAnimKeyState& state : actor->GetPropAnimKeyStates())
        {
            if (state.event_id == ev && state.eventlock_present)
            {
                if (!used)
                    on = state.anim_active;
                used = true;
            }
        }
        if (used)
            switches.push_back({ n, on });
    }

    // Toggle-props on command keys without command beams (e.g. wipers on COMMANDS_15)
    struct CmdSwitch { int key; bool on; };
    std::vector<CmdSwitch> cmd_switches;
    for (const PropAnimKeyState& state : actor->GetPropAnimKeyStates())
    {
        if (!state.eventlock_present || state.event_id < EV_COMMANDS_01 || state.event_id >= EV_COMMANDS_01 + MAX_COMMANDS)
            continue;
        const int key = (int)state.event_id - (int)EV_COMMANDS_01 + 1;
        if (!actor->ar_command_key[key].beams.empty() || !actor->ar_command_key[key].rotators.empty())
            continue; // a real command, listed below
        bool dup = false;
        for (const CmdSwitch& s : cmd_switches)
            dup = dup || (s.key == key);
        if (!dup)
            cmd_switches.push_back({ key, state.anim_active });
    }

    int col = 0;
    auto draw_toggle = [&](const std::string& label, bool on, events ev)
    {
        if (col % 2 == 1)
            ImGui::SameLine();
        if (this->ColoredButton(label.c_str(), half_size, on ? COL_BTN_ON : COL_BTN_GREY))
            this->Pulse(actor, ev);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s %s", _LC("TrainPanel", "Key:"), App::GetInputEngine()->getEventCommandTrimmed(ev).c_str());
        col++;
    };
    for (const Switch& s : switches)
        draw_toggle(fmt::format("{} {}{}###light{}", _LC("TrainPanel", "Light"), s.number + 1, s.on ? std::string(" ") + _LC("TrainPanel", "on") : std::string(), s.number), s.on, (events)(EV_TRUCK_LIGHTTOGGLE01 + s.number));
    for (const CmdSwitch& s : cmd_switches)
        draw_toggle(fmt::format("{} {}{}###cmdsw{}", _LC("TrainPanel", "Switch"), s.key, s.on ? std::string(" ") + _LC("TrainPanel", "on") : std::string(), s.key), s.on, CommandEvent(s.key));
}

void TrainPanel::DrawCouplings(const ActorPtr& actor)
{
    if (actor->ar_hooks.empty())
        return;

    ImGui::Separator();
    ImGui::TextColored(Muted(), "%s", _LC("TrainPanel", "Couplings"));

    const Ogre::Vector3 dir = actor->getDirection();
    const Ogre::Vector3 center = actor->getPosition();
    const float btn_w = 90.f * App::GetGuiManager()->GetUiScaleFactor();

    int index = 0;
    for (hook_t& hook : actor->ar_hooks)
    {
        index++;
        if (!hook.hk_hook_node)
            continue;

        const bool front = (hook.hk_hook_node->AbsPosition - center).dotProduct(dir) > 0.f;
        std::string state;
        switch (hook.hk_locked)
        {
        case LOCKED:
            state = hook.hk_locked_actor ? fmt::format("{} ({})", _LC("TrainPanel", "coupled"), hook.hk_locked_actor->getTruckName()) : std::string(_LC("TrainPanel", "coupled"));
            break;
        case PRELOCK:
            state = _LC("TrainPanel", "locking");
            break;
        default:
            state = _LC("TrainPanel", "open");
            break;
        }

        ImGui::PushID(index);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", front ? _LC("TrainPanel", "front") : _LC("TrainPanel", "rear"));
        ImGui::SameLine();
        ImGui::TextColored(Muted(), "%s", state.c_str());
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - btn_w);
        const bool locked = hook.hk_locked != UNLOCKED;
        if (this->ColoredButton(locked ? _LC("TrainPanel", "Uncouple") : _LC("TrainPanel", "Couple"), ImVec2(btn_w, 0.f), locked ? COL_BTN_RED : COL_BTN_BLUE))
        {
            ActorLinkingRequest* rq = new ActorLinkingRequest();
            rq->alr_type = ActorLinkingRequestType::HOOK_MOUSE_TOGGLE;
            rq->alr_actor_instance_id = actor->ar_instance_id;
            rq->alr_hook_group = hook.hk_group;
            rq->alr_hook_mousenode = hook.hk_hook_node->pos;
            App::GetGameContext()->PushMessage(Message(MSG_SIM_ACTOR_LINKING_REQUESTED, rq));
        }
        ImGui::PopID();
    }
}

void TrainPanel::DrawCommands(const ActorPtr& actor)
{
    if (actor->ar_unique_commandkey_pairs.empty())
        return;

    ImGui::Separator();
    ImGui::TextColored(Muted(), "%s", _LC("TrainPanel", "Commands (hold to operate)"));

    const float scale = App::GetGuiManager()->GetUiScaleFactor();
    const ImVec2 cmd_btn_size(28.f * scale, 0.f);
    int index = 0;
    for (const UniqueCommandKeyPair& pair : actor->ar_unique_commandkey_pairs)
    {
        index++;
        std::string desc = pair.uckp_description;
        std::replace(desc.begin(), desc.end(), '_', ' ');
        if (desc.empty())
            desc = fmt::format("{} {}/{}", _LC("TrainPanel", "Command"), pair.uckp_key1, pair.uckp_key2);

        ImGui::PushID(index);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", desc.c_str());
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - cmd_btn_size.x * 2.f - ImGui::GetStyle().ItemSpacing.x);
        this->HoldButton(actor, "-", CommandEvent(pair.uckp_key1), cmd_btn_size, COL_BTN_GREY);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s %s", _LC("TrainPanel", "Key:"), App::GetInputEngine()->getEventCommandTrimmed(CommandEvent(pair.uckp_key1)).c_str());
        ImGui::SameLine();
        this->HoldButton(actor, "+", CommandEvent(pair.uckp_key2), cmd_btn_size, COL_BTN_GREY);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s %s", _LC("TrainPanel", "Key:"), App::GetInputEngine()->getEventCommandTrimmed(CommandEvent(pair.uckp_key2)).c_str());
        ImGui::PopID();
    }
}
