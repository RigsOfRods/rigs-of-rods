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


///    @file
///    @brief  Vehicle spawning logic.
///    @author Petr Ohlidal
///    @date   12/2013


#include "ActorSpawner.h"

#include "AppContext.h"
#include "AirBrake.h"
#include "Airfoil.h"
#include "Application.h"
#include "ApproxMath.h"
#include "AutoPilot.h"
#include "Actor.h"
#include "ActorManager.h"
#include "BitFlags.h"
#include "Buoyance.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "CmdKeyInertia.h"
#include "Collisions.h"
#include "DashBoardManager.h"
#include "Differentials.h"
#include "EngineSim.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "FlexMesh.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "GameContext.h"
#include "GfxActor.h"
#include "GfxScene.h"
#include "Console.h"
#include "InputEngine.h"
#include "MeshObject.h"
#include "PointColDetector.h"
#include "Renderdash.h"
#include "ScrewProp.h"
#include "Skidmark.h"
#include "SkinFileFormat.h"
#include "SlideNode.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TorqueCurve.h"
#include "TruckValidator.h"
#include "TurboJet.h"
#include "TurboProp.h"
#include "Utils.h"
#include "VehicleAI.h"

#include <OgreMaterialManager.h>
#include <OgreSceneManager.h>
#include <OgreMovableObject.h>
#include <OgreParticleSystem.h>
#include <OgreEntity.h>
#include <climits>
#include <fmt/core.h>

const char* ACTOR_ID_TOKEN = "@Actor_"; // Appended to material name, followed by actor ID (aka 'trucknum')

using namespace RoR;

/* -------------------------------------------------------------------------- */
/* Prepare for loading
/* -------------------------------------------------------------------------- */

void ActorSpawner::Setup(
    Actor *rig,
    Truck::DocumentPtr file,
    Ogre::SceneNode *parent,
    Ogre::Vector3 const & spawn_position
)
{
    m_actor = rig;
    m_file = file;
    m_particles_parent_scenenode = parent;
    m_spawn_position = spawn_position;
    m_current_keyword = Truck::KEYWORD_NONE;
    m_wing_area = 0.f;
    m_fuse_z_min = 1000.0f;
    m_fuse_z_max = -1000.0f;
    m_fuse_y_min = 1000.0f;
    m_fuse_y_max = -1000.0f;
    m_first_wing_index = -1;
    m_driverseat_prop_index = -1;
    m_oldstyle_renderdash = nullptr;

    App::GetCacheSystem()->CheckResourceLoaded(m_actor->ar_filename, m_custom_resource_group);
}

void ActorSpawner::CalcMemoryRequirements(ActorMemoryRequirements& req, Truck::Module* module_def)
{
    // 'nodes'
    req.num_nodes += module_def->nodes.size();
    for (auto& def: module_def->nodes)
    {
        // FIXME: what if user defines the 'h' flag in 'set_node_defaults'? Old parser was fine with pre-defined beam array, we have a problem.
        if (def.options.find(Truck::Node::OPTION_h_HOOK_POINT) != std::string::npos)
        {
            req.num_beams += 1;
        }
    }

    // 'beams'
    req.num_beams += module_def->beams.size();

    // 'ties'
    req.num_beams += module_def->ties.size();

    // 'ropes'
    req.num_beams += module_def->ropes.size();

    // 'hydros'
    req.num_beams += module_def->hydros.size();

    // 'triggers'
    req.num_beams  += module_def->triggers.size();
    req.num_shocks += module_def->triggers.size();

    // 'animators'
    req.num_beams += module_def->animators.size();

    // 'cinecam'
    req.num_nodes += module_def->cinecam.size();
    req.num_beams += module_def->cinecam.size() * 8;

    // 'shocks', 'shocks2', 'shocks3'
    req.num_beams  += module_def->shocks.size();
    req.num_shocks += module_def->shocks.size();
    req.num_beams  += module_def->shocks_2.size();
    req.num_shocks += module_def->shocks_2.size();
    req.num_beams  += module_def->shocks_3.size();
    req.num_shocks += module_def->shocks_3.size();

    // 'commands' and 'commands2' (unified)
    req.num_beams += module_def->commands_2.size();

    // 'rotators'
    req.num_rotators += module_def->rotators.size();
    req.num_rotators += module_def->rotators_2.size();

    // 'wings'
    req.num_wings += module_def->wings.size();

    // 'wheels'
    for (Truck::Wheel& wheel: module_def->wheels)
    {
        req.num_nodes += wheel.num_rays * 2; // BuildWheelObjectAndNodes()
        req.num_beams += wheel.num_rays * ((wheel.rigidity_node.IsValidAnyState()) ? 9 : 8); // BuildWheelBeams()
    }

    // 'wheels2'
    for (Truck::Wheel2& wheel2: module_def->wheels_2)
    {
        req.num_nodes += wheel2.num_rays * 4;
        // Rim beams:  num_rays*10 (*11 with valid rigidity_node)
        // Tyre beams: num_rays*14
        req.num_beams += wheel2.num_rays * ((wheel2.rigidity_node.IsValidAnyState()) ? 25 : 24);
    }

    // 'meshwheels' & 'meshwheels2' (unified)
    for (Truck::MeshWheel& meshwheel: module_def->mesh_wheels)
    {
        req.num_nodes += meshwheel.num_rays * 2; // BuildWheelObjectAndNodes()
        req.num_beams += meshwheel.num_rays * ((meshwheel.rigidity_node.IsValidAnyState()) ? 9 : 8); // BuildWheelBeams()
    }

    // 'flexbodywheels'
    for (Truck::FlexBodyWheel& flexwheel: module_def->flex_body_wheels)
    {
        req.num_nodes += flexwheel.num_rays * 4;
        // Rim beams:      num_rays*8
        // Tyre beams:     num_rays*10 (num_rays*11 with valid rigidity_node)
        // Support beams:  num_rays*2
        req.num_beams += flexwheel.num_rays * ((flexwheel.rigidity_node.IsValidAnyState()) ? 21 : 20);
    }

    // 'airbrakes'
    req.num_airbrakes += module_def->airbrakes.size();

    // 'fixes'
    req.num_fixes += module_def->fixes.size();
}

void ActorSpawner::InitializeRig()
{
    ActorMemoryRequirements & req = m_memory_requirements;
    for (auto module: m_selected_modules) // _Root_ module is included
    {
        this->CalcMemoryRequirements(req, module.get());
    }

    // Allocate memory as needed
    m_actor->ar_beams = new beam_t[req.num_beams];
    m_actor->ar_nodes = new node_t[req.num_nodes];
    m_actor->ar_nodes_id = new int[req.num_nodes];
    for (size_t i = 0; i < req.num_nodes; ++i)
    {
        m_actor->ar_nodes_id[i] = -1;
    }
    m_actor->ar_nodes_name = new std::string[req.num_nodes];

    if (req.num_shocks > 0)
        m_actor->ar_shocks = new shock_t[req.num_shocks];

    if (req.num_rotators > 0)
        m_actor->ar_rotators = new rotator_t[req.num_rotators];

    if (req.num_wings > 0)
        m_actor->ar_wings = new wing_t[req.num_wings];

    // Values of 'set_default_minimass', or -1 where global minimass from 'minimass' should be filled later.
    m_actor->ar_minimass.resize(req.num_nodes, -1.f);

    // commands contain complex data structures, do not memset them ...
    for (int i=0;i<MAX_COMMANDS+1;i++)
    {
        m_actor->ar_command_key[i].commandValue=0;
        m_actor->ar_command_key[i].beams.clear();
        m_actor->ar_command_key[i].rotators.clear();
        m_actor->ar_command_key[i].description="";
    }

    m_actor->exhausts.clear();
    memset(m_actor->ar_custom_particles, 0, sizeof(CParticle) * MAX_CPARTICLES);
    m_actor->ar_num_custom_particles = 0;
    memset(m_actor->ar_soundsources, 0, sizeof(soundsource_t) * MAX_SOUNDSCRIPTS_PER_TRUCK);
    m_actor->ar_num_soundsources = 0;
    memset(m_actor->ar_collcabs, 0, sizeof(int) * MAX_CABS);
    memset(m_actor->ar_inter_collcabrate, 0, sizeof(collcab_rate_t) * MAX_CABS);
    m_actor->ar_num_collcabs = 0;
    memset(m_actor->ar_intra_collcabrate, 0, sizeof(collcab_rate_t) * MAX_CABS);
    memset(m_actor->ar_buoycabs, 0, sizeof(int) * MAX_CABS);
    m_actor->ar_num_buoycabs = 0;
    memset(m_actor->ar_buoycab_types, 0, sizeof(int) * MAX_CABS);
    memset(m_actor->m_skid_trails, 0, sizeof(Skidmark *) * (MAX_WHEELS*2));
    m_actor->description.clear();

    m_actor->ar_extern_camera_mode=0;
    m_actor->authors.clear();

    m_actor->m_odometer_total = 0;
    m_actor->m_odometer_user  = 0;

    m_actor->m_masscount=0;
    m_actor->m_disable_smoke = App::gfx_particles_mode->GetInt() == 0;
    m_actor->m_beam_break_debug_enabled  = App::diag_log_beam_break->GetBool();
    m_actor->m_beam_deform_debug_enabled = App::diag_log_beam_deform->GetBool();
    m_actor->m_trigger_debug_enabled    = App::diag_log_beam_trigger->GetBool();
    m_actor->ar_origin=Ogre::Vector3::ZERO;
    m_actor->m_slidenodes.clear();
    m_actor->ar_num_cinecams=0;
    m_actor->m_deletion_scene_nodes.clear();
    m_actor->m_net_custom_lights[0] = UINT_MAX;
    m_actor->m_net_custom_lights[1] = UINT_MAX;
    m_actor->m_net_custom_lights[2] = UINT_MAX;
    m_actor->m_net_custom_lights[3] = UINT_MAX;
    m_actor->m_net_custom_light_count = 0;

    m_actor->ar_sim_state = Actor::SimState::LOCAL_SLEEPING;
    m_actor->m_fusealge_airfoil = nullptr;
    m_actor->m_fusealge_front = nullptr;
    m_actor->m_fusealge_back = nullptr;
    m_actor->m_fusealge_width=0;
    m_actor->ar_brake_force=30000.0;
    m_actor->m_handbrake_force = 2 * m_actor->ar_brake_force;

    m_actor->m_num_proped_wheels=0;

    m_actor->ar_speedo_max_kph=140;
    m_actor->ar_num_cameras=0;
    for (int i = 0; i < MAX_CAMERAS; ++i)
    {
        m_actor->ar_camera_node_pos [i] = node_t::INVALID_IDX;
        m_actor->ar_camera_node_dir [i] = node_t::INVALID_IDX;
        m_actor->ar_camera_node_roll[i] = node_t::INVALID_IDX;
    }

#ifdef USE_ANGELSCRIPT
    m_actor->ar_vehicle_ai = new VehicleAI(m_actor);
#endif // USE_ANGELSCRIPT

    m_actor->ar_airbrake_intensity = 0;
    m_actor->alb_minspeed = 0.0f;
    m_actor->alb_mode = false;
    m_actor->alb_nodash = true;
    m_actor->alb_notoggle = false;
    m_actor->alb_pulse_time = 2000.0f;
    m_actor->alb_pulse_state = false;
    m_actor->alb_ratio = 1.0f;
    m_actor->alb_timer = 0.0f;
    m_actor->ar_anim_shift_timer = 0.0f;

    m_actor->cc_mode = false;
    m_actor->cc_can_brake = false;
    m_actor->cc_target_rpm = 0.0f;
    m_actor->cc_target_speed = 0.0f;
    m_actor->cc_target_speed_lower_limit = 0.0f;

    m_actor->ar_collision_relevant = false;

    m_actor->ar_hide_in_actor_list = false;

    m_actor->ar_anim_previous_crank = 0.f;

    m_actor->sl_enabled = false;
    m_actor->sl_speed_limit = 0.f;

    m_actor->tc_mode = false;
    m_actor->tc_nodash = true;
    m_actor->tc_notoggle = false;
    m_actor->tc_pulse_time = 2000.0f;
    m_actor->tc_pulse_state = false;
    m_actor->tc_ratio = 1.f;
    m_actor->tc_timer = 0.f;

    m_actor->ar_dashboard = new DashBoardManager();

    /* Collisions */

    if (!App::sim_no_collisions->GetBool())
    {
        m_actor->m_inter_point_col_detector = new PointColDetector(m_actor);
    }

    if (!App::sim_no_self_collisions->GetBool())
    {
        m_actor->m_intra_point_col_detector = new PointColDetector(m_actor);
    }

    m_actor->ar_submesh_ground_model = App::GetSimTerrain()->GetCollisions()->defaultgm;

    // Lights mode
    m_actor->m_flares_mode = App::gfx_flares_mode->GetEnum<GfxFlaresMode>();

    m_actor->m_definition = m_file;

    m_flex_factory = RoR::FlexFactory(this);

    m_flex_factory.CheckAndLoadFlexbodyCache();

    m_placeholder_managedmat = Ogre::MaterialManager::getSingleton().getByName("rigsofrods/managedmaterial-placeholder"); // Built-in

    m_apply_simple_materials = App::diag_simple_materials->GetBool();
    if (m_apply_simple_materials)
    {
        m_simple_material_base = Ogre::MaterialManager::getSingleton().getByName("tracks/simple"); // Built-in material
        if (m_simple_material_base.isNull())
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
                "Failed to load built-in material 'tracks/simple'; disabling 'SimpleMaterials'");
            m_apply_simple_materials = false;
        }
    }

    m_curr_mirror_prop_type = CustomMaterial::MirrorPropType::MPROP_NONE;
    m_curr_mirror_prop_scenenode = nullptr;

    m_generate_wing_position_lights = true;
    for (auto module: m_selected_modules)
    {
        if (module->engine.size() > 0) // Engine present => it's a land vehicle.
            m_generate_wing_position_lights = false; // Disable aerial pos. lights for land vehicles.
    }
}

void ActorSpawner::FinalizeRig()
{
    m_actor->m_dry_mass = m_state.truckmass;
    m_actor->m_load_mass = m_state.loadmass;

    // we should post-process the torque curve if existing
    if (m_actor->ar_engine)
    {
        int result = m_actor->ar_engine->getTorqueCurve()->spaceCurveEvenly(m_actor->ar_engine->getTorqueCurve()->getUsedSpline());
        if (result)
        {
            m_actor->ar_engine->getTorqueCurve()->setTorqueModel("default");
            if (result == 1)
            {
                AddMessage(Message::TYPE_ERROR, "TorqueCurve: Points (rpm) must be in an ascending order. Using default curve");
            }
        }

        //Gearbox
        m_actor->ar_engine->SetAutoMode(App::sim_gearbox_mode->GetEnum<SimGearboxMode>());
    }
    
    // Sanitize trigger_cmdshort and trigger_cmdlong
    for (int i=0; i<m_actor->ar_num_beams; i++)
    {
        shock_t* shock = m_actor->ar_beams[i].shock;
        if (shock && ((shock->flags & SHOCK_FLAG_TRG_BLOCKER) || (shock->flags & SHOCK_FLAG_TRG_BLOCKER_A)))
        {
            shock->trigger_cmdshort = std::min(shock->trigger_cmdshort, m_actor->ar_num_beams - i - 1);
            shock->trigger_cmdlong  = std::min(shock->trigger_cmdlong , m_actor->ar_num_beams - i - 1);
        }
    }

    //calculate gwps height offset
    //get a starting value
    m_actor->ar_posnode_spawn_height=m_actor->ar_nodes[0].RelPosition.y;
    //start at 0 to avoid a crash whith a 1-node truck
    for (int i=0; i<m_actor->ar_num_nodes; i++)
    {
        // scan and store the y-coord for the lowest node of the truck
        if (m_actor->ar_nodes[i].RelPosition.y <= m_actor->ar_posnode_spawn_height)
        {
            m_actor->ar_posnode_spawn_height = m_actor->ar_nodes[i].RelPosition.y;
        }
    }

    m_actor->ar_main_camera_node_pos  = std::max((NodeIdx_t)0, m_actor->ar_camera_node_pos[0]);
    m_actor->ar_main_camera_node_dir  = std::max((NodeIdx_t)0, m_actor->ar_camera_node_dir[0]);
    m_actor->ar_main_camera_node_roll = std::max((NodeIdx_t)0, m_actor->ar_camera_node_roll[0]);
    
    m_actor->m_has_axles_section = m_actor->m_num_wheel_diffs > 0;

    // Fill global 'minimass' value where 'set_default_minimass' value wasn't set.
    for (int i = 0; i < m_actor->ar_num_nodes; i++)
    {
        if (m_actor->ar_minimass[i] == -1.f)
            m_actor->ar_minimass[i] = m_state.global_minimass;
    }

    // Calculate mass of each wheel (without rim)
    for (int i = 0; i < m_actor->ar_num_wheels; i++)
    {
        m_actor->ar_wheels[i].wh_mass = 0.0f;
        for (int j = 0; j < m_actor->ar_wheels[i].wh_num_nodes; j++)
        {
            m_actor->ar_wheels[i].wh_mass += m_actor->ar_wheels[i].wh_nodes[j]->mass;
        }
    }

    if (m_actor->m_num_proped_wheels > 0)
    {
        float proped_wheels_radius_sum = 0.0f;
        for (int i = 0; i < m_actor->ar_num_wheels; i++)
        {
            if (m_actor->ar_wheels[i].wh_propulsed > 0)
            {
                proped_wheels_radius_sum += m_actor->ar_wheels[i].wh_radius;
            }
        }
        m_actor->m_avg_proped_wheel_radius = proped_wheels_radius_sum / m_actor->m_num_proped_wheels;
    }

    // Automatically build interwheel differentials from proped wheel pairs
    if (m_actor->m_num_wheel_diffs == 0)
    {
        for (int i = 1; i < m_actor->m_num_proped_wheels; i++)
        {
            if (i % 2)
            {
                Differential *diff = new Differential();

                diff->di_idx_1 = m_actor->m_proped_wheel_pairs[i - 1];
                diff->di_idx_2 = m_actor->m_proped_wheel_pairs[i - 0];

                diff->AddDifferentialType(VISCOUS_DIFF);

                m_actor->m_wheel_diffs[m_actor->m_num_wheel_diffs] = diff;
                m_actor->m_num_wheel_diffs++;
            }
        }
    }

    // Automatically build interaxle differentials from interwheel differentials pairs
    if (m_actor->m_num_axle_diffs == 0)
    {
        for (int i = 1; i < m_actor->m_num_wheel_diffs; i++)
        {
            if (m_actor->m_transfer_case)
            {
                int a1 = std::min(m_actor->m_transfer_case->tr_ax_1, m_actor->m_transfer_case->tr_ax_2);
                int a2 = std::max(m_actor->m_transfer_case->tr_ax_1, m_actor->m_transfer_case->tr_ax_2);
                if ((a1 == i - 1) && (a2 == i - 0))
                    continue;
            }

            Differential *diff = new Differential();

            diff->di_idx_1 = i - 1;
            diff->di_idx_2 = i - 0;

            if (m_actor->m_has_axles_section)
                diff->AddDifferentialType(LOCKED_DIFF);
            else
                diff->AddDifferentialType(VISCOUS_DIFF);

            m_actor->m_axle_diffs[m_actor->m_num_axle_diffs] = diff;
            m_actor->m_num_axle_diffs++;
        }
    }

    // Automatically build an additional interaxle differential for the transfer case
    if (m_actor->m_transfer_case && m_actor->m_transfer_case->tr_ax_2 >= 0)
    {
        Differential *diff = new Differential();
        diff->di_idx_1 = m_actor->m_transfer_case->tr_ax_1;
        diff->di_idx_2 = m_actor->m_transfer_case->tr_ax_2;
        diff->AddDifferentialType(LOCKED_DIFF);
        m_actor->m_axle_diffs[m_actor->m_num_axle_diffs] = diff;
    }

    if (m_actor->ar_main_camera_node_dir == 0 || m_actor->ar_main_camera_node_dir == node_t::INVALID_IDX)
    {
        Ogre::Vector3 ref = m_actor->ar_nodes[m_actor->ar_main_camera_node_pos].RelPosition;
        // Step 1: Find a suitable camera node dir
        float max_dist = 0.0f;
        NodeIdx_t furthest_node = 0;
        for (int i = 0; i < m_actor->ar_num_nodes; i++)
        {
            float dist = m_actor->ar_nodes[i].RelPosition.squaredDistance(ref);
            if (dist > max_dist)
            {
                max_dist = dist;
                furthest_node = (NodeIdx_t)i;
            }
        }
        m_actor->ar_main_camera_node_dir = furthest_node;
        // Step 2: Correct the misalignment
        Ogre::Vector3 dir = m_actor->ar_nodes[furthest_node].RelPosition - ref;
        float offset = atan2(dir.dotProduct(Ogre::Vector3::UNIT_Z), dir.dotProduct(Ogre::Vector3::UNIT_X));
        m_actor->ar_main_camera_dir_corr = Ogre::Quaternion(Ogre::Radian(offset), Ogre::Vector3::UNIT_Y);
    }

    if (m_actor->ar_camera_node_pos[0] != node_t::INVALID_IDX)
    {
        // store the y-difference between the trucks lowest node and the campos-node for the gwps system
        m_actor->ar_posnode_spawn_height = m_actor->ar_nodes[m_actor->ar_camera_node_pos[0]].RelPosition.y - m_actor->ar_posnode_spawn_height;
    } 
    else
    {
        //this can not be an airplane, just set it to 0.
        m_actor->ar_posnode_spawn_height = 0.0f;
    }

    //cameras workaround
    for (int i=0; i<m_actor->ar_num_cameras; i++)
    {
        //LogManager::getSingleton().logMessage("Camera dir="+StringConverter::toString(ar_nodes[ar_camera_node_dir[i]].RelPosition-ar_nodes[ar_camera_node_pos[i]].RelPosition)+" roll="+StringConverter::toString(ar_nodes[ar_camera_node_roll[i]].RelPosition-ar_nodes[ar_camera_node_pos[i]].RelPosition));
        Ogre::Vector3 dir_node_offset = GetNode(m_actor->ar_camera_node_dir[i]).RelPosition - GetNode(m_actor->ar_camera_node_pos[i]).RelPosition;
        Ogre::Vector3 roll_node_offset = GetNode(m_actor->ar_camera_node_roll[i]).RelPosition - GetNode(m_actor->ar_camera_node_pos[i]).RelPosition;
        Ogre::Vector3 cross = dir_node_offset.crossProduct(roll_node_offset);
        
        m_actor->ar_camera_node_roll_inv[i]=cross.y > 0;//(ar_nodes[ar_camera_node_dir[i]].RelPosition-ar_nodes[ar_camera_node_pos[i]].RelPosition).crossProduct(ar_nodes[ar_camera_node_roll[i]].RelPosition-ar_nodes[ar_camera_node_pos[i]].RelPosition).y>0;
        if (m_actor->ar_camera_node_roll_inv[i])
        {
            AddMessage(Message::TYPE_WARNING, "camera definition is probably invalid and has been corrected. It should be center, back, left");
        }
    }
    
    //wing closure
    if (m_first_wing_index!=-1)
    {
        if (m_actor->ar_autopilot != nullptr) 
        {
            m_actor->ar_autopilot->setInertialReferences(
                & GetNode(m_airplane_left_light),
                & GetNode(m_airplane_right_light),
                m_actor->m_fusealge_back,
                & GetNode(m_actor->ar_camera_node_pos[0])
                );
        }
        //inform wing segments
        float span=GetNode(m_actor->ar_wings[m_first_wing_index].fa->nfrd).RelPosition.distance(GetNode(m_actor->ar_wings[m_actor->ar_num_wings-1].fa->nfld).RelPosition);
        
        m_actor->ar_wings[m_first_wing_index].fa->enableInducedDrag(span,m_wing_area, false);
        m_actor->ar_wings[m_actor->ar_num_wings-1].fa->enableInducedDrag(span,m_wing_area, true);
        //wash calculator
        WashCalculator();
    }

    this->UpdateCollcabContacterNodes();

    m_flex_factory.SaveFlexbodiesToCache();

    m_actor->GetGfxActor()->SortFlexbodies();
}

/* -------------------------------------------------------------------------- */
/* Actual loading
/* ~~~ Implemented in ActorSpawner_ProcessControl.cpp!
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* Processing functions and utilities.
/* -------------------------------------------------------------------------- */

void ActorSpawner::WashCalculator()
{
    //we will compute wash
    int w,p;
    for (p=0; p<m_actor->ar_num_aeroengines; p++)
    {
        Ogre::Vector3 prop=m_actor->ar_nodes[m_actor->ar_aeroengines[p]->getNoderef()].RelPosition;
        float radius=m_actor->ar_aeroengines[p]->getRadius();
        for (w=0; w<m_actor->ar_num_wings; w++)
        {
            //left wash
            Ogre::Vector3 wcent=((m_actor->ar_nodes[m_actor->ar_wings[w].fa->nfld].RelPosition+m_actor->ar_nodes[m_actor->ar_wings[w].fa->nfrd].RelPosition)/2.0);
            //check if wing is near enough along X (less than 15m back)
            if (wcent.x>prop.x && wcent.x<prop.x+15.0)
            {
                //check if it's okay vertically
                if (wcent.y>prop.y-radius && wcent.y<prop.y+radius)
                {
                    //okay, compute wash coverage ratio along Z
                    float wleft=(m_actor->ar_nodes[m_actor->ar_wings[w].fa->nfld].RelPosition).z;
                    float wright=(m_actor->ar_nodes[m_actor->ar_wings[w].fa->nfrd].RelPosition).z;
                    float pleft=prop.z+radius;
                    float pright=prop.z-radius;
                    float aleft=wleft;
                    if (pleft<aleft) aleft=pleft;
                    float aright=wright;
                    if (pright>aright) aright=pright;
                    if (aright<aleft)
                    {
                        //we have a wash
                        float wratio=(aleft-aright)/(wleft-wright);
                        m_actor->ar_wings[w].fa->addwash(p, wratio);
                        Ogre::String msg = "Wing "+TOSTRING(w)+" is washed by prop "+TOSTRING(p)+" at "+TOSTRING((float)(wratio*100.0))+"%";
                        AddMessage(Message::TYPE_INFO, msg);
                    }
                }
            }
        }
    }
}

