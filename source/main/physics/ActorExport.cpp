/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

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

#include "Actor.h"
#include "Application.h"
#include "CacheSystem.h"
#include "GameContext.h"
#include "RigDef_File.h"

#include <Ogre.h>

using namespace RoR;
using namespace RigDef;

static RigDef::Node::Ref BuildNodeRef(Actor* actor, NodeNum_t n)
{
    if (n == NODENUM_INVALID)
    {
        return RigDef::Node::Ref();
    }

    const BitMask_t nodeflags = RigDef::Node::Ref::REGULAR_STATE_IS_VALID;
    if (actor->ar_nodes_name[n] != "")
    {
        return RigDef::Node::Ref(actor->ar_nodes_name[n], 0, nodeflags, 0);
    }
    else
    {
        return RigDef::Node::Ref(fmt::format("{}", actor->ar_nodes_id[n]), actor->ar_nodes_id[n], nodeflags, 0);
    }
}

static bool IsActuallyShockBeam(const beam_t& beam)
{
    // Beams with attributes {SHOCK1 && BEAM_NORMAL} are actually wheel beams ~ don't ask.
    return beam.bm_type == BEAM_HYDRO 
        && (beam.bounded == SHOCK1 || beam.bounded == SHOCK2 || beam.bounded == SHOCK3);
}

static void UpdateSetBeamDefaults(std::shared_ptr<BeamDefaults>& beam_defaults, Actor* actor, int i)
{
    float b_spring = actor->ar_beams[i].k;
    float b_damp = actor->ar_beams[i].d;
    float b_deform = actor->ar_beams[i].default_beam_deform;
    float b_break = actor->ar_beams[i].initial_beam_strength;
    float b_diameter = actor->ar_beams[i].default_beam_diameter;
    if (IsActuallyShockBeam(actor->ar_beams[i]))
    {
        b_spring = actor->ar_beams[i].shock->sbd_spring;
        b_damp = actor->ar_beams[i].shock->sbd_damp;
        b_break = actor->ar_beams[i].shock->sbd_break;
    }

    if (beam_defaults->springiness != b_spring
        || beam_defaults->damping_constant != b_damp
        || beam_defaults->deformation_threshold != b_deform
        || beam_defaults->breaking_threshold != b_break
        || beam_defaults->visual_beam_diameter != b_diameter)
    {
        beam_defaults = std::shared_ptr<BeamDefaults>(new BeamDefaults);
        beam_defaults->springiness = b_spring;
        beam_defaults->damping_constant = b_damp;
        beam_defaults->deformation_threshold = b_deform;
        beam_defaults->breaking_threshold = b_break;
        beam_defaults->visual_beam_diameter = b_diameter;
    }
}

static void UpdateSetNodeDefaults(std::shared_ptr<NodeDefaults>& node_defaults, Actor* actor, NodeNum_t i)
{
    float n_loadweight = actor->ar_nodes_default_loadweights[i];
    float n_friction = actor->ar_nodes[i].friction_coef;
    float n_volume = actor->ar_nodes[i].volume_coef;
    float n_surface = actor->ar_nodes[i].surface_coef;
    if (node_defaults->load_weight != n_loadweight
        || node_defaults->friction != n_friction
        || node_defaults->surface != n_surface
        || node_defaults->volume != n_volume)
    {
        node_defaults = std::shared_ptr<NodeDefaults>(new NodeDefaults);
        node_defaults->load_weight = n_loadweight;
        node_defaults->friction = n_friction;
        node_defaults->surface = n_surface;
        node_defaults->volume = n_volume;
    }
}

static void UpdateSetInertiaDefaults(std::shared_ptr<Inertia>& inertia_defaults, Actor* actor, CmdKeyInertia& cmdkey_inertia)
{
    const float start_delay = cmdkey_inertia.GetStartDelay();
    const float stop_delay = cmdkey_inertia.GetStopDelay();
    const std::string startfn = cmdkey_inertia.GetStartFunction();
    const std::string stopfn = cmdkey_inertia.GetStopFunction();

    if (inertia_defaults->start_delay_factor != start_delay
        || inertia_defaults->stop_delay_factor != stop_delay
        || inertia_defaults->start_function != startfn
        || inertia_defaults->stop_function != stopfn
        )
    {
        inertia_defaults = std::shared_ptr<Inertia>(new Inertia);
        inertia_defaults->start_delay_factor = start_delay;
        inertia_defaults->stop_delay_factor = stop_delay;
        inertia_defaults->start_function = startfn;
        inertia_defaults->stop_function = stopfn;
    }
}

