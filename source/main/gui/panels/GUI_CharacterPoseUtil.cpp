/*
    This source file is part of Rigs of Rods
    Copyright 2016-2020 Petr Ohlidal

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


#include "GUI_CharacterPoseUtil.h"

#include "Application.h"
#include "Actor.h"
#include "GameContext.h"
#include "GfxCharacter.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "Language.h"

#include <Ogre.h>

using namespace RoR;
using namespace GUI;
using namespace Ogre;

void CharacterPoseUtil::Draw()
{
    Character* character = App::GetGameContext()->GetPlayerCharacter();
    if (!character)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            "no player character, closing CharacterPoseUtil");
        this->SetVisible(false);
        return;
    }

    GfxCharacter* gfx_character = nullptr;
    for (GfxCharacter* xchar : App::GetGfxScene()->GetGfxCharacters())
    {
        if (xchar->xc_character == character)
            gfx_character = xchar;
    }
    if (!gfx_character)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            "no player character visuals, closing CharacterPoseUtil");
        this->SetVisible(false);
        return;
    }

    Entity* ent = static_cast<Entity*>(gfx_character->xc_scenenode->getAttachedObject(0));

    const int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    bool keep_open = true;
    ImGui::Begin(_LC("CharacterPoseUtil", "Character pose utility"), &keep_open, flags);

    ImGui::Dummy(ImVec2(250, 1)); // force minimum width

    ImGui::Text("Character: '%s'", gfx_character->xc_instance_name.c_str());
    ImGui::TextDisabled("(gray text means 'disabled')");
    ImGui::Separator();

    AnimationStateSet* stateset = ent->getAllAnimationStates();
    for (auto& state_pair : stateset->getAnimationStates())
    {
        AnimationState* as = state_pair.second;
        ImVec4 color = (as->getEnabled()) ? ImGui::GetStyle().Colors[ImGuiCol_Text] : ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
        ImGui::TextColored(color, "'%s' (%.2f sec)", as->getAnimationName().c_str(), as->getLength());
        std::string caption = fmt::format("{:.2f} sec", as->getTimePosition());
        ImGui::ProgressBar(as->getTimePosition() / as->getLength(), ImVec2(-1, 0), caption.c_str());
    }

    // Common window epilogue:

    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);

    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void CharacterPoseUtil::SetVisible(bool v)
{
    m_is_visible = v;
    m_is_hovered = false;
}