void ActorSpawner::ProcessTurbojet(Truck::Turbojet & def)
{
    NodeIdx_t front,back,ref;
    front = GetNodeIndexOrThrow(def.front_node);
    back  = GetNodeIndexOrThrow(def.back_node);
    ref   = GetNodeIndexOrThrow(def.side_node);

    Turbojet *tj = new Turbojet(m_actor, front, back, ref, def);

    // Visuals
    std::string nozzle_name = this->ComposeName("TurbojetNozzle", m_actor->ar_num_aeroengines);
    Ogre::Entity* nozzle_ent = App::GetGfxScene()->GetSceneManager()->createEntity(nozzle_name, "nozzle.mesh", m_custom_resource_group);
    this->SetupNewEntity(nozzle_ent, Ogre::ColourValue(1, 0.5, 0.5));
    Ogre::Entity* afterburn_ent = nullptr;
    if (def.wet_thrust > 0.f)
    {
        std::string flame_name = this->ComposeName("AfterburnerFlame", m_actor->ar_num_aeroengines);
        afterburn_ent = App::GetGfxScene()->GetSceneManager()->createEntity(flame_name, "abflame.mesh", m_custom_resource_group);
        this->SetupNewEntity(afterburn_ent, Ogre::ColourValue(1, 1, 0));
    }
    std::string propname = this->ComposeName("Turbojet", m_actor->ar_num_aeroengines);
    tj->tjet_visual.SetNodes(front, back, ref);
    tj->tjet_visual.SetupVisuals(def, m_actor->ar_num_aeroengines,
        propname, nozzle_ent, afterburn_ent, m_actor->m_disable_smoke);

    m_actor->ar_aeroengines[m_actor->ar_num_aeroengines]=tj;
    m_actor->ar_driveable=AIRPLANE;
    if (m_actor->ar_autopilot == nullptr && m_actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
    {
        m_actor->ar_autopilot=new Autopilot(m_actor->ar_instance_id);
    }

    m_actor->ar_num_aeroengines++;
}

std::string ActorSpawner::ComposeName(const char* type, int number)
{
    char buf[500];
    snprintf(buf, 500, "%s_%d%s%d", type, number, ACTOR_ID_TOKEN, m_actor->ar_instance_id);
    return buf;
}

// static
void ActorSpawner::ComposeName(RoR::Str<100>& str, const char* type, int number, int actor_id)
{
    str.Clear();
    str << type << "_" << number << ACTOR_ID_TOKEN << actor_id;
}

void ActorSpawner::ProcessScrewprop(Truck::Screwprop & def)
{
    if (! CheckScrewpropLimit(1))
    {
        return;
    }

    m_actor->ar_screwprops[m_actor->ar_num_screwprops] = new Screwprop(
        m_actor,
        GetNodeIndexOrThrow(def.prop_node),
        GetNodeIndexOrThrow(def.back_node),
        GetNodeIndexOrThrow(def.top_node),
        def.power
    );
    m_actor->ar_driveable=BOAT;
    m_actor->ar_num_screwprops++;
}

void ActorSpawner::ProcessFusedrag(Truck::Fusedrag & def)
{
    //parse fusedrag
    NodeIdx_t front_node_idx = GetNodeIndexOrThrow(def.front_node);
    float width = 1.f;
    float factor = 1.f;
    char fusefoil[256];
    strncpy(fusefoil, def.airfoil_name.c_str(), 255);

    if (def.autocalc)
    {
        // fusedrag autocalculation

        // calculate fusedrag by truck size
        factor = def.area_coefficient;
        width  =  (m_fuse_z_max - m_fuse_z_min) * (m_fuse_y_max - m_fuse_y_min) * factor;

        m_actor->m_fusealge_airfoil = new Airfoil(fusefoil);

        m_actor->m_fusealge_front   = & GetNode(front_node_idx);
        m_actor->m_fusealge_back    = & GetNode(front_node_idx); // This equals v0.38 / v0.4.0.7, but it's probably a bug
        m_actor->m_fusealge_width   = width;
        AddMessage(Message::TYPE_INFO, "Fusedrag autocalculation size: "+TOSTRING(width)+" m^2");
    } 
    else
    {
        // original fusedrag calculation

        width  = def.approximate_width;

        m_actor->m_fusealge_airfoil = new Airfoil(fusefoil);

        m_actor->m_fusealge_front   = & GetNode(front_node_idx);
        m_actor->m_fusealge_back    = & GetNode(front_node_idx); // This equals v0.38 / v0.4.0.7, but it's probably a bug
        m_actor->m_fusealge_width   = width;
    }
}

void ActorSpawner::BuildAeroEngine(
    NodeIdx_t ref_node_index,
    NodeIdx_t back_node_index,
    NodeIdx_t blade_1_node_index,
    NodeIdx_t blade_2_node_index,
    NodeIdx_t blade_3_node_index,
    NodeIdx_t blade_4_node_index,
    NodeIdx_t couplenode_index,
    bool is_turboprops,
    Ogre::String const & airfoil,
    float power,
    float pitch
    )
{
    int aeroengine_index = m_actor->ar_num_aeroengines;

    Turboprop *turbo_prop = new Turboprop(
        m_actor, 
        ref_node_index,
        back_node_index,
        blade_1_node_index,
        blade_2_node_index,
        blade_3_node_index,
        blade_4_node_index,
        couplenode_index,
        power,
        airfoil,
        m_actor->m_disable_smoke,
        ! is_turboprops,
        pitch
    );

    m_actor->ar_aeroengines[m_actor->ar_num_aeroengines] = turbo_prop;
    m_actor->ar_num_aeroengines++;
    m_actor->ar_driveable = AIRPLANE;

    /* Autopilot */
    if (m_actor->ar_autopilot == nullptr && m_actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
    {
        m_actor->ar_autopilot = new Autopilot(m_actor->ar_instance_id);
    }

    /* Visuals */
    float scale = GetNode(ref_node_index).RelPosition.distance(GetNode(blade_1_node_index).RelPosition) / 2.25f;
    for (RoR::Prop& prop: m_props)
    {
        if ((prop.pp_node_ref == ref_node_index) && (prop.pp_aero_propeller_blade || prop.pp_aero_propeller_spin))
        {
            prop.pp_scene_node->scale(scale, scale, scale);
            prop.pp_aero_engine_idx = aeroengine_index;
        }
    }
}

void ActorSpawner::ProcessTurboprop2(Truck::Turboprop2 & def)
{
    const NodeIdx_t p3_node_index = (def.blade_tip_nodes[2].IsValidAnyState()) ? GetNodeIndexOrThrow(def.blade_tip_nodes[2]) : -1;
    const NodeIdx_t p4_node_index = (def.blade_tip_nodes[3].IsValidAnyState()) ? GetNodeIndexOrThrow(def.blade_tip_nodes[3]) : -1;
    const NodeIdx_t couple_node_index = (def.couple_node.IsValidAnyState()) ? GetNodeIndexOrThrow(def.couple_node) : -1;

    BuildAeroEngine(
        GetNodeIndexOrThrow(def.reference_node),
        GetNodeIndexOrThrow(def.axis_node),
        GetNodeIndexOrThrow(def.blade_tip_nodes[0]),
        GetNodeIndexOrThrow(def.blade_tip_nodes[1]),
        p3_node_index,
        p4_node_index,
        couple_node_index,
        true,
        def.airfoil,
        def.turbine_power_kW,
        -10
    );
}

void ActorSpawner::ProcessPistonprop(Truck::Pistonprop & def)
{
    const NodeIdx_t p3_node_index = (def.blade_tip_nodes[2].IsValidAnyState()) ? GetNodeIndexOrThrow(def.blade_tip_nodes[2]) : -1;
    const NodeIdx_t p4_node_index = (def.blade_tip_nodes[3].IsValidAnyState()) ? GetNodeIndexOrThrow(def.blade_tip_nodes[3]) : -1;
    const NodeIdx_t couple_node_index = (def.couple_node.IsValidAnyState()) ? GetNodeIndexOrThrow(def.couple_node) : -1;

    BuildAeroEngine(
        GetNodeIndexOrThrow(def.reference_node),
        GetNodeIndexOrThrow(def.axis_node),
        GetNodeIndexOrThrow(def.blade_tip_nodes[0]),
        GetNodeIndexOrThrow(def.blade_tip_nodes[1]),
        p3_node_index,
        p4_node_index,
        couple_node_index,
        false,
        def.airfoil,
        def.turbine_power_kW,
        def.pitch
    );
}

void ActorSpawner::EvaluateSectionConfig()
{
    for (Truck::ModulePtr m: m_file->modules)
    {
        if (!m->defined_explicitly || m->sectionconfigs.size() == 0)
        {
            m_selected_modules.push_back(m);
        }
        else
        {
            for (std::string const& config: m->sectionconfigs)
            {
                if (config == m_selected_config)
                {
                    m_selected_modules.push_back(m);
                    break;
                }
            }
        }
    }
}

void ActorSpawner::ValidateSectionConfig()
{
    Truck::Validator v;
    v.Setup(m_file);
    for (Truck::ModulePtr& modul: m_selected_modules)
    {
        v.AddModule(modul);
    }
    v.Validate(); // Reports issues to console
}

void ActorSpawner::ProcessAirbrake(Truck::Airbrake & def)
{
    const int airbrake_idx = static_cast<int>(m_actor->ar_airbrakes.size());
    m_actor->ar_airbrakes.push_back(new Airbrake(
        m_actor,
        this->ComposeName("Airbrake", airbrake_idx).c_str(),
        airbrake_idx,
        GetNodePointerOrThrow(def.reference_node),
        GetNodePointerOrThrow(def.x_axis_node),
        GetNodePointerOrThrow(def.y_axis_node),
        GetNodePointerOrThrow(def.aditional_node),
        def.offset,
        def.width,
        def.height,
        def.max_inclination_angle,
        m_cab_material_name,
        def.texcoord_x1,
        def.texcoord_y1,
        def.texcoord_x2,
        def.texcoord_y2,
        def.lift_coefficient
    ));
}

void ActorSpawner::ProcessWing(Truck::Wing & def)
{
    if ((m_first_wing_index != -1) && (m_actor->ar_wings[m_actor->ar_num_wings - 1].fa == nullptr))
    {
        this->AddMessage(Message::TYPE_ERROR, "Unable to process wing, previous wing has no Airfoil");
        return;
    }

    if (def.airfoil == "") // May happen for malformed `wings` entry in truckfile
    {
        this->AddMessage(Message::TYPE_ERROR, "Unable to process wing, no Airfoil defined");
        return;
    }

    NodeIdx_t node1 = this->GetNodeIndexOrThrow(def.nodes[1]);

    const std::string wing_name = this->ComposeName("Wing", m_actor->ar_num_wings);
    auto flex_airfoil = new FlexAirfoil(
        wing_name,
        m_actor,
        this->GetNodeIndexOrThrow(def.nodes[0]),
        node1,
        this->GetNodeIndexOrThrow(def.nodes[2]),
        this->GetNodeIndexOrThrow(def.nodes[3]),
        this->GetNodeIndexOrThrow(def.nodes[4]),
        this->GetNodeIndexOrThrow(def.nodes[5]),
        this->GetNodeIndexOrThrow(def.nodes[6]),
        this->GetNodeIndexOrThrow(def.nodes[7]),
        m_cab_material_name,
        Ogre::Vector2(def.tex_coords[0], def.tex_coords[1]),
        Ogre::Vector2(def.tex_coords[2], def.tex_coords[3]),
        Ogre::Vector2(def.tex_coords[4], def.tex_coords[5]),
        Ogre::Vector2(def.tex_coords[6], def.tex_coords[7]),
        def.control_surface,
        def.chord_point,
        def.min_deflection,
        def.max_deflection,
        def.airfoil,
        def.efficacy_coef,
        m_actor->ar_sim_state != Actor::SimState::NETWORKED_OK
    );

    Ogre::Entity* entity = nullptr;
    try
    {
        const std::string wing_instance_name = this->ComposeName("WingEntity", m_actor->ar_num_wings);
        entity = App::GetGfxScene()->GetSceneManager()->createEntity(wing_instance_name, wing_name);
        m_actor->m_deletion_entities.emplace_back(entity);
        this->SetupNewEntity(entity, Ogre::ColourValue(0.5, 1, 0));
    }
    catch (...)
    {
        this->AddMessage(Message::TYPE_ERROR, std::string("Failed to load mesh (flexbody wing): ") + wing_name);
        delete flex_airfoil;
        return;
    }

    // induced drag
    if (m_first_wing_index == -1)
    {
        m_first_wing_index = m_actor->ar_num_wings;
        m_wing_area=ComputeWingArea(
            this->GetNode(flex_airfoil->nfld).AbsPosition,    this->GetNode(flex_airfoil->nfrd).AbsPosition,
            this->GetNode(flex_airfoil->nbld).AbsPosition,    this->GetNode(flex_airfoil->nbrd).AbsPosition
        );
    }
    else
    {
        wing_t & previous_wing = m_actor->ar_wings[m_actor->ar_num_wings - 1];

        if (node1 != previous_wing.fa->nfld)
        {
            wing_t & start_wing    = m_actor->ar_wings[m_first_wing_index];

            //discontinuity
            //inform wing segments
            float span = GetNode( start_wing.fa->nfrd).RelPosition.distance( GetNode(previous_wing.fa->nfld).RelPosition );
            
            start_wing.fa->enableInducedDrag(span, m_wing_area, false);
            previous_wing.fa->enableInducedDrag(span, m_wing_area, true);

            //we want also to add positional lights for first wing
            if (m_generate_wing_position_lights && (m_actor->m_flares_mode != GfxFlaresMode::NONE))
            {
                //Left green
                m_airplane_left_light=previous_wing.fa->nfld;
                RoR::Prop left_green_prop;

                left_green_prop.pp_node_ref=previous_wing.fa->nfld;
                left_green_prop.pp_node_x=previous_wing.fa->nflu;
                left_green_prop.pp_node_y=previous_wing.fa->nfld; //ignored
                left_green_prop.pp_offset.x=0.5;
                left_green_prop.pp_offset.y=0.0;
                left_green_prop.pp_offset.z=0.0;
                left_green_prop.pp_beacon_rot_angle[0]=0.0;
                left_green_prop.pp_beacon_rot_rate[0]=1.0;
                left_green_prop.pp_beacon_type='L';
                left_green_prop.pp_beacon_light[0]=nullptr; //no light
                //the flare billboard
                left_green_prop.pp_beacon_scene_node[0] = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
                left_green_prop.pp_beacon_bbs[0]=App::GetGfxScene()->GetSceneManager()->createBillboardSet(this->ComposeName("Prop", static_cast<int>(m_props.size())+1),1);
                left_green_prop.pp_beacon_bbs[0]->createBillboard(0,0,0);
                if (left_green_prop.pp_beacon_bbs[0])
                {
                    left_green_prop.pp_beacon_bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    left_green_prop.pp_beacon_bbs[0]->setMaterialName("tracks/greenflare");
                    left_green_prop.pp_beacon_scene_node[0]->attachObject(left_green_prop.pp_beacon_bbs[0]);
                }
                left_green_prop.pp_beacon_scene_node[0]->setVisible(false);
                left_green_prop.pp_beacon_bbs[0]->setDefaultDimensions(0.5, 0.5);
                m_props.push_back(left_green_prop);
                
                //Left flash
                RoR::Prop left_flash_prop;

                left_flash_prop.pp_node_ref=previous_wing.fa->nbld;
                left_flash_prop.pp_node_x=previous_wing.fa->nblu;
                left_flash_prop.pp_node_y=previous_wing.fa->nbld; //ignored
                left_flash_prop.pp_offset.x=0.5;
                left_flash_prop.pp_offset.y=0.0;
                left_flash_prop.pp_offset.z=0.0;
                left_flash_prop.pp_beacon_rot_angle[0]=0.5; //alt
                left_flash_prop.pp_beacon_rot_rate[0]=1.0;
                left_flash_prop.pp_beacon_type='w';
                //light
                std::string prop_name = this->ComposeName("Prop", static_cast<int>(m_props.size())+1);
                left_flash_prop.pp_beacon_light[0]=App::GetGfxScene()->GetSceneManager()->createLight(prop_name);
                left_flash_prop.pp_beacon_light[0]->setType(Ogre::Light::LT_POINT);
                left_flash_prop.pp_beacon_light[0]->setDiffuseColour( Ogre::ColourValue(1.0, 1.0, 1.0));
                left_flash_prop.pp_beacon_light[0]->setSpecularColour( Ogre::ColourValue(1.0, 1.0, 1.0));
                left_flash_prop.pp_beacon_light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
                left_flash_prop.pp_beacon_light[0]->setCastShadows(false);
                left_flash_prop.pp_beacon_light[0]->setVisible(false);
                //the flare billboard
                left_flash_prop.pp_beacon_scene_node[0] = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
                left_flash_prop.pp_beacon_bbs[0]=App::GetGfxScene()->GetSceneManager()->createBillboardSet(prop_name,1);
                left_flash_prop.pp_beacon_bbs[0]->createBillboard(0,0,0);
                if (left_flash_prop.pp_beacon_bbs[0])
                {
                    left_flash_prop.pp_beacon_bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    left_flash_prop.pp_beacon_bbs[0]->setMaterialName("tracks/flare");
                    left_flash_prop.pp_beacon_scene_node[0]->attachObject(left_flash_prop.pp_beacon_bbs[0]);
                }
                left_flash_prop.pp_beacon_scene_node[0]->setVisible(false);
                left_flash_prop.pp_beacon_bbs[0]->setDefaultDimensions(1.0, 1.0);
                m_props.push_back(left_flash_prop);
                
                //Right red
                m_airplane_right_light=previous_wing.fa->nfrd;
                RoR::Prop right_red_prop;

                
                right_red_prop.pp_node_ref=start_wing.fa->nfrd;
                right_red_prop.pp_node_x=start_wing.fa->nfru;
                right_red_prop.pp_node_y=start_wing.fa->nfrd; //ignored
                right_red_prop.pp_offset.x=0.5;
                right_red_prop.pp_offset.y=0.0;
                right_red_prop.pp_offset.z=0.0;
                right_red_prop.pp_beacon_rot_angle[0]=0.0;
                right_red_prop.pp_beacon_rot_rate[0]=1.0;
                right_red_prop.pp_beacon_type='R';
                right_red_prop.pp_beacon_light[0]=nullptr; /* No light */
                //the flare billboard
                right_red_prop.pp_beacon_scene_node[0] = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
                right_red_prop.pp_beacon_bbs[0]=App::GetGfxScene()->GetSceneManager()->createBillboardSet(this->ComposeName("Prop", static_cast<int>(m_props.size())+1),1);
                right_red_prop.pp_beacon_bbs[0]->createBillboard(0,0,0);
                if (right_red_prop.pp_beacon_bbs[0])
                {
                    right_red_prop.pp_beacon_bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    right_red_prop.pp_beacon_bbs[0]->setMaterialName("tracks/redflare");
                    right_red_prop.pp_beacon_scene_node[0]->attachObject(right_red_prop.pp_beacon_bbs[0]);
                }
                right_red_prop.pp_beacon_scene_node[0]->setVisible(false);
                right_red_prop.pp_beacon_bbs[0]->setDefaultDimensions(0.5, 0.5);
                m_props.push_back(right_red_prop);
                
                //Right flash
                RoR::Prop right_flash_prop;

                right_flash_prop.pp_node_ref=start_wing.fa->nbrd;
                right_flash_prop.pp_node_x=start_wing.fa->nbru;
                right_flash_prop.pp_node_y=start_wing.fa->nbrd; //ignored
                right_flash_prop.pp_offset.x=0.5;
                right_flash_prop.pp_offset.y=0.0;
                right_flash_prop.pp_offset.z=0.0;
                right_flash_prop.pp_beacon_rot_angle[0]=0.5; //alt
                right_flash_prop.pp_beacon_rot_rate[0]=1.0;
                right_flash_prop.pp_beacon_type='w';
                //light
                prop_name = this->ComposeName("Prop", static_cast<int>(m_props.size())+1);
                right_flash_prop.pp_beacon_light[0]=App::GetGfxScene()->GetSceneManager()->createLight(prop_name);
                right_flash_prop.pp_beacon_light[0]->setType(Ogre::Light::LT_POINT);
                right_flash_prop.pp_beacon_light[0]->setDiffuseColour( Ogre::ColourValue(1.0, 1.0, 1.0));
                right_flash_prop.pp_beacon_light[0]->setSpecularColour( Ogre::ColourValue(1.0, 1.0, 1.0));
                right_flash_prop.pp_beacon_light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
                right_flash_prop.pp_beacon_light[0]->setCastShadows(false);
                right_flash_prop.pp_beacon_light[0]->setVisible(false);
                //the flare billboard
                right_flash_prop.pp_beacon_scene_node[0] = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
                right_flash_prop.pp_beacon_bbs[0]=App::GetGfxScene()->GetSceneManager()->createBillboardSet(prop_name,1);
                right_flash_prop.pp_beacon_bbs[0]->createBillboard(0,0,0);
                if (right_flash_prop.pp_beacon_bbs[0] != nullptr)
                {
                    right_flash_prop.pp_beacon_bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    right_flash_prop.pp_beacon_bbs[0]->setMaterialName("tracks/flare");
                    right_flash_prop.pp_beacon_scene_node[0]->attachObject(right_flash_prop.pp_beacon_bbs[0]);
                }
                right_flash_prop.pp_beacon_scene_node[0]->setVisible(false);
                right_flash_prop.pp_beacon_bbs[0]->setDefaultDimensions(1.0, 1.0);
                m_props.push_back(right_flash_prop);
                
                m_generate_wing_position_lights = false; // Already done
            }

            m_first_wing_index = m_actor->ar_num_wings;
            m_wing_area=ComputeWingArea(
                this->GetNode(flex_airfoil->nfld).AbsPosition,    this->GetNode(flex_airfoil->nfrd).AbsPosition,
                this->GetNode(flex_airfoil->nbld).AbsPosition,    this->GetNode(flex_airfoil->nbrd).AbsPosition
            );
        }
        else 
        {
            m_wing_area+=ComputeWingArea(
                this->GetNode(flex_airfoil->nfld).AbsPosition,    this->GetNode(flex_airfoil->nfrd).AbsPosition,
                this->GetNode(flex_airfoil->nbld).AbsPosition,    this->GetNode(flex_airfoil->nbrd).AbsPosition
            );
        }
    }

    // Add new wing to rig
    m_actor->ar_wings[m_actor->ar_num_wings].fa = flex_airfoil;
    m_actor->ar_wings[m_actor->ar_num_wings].cnode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    m_actor->ar_wings[m_actor->ar_num_wings].cnode->attachObject(entity);

    ++m_actor->ar_num_wings;
}

float ActorSpawner::ComputeWingArea(Ogre::Vector3 const & ref, Ogre::Vector3 const & x, Ogre::Vector3 const & y, Ogre::Vector3 const & aref)
{
    return (((x-ref).crossProduct(y-ref)).length()+((x-aref).crossProduct(y-aref)).length())*0.5f;
}

void ActorSpawner::ProcessSoundSource2(Truck::SoundSource2 & def)
{
#ifdef USE_OPENAL
    int mode = (def.mode == Truck::SoundSource2::MODE_CINECAM) ? def.cinecam_index : def.mode;
    NodeIdx_t node_index = FindNodeIndex(def.node);
    if (node_index == -1)
    {
        return;
    }
    AddSoundSource(
            m_actor,
            App::GetSoundScriptManager()->createInstance(def.sound_script_name, m_actor->ar_instance_id), 
            node_index,
            mode
        );
#endif // USE_OPENAL
}

void ActorSpawner::AddSoundSourceInstance(Actor *vehicle, Ogre::String const & sound_script_name, int node_index, int type)
{
#ifdef USE_OPENAL
    AddSoundSource(vehicle, App::GetSoundScriptManager()->createInstance(sound_script_name, vehicle->ar_instance_id, nullptr), (NodeIdx_t)node_index);
#endif // USE_OPENAL
}

void ActorSpawner::AddSoundSource(Actor *vehicle, SoundScriptInstance *sound_script, NodeIdx_t node_index, int type)
{
    if (! CheckSoundScriptLimit(vehicle, 1))
    {
        return;
    }

    if (sound_script == nullptr)
    {
        return;
    }

    vehicle->ar_soundsources[vehicle->ar_num_soundsources].ssi=sound_script;
    vehicle->ar_soundsources[vehicle->ar_num_soundsources].nodenum=node_index;
    vehicle->ar_soundsources[vehicle->ar_num_soundsources].type=type;
    vehicle->ar_num_soundsources++;
}

void ActorSpawner::ProcessSoundSource(Truck::SoundSource & def)
{
#ifdef USE_OPENAL
    AddSoundSource(
            m_actor,
            App::GetSoundScriptManager()->createInstance(def.sound_script_name, m_actor->ar_instance_id), 
            GetNodeIndexOrThrow(def.node),
            -2
        );
#endif // USE_OPENAL
}

void ActorSpawner::ProcessCameraRail(Truck::CameraRail & def)
{
    auto itor = def.nodes.begin();
    auto end  = def.nodes.end();
    for(; itor != end; ++itor)
    {
        if (! CheckCameraRailLimit(1))
        {
            return;
        }
        m_actor->ar_camera_rail[m_actor->ar_num_camera_rails] = GetNodeIndexOrThrow(*itor);
        m_actor->ar_num_camera_rails++;
    }
}

void ActorSpawner::ProcessExtCamera(Truck::ExtCamera & def)
{
    m_actor->ar_extern_camera_mode = def.mode;
    if (def.node.IsValidAnyState())
    {
        m_actor->ar_extern_camera_node = GetNodeIndexOrThrow(def.node);
    }
}

void ActorSpawner::ProcessGuiSettings(Truck::GuiSettings & def)
{
    // Currently unused (was relevant to overlay-based HUD): def.tacho_material; def.speedo_material;

    if (! def.help_material.empty())
    {
        m_state.helpmat = def.help_material;
    }
    if (def.speedo_highest_kph > 10 && def.speedo_highest_kph < 32000)
    {
        m_actor->ar_speedo_max_kph = def.speedo_highest_kph; /* Handles default */
    }
    else
    {
        std::stringstream msg;
        msg << "Invalid 'speedo_highest_kph' value (" << def.speedo_highest_kph << "), allowed range is <10 -32000>. Falling back to default...";
        AddMessage(Message::TYPE_ERROR, msg.str());
        m_actor->ar_speedo_max_kph = 140;
    }
    m_actor->ar_gui_use_engine_max_rpm = def.use_max_rpm;  /* Handles default */

    // NOTE: Dashboard layouts are processed later
}

void ActorSpawner::ProcessFixedNode(Truck::Node::Ref node_ref)
{
    node_t & node = GetNodeOrThrow(node_ref);
    node.nd_immovable = true;
}

void ActorSpawner::ProcessExhaust(Truck::Exhaust & def)
{
    if (m_actor->m_disable_smoke)
    {
        return;
    }

    Exhaust exhaust;
    exhaust.emitterNode   = this->GetNodeIndexOrThrow(def.reference_node);
    exhaust.directionNode = this->GetNodeIndexOrThrow(def.direction_node);

    std::string template_name = def.particle_name;
    if (template_name.empty() || template_name == "default")
    {
        template_name = "tracks/Smoke"; // defined in `particles/smoke.particle`
    }

    std::string name = this->ComposeName(template_name.c_str(), static_cast<int>(m_actor->exhausts.size()));
    exhaust.smoker = this->CreateParticleSystem(name, template_name);
    if (exhaust.smoker == nullptr)
    {
        std::stringstream msg;
        msg << "Failed to create particle system '" << name << "' (template: '" << template_name <<"')";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return;
    }

    exhaust.smokeNode = m_particles_parent_scenenode->createChildSceneNode();
    exhaust.smokeNode->attachObject(exhaust.smoker);
    exhaust.smokeNode->setPosition(this->GetNode(exhaust.emitterNode).AbsPosition);

    // Update GFX for nodes
    for (NodeGfx& nfx : m_gfx_nodes)
    {
        if (nfx.nx_node_idx == exhaust.emitterNode || nfx.nx_node_idx == exhaust.directionNode)
        {
            nfx.nx_is_hot = true;
        }
    }

    m_actor->exhausts.push_back(exhaust);
}

void ActorSpawner::ProcessFlexbodyForset(int pos)
{
    Truck::Flexbody* def = &m_cur_module->flexbodies[m_next_flexbody];
    m_next_flexbody = -1;

    std::vector<NodeIdx_t> nodes;
    for (Truck::Node::Range range: m_cur_module->forset[pos])
    {
        auto start_res = this->GetNodeIndex(range.start);
        auto end_res = this->GetNodeIndex(range.end);
        if (!start_res.second || !end_res.second)
        {
            this->AddMessage(Message::TYPE_ERROR, "Failed to collect nodes from node-ranges, skipping flexbody: " + def->mesh_name);
            return;
        }

        for (NodeIdx_t i = start_res.first; i != end_res.first; ++i)
        {
            nodes.push_back(i);
        }
    }

    const NodeIdx_t reference_node = this->FindNodeIndex(def->reference_node);
    const NodeIdx_t x_axis_node    = this->FindNodeIndex(def->x_axis_node);
    const NodeIdx_t y_axis_node    = this->FindNodeIndex(def->y_axis_node);
    if (reference_node == -1 || x_axis_node == -1 || y_axis_node == -1)
    {
        this->AddMessage(Message::TYPE_ERROR, "Failed to find required nodes, skipping flexbody '" + def->mesh_name + "'");
        return;
    }

    Ogre::Quaternion rot=Ogre::Quaternion(Ogre::Degree(def->rotation.z), Ogre::Vector3::UNIT_Z);
    rot=rot*Ogre::Quaternion(Ogre::Degree(def->rotation.y), Ogre::Vector3::UNIT_Y);
    rot=rot*Ogre::Quaternion(Ogre::Degree(def->rotation.x), Ogre::Vector3::UNIT_X);

    try
    {
        auto* flexbody = m_flex_factory.CreateFlexBody(
            def, reference_node, x_axis_node, y_axis_node, rot, nodes, m_custom_resource_group);

        if (flexbody == nullptr)
            return; // Error already logged

        m_last_flexbody = flexbody;
        m_actor->GetGfxActor()->AddFlexbody(flexbody);
    }
    catch (Ogre::Exception& e)
    {
        this->AddMessage(Message::TYPE_ERROR, 
            "Failed to create flexbody '" + def->mesh_name + "', reason:" + e.getFullDescription());
    }
}

void ActorSpawner::ProcessProp(Truck::Prop & def)
{
    RoR::Prop prop;
    int prop_index = static_cast<int>(m_props.size());

    prop.pp_node_ref         = GetNodeIndexOrThrow(def.reference_node);
    prop.pp_node_x           = FindNodeIndex(def.x_axis_node);
    prop.pp_node_y           = FindNodeIndex(def.y_axis_node);
    if (prop.pp_node_x == node_t::INVALID_IDX || prop.pp_node_y == node_t::INVALID_IDX)
    {
        return; // Error alredy logged by `FindNodeIndex()`
    }
    prop.pp_offset       = def.offset;
    prop.pp_offset_orig  = def.offset;
    prop.pp_rot          = Ogre::Quaternion(Ogre::Degree(def.rotation.z), Ogre::Vector3::UNIT_Z)
                           * Ogre::Quaternion(Ogre::Degree(def.rotation.y), Ogre::Vector3::UNIT_Y)
                           * Ogre::Quaternion(Ogre::Degree(def.rotation.x), Ogre::Vector3::UNIT_X);
    prop.pp_rota         = def.rotation;
    prop.pp_wheel_rot_degree  = 160.f; // ??

    /* SPECIAL PROPS */

    /* Rear view mirror (left) */
    if (def.special == Truck::Prop::SPECIAL_MIRROR_LEFT)
    {
        m_curr_mirror_prop_type = CustomMaterial::MirrorPropType::MPROP_LEFT;
    }

    /* Rear view mirror (right) */
    if (def.special == Truck::Prop::SPECIAL_MIRROR_RIGHT)
    {
        m_curr_mirror_prop_type = CustomMaterial::MirrorPropType::MPROP_RIGHT;
    }

    /* Custom steering wheel */
    Ogre::Vector3 steering_wheel_offset = Ogre::Vector3::ZERO;
    if (def.special == Truck::Prop::SPECIAL_DASHBOARD_LEFT)
    {
        steering_wheel_offset = Ogre::Vector3(-0.67, -0.61,0.24);
    }
    if (def.special == Truck::Prop::SPECIAL_DASHBOARD_RIGHT)
    {
        steering_wheel_offset = Ogre::Vector3(0.67, -0.61,0.24);
    }
    if (steering_wheel_offset != Ogre::Vector3::ZERO)
    {
        if (def.special_prop_dashboard._offset_is_set)
        {
            steering_wheel_offset = def.special_prop_dashboard.offset;
        }
        prop.pp_wheel_rot_degree = def.special_prop_dashboard.rotation_angle;
        prop.pp_wheel_scene_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
        prop.pp_wheel_pos = steering_wheel_offset;
        const std::string instance_name = this->ComposeName("SteeringWheelPropEntity", prop_index);
        prop.pp_wheel_mesh_obj = new MeshObject(
            def.special_prop_dashboard.mesh_name,
            m_custom_resource_group,
            instance_name,
            prop.pp_wheel_scene_node
            );
        this->SetupNewEntity(prop.pp_wheel_mesh_obj->getEntity(), Ogre::ColourValue(0, 0.5, 0.5));
    }

    /* CREATE THE PROP */

    prop.pp_scene_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    const std::string instance_name = this->ComposeName("PropEntity", prop_index);
    prop.pp_mesh_obj = new MeshObject(def.mesh_name, m_custom_resource_group, instance_name, prop.pp_scene_node);
    prop.pp_mesh_obj->setCastShadows(true); // Orig code {{ prop.pp_mesh_obj->setCastShadows(shadowmode != 0); }}, shadowmode has default value 1 and changes with undocumented directive 'set_shadows'

    if (def.special == Truck::Prop::SPECIAL_AERO_PROP_SPIN)
    {
        prop.pp_aero_propeller_spin = true;
        prop.pp_mesh_obj->setCastShadows(false);
        prop.pp_scene_node->setVisible(false);
    }
    else if(def.special == Truck::Prop::SPECIAL_AERO_PROP_BLADE)
    {
        prop.pp_aero_propeller_blade = true;
    }
    else if(def.special == Truck::Prop::SPECIAL_DRIVER_SEAT)
    {
        //driver seat, used to position the driver and make the seat translucent at times
        if (m_driverseat_prop_index == -1)
        {
            m_driverseat_prop_index = prop_index;
            prop.pp_mesh_obj->setMaterialName("driversseat");
        }
        else
        {
            this->AddMessage(Message::TYPE_INFO, "Found more than one 'seat[2]' special props. Only the first one will be the driver's seat.");
        }
    }
    else if(def.special == Truck::Prop::SPECIAL_DRIVER_SEAT_2)
    {
        // Same as DRIVER_SEAT, except it doesn't force the "driversseat" material
        if (m_driverseat_prop_index == -1)
        {
            m_driverseat_prop_index = prop_index;
        }
        else
        {
            this->AddMessage(Message::TYPE_INFO, "Found more than one 'seat[2]' special props. Only the first one will be the driver's seat.");
        }
    }
    else if (m_actor->m_flares_mode != GfxFlaresMode::NONE)
    {
        if(def.special == Truck::Prop::SPECIAL_BEACON)
        {
            prop.pp_beacon_type = 'b';
            prop.pp_beacon_rot_angle[0] = 2.0 * 3.14 * frand();
            prop.pp_beacon_rot_rate[0] = 4.0 * 3.14 + frand() - 0.5;
            /* the light */
            auto pp_beacon_light = App::GetGfxScene()->GetSceneManager()->createLight();
            pp_beacon_light->setType(Ogre::Light::LT_SPOTLIGHT);
            pp_beacon_light->setDiffuseColour(def.special_prop_beacon.color);
            pp_beacon_light->setSpecularColour(def.special_prop_beacon.color);
            pp_beacon_light->setAttenuation(50.0, 1.0, 0.3, 0.0);
            pp_beacon_light->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
            pp_beacon_light->setCastShadows(false);
            pp_beacon_light->setVisible(false);
            /* the flare billboard */
            auto flare_scene_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
            auto flare_billboard_sys = App::GetGfxScene()->GetSceneManager()->createBillboardSet(1); //(propname,1);
            if (flare_billboard_sys)
            {
                flare_billboard_sys->createBillboard(0,0,0);
                flare_billboard_sys->setMaterialName(def.special_prop_beacon.flare_material_name);
                flare_billboard_sys->setVisibilityFlags(DEPTHMAP_DISABLED);
                flare_scene_node->attachObject(flare_billboard_sys);
            }
            flare_scene_node->setVisible(false);

            // Complete
            prop.pp_beacon_scene_node[0] = flare_scene_node;
            prop.pp_beacon_bbs[0] = flare_billboard_sys;
            prop.pp_beacon_light[0] = pp_beacon_light;
        }
        else if(def.special == Truck::Prop::SPECIAL_REDBEACON)
        {
            prop.pp_beacon_rot_angle[0] = 0.f;
            prop.pp_beacon_rot_rate[0] = 1.0;
            prop.pp_beacon_type = 'r';
            //the light
            auto pp_beacon_light=App::GetGfxScene()->GetSceneManager()->createLight();//propname);
            pp_beacon_light->setType(Ogre::Light::LT_POINT);
            pp_beacon_light->setDiffuseColour( Ogre::ColourValue(1.0, 0.0, 0.0));
            pp_beacon_light->setSpecularColour( Ogre::ColourValue(1.0, 0.0, 0.0));
            pp_beacon_light->setAttenuation(50.0, 1.0, 0.3, 0.0);
            pp_beacon_light->setCastShadows(false);
            pp_beacon_light->setVisible(false);
            //the flare billboard
            auto flare_scene_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
            auto flare_billboard_sys = App::GetGfxScene()->GetSceneManager()->createBillboardSet(1); //propname,1);
            if (flare_billboard_sys)
            {
                flare_billboard_sys->createBillboard(0,0,0);
                flare_billboard_sys->setMaterialName("tracks/redbeaconflare");
                flare_billboard_sys->setVisibilityFlags(DEPTHMAP_DISABLED);
                flare_billboard_sys->setDefaultDimensions(1.0, 1.0);
                flare_scene_node->attachObject(flare_billboard_sys);
            }
            flare_scene_node->setVisible(false);

            // Finalize
            prop.pp_beacon_light[0] = pp_beacon_light;
            prop.pp_beacon_scene_node[0] = flare_scene_node;
            prop.pp_beacon_bbs[0] = flare_billboard_sys;
            
        }
        else if(def.special == Truck::Prop::SPECIAL_LIGHTBAR)
        {
            m_actor->ar_is_police = true;
            prop.pp_beacon_type='p';
            for (int k=0; k<4; k++)
            {
                prop.pp_beacon_rot_angle[k] = 2.0 * 3.14 * frand();
                prop.pp_beacon_rot_rate[k] = 4.0 * 3.14 + frand() - 0.5;
                prop.pp_beacon_bbs[k] = nullptr;
                //the light
                prop.pp_beacon_light[k]=App::GetGfxScene()->GetSceneManager()->createLight();
                prop.pp_beacon_light[k]->setType(Ogre::Light::LT_SPOTLIGHT);
                if (k>1)
                {
                    prop.pp_beacon_light[k]->setDiffuseColour( Ogre::ColourValue(1.0, 0.0, 0.0));
                    prop.pp_beacon_light[k]->setSpecularColour( Ogre::ColourValue(1.0, 0.0, 0.0));
                }
                else
                {
                    prop.pp_beacon_light[k]->setDiffuseColour( Ogre::ColourValue(0.0, 0.5, 1.0));
                    prop.pp_beacon_light[k]->setSpecularColour( Ogre::ColourValue(0.0, 0.5, 1.0));
                }
                prop.pp_beacon_light[k]->setAttenuation(50.0, 1.0, 0.3, 0.0);
                prop.pp_beacon_light[k]->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
                prop.pp_beacon_light[k]->setCastShadows(false);
                prop.pp_beacon_light[k]->setVisible(false);
                //the flare billboard
                prop.pp_beacon_scene_node[k] = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
                prop.pp_beacon_bbs[k]=App::GetGfxScene()->GetSceneManager()->createBillboardSet(1);
                prop.pp_beacon_bbs[k]->createBillboard(0,0,0);
                if (prop.pp_beacon_bbs[k])
                {
                    if (k>1)
                    {
                        prop.pp_beacon_bbs[k]->setMaterialName("tracks/brightredflare");
                    }
                    else
                    {
                        prop.pp_beacon_bbs[k]->setMaterialName("tracks/brightblueflare");
                    }

                    prop.pp_beacon_bbs[k]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    prop.pp_beacon_scene_node[k]->attachObject(prop.pp_beacon_bbs[k]);
                }
                prop.pp_beacon_scene_node[k]->setVisible(false);
            }
        }

        if (m_curr_mirror_prop_type != CustomMaterial::MirrorPropType::MPROP_NONE)
        {
            m_curr_mirror_prop_scenenode = prop.pp_mesh_obj->GetSceneNode();
        }
    }

    this->SetupNewEntity(prop.pp_mesh_obj->getEntity(), Ogre::ColourValue(1.f, 1.f, 0.f));

    m_curr_mirror_prop_scenenode = nullptr;
    m_curr_mirror_prop_type = CustomMaterial::MirrorPropType::MPROP_NONE;

    /* PROCESS ANIMATIONS */

    for (Truck::Animation& anim_def: def.animations)
    {
        PropAnim anim;
        anim.animKeyState = -1.0f; // Orig: hardcoded in {add_animation}

        /* Arg #1: ratio */
        anim.animratio = anim_def.ratio;
        if (anim_def.ratio == 0)
        {
            std::stringstream msg;
            msg << "Prop (mesh: " << def.mesh_name << ") has invalid animation ratio (0), using it anyway (compatibility)...";
            AddMessage(Message::TYPE_WARNING, msg.str());
        }

        /* Arg #2: option1 (lower limit) */
        anim.lower_limit = anim_def.lower_limit; /* Handles default */

        /* Arg #3: option2 (upper limit) */
        anim.upper_limit = anim_def.upper_limit; /* Handles default */

        /* Arg #4: source */
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_AIRSPEED)) { /* (NOTE: code formatting relaxed) */
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_AIRSPEED);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_VERTICAL_VELOCITY)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_VVI);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_ALTIMETER_100K)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_ALTIMETER);
            anim.animOpt3 = 1.f;
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_ALTIMETER_10K)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_ALTIMETER);
            anim.animOpt3 = 2.f;
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_ALTIMETER_1K)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_ALTIMETER);
            anim.animOpt3 = 3.f;
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_ANGLE_OF_ATTACK)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_AOA);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_FLAP)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_FLAP);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_AIR_BRAKE)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_AIRBRAKE);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_ROLL)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_ROLL);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_PITCH)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_PITCH);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_BRAKES)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_BRAKE);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_ACCEL)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_ACCEL);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_CLUTCH)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_CLUTCH);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_SPEEDO)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_SPEEDO);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_TACHO)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_TACHO);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_TURBO)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_TURBO);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_PARKING)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_PBRAKE);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_SHIFT_LEFT_RIGHT)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_SHIFTER);
            anim.animOpt3 = 1.0f;
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_SHIFT_BACK_FORTH)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_SHIFTER);
            anim.animOpt3 = 2.0f;
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_SEQUENTIAL_SHIFT)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_SHIFTER);
            anim.animOpt3 = 3.0f;
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_SHIFTERLIN)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_SHIFTER);
            anim.animOpt3 = 4.0f;
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_TORQUE)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_TORQUE);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_HEADING)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_HEADING);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_DIFFLOCK)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_DIFFLOCK);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_STEERING_WHEEL)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_STEERING);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_AILERON)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_AILERONS);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_ELEVATOR)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_ELEVATORS);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_AIR_RUDDER)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_ARUDDER);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_BOAT_RUDDER)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_BRUDDER);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_BOAT_THROTTLE)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_BTHROTTLE);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_PERMANENT)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_PERMANENT);
        }
        if (BITMASK_IS_1(anim_def.source, Truck::Animation::SOURCE_EVENT)) {
            BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_EVENT);
        }
        /* Motor-indexed sources */
        std::list<Truck::Animation::MotorSource>::iterator source_itor = anim_def.motor_sources.begin();
        for ( ; source_itor != anim_def.motor_sources.end(); source_itor++)
        {
            if (BITMASK_IS_1(source_itor->source, Truck::Animation::MotorSource::SOURCE_AERO_THROTTLE)) {
                BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_THROTTLE);
                anim.animOpt3 = static_cast<float>(source_itor->motor);
            }
            if (BITMASK_IS_1(source_itor->source, Truck::Animation::MotorSource::SOURCE_AERO_RPM)) {
                BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_RPM);
                anim.animOpt3 = static_cast<float>(source_itor->motor);
            }
            if (BITMASK_IS_1(source_itor->source, Truck::Animation::MotorSource::SOURCE_AERO_TORQUE)) {
                BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_AETORQUE);
                anim.animOpt3 = static_cast<float>(source_itor->motor);
            }
            if (BITMASK_IS_1(source_itor->source, Truck::Animation::MotorSource::SOURCE_AERO_PITCH)) {
                BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_AEPITCH);
                anim.animOpt3 = static_cast<float>(source_itor->motor);
            }
            if (BITMASK_IS_1(source_itor->source, Truck::Animation::MotorSource::SOURCE_AERO_STATUS)) {
                BITMASK_SET_1(anim.animFlags, PROP_ANIM_FLAG_AESTATUS);
                anim.animOpt3 = static_cast<float>(source_itor->motor);
            }
        }
        if (anim.animFlags == 0)
        {
            AddMessage(Message::TYPE_ERROR, "Failed to identify animation source");
        }

        /* Anim modes */
        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_ROTATION_X)) {
            BITMASK_SET_1(anim.animMode, PROP_ANIM_MODE_ROTA_X);
        }
        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_ROTATION_Y)) {
            BITMASK_SET_1(anim.animMode, PROP_ANIM_MODE_ROTA_Y);
        }
        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_ROTATION_Z)) {
            BITMASK_SET_1(anim.animMode, PROP_ANIM_MODE_ROTA_Z);
        }
        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_OFFSET_X)) {
            BITMASK_SET_1(anim.animMode, PROP_ANIM_MODE_OFFSET_X);
        }
        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_OFFSET_Y)) {
            BITMASK_SET_1(anim.animMode, PROP_ANIM_MODE_OFFSET_Y);
        }
        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_OFFSET_Z)) {
            BITMASK_SET_1(anim.animMode, PROP_ANIM_MODE_OFFSET_Z);
        }
        if (anim.animMode == 0)
        {
            AddMessage(Message::TYPE_ERROR, "Failed to identify animation mode");
        }

        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_AUTO_ANIMATE)) 
        {
            BITMASK_SET_1(anim.animMode, PROP_ANIM_MODE_AUTOANIMATE);

            // Flag whether default lower and/or upper animation limit constraints are effective
            const bool use_default_lower_limit = (anim_def.lower_limit == 0.f);
            const bool use_default_upper_limit = (anim_def.upper_limit == 0.f);

            if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_ROTATION_X)) {
                anim.lower_limit = (use_default_lower_limit) ? (-180.f) : (anim_def.lower_limit + prop.pp_rota.x);
                anim.upper_limit = (use_default_upper_limit) ? ( 180.f) : (anim_def.upper_limit + prop.pp_rota.x);
            }
            if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_ROTATION_Y)) {
                anim.lower_limit = (use_default_lower_limit) ? (-180.f) : (anim_def.lower_limit + prop.pp_rota.y);
                anim.upper_limit = (use_default_upper_limit) ? ( 180.f) : (anim_def.upper_limit + prop.pp_rota.y);
            }
            if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_ROTATION_Z)) {
                anim.lower_limit = (use_default_lower_limit) ? (-180.f) : (anim_def.lower_limit + prop.pp_rota.z);
                anim.upper_limit = (use_default_upper_limit) ? ( 180.f) : (anim_def.upper_limit + prop.pp_rota.z);
            }
            if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_OFFSET_X)) {
                anim.lower_limit = (use_default_lower_limit) ? (-10.f) : (anim_def.lower_limit + prop.pp_offset_orig.x);
                anim.upper_limit = (use_default_upper_limit) ? ( 10.f) : (anim_def.upper_limit + prop.pp_offset_orig.x);
            }
            if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_OFFSET_Y)) {
                anim.lower_limit = (use_default_lower_limit) ? (-10.f) : (anim_def.lower_limit + prop.pp_offset_orig.y);
                anim.upper_limit = (use_default_upper_limit) ? ( 10.f) : (anim_def.upper_limit + prop.pp_offset_orig.y);
            }
            if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_OFFSET_Z)) {
                anim.lower_limit = (use_default_lower_limit) ? (-10.f) : (anim_def.lower_limit + prop.pp_offset_orig.z);
                anim.upper_limit = (use_default_upper_limit) ? ( 10.f) : (anim_def.upper_limit + prop.pp_offset_orig.z);
            }
        }
        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_NO_FLIP)) 
        {
            BITMASK_SET_1(anim.animMode, PROP_ANIM_MODE_NOFLIP);
        }
        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_BOUNCE)) 
        {
            BITMASK_SET_1(anim.animMode, PROP_ANIM_MODE_BOUNCE);
            anim.animOpt5 = 1.f;
        }
        if (BITMASK_IS_1(anim_def.mode, Truck::Animation::MODE_EVENT_LOCK)) 
        {
            anim.animKeyState = 0.0f;
            anim.lastanimKS = 0.0f;
        }
        
        /* Parameter 'event:' */

        if (! anim_def.event.empty())
        {
            // we are using keys as source
            anim.animFlags |= PROP_ANIM_FLAG_EVENT;

            int event_id = RoR::App::GetInputEngine()->resolveEventName(anim_def.event);
            if (event_id == -1)
            {
                AddMessage(Message::TYPE_ERROR, "Unknown animation event: " + anim_def.event);
            }
            else
            {
                anim.animKey = event_id;
            }
        }
        prop.pp_animations.push_back(anim);
    }
    m_props.push_back(prop);
}

