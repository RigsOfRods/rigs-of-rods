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

#include "ForwardDeclarations.h"
#include "GUI_VehicleDescriptionLayout.h"

namespace RoR
{
    namespace GUI
    {

        class VehicleDescription : public VehicleDescriptionLayout
        {
          public:
            VehicleDescription();
            ~VehicleDescription();

            bool IsVisible();
            void SetVisible(bool v);

          private:
            void notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string &_name);
            void CenterToScreen();
            void LoadText();

            static const unsigned int COMMANDS_VISIBLE = 50;
        };

    } // namespace GUI
} // namespace RoR
