/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

/// @file
/// @author Moncef Ben Slimane
/// @date   11/2014

#include "GUI_VehicleDescription.h"

#include "Application.h"
#include "Actor.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "Language.h"

#include <fmt/format.h>
#include <imgui.h>

using namespace RoR;
using namespace GUI;

void VehicleDescription::Draw()
{
    Actor* actor = App::GetGameContext()->GetPlayerActor();
    if (!actor)
    {
        m_is_visible = false;
        return;
    }

    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);
    ImVec2 size(HELP_TEXTURE_WIDTH + 2*ImGui::GetStyle().WindowPadding.x, 0.f);
    ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);
    if (!ImGui::Begin(actor->getTruckName().c_str(), &m_is_visible))
    {
        ImGui::End(); // The window is collapsed
        return;
    }

    RoR::GfxActor* gfx_actor = actor->GetGfxActor();
    if (gfx_actor->GetHelpTex())
    {
        ImTextureID im_tex = reinterpret_cast<ImTextureID>(gfx_actor->GetHelpTex()->getHandle());
        ImGui::Image(im_tex, ImVec2(HELP_TEXTURE_WIDTH, HELP_TEXTURE_HEIGHT));
    }

    if (ImGui::CollapsingHeader(_LC("VehicleDescription", "Description"), ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (auto line : actor->getDescription())
        {
            ImGui::TextWrapped("%s", line.c_str());
        }
    }

    if (ImGui::CollapsingHeader(_LC("VehicleDescription", "Commands"), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2, /*id=*/nullptr, /*border=*/true);
        for (int i = 1; i < MAX_COMMANDS; i += 2)
        {
            if (actor->ar_command_key[i].description == "hide")
                continue;
            if (actor->ar_command_key[i].beams.empty() && actor->ar_command_key[i].rotators.empty())
                continue;

            int eventID = RoR::InputEngine::resolveEventName(fmt::format("COMMANDS_{:02d}", i));
            Ogre::String keya = RoR::App::GetInputEngine()->getEventCommand(eventID);
            eventID = RoR::InputEngine::resolveEventName(fmt::format("COMMANDS_{:02d}", i + 1));
            Ogre::String keyb = RoR::App::GetInputEngine()->getEventCommand(eventID);

            // cut off expl
            if (keya.size() > 6 && keya.substr(0, 5) == "EXPL+")
                keya = keya.substr(5);
            if (keyb.size() > 6 && keyb.substr(0, 5) == "EXPL+")
                keyb = keyb.substr(5);

            ImGui::Text("%s/%s", keya.c_str(), keyb.c_str());
            ImGui::NextColumn();

            if (!actor->ar_command_key[i].description.empty())
            {
                ImGui::Text("%s", actor->ar_command_key[i].description.c_str());
            }
            else
            {
                ImGui::TextDisabled("%s", _LC("VehicleDescription", "unknown function"));
            }
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader(_LC("VehicleDescription", "Authors")))
    {
        for (authorinfo_t const& author: actor->getAuthors())
        {
            ImGui::Text("%s: %s", author.type.c_str(), author.name.c_str());
        }

        if (actor->getAuthors().empty())
        {
            ImGui::TextDisabled("%s", _LC("VehicleDescription", "(no author information available) "));
        }
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
}
