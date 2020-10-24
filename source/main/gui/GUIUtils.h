/*
    This source file is part of Rigs of Rods
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

#pragma once

#include "Application.h"
#include "GUIManager.h"

namespace RoR {

/// Draws animated loading spinner
void DrawImGuiSpinner(
    float& counter, const ImVec2 size = ImVec2(16.f, 16.f),
    const float spacing = 2.f, const float step_sec = 0.15f);

/// Add rotated textured quad to ImDrawList, source: https://github.com/ocornut/imgui/issues/1982#issuecomment-408834301
void DrawImageRotated(ImTextureID tex_id, ImVec2 center, ImVec2 size, float angle);

/// Draw multiline text with '#rrggbb' color markers. Returns total text size.
ImVec2 DrawColorMarkedText(ImDrawList* drawlist, ImVec2 text_cursor, ImVec4 default_color, float override_alpha, std::string const& line);

void DrawGCheckbox(CVar* cvar, const char* label);

void DrawGIntCheck(CVar* cvar, const char* label);

void DrawGIntBox(CVar* cvar, const char* label);

void DrawGIntSlider(CVar* cvar, const char* label, int v_min, int v_max);

void DrawGFloatSlider(CVar* cvar, const char* label, float v_min, float v_max);

void DrawGFloatBox(CVar* cvar, const char* label);

void DrawGTextEdit(CVar* cvar, const char* label, Str<1000>& buf);

void DrawGCombo(CVar* cvar, const char* label, const char* values);

} // namespace RoR
