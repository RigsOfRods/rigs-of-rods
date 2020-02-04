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

#pragma once

#include "RoRPrerequisites.h"

namespace RoR {
namespace GUI {

class LoadingWindow 
{
public:
    void setProgress(int _percent, const std::string& _text = "", bool render_frame = true);
    void Draw();

    void SetVisible(bool v)           { m_is_visible = v; }
    bool IsVisible() const            { return m_is_visible; }

private:
    int  m_percent = -1; // -1 disables progressbar display
    bool m_is_visible = false;
    std::string m_text;
};

} // namespace GUI
} // namespace RoR
