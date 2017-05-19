/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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
    @file   GUI_RigEditorRigPropertiesWindow.cpp
    @author Petr Ohlidal
    @date   09/2014
*/

#include "GUI_RigEditorRigPropertiesWindow.h"
#include "RigEditor_IMain.h"
#include "RigEditor_RigProperties.h"
#include "RoRPrerequisites.h"
#include "Utils.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;
using namespace RigDef;

#define CLASS        RigEditorRigPropertiesWindow
#define MAIN_WIDGET  m_rig_properties_window

CLASS::CLASS(RigEditor::IMain* rig_editor_interface):
    GuiPanelBase(MAIN_WIDGET),
    m_extcamera_mode(ExtCamera::MODE_CLASSIC)
{
    m_rig_editor_interface = rig_editor_interface;

    // [Save] button
    m_button_save->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::SaveButtonClicked);

    // [Cancel] button
    m_button_cancel->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CancelButtonClicked);

    // Close window [X] button
    MyGUI::Window* main_window = MAIN_WIDGET->castType<MyGUI::Window>();
    main_window->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::WindowButtonClicked);

    // Checkbox toggling
    m_checkbox_hide_in_chooser       ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CheckboxClicked);
    m_checkbox_forwardcommands       ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CheckboxClicked);
    m_checkbox_importcommands        ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CheckboxClicked);
    m_checkbox_rescuer               ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CheckboxClicked);
    m_checkbox_disable_default_sounds->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CheckboxClicked);
    m_checkbox_use_advanced_deform   ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CheckboxClicked);
    m_checkbox_rollon                ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CheckboxClicked);
    m_checkbox_guisettings_speedo_use_engine_max_rpm->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CheckboxClicked);

    // Section 'extcamera' radio button
    m_radio_camera_behaviour_classic->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::ExtcameraRadiobuttonClicked);
    m_radio_camera_behaviour_cinecam->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::ExtcameraRadiobuttonClicked);
    m_radio_camera_behaviour_node   ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::ExtcameraRadiobuttonClicked);
    
    // Section 'guisettings' - minimap type radio button
    m_radio_guisettings_minimap_off   ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::MinimapRadiobuttonClicked);
    m_radio_guisettings_minimap_simple->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::MinimapRadiobuttonClicked);
    m_radio_guisettings_minimap_zoom  ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::MinimapRadiobuttonClicked);

    Hide();
}

void CLASS::SaveButtonClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandSaveContentOfRigPropertiesWindow();
    Hide();
}

void CLASS::WindowButtonClicked(MyGUI::Widget* sender, const std::string& name)
{
    Hide(); // There's only close [X] button -> hide window.
}

void CLASS::CancelButtonClicked(MyGUI::Widget* sender)
{
    Hide();
}

void CLASS::SetExtCameraMode(RigDef::ExtCamera::Mode extcamera_mode, RigDef::Node::Ref node_ref)
{
    m_radio_camera_behaviour_classic->setStateSelected(extcamera_mode == RigDef::ExtCamera::MODE_CLASSIC);
    m_radio_camera_behaviour_cinecam->setStateSelected(extcamera_mode == RigDef::ExtCamera::MODE_CINECAM);
    bool is_node = extcamera_mode == RigDef::ExtCamera::MODE_NODE;
    m_radio_camera_behaviour_node->setStateSelected(is_node);
    m_editbox_extcamera_node->setEnabled(is_node);
    if (! is_node)
    {
        m_editbox_extcamera_node->setCaption("");
    }
    else
    {
        m_editbox_extcamera_node->setCaption(node_ref.Str());
    }
    m_extcamera_mode = extcamera_mode;
}

