/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   GUI_RigEditorFilePropertiesWindow.cpp
	@author Petr Ohlidal
	@date   09/2014
*/

#include "GUI_RigEditorFilePropertiesWindow.h"
#include "RigEditor_RigProperties.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

RigEditorFilePropertiesWindow::RigEditorFilePropertiesWindow(RigEditor::IMain* rig_editor_interface)
{
	m_rig_editor_interface = rig_editor_interface;

	// [Cancel] button
	m_button_cancel->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CancelButtonClicked);

	// Close window [X] button
	MyGUI::Window* main_window = m_rig_properties_window->castType<MyGUI::Window>();
	main_window->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::WindowButtonClicked);

	Hide();
}

void RigEditorFilePropertiesWindow::Show()
{
	m_rig_properties_window->setVisible(true);
}

void RigEditorFilePropertiesWindow::Hide()
{
	m_rig_properties_window->setVisible(false);
}

void RigEditorFilePropertiesWindow::CenterToScreen()
{
	MyGUI::IntSize windowSize = m_rig_properties_window->getSize();
	MyGUI::IntSize parentSize = m_rig_properties_window->getParentSize();

	m_rig_properties_window->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);
}

bool RigEditorFilePropertiesWindow::IsVisible()
{
	return m_rig_properties_window->isVisible();
}

void RigEditorFilePropertiesWindow::WindowButtonClicked(MyGUI::Widget* sender, const std::string& name)
{
	Hide(); // There's only close [X] button -> hide window.
}

void RigEditorFilePropertiesWindow::CancelButtonClicked(MyGUI::Widget* sender)
{
	Hide();
}

#define STR(X) Ogre::StringConverter::toString(X)


void RigEditorFilePropertiesWindow::Import(RigEditor::RigProperties* data)
{
	using namespace RigDef;

	m_editbox_title->setCaption(data->m_title);
	m_editbox_guid->setCaption(data->m_guid);
	if (data->m_fileinfo._has_unique_id)
	{
		m_editbox_uid->setCaption(data->m_fileinfo.unique_id);
	}
	if (data->m_fileinfo._has_category_id)
	{
		m_editbox_uid->setCaption(STR(data->m_fileinfo.category_id));
	}
	if (data->m_fileinfo._has_file_version_set)
	{
		m_editbox_uid->setCaption(STR(data->m_fileinfo.file_version));
	}

	// Description
	Ogre::String description;
	auto desc_end = data->m_description.end();
	for (auto itor = data->m_description.begin(); itor != desc_end; ++itor)
	{
		description += (*itor + std::endl);
	}
	m_editbox_description->setCaption(description);

	// Authors
	std::stringstream authors_buffer;
	auto authors_end = data->m_authors.end();
	for (auto itor = data->m_authors.begin(); itor != authors_end; ++itor)
	{
		authors_buffer << itor->type << " ";
		if (itor->_has_forum_account)
		{
			authors_buffer << "-1 ";
		}
		else
		{
			authors_buffer << STR(itor->forum_account_id) << " ";
		}
		authors_buffer << itor->name << " ";
		authors_buffer << itor->email << std::endl;
	}
	m_editbox_authors->setCaption(authors_buffer.str());

	// Checkboxes
	m_checkbox_hide_in_chooser->setStateSelected(data->m_hide_in_chooser);
	m_checkbox_forwardcommands->setStateSelected(data->m_forward_commands);
	m_checkbox_importcommands->setStateSelected(data->m_import_commands);
	m_checkbox_rescuer->setStateSelected(data->m_is_rescuer);
	m_checkbox_disable_default_sounds->setStateSelected(data->m_disable_default_sounds);
	m_checkbox_use_advanced_deform->setStateSelected(data->m_enable_advanced_deformation);
	m_checkbox_rollon->setStateSelected(data->m_rollon);

	// Section 'extcamera'
	RigDef::ExtCamera::Mode extcamera_mode = data->m_extcamera.mode;
	m_radio_camera_behaviour_classic->setStateSelected(extcamera_mode == RigDef::ExtCamera::MODE_CLASSIC);
	m_radio_camera_behaviour_cinecam->setStateSelected(extcamera_mode == RigDef::ExtCamera::MODE_CINECAM);
	m_radio_camera_behaviour_node->setStateSelected(extcamera_mode == RigDef::ExtCamera::MODE_NODE);
	m_editbox_extcamera_node->setEnabled(extcamera_mode == RigDef::ExtCamera::MODE_NODE);

	// Section 'set_skeleton_settings'
	m_editbox_beam_thickness_meters->setCaption(STR(data->m_skeleton_settings.beam_thickness_meters));
	m_editbox_beam_visibility_range_meters->setCaption(STR(data->m_skeleton_settings.visibility_range_meters));

	m_editbox_help_panel_mat_name->setCaption(data->m_help_panel_material_name);
	m_editbox_cab_material_name->setCaption(data->m_globals_cab_material_name);
	m_editbox_dry_mass->setCaption(STR(data->m_globals_dry_mass));
	m_editbox_load_mass->setCaption(STR(data->m_globals_load_mass));
	m_editbox_minimass->setCaption(STR(data->m_minimass));

	// Section 'guisettings'
	GuiSettings::MapMode minimap_mode = data->m_gui_settings.interactive_overview_map_mode;
	m_radio_guisettings_minimap_off->setStateSelected(minimap_mode == GuiSettings::MAP_MODE_OFF);
	m_radio_guisettings_minimap_simple->setStateSelected(minimap_mode == GuiSettings::MAP_MODE_SIMPLE);
	m_radio_guisettings_minimap_zoom->setStateSelected(minimap_mode == GuiSettings::MAP_MODE_ZOOM);
	
	m_checkbox_guisettings_speedo_use_engine_max_rpm->setStateSelected(data->m_gui_settings.use_max_rpm);
	m_editbox_guisettings_help_mat->setCaption(data->m_gui_settings.help_material);
	m_editbox_guisettings_speedo_max->setCaption(STR(data->m_gui_settings.speedo_highest_kph));
	m_editbox_guisettings_speedo->setCaption(data->m_gui_settings.speedo_material);
	m_editbox_guisettings_tacho->setCaption(data->m_gui_settings.tacho_material);

	MyGUI::UString rtt_layouts;
	static const MyGUI::UString newline("\n");
	auto rtt_end = data->m_gui_settings.rtt_dashboard_layouts.end()
	for (auto itor = data->m_gui_settings.rtt_dashboard_layouts.begin(); itor != rtt_end; ++itor)
	{
		rtt_layouts += (*itor + newline);
	}
	m_editbox_guisettings_rtt_dashboard_layout->setCaption(rtt_layouts);

	MyGUI::UString dash_layouts;
	auto dash_end = data->m_gui_settings.dashboard_layouts.end()
	for (auto itor = data->m_gui_settings.dashboard_layouts.begin(); itor != dash_end; ++itor)
	{
		dash_layouts += (*itor + newline);
	}
	m_editbox_guisettings_dashboard_layout->setCaption(dash_layouts);
}

