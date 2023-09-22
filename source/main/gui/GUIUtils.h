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

// DearIMGUI math utils, copypasted from <imgui_internal.h>
static inline ImVec2 operator*(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }
static inline ImVec2 operator/(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y); }
static inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline ImVec2& operator*=(ImVec2& lhs, const float rhs) { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static inline ImVec2& operator/=(ImVec2& lhs, const float rhs) { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a) { return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a); }
template<typename T> static inline T ImMin(T lhs, T rhs) { return lhs < rhs ? lhs : rhs; }
#define IM_PI 3.14159265358979323846f

namespace RoR {

struct ImTextFeeder /// Helper for drawing multiline wrapped & colored text.
{
    ImTextFeeder(ImDrawList* _drawlist, ImVec2 _origin): drawlist(_drawlist), origin(_origin), cursor(_origin) {}

    /// No wrapping or trimming
    void AddInline(ImU32 color, ImVec2 text_size, const char* text, const char* text_end);
    /// Wraps entire input. Trims leading blanks on extra lines. `wrap_width=-1.f` disables wrapping.
    void AddWrapped(ImU32 color, float wrap_width, const char* text, const char* text_end);
    /// Wraps substrings separated by blanks. `wrap_width=-1.f` disables wrapping.
    void AddMultiline(ImU32 color, float wrap_width, const char* text, const char* text_end);
    void NextLine();

    ImDrawList* drawlist;
    ImVec2 cursor; //!< Next draw position, screen space
    ImVec2 origin; //!< First draw position, screen space
    ImVec2 size = ImVec2(0,0); //!< Accumulated text size
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

void DrawGCheckbox(CVar* cvar, const char* label);

void DrawGIntCheck(CVar* cvar, const char* label);

void DrawGIntBox(CVar* cvar, const char* label);

void DrawGIntSlider(CVar* cvar, const char* label, int v_min, int v_max);

void DrawGFloatSlider(CVar* cvar, const char* label, float v_min, float v_max);

void DrawGFloatBox(CVar* cvar, const char* label);

void DrawGTextEdit(CVar* cvar, const char* label, Str<1000>& buf);

void DrawGCombo(CVar* cvar, const char* label, const char* values);

Ogre::TexturePtr FetchIcon(const char* name);

ImDrawList* GetImDummyFullscreenWindow();

// Helpers for coposing combobox item strings.
void ImAddItemToComboboxString(std::string& target, std::string const& item);
void ImTerminateComboboxString(std::string& target);

// Substitute for `ImSetNextWindowPosCenter()` OBSOLETED in 1.52 (between Aug 2017 and Oct 2017)
void ImSetNextWindowPosCenter(ImGuiCond c = 0);

// Input engine helpers
void ImDrawEventHighlighted(events input_event);
void ImDrawModifierKeyHighlighted(OgreBites::Keycode key);

} // namespace RoR
