/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

#include <string>

namespace RoR {
namespace GUI {

class SimPerfStats
{
public:
    void SetVisible(bool vis) { m_is_visible = vis; }
    bool IsVisible() const { return m_is_visible; }

    void Draw();

private:
    bool         m_is_visible = false;
    std::string Convert(const float f); // converts const float to std::string with precision
};

} // namespace GUI
} // namespace RoR
