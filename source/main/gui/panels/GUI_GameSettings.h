/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013+     Petr Ohlidal & contributors

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

#include "ConfigFile.h"
#include "ForwardDeclarations.h"
#include "GUI_GameSettingsLayout.h"
#include "GUI_GameSettings.h"
#include "InputEngine.h"

#include <OIS.h>

namespace RoR {
namespace GUI {

class GameSettings: public GameSettingsLayout
{
public:
    GameSettings();
    ~GameSettings();

    void Show();
    void Hide(bool isMenu = true);

    void SetVisible(bool v);
    bool IsVisible();

private:
    //Basic things
    void notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name);
    void CheckAndInit();
    void UpdateControls();
    void SaveSettings();

    //Buttons
    void eventMouseButtonClickSaveButton(MyGUI::WidgetPtr _sender);
    void eventMouseButtonClickRegenCache(MyGUI::WidgetPtr _sender);
    void eventMouseButtonClickClearCache(MyGUI::WidgetPtr _sender);

    //Checkboxes
    void OnCheckboxPlain(MyGUI::WidgetPtr _sender);
    void OnCheckboxRestartNotice(MyGUI::WidgetPtr _sender);

    //Sliders
    void OnVolumeSlider(MyGUI::ScrollBar* _sender, size_t _position);
    void OnFPSLimiterSlider(MyGUI::ScrollBar* _sender, size_t _position);
    void OnSightRangeSlider(MyGUI::ScrollBar* _sender, size_t _position);

    //Key mapping things
    void OnTabChange(MyGUI::TabControl* _sender, size_t _index);
    void LoadKeyMap();
    void OnKeymapTypeChange(MyGUI::ComboBox* _sender, size_t _index);
    // FIXME: void OnReMapPress(MyGUI::WidgetPtr _sender);
    void FrameEntered(float dt);
    bool startCounter;
    unsigned long endTime;
    Ogre::String LastKeyCombo;

    std::map<int, std::vector<event_trigger_t>> KeyMap;

    RoR::ConfigFile m_ogre_cfg;

    bool m_is_initialized;
    bool m_is_keymap_loaded;
    bool ShowRestartNotice;
    bool isFrameActivated;
};

} // namespace GUI
} // namespace RoR