void CLASS::Import(RigEditor::RigProperties* data)
{
    m_editbox_title->setCaption(data->m_title);
    m_editbox_guid->setCaption(data->m_guid);
    if (data->m_fileinfo.category_id != -1)
    {
        m_textbox_category_id->setCaption(TOSTRING(data->m_fileinfo.category_id));
    }
    if (data->m_fileinfo.file_version == 0)
    {
        data->m_fileinfo.file_version = 1;
    }
    m_editbox_version->setCaption(TOSTRING(data->m_fileinfo.file_version));
    m_editbox_uid->setCaption(data->m_fileinfo.unique_id);

    // Description
    Ogre::String description;
    auto desc_end = data->m_description.end();
    for (auto itor = data->m_description.begin(); itor != desc_end; ++itor)
    {
        description += (*itor + "\n");
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
            authors_buffer << TOSTRING(itor->forum_account_id) << " ";
        }
        else
        {
            authors_buffer << "-1 ";
        }
        authors_buffer << itor->name << " ";
        authors_buffer << itor->email << std::endl;
    }
    m_editbox_authors->setCaption(authors_buffer.str());

    // Checkboxes
    m_checkbox_hide_in_chooser       ->setStateSelected(data->m_hide_in_chooser);
    m_checkbox_forwardcommands       ->setStateSelected(data->m_forward_commands);
    m_checkbox_importcommands        ->setStateSelected(data->m_import_commands);
    m_checkbox_rescuer               ->setStateSelected(data->m_is_rescuer);
    m_checkbox_disable_default_sounds->setStateSelected(data->m_disable_default_sounds);
    m_checkbox_use_advanced_deform   ->setStateSelected(data->m_enable_advanced_deformation);
    m_checkbox_rollon                ->setStateSelected(data->m_rollon);

    // Section 'extcamera'
    SetExtCameraMode(data->m_extcamera.mode, data->m_extcamera.node);

    // Section 'set_skeleton_settings'
    m_editbox_beam_thickness_meters       ->setCaption(TOSTRING(data->m_skeleton_settings.beam_thickness_meters));
    m_editbox_beam_visibility_range_meters->setCaption(TOSTRING(data->m_skeleton_settings.visibility_range_meters));

    m_editbox_help_panel_mat_name->setCaption(data->m_help_panel_material_name);
    m_editbox_cab_material_name  ->setCaption(data->m_globals_cab_material_name);
    m_editbox_dry_mass ->setCaption(TOSTRING(data->m_globals_dry_mass));
    m_editbox_load_mass->setCaption(TOSTRING(data->m_globals_load_mass));
    m_editbox_minimass ->setCaption(TOSTRING(data->m_minimass));

    // Section 'guisettings'
    SetMinimapMode(data->m_gui_settings.interactive_overview_map_mode);
    m_checkbox_guisettings_speedo_use_engine_max_rpm->setStateSelected(data->m_gui_settings.use_max_rpm);
    m_editbox_guisettings_help_mat  ->setCaption(data->m_gui_settings.help_material);
    m_editbox_guisettings_speedo_max->setCaption(TOSTRING(data->m_gui_settings.speedo_highest_kph));
    m_editbox_guisettings_speedo    ->setCaption(data->m_gui_settings.speedo_material);
    m_editbox_guisettings_tacho     ->setCaption(data->m_gui_settings.tacho_material);

    std::ostringstream rtt_layouts_buf;
    auto rtt_end = data->m_gui_settings.rtt_dashboard_layouts.end();
    for (auto itor = data->m_gui_settings.rtt_dashboard_layouts.begin(); itor != rtt_end; ++itor)
    {
        rtt_layouts_buf << *itor << std::endl;
    }
    m_editbox_guisettings_rtt_dashboard_layout->setCaption(rtt_layouts_buf.str());

    std::ostringstream dash_layouts_buf;
    auto dash_end = data->m_gui_settings.dashboard_layouts.end();
    for (auto itor = data->m_gui_settings.dashboard_layouts.begin(); itor != dash_end; ++itor)
    {
        dash_layouts_buf << *itor << std::endl;
    }
    m_editbox_guisettings_dashboard_layout->setCaption(dash_layouts_buf.str());
}

#define PARSEUINT(STR)    Ogre::StringConverter::parseUnsignedInt(STR)

