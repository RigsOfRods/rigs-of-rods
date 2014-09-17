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
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   RigEditor_Config.cpp
	@date   09/2014
	@author Petr Ohlidal
*/

#include "ConfigFile.h"
#include "RigEditor_Config.h"

using namespace RoR;
using namespace RoR::RigEditor;

Config::Config(Ogre::String const & conf_file_path)
{
	/* Load config */
	RoR::ConfigFile conf_file;
	conf_file.load(conf_file_path);

	/* Parse config */
	viewport_background_color = conf_file.GetColourValue("viewport_background_color_rgb");
	scene_ambient_light_color = conf_file.GetColourValue("scene_ambient_light_color_rgb");

	node_mouse_box_halfsize_px   = conf_file.GetInt("node_mouse_box_halfsize_px");

	beam_generic_color        = conf_file.GetColourValue("beam_generic_color_rgb");
	beam_invisible_color      = conf_file.GetColourValue("beam_invisible_color_rgb");
	beam_rope_color           = conf_file.GetColourValue("beam_rope_color_rgb");
	beam_support_color        = conf_file.GetColourValue("beam_support_color_rgb");

	meshwheel2_beam_bounded_color        = conf_file.GetColourValue("meshwheel2_beam_bounded_color_rgb");
	meshwheel2_beam_reinforcement_color  = conf_file.GetColourValue("meshwheel2_beam_reinforcement_color_rgb");
	meshwheel2_beam_rigidity_color       = conf_file.GetColourValue("meshwheel2_beam_rigidity_color_rgb");

	node_generic_color        = conf_file.GetColourValue("node_generic_color_rgb");
	node_generic_point_size   = conf_file.GetFloat("node_generic_point_size");
	node_hover_color          = conf_file.GetColourValue("node_hover_color_rgb");
	node_hover_point_size     = conf_file.GetFloat("node_hover_point_size");
	node_selected_color       = conf_file.GetColourValue("node_selected_color_rgb");
	node_selected_point_size  = conf_file.GetFloat("node_selected_point_size");

	camera_near_clip_distance = conf_file.GetFloat("camera_near_clip_distance");
	camera_far_clip_distance  = conf_file.GetFloat("camera_far_clip_distance");
	camera_FOVy_degrees       = conf_file.GetFloat("camera_FOVy_degrees");
	ortho_camera_zoom_ratio   = conf_file.GetFloat("ortho_camera_zoom_ratio");
}
