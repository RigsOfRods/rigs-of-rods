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

#include "GUI_VehicleButtons.h"

#include "Application.h"
#include "Actor.h"
#include "GfxActor.h"
#include "GUIManager.h"
#include "SoundScriptManager.h"
#include "GameContext.h"
#include "InputEngine.h"
#include "EngineSim.h"
#include "Console.h"
#include "CameraManager.h"
#include "GUIUtils.h"

#include <Ogre.h>
#include <imgui_internal.h>

using namespace RoR;
using namespace GUI;

void VehicleButtons::Draw(RoR::GfxActor* actorx)
{
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
    ImGui::SetNextWindowPos(ImVec2(theme.screen_edge_padding.x - ImGui::GetStyle().WindowPadding.x, 100.f));
    ImVec2 window_pos = ImVec2(158, 0);
    ImGui::SetNextWindowSize(window_pos);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));

    if (!m_icons_cached)
    {
        this->CacheIcons();
    }

    bool is_visible = false;

    // Show only once for 5 sec, with a notice
    if (actorx && !m_init)
    {
        m_timer.reset();
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                      fmt::format(_LC("VehicleButtons", "Hover the mouse on the left to see controls")), "lightbulb.png");
        m_init = true;
    }
    if (m_timer.getMilliseconds() < 5000)
    {
        is_visible = true;
    }

    // Show when mouse is on the left of screen
    if (App::GetGuiManager()->AreStaticMenusAllowed() &&
        (ImGui::GetIO().MousePos.x <= window_pos.x + ImGui::GetStyle().WindowPadding.x) && ImGui::GetIO().MousePos.y <= ImGui::GetIO().DisplaySize.y && !ImGui::IsMouseDown(1))
    {
        is_visible = true;
    }

    ImGui::Begin("VehicleButtons", nullptr, flags);

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemInnerSpacing.x); // Align with the buttons
    if (is_visible && ImGui::CollapsingHeader(_LC("VehicleButtons", "Main Controls")))
    {
        this->DrawHeadLightButton(actorx);
        ImGui::SameLine();
        this->DrawLeftBlinkerButton(actorx);
        ImGui::SameLine();
        this->DrawRightBlinkerButton(actorx);
        ImGui::SameLine();
        this->DrawWarnBlinkerButton(actorx);
        this->DrawShiftModeButton(actorx);
        ImGui::SameLine();
        this->DrawEngineButton(actorx);
        ImGui::SameLine();
        this->DrawRepairButton(actorx);
        ImGui::SameLine();
        this->DrawParkingBrakeButton(actorx);
        this->DrawTractionControlButton(actorx);
        ImGui::SameLine();
        this->DrawAbsButton(actorx);
        ImGui::SameLine();
        this->DrawLockButton(actorx);
        ImGui::SameLine();
        this->DrawSecureButton(actorx);
        this->DrawAxleDiffButton(actorx);
        ImGui::SameLine();
        this->DrawWheelDiffButton(actorx);
        ImGui::SameLine();
        this->DrawTransferCaseModeButton(actorx);
        ImGui::SameLine();
        this->DrawTransferCaseGearRatioButton(actorx);
        this->DrawParticlesButton(actorx);
        ImGui::SameLine();
        this->DrawBeaconButton(actorx);
        ImGui::SameLine();
        this->DrawHornButton(actorx);
        ImGui::SameLine();
        this->DrawMirrorButton(actorx);
        this->DrawCruiseControlButton(actorx);
        ImGui::SameLine();
        this->DrawCameraButton();
        ImGui::SameLine();
        this->DrawActorPhysicsButton(actorx);
        ImGui::SameLine();
        this->DrawPhysicsButton();
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemInnerSpacing.x); // Align with the buttons
    if (is_visible && ImGui::CollapsingHeader(_LC("VehicleButtons", "Custom Lights")))
    {
        this->DrawCustomLightButton(actorx);
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemInnerSpacing.x); // Align with the buttons
    if (is_visible && ImGui::CollapsingHeader(_LC("VehicleButtons", "Commands")))
    {
        this->DrawCommandButton(actorx);
    }

    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg
}

