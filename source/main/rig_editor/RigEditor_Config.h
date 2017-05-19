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
    @file   RigEditor_Config.h
    @date   09/2014
    @author Petr Ohlidal
*/

#pragma once

#include <OgreColourValue.h>

namespace RoR
{

namespace RigEditor
{

struct GuiPanelPositionData
{
    bool anchor_right;
    bool anchor_bottom;
    int margin_left_px;
    int margin_top_px;
    int margin_right_px;
    int margin_bottom_px;
};

struct Config
{
    Config(Ogre::String const & conf_file_path);

    /* Scene */
    Ogre::ColourValue   viewport_background_color;
    Ogre::ColourValue   scene_ambient_light_color;

    /* GUI */
    Ogre::ColourValue   gui_nodebeam_panels_tooltip_text_color;
    Ogre::ColourValue   gui_nodebeam_panels_field_mixvalues_color;
    int                 gui_dialog_delete_placement_x_px;
    int                 gui_dialog_delete_placement_y_px;
    int                 gui_dialog_delete_cursor_fence_px;

    GuiPanelPositionData   gui_nodes_panel_position;
    GuiPanelPositionData   gui_beams_panel_position;
    GuiPanelPositionData   gui_hydros_panel_position;
    GuiPanelPositionData   gui_commands2_panel_position;
    GuiPanelPositionData   gui_shocks_panel_position;
    GuiPanelPositionData   gui_shocks2_panel_position;
    GuiPanelPositionData   gui_meshwheels2_panel_position;
    GuiPanelPositionData   gui_flexbodywheels_panel_position;
    
    /* Rig manipulation HUD */
    int                 node_mouse_box_halfsize_px;

    /* Beam coloring */
    Ogre::ColourValue   beam_generic_color;
    Ogre::ColourValue   beam_invisible_color;
    Ogre::ColourValue   beam_support_color;
    Ogre::ColourValue   beam_rope_color;

    Ogre::ColourValue   command_hydro_beam_color_rgb;
    Ogre::ColourValue   steering_hydro_beam_color_rgb;
    Ogre::ColourValue   shock_absorber_beam_color_rgb;
    Ogre::ColourValue   shock_absorber_2_beam_color_rgb;
    Ogre::ColourValue   cinecam_beam_color_rgb;

    Ogre::ColourValue   meshwheel2_beam_bounded_color;
    Ogre::ColourValue   meshwheel2_beam_reinforcement_color;
    Ogre::ColourValue   meshwheel2_beam_rigidity_color;

    Ogre::ColourValue   meshwheel_beam_bounded_color;
    Ogre::ColourValue   meshwheel_beam_reinforcement_color;
    Ogre::ColourValue   meshwheel_beam_rigidity_color;

    Ogre::ColourValue   flexbodywheel_rim_connection_color;
    Ogre::ColourValue   flexbodywheel_rim_reinforcement_color;
    Ogre::ColourValue   flexbodywheel_tyre_connection_color;
    Ogre::ColourValue   flexbodywheel_tyre_reinforcement_color;
    Ogre::ColourValue   flexbodywheel_tyre_rigidity_color;

    /* Highlight AABB display */
    Ogre::ColourValue   wheels_selection_highlight_boxes_color;
    float               wheels_selection_highlight_boxes_padding;
    Ogre::ColourValue   wheels_hover_highlight_boxes_color;
    float               wheels_hover_highlight_boxes_padding;

    /* Node display */
    Ogre::ColourValue   node_generic_color;
    float               node_generic_point_size;
    Ogre::ColourValue   node_hover_color;
    float               node_hover_point_size;
    Ogre::ColourValue   node_selected_color;
    float               node_selected_point_size;

    /* Camera */
    float               camera_near_clip_distance;
    float               camera_far_clip_distance;
    float               camera_FOVy_degrees;
    float               ortho_camera_zoom_ratio;

    /* Rig properties */
    float               new_rig_initial_box_half_size;

};

} // namespace RigEditor

} // namespace RoR