void CLASS::Export(RigEditor::RigProperties* data)
{
    data->m_title = m_editbox_title->getCaption();
    data->m_guid = m_editbox_guid->getCaption();

    data->m_fileinfo.unique_id = m_editbox_uid->getCaption();

    bool has_category = ! m_textbox_category_id->getCaption().empty();
    if (has_category)
    {
        data->m_fileinfo.category_id = PARSEUINT(m_textbox_category_id->getCaption());
    }
    else
    {
        data->m_fileinfo.category_id = -1;
    }

    bool has_version = ! m_editbox_version->getCaption().empty();
    if (has_version)
    {
        data->m_fileinfo.file_version = PARSEUINT(m_editbox_version->getCaption());
    }
    else
    {
        data->m_fileinfo.file_version = 0;
    }

    // Description
    data->m_description.clear();
    Ogre::String description_str = m_editbox_description->getCaption();
    Ogre::StringUtil::trim(description_str);
    if (! description_str.empty())
    {
        Ogre::StringVector lines = Ogre::StringUtil::split(description_str, "\r\n"); // Split over CR or LF
        for (auto itor = lines.begin(); itor != lines.end(); ++itor)
        {
            if (! itor->empty()) // Empty line?
            {
                std::string line = RoR::Utils::TrimBlanksAndLinebreaks(*itor);
                if (! line.empty())
                {
                    data->m_description.push_back(line);
                }
            }
        }
    }

    // Authors
    data->m_authors.clear();
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
                int account_id = PARSEINT(tokens[1]);
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
        std::string node_ref_str = m_editbox_extcamera_node->getCaption();
        unsigned flags = Node::Ref::REGULAR_STATE_IS_VALID | Node::Ref::REGULAR_STATE_IS_NAMED; // Fileformatversion>=450 ~ Use named-only nodes
        data->m_extcamera.node = Node::Ref(node_ref_str, 0, flags, 0); 
    }

    // Section 'set_skeleton_settings'
    data->m_skeleton_settings.beam_thickness_meters     = PARSEREAL(m_editbox_beam_thickness_meters->getCaption());
    data->m_skeleton_settings.visibility_range_meters   = PARSEREAL(m_editbox_beam_visibility_range_meters->getCaption());

    data->m_help_panel_material_name    = m_editbox_help_panel_mat_name->getCaption();
    data->m_globals_cab_material_name   = m_editbox_cab_material_name->getCaption();
    data->m_globals_dry_mass            = PARSEREAL(m_editbox_dry_mass->getCaption());
    data->m_globals_load_mass           = PARSEREAL(m_editbox_load_mass->getCaption());
    data->m_minimass                    = PARSEREAL(m_editbox_minimass->getCaption());

    // Section 'guisettings'

    GuiSettings::MapMode minimap_mode = GuiSettings::MAP_MODE_OFF;
    minimap_mode = m_radio_guisettings_minimap_off->getStateSelected()     ? GuiSettings::MAP_MODE_OFF     : minimap_mode;
    minimap_mode = m_radio_guisettings_minimap_simple->getStateSelected()  ? GuiSettings::MAP_MODE_SIMPLE  : minimap_mode;
    minimap_mode = m_radio_guisettings_minimap_zoom->getStateSelected()    ? GuiSettings::MAP_MODE_ZOOM    : minimap_mode;
    data->m_gui_settings.interactive_overview_map_mode = minimap_mode;

    data->m_gui_settings.use_max_rpm         = m_checkbox_guisettings_speedo_use_engine_max_rpm->getStateSelected();
    data->m_gui_settings.help_material       = m_editbox_guisettings_help_mat->getCaption();
    data->m_gui_settings.speedo_highest_kph  = PARSEUINT(m_editbox_guisettings_speedo_max->getCaption());
    data->m_gui_settings.speedo_material     = m_editbox_guisettings_speedo->getCaption();
    data->m_gui_settings.tacho_material      = m_editbox_guisettings_tacho->getCaption();

    Ogre::String rtt_layouts_str = m_editbox_guisettings_rtt_dashboard_layout->getCaption();
    Ogre::StringUtil::trim(rtt_layouts_str);
    if (! rtt_layouts_str.empty())
    {
        Ogre::StringVector lines = Ogre::StringUtil::split(rtt_layouts_str, "\n");
        for (auto itor = lines.begin(); itor != lines.end(); ++itor)
        {
            data->m_gui_settings.rtt_dashboard_layouts.push_back(*itor);
        }
    }

    Ogre::String dash_layouts_str = m_editbox_guisettings_rtt_dashboard_layout->getCaption();
    Ogre::StringUtil::trim(dash_layouts_str);
    if (! dash_layouts_str.empty())
    {
        Ogre::StringVector lines = Ogre::StringUtil::split(dash_layouts_str, "\n");
        for (auto itor = lines.begin(); itor != lines.end(); ++itor)
        {
            data->m_gui_settings.dashboard_layouts.push_back(*itor);
        }
    }
}

