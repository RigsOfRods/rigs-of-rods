/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal & contributors

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
#include "GUI_DebugOptionsLayout.h"

namespace RoR
{

    namespace GUI
    {

        class DebugOptions : public DebugOptionsLayout
        {
          public:
            DebugOptions();
            ~DebugOptions();

            void Show();
            void Hide();
            bool IsVisible();
            void SetVisible(bool v);

          private:
            void notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string &_name);
            void UpdateControls();
            void SaveConfig();

            // checkboxes
            void OnDebugActorMassCheck(MyGUI::WidgetPtr _sender);
            void OnDebugColiMeshesCheck(MyGUI::WidgetPtr _sender);
            void OnIngameConsoleCheck(MyGUI::WidgetPtr _sender);
            void OnDebugEnvMapCheck(MyGUI::WidgetPtr _sender);
            void OnDebugVideoCameraCheck(MyGUI::WidgetPtr _sender);
            void OnDebugTriggerCheck(MyGUI::WidgetPtr _sender);
            void OnDebugDOFCheck(MyGUI::WidgetPtr _sender);
            void OnBeamBreakCheck(MyGUI::WidgetPtr _sender);
            void OnBeamDeformCheck(MyGUI::WidgetPtr _sender);
            void OnAdvLoggingCheck(MyGUI::WidgetPtr _sender);
            void OnCrashReportCheck(MyGUI::WidgetPtr _sender);

            // buttons
            void eventMouseButtonClickSaveButton(MyGUI::WidgetPtr _sender);

            std::map<std::string, std::string> DebugOptionsMap;
        };

    } // namespace GUI

} // namespace RoR