void ActorSpawner::ProcessFlare2(Truck::Flare2 & def)
{
    if (m_actor->m_flares_mode == GfxFlaresMode::NONE) { return; }

    int blink_delay = def.blink_delay_milis;
    float size = def.size;

    /* Backwards compatibility */
    if (blink_delay == -2) 
    {
        if (def.type == Truck::Flare2::TYPE_l_LEFT_BLINKER || def.type == Truck::Flare2::TYPE_r_RIGHT_BLINKER)
        {
            blink_delay = -1; /* Default blink */
        }
        else
        {
            blink_delay = 0; /* Default no blink */
        }
    }
    
    if (size == -2.f && def.type == Truck::Flare2::TYPE_f_HEADLIGHT)
    {
        size = 1.f;
    }
    else if ((size == -2.f && def.type != Truck::Flare2::TYPE_f_HEADLIGHT) || size == -1.f)
    {
        size = 0.5f;
    }

    Flare flare;
    flare.fl_type              = static_cast<FlareType>(def.type);
    flare.controlnumber        = def.control_number;
    flare.controltoggle_status = false;
    flare.blinkdelay           = (blink_delay == -1) ? 0.5f : blink_delay / 1000.f;
    flare.blinkdelay_curr      = 0.f;
    flare.blinkdelay_state     = false;
    flare.noderef              = GetNodeIndexOrThrow(def.reference_node);
    flare.nodex                = GetNodeIndexOrThrow(def.node_axis_x);
    flare.nodey                = GetNodeIndexOrThrow(def.node_axis_y);
    flare.offsetx              = def.offset.x;
    flare.offsety              = def.offset.y;
    flare.offsetz              = def.offset.z;
    flare.size                 = size;

    /* Visuals */
    flare.snode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    std::string flare_name = this->ComposeName("Flare", static_cast<int>(m_actor->ar_flares.size()));
    flare.bbs = App::GetGfxScene()->GetSceneManager()->createBillboardSet(flare_name, 1);
    bool using_default_material = true;
    if (flare.bbs == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, "Failed to create flare: '" + flare_name + "', continuing without it (compatibility)...");
    }
    else
    {
        flare.bbs->createBillboard(0,0,0);
        flare.bbs->setVisibilityFlags(DEPTHMAP_DISABLED);
        std::string material_name = def.material_name;
        using_default_material = (material_name.length() == 0 || material_name == "default");
        if (using_default_material)
        {
            if (def.type == Truck::Flare2::TYPE_b_BRAKELIGHT)
            {
                material_name = "tracks/brakeflare";
            }
            else if (def.type == Truck::Flare2::TYPE_l_LEFT_BLINKER || (def.type == Truck::Flare2::TYPE_r_RIGHT_BLINKER))
            {
                material_name = "tracks/blinkflare";
            }
            else
            {
                material_name = "tracks/flare";
            }
        }
        Ogre::MaterialPtr material = this->FindOrCreateCustomizedMaterial(material_name);
        if (!material.isNull())
        {
            flare.bbs->setMaterial(material);
            flare.snode->attachObject(flare.bbs);
        }
    }
    flare.isVisible = true;
    flare.light = nullptr;

    if ((App::gfx_flares_mode->GetEnum<GfxFlaresMode>() >= GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY) && size > 0.001)
    {
        //if (type == 'f' && usingDefaultMaterial && flaresMode >=2 && size > 0.001)
        if (def.type == Truck::Flare2::TYPE_f_HEADLIGHT && using_default_material )
        {
            /* front light */
            flare.light=App::GetGfxScene()->GetSceneManager()->createLight(flare_name);
            flare.light->setType(Ogre::Light::LT_SPOTLIGHT);
            flare.light->setDiffuseColour( Ogre::ColourValue(1, 1, 1));
            flare.light->setSpecularColour( Ogre::ColourValue(1, 1, 1));
            flare.light->setAttenuation(400, 0.9, 0, 0);
            flare.light->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
            flare.light->setCastShadows(false);
        }
    }
    if ((App::gfx_flares_mode->GetEnum<GfxFlaresMode>() >= GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS) && size > 0.001)
    {
        if (def.type == Truck::Flare2::TYPE_f_HEADLIGHT && ! using_default_material)
        {
            /* this is a quick fix for the red backlight when frontlight is switched on */
            flare.light=App::GetGfxScene()->GetSceneManager()->createLight(flare_name);
            flare.light->setDiffuseColour( Ogre::ColourValue(1.0, 0, 0));
            flare.light->setSpecularColour( Ogre::ColourValue(1.0, 0, 0));
            flare.light->setAttenuation(10.0, 1.0, 0, 0);
        }
        else if (def.type == Truck::Flare2::TYPE_R_REVERSE_LIGHT)
        {
            flare.light=App::GetGfxScene()->GetSceneManager()->createLight(flare_name);
            flare.light->setDiffuseColour(Ogre::ColourValue(1, 1, 1));
            flare.light->setSpecularColour(Ogre::ColourValue(1, 1, 1));
            flare.light->setAttenuation(20.0, 1, 0, 0);
        }
        else if (def.type == Truck::Flare2::TYPE_b_BRAKELIGHT)
        {
            flare.light=App::GetGfxScene()->GetSceneManager()->createLight(flare_name);
            flare.light->setDiffuseColour( Ogre::ColourValue(1.0, 0, 0));
            flare.light->setSpecularColour( Ogre::ColourValue(1.0, 0, 0));
            flare.light->setAttenuation(10.0, 1.0, 0, 0);
        }
        else if (def.type == Truck::Flare2::TYPE_l_LEFT_BLINKER || (def.type == Truck::Flare2::TYPE_r_RIGHT_BLINKER))
        {
            flare.light=App::GetGfxScene()->GetSceneManager()->createLight(flare_name);
            flare.light->setDiffuseColour( Ogre::ColourValue(1, 1, 0));
            flare.light->setSpecularColour( Ogre::ColourValue(1, 1, 0));
            flare.light->setAttenuation(10.0, 1, 1, 0);
        }
        //else if ((type == 'u') && flaresMode >= 4 && size > 0.001)
        else if (def.type == Truck::Flare2::TYPE_u_USER)
        {
            /* user light always white (TODO: improve this) */
            flare.light=App::GetGfxScene()->GetSceneManager()->createLight(flare_name);
            flare.light->setDiffuseColour( Ogre::ColourValue(1, 1, 1));
            flare.light->setSpecularColour( Ogre::ColourValue(1, 1, 1));
            flare.light->setAttenuation(50.0, 1.0, 1, 0.2);

            if (m_actor->m_net_custom_light_count < 4)
            {
                m_actor->m_net_custom_lights[m_actor->m_net_custom_light_count] = static_cast<int>(m_actor->ar_flares.size());
                m_actor->m_net_custom_light_count++;
            }
        }
    }

    /* Finalize light */
    if (flare.light != nullptr)
    {
        flare.light->setType(Ogre::Light::LT_SPOTLIGHT);
        flare.light->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
        flare.light->setCastShadows(false);
    }
    m_actor->ar_flares.push_back(flare);
}

Ogre::MaterialPtr ActorSpawner::InstantiateManagedMaterial(Ogre::String const & source_name, Ogre::String const & clone_name)
{
    Ogre::MaterialPtr src_mat = Ogre::MaterialManager::getSingleton().getByName(source_name);
    if (src_mat.isNull())
    {
        std::stringstream msg;
        msg << "Built-in material '" << source_name << "' missing! Skipping...";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return Ogre::MaterialPtr();
    }

    return src_mat->clone(clone_name, true, m_custom_resource_group);
}

void ActorSpawner::ProcessManagedMaterial(Truck::ManagedMaterial & def)
{

    if (m_managed_materials.find(def.name) != m_managed_materials.end())
    {
        this->AddMessage(Message::TYPE_ERROR, "Duplicate managed material name: '" + def.name + "'. Ignoring definition...");
        return;
    }

    // Create temporary placeholder
    // This is necessary to load meshes with original material names (= unchanged managed mat names)
    // - if not found, OGRE substitutes them with 'BaseWhite' which breaks subsequent processing.
    if (Ogre::MaterialManager::getSingleton().getByName(def.name, m_custom_resource_group).isNull())
    {
        m_placeholder_managedmat->clone(def.name, /*changeGroup=*/true, m_custom_resource_group);
    }

    std::string custom_name = def.name + ACTOR_ID_TOKEN + TOSTRING(m_actor->ar_instance_id);
    Ogre::MaterialPtr material;
    if (def.type == Truck::ManagedMaterial::TYPE_FLEXMESH_STANDARD || def.type == Truck::ManagedMaterial::TYPE_FLEXMESH_TRANSPARENT)
    {
        std::string mat_name_base
            = (def.type == Truck::ManagedMaterial::TYPE_FLEXMESH_STANDARD)
            ? "managed/flexmesh_standard"
            : "managed/flexmesh_transparent";

        if (def.HasDamagedDiffuseMap())
        {
            if (def.HasSpecularMap())
            {
                /* FLEXMESH, damage, specular */
                if (App::gfx_classic_shaders->GetBool())
                {
                material = this->InstantiateManagedMaterial(mat_name_base + "/speculardamage_nicemetal", custom_name);
                }
                else
                {
                material = this->InstantiateManagedMaterial(mat_name_base + "/speculardamage", custom_name);
                }
                if (material.isNull())
                {
                    return;
                }
                if(App::gfx_classic_shaders->GetBool())
                {
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Dmg_Diffuse_Map")->setTextureName(def.damaged_diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Specular_Map")->setTextureName(def.specular_map);
                material->getTechnique("BaseTechnique")->getPass("Specular")->getTextureUnitState("Specular_Map")->setTextureName(def.specular_map);
                }
                else
                {
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Dmg_Diffuse_Map")->setTextureName(def.damaged_diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("SpecularMapping1")->getTextureUnitState("SpecularMapping1_Tex")->setTextureName(def.specular_map);
                }
            }
            else
            {
                /* FLEXMESH, damage, no_specular */
                material = this->InstantiateManagedMaterial(mat_name_base + "/damageonly", custom_name);
                if (material.isNull())
                {
                    return;
                }
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Dmg_Diffuse_Map")->setTextureName(def.damaged_diffuse_map);
            }
        }
        else
        {
            if (def.HasSpecularMap())
            {
                /* FLEXMESH, no_damage, specular */
                if (App::gfx_classic_shaders->GetBool())
                {
                    material = this->InstantiateManagedMaterial(mat_name_base + "/specularonly_nicemetal", custom_name);
                }
                else
                {
                    material = this->InstantiateManagedMaterial(mat_name_base + "/specularonly", custom_name);
                }
                if (material.isNull())
                {
                    return;
                }
                if (App::gfx_classic_shaders->GetBool())
                {
                    material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
                    material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Specular_Map")->setTextureName(def.specular_map);
                    material->getTechnique("BaseTechnique")->getPass("Specular")->getTextureUnitState("Specular_Map")->setTextureName(def.specular_map);
                }
                else
                {
                    material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
                    material->getTechnique("BaseTechnique")->getPass("SpecularMapping1")->getTextureUnitState("SpecularMapping1_Tex")->setTextureName(def.specular_map);
                }
            }
            else
            {
                /* FLEXMESH, no_damage, no_specular */
                material = this->InstantiateManagedMaterial(mat_name_base + "/simple", custom_name);
                if (material.isNull())
                {
                    return;
                }
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
            }
        }
    }
    else if (def.type == Truck::ManagedMaterial::TYPE_MESH_STANDARD || def.type == Truck::ManagedMaterial::TYPE_MESH_TRANSPARENT)
    {
        Ogre::String mat_name_base
            = (def.type == Truck::ManagedMaterial::TYPE_MESH_STANDARD)
            ? "managed/mesh_standard"
            : "managed/mesh_transparent";

        if (def.HasSpecularMap())
        {
            /* MESH, specular */
            if (App::gfx_classic_shaders->GetBool())
            {
                material = this->InstantiateManagedMaterial(mat_name_base + "/specular_nicemetal", custom_name);
            }
            else
            {
                material = this->InstantiateManagedMaterial(mat_name_base + "/specular", custom_name);
            }
            if (material.isNull())
            {
                return;
            }
            if (App::gfx_classic_shaders->GetBool())
            {
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Specular_Map")->setTextureName(def.specular_map);
                material->getTechnique("BaseTechnique")->getPass("Specular")->getTextureUnitState("Specular_Map")->setTextureName(def.specular_map);
            }
            else
            {
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("SpecularMapping1")->getTextureUnitState("SpecularMapping1_Tex")->setTextureName(def.specular_map);
            }
        }
        else
        {
            /* MESH, no_specular */
            material = this->InstantiateManagedMaterial(mat_name_base + "/simple", custom_name);
            if (material.isNull())
            {
                return;
            }
            material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);

        }
    }

    if (def.type != Truck::ManagedMaterial::TYPE_INVALID)
    {
        if (m_state.managedmaterials_doublesided)
        {
            material->getTechnique("BaseTechnique")->getPass("BaseRender")->setCullingMode(Ogre::CULL_NONE);
            if (def.HasSpecularMap())
            {
                if (App::gfx_classic_shaders->GetBool())
                {
                    material->getTechnique("BaseTechnique")->getPass("Specular")->setCullingMode(Ogre::CULL_NONE);
                }
                else
                {
                    material->getTechnique("BaseTechnique")->getPass("SpecularMapping1")->setCullingMode(Ogre::CULL_NONE);
                }
            }
        }
    }

    /* Finalize */

    material->compile();
    m_managed_materials.insert(std::make_pair(def.name, material));
}

void ActorSpawner::ProcessCollisionBox(Truck::CollisionBox & def)
{
    int8_t bbox_id = static_cast<int8_t>(m_actor->ar_collision_bounding_boxes.size());
    for (Truck::Node::Ref& node_ref: def.nodes)
    {
        std::pair<unsigned int, bool> node_result = this->GetNodeIndex(node_ref);
        if (! node_result.second)
        {
            RoR::LogFormat("[RoR|Spawner] Collision box: skipping invalid node '%s'", node_ref.ToString().c_str());
            continue;
        }
        if (m_actor->ar_nodes[node_result.first].nd_coll_bbox_id != node_t::INVALID_BBOX)
        {
            RoR::LogFormat("[RoR|Spawner] Collision box: re-assigning node '%s' from box ID '%d' to '%d'",
                node_ref.ToString().c_str(),
                m_actor->ar_nodes[node_result.first].nd_coll_bbox_id,
                bbox_id);
        }
        m_actor->ar_nodes[node_result.first].nd_coll_bbox_id = bbox_id;
    }

    m_actor->ar_collision_bounding_boxes.push_back(Ogre::AxisAlignedBox()); // Updated later
    m_actor->ar_predicted_coll_bounding_boxes.push_back(Ogre::AxisAlignedBox());
}

bool ActorSpawner::AssignWheelToAxle(int & _out_axle_wheel, node_t *axis_node_1, node_t *axis_node_2)
{
    for (int i = 0; i < m_actor->ar_num_wheels; i++)
    {
        wheel_t & wheel = m_actor->ar_wheels[i];
        if	(	(wheel.wh_axis_node_0 == axis_node_1 && wheel.wh_axis_node_1 == axis_node_2)
            ||	(wheel.wh_axis_node_0 == axis_node_2 && wheel.wh_axis_node_1 == axis_node_1)
            )
        {
            _out_axle_wheel = i;
            return true;
        }
    }
    return false;
}

void ActorSpawner::ProcessAxle(Truck::Axle & def)
{
    if (! CheckAxleLimit(1))
    {
        return;
    }

    node_t *wheel_1_node_1 = GetNodePointerOrThrow(def.wheels[0][0]);
    node_t *wheel_1_node_2 = GetNodePointerOrThrow(def.wheels[0][1]);
    node_t *wheel_2_node_1 = GetNodePointerOrThrow(def.wheels[1][0]);
    node_t *wheel_2_node_2 = GetNodePointerOrThrow(def.wheels[1][1]);

    Differential *diff = new Differential();

    if (! AssignWheelToAxle(diff->di_idx_1, wheel_1_node_1, wheel_1_node_2))
    {
        std::stringstream msg;
        msg << "Couldn't find wheel with axis nodes '" << def.wheels[0][0].ToString()
            << "' and '" << def.wheels[0][1].ToString() << "'";
        AddMessage(Message::TYPE_WARNING, msg.str());
    }

    if (! AssignWheelToAxle(diff->di_idx_2, wheel_2_node_1, wheel_2_node_2))
    {
        std::stringstream msg;
        msg << "Couldn't find wheel with axis nodes '" << def.wheels[1][0].ToString()
            << "' and '" << def.wheels[1][1].ToString() << "'";
        AddMessage(Message::TYPE_WARNING, msg.str());
    }

    if (def.options.size() == 0)
    {
        AddMessage(Message::TYPE_INFO, "No differential defined, defaulting to Open & Locked");
        diff->AddDifferentialType(OPEN_DIFF);
        diff->AddDifferentialType(LOCKED_DIFF);
    }
    else
    {
        auto end = def.options.end();
        for (auto itor = def.options.begin(); itor != end; ++itor)
        {
            switch (*itor)
            {
            case Truck::DifferentialType::DIFF_l_LOCKED:
                diff->AddDifferentialType(LOCKED_DIFF);
                break;
            case Truck::DifferentialType::DIFF_o_OPEN:
                diff->AddDifferentialType(OPEN_DIFF);
                break;
            case Truck::DifferentialType::DIFF_s_SPLIT:
                diff->AddDifferentialType(SPLIT_DIFF);
                break;
            case Truck::DifferentialType::DIFF_v_VISCOUS:
                diff->AddDifferentialType(VISCOUS_DIFF);
                break;
            default:
                AddMessage(Message::TYPE_WARNING, fmt::format("Unknown differential type: '{}'", (char)*itor));
                break;
            }
        }
    }

    m_actor->m_wheel_diffs[m_actor->m_num_wheel_diffs] = diff;
    m_actor->m_num_wheel_diffs++;
}

void ActorSpawner::ProcessInterAxle(Truck::InterAxle & def)
{
    if (def.a1 == def.a2 || std::min(def.a1, def.a2) < 0 || std::max(def.a1 , def.a2) >= m_actor->m_num_wheel_diffs)
    {
        AddMessage(Message::TYPE_ERROR, "Invalid 'interaxle' axle ids, skipping...");
        return;
    }

    if (m_actor->m_transfer_case)
    {
        if ((m_actor->m_transfer_case->tr_ax_1 == def.a1 && m_actor->m_transfer_case->tr_ax_2 == def.a2) ||
            (m_actor->m_transfer_case->tr_ax_1 == def.a2 && m_actor->m_transfer_case->tr_ax_2 == def.a1))
        {
            AddMessage(Message::TYPE_ERROR, "You cannot have both an inter-axle differential and a transfercase between the same two axles, skipping...");
            return;
        }
    }

    Differential *diff = new Differential();

    diff->di_idx_1 = def.a1;
    diff->di_idx_2 = def.a2;

    if (def.options.size() == 0)
    {
        AddMessage(Message::TYPE_INFO, "No differential defined, defaulting to Locked");
        diff->AddDifferentialType(LOCKED_DIFF);
    }
    else
    {
        auto end = def.options.end();
        for (auto itor = def.options.begin(); itor != end; ++itor)
        {
            switch (*itor)
            {
            case Truck::DifferentialType::DIFF_l_LOCKED:
                diff->AddDifferentialType(LOCKED_DIFF);
                break;
            case Truck::DifferentialType::DIFF_o_OPEN:
                diff->AddDifferentialType(OPEN_DIFF);
                break;
            case Truck::DifferentialType::DIFF_s_SPLIT:
                diff->AddDifferentialType(SPLIT_DIFF);
                break;
            case Truck::DifferentialType::DIFF_v_VISCOUS:
                diff->AddDifferentialType(VISCOUS_DIFF);
                break;
            default:
                AddMessage(Message::TYPE_WARNING, fmt::format("Unknown differential type: '{}'", (char)*itor));
                break;
            }
        }
    }

    m_actor->m_axle_diffs[m_actor->m_num_axle_diffs] = diff;
    m_actor->m_num_axle_diffs++;
}

void ActorSpawner::ProcessTransferCase(Truck::TransferCase & def)
{
    if (def.a1 == def.a2 || def.a1 < 0 || std::max(def.a1 , def.a2) >= m_actor->m_num_wheel_diffs)
    {
        AddMessage(Message::TYPE_ERROR, "Invalid 'transfercase' axle ids, skipping...");
        return;
    }
    if (def.a2 < 0) // No 4WD mode
    {
        if (!def.has_2wd) // No 2WD mode
        {
            AddMessage(Message::TYPE_ERROR, "Invalid 'transfercase': Define alternate axle or allow 2WD, skipping...");
            return;
        }
        else // Only 2WD
        {
            AddMessage(Message::TYPE_INFO, "No alternate axle defined, defaulting to 2WD only");
        }
    }

    m_actor->m_transfer_case = new TransferCase(def.a1, def.a2, def.has_2wd, def.has_2wd_lo, def.gear_ratios);

    for (int i = 0; i < m_actor->ar_num_wheels; i++)
    {
        m_actor->ar_wheels[i].wh_propulsed = false;
    }
    m_actor->ar_wheels[m_actor->m_wheel_diffs[def.a1]->di_idx_1].wh_propulsed = true;
    m_actor->ar_wheels[m_actor->m_wheel_diffs[def.a1]->di_idx_2].wh_propulsed = true;
    m_actor->m_num_proped_wheels = 2;
    if (!def.has_2wd)
    {
        m_actor->ar_wheels[m_actor->m_wheel_diffs[def.a2]->di_idx_1].wh_propulsed = true;
        m_actor->ar_wheels[m_actor->m_wheel_diffs[def.a2]->di_idx_2].wh_propulsed = true;
        m_actor->m_num_proped_wheels = 4;
        m_actor->m_transfer_case->tr_4wd_mode = true;
    }
}

void ActorSpawner::ProcessCruiseControl(Truck::CruiseControl & def)
{
    m_actor->cc_target_speed_lower_limit = def.min_speed;
    if (m_actor->cc_target_speed_lower_limit <= 0.f)
    {
        std::stringstream msg;
        msg << "Invalid parameter 'lower_limit' (" << m_actor->cc_target_speed_lower_limit 
            << ") must be positive nonzero number. Using it anyway (compatibility)";
    }
    m_actor->cc_can_brake = def.autobrake != 0;
}

void ActorSpawner::ProcessSpeedLimiter(Truck::SpeedLimiter & def)
{
    m_actor->sl_speed_limit = def.max_speed;
}

void ActorSpawner::ProcessTorqueCurve(Truck::TorqueCurve & def)
{
    if (m_actor->ar_engine == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, "Section 'torquecurve' found but no 'engine' defined, skipping...");
        return;
    }

    TorqueCurve *target_torque_curve = m_actor->ar_engine->getTorqueCurve();

    if (def.predefined_func_name.length() != 0)
    {
        target_torque_curve->setTorqueModel(def.predefined_func_name);
    }
    else
    {
        target_torque_curve->CreateNewCurve(); /* Use default name for custom curve */
        std::vector<Truck::TorqueCurve::Sample>::iterator itor = def.samples.begin();
        for ( ; itor != def.samples.end(); itor++)
        {
            target_torque_curve->AddCurveSample(itor->power, itor->torque_percent);
        }
    }
}

void ActorSpawner::ProcessParticle(Truck::Particle & def)
{
    if (App::gfx_particles_mode->GetInt() != 1)
    {
        return;
    }

    int particle_index = m_actor->ar_num_custom_particles;
    CParticle & particle = m_actor->ar_custom_particles[particle_index];

    particle.emitterNode = GetNodeIndexOrThrow(def.emitter_node);
    particle.directionNode = GetNodeIndexOrThrow(def.reference_node);

    std::string name = this->ComposeName(def.particle_system_name.c_str(), particle_index);
    particle.psys = this->CreateParticleSystem(name, def.particle_system_name);
    if (particle.psys == nullptr)
    {
        std::stringstream msg;
        msg << "Failed to create particle system '" << name << "' (template: '" << def.particle_system_name <<"')";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return;
    }

    particle.snode = m_particles_parent_scenenode->createChildSceneNode();
    particle.snode->attachObject(particle.psys);
    particle.snode->setPosition(this->GetNode(particle.emitterNode).AbsPosition);

    /* Shut down the emitters */
    particle.active = false; 
    for (unsigned int i = 0; i < particle.psys->getNumEmitters(); i++)
    {
        particle.psys->getEmitter(i)->setEnabled(false);
    }

    ++m_actor->ar_num_custom_particles;
}

void ActorSpawner::ProcessRopable(Truck::Ropable & def)
{
    ropable_t ropable;
    ropable.node = GetNodePointerOrThrow(def.node);
    ropable.pos = static_cast<int>(m_actor->ar_ropables.size());
    ropable.group = def.group;
    ropable.attached_ties = 0;
    ropable.attached_ropes = 0;
    ropable.multilock = def.has_multilock;
    m_actor->ar_ropables.push_back(ropable);
}

void ActorSpawner::ProcessTie(Truck::Tie & def)
{

    node_t & node_1 = GetNodeOrThrow(def.root_node);
    node_t & node_2 = GetNode( (node_1.pos == 0) ? 1 : 0 );

    int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(node_1, node_2);
    beam.strength =  m_state.default_break * m_state.default_break_scale;
    beam.k = m_state.default_spring * m_state.default_spring_scale;
    beam.d = m_state.default_damp * m_state.default_damp_scale;
    beam.bm_type = BEAM_HYDRO;
    beam.L = def.max_reach_length;
    beam.refL = def.max_reach_length;
    beam.bounded = ROPE;
    beam.bm_disabled = true;

    if (!def.is_invisible)
    {
        this->CreateBeamVisuals(beam, beam_index, false);
    }

    /* Register tie */
    tie_t tie;
    tie.ti_group = def.group;
    tie.ti_tying = false;
    tie.ti_tied = false;
    tie.ti_beam = & beam;
    tie.ti_locked_actor   = nullptr;
    tie.ti_locked_ropable = nullptr;
    tie.ti_contract_speed = def.auto_shorten_rate;
    tie.ti_max_stress = def.max_stress;
    tie.ti_min_length = def.min_length;
    tie.ti_no_self_lock = def.disable_self_lock;
    m_actor->ar_ties.push_back(tie);

    m_actor->m_has_command_beams = true;
}

