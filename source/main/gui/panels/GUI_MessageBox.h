/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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
/// @author Moncef Ben Slimane
/// @date   12/2014

#pragma once

#include "ForwardDeclarations.h"

#include <string>

namespace RoR {
namespace GUI {

class MessageBoxDialog
{
public:
    MessageBoxDialog();
    ~MessageBoxDialog();

    void          Show(const char* title, const char* text, bool allow_close, const char* button1_text, const char* button2_text);
    void          Draw();
    inline bool   IsVisible() const { return m_is_visible; }

private:
    std::string m_title;
    std::string m_text;
    std::string m_button1_text;
    std::string m_button2_text;
    bool*       m_close_handle; // If nullptr, close button is hidden. Otherwise visible.
    bool        m_is_visible;
};

} // namespace GUI
} // namespace RoR
