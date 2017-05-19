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
    @file   RigEditor_Config.cpp
    @date   09/2014
    @author Petr Ohlidal
*/

#include "RigEditor_Config.h"

#include "ConfigFile.h"

using namespace RoR;
using namespace RoR::RigEditor;

Config::Config(Ogre::String const & conf_file_path)
{
    // Init
    memset(this, 0, sizeof(Config));

    // Load config file
    RoR::ConfigFile conf_file;
    conf_file.load(conf_file_path);

    // Parse config
    viewport_background_color = conf_file.GetColourValue("viewport_background_color_rgb");
    scene_ambient_light_color = conf_file.GetColourValue("scene_ambient_light_color_rgb");

    node_mouse_box_halfsize_px   = conf_file.GetInt("node_mouse_box_halfsize_px");

    gui_nodebeam_panels_tooltip_text_color     = conf_file.GetColourValue("gui_nodebeam_panels_tooltip_text_color");
    gui_nodebeam_panels_field_mixvalues_color  = conf_file.GetColourValue("gui_nodebeam_panels_field_mixvalues_color");

    gui_dialog_delete_placement_x_px     = conf_file.GetInt("gui_dialog_delete_placement_x_px");
    gui_dialog_delete_placement_y_px     = conf_file.GetInt("gui_dialog_delete_placement_y_px");
    gui_dialog_delete_cursor_fence_px    = conf_file.GetInt("gui_dialog_delete_cursor_fence_px");

    // NODE/BEAM panels
    gui_nodes_panel_position.anchor_right      = conf_file.GetBool("gui_nodes_panel_anchor_right");
    gui_nodes_panel_position.anchor_bottom     = conf_file.GetBool("gui_nodes_panel_anchor_bottom");
    gui_nodes_panel_position.margin_left_px    = conf_file.GetInt( "gui_nodes_panel_margin_left_px");
    gui_nodes_panel_position.margin_right_px   = conf_file.GetInt( "gui_nodes_panel_margin_right_px");
    gui_nodes_panel_position.margin_bottom_px  = conf_file.GetInt( "gui_nodes_panel_margin_bottom_px");
    gui_nodes_panel_position.margin_top_px     = conf_file.GetInt( "gui_nodes_panel_margin_top_px");

    gui_beams_panel_position.anchor_right      = conf_file.GetBool("gui_beams_panel_anchor_right");
    gui_beams_panel_position.anchor_bottom     = conf_file.GetBool("gui_beams_panel_anchor_bottom");
    gui_beams_panel_position.margin_left_px    = conf_file.GetInt( "gui_beams_panel_margin_left_px");
    gui_beams_panel_position.margin_right_px   = conf_file.GetInt( "gui_beams_panel_margin_right_px");
    gui_beams_panel_position.margin_bottom_px  = conf_file.GetInt( "gui_beams_panel_margin_bottom_px");
    gui_beams_panel_position.margin_top_px     = conf_file.GetInt( "gui_beams_panel_margin_top_px");

    gui_hydros_panel_position.anchor_right      = conf_file.GetBool("gui_hydros_panel_anchor_right");
    gui_hydros_panel_position.anchor_bottom     = conf_file.GetBool("gui_hydros_panel_anchor_bottom");
    gui_hydros_panel_position.margin_left_px    = conf_file.GetInt( "gui_hydros_panel_margin_left_px");
    gui_hydros_panel_position.margin_right_px   = conf_file.GetInt( "gui_hydros_panel_margin_right_px");
    gui_hydros_panel_position.margin_bottom_px  = conf_file.GetInt( "gui_hydros_panel_margin_bottom_px");
    gui_hydros_panel_position.margin_top_px     = conf_file.GetInt( "gui_hydros_panel_margin_top_px");

    gui_commands2_panel_position.anchor_right      = conf_file.GetBool("gui_commands2_panel_anchor_right");
    gui_commands2_panel_position.anchor_bottom     = conf_file.GetBool("gui_commands2_panel_anchor_bottom");
    gui_commands2_panel_position.margin_left_px    = conf_file.GetInt( "gui_commands2_panel_margin_left_px");
    gui_commands2_panel_position.margin_right_px   = conf_file.GetInt( "gui_commands2_panel_margin_right_px");
    gui_commands2_panel_position.margin_bottom_px  = conf_file.GetInt( "gui_commands2_panel_margin_bottom_px");
    gui_commands2_panel_position.margin_top_px     = conf_file.GetInt( "gui_commands2_panel_margin_top_px");
    
    gui_shocks_panel_position.anchor_right      = conf_file.GetBool("gui_shocks_panel_anchor_right");
    gui_shocks_panel_position.anchor_bottom     = conf_file.GetBool("gui_shocks_panel_anchor_bottom");
    gui_shocks_panel_position.margin_left_px    = conf_file.GetInt( "gui_shocks_panel_margin_left_px");
    gui_shocks_panel_position.margin_right_px   = conf_file.GetInt( "gui_shocks_panel_margin_right_px");
    gui_shocks_panel_position.margin_bottom_px  = conf_file.GetInt( "gui_shocks_panel_margin_bottom_px");
    gui_shocks_panel_position.margin_top_px     = conf_file.GetInt( "gui_shocks_panel_margin_top_px");

    gui_shocks2_panel_position.anchor_right      = conf_file.GetBool("gui_shocks2_panel_anchor_right");
    gui_shocks2_panel_position.anchor_bottom     = conf_file.GetBool("gui_shocks2_panel_anchor_bottom");
    gui_shocks2_panel_position.margin_left_px    = conf_file.GetInt( "gui_shocks2_panel_margin_left_px");
    gui_shocks2_panel_position.margin_right_px   = conf_file.GetInt( "gui_shocks2_panel_margin_right_px");
    gui_shocks2_panel_position.margin_bottom_px  = conf_file.GetInt( "gui_shocks2_panel_margin_bottom_px");
    gui_shocks2_panel_position.margin_top_px     = conf_file.GetInt( "gui_shocks2_panel_margin_top_px");

    // WHEEL panels

    gui_meshwheels2_panel_position.anchor_right      = conf_file.GetBool("gui_meshwheels2_panel_anchor_right");
    gui_meshwheels2_panel_position.anchor_bottom     = conf_file.GetBool("gui_meshwheels2_panel_anchor_bottom");
    gui_meshwheels2_panel_position.margin_left_px    = conf_file.GetInt( "gui_meshwheels2_panel_margin_left_px");
    gui_meshwheels2_panel_position.margin_right_px   = conf_file.GetInt( "gui_meshwheels2_panel_margin_right_px");
    gui_meshwheels2_panel_position.margin_bottom_px  = conf_file.GetInt( "gui_meshwheels2_panel_margin_bottom_px");
    gui_meshwheels2_panel_position.margin_top_px     = conf_file.GetInt( "gui_meshwheels2_panel_margin_top_px");

    gui_flexbodywheels_panel_position.anchor_right      = conf_file.GetBool("gui_flexbodywheels_panel_anchor_right");
    gui_flexbodywheels_panel_position.anchor_bottom     = conf_file.GetBool("gui_flexbodywheels_panel_anchor_bottom");
    gui_flexbodywheels_panel_position.margin_left_px    = conf_file.GetInt( "gui_flexbodywheels_panel_margin_left_px");
    gui_flexbodywheels_panel_position.margin_right_px   = conf_file.GetInt( "gui_flexbodywheels_panel_margin_right_px");
    gui_flexbodywheels_panel_position.margin_bottom_px  = conf_file.GetInt( "gui_flexbodywheels_panel_margin_bottom_px");
    gui_flexbodywheels_panel_position.margin_top_px     = conf_file.GetInt( "gui_flexbodywheels_panel_margin_top_px");

    // NODE/BEAM DISPLAY
    
    beam_generic_color        = conf_file.GetColourValue("beam_generic_color_rgb");
    beam_invisible_color      = conf_file.GetColourValue("beam_invisible_color_rgb");
    beam_rope_color           = conf_file.GetColourValue("beam_rope_color_rgb");
    beam_support_color        = conf_file.GetColourValue("beam_support_color_rgb");

    cinecam_beam_color_rgb               = conf_file.GetColourValue("cinecam_beam_color_rgb");
    shock_absorber_beam_color_rgb        = conf_file.GetColourValue("shock_absorber_beam_color_rgb");
    shock_absorber_2_beam_color_rgb      = conf_file.GetColourValue("shock_absorber_2_beam_color_rgb");
    steering_hydro_beam_color_rgb        = conf_file.GetColourValue("steering_hydro_beam_color_rgb");
    command_hydro_beam_color_rgb         = conf_file.GetColourValue("command_hydro_beam_color_rgb");

    meshwheel2_beam_bounded_color        = conf_file.GetColourValue("meshwheel2_beam_bounded_color_rgb");
    meshwheel2_beam_reinforcement_color  = conf_file.GetColourValue("meshwheel2_beam_reinforcement_color_rgb");
    meshwheel2_beam_rigidity_color       = conf_file.GetColourValue("meshwheel2_beam_rigidity_color_rgb");

    meshwheel_beam_bounded_color        = conf_file.GetColourValue("meshwheel_beam_bounded_color_rgb");
    meshwheel_beam_reinforcement_color  = conf_file.GetColourValue("meshwheel_beam_reinforcement_color_rgb");
    meshwheel_beam_rigidity_color       = conf_file.GetColourValue("meshwheel_beam_rigidity_color_rgb");

    flexbodywheel_rim_connection_color     = conf_file.GetColourValue("flexbodywheel_rim_connection_color");
    flexbodywheel_rim_reinforcement_color  = conf_file.GetColourValue("flexbodywheel_rim_reinforcement_color");
    flexbodywheel_tyre_connection_color	   = conf_file.GetColourValue("flexbodywheel_tyre_connection_color");
    flexbodywheel_tyre_reinforcement_color = conf_file.GetColourValue("flexbodywheel_tyre_reinforcement_color");
    flexbodywheel_tyre_rigidity_color	   = conf_file.GetColourValue("flexbodywheel_tyre_rigidity_color");

    node_generic_color        = conf_file.GetColourValue("node_generic_color_rgb");
    node_generic_point_size   = conf_file.GetFloat("node_generic_point_size");
    node_hover_color          = conf_file.GetColourValue("node_hover_color_rgb");
    node_hover_point_size     = conf_file.GetFloat("node_hover_point_size");
    node_selected_color       = conf_file.GetColourValue("node_selected_color_rgb");
    node_selected_point_size  = conf_file.GetFloat("node_selected_point_size");

    // CAMERA

    camera_near_clip_distance = conf_file.GetFloat("camera_near_clip_distance");
    camera_far_clip_distance  = conf_file.GetFloat("camera_far_clip_distance");
    camera_FOVy_degrees       = conf_file.GetFloat("camera_FOVy_degrees");
    ortho_camera_zoom_ratio   = conf_file.GetFloat("ortho_camera_zoom_ratio");

    // HIGHLIGHT AABB PROPERTIES

    wheels_selection_highlight_boxes_color   = conf_file.GetColourValue("wheels_selection_highlight_boxes_color");
    wheels_selection_highlight_boxes_padding = conf_file.GetFloat("wheels_selection_highlight_boxes_padding");
    wheels_hover_highlight_boxes_color       = conf_file.GetColourValue("wheels_hover_highlight_boxes_color");
    wheels_hover_highlight_boxes_padding     = conf_file.GetFloat("wheels_hover_highlight_boxes_padding");

    // RIG PROPERTIES

    new_rig_initial_box_half_size     = conf_file.GetFloat("new_rig_initial_box_half_size");

}