void ActorSpawner::ProcessRope(Truck::Rope & def)
{

    node_t & root_node = GetNodeOrThrow(def.root_node);
    node_t & end_node = GetNodeOrThrow(def.end_node);

    /* Add beam */
    int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(root_node, end_node);
    beam.strength = m_state.default_break * m_state.default_break_scale;
    beam.k = m_state.default_spring * m_state.default_spring_scale;
    beam.d = m_state.default_damp * m_state.default_damp_scale;
    beam.bounded = ROPE;
    beam.bm_type = BEAM_HYDRO;
    beam.L = root_node.AbsPosition.distance(end_node.AbsPosition);
    beam.refL = beam.L;

    this->CreateBeamVisuals(beam, beam_index, true, "tracks/beam");

    /* Register rope */
    rope_t rope;
    rope.rp_beam = & beam;
    rope.rp_locked = UNLOCKED;
    rope.rp_locked_ropable = nullptr;
    rope.rp_group = 0; // Orig: hardcoded in BTS_ROPES. TODO: To be used.
    m_actor->ar_ropes.push_back(rope);
}

void ActorSpawner::ProcessRailGroup(Truck::RailGroup & def)
{
    RailGroup* rail_group = this->CreateRail(def.node_list);
    rail_group->rg_id = def.id;
    m_actor->m_railgroups.push_back(rail_group);
}

void ActorSpawner::ProcessSlidenode(Truck::SlideNode & def)
{
    node_t & node = GetNodeOrThrow(def.slide_node);
    SlideNode slide_node(& node, nullptr);

    // Optional args
    if (def._spring_rate_set)      { slide_node.SetSpringRate(def.spring_rate); }
    if (def._break_force_set)      { slide_node.SetBreakForce(def.break_force); }
    if (def._tolerance_set)        { slide_node.SetCorThreshold(def.tolerance); }
    if (def._attachment_rate_set)  { slide_node.SetAttachmentRate(def.attachment_rate); }
    if (def._max_attach_dist_set)  { slide_node.SetAttachmentDistance(def.max_attach_dist); }

    // Constraints
    if (BITMASK_IS_1(def.constraint_flags, Truck::SlideNode::CONSTRAINT_ATTACH_ALL))
    {
        slide_node.sn_attach_self = true;
        slide_node.sn_attach_foreign = true;
    }
    if (BITMASK_IS_1(def.constraint_flags, Truck::SlideNode::CONSTRAINT_ATTACH_SELF))
    {
        slide_node.sn_attach_self = true;
        slide_node.sn_attach_foreign = false;
    }
    if (BITMASK_IS_1(def.constraint_flags, Truck::SlideNode::CONSTRAINT_ATTACH_FOREIGN))
    {
        slide_node.sn_attach_self = false;
        slide_node.sn_attach_foreign = true;
    }
    if (BITMASK_IS_1(def.constraint_flags, Truck::SlideNode::CONSTRAINT_ATTACH_NONE))
    {
        slide_node.sn_attach_self = false;
        slide_node.sn_attach_foreign = false;
    }

    // RailGroup
    RailGroup *rail_group = nullptr;
    if (def._railgroup_id_set)
    {
        std::vector<RailGroup*>::iterator itor = m_actor->m_railgroups.begin();
        for ( ; itor != m_actor->m_railgroups.end(); itor++)
        {
            if ((*itor)->rg_id == def.railgroup_id)
            {
                rail_group = *itor;
                break;
            }
        }

        if (rail_group == nullptr)
        {
            std::stringstream msg;
            msg << "Specified rail group id '" << def.railgroup_id << "' not found. Ignoring slidenode...";
            AddMessage(Message::TYPE_ERROR, msg.str());
            return;
        }
    }
    else if (def.rail_node_ranges.size() > 0)
    {
        rail_group = this->CreateRail(def.rail_node_ranges);
        if (rail_group != nullptr)
        {
            m_actor->m_railgroups.push_back(rail_group);
        }
    }
    else
    {
        AddMessage(Message::TYPE_ERROR, "No RailGroup available for SlideNode, skipping...");
    }

    slide_node.SetDefaultRail(rail_group);
    m_actor->m_slidenodes.push_back(slide_node);
}

NodeIdx_t ActorSpawner::FindNodeIndex(Truck::Node::Ref & node_ref, bool silent /* = false */)
{
    std::pair<unsigned int, bool> result = GetNodeIndex(node_ref, /* quiet= */ true);
    if (result.second)
    {
        return static_cast<NodeIdx_t>(result.first);
    }
    else
    {
        if (! silent)
        {
            std::stringstream msg;
            msg << "Failed to find node by reference: " << node_ref.ToString();
            AddMessage(Message::TYPE_ERROR, msg.str());
        }
        return node_t::INVALID_IDX;
    }
}

bool ActorSpawner::CollectNodesFromRanges(
    std::vector<Truck::Node::Range> & node_ranges,
    std::vector<NodeIdx_t> & out_node_indices
    )
{
    std::vector<Truck::Node::Range>::iterator itor = node_ranges.begin();
    for ( ; itor != node_ranges.end(); itor++)
    {
        if (itor->IsRange())
        {

            NodeIdx_t start = FindNodeIndex(itor->start, /* silent= */ false);
            if (start == node_t::INVALID_IDX)
            {
                AddMessage(Message::TYPE_WARNING, fmt::format("Invalid start node in range: {}", itor->start.ToString()));
                return false;
            }

            NodeIdx_t end = FindNodeIndex(itor->end,   /* silent= */ true);

            if (end == node_t::INVALID_IDX)
            {
                std::stringstream msg;
                msg << "Encountered non-existent node '" << itor->end.ToString() << "' in range [" << itor->start.ToString() << " - " << itor->end.ToString() << "], "
                    << "highest node index is '" << m_actor->ar_num_nodes - 1 << "'.";

                if (itor->end.Str().empty()) /* If the node is numeric... */
                {
                    msg << " However, this node must be accepted anyway for backwards compatibility."
                        << " Please fix this as soon as possible.";
                    end = itor->end.Num();
                    AddMessage(Message::TYPE_ERROR, msg.str());
                }
                else
                {
                    AddMessage(Message::TYPE_ERROR, msg.str());
                    return false;
                }
            }

            if (end < start)
            {
                NodeIdx_t swap = start;
                start = end;
                end = swap;
            }

            for (NodeIdx_t i = start; i <= end; i++)
            {
                out_node_indices.push_back(i);
            }
        }
        else
        {
            out_node_indices.push_back(GetNodeIndexOrThrow(itor->start));
        }
    }
    return true;
}

RailGroup *ActorSpawner::CreateRail(std::vector<Truck::Node::Range> & node_ranges)
{
    // Collect nodes
    std::vector<NodeIdx_t> node_indices;
    this->CollectNodesFromRanges(node_ranges, node_indices);

    // Build the rail
    RailGroup* rg = new RailGroup();
    for (unsigned int i = 0; i < node_indices.size() - 1; i++)
    {
        beam_t *beam = FindBeamInRig(node_indices[i], node_indices[i + 1]);
        if (beam == nullptr)
        {
            std::stringstream msg;
            msg << "No beam between nodes indexed '" << node_indices[i] << "' and '" << node_indices[i + 1] << "'";
            AddMessage(Message::TYPE_ERROR, msg.str());
            delete rg;
            return nullptr;
        }
        rg->rg_segments.emplace_back(beam);
    }

    // Link middle segments
    const size_t num_seg = rg->rg_segments.size();
    for (size_t i = 1; i < (num_seg - 1); ++i)
    {
        rg->rg_segments[i].rs_prev = &rg->rg_segments[i - 1];
        rg->rg_segments[i].rs_next = &rg->rg_segments[i + 1];
    }

    // Link end segments
    const bool is_loop = (node_indices.front() == node_indices.back());
    if (rg->rg_segments.size() > 1)
    {
        rg->rg_segments[0].rs_next = &rg->rg_segments[1];
        rg->rg_segments[num_seg - 1].rs_prev = &rg->rg_segments[num_seg - 2];
        if (is_loop)
        {
            rg->rg_segments[0].rs_prev = &rg->rg_segments[num_seg - 1];
            rg->rg_segments[num_seg - 1].rs_next = &rg->rg_segments[0];
        }
    }

    return rg; // Transfers memory ownership
}

beam_t *ActorSpawner::FindBeamInRig(NodeIdx_t node_a_index, NodeIdx_t node_b_index)
{
    node_t *node_a = & m_actor->ar_nodes[node_a_index];
    node_t *node_b = & m_actor->ar_nodes[node_b_index];

    for (unsigned int i = 0; i < static_cast<unsigned int>(m_actor->ar_num_beams); i++)
    {
        if	(
                (GetBeam(i).p1 == node_a && GetBeam(i).p2 == node_b)
            ||	(GetBeam(i).p2 == node_a && GetBeam(i).p1 == node_b)
            )
        {
            return & GetBeam(i);
        }
    }
    return nullptr;
}

void ActorSpawner::ProcessHook(Truck::Hook & def)
{
    /* Find the node */
    node_t *node = GetNodePointer(def.node);
    if (node ==  nullptr)
    {
        return;
    }

    /* Find the hook */
    hook_t *hook = nullptr;
    std::vector <hook_t>::iterator itor = m_actor->ar_hooks.begin();
    for (; itor != m_actor->ar_hooks.end(); itor++)
    {
        if (itor->hk_hook_node == node)
        {
            hook = &*itor;
            break;
        }
    }

    if (hook == nullptr)
    {
        std::stringstream msg;
        msg << "Node '" << def.node.ToString() << "' is not a hook-node (not marked with flag 'h'), ignoring...";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return;
    }

    /* Process options */
    hook->hk_lockrange = def.option_hook_range;
    hook->hk_lockspeed = def.option_speed_coef * HOOK_SPEED_DEFAULT;
    hook->hk_maxforce  = def.option_max_force;
    hook->hk_group     = def.option_hookgroup;
    hook->hk_lockgroup = def.option_lockgroup;
    hook->hk_timer     = 0.f; // Hardcoded in BTS_HOOKS
    hook->hk_timer_preset = def.option_timer;
    hook->hk_min_length = def.option_min_range_meters;
    hook->hk_selflock   = def.flag_self_lock;
    hook->hk_nodisable  = def.flag_no_disable;
    if (def.flag_auto_lock)
    {
        hook->hk_autolock = true;
        if (hook->hk_group == -1)
        {
            hook->hk_group = -2; /* only overwrite hgroup when its still default (-1) */
        }
    }
    if (def.flag_no_rope)
    {
        hook->hk_beam->bounded = REGULAR_BEAM;
    }
    if (!def.flag_visible) // NOTE: This flag can only hide a visible beam - it won't show a beam defined with 'invisible' flag.
    {
        // Find beam index
        int beam_index = -1;
        for (int i = 0; i < m_actor->ar_num_beams; ++i)
        {
            if (hook->hk_beam == &m_actor->ar_beams[i])
            {
                beam_index = i;
                break;
            }
        }

        // Find beam visuals in the queue (only exist if defined without 'invisible' flag - we don't know at this point)
        auto itor = m_beam_visuals_queue.begin();
        auto endi = m_beam_visuals_queue.end();
        while (itor != endi)
        {
            if (itor->beam_index == beam_index)
            {
                m_beam_visuals_queue.erase(itor);
                break;
            }
            ++itor;
        }
    }
}

void ActorSpawner::ProcessLockgroup(Truck::Lockgroup & lockgroup)
{
    auto itor = lockgroup.nodes.begin();
    auto end  = lockgroup.nodes.end();
    for (; itor != end; ++itor)
    {
        GetNodeOrThrow(*itor).nd_lockgroup = lockgroup.number;
    }
}

void ActorSpawner::ProcessTrigger(Truck::Trigger & def)
{
    shock_t & shock = this->GetFreeShock();

    // Disable trigger on startup? (default enabled)
    shock.trigger_enabled = !def.HasFlag_x_StartDisabled();

    m_actor->ar_command_key[def.shortbound_trigger_action].trigger_cmdkeyblock_state = false;
    if (def.longbound_trigger_action != -1)
    {
        m_actor->ar_command_key[def.longbound_trigger_action].trigger_cmdkeyblock_state = false;
    }

    unsigned int shock_flags = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISTRIGGER;
    float short_limit = def.contraction_trigger_limit;
    float long_limit = def.expansion_trigger_limit;

    if (def.HasFlag_B_TriggerBlocker())
    {
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_BLOCKER);
    }
    if (def.HasFlag_s_CmdNumSwitch()) // switch that exchanges cmdshort/cmdshort for all triggers with the same commandnumbers, default false
    {
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_CMD_SWITCH);
    }
    if (def.HasFlag_c_CommandStyle()) // // trigger is set with commandstyle boundaries instead of shocksytle
    {
        short_limit = fabs(short_limit - 1);
        long_limit = long_limit - 1;
    }
    if (def.HasFlag_A_InvTriggerBlocker()) // Blocker that enable/disable other triggers, reversed activation method (inverted Blocker style, auto-ON)
    {
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_BLOCKER_A);
    }
    if (def.HasFlag_h_UnlocksHookGroup())
    {
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_HOOK_UNLOCK);
    }
    if (def.HasFlag_H_LocksHookGroup())
    {
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_HOOK_LOCK);
    }
    if (def.HasFlag_t_Continuous())
    {
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_CONTINUOUS); // this trigger sends values between 0 and 1
    }
    if (def.HasFlag_E_EngineTrigger())
    {
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_ENGINE);
    }

    // Checks
    if (!def.IsTriggerBlockerAnyType() && !def.IsHookToggleTrigger() && !def.HasFlag_E_EngineTrigger())
    {
        if (def.shortbound_trigger_action < 1 || def.shortbound_trigger_action > MAX_COMMANDS)
        {
            std::stringstream msg;
            msg << "Invalid value of 'shortbound_trigger_action': '" << def.shortbound_trigger_action << "'. Must be between 1 and "<<MAX_COMMANDS<<". Ignoring trigger.";
            AddMessage(Message::TYPE_ERROR, msg.str());
            return;
        }
    }
    else if (!def.IsHookToggleTrigger() && !def.HasFlag_E_EngineTrigger())
    {
        // this is a Trigger-Blocker, make special check
        if (def.shortbound_trigger_action < 0 || def.longbound_trigger_action < 0)
        {
            AddMessage(Message::TYPE_ERROR, "Wrong command-eventnumber (Triggers). Trigger-Blocker deactivated.");
            return;
        }
    }
    else if (def.HasFlag_E_EngineTrigger())
    {
        if (def.IsTriggerBlockerAnyType() || def.IsHookToggleTrigger() || def.HasFlag_s_CmdNumSwitch())
        {
            AddMessage(Message::TYPE_ERROR, "Wrong command-eventnumber (Triggers). Engine trigger deactivated.");
            return;
        }
    }

    const NodeIdx_t node_1_index = FindNodeIndex(def.nodes[0]);
    const NodeIdx_t node_2_index = FindNodeIndex(def.nodes[1]);
    if (node_1_index == node_t::INVALID_IDX || node_2_index == node_t::INVALID_IDX)
    {
        this->AddMessage(Message::TYPE_WARNING, "Skipping trigger, some nodes not found");
        return;
    }
    int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(GetNode(node_1_index), GetNode(node_2_index));
    beam.bm_type = BEAM_HYDRO;
    SetBeamStrength(beam, m_state.default_break * m_state.default_break_scale);
    SetBeamSpring(beam, 0.f);
    SetBeamDamping(beam, 0.f);
    CalculateBeamLength(beam);
    beam.shortbound = short_limit;
    beam.longbound = long_limit;
    beam.bounded = TRIGGER;
    beam.shock = &shock;

    if (! def.HasFlag_i_Invisible())
    {
        this->CreateBeamVisuals(beam, beam_index, true);
    }

    if (m_actor->m_trigger_debug_enabled)
    {
        LOG("Trigger added. BeamID " + TOSTRING(beam_index));
    }

    shock.beamid = beam_index;
    shock.trigger_switch_state = 0.0f;   // used as bool and countdowntimer, dont touch!

    if (!def.IsTriggerBlockerAnyType())
    {
        shock.trigger_cmdshort = def.shortbound_trigger_action;
        if (def.longbound_trigger_action != -1 || (def.longbound_trigger_action == -1 && def.IsHookToggleTrigger()))
        {
            // this is a trigger or a hook_toggle
            shock.trigger_cmdlong = def.longbound_trigger_action;
        }
        else
        {
            // this is a commandkeyblocker
            shock_flags |= SHOCK_FLAG_TRG_CMD_BLOCKER;
        }
    } 
    else 
    {
        // this is a trigger_blocker
        if (!def.HasFlag_A_InvTriggerBlocker())
        {
            //normal BLOCKER
            shock_flags |= SHOCK_FLAG_TRG_BLOCKER;
            shock.trigger_cmdshort = def.shortbound_trigger_action;
            shock.trigger_cmdlong  = def.longbound_trigger_action;
        } 
        else
        {
            //inverted BLOCKER
            shock_flags |= SHOCK_FLAG_TRG_BLOCKER_A;
            shock.trigger_cmdshort = def.shortbound_trigger_action;
            shock.trigger_cmdlong  = def.longbound_trigger_action;
        }
    }

    if (def.HasFlag_b_KeyBlocker() && !def.HasFlag_B_TriggerBlocker())
    {
        m_actor->ar_command_key[def.shortbound_trigger_action].trigger_cmdkeyblock_state = true;
        if (def.longbound_trigger_action != -1)
        {
            m_actor->ar_command_key[def.longbound_trigger_action].trigger_cmdkeyblock_state = true;
        }
    }

    shock.trigger_boundary_t = def.boundary_timer;
    shock.flags              = shock_flags;
    shock.sbd_spring         = m_state.default_spring;
    shock.sbd_damp           = m_state.default_damp;
    shock.last_debug_state   = 0;
    
}

void ActorSpawner::ProcessContacter(Truck::Node::Ref & node_ref)
{
    unsigned int node_index = GetNodeIndexOrThrow(node_ref);
    m_actor->ar_nodes[node_index].nd_contacter = true;
};

void ActorSpawner::ProcessRotator(Truck::Rotator & def)
{

    rotator_t & rotator = m_actor->ar_rotators[m_actor->ar_num_rotators];

    rotator.angle     = 0;
    rotator.rate      = def.rate;
    rotator.axis1     = GetNodeIndexOrThrow(def.axis_nodes[0]);
    rotator.axis2     = GetNodeIndexOrThrow(def.axis_nodes[1]);
    rotator.force     = ROTATOR_FORCE_DEFAULT;
    rotator.tolerance = ROTATOR_TOLERANCE_DEFAULT;
    rotator.engine_coupling = def.engine_coupling;
    rotator.needs_engine = def.needs_engine;
    for (unsigned int i = 0; i < 4; i++)
    {
        rotator.nodes1[i] = GetNodeIndexOrThrow(def.base_plate_nodes[i]);
        rotator.nodes2[i] = GetNodeIndexOrThrow(def.rotating_plate_nodes[i]);
    }

    // Validate the reference structure
    this->ValidateRotator(m_actor->ar_num_rotators + 1, rotator.axis1, rotator.axis2, rotator.nodes1, rotator.nodes2);

    // Rotate left key
    m_actor->ar_command_key[def.spin_left_key].rotators.push_back(- (m_actor->ar_num_rotators + 1));
    m_actor->ar_command_key[def.spin_left_key].description = "Rotate_Left/Right";

    // Rotate right key
    m_actor->ar_command_key[def.spin_right_key].rotators.push_back(m_actor->ar_num_rotators + 1);

    this->_ProcessKeyInertia(def.inertia,
                             m_actor->ar_command_key[def.spin_left_key].rotator_inertia,
                             m_actor->ar_command_key[def.spin_right_key].rotator_inertia);

    m_actor->ar_num_rotators++;
    m_actor->m_has_command_beams = true;
}

void ActorSpawner::ProcessRotator2(Truck::Rotator2 & def)
{
    rotator_t & rotator = m_actor->ar_rotators[m_actor->ar_num_rotators];

    rotator.angle = 0;
    rotator.rate = def.rate;
    rotator.axis1 = GetNodeIndexOrThrow(def.axis_nodes[0]);
    rotator.axis2     = GetNodeIndexOrThrow(def.axis_nodes[1]);
    rotator.force     = def.rotating_force; // Default value is set in constructor
    rotator.tolerance = def.tolerance; // Default value is set in constructor
    rotator.engine_coupling = def.engine_coupling;
    rotator.needs_engine = def.needs_engine;
    for (unsigned int i = 0; i < 4; i++)
    {
        rotator.nodes1[i] = GetNodeIndexOrThrow(def.base_plate_nodes[i]);
        rotator.nodes2[i] = GetNodeIndexOrThrow(def.rotating_plate_nodes[i]);
    }

    // Validate the reference structure
    this->ValidateRotator(m_actor->ar_num_rotators + 1, rotator.axis1, rotator.axis2, rotator.nodes1, rotator.nodes2);

    // Rotate left key
    m_actor->ar_command_key[def.spin_left_key].rotators.push_back(- (m_actor->ar_num_rotators + 1));
    if (! def.description.empty())
    {
        m_actor->ar_command_key[def.spin_left_key].description = def.description;
    }
    else
    {
        m_actor->ar_command_key[def.spin_left_key].description = "Rotate_Left/Right";
    }

    // Rotate right key
    m_actor->ar_command_key[def.spin_right_key].rotators.push_back(m_actor->ar_num_rotators + 1);

    this->_ProcessKeyInertia(def.inertia,
                             m_actor->ar_command_key[def.spin_left_key].rotator_inertia,
                             m_actor->ar_command_key[def.spin_right_key].rotator_inertia);

    m_actor->ar_num_rotators++;
    m_actor->m_has_command_beams = true;
}

void ActorSpawner::_ProcessKeyInertia(
    Truck::BaseInertia & inertia,
    RoR::CmdKeyInertia& contract_cmd,
    RoR::CmdKeyInertia& extend_cmd
)
{
    /* Handle placeholders */
    Ogre::String start_function;
    Ogre::String stop_function;
    if (! inertia.start_function.empty() && inertia.start_function != "/" && inertia.start_function != "-")
    {
        start_function = inertia.start_function;
    }
    if (! inertia.stop_function.empty() && inertia.stop_function != "/" && inertia.stop_function != "-")
    {
        stop_function = inertia.stop_function;
    }
    if (inertia.start_delay_factor != 0.f && inertia.stop_delay_factor != 0.f)
    {
        contract_cmd.SetCmdKeyDelay(
            App::GetGameContext()->GetActorManager()->GetInertiaConfig(),
            inertia.start_delay_factor,
            inertia.stop_delay_factor,
            start_function,
            stop_function
        );

        extend_cmd.SetCmdKeyDelay(
            App::GetGameContext()->GetActorManager()->GetInertiaConfig(),
            inertia.start_delay_factor,
            inertia.stop_delay_factor,
            start_function,
            stop_function
        );
    }
    else if (m_state.inertia_startDelay > 0 || m_state.inertia_stopDelay > 0)
    {
        contract_cmd.SetCmdKeyDelay(
            App::GetGameContext()->GetActorManager()->GetInertiaConfig(),
            m_state.inertia_startDelay,
            m_state.inertia_stopDelay,
            m_state.inertia_default_startFunction,
            m_state.inertia_default_stopFunction
        );

        extend_cmd.SetCmdKeyDelay(
            App::GetGameContext()->GetActorManager()->GetInertiaConfig(),
            m_state.inertia_startDelay,
            m_state.inertia_stopDelay,
            m_state.inertia_default_startFunction,
            m_state.inertia_default_stopFunction
        );
    }
}

void ActorSpawner::ProcessCommand(Truck::Command2 & def)
{

    // Set up beam
    int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(def.nodes[0], def.nodes[1]);
    CalculateBeamLength(beam);
    beam.strength = m_state.default_break * m_state.default_break_scale;
    beam.k = m_state.default_spring * m_state.default_spring_scale;
    beam.d = m_state.default_damp * m_state.default_damp_scale;
    beam.bm_type = BEAM_HYDRO;

    /* Options */
    if (def.option_r_rope)          { beam.bounded = ROPE; }

    /* set the middle of the command, so its not required to recalculate this everytime ... */
    float center_length = 0.f;
    if (def.max_extension > def.max_contraction)
    {
        center_length = (def.max_extension - def.max_contraction) / 2 + def.max_contraction;
    }
    else
    {
        center_length = (def.max_contraction - def.max_extension) / 2 + def.max_extension;
    }

    /* Add keys */
    command_t* contract_command = &m_actor->ar_command_key[def.contract_key];
    commandbeam_t cmd_beam;
    cmd_beam.cmb_beam_index = static_cast<uint16_t>(beam_index);
    cmd_beam.cmb_is_contraction = true;
    cmd_beam.cmb_speed = def.shorten_rate;
    cmd_beam.cmb_boundary_length = def.max_contraction;
    cmd_beam.cmb_is_force_restricted = def.option_f_not_faster;
    cmd_beam.cmb_is_autocentering = def.option_c_auto_center;
    cmd_beam.cmb_needs_engine = def.needs_engine;
    cmd_beam.cmb_is_1press = def.option_p_1press;      
    cmd_beam.cmb_is_1press_center = def.option_o_1press_center;
    cmd_beam.cmb_plays_sound = def.plays_sound;
    cmd_beam.cmb_engine_coupling = def.affect_engine;
    cmd_beam.cmb_center_length = center_length;
    cmd_beam.cmb_state = std::shared_ptr<commandbeam_state_t>(new commandbeam_state_t);
    contract_command->beams.push_back(cmd_beam);
    if (contract_command->description.empty())
    {
        contract_command->description = def.description;
    }

    command_t* extend_command = &m_actor->ar_command_key[def.extend_key];
    cmd_beam.cmb_is_contraction = false;
    cmd_beam.cmb_speed = def.lengthen_rate;
    cmd_beam.cmb_boundary_length = def.max_extension;
    extend_command->beams.push_back(cmd_beam);
    if (extend_command->description.empty())
    {
        extend_command->description = def.description;
    }

    this->_ProcessKeyInertia(def.inertia,
                             contract_command->command_inertia,
                             extend_command->command_inertia);

    if (! def.option_i_invisible)
    {
        this->CreateBeamVisuals(beam, beam_index, true);
    }

    m_actor->m_num_command_beams++;
    m_actor->m_has_command_beams = true;
}

void ActorSpawner::ProcessAnimator(Truck::Animator & def)
{
    int anim_flags = 0;
    float anim_option = 0;

    /* Options. '{' intentionally misplaced. */

    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_AIRSPEED)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AIRSPEED);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_VERTICAL_VELOCITY)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_VVI);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_ANGLE_OF_ATTACK)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AOA);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_FLAP)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_FLAP);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_AIR_BRAKE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AIRBRAKE);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_ROLL))	{
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ROLL);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_PITCH)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_PITCH);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_BRAKES)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_BRAKE);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_ACCEL)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ACCEL);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_CLUTCH)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_CLUTCH);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_SPEEDO)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SPEEDO);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_TACHO)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_TACHO);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_TURBO)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_TURBO);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_PARKING)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_PBRAKE);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_TORQUE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_TORQUE);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_BOAT_THROTTLE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_BTHROTTLE);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_BOAT_RUDDER)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_BRUDDER);
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_SHIFT_LEFT_RIGHT)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
        anim_option = 1.f;
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_SHIFT_BACK_FORTH))	{
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
        anim_option = 2.f;
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_SEQUENTIAL_SHIFT)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
        anim_option = 3.f;
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_GEAR_SELECT)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
        anim_option = 4.f;
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_ALTIMETER_100K)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ALTIMETER);
        anim_option = 1.f;
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_ALTIMETER_10K)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ALTIMETER);
        anim_option = 2.f;
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_ALTIMETER_1K)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ALTIMETER);
        anim_option = 3.f;
    }
    
    /* Aerial */
    if (BITMASK_IS_1(def.aero_animator.flags, Truck::AeroAnimator::OPTION_THROTTLE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_THROTTLE);
        anim_option = static_cast<float>(def.aero_animator.engine_idx);
    }
    if (BITMASK_IS_1(def.aero_animator.flags, Truck::AeroAnimator::OPTION_RPM)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_RPM);
        anim_option = static_cast<float>(def.aero_animator.engine_idx);
    }
    if (BITMASK_IS_1(def.aero_animator.flags, Truck::AeroAnimator::OPTION_TORQUE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AETORQUE);
        anim_option = static_cast<float>(def.aero_animator.engine_idx);
    }
    if (BITMASK_IS_1(def.aero_animator.flags, Truck::AeroAnimator::OPTION_PITCH)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AEPITCH);
        anim_option = static_cast<float>(def.aero_animator.engine_idx);
    }
    if (BITMASK_IS_1(def.aero_animator.flags, Truck::AeroAnimator::OPTION_STATUS)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AESTATUS);
        anim_option = static_cast<float>(def.aero_animator.engine_idx);
    }

    
    // Set up beam
    unsigned int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(def.nodes[0], def.nodes[1]);
    /* set the limits to something with sense by default */
    beam.shortbound = 0.99999f;
    beam.longbound = 1000000.0f;
    beam.bm_type = BEAM_HYDRO;
    CalculateBeamLength(beam);
    beam.strength = m_state.default_break * m_state.default_break_scale;
    beam.k = m_state.default_spring * m_state.default_spring_scale;
    beam.d = m_state.default_damp * m_state.default_damp_scale;

    if (BITMASK_IS_0(def.flags, Truck::Animator::OPTION_INVISIBLE))
    {
        this->CreateBeamVisuals(beam, beam_index, true);
    }

    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_SHORT_LIMIT)) 
    {
        beam.shortbound = def.short_limit;
    }
    if (BITMASK_IS_1(def.flags, Truck::Animator::OPTION_LONG_LIMIT)) 
    {
        beam.longbound = def.long_limit;
    }

    hydrobeam_t hb;
    hb.hb_beam_index = static_cast<uint16_t>(beam_index);
    hb.hb_speed = def.lenghtening_factor;
    hb.hb_ref_length = beam.L;
    hb.hb_flags = 0;
    hb.hb_anim_flags = anim_flags;
    hb.hb_anim_param = anim_option;


    if (m_state.inertia_startDelay > 0 && m_state.inertia_stopDelay > 0)
    {
        hb.hb_inertia.SetCmdKeyDelay(
            App::GetGameContext()->GetActorManager()->GetInertiaConfig(),
            m_state.inertia_startDelay,
            m_state.inertia_stopDelay,
            m_state.inertia_default_startFunction,
            m_state.inertia_default_stopFunction
        );
    }

    m_actor->ar_hydros.push_back(hb);
}

beam_t & ActorSpawner::AddBeam(node_t & node_1, node_t & node_2)
{
    beam_t & beam = GetAndInitFreeBeam(node_1, node_2);
    beam.detacher_group = m_state.detacher_group_state;
    beam.bm_disabled = false;

    beam.strength = m_state.default_break;

    beam.default_beam_deform = m_state.default_deform * m_state.default_deform_scale;
    beam.minmaxposnegstress = m_state.default_deform * m_state.default_deform_scale;
    beam.maxposstress = m_state.default_deform * m_state.default_deform_scale;
    beam.maxnegstress = m_state.default_deform * m_state.default_deform_scale;

    beam.plastic_coef = m_state.default_plastic_coef;

    return beam;
}

beam_t& ActorSpawner::AddBeam(Truck::Node::Ref n1, Truck::Node::Ref n2)
{
    return this->AddBeam(
        m_actor->ar_nodes[this->ResolveNodeRef(n1)],
        m_actor->ar_nodes[this->ResolveNodeRef(n2)]);
}

void ActorSpawner::SetBeamStrength(beam_t & beam, float strength)
{
    beam.strength = strength;
}

void ActorSpawner::ProcessHydro(Truck::Hydro & def)
{
    bool invisible = false;
    unsigned int hydro_flags = 0;

    // Parse options
    if (def.options.empty()) // Parse as if option 'n' (OPTION_n_NORMAL) was present
    {
        invisible = false;
        hydro_flags |= HYDRO_FLAG_DIR;
    }
    else
    {
        for (unsigned int i = 0; i < def.options.length(); ++i)
        {
            const char c = def.options[i];
            switch (c)
            {
                case Truck::Hydro::OPTION_i_INVISIBLE:  // i
                    invisible = true;
                    break;
                case Truck::Hydro::OPTION_n_NORMAL:  // n
                    invisible = false;
                    hydro_flags |= HYDRO_FLAG_DIR;
                    break;
                case Truck::Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED:  // 's': // speed changing hydro
                    hydro_flags |= HYDRO_FLAG_SPEED;
                    break;
                case Truck::Hydro::OPTION_a_INPUT_AILERON:  // 'a':
                    hydro_flags |= HYDRO_FLAG_AILERON;
                    break;
                case Truck::Hydro::OPTION_r_INPUT_RUDDER:  // 'r':
                    hydro_flags |= HYDRO_FLAG_RUDDER;
                    break;
                case Truck::Hydro::OPTION_e_INPUT_ELEVATOR:  // 'e':
                    hydro_flags |= HYDRO_FLAG_ELEVATOR;
                    break;
                case Truck::Hydro::OPTION_u_INPUT_AILERON_ELEVATOR:  // 'u':
                    hydro_flags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_ELEVATOR);
                    break;
                case Truck::Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR:  // 'v':
                    hydro_flags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_ELEVATOR);
                    break;
                case Truck::Hydro::OPTION_x_INPUT_AILERON_RUDDER:  // 'x':
                    hydro_flags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_RUDDER);
                    break;
                case Truck::Hydro::OPTION_y_INPUT_InvAILERON_RUDDER:  // 'y':
                    hydro_flags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_RUDDER);
                    break;
                case Truck::Hydro::OPTION_g_INPUT_ELEVATOR_RUDDER:  // 'g':
                    hydro_flags |= (HYDRO_FLAG_ELEVATOR | HYDRO_FLAG_RUDDER);
                    break;
                case Truck::Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER:  // 'h':
                    hydro_flags |= (HYDRO_FLAG_REV_ELEVATOR | HYDRO_FLAG_RUDDER);
                    break;
                default:
                    this->AddMessage(Message::TYPE_WARNING, std::string("Ignoring invalid flag:") + c);
                    break;
            }
            
            // NOTE: This is a quirk ported from v0.4.0.7 spawner (for compatibility)
            //       This code obviously belongs after the options-loop.
            //       However, since it's inside the loop, it only works correctly if the 'i' flag is last.
            //
            // ORIGINAL COMMENT: if you use the i flag on its own, add the direction to it
            if (invisible && !hydro_flags)
            {
                hydro_flags |= HYDRO_FLAG_DIR;
            }
        }
    }


    // Set up beam
    int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(def.nodes[0], def.nodes[1]);
    CalculateBeamLength(beam);
    beam.bm_type              = BEAM_HYDRO;
    beam.strength             = m_state.default_break * m_state.default_break_scale;
    beam.k                    = m_state.default_spring * m_state.default_spring_scale;
    beam.d                    = m_state.default_damp * m_state.default_damp_scale;

    if (!invisible)
    {
        this->CreateBeamVisuals(beam, beam_index, true);
    }

    hydrobeam_t hb;
    hb.hb_flags = hydro_flags;
    hb.hb_speed = def.lenghtening_factor;
    hb.hb_beam_index = static_cast<uint16_t>(beam_index);
    hb.hb_ref_length = beam.L;
    hb.hb_anim_flags = 0;
    hb.hb_anim_param = 0.f;
    this->_ProcessKeyInertia(def.inertia, hb.hb_inertia, hb.hb_inertia);

    m_actor->ar_hydros.push_back(hb);
}

