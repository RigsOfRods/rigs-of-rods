/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2014-2017 Petr Ohlidal & contributors

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
/// @date   11/2014

#pragma once

#include "ForwardDeclarations.h"
#include "GUI_GameMainMenuLayout.h"

#include "GUI_GameAbout.h"

namespace RoR {
namespace GUI {

class GameMainMenu: public GameMainMenuLayout
{
public:
    GameMainMenu();
    ~GameMainMenu();

    void Show();
    void Hide();
    void SetPosition(int pixels_left, int pixels_top);
    int GetHeight();
    void SetVisible(bool v);
    bool IsVisible();

private:
    void eventMouseButtonClickSelectTerrainButton(MyGUI::WidgetPtr _sender);
    void eventMouseButtonClickSettingButton(MyGUI::WidgetPtr _sender);
    void eventMouseButtonClickAboutButton(MyGUI::WidgetPtr _sender);
    void eventMouseButtonClickExitButton(MyGUI::WidgetPtr _sender);
    void eventMouseButtonClickMultiPlayerButton(MyGUI::WidgetPtr _sender);
};

} // namespace GUI
} // namespace RoR
