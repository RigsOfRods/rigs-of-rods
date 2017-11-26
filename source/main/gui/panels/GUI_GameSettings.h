/*
    This source file is part of Rigs of Rods
    Copyright 2016-2017 Petr Ohlidal & contributors

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

#pragma once

#include "Application.h"
#include "OgreImGui.h"

namespace RoR {
namespace GUI {

class GameSettings
{
public:
    enum SettingsTab { GENERAL, CONTROL, VIDEO, DIAG };

    GameSettings(): m_is_visible(false), m_tab(SettingsTab::GENERAL) {}

    void Draw();

    inline bool IsVisible() const { return m_is_visible; }
    inline void SetVisible(bool v)
    {
        if (!v)
        {
            m_tab = SettingsTab::GENERAL;
        }
        m_is_visible = v;
    }

    inline void DrawGCheckbox(GVarPod<bool>& gvar, const char* label)
    {
        bool val = gvar.GetActive();
        if (ImGui::Checkbox(label, &val))
        {
            gvar.SetActive(val);
        }
    }

    inline void DrawGIntCheck(GVarPod<int>& gvar, const char* label)
    {
        bool val = (gvar.GetActive() != 0);
        if (ImGui::Checkbox(label, &val))
        {
            gvar.SetActive(val ? 1 : 0);
        }
    }

    template <size_t GVarLen>
    inline void DrawGTextEdit(GVarStr<GVarLen>& gvar, const char* label, Str<GVarLen>& buf)
    {
        if (ImGui::InputText(label, buf.GetBuffer(), buf.GetCapacity(), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            gvar.SetActive(buf.GetBuffer());
        }
        if (ImGui::IsItemActive())
        {
            ImGui::TextDisabled("(hit Enter key to submit)");
        }
        else
        {
            buf.Assign(gvar.GetActive());
        }
    }

    template <typename GEnum_T>
    inline void DrawGCombo(GVarEnum<GEnum_T>& gvar, const char* label, const char* values)
    {
        int selection = static_cast<int>(gvar.GetActive());
        if (ImGui::Combo(label, &selection, values))
        {
            gvar.SetActive(static_cast<GEnum_T>(selection));
        }
    }

    inline void DrawGIntBox(GVarPod<int>& gvar, const char* label)
    {
        int val = gvar.GetActive();
        if (ImGui::InputInt(label, &val, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            gvar.SetActive(val);
        }
    }

    inline void DrawGFloatBox(GVarPod<float>& gvar, const char* label)
    {
        float fval = gvar.GetActive();
        if (ImGui::InputFloat(label, &fval, 0.f, 0.f, -1, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            gvar.SetActive(fval);
        }
    }

private:
    bool m_is_visible;
    SettingsTab m_tab;

    Str<100> m_buf_diag_preset_terrain;
    Str<100> m_buf_diag_preset_vehicle;
    Str<100> m_buf_diag_preset_veh_config;
    Str<300> m_buf_diag_extra_resource_dir;
    Str<50>  m_buf_io_outgauge_ip;
};

} // namespace GUI
} // namespace RoR