void ActorSpawner::ProcessShock3(Truck::Shock3 & def)
{


    node_t & node_1 = GetNode(def.nodes[0]);
    node_t & node_2 = GetNode(def.nodes[1]);
    float short_bound = def.short_bound;
    float long_bound = def.long_bound;
    unsigned int shock_flags = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISSHOCK3;

    if (BITMASK_IS_1(def.options, Truck::Shock3::OPTION_m_METRIC))
    {
        float beam_length = node_1.AbsPosition.distance(node_2.AbsPosition);
        short_bound /= beam_length;
        long_bound /= beam_length;
    }
    if (BITMASK_IS_1(def.options, Truck::Shock3::OPTION_M_ABSOLUTE_METRIC))
    {
        float beam_length = node_1.AbsPosition.distance(node_2.AbsPosition);
        short_bound = (beam_length - short_bound) / beam_length;
        long_bound = (long_bound - beam_length) / beam_length;

        if (long_bound < 0.f)
        {
            AddMessage(
                Message::TYPE_WARNING, 
                "Metric shock length calculation failed, 'short_bound' less than beams spawn length. Resetting to beam's spawn length (short_bound = 0)"
            );
            long_bound = 0.f;
        }

        if (short_bound > 1.f)
        {
            AddMessage(
                Message::TYPE_WARNING, 
                "Metric shock length calculation failed, 'short_bound' less than 0 meters. Resetting to 0 meters (short_bound = 1)"
            );
            short_bound = 1.f;
        }
    }


    int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(node_1, node_2);
    beam.strength = m_state.default_break * m_state.default_break_scale;
    beam.bm_type              = BEAM_HYDRO;
    beam.bounded              = SHOCK3;
    beam.k                    = def.spring_in;
    beam.d                    = def.damp_in;
    beam.shortbound           = short_bound;
    beam.longbound            = long_bound;

    /* Length + pre-compression */
    CalculateBeamLength(beam);
    beam.L          *= def.precompression;
    beam.refL       *= def.precompression;

    if (BITMASK_IS_0(def.options, Truck::Shock3::OPTION_i_INVISIBLE))
    {
        this->CreateBeamVisuals(beam, beam_index, true);
    }

    shock_t & shock  = GetFreeShock();
    shock.flags      = shock_flags;
    shock.sbd_spring = m_state.default_spring;
    shock.sbd_damp   = m_state.default_damp;
    shock.springin   = def.spring_in;
    shock.dampin     = def.damp_in;
    shock.springout  = def.spring_out;
    shock.dampout    = def.damp_out;
    shock.splitin    = def.split_vel_in;
    shock.dslowin    = def.damp_in_slow;
    shock.dfastin    = def.damp_in_fast;
    shock.splitout   = def.split_vel_out;
    shock.dslowout   = def.damp_out_slow;
    shock.dfastout   = def.damp_out_fast;

    beam.shock = & shock;
    shock.beamid = beam_index;
}

void ActorSpawner::ProcessShock2(Truck::Shock2 & def)
{
    node_t & node_1 = GetNode(def.nodes[0]);
    node_t & node_2 = GetNode(def.nodes[1]);
    float short_bound = def.short_bound;
    float long_bound = def.long_bound;
    unsigned int shock_flags = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISSHOCK2;

    if (BITMASK_IS_1(def.options, Truck::Shock2::OPTION_s_SOFT_BUMP_BOUNDS))
    {
        BITMASK_SET_0(shock_flags, SHOCK_FLAG_NORMAL); /* Not normal anymore */
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_SOFTBUMP);
    }
    if (BITMASK_IS_1(def.options, Truck::Shock2::OPTION_m_METRIC))
    {
        float beam_length = node_1.AbsPosition.distance(node_2.AbsPosition);
        short_bound /= beam_length;
        long_bound /= beam_length;
    }
    if (BITMASK_IS_1(def.options, Truck::Shock2::OPTION_M_ABSOLUTE_METRIC))
    {
        float beam_length = node_1.AbsPosition.distance(node_2.AbsPosition);
        short_bound = (beam_length - short_bound) / beam_length;
        long_bound = (long_bound - beam_length) / beam_length;

        if (long_bound < 0.f)
        {
            AddMessage(
                Message::TYPE_WARNING, 
                "Metric shock length calculation failed, 'short_bound' less than beams spawn length. Resetting to beam's spawn length (short_bound = 0)"
            );
            long_bound = 0.f;
        }

        if (short_bound > 1.f)
        {
            AddMessage(
                Message::TYPE_WARNING, 
                "Metric shock length calculation failed, 'short_bound' less than 0 meters. Resetting to 0 meters (short_bound = 1)"
            );
            short_bound = 1.f;
        }
    }


    // Set up beam
    int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(node_1, node_2);
    beam.strength             = m_state.default_break * m_state.default_break_scale;
    beam.bm_type              = BEAM_HYDRO;
    beam.bounded              = SHOCK2;
    beam.k                    = def.spring_in;
    beam.d                    = def.damp_in;
    beam.shortbound           = short_bound;
    beam.longbound            = long_bound;

    /* Length + pre-compression */
    CalculateBeamLength(beam);
    beam.L          *= def.precompression;
    beam.refL       *= def.precompression;

    if (BITMASK_IS_0(def.options, Truck::Shock2::OPTION_i_INVISIBLE))
    {
        this->CreateBeamVisuals(beam, beam_index, true);
    }

    shock_t & shock  = GetFreeShock();
    shock.flags      = shock_flags;
    shock.sbd_spring = m_state.default_spring;
    shock.sbd_damp   = m_state.default_damp;
    shock.springin   = def.spring_in;
    shock.dampin     = def.damp_in;
    shock.springout  = def.spring_out;
    shock.dampout    = def.damp_out;
    shock.sprogin    = def.progress_factor_spring_in;
    shock.dprogin    = def.progress_factor_damp_in;
    shock.sprogout   = def.progress_factor_spring_out;
    shock.dprogout   = def.progress_factor_damp_out;

    beam.shock = & shock;
    shock.beamid = beam_index;
}

void ActorSpawner::ProcessShock(Truck::Shock & def)
{

    // Set up beam
    int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(def.nodes[0], def.nodes[1]);

    // Resolve options
    float short_bound = def.short_bound;
    float long_bound = def.long_bound;
    unsigned int shock_flags = SHOCK_FLAG_NORMAL;

    if (BITMASK_IS_1(def.options, Truck::Shock::OPTION_L_ACTIVE_LEFT))
    {
        BITMASK_SET_0(shock_flags, SHOCK_FLAG_NORMAL); /* Not normal anymore */
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_LACTIVE);
        m_actor->ar_has_active_shocks = true;
    }
    if (BITMASK_IS_1(def.options, Truck::Shock::OPTION_R_ACTIVE_RIGHT))
    {
        BITMASK_SET_0(shock_flags, SHOCK_FLAG_NORMAL); /* Not normal anymore */
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_RACTIVE);
        m_actor->ar_has_active_shocks = true;
    }
    if (BITMASK_IS_1(def.options, Truck::Shock::OPTION_m_METRIC))
    {
        float beam_length = beam.p1->AbsPosition.distance(beam.p2->AbsPosition);
        short_bound /= beam_length;
        long_bound /= beam_length;
    }

    beam.shortbound = short_bound;
    beam.longbound  = long_bound;
    beam.bounded    = SHOCK1;
    beam.bm_type    = BEAM_HYDRO;
    beam.k          = def.spring_rate;
    beam.d          = def.damping;
    SetBeamStrength(beam, m_state.default_break * 4.f);

    /* Length + pre-compression */
    CalculateBeamLength(beam);
    beam.L          *= def.precompression;
    beam.refL       *= def.precompression;

    shock_t & shock  = GetFreeShock();
    shock.flags      = shock_flags;
    shock.sbd_spring = m_state.default_spring;
    shock.sbd_damp   = m_state.default_damp;

    if (BITMASK_IS_0(def.options, Truck::Shock::OPTION_i_INVISIBLE))
    {
        this->CreateBeamVisuals(beam, beam_index, true);
    }

    beam.shock = & shock;
    shock.beamid = beam_index;
}

void ActorSpawner::FetchAxisNodes(
    node_t* & axis_node_1, 
    node_t* & axis_node_2, 
    Truck::Node::Ref const & axis_node_1_id,
    Truck::Node::Ref const & axis_node_2_id
)
{
    axis_node_1 = GetNodePointer(axis_node_1_id);
    axis_node_2 = GetNodePointer(axis_node_2_id);

    Ogre::Vector3 pos_1 = axis_node_1->AbsPosition;
    Ogre::Vector3 pos_2 = axis_node_2->AbsPosition;

    /* Enforce the "second node must have a larger Z coordinate than the first" constraint */
    if (pos_1.z > pos_2.z)
    {
        node_t *swap = axis_node_1;
        axis_node_1 = axis_node_2;
        axis_node_2 = swap;
    }
}

void ActorSpawner::ProcessFlexBodyWheel(Truck::FlexBodyWheel & def)
{
    unsigned int base_node_index = m_actor->ar_num_nodes;
    wheel_t & wheel = m_actor->ar_wheels[m_actor->ar_num_wheels];

    node_t *axis_node_1 = nullptr;
    node_t *axis_node_2 = nullptr;
    FetchAxisNodes(axis_node_1, axis_node_2, def.nodes[0], def.nodes[1]);

    // Rigidity node
    node_t *rigidity_node = nullptr;
    node_t *axis_node_closest_to_rigidity_node = nullptr;
    if (def.rigidity_node.IsValidAnyState())
    {
        rigidity_node = GetNodePointer(def.rigidity_node);
        Ogre::Real distance_1 = (rigidity_node->RelPosition - axis_node_1->RelPosition).length();
        Ogre::Real distance_2 = (rigidity_node->RelPosition - axis_node_2->RelPosition).length();
        axis_node_closest_to_rigidity_node = ((distance_1 < distance_2)) ? axis_node_1 : axis_node_2;
    }

    // Node&beam generation
    Ogre::Vector3 axis_vector = axis_node_2->RelPosition - axis_node_1->RelPosition;
    wheel.wh_width = axis_vector.length(); // wheel_def.width is ignored.
    axis_vector.normalise();
    Ogre::Vector3 rim_ray_vector = axis_vector.perpendicular() * def.rim_radius;
    Ogre::Quaternion rim_ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.f / (def.num_rays * 2)), axis_vector);

    // Rim nodes
    for (unsigned int i = 0; i < def.num_rays; i++)
    {
        float node_mass = def.mass / (4.f * def.num_rays);

        // Outer ring
        Ogre::Vector3 ray_point = axis_node_1->RelPosition + rim_ray_vector;
        rim_ray_vector = rim_ray_rotator * rim_ray_vector;

        node_t & outer_node      = GetFreeNode();
        InitNode(outer_node, ray_point);

        outer_node.mass          = node_mass;
        outer_node.friction_coef = m_state.default_node_friction;
        outer_node.nd_rim_node   = true;
        AdjustNodeBuoyancy(outer_node);
        m_actor->ar_minimass[outer_node.pos] = m_state.default_minimass;

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(outer_node.pos)));

        // Inner ring
        ray_point = axis_node_2->RelPosition + rim_ray_vector;
        rim_ray_vector = rim_ray_rotator * rim_ray_vector;

        node_t & inner_node      = GetFreeNode();
        InitNode(inner_node, ray_point);

        inner_node.mass          = node_mass;
        inner_node.friction_coef = m_state.default_node_friction;
        inner_node.nd_rim_node   = true;
        AdjustNodeBuoyancy(inner_node);
        m_actor->ar_minimass[inner_node.pos] = m_state.default_minimass;

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(inner_node.pos)));

        // Wheel object
        wheel.wh_rim_nodes[i * 2]       = & outer_node;
        wheel.wh_rim_nodes[(i * 2) + 1] = & inner_node;
    }

    Ogre::Vector3 tyre_ray_vector = axis_vector.perpendicular() * def.tyre_radius;
    Ogre::Quaternion& tyre_ray_rotator = rim_ray_rotator;
    tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

    // Tyre nodes
    for (unsigned int i = 0; i < def.num_rays; i++)
    {
        /* Outer ring */
        float node_mass = def.mass / (4.f * def.num_rays);
        Ogre::Vector3 ray_point = axis_node_1->RelPosition + tyre_ray_vector;
        tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

        node_t & outer_node = GetFreeNode();
        InitNode(outer_node, ray_point);
        outer_node.mass          = node_mass;
        outer_node.friction_coef = m_state.default_node_friction;
        outer_node.volume_coef   = m_state.default_node_volume;
        outer_node.surface_coef  = m_state.default_node_surface;
        outer_node.nd_contacter  = true;
        outer_node.nd_tyre_node  = true;
        AdjustNodeBuoyancy(outer_node);

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(outer_node.pos)));

        // Inner ring
        ray_point = axis_node_2->RelPosition + tyre_ray_vector;
        tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

        node_t & inner_node = GetFreeNode();
        InitNode(inner_node, ray_point);
        inner_node.mass          = node_mass;
        inner_node.friction_coef = m_state.default_node_friction;
        inner_node.volume_coef   = m_state.default_node_surface;
        inner_node.surface_coef  = m_state.default_node_surface;
        inner_node.nd_contacter  = true;
        inner_node.nd_tyre_node  = true;
        AdjustNodeBuoyancy(inner_node);

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(inner_node.pos)));

        // Wheel object
        wheel.wh_nodes[i * 2] = & outer_node;
        wheel.wh_nodes[(i * 2) + 1] = & inner_node;
    }

    // Beams
    float rim_spring = def.rim_springiness;
    float rim_damp = def.rim_damping;
    float tyre_spring = def.tyre_springiness;
    float tyre_damp = def.tyre_damping;
    float tread_spring = m_state.default_spring;
    float tread_damp = m_state.default_damp;

    for (unsigned int i = 0; i < def.num_rays; i++)
    {
        // --- Rim --- 

        // Rim axis to rim ring
        unsigned int rim_outer_node_index = base_node_index + (i * 2);
        node_t *rim_outer_node = & m_actor->ar_nodes[rim_outer_node_index];
        node_t *rim_inner_node = & m_actor->ar_nodes[rim_outer_node_index + 1];

        AddWheelBeam(axis_node_1, rim_outer_node, rim_spring, rim_damp);
        AddWheelBeam(axis_node_2, rim_inner_node, rim_spring, rim_damp);
        AddWheelBeam(axis_node_2, rim_outer_node, rim_spring, rim_damp);
        AddWheelBeam(axis_node_1, rim_inner_node, rim_spring, rim_damp);

        // Reinforcement rim ring
        unsigned int rim_next_outer_node_index = base_node_index + (((i + 1) % def.num_rays) * 2);
        node_t *rim_next_outer_node = & m_actor->ar_nodes[rim_next_outer_node_index];
        node_t *rim_next_inner_node = & m_actor->ar_nodes[rim_next_outer_node_index + 1];

        AddWheelBeam(rim_outer_node, rim_inner_node,      rim_spring, rim_damp);
        AddWheelBeam(rim_outer_node, rim_next_outer_node, rim_spring, rim_damp);
        AddWheelBeam(rim_inner_node, rim_next_inner_node, rim_spring, rim_damp);
        AddWheelBeam(rim_inner_node, rim_next_outer_node, rim_spring, rim_damp);
    }

    // Tyre beams
    // Quick&dirty port from original SerializedRig::addWheel3()
    for (unsigned int i = 0; i < def.num_rays; i++)
    {
        int rim_node_index    = base_node_index + i*2;
        int tyre_node_index   = base_node_index + i*2 + def.num_rays*2;
        node_t * rim_node     = & m_actor->ar_nodes[rim_node_index];

        AddWheelBeam(rim_node, & m_actor->ar_nodes[tyre_node_index], tyre_spring/2.f, tyre_damp);

        int tyre_base_index = (i == 0) ? tyre_node_index + (def.num_rays * 2) : tyre_node_index;
        AddWheelBeam(rim_node, & m_actor->ar_nodes[tyre_base_index - 1], tyre_spring/2.f, tyre_damp);
        AddWheelBeam(rim_node, & m_actor->ar_nodes[tyre_base_index - 2], tyre_spring/2.f, tyre_damp);

        node_t * next_rim_node = & m_actor->ar_nodes[rim_node_index + 1];
        AddWheelBeam(next_rim_node, & m_actor->ar_nodes[tyre_node_index],     tyre_spring/2.f, tyre_damp);
        AddWheelBeam(next_rim_node, & m_actor->ar_nodes[tyre_node_index + 1], tyre_spring/2.f, tyre_damp);

        {
            int index = (i == 0) ? tyre_node_index + (def.num_rays * 2) - 1 : tyre_node_index - 1;
            node_t * tyre_node = & m_actor->ar_nodes[index];
            AddWheelBeam(next_rim_node, tyre_node, tyre_spring/2.f, tyre_damp);
        }

        //reinforcement (tire tread)
        {
            // Very messy port :(
            // Aliases
            int rimnode = rim_node_index;
            int rays = def.num_rays;

            AddWheelBeam(&m_actor->ar_nodes[rimnode+rays*2], &m_actor->ar_nodes[base_node_index+i*2+1+rays*2], tread_spring, tread_damp);
            AddWheelBeam(&m_actor->ar_nodes[rimnode+rays*2], &m_actor->ar_nodes[base_node_index+((i+1)%rays)*2+rays*2], tread_spring, tread_damp);
            AddWheelBeam(&m_actor->ar_nodes[base_node_index+i*2+1+rays*2], &m_actor->ar_nodes[base_node_index+((i+1)%rays)*2+1+rays*2], tread_spring, tread_damp);
            AddWheelBeam(&m_actor->ar_nodes[rimnode+1+rays*2], &m_actor->ar_nodes[base_node_index+((i+1)%rays)*2+rays*2], tread_spring, tread_damp);

            if (rigidity_node != nullptr)
            {
                if (axis_node_closest_to_rigidity_node == axis_node_1)
                {
                    axis_node_closest_to_rigidity_node = & m_actor->ar_nodes[base_node_index+i*2+rays*2];
                } else
                {
                    axis_node_closest_to_rigidity_node = & m_actor->ar_nodes[base_node_index+i*2+1+rays*2];
                };
                unsigned int beam_index = AddWheelBeam(rigidity_node, axis_node_closest_to_rigidity_node, tyre_spring, tyre_damp);
                GetBeam(beam_index).bm_type = BEAM_VIRTUAL;
            }
        }
    }

    //    Calculate the point where the support beams get stiff and prevent the tire tread nodes
    //    bounce into the rim rimradius / tire radius and add 5%, this is a shortbound calc in % !

    float support_beams_short_bound = 1.0f - ((def.rim_radius / def.tyre_radius) * 0.95f);

    for (unsigned int i=0; i<def.num_rays; i++)
    {
        // tiretread anti collapse reinforcements, using precalced support beams
        unsigned int tirenode = base_node_index + i*2 + def.num_rays*2;
        unsigned int beam_index;

        beam_index = AddWheelBeam(axis_node_1, &m_actor->ar_nodes[tirenode],     tyre_spring/2.f, tyre_damp);
        GetBeam(beam_index).shortbound = support_beams_short_bound;
        GetBeam(beam_index).longbound  = 0.f;
        GetBeam(beam_index).bounded = SHOCK1;

        beam_index = AddWheelBeam(axis_node_2, &m_actor->ar_nodes[tirenode + 1], tyre_spring/2.f, tyre_damp);
        GetBeam(beam_index).shortbound = support_beams_short_bound;
        GetBeam(beam_index).longbound  = 0.f;
        GetBeam(beam_index).bounded = SHOCK1;
    }

    // Wheel object
    wheel.wh_braking = this->TranslateBrakingDef(def.braking);
    wheel.wh_propulsed = def.propulsion;
    wheel.wh_num_nodes = 2 * def.num_rays;
    wheel.wh_num_rim_nodes = wheel.wh_num_nodes;
    wheel.wh_axis_node_0 = axis_node_1;
    wheel.wh_axis_node_1 = axis_node_2;
    wheel.wh_radius = def.tyre_radius;
    wheel.wh_rim_radius = def.rim_radius;
    wheel.wh_arm_node = this->GetNodePointer(def.reference_arm_node);

    if (def.propulsion != Truck::Wheels::PROPULSION_NONE)
    {
        // for inter-differential locking
        m_actor->m_proped_wheel_pairs[m_actor->m_num_proped_wheels] = m_actor->ar_num_wheels;
        m_actor->m_num_proped_wheels++;
    }

    // Find near attach
    Ogre::Real length_1 = (axis_node_1->RelPosition - wheel.wh_arm_node->RelPosition).length();
    Ogre::Real length_2 = (axis_node_2->RelPosition - wheel.wh_arm_node->RelPosition).length();
    wheel.wh_near_attach_node = (length_1 < length_2) ? axis_node_1 : axis_node_2;

    // Commit the wheel
    int wheel_index = m_actor->ar_num_wheels;
    ++m_actor->ar_num_wheels;

    // Create visuals
    m_wheel_visuals_queue.push_back(WheelVisualsTicket(wheel_index, base_node_index, &def, axis_node_1->pos, axis_node_2->pos));
}

wheel_t::BrakeCombo ActorSpawner::TranslateBrakingDef(Truck::Wheels::Braking def)
{
    switch (def)
    {
    case Truck::Wheels::Braking::BRAKING_NO:                return wheel_t::BrakeCombo::NONE;
    case Truck::Wheels::Braking::BRAKING_YES:               return wheel_t::BrakeCombo::FOOT_HAND;
    case Truck::Wheels::Braking::BRAKING_DIRECTIONAL_LEFT:  return wheel_t::BrakeCombo::FOOT_HAND_SKID_LEFT;
    case Truck::Wheels::Braking::BRAKING_DIRECTIONAL_RIGHT: return wheel_t::BrakeCombo::FOOT_HAND_SKID_RIGHT;
    case Truck::Wheels::Braking::BRAKING_ONLY_FOOT:         return wheel_t::BrakeCombo::FOOT_ONLY;
    default:                                                 return wheel_t::BrakeCombo::NONE;
    }
}

void ActorSpawner::ProcessMeshWheel(Truck::MeshWheel & meshwheel_def)
{
    if (meshwheel_def._is_meshwheel2)
    {
        this->ProcessMeshWheel2(meshwheel_def);
        return;
    }


    unsigned int base_node_index = m_actor->ar_num_nodes;
    node_t *axis_node_1 = GetNodePointer(meshwheel_def.nodes[0]);
    node_t *axis_node_2 = GetNodePointer(meshwheel_def.nodes[1]);

    Ogre::Vector3 pos_1 = axis_node_1->AbsPosition;
    Ogre::Vector3 pos_2 = axis_node_2->AbsPosition;

    /* Enforce the "second node must have a larger Z coordinate than the first" constraint */
    if (pos_1.z > pos_2.z)
    {
        node_t *swap = axis_node_1;
        axis_node_1 = axis_node_2;
        axis_node_2 = swap;
    }

    unsigned int wheel_index = BuildWheelObjectAndNodes(
        meshwheel_def.num_rays,
        axis_node_1,
        axis_node_2,
        GetNodePointer(meshwheel_def.reference_arm_node),
        meshwheel_def.num_rays * 2,
        meshwheel_def.num_rays * 8,
        meshwheel_def.tyre_radius,
        meshwheel_def.propulsion,
        meshwheel_def.braking,
        meshwheel_def.mass
    );

    BuildWheelBeams(
        meshwheel_def.num_rays,
        base_node_index,
        axis_node_1,
        axis_node_2,
        meshwheel_def.spring,      /* Tyre */
        meshwheel_def.damping,     /* Tyre */
        meshwheel_def.spring,      /* Rim */
        meshwheel_def.damping,     /* Rim */
        meshwheel_def.rigidity_node
    );

    m_wheel_visuals_queue.push_back(
        WheelVisualsTicket(wheel_index, base_node_index, &meshwheel_def, axis_node_1->pos, axis_node_2->pos));

    CreateWheelSkidmarks(wheel_index);
}

void ActorSpawner::ProcessMeshWheel2(Truck::MeshWheel & def)
{

    unsigned int base_node_index = m_actor->ar_num_nodes;
    node_t *axis_node_1 = GetNodePointer(def.nodes[0]);
    node_t *axis_node_2 = GetNodePointer(def.nodes[1]);

    if (axis_node_1 == nullptr || axis_node_2 == nullptr)
    {
        this->AddMessage(Message::TYPE_ERROR, "Failed to find axis nodes, skipping meshwheel2...");
        return;
    }

    Ogre::Vector3 pos_1 = axis_node_1->AbsPosition;
    Ogre::Vector3 pos_2 = axis_node_2->AbsPosition;

    /* Enforce the "second node must have a larger Z coordinate than the first" constraint */
    if (pos_1.z > pos_2.z)
    {
        node_t *swap = axis_node_1;
        axis_node_1 = axis_node_2;
        axis_node_2 = swap;
    }	

    unsigned int wheel_index = BuildWheelObjectAndNodes(
        def.num_rays,
        axis_node_1,
        axis_node_2,
        GetNodePointer(def.reference_arm_node),
        def.num_rays * 2,
        def.num_rays * 8,
        def.tyre_radius,
        def.propulsion,
        def.braking,
        def.mass
    );

    /* --- Beams --- */
    /* Use data from directive 'set_beam_defaults' for the tiretread beams */
    float tyre_spring = def.spring;
    float tyre_damp = def.damping;
    float rim_spring = m_state.default_spring;
    float rim_damp = m_state.default_damp;

    BuildWheelBeams(
        def.num_rays,
        base_node_index,
        axis_node_1,
        axis_node_2,
        tyre_spring,
        tyre_damp,
        rim_spring,
        rim_damp,
        def.rigidity_node,
        /*max_extension:*/0.15
    );

    m_wheel_visuals_queue.push_back(WheelVisualsTicket(
        wheel_index, base_node_index, &def, axis_node_1->pos, axis_node_2->pos));

    CreateWheelSkidmarks(wheel_index);
}

void ActorSpawner::BuildMeshWheelVisuals(
    unsigned int wheel_index,
    unsigned int base_node_index,
    unsigned int axis_node_1_index,
    unsigned int axis_node_2_index,
    unsigned int num_rays,
    Ogre::String mesh_name,
    Ogre::String material_name,
    float rim_radius,
    bool rim_reverse
)
{
    try
    {
        FlexMeshWheel* flexmesh_wheel = m_flex_factory.CreateFlexMeshWheel(
            wheel_index, 
            axis_node_1_index,
            axis_node_2_index,
            base_node_index,
            num_rays,
            rim_radius,
            rim_reverse,
            mesh_name,
            material_name);
        Ogre::SceneNode* scene_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
        scene_node->attachObject(flexmesh_wheel->GetTireEntity());

        WheelGfx visual_wheel;
        visual_wheel.wx_is_meshwheel = false;
        visual_wheel.wx_flex_mesh = flexmesh_wheel;
        visual_wheel.wx_scenenode = scene_node;
        m_actor->m_gfx_actor->SetWheelVisuals(wheel_index, visual_wheel);
    }
    catch (Ogre::Exception& e)
    {
        this->AddMessage(Message::TYPE_ERROR, "Failed to create meshwheel visuals, message: " + e.getFullDescription());
        return;
    }
}

unsigned int ActorSpawner::BuildWheelObjectAndNodes( 
    unsigned int num_rays,
    node_t *axis_node_1,
    node_t *axis_node_2,
    node_t *reference_arm_node,
    unsigned int reserve_nodes,
    unsigned int reserve_beams,
    float wheel_radius,
    Truck::Wheels::Propulsion propulsion,
    Truck::Wheels::Braking braking,
    float wheel_mass,
    float wheel_width       /* Default: -1.f */
)
{
    wheel_t & wheel = m_actor->ar_wheels[m_actor->ar_num_wheels];

    /* Axis */
    Ogre::Vector3 axis_vector = axis_node_2->RelPosition - axis_node_1->RelPosition;
    float axis_length = axis_vector.length();
    axis_vector.normalise();

    /* Wheel object */
    wheel.wh_braking      = this->TranslateBrakingDef(braking);
    wheel.wh_propulsed    = propulsion;
    wheel.wh_num_nodes    = 2 * num_rays;
    wheel.wh_axis_node_0  = axis_node_1;
    wheel.wh_axis_node_1  = axis_node_2;
    wheel.wh_radius       = wheel_radius;
    wheel.wh_width        = (wheel_width < 0) ? axis_length : wheel_width;
    wheel.wh_arm_node     = reference_arm_node;

    /* Find near attach */
    Ogre::Real length_1 = (axis_node_1->RelPosition - wheel.wh_arm_node->RelPosition).length();
    Ogre::Real length_2 = (axis_node_2->RelPosition - wheel.wh_arm_node->RelPosition).length();
    wheel.wh_near_attach_node = (length_1 < length_2) ? axis_node_1 : axis_node_2;

    if (propulsion != Truck::Wheels::PROPULSION_NONE)
    {
        /* for inter-differential locking */
        m_actor->m_proped_wheel_pairs[m_actor->m_num_proped_wheels] = m_actor->ar_num_wheels;
        m_actor->m_num_proped_wheels++;
    }
    
    /* Nodes */
    Ogre::Vector3 ray_vector = axis_vector.perpendicular() * wheel_radius;
    Ogre::Quaternion ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.0 / (num_rays * 2)), axis_vector);

#ifdef DEBUG_TRUCKPARSER2013
    // TRUCK PARSER 2013 DEBUG
    std::stringstream msg;
    msg << "\nDBG ActorSpawner::BuildWheelObjectAndNodes()\nDBG nodebase:" << m_actor->ar_num_nodes <<", axis-node-0:"<<axis_node_1->pos <<", axis-node-1:"<<axis_node_2->pos<<"\n";
    msg << "DBG ==== Adding nodes ====";
    // END
#endif

    for (unsigned int i = 0; i < num_rays; i++)
    {
        /* Outer ring */
        Ogre::Vector3 ray_point = axis_node_1->RelPosition + ray_vector;
        ray_vector = ray_rotator * ray_vector;

        node_t & outer_node = GetFreeNode();
        InitNode(outer_node, ray_point);
        outer_node.mass          = wheel_mass / (2.f * num_rays);
        outer_node.nd_contacter  = true;
        outer_node.nd_tyre_node  = true;

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(outer_node.pos)));

        /* Inner ring */
        ray_point = axis_node_2->RelPosition + ray_vector;
        ray_vector = ray_rotator * ray_vector;

        node_t & inner_node = GetFreeNode();
        InitNode(inner_node, ray_point);
        inner_node.mass          = wheel_mass / (2.f * num_rays);
        inner_node.nd_contacter  = true;
        inner_node.nd_tyre_node  = true;

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(inner_node.pos)));

        /* Wheel object */
        wheel.wh_nodes[i * 2] = & outer_node;
        wheel.wh_nodes[(i * 2) + 1] = & inner_node;

#ifdef DEBUG_TRUCKPARSER2013
        // TRUCK PARSER 2013 DEBUG
        int modifier = 0;
        msg << "\nDBG\tN1: index=" << outer_node.pos + modifier << ", iswheel=" << WHEEL_DEFAULT 
            <<", X=" << outer_node.AbsPosition.x <<", Y=" << outer_node.AbsPosition.y <<", Z=" << outer_node.AbsPosition.z << std::endl
            << "DBG\tN2: index=" << inner_node.pos + modifier << ", iswheel=" << WHEEL_DEFAULT 
            <<", X=" << inner_node.AbsPosition.x <<", Y=" << inner_node.AbsPosition.y <<", Z=" << inner_node.AbsPosition.z;
        // END
#endif
    }

#ifdef DEBUG_TRUCKPARSER2013
    // TRUCK PARSER 2013 DEBUG
    LOG(msg.str());
    // END
#endif

    /* Advance */
    unsigned int wheel_index = m_actor->ar_num_wheels;
    m_actor->ar_num_wheels++;
    return wheel_index;
}

void ActorSpawner::AdjustNodeBuoyancy(node_t & node, Truck::Node & node_def)
{
    std::string options = node_def.options + m_state.default_node_options; // Merge options
    node.buoyancy = (options.find(Truck::Node::OPTION_b_EXTRA_BUOYANCY)!=std::string::npos) ? 10000.f : m_actor->m_dry_mass/15.f;
}

void ActorSpawner::AdjustNodeBuoyancy(node_t & node)
{
    node.buoyancy = (m_state.default_node_options.find(Truck::Node::OPTION_b_EXTRA_BUOYANCY)!=std::string::npos) ? 10000.f : m_actor->m_dry_mass/15.f;
}

