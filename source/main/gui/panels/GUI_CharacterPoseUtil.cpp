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

    const int flags = ImGuiWindowFlags_NoCollapse;
    bool keep_open = true;
    ImGui::Begin(_LC("CharacterPoseUtil", "Character pose utility"), &keep_open, flags);

    ImGui::Text("Character: '%s' (mesh: '%s')", 
        gfx_character->xc_instance_name.c_str(), 
        gfx_character->xc_character->getCharacterDef()->mesh_name.c_str());

    ImGui::BeginTabBar("CharacterPoseUtilTabs");

    if (ImGui::BeginTabItem("Skeletal anims"))
    {
        this->DrawSkeletalPanel(ent);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Game anims"))
    {
        this->DrawAnimDbgPanel();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();

    // Common window epilogue:

    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);

    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void CharacterPoseUtil::DrawSkeletalPanel(Ogre::Entity* ent)
{
    ImGui::Checkbox("Manual pose mode", &m_manual_pose_active);
    if (!m_manual_pose_active)
    {
        ImGui::TextDisabled("(gray text means 'disabled')");
    }
    ImGui::Dummy(ImVec2(350, 1)); // force minimum width
    ImGui::Separator();

    AnimationStateSet* stateset = ent->getAllAnimationStates();
    for (auto& state_pair : stateset->getAnimationStates())
    {
        AnimationState* as = state_pair.second;
        this->DrawAnimControls(as);

    }
}

void CharacterPoseUtil::DrawAnimControls(Ogre::AnimationState* anim_state)
{
    ImGui::PushID(anim_state);

    // anim name line
    ImVec4 color = (anim_state->getEnabled()) ? ImGui::GetStyle().Colors[ImGuiCol_Text] : ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
    ImGui::TextColored(color, "'%s' (%.2f sec)", anim_state->getAnimationName().c_str(), anim_state->getLength());
    if (m_manual_pose_active)
    {
        ImGui::SameLine();
        bool enabled = anim_state->getEnabled();
        if (ImGui::Checkbox("Enabled", &enabled))
        {
            anim_state->setEnabled(enabled);
            anim_state->setWeight(enabled ? 1.f : 0.f);
        }
        ImGui::SameLine();
        float weight = anim_state->getWeight();
        ImGui::SetNextItemWidth(50.f);
        if (ImGui::InputFloat("Weight", &weight))
        {
            anim_state->setWeight(weight);
        }
    }

    // anim progress line
    if (m_manual_pose_active)
    {
        float timepos = anim_state->getTimePosition();
        if (ImGui::SliderFloat("Time pos", &timepos, 0.f, anim_state->getLength()))
        {
            anim_state->setTimePosition(timepos);
        }
    }
    else
    {
        std::string caption = fmt::format("{:.2f} sec", anim_state->getTimePosition());
        ImGui::ProgressBar(anim_state->getTimePosition() / anim_state->getLength(), ImVec2(-1, 0), caption.c_str());
    }

    ImGui::PopID(); // AnimationState*
}

ImVec4 ForFlagColor(BitMask_t flags, BitMask_t mask, bool active)
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();
    ImVec4 normal_text_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
    return (active)
        ? theme.success_text_color
        : ((BITMASK_IS_1(flags, mask)) ? normal_text_color : theme.warning_text_color);
}

ImVec4 ExceptFlagColor(BitMask_t flags, BitMask_t mask, bool active)
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();
    ImVec4 normal_text_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
    return (active)
        ? theme.value_blue_text_color
        : ((BITMASK_IS_1(flags, mask)) ? theme.error_text_color : normal_text_color);
}

void CharacterPoseUtil::DrawAnimDbgItem(int id)
{
    CharacterAnimDbg const& dbg = anim_dbg_states[id];
    CharacterAnimDef* def = App::GetGameContext()->GetPlayerCharacter()->getCharacterDef()->getAnimById(id);


    // Draw attributes
    ImGui::Text("%s", fmt::format("anim: '{}', continuous: {}, autorestart: {}, neutral_mid: {}",
        def->anim_name, def->anim_continuous, def->anim_autorestart, def->anim_neutral_mid).c_str());

    // Draw the 'for_' flags, the satisfied get colored yellow. If all are satisfied, all get colored green.
    ImGui::TextDisabled("For flags:");
    int num_flags = 0;
    const int MAX_FLAGS_PER_LINE = 3;
    for (int i = 1; i <= 32; i++)
    {
        BitMask_t testmask = BITMASK(i);
        if (BITMASK_IS_1(def->for_situations, testmask))
        {
            ImVec4 color = ForFlagColor(dbg.missing_situations, testmask, dbg.active);
            if (num_flags > 0 && num_flags % MAX_FLAGS_PER_LINE == 0)
            {
                ImGui::TextDisabled("    (more):");
            }
            ImGui::SameLine();
            ImGui::TextColored(color, "%s", Character::SituationFlagToString(testmask));
            num_flags++;
        }
        if (BITMASK_IS_1(def->for_actions, testmask))
        {
            ImVec4 color = ForFlagColor(dbg.missing_actions, testmask, dbg.active);
            if (num_flags > 0 && num_flags % MAX_FLAGS_PER_LINE == 0)
            {
                ImGui::TextDisabled("    (more):");
            }
            ImGui::SameLine();
            ImGui::TextColored(color, "%s", Character::ActionFlagToString(testmask));
            num_flags++;
        }
    }

    // Draw the 'except_' flags, blocking get colored red.
    ImGui::TextDisabled("Except flags:");
    num_flags = 0;
    for (int i = 1; i <= 32; i++)
    {
        BitMask_t testmask = BITMASK(i);
        if (BITMASK_IS_1(def->except_situations, testmask))
        {
            ImVec4 color = ExceptFlagColor(dbg.blocking_situations, testmask, dbg.active);
            if (num_flags > 0 && num_flags % MAX_FLAGS_PER_LINE == 0)
            {
                ImGui::TextDisabled("    (more):");
            }
            ImGui::SameLine();
            ImGui::TextColored(color, "%s", Character::SituationFlagToString(testmask));
            num_flags++;
        }
        if (BITMASK_IS_1(def->except_actions, testmask))
        {
            ImVec4 color = ExceptFlagColor(dbg.blocking_actions, testmask, dbg.active);
            if (num_flags > 0 && num_flags % MAX_FLAGS_PER_LINE == 0)
            {
                ImGui::TextDisabled("    (more):");
            }
            ImGui::SameLine();
            ImGui::TextColored(color, "%s", Character::ActionFlagToString(testmask));
            num_flags++;
        }
    }
}

void CharacterPoseUtil::DrawAnimDbgPanel()
{
    const float child_height = ImGui::GetWindowHeight()
        - ((2.f * ImGui::GetStyle().WindowPadding.y) + (3.f * ImGui::GetItemsLineHeightWithSpacing())
            + ImGui::GetStyle().ItemSpacing.y);

    ImGui::BeginChild("CharacterPoseUi-animDbg-scroll", ImVec2(0.f, child_height), false);

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Game animations"))
    {
        for (CharacterAnimDef const& anim : App::GetGameContext()->GetPlayerCharacter()->getCharacterDef()->anims)
        {
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode(&anim, "%s", anim.game_description.c_str()))
            {
                this->DrawAnimDbgItem(anim.game_id);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    ImGui::EndChild();
}

void CharacterPoseUtil::SetVisible(bool v)
{
    m_is_visible = v;
    m_is_hovered = false;
    if (!v)
    {
        m_manual_pose_active = false;
    }
}
