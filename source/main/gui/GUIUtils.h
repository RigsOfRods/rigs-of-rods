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
#include "GUIManager.h"

namespace RoR {

/// Draws animated loading spinner
void DrawImGuiSpinner(
    float& counter, const ImVec2 size = ImVec2(16.f, 16.f),
    const float spacing = 2.f, const float step_sec = 0.15f);

void DrawGCheckbox(GVar_A<bool>& gvar, const char* label);

void DrawGCheckbox(GVar_APS<bool>& gvar, const char* label);

void DrawGIntCheck(GVar_A<int>& gvar, const char* label);

void DrawGIntBox(GVar_A<int>& gvar, const char* label);

void DrawGFloatBox(GVar_A<float>& gvar, const char* label);

void DrawGFloatBox(GVar_APS<float>& gvar, const char* label);

template <typename GVar_T, size_t Len>
void DrawGTextEdit(GVar_T& gvar, const char* label, Str<Len>& buf)
{
    if (ImGui::InputText(label, buf.GetBuffer(), buf.GetCapacity(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        gvar.SetActive(buf.ToCStr());
    }
    if (ImGui::IsItemActive())
    {
        ImGui::TextDisabled("(hit Enter key to submit)");
    }
    else
    {
        buf.Assign(gvar.GetActive().c_str());
    }
}

template <typename GVar_T, size_t Len>
void DrawGTextEdit(GVar_T& gvar, const char* label, Str<Len>& buf, bool update_active)
{
    if (ImGui::InputText(label, buf.GetBuffer(), buf.GetCapacity(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (update_active)
        {
            gvar.SetActive(buf.ToCStr());
        }
        gvar.SetStored(buf.ToCStr());
    }
    if (ImGui::IsItemActive())
    {
        ImGui::TextDisabled("(hit Enter key to submit)");
    }
    else
    {
        buf.Assign(gvar.GetStored().c_str());
    }
}

template <typename GVar_T>
void DrawGCombo(GVar_T& gvar, const char* label, const char* values)
{
    int selection = static_cast<int>(gvar.GetActive());
    if (ImGui::Combo(label, &selection, values))
    {
        gvar.SetActive(static_cast<GVar_T::Value_t>(selection));
    }
}

} // namespace RoR