void VehicleButtons::DrawHeadLightButton(RoR::GfxActor* actorx)
{
    bool has_headlight = false;
    for (int i = 0; i < actorx->GetActor()->ar_flares.size(); i++)
    {
        if (actorx->GetActor()->ar_flares[i].fl_type == FlareType::HEADLIGHT || actorx->GetActor()->ar_flares[i].fl_type == FlareType::TAIL_LIGHT)
        {
            has_headlight = true;
        }
    }

    if (!has_headlight)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->getHeadlightsVisible())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_headlight_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->toggleHeadlights();
    }

    if (!has_headlight)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Head Lights", App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_TOGGLE_TRUCK_LOW_BEAMS).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawLeftBlinkerButton(RoR::GfxActor* actorx)
{
    bool has_blink = false;
    for (int i = 0; i < actorx->GetActor()->ar_flares.size(); i++)
    {
        if (actorx->GetActor()->ar_flares[i].fl_type == FlareType::BLINKER_LEFT)
        {
            has_blink = true;
        }
    }

    if (!has_blink)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->getBlinkType() == BLINK_LEFT)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_left_blinker_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->toggleBlinkType(BLINK_LEFT);
    }

    if (!has_blink)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Left Blinker", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_BLINK_LEFT).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawRightBlinkerButton(RoR::GfxActor* actorx)
{
    bool has_blink = false;
    for (int i = 0; i < actorx->GetActor()->ar_flares.size(); i++)
    {
        if (actorx->GetActor()->ar_flares[i].fl_type == FlareType::BLINKER_RIGHT)
        {
            has_blink = true;
        }
    }

    if (!has_blink)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->getBlinkType() == BLINK_RIGHT)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_right_blinker_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->toggleBlinkType(BLINK_RIGHT);
    }

    if (!has_blink)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Right Blinker", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_BLINK_RIGHT).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawWarnBlinkerButton(RoR::GfxActor* actorx)
{
    bool has_blink = false;
    for (int i = 0; i < actorx->GetActor()->ar_flares.size(); i++)
    {
        if (actorx->GetActor()->ar_flares[i].fl_type == FlareType::BLINKER_LEFT)
        {
            has_blink = true;
        }
    }

    if (!has_blink)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->getBlinkType() == BLINK_WARN)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_warning_light_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->toggleBlinkType(BLINK_WARN);
    }

    if (!has_blink)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Warning Lights", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_BLINK_WARN).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawHornButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->getTruckType() != TRUCK)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (SOUND_GET_STATE(actorx->GetActor()->ar_instance_id, SS_TRIG_HORN))
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (actorx->GetActor()->ar_is_police) // Police siren
    {
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_horn_icon->getHandle()), ImVec2(24, 24)))
        {
            SOUND_TOGGLE(actorx->GetActor(), SS_TRIG_HORN);
        }
    }
    else
    {
        // Triggering continuous command every frame is sloppy
        // Set state and read it in GameContext via GetHornButtonState()
        ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_horn_icon->getHandle()), ImVec2(24, 24));
        if (ImGui::IsItemActive())
        {
            m_horn = true;
        }
        else
        {
            m_horn = false;
        }
    }

    if (actorx->GetActor()->getTruckType() != TRUCK)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Horn", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_HORN).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawMirrorButton(RoR::GfxActor* actorx)
{
    if (!actorx->hasCamera())
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetVideoCamState() == VideoCamState::VCSTATE_ENABLED_ONLINE)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_mirror_icon->getHandle()), ImVec2(24, 24)))
    {
        if (actorx->GetVideoCamState() == VideoCamState::VCSTATE_DISABLED)
        {
            actorx->SetVideoCamState(VideoCamState::VCSTATE_ENABLED_ONLINE);
        }
        else
        {
            actorx->SetVideoCamState(VideoCamState::VCSTATE_DISABLED);
        }
    }

    if (!actorx->hasCamera())
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Mirrors", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TOGGLE_VIDEOCAMERA).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawRepairButton(RoR::GfxActor* actorx)
{
    if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_repair_icon->getHandle()), ImVec2(24, 24)))
    {
        ActorModifyRequest* rq = new ActorModifyRequest;
        rq->amr_actor = actorx->GetActor()->ar_instance_id;
        rq->amr_type  = ActorModifyRequest::Type::RESET_ON_SPOT;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
    }

    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Repair", App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_REPAIR_TRUCK).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawParkingBrakeButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->getTruckType() == NOT_DRIVEABLE || actorx->GetActor()->getTruckType() == BOAT)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->getParkingBrake())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_parking_brake_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->parkingbrakeToggle();
    }

    if (actorx->GetActor()->getTruckType() == NOT_DRIVEABLE || actorx->GetActor()->getTruckType() == BOAT)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Parking Brake", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_PARKING_BRAKE).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawTractionControlButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->tc_nodash)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->tc_mode)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_traction_control_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->tractioncontrolToggle();
    }

    if (actorx->GetActor()->tc_nodash)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Traction Control", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TRACTION_CONTROL).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawAbsButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->alb_nodash)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->alb_mode)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_abs_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->antilockbrakeToggle();
    }

    if (actorx->GetActor()->alb_nodash)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "ABS", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_ANTILOCK_BRAKE).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawPhysicsButton()
{
    if (App::GetGameContext()->GetActorManager()->IsSimulationPaused())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_physics_icon->getHandle()), ImVec2(24, 24)))
    {
        if (App::GetGameContext()->GetActorManager()->IsSimulationPaused())
        {
            App::GetGameContext()->GetActorManager()->SetSimulationPaused(false);
        }
        else
        {
            App::GetGameContext()->GetActorManager()->SetSimulationPaused(true);
        }
    }

    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Physics", App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_TOGGLE_PHYSICS).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawActorPhysicsButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->ar_physics_paused)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_actor_physics_icon->getHandle()), ImVec2(24, 24)))
    {
        for (ActorPtr actor : actorx->GetActor()->getAllLinkedActors())
        {
            actor->ar_physics_paused = !actorx->GetActor()->ar_physics_paused;
        }
        actorx->GetActor()->ar_physics_paused = !actorx->GetActor()->ar_physics_paused;
    }

    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Actor Physics", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TOGGLE_PHYSICS).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawAxleDiffButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->getAxleDiffMode() == 0)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF))
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_a_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->toggleAxleDiffMode();
        actorx->GetActor()->displayAxleDiffMode();
    }

    if (actorx->GetActor()->getAxleDiffMode() == 0)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Axle Differential", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawWheelDiffButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->getWheelDiffMode() == 0)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF))
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_w_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->toggleWheelDiffMode();
        actorx->GetActor()->displayWheelDiffMode();
    }

    if (actorx->GetActor()->getWheelDiffMode() == 0)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Wheel Differential", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawTransferCaseModeButton(RoR::GfxActor* actorx)
{
    if (!actorx->GetActor()->ar_engine || !actorx->GetActor()->getTransferCaseMode() ||
         actorx->GetActor()->getTransferCaseMode()->tr_ax_2 < 0 || !actorx->GetActor()->getTransferCaseMode()->tr_2wd)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TOGGLE_TCASE_4WD_MODE))
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_m_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->toggleTransferCaseMode();
        actorx->GetActor()->displayTransferCaseMode();
    }

    if (!actorx->GetActor()->ar_engine || !actorx->GetActor()->getTransferCaseMode() ||
         actorx->GetActor()->getTransferCaseMode()->tr_ax_2 < 0 || !actorx->GetActor()->getTransferCaseMode()->tr_2wd)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Transfer Case 4WD", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TOGGLE_TCASE_4WD_MODE).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawTransferCaseGearRatioButton(RoR::GfxActor* actorx)
{
    if (!actorx->GetActor()->ar_engine || !actorx->GetActor()->getTransferCaseMode() ||
         actorx->GetActor()->getTransferCaseMode()->tr_gear_ratios.size() < 2)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO))
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_g_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->toggleTransferCaseGearRatio();
        actorx->GetActor()->displayTransferCaseMode();
    }

    if (!actorx->GetActor()->ar_engine || !actorx->GetActor()->getTransferCaseMode() ||
         actorx->GetActor()->getTransferCaseMode()->tr_gear_ratios.size() < 2)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Transfer Case Gear Ratio", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawParticlesButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->ar_num_custom_particles == 0)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->getCustomParticleMode())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_particle_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->toggleCustomParticles();
    }

    if (actorx->GetActor()->ar_num_custom_particles == 0)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Particles", App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_TOGGLE_CUSTOM_PARTICLES).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawBeaconButton(RoR::GfxActor* actorx)
{
    bool has_beacon = false;
    for (Prop& prop: actorx->getProps())
    {
        if (prop.pp_beacon_type != 0)
        {
            has_beacon = true;
        }
    }

    if (!has_beacon)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->getBeaconMode())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_beacons_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->beaconsToggle();
    }

    if (!has_beacon)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Beacons", App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_TOGGLE_TRUCK_BEACONS).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawShiftModeButton(RoR::GfxActor* actorx)
{
    if (!actorx->GetActor()->ar_engine)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SWITCH_SHIFT_MODES))
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_shift_icon->getHandle()), ImVec2(24, 24)))
    {
        if (actorx->GetActor()->ar_engine)
        {
            actorx->GetActor()->ar_engine->ToggleAutoShiftMode();
            // force gui update
            actorx->GetActor()->RequestUpdateHudFeatures();
            const char* msg = nullptr;
            switch (actorx->GetActor()->ar_engine->GetAutoShiftMode())
            {
            case SimGearboxMode::AUTO: msg = "Automatic shift";
                break;
            case SimGearboxMode::SEMI_AUTO: msg = "Manual shift - Auto clutch";
                break;
            case SimGearboxMode::MANUAL: msg = "Fully Manual: sequential shift";
                break;
            case SimGearboxMode::MANUAL_STICK: msg = "Fully manual: stick shift";
                break;
            case SimGearboxMode::MANUAL_RANGES: msg = "Fully Manual: stick shift with ranges";
                break;
            }
            App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L(msg), "cog.png");
        }
    }

    if (!actorx->GetActor()->ar_engine)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Shift Mode", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_SWITCH_SHIFT_MODES).c_str());

        if (actorx->GetActor()->ar_engine)
        {
            switch (actorx->GetActor()->ar_engine->GetAutoShiftMode())
            {
            case SimGearboxMode::AUTO:
                ImGui::Separator();
                ImGui::TextDisabled("%s (%s)", "Shift Up", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_AUTOSHIFT_UP).c_str());
                ImGui::TextDisabled("%s (%s)", "Shift Down", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_AUTOSHIFT_DOWN).c_str());
                break;
            case SimGearboxMode::SEMI_AUTO:
                ImGui::Separator();
                ImGui::TextDisabled("%s (%s)", "Shift Up", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_SHIFT_UP).c_str());
                ImGui::TextDisabled("%s (%s)", "Shift Down", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_SHIFT_DOWN).c_str());
                ImGui::TextDisabled("%s (%s)", "Shift Neutral", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_SHIFT_NEUTRAL).c_str());
                break;
            case SimGearboxMode::MANUAL:
                ImGui::Separator();
                ImGui::TextDisabled("%s (%s)", "Shift Up", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_SHIFT_UP).c_str());
                ImGui::TextDisabled("%s (%s)", "Shift Down", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_SHIFT_DOWN).c_str());
                ImGui::TextDisabled("%s (%s)", "Shift Neutral", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_SHIFT_NEUTRAL).c_str());
                ImGui::TextDisabled("%s (%s)", "Clutch", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_MANUAL_CLUTCH).c_str());
                break;
            case SimGearboxMode::MANUAL_STICK:
                break;
            case SimGearboxMode::MANUAL_RANGES:
                break;
            }
        }
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawEngineButton(RoR::GfxActor* actorx)
{
    if (!actorx->GetActor()->ar_engine)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->ar_engine->isRunning())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_engine_icon->getHandle()), ImVec2(24, 24)))
    {
        if (actorx->GetActor()->ar_engine && actorx->GetActor()->ar_engine->isRunning())
        {
            actorx->GetActor()->ar_engine->toggleContact();
        }
        else if (actorx->GetActor()->ar_engine)
        {
            actorx->GetActor()->ar_engine->StartEngine();
        }
    }

    if (!actorx->GetActor()->ar_engine)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        if (actorx->GetActor()->ar_engine && !actorx->GetActor()->ar_engine->isRunning())
        {
            ImGui::TextDisabled("%s (%s + %s)", "Start Engine", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TOGGLE_CONTACT).c_str(), App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_STARTER).c_str());
        }
        else
        {
            ImGui::TextDisabled("%s (%s)", "Stop Engine", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TOGGLE_CONTACT).c_str());
        }
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawCustomLightButton(RoR::GfxActor* actorx)
{
    int num_custom_flares = 0;

    for (int i = 0; i < MAX_CLIGHTS; i++)
    {
        if (actorx->GetActor()->countCustomLights(i) > 0)
        {
            ImGui::PushID(i);
            num_custom_flares++;

            if (i == 5 || i == 9) // Add new line every 4 buttons
            {
                ImGui::NewLine();
            }

            std::string label = "L" + std::to_string(i + 1);

            if (actorx->GetActor()->getCustomLightVisible(i))
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
            }

            if (ImGui::Button(label.c_str(), ImVec2(32, 0)))
            {
                actorx->GetActor()->setCustomLightVisible(i, !actorx->GetActor()->getCustomLightVisible(i));
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextDisabled("%s %d (%s)", "Custom Light", i + 1, App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_LIGHTTOGGLE01 + i).c_str());
                ImGui::EndTooltip();
            }
            ImGui::SameLine();

            ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }
    if (num_custom_flares > 0)
    {
        ImGui::NewLine();
    }
}

