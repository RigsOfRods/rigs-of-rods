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

struct ImTextFeeder /// Helper for drawing multiline wrapped & colored text.
{
    ImTextFeeder(ImDrawList* _drawlist, ImVec2 _origin);

    /// No wrapping or trimming
    void AddInline(ImU32 color, ImVec2 text_size, const char* text, const char* text_end);
    /// Wraps entire input. Trims leading blanks on extra lines. `wrap_width=-1.f` disables wrapping.
    void AddWrapped(ImU32 color, float wrap_width, const char* text, const char* text_end);
    /// Wraps substrings separated by blanks. `wrap_width=-1.f` disables wrapping.
    void AddMultiline(ImU32 color, float wrap_width, const char* text, const char* text_end);
    void NextLine();

    ImDrawList* drawlist = nullptr;
    ImVec2 cursor; //!< Next draw position, screen space
    ImVec2 origin; //!< First draw position, screen space
    ImVec2 size = ImVec2(0,0); //!< Accumulated text size
    float line_height = 0.f;
};

/// Draws animated loading spinner
void LoadingIndicatorCircle(const char* label, const float indicator_radius, const ImVec4& main_color, const ImVec4& backdrop_color, const int circle_count, const float speed);

/// Add rotated textured quad to ImDrawList, source: https://github.com/ocornut/imgui/issues/1982#issuecomment-408834301
void DrawImageRotated(ImTextureID tex_id, ImVec2 center, ImVec2 size, float angle);

/// Draw multiline text with '#rrggbb' color markers. Returns total text size.
ImVec2 DrawColorMarkedText(ImDrawList* drawlist, ImVec2 text_cursor, ImVec4 default_color, float override_alpha, float wrap_width, std::string const& line);

std::string StripColorMarksFromText(std::string const& text);

/// Prints multiline text with '#rrggbb' color markers. Behaves like `ImGui::Text*` functions.
void ImTextWrappedColorMarked(std::string const& text);

bool DrawGCheckbox(CVar* cvar, const char* label);

void DrawGIntCheck(CVar* cvar, const char* label);

void DrawGIntBox(CVar* cvar, const char* label);

void DrawGIntSlider(CVar* cvar, const char* label, int v_min, int v_max);

void DrawGFloatSlider(CVar* cvar, const char* label, float v_min, float v_max);

void DrawGFloatBox(CVar* cvar, const char* label);

void DrawGTextEdit(CVar* cvar, const char* label, Str<1000>& buf);

bool DrawGCombo(CVar* cvar, const char* label, const char* values);

Ogre::TexturePtr FetchIcon(const char* name);

// Fullscreen drawing helpers
ImDrawList* GetImDummyFullscreenWindow(const std::string& name = "RoR_TransparentFullscreenWindow");
bool GetScreenPosFromWorldPos(Ogre::Vector3 const& world_pos, ImVec2& out_screen);

// Helpers for coposing combobox item strings.
void ImAddItemToComboboxString(std::string& target, std::string const& item);
void ImTerminateComboboxString(std::string& target);

// Input engine helpers
void ImDrawEventHighlighted(events input_event);
bool ImDrawEventHighlightedButton(events input_event, bool* btn_hovered = nullptr, bool* btn_active = nullptr);
void ImDrawModifierKeyHighlighted(OIS::KeyCode key);
ImVec2 ImCalcEventHighlightedSize(events input_event);

// Draws button which must be held for a period to report "clicked" - shows a tooltip with countdown progressbar.
bool ImButtonHoldToConfirm(const std::string& btn_idstr, const bool smallbutton, const float time_limit);

// Returns true if succeeded (needs the text box to have focus).
bool ImMoveTextInputCursorToEnd(const char* label);

/// Looks and behaves (mouuse cursor) like a hypertext, but doesn't open URL.
void ImDummyHyperlink(std::string caption);
void ImHyperlink(std::string url, std::string caption = "", bool tooltip = true);

} // namespace RoR
