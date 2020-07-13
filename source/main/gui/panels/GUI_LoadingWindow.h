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

#include "Application.h"
#include <OgreRoot.h>
#include <OgreTimer.h>


namespace RoR {
namespace GUI {

class LoadingWindow 
{
public:
    const int PERC_HIDE_PROGRESSBAR = -1;
    const int PERC_SHOW_SPINNER = -2;

    void SetProgress(int _percent, const std::string& _text = "", bool render_frame = true);
    void SetProgressNetConnect(const std::string& net_status);
    void Draw();

    void SetVisible(bool v)           { m_is_visible = v; }
    bool IsVisible() const            { return m_is_visible; }

private:
    int                 m_percent = PERC_HIDE_PROGRESSBAR;
    bool                m_is_visible = false;
    std::string         m_text;
    int                 m_text_num_lines = -1;
    Ogre::Timer         m_timer;
    float               m_spinner_counter = 0.f;
};

} // namespace GUI
} // namespace RoR