void VehicleButtons::DrawCommandButton(RoR::GfxActor* actorx)
{
    m_id.resize(MAX_COMMANDS);

    for (int i = 1; i < MAX_COMMANDS; i++)
    {
        if (actorx->GetActor()->ar_command_key[i].description == "hide")
            continue;
        if (actorx->GetActor()->ar_command_key[i].beams.empty() && actorx->GetActor()->ar_command_key[i].rotators.empty())
            continue;

        ImGui::PushID(i);

        std::string label = "C" + std::to_string(i);
        int eventID = RoR::InputEngine::resolveEventName(fmt::format("COMMANDS_{:02d}", i));

        if (actorx->GetActor()->ar_command_key[i].playerInputValue != 0.0f)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
        }

        // Triggering continuous command every frame is sloppy
        // Set state and read it in GameContext via GetCommandEventID()
        ImGui::Button(label.c_str(), ImVec2(32, 0));
        if (ImGui::IsItemActive())
        {
            m_id[i] = eventID;
        }
        else
        {
            m_id[i] = -1;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            std::string desc = "unknown function";

            if (!actorx->GetActor()->ar_command_key[i].description.empty())
            {
                desc = actorx->GetActor()->ar_command_key[i].description.c_str();
            }

            ImGui::TextDisabled("%s (%s)", desc.c_str(), App::GetInputEngine()->getEventCommandTrimmed(eventID).c_str());
            ImGui::EndTooltip();
        }
        ImGui::SameLine();

        if (ImGui::GetCursorPos().x > 150) // Add new line approximately every 4 buttons
        {
            ImGui::NewLine();
        }

        ImGui::PopStyleColor();
        ImGui::PopID();
    }
}