void ActorSpawner::BuildWheelBeams(
    unsigned int num_rays,
    unsigned int base_node_index,
    node_t *axis_node_1,
    node_t *axis_node_2,
    float tyre_spring,
    float tyre_damping,
    float rim_spring,
    float rim_damping,
    Truck::Node::Ref const & rigidity_node_id,
    float max_extension // = 0.f
)
{
    /* Find out where to connect rigidity node */
    bool rigidity_beam_side_1 = false;
    node_t *rigidity_node = nullptr;
    if (rigidity_node_id.IsValidAnyState())
    {
        rigidity_node = GetNodePointerOrThrow(rigidity_node_id);
        float distance_1 = rigidity_node->RelPosition.distance(axis_node_1->RelPosition);
        float distance_2 = rigidity_node->RelPosition.distance(axis_node_2->RelPosition);
        rigidity_beam_side_1 = distance_1 < distance_2;
    }

    for (unsigned int i = 0; i < num_rays; i++)
    {
        /* Bounded */
        unsigned int outer_ring_node_index = base_node_index + (i * 2);
        node_t *outer_ring_node = & m_actor->ar_nodes[outer_ring_node_index];
        node_t *inner_ring_node = & m_actor->ar_nodes[outer_ring_node_index + 1];
        
        AddWheelBeam(axis_node_1, outer_ring_node, tyre_spring, tyre_damping,  0.66f, max_extension);
        AddWheelBeam(axis_node_2, inner_ring_node, tyre_spring, tyre_damping,  0.66f, max_extension);
        AddWheelBeam(axis_node_2, outer_ring_node, tyre_spring, tyre_damping);
        AddWheelBeam(axis_node_1, inner_ring_node, tyre_spring, tyre_damping);

        /* Reinforcement */
        unsigned int next_outer_ring_node_index = base_node_index + (((i + 1) % num_rays) * 2);
        node_t *next_outer_ring_node = & m_actor->ar_nodes[next_outer_ring_node_index];
        node_t *next_inner_ring_node = & m_actor->ar_nodes[next_outer_ring_node_index + 1];

        AddWheelBeam(outer_ring_node, inner_ring_node,      rim_spring, rim_damping);
        AddWheelBeam(outer_ring_node, next_outer_ring_node, rim_spring, rim_damping);
        AddWheelBeam(inner_ring_node, next_inner_ring_node, rim_spring, rim_damping);
        AddWheelBeam(inner_ring_node, next_outer_ring_node, rim_spring, rim_damping);

        /* Rigidity beams */
        if (rigidity_node != nullptr)
        {
            node_t *target_node = (rigidity_beam_side_1) ? outer_ring_node : inner_ring_node;
            unsigned int beam_index = AddWheelBeam(rigidity_node, target_node, tyre_spring, tyre_damping, -1.f, -1.f, BEAM_VIRTUAL);
            m_actor->ar_beams[beam_index].bm_type = BEAM_VIRTUAL;
        }
    }
}

unsigned int ActorSpawner::AddWheel(Truck::Wheel & wheel_def)
{
    unsigned int base_node_index = m_actor->ar_num_nodes;
    node_t *axis_node_1 = GetNodePointer(wheel_def.nodes[0]);
    node_t *axis_node_2 = GetNodePointer(wheel_def.nodes[1]);

    if (axis_node_1 == nullptr || axis_node_2 == nullptr)
    {
        std::stringstream msg;
        msg << "Error creating 'wheel': Some axis nodes were not found";
        msg << " (Node1: " << wheel_def.nodes[0].ToString() << " => " << (axis_node_1 == nullptr) ? "NOT FOUND)" : "found)";
        msg << " (Node2: " << wheel_def.nodes[1].ToString() << " => " << (axis_node_2 == nullptr) ? "NOT FOUND)" : "found)";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return -1;
    }

    Ogre::Vector3 pos_1 = axis_node_1->AbsPosition;
    Ogre::Vector3 pos_2 = axis_node_2->AbsPosition;

    /* Enforce the "second node must have a larger Z coordinate than the first" constraint */
    if (pos_1.z > pos_2.z)
    {
        node_t *swap = axis_node_1;
        axis_node_1 = axis_node_2;
        axis_node_2 = swap;
    }	

    unsigned int wheel_index = BuildWheelObjectAndNodes(
        wheel_def.num_rays,
        axis_node_1,
        axis_node_2,
        GetNodePointer(wheel_def.reference_arm_node),
        wheel_def.num_rays * 2,
        wheel_def.num_rays * 8,
        wheel_def.radius,
        wheel_def.propulsion,
        wheel_def.braking,
        wheel_def.mass,
        -1.f // Set width to axis length (width in definition is ignored)
    );

    BuildWheelBeams(
        wheel_def.num_rays,
        base_node_index,
        axis_node_1,
        axis_node_2,
        wheel_def.springiness, /* Tyre */
        wheel_def.damping,     /* Tyre */
        wheel_def.springiness, /* Rim */
        wheel_def.damping,     /* Rim */
        wheel_def.rigidity_node
    );

    m_wheel_visuals_queue.push_back(WheelVisualsTicket(wheel_index, base_node_index, &wheel_def));

    CreateWheelSkidmarks(wheel_index);

    return wheel_index;
}

void ActorSpawner::CreateWheelSkidmarks(unsigned int wheel_index)
{
    // Always create, even if disabled by config
    m_actor->m_skid_trails[wheel_index] = new RoR::Skidmark(
        RoR::App::GetGfxScene()->GetSkidmarkConf(), &m_actor->ar_wheels[wheel_index], m_particles_parent_scenenode, 300, 20);
}

unsigned int ActorSpawner::AddWheel2(Truck::Wheel2 & wheel_2_def)
{
    unsigned int base_node_index = m_actor->ar_num_nodes;
    wheel_t & wheel = m_actor->ar_wheels[m_actor->ar_num_wheels];
    node_t *axis_node_1 = GetNodePointer(wheel_2_def.nodes[0]);
    node_t *axis_node_2 = GetNodePointer(wheel_2_def.nodes[1]);

    if (axis_node_1 == nullptr || axis_node_2 == nullptr)
    {
        std::stringstream msg;
        msg << "Error creating 'wheel2': Some axis nodes were not found";
        msg << " (Node1: " << wheel_2_def.nodes[0].ToString() << " => " << (axis_node_1 == nullptr) ? "NOT FOUND)" : "found)";
        msg << " (Node2: " << wheel_2_def.nodes[1].ToString() << " => " << (axis_node_2 == nullptr) ? "NOT FOUND)" : "found)";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return -1;
    }

    Ogre::Vector3 pos_1 = axis_node_1->AbsPosition;
    Ogre::Vector3 pos_2 = axis_node_2->AbsPosition;

    /* Enforce the "second node must have a larger Z coordinate than the first" constraint */
    if (pos_1.z > pos_2.z)
    {
        node_t *swap = axis_node_1;
        axis_node_1 = axis_node_2;
        axis_node_2 = swap;
    }

    /* Find out where to connect rigidity node */
    bool rigidity_beam_side_1 = false;
    if (wheel_2_def.rigidity_node.IsValidAnyState())
    {
        node_t & rigidity_node = GetNode(wheel_2_def.rigidity_node);
        Ogre::Real distance_1 = (rigidity_node.RelPosition - axis_node_1->RelPosition).length();
        Ogre::Real distance_2 = (rigidity_node.RelPosition - axis_node_2->RelPosition).length();
        rigidity_beam_side_1 = distance_1 < distance_2;
    }


    /* Node&beam generation */
    Ogre::Vector3 axis_vector = axis_node_2->RelPosition - axis_node_1->RelPosition;
    axis_vector.normalise();
    Ogre::Vector3 rim_ray_vector = Ogre::Vector3(0, wheel_2_def.rim_radius, 0);
    Ogre::Quaternion rim_ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.f / wheel_2_def.num_rays), axis_vector);

    /* Width */
    wheel.wh_width = axis_vector.length(); /* wheel_def.width is ignored. */

    /* Rim nodes */
    for (unsigned int i = 0; i < wheel_2_def.num_rays; i++)
    {
        float node_mass = wheel_2_def.mass / (4.f * wheel_2_def.num_rays);

        /* Outer ring */
        Ogre::Vector3 ray_point = axis_node_1->RelPosition + rim_ray_vector;

        node_t & outer_node    = GetFreeNode();
        InitNode(outer_node, ray_point);
        outer_node.mass        = node_mass;
        outer_node.nd_rim_node = true;

        m_actor->ar_minimass[outer_node.pos] = m_state.default_minimass;

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(outer_node.pos)));

        /* Inner ring */
        ray_point = axis_node_2->RelPosition + rim_ray_vector;

        node_t & inner_node    = GetFreeNode();
        InitNode(inner_node, ray_point);
        inner_node.mass        = node_mass;
        inner_node.nd_rim_node = true;

        m_actor->ar_minimass[inner_node.pos] = m_state.default_minimass;

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(inner_node.pos)));

        /* Wheel object */
        wheel.wh_rim_nodes[i * 2] = & outer_node;
        wheel.wh_rim_nodes[(i * 2) + 1] = & inner_node;

        rim_ray_vector = rim_ray_rotator * rim_ray_vector;
    }

    Ogre::Vector3 tyre_ray_vector = Ogre::Vector3(0, wheel_2_def.tyre_radius, 0);
    Ogre::Quaternion tyre_ray_rotator = Ogre::Quaternion(Ogre::Degree(-180.f / wheel_2_def.num_rays), axis_vector);
    tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

    /* Tyre nodes */
    for (unsigned int i = 0; i < wheel_2_def.num_rays; i++)
    {
        /* Outer ring */
        Ogre::Vector3 ray_point = axis_node_1->RelPosition + tyre_ray_vector;

        node_t & outer_node = GetFreeNode();
        InitNode(outer_node, ray_point);
        outer_node.mass          = (0.67f * wheel_2_def.mass) / (2.f * wheel_2_def.num_rays);
        outer_node.friction_coef = wheel.wh_width * WHEEL_FRICTION_COEF;
        outer_node.volume_coef   = m_state.default_node_surface;
        outer_node.surface_coef  = m_state.default_node_surface;
        outer_node.nd_contacter  = true;
        outer_node.nd_tyre_node  = true;

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(outer_node.pos)));

        /* Inner ring */
        ray_point = axis_node_2->RelPosition + tyre_ray_vector;

        node_t & inner_node = GetFreeNode();
        InitNode(inner_node, ray_point);
        inner_node.mass          = (0.33f * wheel_2_def.mass) / (2.f * wheel_2_def.num_rays);
        inner_node.friction_coef = wheel.wh_width * WHEEL_FRICTION_COEF;
        inner_node.volume_coef   = m_state.default_node_surface;
        inner_node.surface_coef  = m_state.default_node_surface;
        inner_node.nd_contacter  = true;
        inner_node.nd_tyre_node  = true;

        m_gfx_nodes.push_back(NodeGfx(static_cast<uint16_t>(inner_node.pos)));

        /* Wheel object */
        wheel.wh_nodes[i * 2] = & outer_node;
        wheel.wh_nodes[(i * 2) + 1] = & inner_node;

        tyre_ray_vector = rim_ray_rotator * tyre_ray_vector; // This is OK
    }

    /* Beams */
    for (unsigned int i = 0; i < wheel_2_def.num_rays; i++)
    {
        /* --- Rim ---  */

        /* Bounded */
        unsigned int rim_outer_node_index = base_node_index + (i * 2);
        node_t *rim_outer_node = & m_actor->ar_nodes[rim_outer_node_index];
        node_t *rim_inner_node = & m_actor->ar_nodes[rim_outer_node_index + 1];

        unsigned int beam_index;
        beam_index = AddWheelRimBeam(wheel_2_def, axis_node_1, rim_outer_node);
        GetBeam(beam_index).shortbound = 0.66;
        beam_index = AddWheelRimBeam(wheel_2_def, axis_node_2, rim_inner_node);
        GetBeam(beam_index).shortbound = 0.66;
        AddWheelRimBeam(wheel_2_def, axis_node_2, rim_outer_node);
        AddWheelRimBeam(wheel_2_def, axis_node_1, rim_inner_node);

        /* Reinforcement */
        unsigned int rim_next_outer_node_index = base_node_index + (((i + 1) % wheel_2_def.num_rays) * 2);
        node_t *rim_next_outer_node = & m_actor->ar_nodes[rim_next_outer_node_index];
        node_t *rim_next_inner_node = & m_actor->ar_nodes[rim_next_outer_node_index + 1];

        AddWheelRimBeam(wheel_2_def, axis_node_1, rim_outer_node);
        AddWheelRimBeam(wheel_2_def, rim_outer_node, rim_inner_node);
        AddWheelRimBeam(wheel_2_def, rim_outer_node, rim_next_outer_node);
        AddWheelRimBeam(wheel_2_def, rim_inner_node, rim_next_inner_node);
        AddWheelRimBeam(wheel_2_def, rim_outer_node, rim_next_inner_node);
        AddWheelRimBeam(wheel_2_def, rim_inner_node, rim_next_outer_node);

        /* -- Rigidity -- */
        if (wheel_2_def.rigidity_node.IsValidAnyState())
        {
            unsigned int rig_beam_index = AddWheelRimBeam(wheel_2_def,
                            GetNodePointer(wheel_2_def.rigidity_node),
                            (rigidity_beam_side_1) ? rim_outer_node : rim_inner_node
            );
            m_actor->ar_beams[rig_beam_index].bm_type = BEAM_VIRTUAL;
        }

        /* --- Tyre --- */

        unsigned int tyre_node_index = rim_outer_node_index + (2 * wheel_2_def.num_rays);
        node_t *tyre_outer_node = & m_actor->ar_nodes[tyre_node_index];
        node_t *tyre_inner_node = & m_actor->ar_nodes[tyre_node_index + 1];
        unsigned int tyre_next_node_index = rim_next_outer_node_index + (2 * wheel_2_def.num_rays);
        node_t *tyre_next_outer_node = & m_actor->ar_nodes[tyre_next_node_index];
        node_t *tyre_next_inner_node = & m_actor->ar_nodes[tyre_next_node_index + 1];

        /* Tyre band */
        AddTyreBeam(wheel_2_def, tyre_outer_node, tyre_next_outer_node);
        AddTyreBeam(wheel_2_def, tyre_outer_node, tyre_next_inner_node);
        AddTyreBeam(wheel_2_def, tyre_inner_node, tyre_next_outer_node);
        AddTyreBeam(wheel_2_def, tyre_inner_node, tyre_next_inner_node);
        /* Tyre sidewalls */
        AddTyreBeam(wheel_2_def, tyre_outer_node, rim_outer_node);
        AddTyreBeam(wheel_2_def, tyre_outer_node, rim_next_outer_node);
        AddTyreBeam(wheel_2_def, tyre_inner_node, rim_inner_node);
        AddTyreBeam(wheel_2_def, tyre_inner_node, rim_next_inner_node);
        /* Reinforcement */
        AddTyreBeam(wheel_2_def, tyre_outer_node, rim_inner_node);
        AddTyreBeam(wheel_2_def, tyre_outer_node, rim_next_inner_node);
        AddTyreBeam(wheel_2_def, tyre_inner_node, rim_outer_node);
        AddTyreBeam(wheel_2_def, tyre_inner_node, rim_next_outer_node);
        /* Backpressure, bounded */
        AddTyreBeam(wheel_2_def, axis_node_1, tyre_outer_node);
        AddTyreBeam(wheel_2_def, axis_node_2, tyre_inner_node);
    }

    /* Wheel object */
    wheel.wh_braking       = this->TranslateBrakingDef(wheel_2_def.braking);
    wheel.wh_propulsed     = wheel_2_def.propulsion;
    wheel.wh_num_nodes     = 2 * wheel_2_def.num_rays;
    wheel.wh_num_rim_nodes = wheel.wh_num_nodes;
    wheel.wh_axis_node_0   = axis_node_1;
    wheel.wh_axis_node_1   = axis_node_2;
    wheel.wh_radius        = wheel_2_def.tyre_radius;
    wheel.wh_rim_radius    = wheel_2_def.rim_radius;
    wheel.wh_arm_node      = this->GetNodePointer(wheel_2_def.reference_arm_node);

    if (wheel_2_def.propulsion != Truck::Wheels::PROPULSION_NONE)
    {
        /* for inter-differential locking */
        m_actor->m_proped_wheel_pairs[m_actor->m_num_proped_wheels] = m_actor->ar_num_wheels;
        m_actor->m_num_proped_wheels++;
    }

    /* Find near attach */
    Ogre::Real length_1 = (axis_node_1->RelPosition - wheel.wh_arm_node->RelPosition).length();
    Ogre::Real length_2 = (axis_node_2->RelPosition - wheel.wh_arm_node->RelPosition).length();
    wheel.wh_near_attach_node = (length_1 < length_2) ? axis_node_1 : axis_node_2;

    CreateWheelSkidmarks(static_cast<unsigned>(m_actor->ar_num_wheels));

    /* Advance */
    unsigned int wheel_index = m_actor->ar_num_wheels;
    m_actor->ar_num_wheels++;
    return wheel_index;
}

void ActorSpawner::CreateWheelVisuals(
    unsigned int wheel_index, 
    unsigned int node_base_index,
    unsigned int num_rays,
    Ogre::String const & rim_material_name,
    Ogre::String const & band_material_name,
    bool separate_rim,
    float rim_ratio
)
{
    wheel_t & wheel = m_actor->ar_wheels[wheel_index];

    try
    {
        WheelGfx visual_wheel;

        const std::string wheel_mesh_name = this->ComposeName("WheelMesh", wheel_index);
        visual_wheel.wx_is_meshwheel = false;
        visual_wheel.wx_flex_mesh = new FlexMesh(
            wheel_mesh_name,
            m_actor->m_gfx_actor.get(),
            wheel.wh_axis_node_0->pos,
            wheel.wh_axis_node_1->pos,
            node_base_index,
            num_rays,
            rim_material_name,
            band_material_name,
            separate_rim,
            rim_ratio
        );

        const std::string instance_name = this->ComposeName("WheelEntity", wheel_index);
        Ogre::Entity *ec = App::GetGfxScene()->GetSceneManager()->createEntity(instance_name, wheel_mesh_name);
        this->SetupNewEntity(ec, Ogre::ColourValue(0, 0.5, 0.5));
        visual_wheel.wx_scenenode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
        m_actor->m_deletion_entities.emplace_back(ec);
        visual_wheel.wx_scenenode->attachObject(ec);
        m_actor->m_gfx_actor->SetWheelVisuals(wheel_index, visual_wheel);
    }
    catch (Ogre::Exception& e)
    {
        AddMessage(Message::TYPE_ERROR, "Failed to create wheel visuals: " +  e.getFullDescription());
    }
}

unsigned int ActorSpawner::AddWheelBeam(
    node_t *node_1, 
    node_t *node_2, 
    float spring, 
    float damping, 
    float max_contraction,   /* Default: -1.f */
    float max_extension,     /* Default: -1.f */
    BeamType type            /* Default: BEAM_INVISIBLE */
)
{
    unsigned int index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(*node_1, *node_2);
    beam.bm_type = type;
    beam.k = spring;
    beam.d = damping;
    beam.detacher_group = DEFAULT_DETACHER_GROUP;
    if (max_contraction > 0.f)
    {
        beam.shortbound = max_contraction;
        beam.longbound = max_extension;
        beam.bounded = SHOCK1;
    }
    CalculateBeamLength(beam);

    return index;
}

unsigned int ActorSpawner::AddWheelRimBeam(Truck::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2)
{
    unsigned int beam_index = _SectionWheels2AddBeam(wheel_2_def, node_1, node_2);
    beam_t & beam = GetBeam(beam_index);
    beam.k = wheel_2_def.rim_springiness;
    beam.d = wheel_2_def.rim_damping;
    return beam_index;
}

unsigned int ActorSpawner::AddTyreBeam(Truck::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2)
{
    unsigned int beam_index = _SectionWheels2AddBeam(wheel_2_def, node_1, node_2);
    beam_t & beam = GetBeam(beam_index);
    beam.k = wheel_2_def.tyre_springiness;
    beam.d = wheel_2_def.tyre_damping;

    m_actor->GetTyrePressure().AddBeam((int)beam_index);

    return beam_index;
}

unsigned int ActorSpawner::_SectionWheels2AddBeam(Truck::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2)
{
    unsigned int index = m_actor->ar_num_beams;
    beam_t & beam = GetFreeBeam();
    InitBeam(beam, node_1, node_2);
    beam.bm_type = BEAM_NORMAL;
    SetBeamStrength(beam, m_state.default_break);
    beam.strength = m_state.default_deform;
    return index;
}

void ActorSpawner::ProcessWheel2(Truck::Wheel2 & def)
{
    unsigned int node_base_index = m_actor->ar_num_nodes;
    unsigned int wheel_index = AddWheel2(def);
    m_wheel_visuals_queue.push_back(WheelVisualsTicket(wheel_index, node_base_index, &def));
};

void ActorSpawner::ProcessWheel(Truck::Wheel & def)
{
    AddWheel(def);
};

void ActorSpawner::ProcessWheelDetacher(Truck::WheelDetacher & def)
{
    if (def.wheel_id > m_actor->ar_num_wheels - 1)
    {
        AddMessage(Message::TYPE_ERROR, std::string("Invalid wheel_id: ") + TOSTRING(def.wheel_id));
        return;
    }

    m_actor->ar_wheels[def.wheel_id].wh_detacher_group = def.detacher_group;
};

void ActorSpawner::ProcessTractionControl(Truck::TractionControl & def)
{
    /* #1: regulating_force */
    float force = def.regulation_force;
    if (force < 1.f || force > 20.f)
    {
        std::stringstream msg;
        msg << "Clamping 'regulating_force' value '" << force << "' to allowed range <1 - 20>";
        AddMessage(Message::TYPE_INFO, msg.str());
        force = (force < 1.f) ? 1.f : 20.f;
    }
    m_actor->tc_ratio = force;

    /* #2: wheelslip */
    // no longer needed

    /* #3: fade_speed */
    // no longer needed

    /* #4: pulse/sec */
    float pulse = def.pulse_per_sec;
    if (pulse <= 1.0f || pulse >= 2000.0f)
    {
        pulse = 2000.0f;
    } 
    m_actor->tc_pulse_time = 1 / pulse;

    /* #4: mode */
    m_actor->tc_mode = def.attr_is_on;
    m_actor->tc_nodash = def.attr_no_dashboard;
    m_actor->tc_notoggle = def.attr_no_toggle;
};

void ActorSpawner::ProcessAntiLockBrakes(Truck::AntiLockBrakes & def)
{
    /* #1: regulating_force */
    float force = def.regulation_force;
    if (force < 1.f || force > 20.f)
    {
        std::stringstream msg;
        msg << "Clamping 'regulating_force' value '" << force << "' to allowed range <1 - 20>";
        AddMessage(Message::TYPE_INFO, msg.str());
        force = (force < 1.f) ? 1.f : 20.f;
    }
    m_actor->alb_ratio = force;

    /* #2: min_speed */
    /* Wheelspeed adaption: 60 sec * 60 mins / 1000(kilometer) = 3.6 to get meter per sec */
    float min_speed = def.min_speed / 3.6f;
    m_actor->alb_minspeed = std::max(0.5f, min_speed);

    /* #3: pulse_per_sec */
    float pulse = def.pulse_per_sec;
    if (pulse <= 1.0f || pulse >= 2000.0f)
    {
        pulse = 2000.0f;
    } 
    m_actor->alb_pulse_time = 1 / pulse;

    /* #4: mode */
    m_actor->alb_mode = def.attr_is_on;
    m_actor->alb_nodash = def.attr_no_dashboard;
    m_actor->alb_notoggle = def.attr_no_toggle;
}

void ActorSpawner::ProcessBrakes(Truck::Brakes & def)
{
    m_actor->ar_brake_force = def.default_braking_force;
    m_actor->m_handbrake_force = 2.f * m_actor->ar_brake_force;
    if (def.parking_brake_force != -1.f)
    {
        m_actor->m_handbrake_force = def.parking_brake_force;
    }
};

void ActorSpawner::ProcessEngturbo(Truck::Engturbo& def)
{
    /* Is this a land vehicle? */
    if (m_actor->ar_engine == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, "Section 'engturbo' found but no engine defined. Skipping ...");
        return;
    }

    m_actor->ar_engine->SetTurboOptions(def.version, def.tinertiaFactor, def.nturbos, def.param1, def.param2, def.param3, def.param4, def.param5, def.param6, def.param7, def.param8, def.param9, def.param10, def.param11);
};

void ActorSpawner::ProcessEngoption(Truck::Engoption& def)
{
    /* Is this a land vehicle? */
    if (m_actor->ar_engine == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, "Section 'engoption' found but no engine defined. Skipping ...");
        return;
    }

    if (def.idle_rpm > 0 && def.stall_rpm > 0 && def.stall_rpm > def.idle_rpm)
    {
        AddMessage(Message::TYPE_WARNING, "Stall RPM is set higher than Idle RPM.");
    }

    /* Process it */
    m_actor->ar_engine->SetEngineOptions(
        def.inertia,
        def.type,
        def.clutch_force,
        def.shift_time,
        def.clutch_time,
        def.post_shift_time,
        def.idle_rpm,
        def.stall_rpm,
        def.max_idle_mixture,
        def.min_idle_mixture,
        def.braking_torque
    );
};

void ActorSpawner::ProcessEngine(Truck::Engine & def)
{
    /* Process it */
    m_actor->ar_driveable = TRUCK;

    /* Process gear list to EngineSim-compatible format */
    /* TODO: Move this to EngineSim::EngineSim() */
    std::vector<float> gears_compat;
    gears_compat.reserve(2 + def.gear_ratios.size());
    gears_compat.push_back(def.reverse_gear_ratio);
    gears_compat.push_back(def.neutral_gear_ratio);
    std::vector<float>::iterator itor = def.gear_ratios.begin();
    for (; itor < def.gear_ratios.end(); itor++)
    {
        gears_compat.push_back(*itor);
    }

    m_actor->ar_engine = new EngineSim(
        def.shift_down_rpm,
        def.shift_up_rpm,
        def.torque,
        gears_compat,
        def.global_gear_ratio,
        m_actor
    );

    m_actor->ar_engine->SetAutoMode(App::sim_gearbox_mode->GetEnum<SimGearboxMode>());
};

void ActorSpawner::ProcessAuthor(int pos)
{

    authorinfo_t author;
    author.type = m_cur_module->authors[pos].type;
    author.name = m_cur_module->authors[pos].name;
    author.email = m_cur_module->authors[pos].email;
    if (m_cur_module->authors[pos]._has_forum_account)
    {
        author.id = m_cur_module->authors[pos].forum_account_id;
    }
    m_actor->authors.push_back(author);

};

NodeIdx_t ActorSpawner::GetNodeIndexOrThrow(Truck::Node::Ref const & node_ref)
{
    std::pair<unsigned int, bool> result = GetNodeIndex(node_ref);
    if (! result.second)
    {
        std::stringstream msg;
        msg << "Failed to retrieve required node: " << node_ref.ToString();
        throw Exception(msg.str());
    }
    return result.first;
}

node_t & ActorSpawner::GetNodeOrThrow(Truck::Node::Ref const & node_ref)
{
    return m_actor->ar_nodes[GetNodeIndexOrThrow(node_ref)];
}

void ActorSpawner::ProcessCamera(Truck::Camera & def)
{
    if (def.center_node.IsValidAnyState())
    {
        m_actor->ar_camera_node_pos[m_actor->ar_num_cameras] = GetNodeIndexOrThrow(def.center_node);
    }

    if (def.back_node.IsValidAnyState())
    {
        m_actor->ar_camera_node_dir[m_actor->ar_num_cameras] = GetNodeIndexOrThrow(def.back_node);
    }

    if (def.left_node.IsValidAnyState())
    {
        m_actor->ar_camera_node_roll[m_actor->ar_num_cameras] = GetNodeIndexOrThrow(def.left_node);
    }

    m_actor->ar_num_cameras++;
};

node_t* ActorSpawner::GetBeamNodePointer(Truck::Node::Ref const & node_ref)
{
    node_t* node = GetNodePointer(node_ref);
    if (node != nullptr)
    {
        return node;
    }
    return nullptr;
}

void ActorSpawner::ProcessBeam(Truck::Beam & def)
{

    // Set up beam
    int beam_index = m_actor->ar_num_beams;
    beam_t & beam = AddBeam(def.nodes[0], def.nodes[1]);
    beam.strength          = m_state.default_break * m_state.default_break_scale;
    beam.bm_type           = BEAM_NORMAL;
    beam.d                 = m_state.default_damp * m_state.default_damp_scale;
    beam.k                 = m_state.default_spring * m_state.default_spring_scale;
    beam.bounded           = def.bounded;
    CalculateBeamLength(beam);

    if (!def.invisible)
    {
        this->CreateBeamVisuals(beam, beam_index, /*visible=*/true);
    }
}


void ActorSpawner::CreateBeamVisuals(beam_t const & beam, int beam_index, bool visible, std::string material_override)
{
    std::string material_name = material_override;
    if (material_name.empty())
    {
        if (beam.bm_type == BEAM_HYDRO)
        {
            material_name = "tracks/Chrome";
        }
        else
        {
            material_name = m_state.default_beam_material;
            // Check for existing substitute
            auto it = m_managed_materials.find(material_name);
            if (it != m_managed_materials.end())
            {
                auto material = it->second;
                if (!material.isNull())
                {
                    material_name = material->getName();
                }
            }
        }
    }

    m_beam_visuals_queue.emplace_back(beam_index, m_state.default_beam_diameter, material_name.c_str(), visible);
}

void ActorSpawner::ProcessSubmesh()
{
    //close the current mesh
    m_state.subtexcoords.push_back((int)m_state.texcoords.size());
    m_state.subcabs.push_back(m_actor->ar_num_cabs);
    m_state.free_sub++;

    //initialize the next
    m_state.subisback.push_back(BACKMESH_NONE);
}

void ActorSpawner::ProcessCab(Truck::Cab & def)
{
    if (m_actor->ar_num_cabs >= MAX_CABS)
    {
        this->AddMessage(Message::TYPE_WARNING, "cabs limit reached ("+TOSTRING(MAX_CABS)+")");
        return;
    }
    m_actor->ar_cabs[m_actor->ar_num_cabs*3]  =this->ResolveNodeRef(def.nodes[0]);
    m_actor->ar_cabs[m_actor->ar_num_cabs*3+1]=this->ResolveNodeRef(def.nodes[1]);
    m_actor->ar_cabs[m_actor->ar_num_cabs*3+2]=this->ResolveNodeRef(def.nodes[2]);

    if (m_actor->ar_num_collcabs >= MAX_CABS)
    {
        this->AddMessage(Message::TYPE_WARNING, "unable to create collcab; cabs limit reached ("+TOSTRING(MAX_CABS)+")");
        return;
    }

    if (def.type=='c' || def.type=='p' || def.type=='u')
    {
        // `collcabtype` was removed, these types are equivalent now.
        m_actor->ar_collcabs[m_actor->ar_num_collcabs]=m_actor->ar_num_cabs; m_actor->ar_num_collcabs++;
    }

    bool buoy_add = false;
    Buoyance::BuoyType buoy_type = Buoyance::BUOY_NORMAL;
    if (def.type=='b') {buoy_add=true; buoy_type=Buoyance::BUOY_NORMAL;   };
    if (def.type=='r') {buoy_add=true; buoy_type=Buoyance::BUOY_DRAGONLY; };
    if (def.type=='s') {buoy_add=true; buoy_type=Buoyance::BUOY_DRAGLESS; };
    if (def.type=='D' || def.type == 'F' || def.type == 'S')
    {
        m_actor->ar_collcabs[m_actor->ar_num_collcabs]=m_actor->ar_num_cabs;
        m_actor->ar_num_collcabs++;
        buoy_add = true;
        buoy_type=Buoyance::BUOY_NORMAL;
    }

    if (buoy_add)
    {
        if (m_actor->ar_num_collcabs >= MAX_CABS || m_actor->ar_num_buoycabs >= MAX_CABS)
        {
            this->AddMessage(Message::TYPE_WARNING, "unable to create buoycab; cabs limit reached ("+TOSTRING(MAX_CABS)+")");
            return;
        }
        m_actor->ar_buoycabs[m_actor->ar_num_buoycabs]=m_actor->ar_num_cabs;
        m_actor->ar_num_buoycabs++;
    }

    m_actor->ar_num_cabs++;
}

void ActorSpawner::ProcessBackmesh()
{
    //close the current mesh
    m_state.subtexcoords.push_back((int)m_state.texcoords.size());
    m_state.subcabs.push_back(m_actor->ar_num_cabs);

    //make it normal
    m_state.subisback.push_back(BACKMESH_NONE);
    m_state.free_sub++;


    //add an extra front mesh
    int start;
    //texcoords
    if (m_state.free_sub==1) start=0; else start=m_state.subtexcoords[m_state.free_sub-2];
    for (int i=start; i<m_state.subtexcoords[m_state.free_sub-1]; i++)
    {
        m_state.texcoords.push_back(m_state.texcoords[i]);
    }
    //cab
    if (m_state.free_sub==1) start=0; else start=m_state.subcabs[m_state.free_sub-2];
    for (int i=start; i<m_state.subcabs[m_state.free_sub-1]; i++)
    {
        m_actor->ar_cabs[m_actor->ar_num_cabs] = m_actor->ar_cabs[i*3];
        m_actor->ar_cabs[m_actor->ar_num_cabs] = m_actor->ar_cabs[i*3+1];
        m_actor->ar_cabs[m_actor->ar_num_cabs] = m_actor->ar_cabs[i*3+2];
        m_actor->ar_num_cabs++;
    }
    //finish it, this is a window
    m_state.subisback.push_back(BACKMESH_TRANSPARENT);
    //close the current mesh
    m_state.subtexcoords.push_back((int)m_state.texcoords.size());
    m_state.subcabs.push_back(m_actor->ar_num_cabs);
    //make is transparent
    m_state.free_sub++;


    //add an extra back mesh
    //texcoords
    if (m_state.free_sub==1) start=0; else start=m_state.subtexcoords[m_state.free_sub-2];
    for (int i=start; i<m_state.subtexcoords[m_state.free_sub-1]; i++)
    {
        m_state.texcoords.push_back(m_state.texcoords[i]);
    }
    //cab
    if (m_state.free_sub==1) start=0; else start=m_state.subcabs[m_state.free_sub-2];
    for (int i=start; i<m_state.subcabs[m_state.free_sub-1]; i++)
    {
        m_actor->ar_cabs[m_actor->ar_num_cabs] = m_actor->ar_cabs[i*3];
        m_actor->ar_cabs[m_actor->ar_num_cabs] = m_actor->ar_cabs[i*3+1];
        m_actor->ar_cabs[m_actor->ar_num_cabs] = m_actor->ar_cabs[i*3+2];
        m_actor->ar_num_cabs++;
    }
    //we don't finish, there will be a submesh statement later
    m_state.subisback.push_back(BACKMESH_OPAQUE);
}

void ActorSpawner::CalculateBeamLength(beam_t & beam)
{
    float beam_length = (beam.p1->RelPosition - beam.p2->RelPosition).length();
    beam.L = beam_length;
    beam.refL = beam_length;
}

void ActorSpawner::InitBeam(beam_t & beam, node_t *node_1, node_t *node_2)
{
    beam.p1 = node_1;
    beam.p2 = node_2;
    CalculateBeamLength(beam);
}

