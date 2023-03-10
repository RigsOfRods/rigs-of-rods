/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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
/// @brief  Values used (and updated) during spawn. Names are chosen to match the original sourcecode.

#pragma once

#include "BitFlags.h"
#include "SimConstants.h"

#include <string>

namespace RoR {

/// @addtogroup Physics
/// @{

struct ActorSpawnContext
{
    // 'globals':
    std::string texname;       //!< Keyword 'globals' - material

    bool        wheel_contact_requested = false;
    bool        rescuer = false;
    bool        disable_default_sounds = false;
    int         detacher_group_state = DEFAULT_DETACHER_GROUP;
    bool        slope_brake = false;
    float       beam_creak = BEAM_CREAK_DEFAULT;
    int         externalcameramode = 0;
    int         externalcameranode = -1;

    float       default_spring = DEFAULT_SPRING;
    float       default_spring_scale = 1;
    float       default_damp = DEFAULT_DAMP;
    float       default_damp_scale = 1;
    float       default_deform = BEAM_DEFORM;
    float       default_deform_scale = 1;
    float       default_break = BEAM_BREAK;
    float       default_break_scale = 1;

    float       default_beam_diameter = DEFAULT_BEAM_DIAMETER;
    float       default_plastic_coef = 0;
    float       skeleton_beam_diameter = BEAM_SKELETON_DIAMETER;
    std::string default_beam_material = "tracks/beam";
    float       default_node_friction = NODE_FRICTION_COEF_DEFAULT;
    float       default_node_volume = NODE_VOLUME_COEF_DEFAULT;
    float       default_node_surface = NODE_SURFACE_COEF_DEFAULT;
    float       default_node_loadweight = NODE_LOADWEIGHT_DEFAULT;
    BitMask_t   default_node_options = 0;

    bool        managedmaterials_doublesided = false;
    float       inertia_startDelay = -1;
    float       inertia_stopDelay = -1;
    std::string inertia_default_startFunction;
    std::string inertia_default_stopFunction;

    // Fusedrag autocalc
    float       fuse_z_min = 1000.0f;
    float       fuse_z_max = -1000.0f;
    float       fuse_y_min = 1000.0f;
    float       fuse_y_max = -1000.0f;

    bool        enable_advanced_deformation = false;
    int         lockgroup_default = NODE_LOCKGROUP_DEFAULT;

    // Minimass
    float       global_minimass = DEFAULT_MINIMASS;   //!< 'minimass' - does not change default minimass (only updates global fallback value)!
    float       default_minimass = -1;                //!< 'set_default_minimass' - does not change global minimass!
    bool        minimass_skip_loaded = false;       //!< The 'l' flag on 'minimass'.

    // Submeshes
    std::string submesh_groundmodel;

    // 'guisettings':
    std::string tachomat;
    std::string speedomat;
};

/// @} // addtogroup Physics

} // namespace RoR