// Lazy coder's macros
#define INT(STR)     Ogre::StringConverter::parseInt(STR)
#define UINT(STR)    Ogre::StringConverter::parseUnsignedInt(STR)
#define FLOAT(STR)   Ogre::StringConverter::parseReal(STR);

void RigEditorFilePropertiesWindow::Export(RigEditor::RigProperties* data)
{
	data->m_title = m_editbox_title->getCaption();
	data->m_guid = m_editbox_guid->getCaption();

	bool has_uid = ! m_editbox_uid->getCaption().empty();
	if (has_uid)
	{
		data->m_fileinfo.unique_id = m_editbox_uid->getCaption();
	}
	data->m_fileinfo._has_unique_id = has_uid;

	bool has_category = ! m_textbox_category_id->getCaption().empty();
	if (has_category)
	{
		data->m_fileinfo.category_id = UINT(m_textbox_category_id->getCaption());
	}
	data->m_fileinfo._has_category_id = has_category;

	bool has_version = ! m_editbox_version->getCaption().empty();
	if (has_version)
	{
		data->m_fileinfo.file_version = UINT(m_editbox_version->getCaption());
	}
	data->m_fileinfo._has_file_version_set = has_version;

	// Description
	Ogre::String description_str = m_editbox_description->getCaption();
	Ogre::StringUtil::trim(description_str);
	if (! description_str.empty())
	{
		Ogre::StringVector lines = Ogre::StringUtil::split(description_str, "\n");
		for (auto itor = lines.begin(); itor != lines.end(); ++itor)
		{
			data->m_description.push_back(*itor);
		}
	}

	// Authors
	Ogre::String authors_str = m_editbox_authors->getCaption();
	Ogre::StringUtil::trim(authors_str);
	if (! authors_str.empty())
	{
		Ogre::StringVector lines = Ogre::StringUtil::split(authors_str, "\n");
		for (auto itor = lines.begin(); itor != lines.end(); ++itor)
		{
			Ogre::String line = *itor;
			Ogre::StringUtil::trim(line);
			if (line.empty())
			{
				continue;
			}
			Ogre::StringVector tokens = Ogre::StringUtil::split(line, " \t");
			RigDef::Author author;
			author.type = tokens[0];
			if (tokens.size() > 1)
			{
				int account_id = INT(tokens[1]);
				if (account_id > -1)
				{
					author.forum_account_id = static_cast<unsigned int>(account_id);
					author._has_forum_account = true;
				}

				if (tokens.size() > 2)
				{
					author.name = tokens[2];

					if (tokens.size() > 3)
					{
						author.email = tokens[3];
					}
				}
			}
			
			data->m_authors.push_back(author);
		}
	}

	// Checkboxes
	data->m_hide_in_chooser               = m_checkbox_hide_in_chooser->getStateSelected();
	data->m_forward_commands              = m_checkbox_forwardcommands->getStateSelected();
	data->m_import_commands               = m_checkbox_importcommands->getStateSelected();
	data->m_is_rescuer                    = m_checkbox_rescuer->getStateSelected();
	data->m_disable_default_sounds        = m_checkbox_disable_default_sounds->getStateSelected();
	data->m_enable_advanced_deformation   = m_checkbox_use_advanced_deform->getStateSelected();
	data->m_rollon                        = m_checkbox_rollon->getStateSelected();

	// Section 'extcamera'
	RigDef::ExtCamera::Mode extcamera_mode = RigDef::ExtCamera::MODE_CLASSIC;
	extcamera_mode = m_radio_camera_behaviour_classic->getStateSelected()   ? RigDef::ExtCamera::MODE_CLASSIC   : extcamera_mode;
	extcamera_mode = m_radio_camera_behaviour_cinecam->getStateSelected()   ? RigDef::ExtCamera::MODE_CINECAM   : extcamera_mode;
	extcamera_mode = m_radio_camera_behaviour_node->getStateSelected()      ? RigDef::ExtCamera::MODE_NODE      : extcamera_mode;
	data->m_extcamera.mode = extcamera_mode;
	if (extcamera_mode == RigDef::ExtCamera::MODE_NODE)
	{
		data->m_extcamera.node.SetNum(INT(m_editbox_extcamera_node->getCaption()));
	}

	// Section 'set_skeleton_settings'
	data->m_skeleton_settings.beam_thickness_meters     = FLOAT(m_editbox_beam_thickness_meters->getCaption());
	data->m_skeleton_settings.visibility_range_meters   = FLOAT(m_editbox_beam_visibility_range_meters->getCaption());

	data->m_help_panel_material_name    = m_editbox_help_panel_mat_name->getCaption();
	data->m_globals_cab_material_name   = m_editbox_cab_material_name->getCaption();
	data->m_globals_dry_mass            = FLOAT(m_editbox_dry_mass->getCaption());
	data->m_globals_load_mass           = FLOAT(m_editbox_load_mass->getCaption());
	data->m_minimass                    = FLOAT(m_editbox_minimass->getCaption());

	// Section 'guisettings'

	GuiSettings::MapMode minimap_mode = GuiSettings::MAP_MODE_OFF;
	minimap_mode = m_radio_guisettings_minimap_off->getStateSelected()     ? GuiSettings::MAP_MODE_OFF     : minimap_mode;
	minimap_mode = m_radio_guisettings_minimap_simple->getStateSelected()  ? GuiSettings::MAP_MODE_SIMPLE  : minimap_mode;
	minimap_mode = m_radio_guisettings_minimap_zoom->getStateSelected()    ? GuiSettings::MAP_MODE_ZOOM    : minimap_mode;
	data->m_gui_settings.interactive_overview_map_mode = minimap_mode;

	data->m_gui_settings.use_max_rpm         = m_checkbox_guisettings_speedo_use_engine_max_rpm->getStateSelected();
	data->m_gui_settings.help_material       = m_editbox_guisettings_help_mat->getCaption();
	data->m_gui_settings.speedo_highest_kph  = UINT(m_editbox_guisettings_speedo_max->getCaption());
	data->m_gui_settings.speedo_material     = m_editbox_guisettings_speedo->getCaption();
	data->m_gui_settings.tacho_material      = m_editbox_guisettings_tacho->getCaption();

	Ogre::String rtt_layouts_str = m_editbox_guisettings_rtt_dashboard_layout->getCaption();
	Ogre::StringUtil::trim(rtt_layouts_str);
	if (! rtt_layouts_str.empty())
	{
		Ogre::StringVector lines = Ogre::StringUtil::split(rtt_layouts_str, "\n");
		for (auto itor = lines.begin(); itor != lines.end(); ++itor)
		{
			ata->m_gui_settings.rtt_dashboard_layouts.push_back(*itor);
		}
	}

	Ogre::String dash_layouts_str = m_editbox_guisettings_rtt_dashboard_layout->getCaption();
	Ogre::StringUtil::trim(dash_layouts_str);
	if (! dash_layouts_str.empty())
	{
		Ogre::StringVector lines = Ogre::StringUtil::split(dash_layouts_str, "\n");
		for (auto itor = lines.begin(); itor != lines.end(); ++itor)
		{
			ata->m_gui_settings.dashboard_layouts.push_back(*itor);
		}
	}
}