void VehicleButtons::DrawCameraButton()
{
    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_camera_icon->getHandle()), ImVec2(24, 24)))
    {
        if (App::GetCameraManager()->EvaluateSwitchBehavior())
        {
            App::GetCameraManager()->switchToNextBehavior();
        }
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Switch Camera", App::GetInputEngine()->getEventCommandTrimmed(EV_CAMERA_CHANGE).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawLockButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->ar_hooks.empty())
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->isLocked())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_lock_icon->getHandle()), ImVec2(24, 24)))
    {
        //actorx->GetActor()->hookToggle(-1, HOOK_TOGGLE, -1);
        ActorLinkingRequest* hook_rq = new ActorLinkingRequest();
        hook_rq->alr_type = ActorLinkingRequestType::HOOK_ACTION;
        hook_rq->alr_actor_instance_id = actorx->GetActor()->ar_instance_id;
        hook_rq->alr_hook_action = HOOK_TOGGLE;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_ACTOR_LINKING_REQUESTED, hook_rq));

        //actorx->GetActor()->toggleSlideNodeLock();
        ActorLinkingRequest* slidenode_rq = new ActorLinkingRequest();
        slidenode_rq->alr_type = ActorLinkingRequestType::SLIDENODE_ACTION;
        hook_rq->alr_actor_instance_id = actorx->GetActor()->ar_instance_id;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_ACTOR_LINKING_REQUESTED, slidenode_rq));
    }

    if (actorx->GetActor()->ar_hooks.empty())
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Lock", App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_LOCK).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawSecureButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->ar_ties.empty())
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->isTied())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_secure_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->ar_toggle_ties = true;
    }

    if (actorx->GetActor()->ar_ties.empty())
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Secure", App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_SECURE_LOAD).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::DrawCruiseControlButton(RoR::GfxActor* actorx)
{
    if (!actorx->GetActor()->ar_engine)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    else if (actorx->GetActor()->cc_mode)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_cruise_control_icon->getHandle()), ImVec2(24, 24)))
    {
        actorx->GetActor()->cruisecontrolToggle();
    }

    if (!actorx->GetActor()->ar_engine)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("%s (%s)", "Cruise Control", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_CRUISE_CONTROL).c_str());
        ImGui::EndTooltip();
    }
}

