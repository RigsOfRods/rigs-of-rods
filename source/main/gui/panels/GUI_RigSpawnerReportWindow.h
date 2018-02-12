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
/// @author Petr Ohlidal
/// @date   02/2015

#pragma once

#include "ForwardDeclarations.h"
#include "GUI_RigSpawnerReportWindowLayout.h"
#include "GuiPanelBase.h"

namespace RoR {
namespace GUI {

class ActorSpawnerReportWindow: public ActorSpawnerReportWindowLayout, public GuiPanelBase
{
public:

    ActorSpawnerReportWindow();

    void SetRigLoadingReport(std::string const& vehicle_name, std::string const& text, int num_errors, int num_warnings, int num_other);

    void SetVisible(bool v);
    bool IsVisible();

private:

    void WindowButtonClicked(MyGUI::Widget* sender, const std::string& name);

    GuiManagerInterface* m_gui_manager_interface;
};

} // namespace GUI
} // namespace RoR