void ActorSpawner::AddMessage(ActorSpawner::Message::Type type,	Ogre::String const & text)
{
    Str<4000> txt;
    txt << m_file->name;
    if (m_current_keyword != Truck::KEYWORD_NONE)
    {
        txt << " (" << Truck::Document::KeywordToString(m_current_keyword) << ")";
    }
    txt << ": " << text;
    RoR::Console::MessageType cm_type;
    switch (type)
    {
    case Message::TYPE_ERROR:
    case Message::TYPE_INTERNAL_ERROR:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_ERROR;
        break;

    case Message::TYPE_WARNING:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_WARNING;
        break;

    default:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_NOTICE;
        break;
    }

    RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_ACTOR, cm_type, txt.ToCStr());
}

NodeIdx_t ActorSpawner::ResolveNodeRef(Truck::Node::Ref const & node_ref)
{
    // Equivalent of `SerializedRig::parse_node_number()`, see https://github.com/only-a-ptr/ror-legacy-svn-trunk

    // Query named nodes if possible
    if (!m_node_names.empty() && node_ref.name != "")
    {
        auto itor = m_node_names.find(node_ref.name);
        if (itor != m_node_names.end())
        {
            return itor->second;
        }
    }

    // Query numbered nodes
    if (node_ref.number >= m_actor->ar_num_nodes)
    {
        this->AddMessage(Message::TYPE_ERROR,
            fmt::format("Fatal: requested non-existent node number {} (highest existing is {})",
                node_ref.number, m_actor->ar_num_nodes - 1));
        throw Exception("");
    }

    return node_ref.number;
}

std::pair<NodeIdx_t, bool> ActorSpawner::GetNodeIndex(Truck::Node::Ref const & node_ref, bool quiet /* = false */)
{
    return std::make_pair(this->ResolveNodeRef(node_ref), true);
}

node_t* ActorSpawner::GetNodePointer(Truck::Node::Ref const & node_ref)
{
    return &m_actor->ar_nodes[this->ResolveNodeRef(node_ref)];
}

node_t* ActorSpawner::GetNodePointerOrThrow(Truck::Node::Ref const & node_ref)
{
    return GetNodePointer(node_ref);
}

void ActorSpawner::ProcessNode(Truck::Node & def)
{

    // Set up node
    node_t& node = this->GetFreeNode();
    InitNode(node, m_spawn_position + def.position);
    node.nd_lockgroup = m_state.lockgroup_default;

    // Fill debug info
    m_actor->ar_nodes_name[node.pos] = def.name;
    m_actor->ar_nodes_id[node.pos] = def.number;
    m_actor->ar_nodes_name_top_length = std::max(m_actor->ar_nodes_name_top_length, (int)def.name.length());

    // Configure mass
    m_actor->ar_minimass[node.pos] = m_state.default_minimass;
    if (m_state.default_node_loadweight >= 0.f) // The >= operator is intentional.
    {
        // orig = further override of hardcoded default.
        node.mass = m_state.default_node_loadweight;
        node.nd_override_mass = true;
        node.nd_loaded_mass = true;
    }
    else
    {
        node.mass = 10; // Hardcoded in original (bts_nodes, call to init_node())
        node.nd_loaded_mass = false;
    }

    /* Options */
    std::string options = def.options + m_state.default_node_options;
    if (options.find(Truck::Node::OPTION_l_LOAD_WEIGHT) != std::string::npos)
    {
        node.nd_loaded_mass = true;
        if (def._num_args > 5) // Was override specified? 
        {
            node.nd_override_mass = true;
            node.mass = def.load_weight_override;
        }
        else
        {
            m_actor->m_masscount++;
        }
    }

    if (options.find(Truck::Node::OPTION_b_EXTRA_BUOYANCY) != std::string::npos)
    {
        node.buoyancy = 10000.0f;
    }

    if (options.find(Truck::Node::OPTION_h_HOOK_POINT) != std::string::npos)
    {

        // Link [current-node] -> [node-0]
        // If current node is 0, link [node-0] -> [node-1]
        node_t & node_2 = (node.pos == 0) ? GetNode(1) : GetNode(0);
        unsigned int beam_index = m_actor->ar_num_beams;

        // Set up beam
        beam_t & beam = AddBeam(node, node_2);
        beam.strength          = m_state.default_break * m_state.default_break_scale * 100.f;
        beam.bm_type           = BEAM_HYDRO;
        beam.d                 = m_state.default_damp * m_state.default_damp_scale * 0.1f;
        beam.k                 = m_state.default_spring * m_state.default_spring_scale;
        beam.bounded           = ROPE;
        beam.bm_disabled       = true;
        beam.L                 = HOOK_RANGE_DEFAULT;
        beam.refL              = HOOK_RANGE_DEFAULT;
        beam.default_beam_deform = m_state.default_deform * m_state.default_deform_scale;
        beam.minmaxposnegstress  = m_state.default_deform * m_state.default_deform_scale;
        beam.maxposstress        = m_state.default_deform * m_state.default_deform_scale;
        beam.maxnegstress        = m_state.default_deform * m_state.default_deform_scale;
        
        CreateBeamVisuals(beam, beam_index, false);

        // Logic cloned from SerializedRig.cpp, section BTS_NODES
        hook_t hook;
        hook.hk_hook_node         = & node;
        hook.hk_group             = -1;
        hook.hk_locked            = UNLOCKED;
        hook.hk_lock_node         = nullptr;
        hook.hk_locked_actor      = nullptr;
        hook.hk_lockgroup         = -1;
        hook.hk_beam              = & beam;
        hook.hk_maxforce          = HOOK_FORCE_DEFAULT;
        hook.hk_lockrange         = HOOK_RANGE_DEFAULT;
        hook.hk_lockspeed         = HOOK_SPEED_DEFAULT;
        hook.hk_selflock          = false;
        hook.hk_nodisable         = false;
        hook.hk_timer             = 0.0f;
        hook.hk_timer_preset      = HOOK_LOCK_TIMER_DEFAULT;
        hook.hk_autolock          = false;
        hook.hk_min_length        = 0.f;
        m_actor->ar_hooks.push_back(hook);
    }
    node.nd_no_ground_contact = options.find(Truck::Node::OPTION_c_NO_GROUND_CONTACT) != std::string::npos;

    // Order matters for flags 'm' and 'n'
    for (char c: options)
    {
        switch (c)
        {
        case Truck::Node::OPTION_m_NO_MOUSE_GRAB: node.nd_no_mouse_grab = true; break;
        case Truck::Node::OPTION_n_MOUSE_GRAB:    node.nd_no_mouse_grab = false; break;
        default:;
        }
    }

    m_actor->ar_exhaust_dir_node = options.find(Truck::Node::OPTION_y_EXHAUST_DIRECTION) != std::string::npos;
    m_actor->ar_exhaust_pos_node = options.find(Truck::Node::OPTION_x_EXHAUST_POINT) != std::string::npos;

    // Update "fusedrag" autocalc y & z span
    if (def.position.z < m_fuse_z_min) { m_fuse_z_min = def.position.z; }
    if (def.position.z > m_fuse_z_max) { m_fuse_z_max = def.position.z; }
    if (def.position.y < m_fuse_y_min) { m_fuse_y_min = def.position.y; }
    if (def.position.y > m_fuse_y_max) { m_fuse_y_max = def.position.y; }

    // GFX
    NodeGfx nfx(static_cast<uint16_t>(node.pos));
    nfx.nx_may_get_wet  = !node.nd_no_mouse_grab;
    nfx.nx_no_particles = options.find(Truck::Node::OPTION_p_NO_PARTICLES) != std::string::npos;
    nfx.nx_no_particles = options.find(Truck::Node::OPTION_p_NO_PARTICLES) != std::string::npos;
    nfx.nx_no_sparks    = options.find(Truck::Node::OPTION_f_NO_SPARKS) != std::string::npos;
    m_gfx_nodes.push_back(nfx);
}

void ActorSpawner::AddExhaust(
        unsigned int emitter_node_idx,
        unsigned int direction_node_idx
    )
{
    if (m_actor->m_disable_smoke)
    {
        return;
    }
    Exhaust exhaust;
    exhaust.emitterNode = emitter_node_idx;
    exhaust.directionNode = direction_node_idx;

    exhaust.smoker = App::GetGfxScene()->GetSceneManager()->createParticleSystem(
        this->ComposeName("Exhaust", static_cast<int>(m_actor->exhausts.size())),
        /*quota=*/500, // Default value
        m_custom_resource_group);

    if (exhaust.smoker == nullptr)
    {
        AddMessage(Message::TYPE_INTERNAL_ERROR, "Failed to create exhaust");
        return;
    }
    exhaust.smoker->setVisibilityFlags(DEPTHMAP_DISABLED); // Disable particles in depthmap

    Ogre::MaterialPtr mat = this->FindOrCreateCustomizedMaterial("tracks/Smoke");
    exhaust.smoker->setMaterialName(mat->getName(), mat->getGroup());

    exhaust.smokeNode = m_particles_parent_scenenode->createChildSceneNode();
    exhaust.smokeNode->attachObject(exhaust.smoker);
    exhaust.smokeNode->setPosition(m_actor->ar_nodes[exhaust.emitterNode].AbsPosition);

    // Update GFX for nodes
    for (NodeGfx& nfx : m_gfx_nodes)
    {
        if (nfx.nx_node_idx == emitter_node_idx || nfx.nx_node_idx == direction_node_idx)
        {
            nfx.nx_is_hot = true;
        }
    }

    m_actor->exhausts.push_back(exhaust);
}

void ActorSpawner::ProcessCinecam(Truck::Cinecam & def)
{
    // Node
    node_t & node = GetFreeNode();
    InitNode(node, m_spawn_position + def.position);
    node.nd_no_ground_contact = true; // Historical name 'contactless'
    node.friction_coef = NODE_FRICTION_COEF_DEFAULT;
    node.surface_coef = NODE_SURFACE_COEF_DEFAULT;
    node.volume_coef = NODE_VOLUME_COEF_DEFAULT;

    m_actor->ar_minimass[node.pos] = m_state.default_minimass;

    m_actor->ar_cinecam_node[m_actor->ar_num_cinecams] = node.pos;
    m_actor->ar_cinecam_node_predef_mass[m_actor->ar_num_cinecams] = node.mass;
    m_actor->ar_num_cinecams++;

    // Beams
    for (unsigned int i = 0; i < 8; i++)
    {
        int beam_index = m_actor->ar_num_beams;
        beam_t & beam = AddBeam(node, GetNode(def.nodes[i]));
        beam.bm_type = BEAM_NORMAL;
        CalculateBeamLength(beam);
        beam.k = def.spring;
        beam.d = def.damping;
    }
};

void ActorSpawner::InitNode(node_t & node, Ogre::Vector3 const & position)
{
    node.AbsPosition = position;
    node.RelPosition = position - m_actor->ar_origin;

    node.friction_coef = m_state.default_node_friction;
    node.volume_coef = m_state.default_node_volume;
    node.surface_coef = m_state.default_node_surface;

    node.buoyancy = m_state.truckmass / 15.f; // Ignore the 'b' flag in 'set_node_defaults' for backwards compatibility
    node.mass = 10.f;
}

void ActorSpawner::InitNode(
    node_t & node, 
    Ogre::Vector3 const & position,
    Truck::NodeDefaults& node_defaults
)
{
    InitNode(node, position);
    node.friction_coef = m_state.default_node_friction;
    node.volume_coef = m_state.default_node_surface;
    node.surface_coef = m_state.default_node_surface;
}

