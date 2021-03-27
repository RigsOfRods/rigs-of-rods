/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
/// @brief Presets in truck format.
/// All values are initialized to -1 which means empty value.
/// Actual defaults are filled in spawner.

#pragma once

#include "SimData.h"

namespace RoR {
namespace Truck {

/// Directive 'set_beam_defaults'
struct BeamDefaults: Element
{
    float springiness = -1.f;
    float damping_constant = -1.f;
    float deformation_threshold = -1.f;
    float breaking_threshold = -1.f;
    float visual_beam_diameter = -1.f;
    std::string beam_material_name = "-1";
    float plastic_deform_coef = -1.f;
};

/// Directive 'set_beam_defaults_scale'
struct BeamDefaultsScale: Element
{
    float springiness = -1.f;
    float damping_constant = -1.f;
    float deformation_threshold_constant = -1.f;
    float breaking_threshold_constant = -1.f;
};

/// Directives 'set_camera_settings', 'prop_camera_mode', 'flexbody_camera_mode'
struct CameraSettings: Element
{
    enum Mode
    {
        MODE_ALWAYS   = -2,
        MODE_EXTERNAL = -1,
        MODE_CINECAM  = 1,

        MODE_INVALID  = 0xFFFFFFFF
    };

    Mode      mode = MODE_INVALID;
    NodeIdx_t cinecam_index = node_t::INVALID_IDX;
};

/// Directive 'set_collision_range'
struct CollisionRangePreset: Element
{
    int collrange = -1;
};

/// Directive 'detacher_group'
struct DetacherGroupPreset: Element
{
    int detacher_group = -1;
    bool _end = false;
};

/// Directive 'set_inertia_defaults'
struct InertiaDefaults: public Element, public BaseInertia
{
};

/// Directive 'set_managedmaterials_options'
struct ManagedMatOptions: Element
{
    bool double_sided = false;
};

/// Directive 'set_node_defaults'
struct NodeDefaults: Element
{
    float loadweight  = -1.f;
    float friction    = -1.f;
    float volume      = -1.f;
    float surface     = -1.f;
    std::string options;
};

/// Inline-section 'set_skeleton_display'
struct SkeletonSettings: Element
{
    float visibility_range_meters = -1;
    float beam_thickness_meters = -1;
};

} // namespace Truck
} // namespace RoR