void Actor::propagateNodeBeamChangesToDef()
{
    // PROOF OF CONCEPT:
    // * assumes 'section/end_section' is not used (=only root module exists)
    // * uses dummy 'set_beam_defaults' and 'detacher_group' for the hookbeams (nodes with 'h' option)
    // * doesn't separate out 'set_beam_defaults_scale' from 'set_beam_defaults'
    // * uses only 'set_default_minimass', ignores global 'minimass'
    // * ignores 'detacher_group' for the hookbeams (nodes with 'h' option)
    // -------------------------------------------------------------------------

    

    // Purge old data
    m_used_actor_entry->actor_def->root_module->nodes.clear();
    m_used_actor_entry->actor_def->root_module->beams.clear();
    m_used_actor_entry->actor_def->root_module->cinecam.clear();
    m_used_actor_entry->actor_def->root_module->wheels.clear();
    m_used_actor_entry->actor_def->root_module->wheels2.clear();
    m_used_actor_entry->actor_def->root_module->meshwheels.clear();
    m_used_actor_entry->actor_def->root_module->meshwheels2.clear();
    m_used_actor_entry->actor_def->root_module->flexbodywheels.clear();
    m_used_actor_entry->actor_def->root_module->shocks.clear();
    m_used_actor_entry->actor_def->root_module->shocks2.clear();
    m_used_actor_entry->actor_def->root_module->shocks3.clear();
    m_used_actor_entry->actor_def->root_module->hydros.clear();

    // Prepare 'set_node_defaults' with builtin values.
    auto node_defaults = std::shared_ptr<NodeDefaults>(new NodeDefaults); // comes pre-filled
    
    // Prepare 'set_default_minimass' with builtin values.
    auto default_minimass = std::shared_ptr<DefaultMinimass>(new DefaultMinimass());
    default_minimass->min_mass_Kg = DEFAULT_MINIMASS;

    // Prepare 'set_beam_defaults' with builtin values.
    auto beam_defaults = std::shared_ptr<BeamDefaults>(new BeamDefaults);
    beam_defaults->springiness           = DEFAULT_SPRING;
    beam_defaults->damping_constant      = DEFAULT_DAMP;
    beam_defaults->deformation_threshold = BEAM_DEFORM;
    beam_defaults->breaking_threshold    = BEAM_BREAK;
    beam_defaults->visual_beam_diameter  = DEFAULT_BEAM_DIAMETER;

    // Prepare 'set_inertia_defaults' with builtin values.
    auto inertia_defaults = std::shared_ptr<Inertia>(new Inertia);

    // Prepare 'detacher_group' with builtin values.
    int detacher_group = DEFAULT_DETACHER_GROUP;


    // ~~~ Nodes ~~~

    for (NodeNum_t i = 0; i < ar_num_nodes; i++)
    {
        if (ar_nodes[i].nd_rim_node || ar_nodes[i].nd_tyre_node || ar_nodes[i].nd_cinecam_node)
        {
            // Skip wheel nodes and cinecam nodes
            continue;
        }

        UpdateSetNodeDefaults(node_defaults, this, i);

        // Check if 'set_default_minimass' needs update.
        if (default_minimass->min_mass_Kg != ar_minimass[i])
        {
            default_minimass = std::shared_ptr<DefaultMinimass>(new DefaultMinimass());
            default_minimass->min_mass_Kg = ar_minimass[i];
        }

        // Build the node
        RigDef::Node node;
        node.node_defaults = node_defaults;
        node.default_minimass = default_minimass;
        node.beam_defaults = beam_defaults; // Needed for hookbeams (nodes with 'h' option)
        node.detacher_group = detacher_group; // Needed for hookbeams (nodes with 'h' option)
        if (ar_nodes_name[i] != "")
        {
            node.id = RigDef::Node::Id(ar_nodes_name[i]);
        }
        else
        {
            node.id = RigDef::Node::Id(ar_nodes_id[i]);
        }
        node.position = ar_nodes_spawn_offsets[i];
        node.load_weight_override = ar_nodes_override_loadweights[i];
        node._has_load_weight_override = (node.load_weight_override >= 0.0f);
        node.options = ar_nodes_options[i];

        // Submit the node
        m_used_actor_entry->actor_def->root_module->nodes.push_back(node);
    }


    // ~~~ Beams ~~~

    for (int i = 0; i < ar_num_beams; i++)
    {
        if (!ar_beams_user_defined[i])
        {
            // Skip everything not from 'beams' (wheels/cinecam/hooknode/wings/rotators etc...)
            continue;
        }

        UpdateSetBeamDefaults(beam_defaults, this, i);

        // Check if 'detacher_group' needs update.
        if (detacher_group != ar_beams[i].detacher_group)
        {
            detacher_group = ar_beams[i].detacher_group;
        }

        // Build the beam
        RigDef::Beam beam;
        beam.defaults = beam_defaults;
        beam.detacher_group = detacher_group;
        beam.extension_break_limit = ar_beams[i].longbound;
        
        if (ar_beams[i].bounded == SUPPORTBEAM)
        {
            beam._has_extension_break_limit = true;
            beam.options |= RigDef::Beam::OPTION_s_SUPPORT;
        }
        else if (ar_beams[i].bounded == ROPE)
        {
            beam.options |= RigDef::Beam::OPTION_r_ROPE;
        }
        
        if (ar_beams_invisible[i])
        {
            beam.options |= RigDef::Beam::OPTION_i_INVISIBLE;
        }

        // Build refs to the nodes
        beam.nodes[0] = BuildNodeRef(this, ar_beams[i].p1->pos);
        beam.nodes[1] = BuildNodeRef(this, ar_beams[i].p2->pos);

        // Submit the beam
        m_used_actor_entry->actor_def->root_module->beams.push_back(beam);
    }


    // ~~~ Cinecam ~~~

    for (NodeNum_t i = 0; i < ar_num_nodes; i++)
    {
        if (!ar_nodes[i].nd_cinecam_node)
        {
            // Skip everything but cinecam nodes
            continue;
        }

        UpdateSetNodeDefaults(node_defaults, this, i);

        // Find all beams that connect to the cinecam node
        std::array<int, 8> cinecam_nodes = { -1, -1, -1, -1, -1, -1, -1, -1 };
        int num_cinecam_nodes = 0;
        int cinecam_beamid = -1;
        for (int j = 0; j < ar_num_beams; j++)
        {
            if (ar_beams[j].p1->pos == i)
            {
                cinecam_beamid = j;
                cinecam_nodes[num_cinecam_nodes++] = ar_beams[j].p2->pos;
            }
            else if (ar_beams[j].p2->pos == i)
            {
                cinecam_beamid = j;
                cinecam_nodes[num_cinecam_nodes++] = ar_beams[j].p1->pos;
            }
        }
        ROR_ASSERT(num_cinecam_nodes == 8);
        ROR_ASSERT(cinecam_beamid != -1);

        UpdateSetBeamDefaults(beam_defaults, this, cinecam_beamid);

        // Build the cinecam (NOTE: no minimass/detacher_group)
        RigDef::Cinecam cinecam;
        cinecam.node_defaults = node_defaults;
        cinecam.beam_defaults = beam_defaults;
        cinecam.position = ar_nodes_spawn_offsets[i];
        for (int n = 0; n < 8; n++)
        {
            cinecam.nodes[n] = BuildNodeRef(this, cinecam_nodes[n]);
        }
        cinecam.node_mass = ar_nodes[i].mass;
        cinecam.spring = ar_beams[cinecam_beamid].k;
        cinecam.damping = ar_beams[cinecam_beamid].d;

        // Submit the cinecam
        m_used_actor_entry->actor_def->root_module->cinecam.push_back(cinecam);
    }


    // ~~~ Wheels ~~~
    for (int i = 0; i < ar_num_wheels; i++)
    {
        // PLEASE maintain the same order of arguments as the docs: https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#vehicle-specific

        UpdateSetNodeDefaults(node_defaults, this, ar_wheels[i].wh_nodes[0]->pos);
        UpdateSetBeamDefaults(beam_defaults, this, ar_wheels[i].wh_beam_start);
        switch (ar_wheels[i].wh_arg_keyword)
        {
        case Keyword::WHEELS:
        {
            RigDef::Wheel wheel;
            wheel.beam_defaults = beam_defaults;
            wheel.node_defaults = node_defaults;
            // radius
            wheel.radius = ar_wheels[i].wh_radius;
            // rays
            wheel.num_rays = ar_wheels[i].wh_arg_num_rays;
            // nodes
            wheel.nodes[0] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_0->pos);
            wheel.nodes[1] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_1->pos);
            wheel.rigidity_node = BuildNodeRef(this, ar_wheels[i].wh_arg_rigidity_node);
            // braking, propulsion
            wheel.braking = ar_wheels[i].wh_braking;
            wheel.propulsion = ar_wheels[i].wh_propulsed;
            // arm node
            wheel.reference_arm_node = BuildNodeRef(this, (ar_wheels[i].wh_arm_node ? ar_wheels[i].wh_arm_node->pos : NODENUM_INVALID));
            // mass
            wheel.mass = ar_wheels[i].wh_mass;
            // springiness, damping
            wheel.springiness = ar_beams[ar_wheels[i].wh_beam_start].k;
            wheel.damping = ar_beams[ar_wheels[i].wh_beam_start].d;
            // materials
            wheel.face_material_name = ar_wheels[i].wh_arg_media1;
            wheel.band_material_name = ar_wheels[i].wh_arg_media2;

            m_used_actor_entry->actor_def->root_module->wheels.push_back(wheel);
            break;
        }
        case Keyword::WHEELS2:
        {
            RigDef::Wheel2 wheel;
            wheel.beam_defaults = beam_defaults;
            wheel.node_defaults = node_defaults;
            // radius
            wheel.rim_radius = ar_wheels[i].wh_rim_radius;
            wheel.tyre_radius = ar_wheels[i].wh_radius;
            // rays
            wheel.num_rays = ar_wheels[i].wh_arg_num_rays;
            // nodes
            wheel.nodes[0] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_0->pos);
            wheel.nodes[1] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_1->pos);
            wheel.rigidity_node = BuildNodeRef(this, ar_wheels[i].wh_arg_rigidity_node);
            // braking, propulsion
            wheel.braking = ar_wheels[i].wh_braking;
            wheel.propulsion = ar_wheels[i].wh_propulsed;
            // arm node
            wheel.reference_arm_node = BuildNodeRef(this, (ar_wheels[i].wh_arm_node ? ar_wheels[i].wh_arm_node->pos : NODENUM_INVALID));
            // mass
            wheel.mass = ar_wheels[i].wh_mass;
            // springiness, damping
            wheel.rim_springiness = ar_wheels[i].wh_arg_rim_spring;
            wheel.rim_damping = ar_wheels[i].wh_arg_rim_damping;
            wheel.tyre_springiness = ar_beams[ar_wheels[i].wh_beam_start].k;
            wheel.tyre_damping = ar_beams[ar_wheels[i].wh_beam_start].d;
            // materials
            wheel.face_material_name = ar_wheels[i].wh_arg_media1;
            wheel.band_material_name = ar_wheels[i].wh_arg_media2;

            m_used_actor_entry->actor_def->root_module->wheels2.push_back(wheel);
            break;
        }
        case Keyword::MESHWHEELS:
        {
            RigDef::MeshWheel wheel;
            wheel.beam_defaults = beam_defaults;
            wheel.node_defaults = node_defaults;
            // radius
            wheel.rim_radius = ar_wheels[i].wh_rim_radius;
            wheel.tyre_radius = ar_wheels[i].wh_radius;
            // rays
            wheel.num_rays = ar_wheels[i].wh_arg_num_rays;
            // nodes
            wheel.nodes[0] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_0->pos);
            wheel.nodes[1] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_1->pos);
            wheel.rigidity_node = BuildNodeRef(this, ar_wheels[i].wh_arg_rigidity_node);
            // braking, propulsion
            wheel.braking = ar_wheels[i].wh_braking;
            wheel.propulsion = ar_wheels[i].wh_propulsed;
            // arm node
            wheel.reference_arm_node = BuildNodeRef(this, (ar_wheels[i].wh_arm_node ? ar_wheels[i].wh_arm_node->pos : NODENUM_INVALID));
            // mass
            wheel.mass = ar_wheels[i].wh_mass;
            // springiness, damping
            wheel.spring = ar_beams[ar_wheels[i].wh_beam_start].k;
            wheel.damping = ar_beams[ar_wheels[i].wh_beam_start].d;
            // media
            wheel.side = ar_wheels[i].wh_arg_side;
            wheel.mesh_name = ar_wheels[i].wh_arg_media1;
            wheel.material_name = ar_wheels[i].wh_arg_media2;

            m_used_actor_entry->actor_def->root_module->meshwheels.push_back(wheel);
            break;
        }
        case Keyword::MESHWHEELS2:
        {
            RigDef::MeshWheel2 wheel;
            wheel.beam_defaults = beam_defaults;
            wheel.node_defaults = node_defaults;
            // Special handling of 'set_beam_defaults' (these define rim params)
            wheel.beam_defaults->springiness = ar_wheels[i].wh_arg_rim_spring;
            wheel.beam_defaults->damping_constant = ar_wheels[i].wh_arg_rim_damping;

            // radius
            wheel.rim_radius = ar_wheels[i].wh_rim_radius;
            wheel.tyre_radius = ar_wheels[i].wh_radius;
            // rays
            wheel.num_rays = ar_wheels[i].wh_arg_num_rays;
            // nodes
            wheel.nodes[0] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_0->pos);
            wheel.nodes[1] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_1->pos);
            wheel.rigidity_node = BuildNodeRef(this, ar_wheels[i].wh_arg_rigidity_node);
            // braking, propulsion
            wheel.braking = ar_wheels[i].wh_braking;
            wheel.propulsion = ar_wheels[i].wh_propulsed;
            // arm node
            wheel.reference_arm_node = BuildNodeRef(this, (ar_wheels[i].wh_arm_node ? ar_wheels[i].wh_arm_node->pos : NODENUM_INVALID));
            // mass
            wheel.mass = ar_wheels[i].wh_mass;
            // springiness, damping
            wheel.spring = ar_beams[ar_wheels[i].wh_beam_start].k;
            wheel.damping = ar_beams[ar_wheels[i].wh_beam_start].d;
            // media
            wheel.side = ar_wheels[i].wh_arg_side;
            wheel.mesh_name = ar_wheels[i].wh_arg_media1;
            wheel.material_name = ar_wheels[i].wh_arg_media2;

            m_used_actor_entry->actor_def->root_module->meshwheels2.push_back(wheel);
            break;
        }
        case Keyword::FLEXBODYWHEELS:
        {
            RigDef::FlexBodyWheel wheel;
            wheel.beam_defaults = beam_defaults;
            wheel.node_defaults = node_defaults;
            // Special handling of 'set_beam_defaults' (these define rim params)
            wheel.beam_defaults->springiness = ar_wheels[i].wh_arg_rim_spring;
            wheel.beam_defaults->damping_constant = ar_wheels[i].wh_arg_rim_damping;

            // radius
            wheel.rim_radius = ar_wheels[i].wh_rim_radius;
            wheel.tyre_radius = ar_wheels[i].wh_radius;
            // rays
            wheel.num_rays = ar_wheels[i].wh_arg_num_rays;
            // nodes
            wheel.nodes[0] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_0->pos);
            wheel.nodes[1] = BuildNodeRef(this, ar_wheels[i].wh_axis_node_1->pos);
            wheel.rigidity_node = BuildNodeRef(this, ar_wheels[i].wh_arg_rigidity_node);
            // braking, propulsion
            wheel.braking = ar_wheels[i].wh_braking;
            wheel.propulsion = ar_wheels[i].wh_propulsed;
            // arm node
            wheel.reference_arm_node = BuildNodeRef(this, (ar_wheels[i].wh_arm_node ? ar_wheels[i].wh_arm_node->pos : NODENUM_INVALID));
            // mass
            wheel.mass = ar_wheels[i].wh_mass;
            // springiness, damping
            wheel.rim_springiness = ar_beams[ar_wheels[i].wh_beam_start].k;
            wheel.rim_damping = ar_beams[ar_wheels[i].wh_beam_start].d;
            // media
            wheel.side = ar_wheels[i].wh_arg_side;
            wheel.rim_mesh_name = ar_wheels[i].wh_arg_media1;
            wheel.tyre_mesh_name = ar_wheels[i].wh_arg_media2;

            m_used_actor_entry->actor_def->root_module->flexbodywheels.push_back(wheel);
            break;
        }
        default:
            ROR_ASSERT(false);
            break;
        }
    }

    // ~~~ Shocks ~~~

    for (int i = 0; i < ar_num_beams; i++)
    {
        const beam_t& beam = ar_beams[i];
        switch (beam.bounded)
        {
        case SpecialBeam::SHOCK1:
        {
            if (!IsActuallyShockBeam(beam))
            {
                // This is actually a wheel beam - skip it.
                continue;
            }

            UpdateSetBeamDefaults(beam_defaults, this, i);

            RigDef::Shock def;
            def.beam_defaults = beam_defaults;
            def.nodes[0] = BuildNodeRef(this, beam.p1->pos);
            def.nodes[1] = BuildNodeRef(this, beam.p2->pos);
            def.short_bound = beam.shortbound;
            def.long_bound = beam.longbound;
            def.precompression = beam.shock->shock_precompression;

            // shock1-specific
            def.spring_rate = beam.k;
            def.damping = beam.d;

            // options
            if (BITMASK_IS_1(beam.shock->flags, SHOCK_FLAG_LACTIVE))
            {
                BITMASK_SET_1(def.options, RigDef::Shock::OPTION_L_ACTIVE_LEFT);
            }
            if (BITMASK_IS_1(beam.shock->flags, SHOCK_FLAG_RACTIVE))
            {
                BITMASK_SET_1(def.options, RigDef::Shock::OPTION_R_ACTIVE_RIGHT);
            }
            if (ar_beams_invisible[i])
            {
                BITMASK_SET_1(def.options, RigDef::Shock::OPTION_i_INVISIBLE);
            }

            m_used_actor_entry->actor_def->root_module->shocks.push_back(def);
            break;
        }

        case SpecialBeam::SHOCK2:
        {
            UpdateSetBeamDefaults(beam_defaults, this, i);

            RigDef::Shock2 def;
            def.beam_defaults = beam_defaults;
            def.nodes[0] = BuildNodeRef(this, beam.p1->pos);
            def.nodes[1] = BuildNodeRef(this, beam.p2->pos);
            def.short_bound = beam.shortbound;
            def.long_bound = beam.longbound;
            def.precompression = beam.shock->shock_precompression;

            // shock2-specific
            def.beam_defaults->springiness      = beam.shock->sbd_spring;
            def.beam_defaults->damping_constant = beam.shock->sbd_damp  ;
            def.spring_in                       = beam.shock->springin  ;
            def.damp_in                         = beam.shock->dampin    ;
            def.spring_out                      = beam.shock->springout ;
            def.damp_out                        = beam.shock->dampout   ;
            def.progress_factor_spring_in       = beam.shock->sprogin   ;
            def.progress_factor_damp_in         = beam.shock->dprogin   ;
            def.progress_factor_spring_out      = beam.shock->sprogout  ;
            def.progress_factor_damp_out        = beam.shock->dprogout  ;

            // options
            if (ar_beams_invisible[i])
            {
                BITMASK_SET_1(def.options, RigDef::Shock::OPTION_i_INVISIBLE);
            }

            m_used_actor_entry->actor_def->root_module->shocks2.push_back(def);
            break;
        }

        case SpecialBeam::SHOCK3:
        {
            UpdateSetBeamDefaults(beam_defaults, this, i);

            RigDef::Shock3 def;
            def.beam_defaults = beam_defaults;
            def.nodes[0] = BuildNodeRef(this, beam.p1->pos);
            def.nodes[1] = BuildNodeRef(this, beam.p2->pos);
            def.short_bound = beam.shortbound;
            def.long_bound = beam.longbound;
            def.precompression = beam.shock->shock_precompression;

            // same as shock2
            def.beam_defaults->springiness = beam.shock->sbd_spring;
            def.beam_defaults->damping_constant = beam.shock->sbd_damp;
            def.spring_in = beam.shock->springin;
            def.damp_in = beam.shock->dampin;
            def.spring_out = beam.shock->springout;
            def.damp_out = beam.shock->dampout;

            // shock3-specific
            def.split_vel_in  = beam.shock->splitin  ;
            def.damp_in_slow  = beam.shock->dslowin  ;
            def.damp_in_fast  = beam.shock->dfastin  ;
            def.split_vel_out = beam.shock->splitout ;
            def.damp_out_slow = beam.shock->dslowout ;
            def.damp_out_fast = beam.shock->dfastout ;

            // options
            if (ar_beams_invisible[i])
            {
                BITMASK_SET_1(def.options, RigDef::Shock::OPTION_i_INVISIBLE);
            }

            m_used_actor_entry->actor_def->root_module->shocks3.push_back(def);
            break;
        }

        default: // Not a shock
            break;
        }
    }

    // ~~~ Hydros ~~~
    for (hydrobeam_t& hydrobeam: ar_hydros)
    {
        int i = hydrobeam.hb_beam_index;
        const beam_t& beam = ar_beams[i];
        if (beam.bm_type != BEAM_HYDRO)
        {
            continue; // Should never happen.
        }

        UpdateSetBeamDefaults(beam_defaults, this, i);
        UpdateSetInertiaDefaults(inertia_defaults, this, hydrobeam.hb_inertia);

        RigDef::Hydro def;
        def.beam_defaults = beam_defaults;
        def.inertia_defaults = inertia_defaults;
        def.nodes[0] = BuildNodeRef(this, beam.p1->pos);
        def.nodes[1] = BuildNodeRef(this, beam.p2->pos);
        def.lenghtening_factor = hydrobeam.hb_speed;

        // HEADS UP: hydro options have quirks:
        // * 'n' is not 'dummy' like elsewhere, but activates steering wheel input (n = normal)
        // * 'i' makes beam invisible, but (!) also activates 'n' when first in the list. This is an old bug preserved for compatibility.
        // * 'j' makes beam invisible without any side effects (unique to hydros)
        // The exporter never uses 'i', just 'j' and 'n'.

        // individual options
        if (ar_beams_invisible[i])
        {
            BITMASK_SET_1(def.options, RigDef::Hydro::OPTION_j_INVISIBLE);
        }
        if (BITMASK_IS_1(hydrobeam.hb_flags, HYDRO_FLAG_SPEED))
        {
            BITMASK_SET_1(def.options, RigDef::Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED);
        }
        if (BITMASK_IS_1(hydrobeam.hb_flags, HYDRO_FLAG_ELEVATOR))
        {
            BITMASK_SET_1(def.options, RigDef::Hydro::OPTION_e_INPUT_ELEVATOR);
        }
        if (BITMASK_IS_1(hydrobeam.hb_flags, HYDRO_FLAG_RUDDER))
        {
            BITMASK_SET_1(def.options, RigDef::Hydro::OPTION_r_INPUT_RUDDER);
        }
        if (BITMASK_IS_1(hydrobeam.hb_flags, HYDRO_FLAG_AILERON))
        {
            BITMASK_SET_1(def.options, RigDef::Hydro::OPTION_a_INPUT_AILERON);
        }
        if (BITMASK_IS_1(hydrobeam.hb_flags, HYDRO_FLAG_DIR))
        {
            BITMASK_SET_1(def.options, RigDef::Hydro::OPTION_n_INPUT_NORMAL);
        }

        // combined options
        if (BITMASK_IS_1(hydrobeam.hb_flags, HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_ELEVATOR))
        {
            BITMASK_SET_1(def.options, RigDef::Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR);
        }
        if (BITMASK_IS_1(hydrobeam.hb_flags, HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_RUDDER))
        {
            BITMASK_SET_1(def.options, RigDef::Hydro::OPTION_y_INPUT_InvAILERON_RUDDER);
        }
        if (BITMASK_IS_1(hydrobeam.hb_flags, HYDRO_FLAG_REV_ELEVATOR | HYDRO_FLAG_RUDDER))
        {
            BITMASK_SET_1(def.options, RigDef::Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER);
        }

        m_used_actor_entry->actor_def->root_module->hydros.push_back(def);
    }

    // ~~~ Globals (update in-place) ~~~

    m_used_actor_entry->actor_def->root_module->globals[0].dry_mass = ar_dry_mass;
    m_used_actor_entry->actor_def->root_module->globals[0].cargo_mass = ar_load_mass;
}