void ToggleCheckbox(MyGUI::Button* checkbox)
{
    checkbox->setStateSelected(! checkbox->getStateSelected());
}

void CLASS::CheckboxClicked(MyGUI::Widget* sender)
{
    if (sender == m_checkbox_hide_in_chooser)
    {
        ToggleCheckbox(m_checkbox_hide_in_chooser);
    }
    else if (sender == m_checkbox_forwardcommands)
    {
        ToggleCheckbox(m_checkbox_forwardcommands);
    }
    else if (sender == m_checkbox_importcommands)
    {
        ToggleCheckbox(m_checkbox_importcommands);
    }
    else if (sender == m_checkbox_rescuer)
    {
        ToggleCheckbox(m_checkbox_rescuer);
    }
    else if (sender == m_checkbox_disable_default_sounds)
    {
        ToggleCheckbox(m_checkbox_disable_default_sounds);
    }
    else if (sender == m_checkbox_use_advanced_deform)
    {
        ToggleCheckbox(m_checkbox_use_advanced_deform);
    }
    else if (sender == m_checkbox_rollon)
    {
        ToggleCheckbox(m_checkbox_rollon);
    }
    else if (sender == m_checkbox_guisettings_speedo_use_engine_max_rpm)
    {
        ToggleCheckbox(m_checkbox_guisettings_speedo_use_engine_max_rpm);
    }
}

void CLASS::ExtcameraRadiobuttonClicked(MyGUI::Widget* sender)
{
    using namespace RigDef;

    ExtCamera::Mode mode = ExtCamera::MODE_INVALID;
    if (sender == m_radio_camera_behaviour_classic)
    {
        mode = ExtCamera::MODE_CLASSIC;
    }
    else if (sender == m_radio_camera_behaviour_cinecam)
    {
        mode = ExtCamera::MODE_CINECAM;
    }
    else if (sender == m_radio_camera_behaviour_node)
    {
        mode = ExtCamera::MODE_NODE;
    }
    if (mode != m_extcamera_mode)
    {
        SetExtCameraMode(mode, Node::Ref());
    }
}

void CLASS::SetMinimapMode(RigDef::GuiSettings::MapMode mode)
{
    using namespace RigDef;

    m_radio_guisettings_minimap_off   ->setStateSelected(mode == GuiSettings::MAP_MODE_OFF);
    m_radio_guisettings_minimap_simple->setStateSelected(mode == GuiSettings::MAP_MODE_SIMPLE);
    m_radio_guisettings_minimap_zoom  ->setStateSelected(mode == GuiSettings::MAP_MODE_ZOOM);
}

void CLASS::MinimapRadiobuttonClicked(MyGUI::Widget* sender)
{
    GuiSettings::MapMode mode = GuiSettings::MAP_MODE_INVALID;
    if (sender == m_radio_guisettings_minimap_off)
    {
        mode = GuiSettings::MAP_MODE_OFF;
    }
    else if (sender == m_radio_guisettings_minimap_simple)
    {
        mode = GuiSettings::MAP_MODE_SIMPLE;
    }
    else if (sender == m_radio_guisettings_minimap_zoom)
    {
        mode = GuiSettings::MAP_MODE_ZOOM;
    }
    SetMinimapMode(mode);
}