void VehicleButtons::CacheIcons()
{
    // Icons used https://materialdesignicons.com/
    // Licence https://github.com/Templarian/MaterialDesign/blob/master/LICENSE

    m_headlight_icon = FetchIcon("car-light-high.png");
    m_left_blinker_icon = FetchIcon("arrow-left-bold.png");
    m_right_blinker_icon = FetchIcon("arrow-right-bold.png");
    m_warning_light_icon = FetchIcon("hazard-lights.png");
    m_horn_icon = FetchIcon("bugle.png");
    m_mirror_icon = FetchIcon("mirror-rectangle.png");
    m_repair_icon = FetchIcon("car-wrench.png");
    m_parking_brake_icon = FetchIcon("car-brake-alert.png");
    m_traction_control_icon = FetchIcon("car-traction-control.png");
    m_abs_icon = FetchIcon("car-brake-abs.png");
    m_physics_icon = FetchIcon("pause.png");
    m_actor_physics_icon = FetchIcon("pause-circle-outline.png");
    m_a_icon = FetchIcon("alpha-a-circle.png");
    m_w_icon = FetchIcon("alpha-w-circle.png");
    m_m_icon = FetchIcon("alpha-m-circle.png");
    m_g_icon = FetchIcon("alpha-g-circle.png");
    m_particle_icon = FetchIcon("water.png");
    m_shift_icon = FetchIcon("car-shift-pattern.png");
    m_engine_icon = FetchIcon("engine.png");
    m_beacons_icon = FetchIcon("alarm-light-outline.png");
    m_camera_icon = FetchIcon("camera-switch-outline.png");
    m_lock_icon = FetchIcon("alpha-l-circle.png");
    m_secure_icon = FetchIcon("lock-outline.png");
    m_cruise_control_icon = FetchIcon("car-cruise-control.png");

    m_icons_cached = true;
}