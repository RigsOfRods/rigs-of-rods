/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   7th of September 2009

// Remade using DearIMGUI by Petr Ohlidal, 10/2019


#include "GUI_FrictionSettings.h"

#include "Application.h"
#include "SimData.h"
#include "Collisions.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "Language.h"
#include "Terrain.h"
#include "Utils.h"

using namespace RoR;
using namespace GUI;

void FrictionSettings::Draw()
{
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse;
    bool keep_open = true;
    ImGui::Begin(_LC("FrictionSettings", "Friction Settings"), &keep_open, win_flags);

    ImGui::Text("%s", _LC("FrictionSettings", "Current active Ground: "));
    ImGui::SameLine();
    ImGui::Text("%s", (m_nearest_gm != nullptr) ? m_nearest_gm->name : "~");

    ImGui::Separator();
    ImGui::PushItemWidth(200.f);

    ImGui::Combo(_LC("FrictionSettings", "selected Ground Type:"), &m_selected_gm,
                 &FrictionSettings::GmComboItemGetter, &m_gm_entries,
                 static_cast<int>(m_gm_entries.size()));
    ground_model_t& gm = m_gm_entries[m_selected_gm].working_copy;
    bool dirty = false;

    ImGui::TextDisabled("%s", _LC("FrictionSettings", "Solid ground settings"));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Solid ground level:"), &gm.solid_ground_level, 0.f, 200.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Solid ground level"), _LC("FrictionSettings", "With this you can define how deep the solid ground is. If it is 0 then the surface will be solid. If it is 0.1 then you'll have 10 cm of fluid on top of solid ground. If it is 100 then the solid ground will be way deep (100m), with fluid on top."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Strength:"), &gm.strength, 0.f, 2.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Strength"), _LC("FrictionSettings", "This parameter raises or diminishes surface friction in a generic way. It is here so as to be able to do quick calibrations of friction. Start with having this to 1.0 and after tuning the rest of the surface variables, come back and play with this."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Static friction coef:"), &gm.ms, 0.1f, 2.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Static friction coef"), _LC("FrictionSettings", "Static friction keeps you in the same place when you are stopped on a hill. In the real world this friction is always bigger than dynamic friction (sliding friction). Start with 0.5 and work from there. It is better to try to find some experimentally validated values for this and the rest of surface friction variables in the net, and then to fine tune via strength."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Adhesion velocity:"), &gm.va, 0.1f, 5.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Adhesion velocity"), _LC("FrictionSettings", "Below this value static friction laws apply, above it dynamic friction laws apply. Value should be small, in the range of 0.1-0.4 . This velocity threshold is also used by fluid dynamics so you should always define it. NEVER set it to 0."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Dynamic friction coef:"), &gm.mc, 0.1f, 1.5f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Dynamic friction coef"), _LC("FrictionSettings", "Or sliding friction coef. It should be smaller than static friction coef. This parameter defines how much friction you'll have when sliding. Try to find some values for it from the net."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Hydrodynamic friction coef:"), &gm.t2, 0.f, 1.5f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Hydrodynamic friction coef"), _LC("FrictionSettings", "This friction defines the added friction that you'll feel from a surface that has a little film of fluid on it. It is kind of redundant with all the fluid physics below, but it is here so as for experimentally validated values from the net to be usable. If you decide that you'll simulate the film of fluid with the more complex fluid physics below, then just set this to 0."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Stribeck velocity:"), &gm.vs, 0.f, 1000.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Stribeck velocity"), _LC("FrictionSettings", "You'll either find stribeck velocity in the net, or the inverse (1/stribeck velocity) of it described as 'stribeck coef'. It defines the shape of the dynamic friction curve. Lets leave it at that. Just find some nice values for it from the net."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "alpha:"), &gm.alpha, 0.f, 200.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Alpha"), _LC("FrictionSettings", "Its usual value is 2. But you can try others."));

    ImGui::TextDisabled("%s", _LC("FrictionSettings", "Fluid Settings"));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Flow behavior index:"), &gm.flow_behavior_index, -2.f, 2.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Flow behavior index"), _LC("FrictionSettings", "If it is 1.0 then the fluid will behave like water. The lower you get from 1.0, the more like mud the fluid will behave, meaning that for small velocities the fluid will resist motion and for large velocities the fluid will not resist so much. The higher you get from 1.0 the more like sand the fluid will behave. The bigger the velocity, the bigger the resistance of the fluid (try to hit sand hard it'll feel like stone)."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Flow consistency:"), &gm.flow_consistency_index, 10.f, 100000.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Flow consistency"), _LC("FrictionSettings", "Think of it as default fluid resistance. Behavior index above changes it at real time. Useful values in practice are quite large."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Fluid density:"), &gm.fluid_density, 10.f, 100000.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Fluid density"), _LC("FrictionSettings", "In mud (or sand) the resistance of the fluid described by the parameters above will stop you and so keep you from sinking. But for substances like water it isn't the drag that stops you from sinking. Its buoyancy. This parameter is here so as to keep you from sinking when you wish to simulate fluids with low drag (resistance). For fluids like mud or sand you can put it at 0, but it is best to keep it at some minimum value. For fluids with behavior index >=1 it will behave like you are in water. For fluids with behavior index <1 it'll behave like you are in mud."));

    if (ImGui::SliderFloat(_LC("FrictionSettings", "Drag anisotropy:"), &gm.drag_anisotropy, 0.f, 1.f)) { dirty = true; }
    this->DrawTooltip(_LC("FrictionSettings", "Drag anisotropy"), _LC("FrictionSettings", "This parameter is for making it easier(cheating) to get out from mud. To get stuck in real mud isn't fun at all, so this makes the mud push up. Ranges in this parameter are from 0 to 1 . If you set it at 1 then you'll get real mud. For values from 0 to 1, the behavior goes from real mud to easy mud depending on this parameter and the value of Adhesion velocity. For velocity 0 real mud it is. For velocity >= adhesion velocity easy mud it is."));

    //TODO: _LC("FrictionSettings", "fx_type:")
    // _LC("FrictionSettings", "FX Type"), _LC("FrictionSettings", "The type of special effects that RoR will use to give the appearance of a surface. It doesn't affect the physics at all")

    // TODO _LC("FrictionSettings", "fx_color:")
    // _LC("FrictionSettings", "FX Colour"), _LC("FrictionSettings", "The color of RoR's special effects")

    // TODO ... all other fx

    //Unused: _LC("FrictionSettings", "Friction Help")

    if (dirty)
    {
        App::GetGameContext()->PushMessage(Message(MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED, (void*)&gm));
    }

    ImGui::PopItemWidth();

    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);

    ImGui::End();

    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

// Static helper
bool FrictionSettings::GmComboItemGetter(void* data, int idx, const char** out_text)
{
    auto items = static_cast<std::vector<FrictionSettings::Entry>*>(data);
    if (out_text)
        *out_text = (*items)[idx].live_data->name;
    return true;
}

void FrictionSettings::AnalyzeTerrain()
{
    m_gm_entries.clear();
    auto itor = App::GetGameContext()->GetTerrain()->GetCollisions()->getGroundModels()->begin();
    auto endi = App::GetGameContext()->GetTerrain()->GetCollisions()->getGroundModels()->end();
    for (; itor != endi; ++itor)
    {
        m_gm_entries.emplace_back(&itor->second);
    }
}

void FrictionSettings::DrawTooltip(const char* title, const char* text)
{
    ImGui::SameLine();
    ImGui::TextDisabled("[?]");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetNextWindowSizeConstraints(/*size_min=*/ImVec2(250.f, 50.f), /*size_max=*/ImVec2(1000.f, 1000.f));
        ImGui::BeginTooltip();
        ImGui::Text("%s", title);
        ImGui::Separator();
        ImGui::TextWrapped("%s", text);
        ImGui::EndTooltip();
    }
}

