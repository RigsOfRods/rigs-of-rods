/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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
/// @author Petr Ohlidal
/// @date   05/2023

#pragma once

#include <imgui.h>

namespace RoR {
namespace GUI {

class ThemeEditor
{
public:


    void SetVisible(bool vis) { m_is_visible = vis; }
    bool IsVisible() const { return m_is_visible; }

    void Draw();

private:
    void EditColor(ImVec4& color, const char* desc);
    void EditFloat(float& val, const char* desc);
    bool m_is_visible = false;
};

} // namespace GUI
} // namespace RoR
