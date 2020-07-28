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
#include "OgreImGui.h"

namespace RoR {

/// @author http://www.ogre3d.org/forums/viewtopic.php?p=463232#p463232
/// @author http://www.ogre3d.org/tikiwiki/tiki-index.php?page=GetScreenspaceCoords&structure=Cookbook
class World2ScreenConverter //!< Keeps data close for faster access.
{
public:
    World2ScreenConverter(Ogre::Matrix4 view_mtx, Ogre::Matrix4 proj_mtx, Ogre::Vector2 screen_size):
        m_view_matrix(view_mtx), m_projection_matrix(proj_mtx), m_screen_size(screen_size)
    {}

    World2ScreenConverter(); //!< Gets data from CameraManager and DearIMGUI.

    /// @return X,Y=screen pos, Z=view space pos ('Z<0' means 'in front of the camera')
    Ogre::Vector3 Convert(Ogre::Vector3 world_pos)
    {
        Ogre::Vector3 view_space_pos = m_view_matrix * world_pos;
        Ogre::Vector3 clip_space_pos = m_projection_matrix * view_space_pos; // Clip space pos is [-1.f, 1.f]
        float screen_x = ((clip_space_pos.x / 2.f) + 0.5f) * m_screen_size.x;
        float screen_y = (1.f - ((clip_space_pos.y / 2.f) + 0.5f)) * m_screen_size.y;
        return Ogre::Vector3(screen_x, screen_y, view_space_pos.z);
    }

private:
    Ogre::Matrix4 m_view_matrix;
    Ogre::Matrix4 m_projection_matrix;
    Ogre::Vector2 m_screen_size;
};

/// Draws animated loading spinner
void DrawImGuiSpinner(
    float& counter, const ImVec2 size = ImVec2(16.f, 16.f),
    const float spacing = 2.f, const float step_sec = 0.15f);

/// Add rotated textured quad to ImDrawList, source: https://github.com/ocornut/imgui/issues/1982#issuecomment-408834301
void DrawImageRotated(ImTextureID tex_id, ImVec2 center, ImVec2 size, float angle);

void DrawGCheckbox(CVar* cvar, const char* label);

void DrawGIntCheck(CVar* cvar, const char* label);

void DrawGIntBox(CVar* cvar, const char* label);

void DrawGIntSlider(CVar* cvar, const char* label, int v_min, int v_max);

void DrawGFloatSlider(CVar* cvar, const char* label, float v_min, float v_max);

void DrawGFloatBox(CVar* cvar, const char* label);

void DrawGTextEdit(CVar* cvar, const char* label, Str<1000>& buf);

void DrawGCombo(CVar* cvar, const char* label, const char* values);

ImDrawList* ObtainImGuiDrawList(const char* window_name);

} // namespace RoR