void ActorSpawner::ProcessGlobals(Truck::Globals & def)
{
    m_state.truckmass = def.dry_mass;
    m_state.loadmass = def.cargo_mass;

    // NOTE: Don't do any material pre-processing here; it'll be done on actual entities (via `SetupNewEntity()`).
    if (! def.material_name.empty())
    {
        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(def.material_name); // Check if exists (compatibility)
        if (!mat.isNull())
        {
            m_cab_material_name = def.material_name;
        }
        else
        {
            std::stringstream msg;
            msg << "Material '" << def.material_name << "' defined in section 'globals' not found. Trying material 'tracks/transred'";
            this->AddMessage(Message::TYPE_ERROR, msg.str());

            m_cab_material_name = "tracks/transred";
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Limits.
/* -------------------------------------------------------------------------- */

bool ActorSpawner::CheckParticleLimit(unsigned int count)
{
    if ((m_actor->ar_num_custom_particles + count) > MAX_CPARTICLES)
    {
        std::stringstream msg;
        msg << "Particle limit (" << MAX_CPARTICLES << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool ActorSpawner::CheckAxleLimit(unsigned int count)
{
    if ((m_actor->m_num_wheel_diffs + count) > MAX_WHEELS/2)
    {
        std::stringstream msg;
        msg << "Axle limit (" << MAX_WHEELS/2 << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

/* Static version */
bool ActorSpawner::CheckSoundScriptLimit(Actor *vehicle, unsigned int count)
{
    if ((vehicle->ar_num_soundsources + count) > MAX_SOUNDSCRIPTS_PER_TRUCK)
    {
        std::stringstream msg;
        msg << "SoundScript limit (" << MAX_SOUNDSCRIPTS_PER_TRUCK << ") exceeded";
        LOG(msg.str());
        return false;
    }
    return true;
}

bool ActorSpawner::CheckCabLimit(unsigned int count)
{
    if ((m_actor->ar_num_cabs + count) > MAX_CABS)
    {
        std::stringstream msg;
        msg << "Cab limit (" << MAX_CABS << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool ActorSpawner::CheckCameraRailLimit(unsigned int count)
{
    if ((m_actor->ar_num_camera_rails + count) > MAX_CAMERARAIL)
    {
        std::stringstream msg;
        msg << "CameraRail limit (" << MAX_CAMERARAIL << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool ActorSpawner::CheckAeroEngineLimit(unsigned int count)
{
    if ((m_actor->ar_num_aeroengines + count) > MAX_AEROENGINES)
    {
        std::stringstream msg;
        msg << "AeroEngine limit (" << MAX_AEROENGINES << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool ActorSpawner::CheckScrewpropLimit(unsigned int count)
{
    if ((m_actor->ar_num_screwprops + count) > MAX_SCREWPROPS)
    {
        std::stringstream msg;
        msg << "Screwprop limit (" << MAX_SCREWPROPS << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

node_t &ActorSpawner:: GetNode(NodeIdx_t node_index)
{
    return m_actor->ar_nodes[node_index];
}

void ActorSpawner::InitNode(unsigned int node_index, Ogre::Vector3 const & position)
{
    InitNode(m_actor->ar_nodes[node_index], position);
}

beam_t & ActorSpawner::GetBeam(unsigned int index)
{
    return m_actor->ar_beams[index];
}

node_t & ActorSpawner::GetFreeNode()
{
    if (m_actor->ar_num_nodes == node_t::INVALID_IDX)
    {
        this->AddMessage(Message::TYPE_ERROR,
            fmt::format("Fatal: actor can't have more than {} nodes.",
                node_t::INVALID_IDX));
        throw Exception("");
    }

    node_t & node = m_actor->ar_nodes[m_actor->ar_num_nodes];
    node.pos = m_actor->ar_num_nodes;
    m_actor->ar_num_nodes++;
    return node;
}

beam_t & ActorSpawner::GetFreeBeam()
{
    beam_t & beam = m_actor->ar_beams[m_actor->ar_num_beams];
    m_actor->ar_num_beams++;
    return beam;
}

shock_t & ActorSpawner::GetFreeShock()
{
    shock_t & shock = m_actor->ar_shocks[m_actor->ar_num_shocks];
    m_actor->ar_num_shocks++;
    return shock;
}

beam_t & ActorSpawner::GetAndInitFreeBeam(node_t & node_1, node_t & node_2)
{
    beam_t & beam = GetFreeBeam();
    beam.p1 = & node_1;
    beam.p2 = & node_2;
    return beam;
}

node_t & ActorSpawner::GetAndInitFreeNode(Ogre::Vector3 const & position)
{
    node_t & node = GetFreeNode();
    InitNode(node, position);
    return node;
}

void ActorSpawner::SetBeamSpring(beam_t & beam, float spring)
{
    beam.k = spring;
}

void ActorSpawner::SetBeamDamping(beam_t & beam, float damping)
{
    beam.d = damping;
}

void ActorSpawner::SetupDefaultSoundSources(Actor *vehicle)
{
    int trucknum = vehicle->ar_instance_id;
    int ar_exhaust_pos_node = vehicle->ar_exhaust_pos_node;

#ifdef USE_OPENAL
    if (App::GetSoundScriptManager()->isDisabled()) 
    {
        return;
    }

    //engine
    if (vehicle->ar_engine != nullptr) /* Land vehicle */
    {
        if (vehicle->ar_engine->m_engine_type == 't')
        {
            AddSoundSourceInstance(vehicle, "tracks/default_diesel", ar_exhaust_pos_node);
            AddSoundSourceInstance(vehicle, "tracks/default_force", ar_exhaust_pos_node);
            AddSoundSourceInstance(vehicle, "tracks/default_brakes", 0);
            AddSoundSourceInstance(vehicle, "tracks/default_parkbrakes", 0);
            AddSoundSourceInstance(vehicle, "tracks/default_reverse_beep", 0);
        }
        if (vehicle->ar_engine->m_engine_type == 'c')
            AddSoundSourceInstance(vehicle, "tracks/default_car", ar_exhaust_pos_node);
        if (vehicle->ar_engine->HasTurbo())
        {
            if (vehicle->ar_engine->m_turbo_inertia_factor >= 3)
                AddSoundSourceInstance(vehicle, "tracks/default_turbo_big", ar_exhaust_pos_node);
            else if (vehicle->ar_engine->m_turbo_inertia_factor <= 0.5)
                AddSoundSourceInstance(vehicle, "tracks/default_turbo_small", ar_exhaust_pos_node);
            else
                AddSoundSourceInstance(vehicle, "tracks/default_turbo_mid", ar_exhaust_pos_node);

            AddSoundSourceInstance(vehicle, "tracks/default_turbo_bov", ar_exhaust_pos_node);
            AddSoundSourceInstance(vehicle, "tracks/default_wastegate_flutter", ar_exhaust_pos_node);
        }

        if (vehicle->ar_engine->m_engine_has_air)
            AddSoundSourceInstance(vehicle, "tracks/default_air_purge", 0);
        //starter
        AddSoundSourceInstance(vehicle, "tracks/default_starter", 0);
        // turn signals
        AddSoundSourceInstance(vehicle, "tracks/default_turn_signal", 0);
    }
    if (vehicle->ar_driveable==TRUCK)
    {
        //horn
        if (vehicle->ar_is_police)
            AddSoundSourceInstance(vehicle, "tracks/default_police", 0);
        else
            AddSoundSourceInstance(vehicle, "tracks/default_horn", 0);
        //shift
            AddSoundSourceInstance(vehicle, "tracks/default_shift", 0);
    }
    //pump
    if (vehicle->m_has_command_beams)
    {
        AddSoundSourceInstance(vehicle, "tracks/default_pump", 0);
    }
    //antilock brake
    if (vehicle->alb_mode || !vehicle->alb_notoggle)
    {
        AddSoundSourceInstance(vehicle, "tracks/default_antilock", 0);
    }
    //tractioncontrol
    if (vehicle->tc_mode || !vehicle->tc_notoggle)
    {
        AddSoundSourceInstance(vehicle, "tracks/default_tractioncontrol", 0);
    }
    //screetch
    if ((vehicle->ar_driveable==TRUCK || vehicle->ar_driveable==AIRPLANE) && vehicle->ar_num_wheels != 0)
    {
        AddSoundSourceInstance(vehicle, "tracks/default_screetch", 0);
    }
    //break & creak
    AddSoundSourceInstance(vehicle, "tracks/default_break", 0);
    AddSoundSourceInstance(vehicle, "tracks/default_creak", 0);
    //boat engine
    if (vehicle->ar_driveable==BOAT)
    {
        if (vehicle->m_total_mass>50000.0)
            AddSoundSourceInstance(vehicle, "tracks/default_marine_large", ar_exhaust_pos_node);
        else
            AddSoundSourceInstance(vehicle, "tracks/default_marine_small", ar_exhaust_pos_node);
        //no start/stop engine for boats, so set sound always on!
        SOUND_START(trucknum, SS_TRIG_ENGINE);
        SOUND_MODULATE(trucknum, SS_MOD_ENGINE, 0.5);
    }
    //airplane warnings
    if (vehicle->ar_driveable==AIRPLANE)
    {
        AddSoundSourceInstance(vehicle, "tracks/default_gpws_10", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_gpws_20", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_gpws_30", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_gpws_40", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_gpws_50", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_gpws_100", 0);

        AddSoundSourceInstance(vehicle, "tracks/default_gpws_pullup", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_gpws_minimums", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_gpws_apdisconnect", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aoa_warning", 0);

        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat01", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat02", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat03", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat04", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat05", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat06", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat07", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat08", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat09", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat10", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat11", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat12", 0);
        AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat13", 0);
    }
    //airplane engines
    for (int i=0; i<vehicle->ar_num_aeroengines && i<8; i++)
    {
        int turbojet_node = vehicle->ar_aeroengines[i]->getNoderef();
        Ogre::String index_str = TOSTRING(i+1);

        if (vehicle->ar_aeroengines[i]->getType()==AEROENGINE_TURBOJET)
        {
            AddSoundSourceInstance(vehicle, "tracks/default_turbojet_start" + index_str, turbojet_node);
            AddSoundSourceInstance(vehicle, "tracks/default_turbojet_lopower" + index_str, turbojet_node);
            AddSoundSourceInstance(vehicle, "tracks/default_turbojet_hipower" + index_str, turbojet_node);
            if (((Turbojet*)(vehicle->ar_aeroengines[i]))->tjet_afterburnable)
            {
                AddSoundSourceInstance(vehicle, "tracks/default_turbojet_afterburner" + index_str, turbojet_node);
            }
        }
        else if (vehicle->ar_aeroengines[i]->getType()==AEROENGINE_TURBOPROP_PISTONPROP)
        {
            if (((Turboprop*)vehicle->ar_aeroengines[i])->is_piston)
            {
                AddSoundSourceInstance(vehicle, "tracks/default_pistonprop_start" + index_str, turbojet_node);
                AddSoundSourceInstance(vehicle, "tracks/default_pistonprop_lopower" + index_str, turbojet_node);
                AddSoundSourceInstance(vehicle, "tracks/default_pistonprop_hipower" + index_str, turbojet_node);
            }
            else
            {
                AddSoundSourceInstance(vehicle, "tracks/default_turboprop_start" + index_str, turbojet_node);
                AddSoundSourceInstance(vehicle, "tracks/default_turboprop_lopower" + index_str, turbojet_node);
                AddSoundSourceInstance(vehicle, "tracks/default_turboprop_hipower" + index_str, turbojet_node);
            }
        }
    }

    // linked sounds
    for (int i=0; i<vehicle->m_num_command_beams; i++)
    {
        AddSoundSource(vehicle, App::GetSoundScriptManager()->createInstance(Ogre::String("tracks/linked/default_command/extend"), trucknum, NULL, SL_COMMAND, i), 0);
        AddSoundSource(vehicle, App::GetSoundScriptManager()->createInstance(Ogre::String("tracks/linked/default_command/retract"), trucknum, NULL, SL_COMMAND, -i), 0);
    }

#endif //OPENAL
}

void ActorSpawner::UpdateCollcabContacterNodes()
{
    for (int i=0; i<m_actor->ar_num_collcabs; i++)
    {
        int tmpv = m_actor->ar_collcabs[i] * 3;
        m_actor->ar_nodes[m_actor->ar_cabs[tmpv]].nd_cab_node = true;
        m_actor->ar_nodes[m_actor->ar_cabs[tmpv+1]].nd_cab_node = true;
        m_actor->ar_nodes[m_actor->ar_cabs[tmpv+2]].nd_cab_node = true;
    }
    for (int i = 0; i < m_actor->ar_num_nodes; i++)
    {
        if (m_actor->ar_nodes[i].nd_contacter)
        {
            m_actor->ar_num_contactable_nodes++;
            m_actor->ar_num_contacters++;
        }
        else if (!m_actor->ar_nodes[i].nd_no_ground_contact &&
                 (m_actor->ar_nodes[i].nd_cab_node || m_actor->ar_nodes[i].nd_rim_node || m_actor->ar_num_collcabs == 0))
        {
            m_actor->ar_nodes[i].nd_contactable = true;
            m_actor->ar_num_contactable_nodes++;
        }
    }
}

Truck::MaterialFlareBinding* ActorSpawner::FindFlareBindingForMaterial(std::string const & material_name)
{
    for (auto& module: m_selected_modules)
    {
        for (auto& def: module->material_flare_bindings)
        {
            if (def.material_name == material_name)
            {
                return &def;
            }
        }
    }
    return nullptr;
}

Truck::VideoCamera* ActorSpawner::FindVideoCameraByMaterial(std::string const & material_name)
{
    for (auto& module: m_selected_modules)
    {
        for (auto& def: module->videocameras)
        {
            if (def.material_name == material_name)
            {
                return &def;
            }
        }
    }

    return nullptr;
}

Ogre::MaterialPtr ActorSpawner::FindOrCreateCustomizedMaterial(std::string mat_lookup_name)
{
    try
    {
        // Check for existing substitute
        auto lookup_res = m_material_substitutions.find(mat_lookup_name);
        if (lookup_res != m_material_substitutions.end())
        {
            return lookup_res->second.material;
        }

        CustomMaterial lookup_entry;

        // Query old-style mirrors (=special props, hardcoded material name 'mirror')
        if (mat_lookup_name == "mirror")
        {
            lookup_entry.mirror_prop_type = m_curr_mirror_prop_type;
            lookup_entry.mirror_prop_scenenode = m_curr_mirror_prop_scenenode;
            lookup_entry.material_flare_def = nullptr;
            static int mirror_counter = 0;
            const std::string new_mat_name = this->ComposeName("RenderMaterial", mirror_counter);
            ++mirror_counter;
            lookup_entry.material = Ogre::MaterialManager::getSingleton().getByName("mirror")->clone(new_mat_name, true, m_custom_resource_group);
            // Special case - register under generated name. This is because all mirrors use the same material 'mirror'
            m_material_substitutions.insert(std::make_pair(new_mat_name, lookup_entry));
            return lookup_entry.material; // Done!
        }

        // Query 'videocameras'
        Truck::VideoCamera* videocam_def = this->FindVideoCameraByMaterial(mat_lookup_name);
        if (videocam_def != nullptr)
        {
            Ogre::MaterialPtr video_mat_shared;
            auto found_managedmat = m_managed_materials.find(mat_lookup_name);
            if (found_managedmat != m_managed_materials.end())
            {
                video_mat_shared = found_managedmat->second;
            }
            else
            {
                video_mat_shared = Ogre::MaterialManager::getSingleton().getByName(mat_lookup_name);
            }

            if (!video_mat_shared.isNull())
            {
                lookup_entry.video_camera_def = videocam_def;
                const std::string video_mat_name = this->ComposeName(videocam_def->material_name.c_str(), 0);
                lookup_entry.material = video_mat_shared->clone(video_mat_name, true, m_custom_resource_group);
                m_material_substitutions.insert(std::make_pair(mat_lookup_name, lookup_entry));
                return lookup_entry.material; // Done!
            }
            else
            {
                std::stringstream msg;
                msg << "VideoCamera material '" << mat_lookup_name << "' not found! Ignoring videocamera.";
                this->AddMessage(Message::TYPE_WARNING, msg.str());
            }
        }

        // Resolve 'materialflarebindings'.
        Truck::MaterialFlareBinding* mat_flare_def = this->FindFlareBindingForMaterial(mat_lookup_name);
        if (mat_flare_def != nullptr)
        {
            lookup_entry.material_flare_def = mat_flare_def;
        }

        // Query .skin material replacements
        if (m_actor->m_used_skin_entry != nullptr)
        {
            std::shared_ptr<RoR::SkinDef>& skin_def = m_actor->m_used_skin_entry->skin_def;

            auto skin_res = skin_def->replace_materials.find(mat_lookup_name);
            if (skin_res != skin_def->replace_materials.end())
            {
                Ogre::MaterialPtr skin_mat = Ogre::MaterialManager::getSingleton().getByName(
                    skin_res->second, m_actor->m_used_skin_entry->resource_group);
                if (!skin_mat.isNull())
                {
                    std::stringstream name_buf;
                    name_buf << skin_mat->getName() << ACTOR_ID_TOKEN << m_actor->ar_instance_id;
                    lookup_entry.material = skin_mat->clone(name_buf.str(), /*changeGroup=*/true, m_custom_resource_group);
                    m_material_substitutions.insert(std::make_pair(mat_lookup_name, lookup_entry));
                    return lookup_entry.material;
                }
                else
                {
                    std::stringstream buf;
                    buf << "Material '" << skin_res->second << "' from skin '" << m_actor->m_used_skin_entry->dname
                        << "' not found (filename: '" << m_actor->m_used_skin_entry->fname 
                        << "', resource group: '"<< m_actor->m_used_skin_entry->resource_group
                        <<"')! Ignoring it...";
                    this->AddMessage(Message::TYPE_ERROR, buf.str());
                }
            }
        }

        // Acquire substitute - either use managedmaterial or generate new by cloning.
        auto mmat_res = m_managed_materials.find(mat_lookup_name);
        if (mmat_res != m_managed_materials.end())
        {
            // Use managedmaterial as substitute
            lookup_entry.material = mmat_res->second;
        }
        else
        {
            // Generate new substitute
            Ogre::MaterialPtr orig_mat = Ogre::MaterialManager::getSingleton().getByName(mat_lookup_name, m_custom_resource_group);
            if (orig_mat.isNull())
            {
                std::stringstream buf;
                buf << "Material doesn't exist:" << mat_lookup_name;
                this->AddMessage(Message::TYPE_ERROR, buf.str());
                return Ogre::MaterialPtr(); // NULL
            }

            std::stringstream name_buf;
            name_buf << orig_mat->getName() << ACTOR_ID_TOKEN << m_actor->ar_instance_id;
            lookup_entry.material = orig_mat->clone(name_buf.str(), true, m_custom_resource_group);
        }

        // Finally, query texture replacements - .skin and builtins
        for (auto& technique: lookup_entry.material->getTechniques())
        {
            for (auto& pass: technique->getPasses())
            {
                for (auto& tex_unit: pass->getTextureUnitStates())
                {
                    // Built-ins
                    if (tex_unit->getTextureName() == "dashtexture")
                    {
                        if (!m_oldstyle_renderdash)
                        {
                            // This is technically a bug, but does it matter at all? Let's watch ~ only_a_ptr, 05/2019
                            std::stringstream msg;
                            msg << "Warning: '" << mat_lookup_name
                                << "' references 'dashtexture', but Renderdash isn't created yet! Texture will be blank.";
                            this->AddMessage(Message::TYPE_WARNING, msg.str());
                        }
                        else
                        {
                            tex_unit->setTexture(m_oldstyle_renderdash->getTexture());
                        }
                    }
                    // .skin
                    else if (m_actor->m_used_skin_entry != nullptr)
                    {
                        const size_t num_frames = tex_unit->getNumFrames();
                        for (size_t i = 0; i < num_frames; ++i)
                        {
                            const auto end = m_actor->m_used_skin_entry->skin_def->replace_textures.end();
                            const auto query = m_actor->m_used_skin_entry->skin_def->replace_textures.find(tex_unit->getFrameTextureName((unsigned int)i));
                            if (query != end)
                            {
                                // Skin has replacement for this texture
                                if (m_actor->m_used_skin_entry->resource_group != m_custom_resource_group) // The skin comes from a SkinZip bundle (different resource group)
                                {
                                    Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().getByName(
                                        query->second, m_actor->m_used_skin_entry->resource_group);
                                    if (tex.isNull())
                                    {
                                        // `Ogre::TextureManager` doesn't automatically register all images in resource groups,
                                        // it waits for `Ogre::Resource`s to be created explicitly.
                                        // Normally this is done by `Ogre::MaterialManager` when loading a material.
                                        // In this case we must do it manually
                                        tex = Ogre::TextureManager::getSingleton().create(
                                            query->second, m_actor->m_used_skin_entry->resource_group);
                                    }
                                    tex_unit->_setTexturePtr(tex, i);
                                }
                                else // The skin lives in the vehicle bundle (same resource group)
                                {
                                    tex_unit->setFrameTextureName(query->second, (unsigned int)i);
                                }
                            }
                        }
                    }
                } // texture unit states
            } // passes
        } // techniques

        m_material_substitutions.insert(std::make_pair(mat_lookup_name, lookup_entry)); // Register the substitute
        return lookup_entry.material;
    }
    catch (Ogre::Exception& e)
    {
        std::stringstream msg;
        msg << "Exception while customizing material \"" << mat_lookup_name << "\", message: " << e.getFullDescription();
        this->AddMessage(Message::TYPE_ERROR, msg.str());
    }
    return Ogre::MaterialPtr(); // NULL
}

Ogre::MaterialPtr ActorSpawner::CreateSimpleMaterial(Ogre::ColourValue color)
{
    ROR_ASSERT(!m_simple_material_base.isNull());

    static unsigned int simple_mat_counter = 0;
    char name_buf[300];
    snprintf(name_buf, 300, "SimpleMaterial-%u%s%d", simple_mat_counter, ACTOR_ID_TOKEN, m_actor->ar_instance_id);
    Ogre::MaterialPtr newmat = m_simple_material_base->clone(name_buf);
    ++simple_mat_counter;
    newmat->getTechnique(0)->getPass(0)->setAmbient(color);

    return newmat;
}

void ActorSpawner::SetupNewEntity(Ogre::Entity* ent, Ogre::ColourValue simple_color)
{
    if (ent == nullptr)
    {
        // Dirty but I don't see any alternative ... ~ ulteq, 10/2018
        AddMessage(Message::TYPE_WARNING, "Failed to create entity: continuing without it ...");
        return;
    }

    // Use simple materials if applicable
    if (m_apply_simple_materials)
    {
        Ogre::MaterialPtr mat = this->CreateSimpleMaterial(simple_color);

        size_t num_sub_entities = ent->getNumSubEntities();
        for (size_t i = 0; i < num_sub_entities; i++)
        {
            Ogre::SubEntity* subent = ent->getSubEntity(i);
            subent->setMaterial(mat);
        }

        return; // Done!
    }

    // Create unique sub-entity (=instance of submesh) materials
    size_t subent_max = ent->getNumSubEntities();
    for (size_t i = 0; i < subent_max; ++i)
    {
        Ogre::SubEntity* subent = ent->getSubEntity(i);

        if (!subent->getMaterial().isNull())
        {
            Ogre::MaterialPtr own_mat = this->FindOrCreateCustomizedMaterial(subent->getMaterialName());
            if (!own_mat.isNull())
            {
                subent->setMaterial(own_mat);
            }
        }
    }
}

void ActorSpawner::CreateGfxActor()
{
    // Create the actor
    m_actor->m_gfx_actor = std::unique_ptr<RoR::GfxActor>(
        new RoR::GfxActor(m_actor, this, m_custom_resource_group, m_gfx_nodes, m_oldstyle_renderdash));

    m_actor->GetGfxActor()->UpdateSimDataBuffer(); // Initial fill (to setup flexing meshes)
}

void ActorSpawner::FinalizeGfxSetup()
{
    // Check and warn if there are unclaimed managed materials
    // TODO &*&*

    // Process special materials
    for (auto& entry: m_material_substitutions)
    {
        if (entry.second.material_flare_def != nullptr) // 'materialflarebindings'
        {
            m_actor->m_gfx_actor->AddMaterialFlare(
                entry.second.material_flare_def->flare_number, entry.second.material);
        }
        else if (entry.second.mirror_prop_type != CustomMaterial::MirrorPropType::MPROP_NONE) // special 'prop' - rear view mirror
        {
            this->CreateMirrorPropVideoCam(
                entry.second.material, entry.second.mirror_prop_type, entry.second.mirror_prop_scenenode);
        }
        else if (entry.second.video_camera_def != nullptr) // 'videocameras'
        {
            this->SetCurrentKeyword(Truck::KEYWORD_VIDEOCAMERA); // Logging
            this->CreateVideoCamera(entry.second.video_camera_def);
            this->SetCurrentKeyword(Truck::KEYWORD_NONE); // Logging
        }
    }

    if (!App::gfx_enable_videocams->GetBool())
    {
        m_actor->m_gfx_actor->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_DISABLED);
    }

    // Load dashboard layouts
    for (auto& module: m_selected_modules)
    {
        if (module->gui_settings.size() > 0)
        {
            for (std::string& layout: module->gui_settings.back().dashboard_layouts)
            {
                m_actor->ar_dashboard->loadDashBoard(layout, false);
            }

            for (std::string& layout: module->gui_settings.back().rtt_dashboard_layouts)
            {
                m_actor->ar_dashboard->loadDashBoard(layout, true);
            }
        }
    }

    // If none specified, load default dashboard layouts
    if (!m_actor->ar_dashboard->WasDashboardLoaded())
    {
        if (m_actor->ar_driveable == TRUCK) // load default for a truck
        {
            if (App::gfx_speedo_digital->GetBool())
            {
                if (App::gfx_speedo_imperial->GetBool())
                {
                    if (m_actor->ar_engine->getMaxRPM() > 3500)
                    {
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard7000_mph.layout", false); //7000 rpm tachometer thanks to Klink
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard7000_mph.layout", true);
                    }
                    else
                    {
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard3500_mph.layout", false);
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard3500_mph.layout", true);
                    }
                }
                else
                {
                    if (m_actor->ar_engine->getMaxRPM() > 3500)
                    {
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard7000.layout", false); //7000 rpm tachometer thanks to Klink
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard7000.layout", true);
                    }
                    else
                    {
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard3500.layout", false);
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard3500.layout", true);
                    }
                }
            }
            else // Analog speedometer
            {
                if (App::gfx_speedo_imperial->GetBool())
                {
                    if (m_actor->ar_engine->getMaxRPM() > 3500)
                    {
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard7000_analog_mph.layout", false); //7000 rpm tachometer thanks to Klink
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard7000_analog_mph.layout", true);
                    }
                    else
                    {
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard3500_analog_mph.layout", false);
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard3500_analog_mph.layout", true);
                    }
                }
                else
                {
                    if (m_actor->ar_engine->getMaxRPM() > 3500)
                    {
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard7000_analog.layout", false); //7000 rpm tachometer thanks to Klink
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard7000_analog.layout", true);
                    }
                    else
                    {
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard3500_analog.layout", false);
                        m_actor->ar_dashboard->loadDashBoard("default_dashboard3500_analog.layout", true);
                    }
                }
            }
        }
        else if (m_actor->ar_driveable == BOAT)
        {
            m_actor->ar_dashboard->loadDashBoard("default_dashboard_boat.layout", false);
            m_actor->ar_dashboard->loadDashBoard("default_dashboard_boat.layout", true);
        }
    }

    m_actor->ar_dashboard->setVisible(false);

    // Process rods (beam visuals)
    for (BeamVisualsTicket& bv: m_beam_visuals_queue)
    {
        int node1 = m_actor->ar_beams[bv.beam_index].p1->pos;
        int node2 = m_actor->ar_beams[bv.beam_index].p2->pos;
        m_actor->m_gfx_actor->AddRod(bv.beam_index, node1, node2, bv.material_name.c_str(), bv.visible, bv.diameter);
    }


    //add the cab visual
    if (m_state.texcoords.size() > 0 && m_actor->ar_num_cabs > 0)
    {
        //the cab materials are as follow:
        //texname: base texture with emissive(2 pass) or without emissive if none available(1 pass), alpha cutting
        //texname-trans: transparency texture (1 pass)
        //texname-back: backface texture: black+alpha cutting (1 pass)
        //texname-noem: base texture without emissive (1 pass), alpha cutting

        //material passes must be:
        //0: normal texture
        //1: transparent (windows)
        //2: emissive

        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(m_cab_material_name);
        if (mat.isNull())
        {
            Ogre::String msg = "Material '"+m_cab_material_name+"' missing!";
            AddMessage(Message::TYPE_ERROR, msg);
            return;
        }

        //-trans
        char transmatname[256];
        static int trans_counter = 0;
        sprintf(transmatname, "%s-trans-%d", m_cab_material_name.c_str(), trans_counter++);
        Ogre::MaterialPtr transmat=mat->clone(transmatname);
        if (mat->getTechnique(0)->getNumPasses()>1) // If there's the "emissive pass", remove it from the 'transmat'
        {
            transmat->getTechnique(0)->removePass(1);
        }
        transmat->getTechnique(0)->getPass(0)->setAlphaRejectSettings(Ogre::CMPF_LESS_EQUAL, 128);
        transmat->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
        if (transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
        {
            transmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_NONE);
        }
        transmat->compile();
        m_cab_trans_material = transmat;

        //-back
        char backmatname[256];
        static int back_counter = 0;
        sprintf(backmatname, "%s-back-%d", m_cab_material_name.c_str(), back_counter++);
        Ogre::MaterialPtr backmat=mat->clone(backmatname);
        if (mat->getTechnique(0)->getNumPasses()>1)// If there's the "emissive pass", remove it from the 'transmat'
        {
            backmat->getTechnique(0)->removePass(1);
        }
        if (transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
        {
            backmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setColourOperationEx(
                Ogre::LBX_SOURCE1, 
                Ogre::LBS_MANUAL, 
                Ogre::LBS_MANUAL, 
                Ogre::ColourValue(0,0,0),
                Ogre::ColourValue(0,0,0)
            );
        }
        if (App::gfx_reduce_shadows->GetBool())
        {
            backmat->setReceiveShadows(false);
        }
        backmat->compile();

        // Convert retro-style defs to modern-style defs
        std::vector<CabSubmesh> submeshes;
        for (int i = 0; i < m_state.free_sub; i++)
        {
            CabSubmesh submesh;
            submesh.backmesh_type = m_state.subisback[i];
            submesh.cabs_pos = m_state.subcabs[i];
            submesh.texcoords_pos = m_state.subtexcoords[i];
            submeshes.push_back(submesh);
        }

        char cab_material_name_cstr[1000] = {};
        strncpy(cab_material_name_cstr, m_cab_material_name.c_str(), 999);
        std::string mesh_name = this->ComposeName("VehicleCabMesh", 0);
        FlexObj* cab_mesh =new FlexObj(
            m_actor->m_gfx_actor.get(),
            m_actor->ar_nodes,
            m_state.texcoords,
            m_actor->ar_num_cabs,
            m_actor->ar_cabs,
            submeshes,
            cab_material_name_cstr,
            mesh_name.c_str(),
            backmatname,
            transmatname
        );

        Ogre::SceneNode* cab_scene_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
        Ogre::Entity *ec = nullptr;
        try
        {
            ec = App::GetGfxScene()->GetSceneManager()->createEntity(this->ComposeName("VehicleCabEntity", 0), mesh_name);
            this->SetupNewEntity(ec, Ogre::ColourValue(0.5, 1, 0.5));
            if (ec)
            {
                cab_scene_node->attachObject(ec);
            }

            // Process "emissive cab" materials
            auto search_itor = m_material_substitutions.find(m_cab_material_name);
            m_actor->m_gfx_actor->RegisterCabMaterial(search_itor->second.material, m_cab_trans_material);
            m_actor->m_gfx_actor->SetCabLightsActive(false); // Reset emissive lights to "off" state

            m_actor->GetGfxActor()->RegisterCabMesh(ec, cab_scene_node, cab_mesh);
        }
        catch (...)
        {
            this->AddMessage(Message::TYPE_ERROR, "error loading mesh: "+mesh_name);
            // TODO: do not leak memory here! ~ 08/2018
        }
    };

    // Process wheel visuals
    for (WheelVisualsTicket& ticket: m_wheel_visuals_queue)
    {
        if (ticket.wheel_def != nullptr)
        {
            this->CreateWheelVisuals(
                ticket.wheel_index,
                ticket.base_node_index,
                ticket.wheel_def->num_rays,
                ticket.wheel_def->face_material_name,
                ticket.wheel_def->band_material_name,
                false
                );
        }
        else if (ticket.wheel2_def != nullptr)
        {
            this->CreateWheelVisuals(
                ticket.wheel_index,
                ticket.base_node_index,
                ticket.wheel2_def->num_rays,
                ticket.wheel2_def->face_material_name,
                ticket.wheel2_def->band_material_name,
                true,
                ticket.wheel2_def->rim_radius / ticket.wheel2_def->tyre_radius
                );
        }
        else if (ticket.meshwheel_def != nullptr)
        {
            this->BuildMeshWheelVisuals(
                ticket.wheel_index,
                ticket.base_node_index,
                ticket.axis_node_1,
                ticket.axis_node_2,
                ticket.meshwheel_def->num_rays,
                ticket.meshwheel_def->mesh_name,
                ticket.meshwheel_def->material_name,
                ticket.meshwheel_def->rim_radius,
                ticket.meshwheel_def->side != Truck::MeshWheel::SIDE_RIGHT
                );
        }
        else if (ticket.flexbodywheel_def != nullptr)
        {
            Truck::FlexBodyWheel& def = *ticket.flexbodywheel_def;
            this->BuildMeshWheelVisuals(
                ticket.wheel_index,
                ticket.base_node_index,
                ticket.axis_node_1,
                ticket.axis_node_2,
                def.num_rays,
                def.rim_mesh_name,
                "tracks/trans", // Rim material name. Original parser: was hardcoded in BTS_FLEXBODYWHEELS
                def.rim_radius,
                def.side != Truck::MeshWheel::SIDE_RIGHT
                );

            int num_nodes = def.num_rays * 4;
            std::vector<NodeIdx_t> node_indices;
            node_indices.reserve(num_nodes);
            for (int i = 0; i < num_nodes; ++i)
            {
                node_indices.push_back( ticket.base_node_index + i );
            }

            Truck::Flexbody flexbody_def;
            flexbody_def.mesh_name = def.tyre_mesh_name;
            flexbody_def.offset = Ogre::Vector3(0.5,0,0);

            try
            {
                auto* flexbody = m_flex_factory.CreateFlexBody(
                    &flexbody_def,
                    ticket.axis_node_1,
                    ticket.axis_node_2,
                    static_cast<int>(ticket.base_node_index),
                    Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y),
                    node_indices,
                    m_custom_resource_group
                    );

                if (flexbody == nullptr)
                    return; // Error already logged

                this->CreateWheelSkidmarks(static_cast<unsigned>(ticket.wheel_index));

                m_actor->GetGfxActor()->AddFlexbody(flexbody);
            }
            catch (Ogre::Exception& e)
            {
                this->AddMessage(Message::TYPE_ERROR, 
                    "Failed to create flexbodywheel visuals '" + def.tyre_mesh_name + "', reason:" + e.getFullDescription());
            }
        }
    }

    m_actor->GetGfxActor()->RegisterAirbrakes();

    m_actor->GetGfxActor()->RegisterProps(m_props, m_driverseat_prop_index);

    if (m_state.helpmat != "")
    {
        try
        {
            Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(m_state.helpmat, m_custom_resource_group);
            m_actor->GetGfxActor()->GetAttributes().xa_help_mat = mat;
            if (mat &&
                mat->getNumTechniques() > 0 &&
                mat->getTechnique(0)->getNumPasses() > 0 &&
                mat->getTechnique(0)->getPass(0)->getNumTextureUnitStates() > 0 &&
                mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getNumFrames() > 0)
            {
                m_actor->GetGfxActor()->GetAttributes().xa_help_tex =
                    Ogre::TextureManager::getSingleton().getByName(
                        mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getFrameTextureName(0), m_custom_resource_group);
            }
        }
        catch (Ogre::Exception& e)
        {
            this->AddMessage(Message::TYPE_ERROR,
                "Failed to load `help` material '" + m_state.helpmat + "', message:" + e.getFullDescription());
        }
    }
}

void ActorSpawner::ValidateRotator(int id, int axis1, int axis2, NodeIdx_t *nodes1, NodeIdx_t *nodes2)
{
    const float eps = 0.001f;
    const Ogre::Vector3 ax1 = m_actor->ar_nodes[axis1].AbsPosition;
    const Ogre::Vector3 ax2 = m_actor->ar_nodes[axis2].AbsPosition;
    Ogre::Plane pl = Ogre::Plane((ax1 - ax2).normalisedCopy(), 0);

    Ogre::Vector3 a1 = pl.projectVector(ax1 - m_actor->ar_nodes[nodes1[0]].AbsPosition);
    Ogre::Vector3 a2 = pl.projectVector(ax1 - m_actor->ar_nodes[nodes1[1]].AbsPosition);
    Ogre::Vector3 a3 = pl.projectVector(ax1 - m_actor->ar_nodes[nodes1[2]].AbsPosition);
    Ogre::Vector3 a4 = pl.projectVector(ax1 - m_actor->ar_nodes[nodes1[3]].AbsPosition);
    float a1len = a1.normalise();
    float a2len = a2.normalise();
    float a3len = a3.normalise();
    float a4len = a4.normalise();
    if ((std::max(a1len, a3len) / std::min(a1len, a3len) > 1.f + eps) ||
        (std::max(a2len, a4len) / std::min(a2len, a4len) > 1.f + eps))
    {
        Ogre::String msg = Ogre::StringUtil::format("Off-centered axis on base plate of rotator %d", id);
        AddMessage(Message::TYPE_WARNING, msg);	
    }

    Ogre::Vector3 b1 = pl.projectVector(ax2 - m_actor->ar_nodes[nodes2[0]].AbsPosition);
    Ogre::Vector3 b2 = pl.projectVector(ax2 - m_actor->ar_nodes[nodes2[1]].AbsPosition);
    Ogre::Vector3 b3 = pl.projectVector(ax2 - m_actor->ar_nodes[nodes2[2]].AbsPosition);
    Ogre::Vector3 b4 = pl.projectVector(ax2 - m_actor->ar_nodes[nodes2[3]].AbsPosition);
    float b1len = b1.normalise();
    float b2len = b2.normalise();
    float b3len = b3.normalise();
    float b4len = b4.normalise();
    if ((std::max(b1len, b3len) / std::min(b1len, b3len) > 1.f + eps) ||
        (std::max(b2len, b4len) / std::min(b2len, b4len) > 1.f + eps))
    {
        Ogre::String msg = Ogre::StringUtil::format("Off-centered axis on rotating plate of rotator %d", id);
        AddMessage(Message::TYPE_WARNING, msg);	
    }

    float rot1 = a1.dotProduct(b1);
    float rot2 = a2.dotProduct(b2);
    float rot3 = a3.dotProduct(b3);
    float rot4 = a4.dotProduct(b4);
    if ((std::max(rot1, rot2) / std::min(rot1, rot2) > 1.f + eps) ||
        (std::max(rot2, rot3) / std::min(rot2, rot3) > 1.f + eps) ||
        (std::max(rot3, rot4) / std::min(rot3, rot4) > 1.f + eps) ||
        (std::max(rot4, rot1) / std::min(rot4, rot1) > 1.f + eps))
    {
        Ogre::String msg = Ogre::StringUtil::format("Misaligned plates on rotator %d", id);
        AddMessage(Message::TYPE_WARNING, msg);	
    }
}

Ogre::ManualObject* CreateVideocameraDebugMesh()
{
    // Create material
    static size_t counter = 0;
    Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(
        "VideoCamDebugMat-" + TOSTRING(counter), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    ++counter;
    mat->getTechnique(0)->getPass(0)->createTextureUnitState();
    mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
    mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
    mat->setLightingEnabled(false);
    mat->setReceiveShadows(false);
    // Create mesh
    Ogre::ManualObject* mo = App::GetGfxScene()->GetSceneManager()->createManualObject(); // TODO: Eliminate gEnv
    mo->begin(mat->getName(), Ogre::RenderOperation::OT_LINE_LIST);
    Ogre::ColourValue pos_mark_col(1.f, 0.82f, 0.26f);
    Ogre::ColourValue dir_mark_col(0.f, 1.f, 1.f); // TODO: This comes out green in simulation - why? ~ only_a_ptr, 05/2017
    const float pos_mark_len = 0.8f;
    const float dir_mark_len = 4.f;
    // X
    mo->position(pos_mark_len,0,0);
    mo->colour(pos_mark_col);
    mo->position(-pos_mark_len,0,0);
    mo->colour(pos_mark_col);
    // Y
    mo->position(0,pos_mark_len,0);
    mo->colour(pos_mark_col);
    mo->position(0,-pos_mark_len,0);
    mo->colour(pos_mark_col);
    // +Z
    mo->position(0,0,pos_mark_len);
    mo->colour(pos_mark_col);
    mo->position(0,0,0);
    mo->colour(pos_mark_col);
    // -Z = the direction
    mo->position(0,0,-dir_mark_len);
    mo->colour(dir_mark_col);
    mo->position(0,0,0);
    mo->colour(dir_mark_col);
    mo->end(); // Don't forget this!

    return mo;
}

void ActorSpawner::CreateVideoCamera(Truck::VideoCamera* def)
{
    try
    {
        RoR::VideoCamera vcam;

        vcam.vcam_material = this->FindOrCreateCustomizedMaterial(def->material_name);
        if (vcam.vcam_material.isNull())
        {
            this->AddMessage(Message::TYPE_ERROR, "Failed to create VideoCamera with material: " + def->material_name);
            return;
        }

        switch (def->camera_role)
        {
        case -1: vcam.vcam_type = VCTYPE_VIDEOCAM;          break;
        case  0: vcam.vcam_type = VCTYPE_TRACKING_VIDEOCAM; break;
        case  1: vcam.vcam_type = VCTYPE_MIRROR;            break;
        default:
            this->AddMessage(Message::TYPE_ERROR, "VideoCamera (mat: " + def->material_name + ") has invalid 'role': " + TOSTRING(def->camera_role));
            return;
        }

        vcam.vcam_node_center = static_cast<uint16_t>(this->GetNodeOrThrow(def->reference_node).pos);
        vcam.vcam_node_dir_y  = static_cast<uint16_t>(this->GetNodeOrThrow(def->bottom_node).pos);
        vcam.vcam_node_dir_z  = static_cast<uint16_t>(this->GetNodeOrThrow(def->left_node).pos);
        vcam.vcam_pos_offset  = def->offset;

        //rotate camera picture 180 degrees, skip for mirrors
        float rotation_z = (def->camera_role != 1) ? def->rotation.z + 180 : def->rotation.z;
        vcam.vcam_rotation
            = Ogre::Quaternion(Ogre::Degree(rotation_z), Ogre::Vector3::UNIT_Z)
            * Ogre::Quaternion(Ogre::Degree(def->rotation.y), Ogre::Vector3::UNIT_Y)
            * Ogre::Quaternion(Ogre::Degree(def->rotation.x), Ogre::Vector3::UNIT_X);

        // set alternative camposition (optional)
        if (def->alt_reference_node.IsValidAnyState())
        {
            vcam.vcam_node_alt_pos = static_cast<uint16_t>(this->GetNodeOrThrow(def->alt_reference_node).pos);
        }
        else
        {
            vcam.vcam_node_alt_pos = vcam.vcam_node_center;
        }

        // set alternative lookat position (optional)
        if (def->alt_orientation_node.IsValidAnyState())
        {
            // This is a tracker camera
            vcam.vcam_type = VCTYPE_TRACKING_VIDEOCAM;
            vcam.vcam_node_lookat = static_cast<uint16_t>(this->GetNodeOrThrow(def->alt_orientation_node).pos);
        }

        // TODO: Eliminate gEnv
        vcam.vcam_ogre_camera = App::GetGfxScene()->GetSceneManager()->createCamera(vcam.vcam_material->getName() + "_camera");

        if (!App::gfx_window_videocams->GetBool())
        {
            vcam.vcam_render_tex = Ogre::TextureManager::getSingleton().createManual(
                vcam.vcam_material->getName() + "_texture",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D,
                def->texture_width,
                def->texture_height,
                0, // no mip maps
                Ogre::PF_R8G8B8,
                Ogre::TU_RENDERTARGET);
            vcam.vcam_render_target = vcam.vcam_render_tex->getBuffer()->getRenderTarget();
            vcam.vcam_render_target->setAutoUpdated(false);
        }
        else
        {
            const std::string window_name = (!def->camera_name.empty()) ? def->camera_name : def->material_name;
            vcam.vcam_render_window = App::GetAppContext()->CreateCustomRenderWindow(window_name, def->texture_width, def->texture_height);
            vcam.vcam_render_window->setAutoUpdated(false);
            vcam.vcam_render_window->setDeactivateOnFocusChange(false);

            // TODO: disable texture mirrors
        }

        vcam.vcam_ogre_camera->setNearClipDistance(def->min_clip_distance);
        vcam.vcam_ogre_camera->setFarClipDistance(def->max_clip_distance);
        vcam.vcam_ogre_camera->setFOVy(Ogre::Degree(def->field_of_view));
        const float aspect_ratio = static_cast<float>(def->texture_width) / static_cast<float>(def->texture_height);
        vcam.vcam_ogre_camera->setAspectRatio(aspect_ratio);
        vcam.vcam_material->getTechnique(0)->getPass(0)->setLightingEnabled(false);
        vcam.vcam_off_tex_name = "Chrome.dds"; // Built-in gray texture

        if (vcam.vcam_render_target)
        {
            Ogre::Viewport* vp = vcam.vcam_render_target->addViewport(vcam.vcam_ogre_camera);
            vp->setClearEveryFrame(true);
            vp->setBackgroundColour(App::GetCameraManager()->GetCamera()->getViewport()->getBackgroundColour());
            vp->setVisibilityMask(~HIDE_MIRROR);
            vp->setVisibilityMask(~DEPTHMAP_DISABLED);
            vp->setOverlaysEnabled(false);

            vcam.vcam_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(vcam.vcam_render_tex->getName());

            // this is a mirror, flip the image left<>right to have a mirror and not a cameraimage
            if (def->camera_role == 1)
                vcam.vcam_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureUScale(-1);
        }

        if (vcam.vcam_render_window)
        {
            Ogre::Viewport* vp = vcam.vcam_render_window->addViewport(vcam.vcam_ogre_camera);
            vp->setClearEveryFrame(true);
            vp->setBackgroundColour(App::GetCameraManager()->GetCamera()->getViewport()->getBackgroundColour());
            vp->setVisibilityMask(~HIDE_MIRROR);
            vp->setVisibilityMask(~DEPTHMAP_DISABLED);
            vp->setOverlaysEnabled(false);
            vcam.vcam_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(vcam.vcam_off_tex_name);
        }

        if (App::diag_videocameras->GetBool())
        {
            Ogre::ManualObject* mo = CreateVideocameraDebugMesh(); // local helper function
            vcam.vcam_debug_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
            vcam.vcam_debug_node->attachObject(mo);
        }

        m_actor->m_gfx_actor->m_videocameras.push_back(vcam);
    }
    catch (std::exception & ex)
    {
        this->AddMessage(Message::TYPE_ERROR, ex.what());
    }
    catch (...)
    {
        this->AddMessage(Message::TYPE_ERROR, "An unknown exception has occured");
    }
}

void ActorSpawner::CreateMirrorPropVideoCam(
    Ogre::MaterialPtr custom_mat, CustomMaterial::MirrorPropType type, Ogre::SceneNode* prop_scenenode)
{
    static size_t mprop_counter = 0;
    try
    {
        // Prepare videocamera entry
        RoR::VideoCamera vcam;
        vcam.vcam_off_tex_name = "mirror.dds";
        vcam.vcam_prop_scenenode = prop_scenenode;
        switch (type)
        {
        case CustomMaterial::MirrorPropType::MPROP_LEFT:
            vcam.vcam_type = VCTYPE_MIRROR_PROP_LEFT;
            break;

        case CustomMaterial::MirrorPropType::MPROP_RIGHT:
            vcam.vcam_type = VCTYPE_MIRROR_PROP_RIGHT;
            break;

        default:
            this->AddMessage(Message::TYPE_ERROR, "Cannot create mirror prop of type 'MPROP_NONE'");
            return;
        }

        // Create rendering texture
        const std::string mirror_tex_name = this->ComposeName("MirrorPropTexture-", static_cast<int>(mprop_counter));
        vcam.vcam_render_tex = Ogre::TextureManager::getSingleton().createManual(mirror_tex_name
            , m_custom_resource_group
            , Ogre::TEX_TYPE_2D
            , 128
            , 256
            , 0
            , Ogre::PF_R8G8B8
            , Ogre::TU_RENDERTARGET);

        // Create OGRE camera
        vcam.vcam_ogre_camera = App::GetGfxScene()->GetSceneManager()->createCamera(this->ComposeName("MirrorPropCamera-", static_cast<int>(mprop_counter)));
        vcam.vcam_ogre_camera->setNearClipDistance(0.2f);
        vcam.vcam_ogre_camera->setFarClipDistance(App::GetCameraManager()->GetCamera()->getFarClipDistance());
        vcam.vcam_ogre_camera->setFOVy(Ogre::Degree(50));
        vcam.vcam_ogre_camera->setAspectRatio(
            (App::GetCameraManager()->GetCamera()->getViewport()->getActualWidth() / App::GetCameraManager()->GetCamera()->getViewport()->getActualHeight()) / 2.0f);

        // Setup rendering
        vcam.vcam_render_target = vcam.vcam_render_tex->getBuffer()->getRenderTarget();
        vcam.vcam_render_target->setActive(true);
        Ogre::Viewport* v = vcam.vcam_render_target->addViewport(vcam.vcam_ogre_camera);
        v->setClearEveryFrame(true);
        v->setBackgroundColour(App::GetCameraManager()->GetCamera()->getViewport()->getBackgroundColour());
        v->setOverlaysEnabled(false);

        // Setup material
        vcam.vcam_material = custom_mat;
        vcam.vcam_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(vcam.vcam_render_tex->getName());
        vcam.vcam_material->getTechnique(0)->getPass(0)->setLightingEnabled(false);

        // Submit the videocamera
        m_actor->m_gfx_actor->m_videocameras.push_back(vcam);
    }
    catch (std::exception & ex)
    {
        this->AddMessage(Message::TYPE_ERROR, ex.what());
    }
    catch (...)
    {
        this->AddMessage(Message::TYPE_ERROR, "An unknown exception has occured");
    }
    ++mprop_counter;
}

void ActorSpawner::HandleException()
{
    try { throw; } // Rethrow

    catch (Ogre::Exception& ogre_e)
    {
        // Add the message silently, OGRE already printed it to RoR.log
        RoR::Str<2000> txt;
        txt << "(Keyword: " << Truck::Document::KeywordToString(m_current_keyword)
            << ") " << ogre_e.getFullDescription();
        RoR::App::GetConsole()->putMessage(
            RoR::Console::CONSOLE_MSGTYPE_ACTOR, RoR::Console::CONSOLE_SYSTEM_ERROR, txt.ToCStr());
    }
    catch (std::exception& std_e)
    {
        this->AddMessage(Message::TYPE_ERROR, std_e.what());
    }
    catch (...)
    {
        this->AddMessage(Message::TYPE_ERROR, "An unknown exception has occurred");
    }
}

Ogre::ParticleSystem* ActorSpawner::CreateParticleSystem(std::string const & name, std::string const & template_name)
{
    // None of `Ogre::SceneManager::createParticleSystem()` overloads
    // lets us specify both resource group and template name.

    Ogre::NameValuePairList params;
    params["resourceGroup"] = m_custom_resource_group;
    params["templateName"] = template_name;

    Ogre::MovableObject* obj = App::GetGfxScene()->GetSceneManager()->createMovableObject(
       name, Ogre::ParticleSystemFactory::FACTORY_TYPE_NAME, &params);
    Ogre::ParticleSystem* psys = static_cast<Ogre::ParticleSystem*>(obj);
    psys->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
    return psys;
}

void ActorSpawner::ProcessNodeDefaults(int pos)
{
    int n = m_cur_module->node_defaults[pos]._num_args;

    m_state.default_node_loadweight          = m_cur_module->node_defaults[pos].loadweight;
    if (n > 2) m_state.default_node_friction = m_cur_module->node_defaults[pos].friction;
    if (n > 3) m_state.default_node_volume   = m_cur_module->node_defaults[pos].volume;
    if (n > 4) m_state.default_node_surface  = m_cur_module->node_defaults[pos].surface;
    if (n > 5) m_state.default_node_options  = m_cur_module->node_defaults[pos].options;

    if (m_state.default_node_friction < 0)   m_state.default_node_friction=NODE_FRICTION_COEF_DEFAULT;
    if (m_state.default_node_volume < 0)     m_state.default_node_volume=NODE_VOLUME_COEF_DEFAULT;
    if (m_state.default_node_surface < 0)    m_state.default_node_surface=NODE_SURFACE_COEF_DEFAULT;
    if (m_state.default_node_loadweight < 0) m_state.default_node_loadweight=NODE_LOADWEIGHT_DEFAULT;
    if (n <= 4) m_state.default_node_options = "";
}

void ActorSpawner::ProcessInertiaDefaults(int pos)
{
    int n = m_cur_module->inertia_defaults[pos]._num_args;
    m_state.inertia_startDelay                       = m_cur_module->inertia_defaults[pos].start_delay_factor;
    if (n > 2) m_state.inertia_stopDelay             = m_cur_module->inertia_defaults[pos].stop_delay_factor;
    if (n > 3) m_state.inertia_default_startFunction = m_cur_module->inertia_defaults[pos].start_function;
    if (n > 4) m_state.inertia_default_stopFunction  = m_cur_module->inertia_defaults[pos].stop_function;

    if (m_state.inertia_startDelay < 0 || m_state.inertia_stopDelay < 0)
    {
        // reset it
        m_state.inertia_startDelay = -1;
        m_state.inertia_stopDelay  = -1;
        m_state.inertia_default_startFunction = "";
        m_state.inertia_default_stopFunction = "";
    }
}

void ActorSpawner::ProcessBeamDefaults(int pos)
{
    int n = m_cur_module->beam_defaults[pos]._num_args;
    float tmpdefault_plastic_coef=-1.0f;

    m_state.default_spring =          m_cur_module->beam_defaults[pos].springiness;
    if (n > 2) m_state.default_damp            = m_cur_module->beam_defaults[pos].damping_constant;
    if (n > 3) m_state.default_deform          = m_cur_module->beam_defaults[pos].deformation_threshold;
    if (n > 4) m_state.default_break           = m_cur_module->beam_defaults[pos].breaking_threshold;
    if (n > 5) m_state.default_beam_diameter   = m_cur_module->beam_defaults[pos].visual_beam_diameter;
    if (n > 6) m_state.default_beam_material   = m_cur_module->beam_defaults[pos].beam_material_name;
    if (n > 7) tmpdefault_plastic_coef         = m_cur_module->beam_defaults[pos].plastic_deform_coef;

    if (m_state.default_spring<0) m_state.default_spring=DEFAULT_SPRING;
    if (m_state.default_damp<0) m_state.default_damp=DEFAULT_DAMP;
    if (m_state.default_deform<0) m_state.default_deform=BEAM_DEFORM;
    if (m_state.default_break<0) m_state.default_break=BEAM_BREAK;
    if (m_state.default_beam_diameter<0) m_state.default_beam_diameter=DEFAULT_BEAM_DIAMETER;

    // get the old 400k deformation limit for trucks with beams and set_beam_defaults based on old physics (pre beamv2)
    if (!m_state.enable_advanced_deformation && m_state.default_deform < BEAM_DEFORM)
    {
        m_state.default_deform=BEAM_DEFORM;
    }

    if (tmpdefault_plastic_coef >=0.0f)
    {
        m_state.beam_creak=0.0f;
        m_state.default_plastic_coef=m_cur_module->beam_defaults[pos].plastic_deform_coef;
    }
}

void ActorSpawner::ProcessBeamDefaultsScale(int pos)
{
    int n = m_cur_module->beam_defaults_scale[pos]._num_args;

    m_state.default_spring_scale            = m_cur_module->beam_defaults_scale[pos].springiness;
    if (n > 2) m_state.default_damp_scale   = m_cur_module->beam_defaults_scale[pos].damping_constant;
    if (n > 3) m_state.default_deform_scale = m_cur_module->beam_defaults_scale[pos].deformation_threshold_constant;
    if (n > 4) m_state.default_break_scale  = m_cur_module->beam_defaults_scale[pos].breaking_threshold_constant;
}

void ActorSpawner::ProcessCollisionRangePreset(int pos)
{
}

void ActorSpawner::ProcessManagedMatOptions(int pos)
{
    m_state.managedmaterials_doublesided = m_cur_module->managed_mat_options[pos].double_sided;
}

void ActorSpawner::ProcessSkeletonSettings(int pos)
{
}

