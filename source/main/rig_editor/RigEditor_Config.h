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

struct Config
{
	Config(Ogre::String const & conf_file_path);

	Ogre::ColourValue   viewport_background_color;
	Ogre::ColourValue   scene_ambient_light_color;

	int                 gui_dialog_delete_placement_x_px;
	int                 gui_dialog_delete_placement_y_px;
	int                 gui_dialog_delete_cursor_fence_px;

	/* Rig manipulation */
	int                 node_mouse_box_halfsize_px;

	/* Beam coloring */
	Ogre::ColourValue   beam_generic_color;
	Ogre::ColourValue   beam_invisible_color;
	Ogre::ColourValue   beam_support_color;
	Ogre::ColourValue   beam_rope_color;

	Ogre::ColourValue   meshwheel2_beam_bounded_color;
	Ogre::ColourValue   meshwheel2_beam_reinforcement_color;
	Ogre::ColourValue   meshwheel2_beam_rigidity_color;

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
};

} // namespace RigEditor

} // namespace RoR
