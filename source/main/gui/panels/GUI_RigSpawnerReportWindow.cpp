/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

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

/** 
	@file   
	@author Petr Ohlidal
	@date   02/2015
*/

#include "GUI_RigSpawnerReportWindow.h"

using namespace RoR;
using namespace GUI;

RigSpawnerReportWindow::RigSpawnerReportWindow(GuiManagerInterface* gui_manager_interface):
	GuiPanelBase(m_rig_spawner_report_window),
	m_gui_manager_interface(gui_manager_interface)
{
	// Close window [X] button
	MyGUI::Window* main_window = m_rig_spawner_report_window->castType<MyGUI::Window>();
	main_window->eventWindowButtonPressed += MyGUI::newDelegate(this, &RigSpawnerReportWindow::WindowButtonClicked);

	// Start hidden
	Hide();
}

void RigSpawnerReportWindow::SetRigLoadingReport(std::string const & vehicle_name, std::string const & text, int num_errors, int num_warnings, int num_other)
{
	m_rig_spawner_report_window->setCaption("Loading report: " + vehicle_name);

	std::stringstream summary;
	bool first = true;
	if (num_errors > 0)
	{
		summary << num_errors <<  "#FF3300 Errors #FFFFFF";
		first = false;
	}
	if (num_warnings > 0)
	{
		if (!first)
		{
			summary << ", ";
		}
		summary << num_warnings << "#FFFF00 Warnings #FFFFFF";
		first = false;
	}
	if (num_other > 0)
	{
		if (!first)
		{
			summary << ", ";
		}
		summary << num_other << " notes";
	}
	m_report_summary_textbox->setCaption(summary.str());

	m_report_text_area->setCaption(text);
}

void RigSpawnerReportWindow::WindowButtonClicked(MyGUI::Widget* sender, const std::string& name)
{
	m_gui_manager_interface->HideRigSpawnerReportWindow(); // There's only close [X] button -> hide window.
}
