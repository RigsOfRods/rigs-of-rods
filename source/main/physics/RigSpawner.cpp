/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

/**	
    @file   RigSpawner.cpp
    @brief  Vehicle spawning logic.
    @author Petr Ohlidal
    @date   12/2013
*/

#include "RoRPrerequisites.h"
#include "RigSpawner.h"

#include "AirBrake.h"
#include "Airfoil.h"
#include "Application.h"
#include "AutoPilot.h"
#include "VehicleAI.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "BitFlags.h"
#include "Buoyance.h"
#include "CmdKeyInertia.h"
#include "Collisions.h"
#include "DashBoardManager.h"
#include "Differentials.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "FlexMesh.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "GfxActor.h"
#include "GUI_GameConsole.h"
#include "InputEngine.h"
#include "MeshObject.h"
#include "OgreSubsystem.h"
#include "PointColDetector.h"
#include "RigLoadingProfilerControl.h"
#include "RoRFrameListener.h"
#include "ScrewProp.h"
#include "Settings.h"
#include "Skidmark.h"
#include "SkinManager.h"
#include "SlideNode.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TorqueCurve.h"
#include "TurboJet.h"
#include "TurboProp.h"
#include "GUIManager.h"
#include "Utils.h"

#include <OgreMaterialManager.h>
#include <OgreSceneManager.h>
#include <OgreMovableObject.h>
#include <OgreParticleSystem.h>
#include <OgreEntity.h>

const char* ACTOR_ID_TOKEN = "@Actor_"; // Appended to material name, followed by actor ID (aka 'trucknum')

using namespace RoR;

/* -------------------------------------------------------------------------- */
/* Prepare for loading
/* -------------------------------------------------------------------------- */

void RigSpawner::Setup( 
    Beam *rig,
    std::shared_ptr<RigDef::File> file,
    Ogre::SceneNode *parent,
    Ogre::Vector3 const & spawn_position,
    int cache_entry_number
)
{
    SPAWNER_PROFILE_SCOPED();

    m_rig = rig;
    m_file = file;
    m_cache_entry_number = cache_entry_number;
    m_parent_scene_node = parent;
    m_spawn_position = spawn_position;
    m_current_keyword = RigDef::File::KEYWORD_INVALID;
    m_enable_background_loading = BSETTING("Background Loading", false);
    m_wing_area = 0.f;
    m_fuse_z_min = 1000.0f;
    m_fuse_z_max = -1000.0f;
    m_fuse_y_min = 1000.0f;
    m_fuse_y_max = -1000.0f;
    m_first_wing_index = -1;

    m_generate_wing_position_lights = true;
    // TODO: Handle modules
    if (file->root_module->engine.get() != nullptr) // Engine present => it's a land vehicle.
    {
        m_generate_wing_position_lights = false; // Disable aerial pos. lights for land vehicles.
    }

    if (App::diag_extra_resource_dir.GetActive() != App::diag_extra_resource_dir.GetPending())
    {
        App::diag_extra_resource_dir.ApplyPending();
    }
    if (! App::diag_extra_resource_dir.IsActiveEmpty())
    {
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
            App::diag_extra_resource_dir.GetActive(), "FileSystem", "customInclude");
        Ogre::ResourceBackgroundQueue::getSingleton().initialiseResourceGroup("customInclude");
    }

    m_messages_num_errors = 0;
    m_messages_num_warnings = 0;
    m_messages_num_other = 0;
}

void RigSpawner::CalcMemoryRequirements(ActorMemoryRequirements& req, RigDef::File::Module* module_def)
{
    // 'nodes'
    req.num_nodes += module_def->nodes.size();
    for (auto& def: module_def->nodes)
    {
        if (BITMASK_IS_1(def.options, RigDef::Node::OPTION_h_HOOK_POINT))
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

    // 'shocks' and 'shocks2'
    req.num_beams  += module_def->shocks.size();
    req.num_shocks += module_def->shocks.size();
    req.num_beams  += module_def->shocks_2.size();
    req.num_shocks += module_def->shocks_2.size();

    // 'commands' and 'commands2' (unified)
    req.num_beams += module_def->commands_2.size();

    // 'rotators'
    req.num_rotators += module_def->rotators.size();
    req.num_rotators += module_def->rotators_2.size();

    // 'wings'
    req.num_wings += module_def->wings.size();

    // 'wheels'
    for (RigDef::Wheel& wheel: module_def->wheels)
    {
        req.num_nodes += wheel.num_rays * 2; // BuildWheelObjectAndNodes()
        req.num_beams += wheel.num_rays * ((wheel.rigidity_node.IsValidAnyState()) ? 9 : 8); // BuildWheelBeams()
    }

    // 'wheels2'
    for (RigDef::Wheel2& wheel2: module_def->wheels_2)
    {
        req.num_nodes += wheel2.num_rays * 4;
        // Rim beams:  num_rays*10 (*11 with valid rigidity_node)
        // Tyre beams: num_rays*14
        req.num_beams += wheel2.num_rays * ((wheel2.rigidity_node.IsValidAnyState()) ? 25 : 24);
    }

    // 'meshwheels' & 'meshwheels2' (unified)
    for (RigDef::MeshWheel& meshwheel: module_def->mesh_wheels)
    {
        req.num_nodes += meshwheel.num_rays * 2; // BuildWheelObjectAndNodes()
        req.num_beams += meshwheel.num_rays * ((meshwheel.rigidity_node.IsValidAnyState()) ? 9 : 8); // BuildWheelBeams()
    }

    // 'flexbodywheels'
    for (RigDef::FlexBodyWheel& flexwheel: module_def->flex_body_wheels)
    {
        req.num_nodes += flexwheel.num_rays * 4;
        // Rim beams:      num_rays*8
        // Tyre beams:     num_rays*10 (num_rays*11 with valid rigidity_node)
        // Support beams:  num_rays*2
        req.num_beams += flexwheel.num_rays * ((flexwheel.rigidity_node.IsValidAnyState()) ? 21 : 20);
    }
}

void RigSpawner::InitializeRig()
{
    SPAWNER_PROFILE_SCOPED();

    ActorMemoryRequirements req;
    for (auto module: m_selected_modules) // _Root_ module is included
    {
        this->CalcMemoryRequirements(req, module.get());
    }

    // Allocate memory as needed
    m_rig->ar_beams = new beam_t[req.num_beams];
    m_rig->ar_nodes = new node_t[req.num_nodes];

    if (req.num_shocks > 0)
        m_rig->ar_shocks = new shock_t[req.num_shocks];

    if (req.num_rotators > 0)
        m_rig->ar_rotators = new rotator_t[req.num_rotators];

    if (req.num_wings > 0)
        m_rig->ar_wings = new wing_t[req.num_wings];

    // clear rig parent structure
    memset(m_rig->contacters, 0, sizeof(contacter_t) * MAX_CONTACTERS);
    m_rig->free_contacter = 0;
    memset(m_rig->wheels, 0, sizeof(wheel_t) * MAX_WHEELS);
    m_rig->free_wheel = 0;
    memset(m_rig->vwheels, 0, sizeof(vwheel_t) * MAX_WHEELS);
    m_rig->ropes.clear();
    m_rig->ropables.clear();
    m_rig->ties.clear();
    m_rig->hooks.clear();

    // commands contain complex data structures, do not memset them ...
    for (int i=0;i<MAX_COMMANDS+1;i++)
    {
        m_rig->commandkey[i].commandValue=0;
        m_rig->commandkey[i].beams.clear();
        m_rig->commandkey[i].rotators.clear();
        m_rig->commandkey[i].description="";
    }

    memset(m_rig->ar_props, 0, sizeof(prop_t) * MAX_PROPS);
    m_rig->ar_num_props = 0;
    m_rig->exhausts.clear();
    memset(m_rig->cparticles, 0, sizeof(cparticle_t) * MAX_CPARTICLES);
    m_rig->free_cparticle = 0;
    m_rig->nodes_debug.clear();
    m_rig->beams_debug.clear();
    memset(m_rig->soundsources, 0, sizeof(soundsource_t) * MAX_SOUNDSCRIPTS_PER_TRUCK);
    m_rig->free_soundsource = 0;
    memset(m_rig->pressure_beams, 0, sizeof(int) * MAX_PRESSURE_BEAMS);
    m_rig->free_pressure_beam = 0;
    memset(m_rig->aeroengines, 0, sizeof(AeroEngine *) * MAX_AEROENGINES);
    m_rig->free_aeroengine = 0;
    memset(m_rig->cabs, 0, sizeof(int) * (MAX_CABS*3));
    m_rig->free_cab = 0;
    memset(m_rig->hydro, 0, sizeof(int) * MAX_HYDROS);
    m_rig->free_hydro = 0;
    memset(m_rig->collcabs, 0, sizeof(int) * MAX_CABS);
    memset(m_rig->inter_collcabrate, 0, sizeof(collcab_rate_t) * MAX_CABS);
    m_rig->free_collcab = 0;
    memset(m_rig->intra_collcabrate, 0, sizeof(collcab_rate_t) * MAX_CABS);
    memset(m_rig->buoycabs, 0, sizeof(int) * MAX_CABS);
    m_rig->free_buoycab = 0;
    memset(m_rig->buoycabtypes, 0, sizeof(int) * MAX_CABS);
    memset(m_rig->airbrakes, 0, sizeof(Airbrake *) * MAX_AIRBRAKES);
    m_rig->free_airbrake = 0;
    memset(m_rig->m_skid_trails, 0, sizeof(Skidmark *) * (MAX_WHEELS*2));
    memset(m_rig->flexbodies, 0, sizeof(FlexBody *) * MAX_FLEXBODIES);
    m_rig->free_flexbody = 0;
    m_rig->description.clear();
    m_rig->free_camerarail = 0;
    m_rig->free_screwprop = 0;

    m_rig->realtruckname = "";
    m_rig->forwardcommands=false;

    m_rig->wheel_contact_requested = false;
    m_rig->rescuer = false;
    m_rig->has_slope_brake=false;
    m_rig->ar_extern_camera_mode=0;
    m_rig->ar_extern_camera_node=-1;
    m_rig->authors.clear();
    m_rig->m_slidenodes_connect_on_spawn=false;

    m_rig->m_odometer_total = 0;
    m_rig->m_odometer_user  = 0;

    m_rig->m_masscount=0;
    m_rig->disable_smoke = App::gfx_particles_mode.GetActive() == 0;
    m_rig->ar_exhaust_pos_node=0;
    m_rig->ar_exhaust_dir_node=0;
    m_rig->m_beam_break_debug_enabled  = App::diag_log_beam_break.GetActive();
    m_rig->m_beam_deform_debug_enabled = App::diag_log_beam_deform.GetActive();
    m_rig->m_trigger_debug_enabled    = App::diag_log_beam_trigger.GetActive();
    m_rig->m_rotator_inertia = nullptr;
    m_rig->m_hydro_inertia = nullptr;
    m_rig->m_command_inertia = nullptr;
    m_rig->ar_origin=Ogre::Vector3::ZERO;
    m_rig->m_slidenodes.clear();

    m_rig->ar_cinecam_node[0]=-1;
    m_rig->ar_num_cinecams=0;
    m_rig->deletion_sceneNodes.clear();
    m_rig->deletion_Objects.clear();
    m_rig->m_net_custom_lights[0] = UINT_MAX;
    m_rig->m_net_custom_lights[1] = UINT_MAX;
    m_rig->m_net_custom_lights[2] = UINT_MAX;
    m_rig->m_net_custom_lights[3] = UINT_MAX;
    m_rig->m_net_custom_light_count = 0;

    m_rig->ar_sim_state = Beam::SimState::LOCAL_SLEEPING;
    m_rig->ar_use_heathaze=false;
    m_rig->m_fusealge_airfoil = nullptr;
    m_rig->m_fusealge_front = nullptr;
    m_rig->m_fusealge_back = nullptr;
    m_rig->m_fusealge_width=0;
    m_rig->ar_brake_force=30000.0;
    m_rig->m_handbrake_force = 2 * m_rig->ar_brake_force;
    m_rig->m_debug_visuals = SETTINGS.getBooleanSetting("DebugBeams", false);
    m_rig->m_gfx_reduce_shadows = SETTINGS.getBooleanSetting("Shadow optimizations", true);

    m_rig->m_num_proped_wheels=0;
    m_rig->m_num_braked_wheels=0;

    m_rig->ar_speedo_max_kph=140;
    m_rig->ar_num_cameras=0;
    m_rig->m_cab_mesh = nullptr;
    m_rig->m_cab_scene_node = nullptr;
    m_rig->ar_camera_node_pos[0]=-1;
    m_rig->ar_camera_node_dir[0]=-1;
    m_rig->ar_camera_node_roll[0]=-1;
    m_rig->ar_lowest_node=0;

#ifdef USE_ANGELSCRIPT
    m_rig->ar_vehicle_ai = new VehicleAI(m_rig);
#endif // USE_ANGELSCRIPT

    /* Init code from Beam::Beam() */

    m_rig->ar_airbrake_intensity = 0;
    m_rig->alb_minspeed = 0.0f;
    m_rig->alb_mode = 0;
    m_rig->alb_notoggle = false;
    m_rig->alb_present = false;
    m_rig->alb_pulse_time = 2000.0f;
    m_rig->alb_pulse_state = false;
    m_rig->alb_ratio = 0.0f;
    m_rig->alb_timer = 0.0f;
    m_rig->ar_anim_shift_timer = 0.0f;
    m_rig->antilockbrake = 0;

    m_rig->m_cab_mesh = nullptr;

    m_rig->cc_mode = false;
    m_rig->cc_can_brake = false;
    m_rig->cc_target_rpm = 0.0f;
    m_rig->cc_target_speed = 0.0f;
    m_rig->cc_target_speed_lower_limit = 0.0f;

    m_rig->ar_collision_relevant = false;

    m_rig->m_debug_visuals = 0;

    m_rig->ar_driverseat_prop = nullptr;

    m_rig->ar_use_heathaze = !m_rig->disable_smoke && App::gfx_enable_heathaze.GetActive();
    m_rig->ar_hide_in_actor_list = false;

    m_rig->ar_anim_previous_crank = 0.f;

    m_rig->sl_enabled = false;
    m_rig->sl_speed_limit = 0.f;

    m_rig->tc_fade = 0.f;
    m_rig->tc_mode = 0;
    m_rig->tc_notoggle = false;
    m_rig->tc_present = false;
    m_rig->tc_pulse_time = 2000.0f;
    m_rig->tc_pulse_state = false;
    m_rig->tc_ratio = 0.f;
    m_rig->tc_wheelslip = 0.f;
    m_rig->tc_timer = 0.f;

    m_rig->tractioncontrol = 0;


    m_rig->ar_dashboard = new DashBoardManager();


#ifdef FEAT_TIMING
    // this enables beam engine timing statistics
    statistics = BES.getClient(tnum, BES_CORE);
    statistics_gfx = BES.getClient(tnum, BES_GFX);
#endif

    m_rig->m_skeletonview_scenenode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    m_rig->deletion_sceneNodes.emplace_back(m_rig->m_skeletonview_scenenode);
    
    m_rig->m_beam_visuals_parent_scenenode = m_parent_scene_node;

    /* Collisions */

    m_rig->ar_disable_actor2actor_collision = BSETTING("DisableCollisions", false);
    if (! m_rig->ar_disable_actor2actor_collision)
    {
        m_rig->m_inter_point_col_detector = new PointColDetector();
    }

    m_rig->ar_disable_self_collision = BSETTING("DisableSelfCollisions", false);
    if (! m_rig->ar_disable_self_collision)
    {
        m_rig->m_intra_point_col_detector = new PointColDetector();
    }

    m_rig->ar_submesh_ground_model = gEnv->collisions->defaultgm;

    DustManager& dustman = RoR::App::GetSimController()->GetBeamFactory()->GetParticleManager();
    m_rig->m_particles_dust   = dustman.getDustPool("dust");
    m_rig->m_particles_drip   = dustman.getDustPool("drip");
    m_rig->m_particles_sparks = dustman.getDustPool("sparks");
    m_rig->m_particles_clump  = dustman.getDustPool("clump");
    m_rig->m_particles_splash = dustman.getDustPool("splash");
    m_rig->m_particles_ripple = dustman.getDustPool("ripple");

    m_rig->m_command_inertia   = new CmdKeyInertia();
    m_rig->m_hydro_inertia = new CmdKeyInertia();
    m_rig->m_rotator_inertia  = new CmdKeyInertia();

    // Lights mode
    m_rig->m_flares_mode = App::gfx_flares_mode.GetActive();

    m_rig->m_definition = m_file;

    m_flex_factory = RoR::FlexFactory(
        this,
        BSETTING("Flexbody_UseCache", false),
        m_cache_entry_number
        );

    m_flex_factory.CheckAndLoadFlexbodyCache();

    m_custom_resource_group = this->ComposeName("ResourceGroup", 1);
    Ogre::ResourceGroupManager::getSingleton().createResourceGroup(m_custom_resource_group);

    m_placeholder_managedmat = RoR::OgreSubsystem::GetMaterialByName("rigsofrods/managedmaterial-placeholder"); // Built-in

    m_apply_simple_materials = BSETTING("SimpleMaterials", false);
    if (m_apply_simple_materials)
    {
        m_simple_material_base = RoR::OgreSubsystem::GetMaterialByName("tracks/simple"); // Built-in material
        if (m_simple_material_base.isNull())
        {
            this->AddMessage(Message::TYPE_INTERNAL_ERROR, "Failed to load built-in material 'tracks/simple'; disabling 'SimpleMaterials'");
            m_apply_simple_materials = false;
        }
    }

    m_curr_mirror_prop_type = CustomMaterial::MirrorPropType::MPROP_NONE;
    m_curr_mirror_prop_scenenode = nullptr;
}

void RigSpawner::FinalizeRig()
{
    SPAWNER_PROFILE_SCOPED();

    // we should post-process the torque curve if existing
    if (m_rig->ar_engine)
    {
        int result = m_rig->ar_engine->getTorqueCurve()->spaceCurveEvenly(m_rig->ar_engine->getTorqueCurve()->getUsedSpline());
        if (result)
        {
            m_rig->ar_engine->getTorqueCurve()->setTorqueModel("default");
            if (result == 1)
            {
                AddMessage(Message::TYPE_ERROR, "TorqueCurve: Points (rpm) must be in an ascending order. Using default curve");
            }
        }

        //Gearbox
        m_rig->ar_engine->setAutoMode(App::sim_gearbox_mode.GetActive());
    }
    
    //calculate gwps height offset
    //get a starting value
    m_rig->ar_posnode_spawn_height=m_rig->ar_nodes[0].RelPosition.y;
    //start at 0 to avoid a crash whith a 1-node truck
    for (int i=0; i<m_rig->ar_num_nodes; i++)
    {
        // scan and store the y-coord for the lowest node of the truck
        if (m_rig->ar_nodes[i].RelPosition.y <= m_rig->ar_posnode_spawn_height)
        {
            m_rig->ar_posnode_spawn_height = m_rig->ar_nodes[i].RelPosition.y;
        }
    }

    if (m_rig->ar_camera_node_pos[0] > 0)
    {
        // store the y-difference between the trucks lowest node and the campos-node for the gwps system
        m_rig->ar_posnode_spawn_height = m_rig->ar_nodes[m_rig->ar_camera_node_pos[0]].RelPosition.y - m_rig->ar_posnode_spawn_height;
    } 
    else
    {
        //this can not be an airplane, just set it to 0.
        m_rig->ar_posnode_spawn_height = 0.0f;
    }

    //cameras workaround
    for (int i=0; i<m_rig->ar_num_cameras; i++)
    {
        //LogManager::getSingleton().logMessage("Camera dir="+StringConverter::toString(ar_nodes[ar_camera_node_dir[i]].RelPosition-ar_nodes[ar_camera_node_pos[i]].RelPosition)+" roll="+StringConverter::toString(ar_nodes[ar_camera_node_roll[i]].RelPosition-ar_nodes[ar_camera_node_pos[i]].RelPosition));
        Ogre::Vector3 dir_node_offset = GetNode(m_rig->ar_camera_node_dir[i]).RelPosition - GetNode(m_rig->ar_camera_node_pos[i]).RelPosition;
        Ogre::Vector3 roll_node_offset = GetNode(m_rig->ar_camera_node_roll[i]).RelPosition - GetNode(m_rig->ar_camera_node_pos[i]).RelPosition;
        Ogre::Vector3 cross = dir_node_offset.crossProduct(roll_node_offset);
        
        m_rig->ar_camera_node_roll_inv[i]=cross.y > 0;//(ar_nodes[ar_camera_node_dir[i]].RelPosition-ar_nodes[ar_camera_node_pos[i]].RelPosition).crossProduct(ar_nodes[ar_camera_node_roll[i]].RelPosition-ar_nodes[ar_camera_node_pos[i]].RelPosition).y>0;
        if (m_rig->ar_camera_node_roll_inv[i])
        {
            AddMessage(Message::TYPE_WARNING, "camera definition is probably invalid and has been corrected. It should be center, back, left");
        }
    }
    
    //wing closure
    if (m_first_wing_index!=-1)
    {
        if (m_rig->ar_autopilot != nullptr) 
        {
            m_rig->ar_autopilot->setInertialReferences(
                & GetNode(m_airplane_left_light),
                & GetNode(m_airplane_right_light),
                m_rig->m_fusealge_back,
                & GetNode(m_rig->ar_camera_node_pos[0])
                );
        }
        //inform wing segments
        float span=GetNode(m_rig->ar_wings[m_first_wing_index].fa->nfrd).RelPosition.distance(GetNode(m_rig->ar_wings[m_rig->ar_num_wings-1].fa->nfld).RelPosition);
        
        m_rig->ar_wings[m_first_wing_index].fa->enableInducedDrag(span,m_wing_area, false);
        m_rig->ar_wings[m_rig->ar_num_wings-1].fa->enableInducedDrag(span,m_wing_area, true);
        //wash calculator
        WashCalculator();
    }
    //add the cab visual
    if (!m_oldstyle_cab_texcoords.empty() && m_rig->free_cab>0)
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

        Ogre::MaterialPtr mat = RoR::OgreSubsystem::GetMaterialByName(m_cab_material_name);
        if (mat.isNull())
        {
            Ogre::String msg = "Material '"+m_cab_material_name+"' missing!";
            AddMessage(Message::TYPE_ERROR, msg);
            return;
        }

        //-trans
        char transmatname[256];
        sprintf(transmatname, "%s-trans", m_cab_material_name.c_str());
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
        sprintf(backmatname, "%s-back", m_cab_material_name.c_str());
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
        if (m_rig->m_gfx_reduce_shadows)
        {
            backmat->setReceiveShadows(false);
        }
        backmat->compile();

        char cab_material_name_cstr[1000] = {};
        strncpy(cab_material_name_cstr, m_cab_material_name.c_str(), 999);
        std::string mesh_name = this->ComposeName("VehicleCabMesh", 0);
        m_rig->m_cab_mesh =new FlexObj( // Names in FlexObj ctor
            m_rig->ar_nodes,            // node_t* nds
            m_oldstyle_cab_texcoords,// std::vector<CabNodeTexcoords>& texcoords
            m_rig->free_cab,         // int     numtriangles
            m_rig->cabs,             // int*    triangles
            m_oldstyle_cab_submeshes,// std::vector<CabSubmesh>& submeshes
            cab_material_name_cstr,          // char*   texname
            mesh_name.c_str(),
            backmatname,             // char*   backtexname
            transmatname             // char*   transtexname
        );

        m_rig->m_cab_scene_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        Ogre::Entity *ec = nullptr;
        try
        {
            ec = gEnv->sceneManager->createEntity(this->ComposeName("VehicleCabEntity", 0), mesh_name);
            this->SetupNewEntity(ec, Ogre::ColourValue(0.5, 1, 0.5));
            if (ec)
            {
                m_rig->m_cab_scene_node->attachObject(ec);
            }
        }
        catch (...)
        {
            this->AddMessage(Message::TYPE_ERROR, "error loading mesh: "+mesh_name);
        }
        m_rig->m_cab_entity = ec;
    };

    m_rig->ar_lowest_node = FindLowestNodeInRig();
    m_rig->ar_lowest_contacting_node = FindLowestContactingNodeInRig();

    this->UpdateCollcabContacterNodes();

    m_flex_factory.SaveFlexbodiesToCache();

    this->FinalizeGfxSetup(); // Creates the GfxActor
}

/* -------------------------------------------------------------------------- */
/* Actual loading
/* ~~~ Implemented in RigSpawner_ProcessControl.cpp!
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* Processing functions and utilities.
/* -------------------------------------------------------------------------- */

void RigSpawner::WashCalculator()
{
    SPAWNER_PROFILE_SCOPED();

    //we will compute wash
    int w,p;
    for (p=0; p<m_rig->free_aeroengine; p++)
    {
        Ogre::Vector3 prop=m_rig->ar_nodes[m_rig->aeroengines[p]->getNoderef()].RelPosition;
        float radius=m_rig->aeroengines[p]->getRadius();
        for (w=0; w<m_rig->ar_num_wings; w++)
        {
            //left wash
            Ogre::Vector3 wcent=((m_rig->ar_nodes[m_rig->ar_wings[w].fa->nfld].RelPosition+m_rig->ar_nodes[m_rig->ar_wings[w].fa->nfrd].RelPosition)/2.0);
            //check if wing is near enough along X (less than 15m back)
            if (wcent.x>prop.x && wcent.x<prop.x+15.0)
            {
                //check if it's okay vertically
                if (wcent.y>prop.y-radius && wcent.y<prop.y+radius)
                {
                    //okay, compute wash coverage ratio along Z
                    float wleft=(m_rig->ar_nodes[m_rig->ar_wings[w].fa->nfld].RelPosition).z;
                    float wright=(m_rig->ar_nodes[m_rig->ar_wings[w].fa->nfrd].RelPosition).z;
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
                        m_rig->ar_wings[w].fa->addwash(p, wratio);
                        Ogre::String msg = "Wing "+TOSTRING(w)+" is washed by prop "+TOSTRING(p)+" at "+TOSTRING((float)(wratio*100.0))+"%";
                        AddMessage(Message::TYPE_INFO, msg);
                    }
                }
            }
        }
    }
}

void RigSpawner::ProcessTurbojet(RigDef::Turbojet & def)
{
    SPAWNER_PROFILE_SCOPED();

    int front,back,ref;
    front = GetNodeIndexOrThrow(def.front_node);
    back  = GetNodeIndexOrThrow(def.back_node);
    ref   = GetNodeIndexOrThrow(def.side_node);
    
    Turbojet *tj=new Turbojet(
        m_rig->free_aeroengine, 
        m_rig->ar_instance_id, 
        m_rig->ar_nodes, 
        front, 
        back, 
        ref, 
        def.dry_thrust,
        def.is_reversable != 0,
        def.wet_thrust,
        def.front_diameter, 
        m_rig->ar_use_heathaze);
    
    // Visuals
    std::string nozzle_name = this->ComposeName("TurbojetNozzle", m_rig->free_aeroengine);
    Ogre::Entity* nozzle_ent = gEnv->sceneManager->createEntity(nozzle_name, "nozzle.mesh");
    this->SetupNewEntity(nozzle_ent, Ogre::ColourValue(1, 0.5, 0.5));
    Ogre::Entity* afterburn_ent = nullptr;
    if (def.wet_thrust > 0.f)
    {
        std::string flame_name = this->ComposeName("AfterburnerFlame", m_rig->free_aeroengine);
        afterburn_ent = gEnv->sceneManager->createEntity(flame_name, "abflame.mesh");
        this->SetupNewEntity(afterburn_ent, Ogre::ColourValue(1, 1, 0));
    }
    std::string propname = this->ComposeName("Turbojet", m_rig->free_aeroengine);
    tj->SetupVisuals(propname, nozzle_ent, def.back_diameter, def.nozzle_length, afterburn_ent, m_rig->disable_smoke);
    
    m_rig->aeroengines[m_rig->free_aeroengine]=tj;
    m_rig->ar_driveable=AIRPLANE;
    if (m_rig->ar_autopilot == nullptr && m_rig->ar_sim_state != Beam::SimState::NETWORKED_OK)
    {
        m_rig->ar_autopilot=new Autopilot(m_rig->ar_instance_id);
    }

    m_rig->free_aeroengine++;
}

std::string RigSpawner::ComposeName(const char* type, int number)
{
    char buf[500];
    snprintf(buf, 500, "%s_%d%s%d", type, number, ACTOR_ID_TOKEN, m_rig->ar_instance_id);
    return buf;
}

// static
void RigSpawner::ComposeName(RoR::Str<100>& str, const char* type, int number, int actor_id)
{
    str.Clear();
    str << type << "_" << number << ACTOR_ID_TOKEN << actor_id;
}

void RigSpawner::ProcessScrewprop(RigDef::Screwprop & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (! CheckScrewpropLimit(1))
    {
        return;
    }

    int ref_node_idx = GetNodeIndexOrThrow(def.prop_node);
    int back_node_idx = GetNodeIndexOrThrow(def.back_node);
    int top_node_idx = GetNodeIndexOrThrow(def.top_node);

    m_rig->screwprops[m_rig->free_screwprop] = new Screwprop(
        &RoR::App::GetSimController()->GetBeamFactory()->GetParticleManager(),
        m_rig->ar_nodes,
        ref_node_idx,
        back_node_idx,
        top_node_idx,
        def.power,
        m_rig->ar_instance_id
    );
    m_rig->ar_driveable=BOAT;
    m_rig->free_screwprop++;
}

void RigSpawner::ProcessFusedrag(RigDef::Fusedrag & def)
{
    SPAWNER_PROFILE_SCOPED();

    //parse fusedrag
    int front_node_idx = GetNodeIndexOrThrow(def.front_node);
    int back_node_idx = GetNodeIndexOrThrow(def.rear_node);
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

        m_rig->m_fusealge_airfoil = new Airfoil(fusefoil);

        m_rig->m_fusealge_front   = & GetNode(front_node_idx);
        m_rig->m_fusealge_back    = & GetNode(front_node_idx); // This equals v0.38 / v0.4.0.7, but it's probably a bug
        m_rig->m_fusealge_width   = width;
        AddMessage(Message::TYPE_INFO, "Fusedrag autocalculation size: "+TOSTRING(width)+" m^2");
    } 
    else
    {
        // original fusedrag calculation

        width  = def.approximate_width;

        m_rig->m_fusealge_airfoil = new Airfoil(fusefoil);

        m_rig->m_fusealge_front   = & GetNode(front_node_idx);
        m_rig->m_fusealge_back    = & GetNode(front_node_idx); // This equals v0.38 / v0.4.0.7, but it's probably a bug
        m_rig->m_fusealge_width   = width;
    }
}

void RigSpawner::BuildAerialEngine(
    int ref_node_index,
    int back_node_index,
    int blade_1_node_index,
    int blade_2_node_index,
    int blade_3_node_index,
    int blade_4_node_index,
    int couplenode_index,
    bool is_turboprops,
    Ogre::String const & airfoil,
    float power,
    float pitch
    )
{
    SPAWNER_PROFILE_SCOPED();

    Turboprop *turbo_prop = new Turboprop(
        this->ComposeName("Turboprop", m_rig->free_aeroengine).c_str(),
        m_rig->ar_nodes, 
        ref_node_index,
        back_node_index,
        blade_1_node_index,
        blade_2_node_index,
        blade_3_node_index,
        blade_4_node_index,
        couplenode_index,
        power,
        airfoil,
        m_rig->free_aeroengine,
        m_rig->ar_instance_id,
        m_rig->disable_smoke,
        ! is_turboprops,
        pitch,
        m_rig->ar_use_heathaze
    );
    m_rig->aeroengines[m_rig->free_aeroengine] = turbo_prop;
    m_rig->free_aeroengine++;
    m_rig->ar_driveable = AIRPLANE;

    /* Autopilot */
    if (m_rig->ar_autopilot == nullptr && m_rig->ar_sim_state != Beam::SimState::NETWORKED_OK)
    {
        m_rig->ar_autopilot = new Autopilot(m_rig->ar_instance_id);
    }

    /* Visuals */
    float scale = GetNode(ref_node_index).RelPosition.distance(GetNode(blade_1_node_index).RelPosition) / 2.25f;
    for (unsigned int i = 0; i < static_cast<unsigned int>(m_rig->ar_num_props); i++)
    {
        prop_t & prop = m_rig->ar_props[i];
        if (prop.noderef == ref_node_index)
        {
            if (prop.pale == 1)
            {
                prop.scene_node->scale(scale, scale, scale);
                turbo_prop->addPale(prop.scene_node);
            }
            if (prop.spinner == 1)
            {
                prop.scene_node->scale(scale, scale, scale);
                turbo_prop->addSpinner(prop.scene_node);
            }
        }
    }
}

void RigSpawner::ProcessTurboprop2(RigDef::Turboprop2 & def)
{
    SPAWNER_PROFILE_SCOPED();

    int p3_node_index = (def.blade_tip_nodes[2].IsValidAnyState()) ? GetNodeIndexOrThrow(def.blade_tip_nodes[2]) : -1;
    int p4_node_index = (def.blade_tip_nodes[3].IsValidAnyState()) ? GetNodeIndexOrThrow(def.blade_tip_nodes[3]) : -1;
    int couple_node_index = (def.couple_node.IsValidAnyState()) ? GetNodeIndexOrThrow(def.couple_node) : -1;

    BuildAerialEngine(
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

void RigSpawner::ProcessPistonprop(RigDef::Pistonprop & def)
{
    SPAWNER_PROFILE_SCOPED();

    int p3_node_index = (def.blade_tip_nodes[2].IsValidAnyState()) ? GetNodeIndexOrThrow(def.blade_tip_nodes[2]) : -1;
    int p4_node_index = (def.blade_tip_nodes[3].IsValidAnyState()) ? GetNodeIndexOrThrow(def.blade_tip_nodes[3]) : -1;
    int couple_node_index = (def.couple_node.IsValidAnyState()) ? GetNodeIndexOrThrow(def.couple_node) : -1;

    BuildAerialEngine(
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

void RigSpawner::ProcessAirbrake(RigDef::Airbrake & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (! CheckAirBrakeLimit(1))
    {
        return;
    }

    m_rig->airbrakes[m_rig->free_airbrake] = new Airbrake(
        this->ComposeName("Airbrake", m_rig->free_airbrake).c_str(),
        m_rig->free_airbrake, 
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
    );
    m_rig->free_airbrake++;
}

void RigSpawner::ProcessWing(RigDef::Wing & def)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_first_wing_index != -1) && (m_rig->ar_wings[m_rig->ar_num_wings - 1].fa == nullptr))
    {
        this->AddMessage(Message::TYPE_ERROR, "Unable to process wing, previous wing has no Airfoil");
        return;
    }

    // Create airfoil
    int node_indices[8];
    for (unsigned int i = 0; i < 8; i++)
    {
        node_indices[i] = this->GetNodeIndexOrThrow(def.nodes[i]);
    }

    const std::string wing_name = this->ComposeName("Wing", m_rig->ar_num_wings);
    auto flex_airfoil = new FlexAirfoil(
        wing_name,
        m_rig->ar_nodes,
        node_indices[0],
        node_indices[1],
        node_indices[2],
        node_indices[3],
        node_indices[4],
        node_indices[5],
        node_indices[6],
        node_indices[7],
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
        m_rig->aeroengines,
        m_rig->ar_sim_state != Beam::SimState::NETWORKED_OK
    );

    Ogre::Entity* entity = nullptr;
    try
    {
        const std::string wing_instance_name = this->ComposeName("WingEntity", m_rig->ar_num_wings);
        entity = gEnv->sceneManager->createEntity(wing_instance_name, wing_name);
        m_rig->deletion_Entities.emplace_back(entity);
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
        m_first_wing_index = m_rig->ar_num_wings;
        m_wing_area=ComputeWingArea(
            this->GetNode(flex_airfoil->nfld).AbsPosition,    this->GetNode(flex_airfoil->nfrd).AbsPosition,
            this->GetNode(flex_airfoil->nbld).AbsPosition,    this->GetNode(flex_airfoil->nbrd).AbsPosition
        );
    }
    else
    {
        wing_t & previous_wing = m_rig->ar_wings[m_rig->ar_num_wings - 1];

        if (node_indices[1] != previous_wing.fa->nfld)
        {
            wing_t & start_wing    = m_rig->ar_wings[m_first_wing_index];

            //discontinuity
            //inform wing segments
            float span = GetNode( start_wing.fa->nfrd).RelPosition.distance( GetNode(previous_wing.fa->nfld).RelPosition );
            
            start_wing.fa->enableInducedDrag(span, m_wing_area, false);
            previous_wing.fa->enableInducedDrag(span, m_wing_area, true);

            //we want also to add positional lights for first wing
            if (m_generate_wing_position_lights && (m_rig->m_flares_mode != GfxFlaresMode::NONE))
            {
                if (! CheckPropLimit(4))
                {
                    return;
                }

                //Left green
                m_airplane_left_light=previous_wing.fa->nfld;
                prop_t & left_green_prop = m_rig->ar_props[m_rig->ar_num_props];
                m_rig->ar_num_props++;

                left_green_prop.noderef=previous_wing.fa->nfld;
                left_green_prop.nodex=previous_wing.fa->nflu;
                left_green_prop.nodey=previous_wing.fa->nfld; //ignored
                left_green_prop.offsetx=0.5;
                left_green_prop.offsety=0.0;
                left_green_prop.offsetz=0.0;
                left_green_prop.rot=Ogre::Quaternion::IDENTITY;
                left_green_prop.wheel=nullptr;
                left_green_prop.wheelrotdegree=0.0;
                left_green_prop.mirror=0;
                left_green_prop.pale=0;
                left_green_prop.spinner=0;
                left_green_prop.scene_node=nullptr; //no visible prop
                left_green_prop.beacon_light_rotation_angle[0]=0.0;
                left_green_prop.beacon_light_rotation_rate[0]=1.0;
                left_green_prop.beacontype='L';
                left_green_prop.beacon_light[0]=nullptr; //no light
                //the flare billboard
                left_green_prop.beacon_flare_billboard_scene_node[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
                left_green_prop.beacon_flares_billboard_system[0]=gEnv->sceneManager->createBillboardSet(this->ComposeName("Prop", m_rig->ar_num_props),1);
                left_green_prop.beacon_flares_billboard_system[0]->createBillboard(0,0,0);
                if (left_green_prop.beacon_flares_billboard_system[0])
                {
                    left_green_prop.beacon_flares_billboard_system[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    left_green_prop.beacon_flares_billboard_system[0]->setMaterialName("tracks/greenflare");
                    left_green_prop.beacon_flare_billboard_scene_node[0]->attachObject(left_green_prop.beacon_flares_billboard_system[0]);
                }
                left_green_prop.beacon_flare_billboard_scene_node[0]->setVisible(false);
                left_green_prop.beacon_flares_billboard_system[0]->setDefaultDimensions(0.5, 0.5);
                left_green_prop.animFlags[0]=0;
                left_green_prop.animMode[0]=0;
                
                //Left flash
                prop_t & left_flash_prop = m_rig->ar_props[m_rig->ar_num_props];
                m_rig->ar_num_props++;

                left_flash_prop.noderef=previous_wing.fa->nbld;
                left_flash_prop.nodex=previous_wing.fa->nblu;
                left_flash_prop.nodey=previous_wing.fa->nbld; //ignored
                left_flash_prop.offsetx=0.5;
                left_flash_prop.offsety=0.0;
                left_flash_prop.offsetz=0.0;
                left_flash_prop.rot=Ogre::Quaternion::IDENTITY;
                left_flash_prop.wheel=nullptr;
                left_flash_prop.wheelrotdegree=0.0;
                left_flash_prop.mirror=0;
                left_flash_prop.pale=0;
                left_flash_prop.spinner=0;
                left_flash_prop.scene_node=nullptr; //no visible prop
                left_flash_prop.beacon_light_rotation_angle[0]=0.5; //alt
                left_flash_prop.beacon_light_rotation_rate[0]=1.0;
                left_flash_prop.beacontype='w';
                //light
                std::string prop_name = this->ComposeName("Prop", m_rig->ar_num_props);
                left_flash_prop.beacon_light[0]=gEnv->sceneManager->createLight(prop_name);
                left_flash_prop.beacon_light[0]->setType(Ogre::Light::LT_POINT);
                left_flash_prop.beacon_light[0]->setDiffuseColour( Ogre::ColourValue(1.0, 1.0, 1.0));
                left_flash_prop.beacon_light[0]->setSpecularColour( Ogre::ColourValue(1.0, 1.0, 1.0));
                left_flash_prop.beacon_light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
                left_flash_prop.beacon_light[0]->setCastShadows(false);
                left_flash_prop.beacon_light[0]->setVisible(false);
                //the flare billboard
                left_flash_prop.beacon_flare_billboard_scene_node[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
                left_flash_prop.beacon_flares_billboard_system[0]=gEnv->sceneManager->createBillboardSet(prop_name,1);
                left_flash_prop.beacon_flares_billboard_system[0]->createBillboard(0,0,0);
                if (left_flash_prop.beacon_flares_billboard_system[0])
                {
                    left_flash_prop.beacon_flares_billboard_system[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    left_flash_prop.beacon_flares_billboard_system[0]->setMaterialName("tracks/flare");
                    left_flash_prop.beacon_flare_billboard_scene_node[0]->attachObject(left_flash_prop.beacon_flares_billboard_system[0]);
                }
                left_flash_prop.beacon_flare_billboard_scene_node[0]->setVisible(false);
                left_flash_prop.beacon_flares_billboard_system[0]->setDefaultDimensions(1.0, 1.0);
                
                //Right red
                m_airplane_right_light=previous_wing.fa->nfrd;
                prop_t & right_red_prop = m_rig->ar_props[m_rig->ar_num_props];
                m_rig->ar_num_props++;

                
                right_red_prop.noderef=start_wing.fa->nfrd;
                right_red_prop.nodex=start_wing.fa->nfru;
                right_red_prop.nodey=start_wing.fa->nfrd; //ignored
                right_red_prop.offsetx=0.5;
                right_red_prop.offsety=0.0;
                right_red_prop.offsetz=0.0;
                right_red_prop.rot=Ogre::Quaternion::IDENTITY;
                right_red_prop.wheel=nullptr;
                right_red_prop.wheelrotdegree=0.0;
                right_red_prop.mirror=0;
                right_red_prop.pale=0;
                right_red_prop.spinner=0;
                right_red_prop.scene_node=nullptr; //no visible prop
                right_red_prop.beacon_light_rotation_angle[0]=0.0;
                right_red_prop.beacon_light_rotation_rate[0]=1.0;
                right_red_prop.beacontype='R';
                right_red_prop.beacon_light[0]=nullptr; /* No light */
                //the flare billboard
                right_red_prop.beacon_flare_billboard_scene_node[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
                right_red_prop.beacon_flares_billboard_system[0]=gEnv->sceneManager->createBillboardSet(this->ComposeName("Prop", m_rig->ar_num_props),1);
                right_red_prop.beacon_flares_billboard_system[0]->createBillboard(0,0,0);
                if (right_red_prop.beacon_flares_billboard_system[0])
                {
                    right_red_prop.beacon_flares_billboard_system[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    right_red_prop.beacon_flares_billboard_system[0]->setMaterialName("tracks/redflare");
                    right_red_prop.beacon_flare_billboard_scene_node[0]->attachObject(right_red_prop.beacon_flares_billboard_system[0]);
                }
                right_red_prop.beacon_flare_billboard_scene_node[0]->setVisible(false);
                right_red_prop.beacon_flares_billboard_system[0]->setDefaultDimensions(0.5, 0.5);
                right_red_prop.animFlags[0]=0;
                right_red_prop.animMode[0]=0;
                
                //Right flash
                prop_t & right_flash_prop = m_rig->ar_props[m_rig->ar_num_props];
                m_rig->ar_num_props++;

                right_flash_prop.noderef=start_wing.fa->nbrd;
                right_flash_prop.nodex=start_wing.fa->nbru;
                right_flash_prop.nodey=start_wing.fa->nbrd; //ignored
                right_flash_prop.offsetx=0.5;
                right_flash_prop.offsety=0.0;
                right_flash_prop.offsetz=0.0;
                right_flash_prop.rot=Ogre::Quaternion::IDENTITY;
                right_flash_prop.wheel=nullptr;
                right_flash_prop.wheelrotdegree=0.0;
                right_flash_prop.mirror=0;
                right_flash_prop.pale=0;
                right_flash_prop.spinner=0;
                right_flash_prop.scene_node=nullptr; //no visible prop
                right_flash_prop.beacon_light_rotation_angle[0]=0.5; //alt
                right_flash_prop.beacon_light_rotation_rate[0]=1.0;
                right_flash_prop.beacontype='w';
                //light
                prop_name = this->ComposeName("Prop", m_rig->ar_num_props);
                right_flash_prop.beacon_light[0]=gEnv->sceneManager->createLight(prop_name);
                right_flash_prop.beacon_light[0]->setType(Ogre::Light::LT_POINT);
                right_flash_prop.beacon_light[0]->setDiffuseColour( Ogre::ColourValue(1.0, 1.0, 1.0));
                right_flash_prop.beacon_light[0]->setSpecularColour( Ogre::ColourValue(1.0, 1.0, 1.0));
                right_flash_prop.beacon_light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
                right_flash_prop.beacon_light[0]->setCastShadows(false);
                right_flash_prop.beacon_light[0]->setVisible(false);
                //the flare billboard
                right_flash_prop.beacon_flare_billboard_scene_node[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
                right_flash_prop.beacon_flares_billboard_system[0]=gEnv->sceneManager->createBillboardSet(prop_name,1);
                right_flash_prop.beacon_flares_billboard_system[0]->createBillboard(0,0,0);
                if (right_flash_prop.beacon_flares_billboard_system[0] != nullptr)
                {
                    right_flash_prop.beacon_flares_billboard_system[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    right_flash_prop.beacon_flares_billboard_system[0]->setMaterialName("tracks/flare");
                    right_flash_prop.beacon_flare_billboard_scene_node[0]->attachObject(right_flash_prop.beacon_flares_billboard_system[0]);
                }
                right_flash_prop.beacon_flare_billboard_scene_node[0]->setVisible(false);
                right_flash_prop.beacon_flares_billboard_system[0]->setDefaultDimensions(1.0, 1.0);
                right_flash_prop.animFlags[0]=0;
                right_flash_prop.animMode[0]=0;
                
                m_generate_wing_position_lights = false; // Already done
            }

            m_first_wing_index = m_rig->ar_num_wings;
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
    m_rig->ar_wings[m_rig->ar_num_wings].fa = flex_airfoil;
    m_rig->ar_wings[m_rig->ar_num_wings].cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    m_rig->ar_wings[m_rig->ar_num_wings].cnode->attachObject(entity);

    ++m_rig->ar_num_wings;
}

float RigSpawner::ComputeWingArea(Ogre::Vector3 const & ref, Ogre::Vector3 const & x, Ogre::Vector3 const & y, Ogre::Vector3 const & aref)
{
    SPAWNER_PROFILE_SCOPED();

    return (((x-ref).crossProduct(y-ref)).length()+((x-aref).crossProduct(y-aref)).length())*0.5f;
}

void RigSpawner::ProcessSoundSource2(RigDef::SoundSource2 & def)
{
#ifdef USE_OPENAL
    SPAWNER_PROFILE_SCOPED();

    int mode = (def.mode == RigDef::SoundSource2::MODE_CINECAM) ? def.cinecam_index : def.mode;
    int node_index = FindNodeIndex(def.node);
    if (node_index == -1)
    {
        return;
    }
    AddSoundSource(
            m_rig,
            SoundScriptManager::getSingleton().createInstance(def.sound_script_name, m_rig->ar_instance_id), 
            node_index,
            mode
        );
#endif // USE_OPENAL
}

void RigSpawner::AddSoundSourceInstance(Beam *vehicle, Ogre::String const & sound_script_name, int node_index, int type)
{
    SPAWNER_PROFILE_SCOPED();
#ifdef USE_OPENAL
    AddSoundSource(vehicle, SoundScriptManager::getSingleton().createInstance(sound_script_name, vehicle->ar_instance_id, nullptr), node_index);
#endif // USE_OPENAL
}

void RigSpawner::AddSoundSource(Beam *vehicle, SoundScriptInstance *sound_script, int node_index, int type)
{
    SPAWNER_PROFILE_SCOPED();

    if (! CheckSoundScriptLimit(vehicle, 1))
    {
        return;
    }

    if (sound_script == nullptr)
    {
        return;
    }

    vehicle->soundsources[vehicle->free_soundsource].ssi=sound_script;
    vehicle->soundsources[vehicle->free_soundsource].nodenum=node_index;
    vehicle->soundsources[vehicle->free_soundsource].type=type;
    vehicle->free_soundsource++;
}

void RigSpawner::ProcessSoundSource(RigDef::SoundSource & def)
{
#ifdef USE_OPENAL
    SPAWNER_PROFILE_SCOPED();

    AddSoundSource(
            m_rig,
            SoundScriptManager::getSingleton().createInstance(def.sound_script_name, m_rig->ar_instance_id), 
            GetNodeIndexOrThrow(def.node),
            -2
        );
#endif // USE_OPENAL
}

void RigSpawner::ProcessCameraRail(RigDef::CameraRail & def)
{
    SPAWNER_PROFILE_SCOPED();

    auto itor = def.nodes.begin();
    auto end  = def.nodes.end();
    for(; itor != end; ++itor)
    {
        if (! CheckCameraRailLimit(1))
        {
            return;
        }
        m_rig->ar_camera_rail[m_rig->free_camerarail] = GetNodeIndexOrThrow(*itor);
        m_rig->free_camerarail++;
    }
}

void RigSpawner::ProcessExtCamera(RigDef::ExtCamera & def)
{
    SPAWNER_PROFILE_SCOPED();

    m_rig->ar_extern_camera_mode = def.mode;
    if (def.node.IsValidAnyState())
    {
        m_rig->ar_extern_camera_node = GetNodeIndexOrThrow(def.node);
    }
}

void RigSpawner::ProcessGuiSettings(RigDef::GuiSettings & def)
{
    SPAWNER_PROFILE_SCOPED();

    // Currently unused (was relevant to overlay-based HUD): def.tacho_material; def.speedo_material;

    if (! def.help_material.empty())
    {
        m_rig->ar_help_panel_material = def.help_material;
    }
    if (def.speedo_highest_kph > 10 && def.speedo_highest_kph < 32000)
    {
        m_rig->ar_speedo_max_kph = def.speedo_highest_kph; /* Handles default */
    }
    else
    {
        std::stringstream msg;
        msg << "Invalid 'speedo_highest_kph' value (" << def.speedo_highest_kph << "), allowed range is <10 -32000>. Falling back to default...";
        AddMessage(Message::TYPE_ERROR, msg.str());
        m_rig->ar_speedo_max_kph = RigDef::GuiSettings::DEFAULT_SPEEDO_MAX;
    }
    m_rig->ar_gui_use_engine_max_rpm = def.use_max_rpm;  /* Handles default */

    std::list<Ogre::String>::iterator dash_itor = def.dashboard_layouts.begin();
    for ( ; dash_itor != def.dashboard_layouts.end(); dash_itor++)
    {
        m_rig->m_dashboard_layouts.push_back(std::pair<Ogre::String, bool>(*dash_itor, false));
    }

    std::list<Ogre::String>::iterator rtt_itor = def.rtt_dashboard_layouts.begin();
    for ( ; rtt_itor != def.rtt_dashboard_layouts.end(); rtt_itor++)
    {
        m_rig->m_dashboard_layouts.push_back(std::pair<Ogre::String, bool>(*rtt_itor, true));
    }

}

void RigSpawner::ProcessFixedNode(RigDef::Node::Ref node_ref)
{
    node_t & node = GetNodeOrThrow(node_ref);
    node.locked = 1;
}

void RigSpawner::ProcessExhaust(RigDef::Exhaust & def)
{
    SPAWNER_PROFILE_SCOPED();

    // parse exhausts
    if (m_rig->disable_smoke)
    {
        return;
    }
    
    node_t & ref_node = GetNodeOrThrow(def.reference_node);//id1;
    node_t & dir_node = GetNodeOrThrow(def.direction_node);//id2;

    exhaust_t exhaust;
    exhaust.emitterNode = ref_node.pos;
    exhaust.directionNode = dir_node.pos;
    exhaust.isOldFormat = false;
    exhaust.factor = 1.f; // Unused, according to wiki documentation.
    std::memset(exhaust.material, 0, sizeof(exhaust.material));

    Ogre::String material_name;
    if (def.material_name.empty() || def.material_name == "default")
    {
        material_name = "tracks/Smoke";
    }
    else
    {
        material_name = def.material_name;
    }

    if (m_rig->m_used_skin != nullptr)
    {
        auto search = m_rig->m_used_skin->replace_materials.find(material_name);
        if (search != m_rig->m_used_skin->replace_materials.end())
        {
            material_name = search->second;
        }
    }

    exhaust.smoker = gEnv->sceneManager->createParticleSystem(
        this->ComposeName("Exhaust", static_cast<int>(m_rig->exhausts.size())), material_name);
    if (!exhaust.smoker)
    {
        AddMessage(Message::TYPE_ERROR, "Failed to create exhaust particle system");
        return;
    }
    exhaust.smoker->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
    exhaust.smokeNode = m_parent_scene_node->createChildSceneNode();
    exhaust.smokeNode->attachObject(exhaust.smoker);
    exhaust.smokeNode->setPosition(m_rig->ar_nodes[exhaust.emitterNode].AbsPosition);

    ref_node.isHot=true;
    dir_node.isHot=true;
    m_rig->exhausts.push_back(exhaust);
}

std::string RigSpawner::GetSubmeshGroundmodelName()
{
    auto module_itor = m_selected_modules.begin();
    auto module_end  = m_selected_modules.end();
    for (; module_itor != module_end; ++module_itor)
    {
        if (! module_itor->get()->submeshes_ground_model_name.empty())
        {
            return module_itor->get()->submeshes_ground_model_name;
        }
    }
    return std::string();
};

void RigSpawner::ProcessSubmesh(RigDef::Submesh & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (! CheckSubmeshLimit(1))
    {
        return;
    }

    /* TEXCOORDS */

    std::vector<RigDef::Texcoord>::iterator texcoord_itor = def.texcoords.begin();
    for ( ; texcoord_itor != def.texcoords.end(); texcoord_itor++)
    {
        if (! CheckTexcoordLimit(1))
        {
            break;
        }

        CabTexcoord texcoord;
        texcoord.node_id    = GetNodeIndexOrThrow(texcoord_itor->node);
        texcoord.texcoord_u = texcoord_itor->u;
        texcoord.texcoord_v = texcoord_itor->v;
        m_oldstyle_cab_texcoords.push_back(texcoord);
    }

    /* CAB */

    auto cab_itor = def.cab_triangles.begin();
    auto cab_itor_end = def.cab_triangles.end();
    for ( ; cab_itor != cab_itor_end; ++cab_itor)
    {
        if (! CheckCabLimit(1))
        {
            return;
        }
        else if (m_rig->free_collcab >= MAX_CABS)
        {
            std::stringstream msg;
            msg << "Collcab limit (" << MAX_CABS << ") exceeded";
            AddMessage(Message::TYPE_ERROR, msg.str());
            return;
        }

        bool mk_buoyance = false;

        m_rig->cabs[m_rig->free_cab*3]=GetNodeIndexOrThrow(cab_itor->nodes[0]);
        m_rig->cabs[m_rig->free_cab*3+1]=GetNodeIndexOrThrow(cab_itor->nodes[1]);
        m_rig->cabs[m_rig->free_cab*3+2]=GetNodeIndexOrThrow(cab_itor->nodes[2]);

        if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_c_CONTACT))
        {
            m_rig->collcabs[m_rig->free_collcab]=m_rig->free_cab; 
            m_rig->free_collcab++;
        }
        if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_p_10xTOUGHER))
        {
            m_rig->collcabs[m_rig->free_collcab]=m_rig->free_cab; 
            m_rig->free_collcab++;
        }
        if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_u_INVULNERABLE))
        {
            m_rig->collcabs[m_rig->free_collcab]=m_rig->free_cab; 
            m_rig->free_collcab++;
        }
        if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_b_BUOYANT))
        {
            m_rig->buoycabs[m_rig->free_buoycab]=m_rig->free_cab; 
            m_rig->buoycabtypes[m_rig->free_buoycab]=Buoyance::BUOY_NORMAL; 
            m_rig->free_buoycab++;   
            mk_buoyance = true;
        }
        if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_r_BUOYANT_ONLY_DRAG))
        {
            m_rig->buoycabs[m_rig->free_buoycab]=m_rig->free_cab; 
            m_rig->buoycabtypes[m_rig->free_buoycab]=Buoyance::BUOY_DRAGONLY; 
            m_rig->free_buoycab++; 
            mk_buoyance = true;
        }
        if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_s_BUOYANT_NO_DRAG))
        {
            m_rig->buoycabs[m_rig->free_buoycab]=m_rig->free_cab; 
            m_rig->buoycabtypes[m_rig->free_buoycab]=Buoyance::BUOY_DRAGLESS; 
            m_rig->free_buoycab++; 
            mk_buoyance = true;
        }

        int collcabs_type = -1;
        if (cab_itor->GetOption_D_ContactBuoyant())
        {
            collcabs_type = 0;
        }
        if (cab_itor->GetOption_F_10xTougherBuoyant())
        {
            collcabs_type = 1;
        }
        if (cab_itor->GetOption_S_UnpenetrableBuoyant())
        {
            collcabs_type = 2;
        }

        //if (type=='D' || type == 'F' || type == 'S')
        if (collcabs_type != -1)
        {

            if (m_rig->free_collcab >= MAX_CABS)
            {
                std::stringstream msg;
                msg << "Collcab limit (" << MAX_CABS << ") exceeded";
                AddMessage(Message::TYPE_ERROR, msg.str());
                return;
            }
            else if (m_rig->free_buoycab >= MAX_CABS)
            {
                std::stringstream msg;
                msg << "Buoycab limit (" << MAX_CABS << ") exceeded";
                AddMessage(Message::TYPE_ERROR, msg.str());
                return;
            }

            m_rig->collcabs[m_rig->free_collcab]=m_rig->free_cab;
            m_rig->free_collcab++;
            m_rig->buoycabs[m_rig->free_buoycab]=m_rig->free_cab; 
            m_rig->buoycabtypes[m_rig->free_buoycab]=Buoyance::BUOY_NORMAL; 
            m_rig->free_buoycab++; 
            mk_buoyance = true;
        }

        if (mk_buoyance && (m_rig->m_buoyance == nullptr))
        {
            auto& dustman = App::GetSimController()->GetBeamFactory()->GetParticleManager();
            m_rig->m_buoyance=std::make_unique<Buoyance>(dustman.getDustPool("splash"), dustman.getDustPool("ripple"));
        }
        m_rig->free_cab++;
    }

    //close the current mesh
    CabSubmesh submesh;
    submesh.texcoords_pos = m_oldstyle_cab_texcoords.size();
    submesh.cabs_pos = static_cast<unsigned int>(m_rig->free_cab);
    submesh.backmesh_type = CabSubmesh::BACKMESH_NONE;
    m_oldstyle_cab_submeshes.push_back(submesh);

    /* BACKMESH */

    if (def.backmesh)
    {

        // Check limit
        if (! CheckCabLimit(1))
        {
            return;
        }

        // === add an extra front mesh ===
        //texcoords
        int uv_start = (m_oldstyle_cab_submeshes.size()==1) ? 0 : (m_oldstyle_cab_submeshes.rbegin()+1)->texcoords_pos;
        for (size_t i=uv_start; i<m_oldstyle_cab_submeshes.back().texcoords_pos; i++)
        {
            m_oldstyle_cab_texcoords.push_back(m_oldstyle_cab_texcoords[i]);
        }
        //cab
        int cab_start =  (m_oldstyle_cab_submeshes.size()==1) ? 0 : (m_oldstyle_cab_submeshes.rbegin()+1)->cabs_pos;
        for (size_t i=cab_start; i<m_oldstyle_cab_submeshes.back().cabs_pos; i++)
        {
            m_rig->cabs[m_rig->free_cab*3]=m_rig->cabs[i*3];
            m_rig->cabs[m_rig->free_cab*3+1]=m_rig->cabs[i*3+1];
            m_rig->cabs[m_rig->free_cab*3+2]=m_rig->cabs[i*3+2];
            m_rig->free_cab++;
        }
        // Finalize
        CabSubmesh submesh;
        submesh.backmesh_type = CabSubmesh::BACKMESH_TRANSPARENT;
        submesh.texcoords_pos = m_oldstyle_cab_texcoords.size();
        submesh.cabs_pos      = static_cast<unsigned int>(m_rig->free_cab);
        m_oldstyle_cab_submeshes.push_back(submesh);

        // === add an extra back mesh ===
        //texcoords
        uv_start = (m_oldstyle_cab_submeshes.size()==1) ? 0 : (m_oldstyle_cab_submeshes.rbegin()+1)->texcoords_pos;
        for (size_t i=uv_start; i<m_oldstyle_cab_submeshes.back().texcoords_pos; i++)
        {
            m_oldstyle_cab_texcoords.push_back(m_oldstyle_cab_texcoords[i]);
        }

        //cab
        cab_start =  (m_oldstyle_cab_submeshes.size()==1) ? 0 : (m_oldstyle_cab_submeshes.rbegin()+1)->cabs_pos;
        for (size_t i=cab_start; i<m_oldstyle_cab_submeshes.back().cabs_pos; i++)
        {
            m_rig->cabs[m_rig->free_cab*3]=m_rig->cabs[i*3+1];
            m_rig->cabs[m_rig->free_cab*3+1]=m_rig->cabs[i*3];
            m_rig->cabs[m_rig->free_cab*3+2]=m_rig->cabs[i*3+2];
            m_rig->free_cab++;
        }
    
        //close the current mesh
        CabSubmesh submesh2;
        submesh2.texcoords_pos = m_oldstyle_cab_texcoords.size();
        submesh2.cabs_pos = static_cast<unsigned int>(m_rig->free_cab);
        submesh2.backmesh_type = CabSubmesh::BACKMESH_OPAQUE;
        m_oldstyle_cab_submeshes.push_back(submesh2);
    }
}

void RigSpawner::ProcessFlexbody(std::shared_ptr<RigDef::Flexbody> def)
{
    SPAWNER_PROFILE_SCOPED();

    if (!this->CheckFlexbodyLimit(1))
    {
        return;
    }

    // Collect nodes
    std::vector<unsigned int> node_indices;
    bool nodes_found = true;
    for (auto& node_def: def->node_list)
    {
        auto result = this->GetNodeIndex(node_def);
        if (!result.second)
        {
            nodes_found = false;
            break;
        }
        node_indices.push_back(result.first);
    }

    if (! nodes_found)
    {
        this->AddMessage(Message::TYPE_ERROR, "Failed to collect nodes from node-ranges, skipping flexbody: " + def->mesh_name);
        return;
    }

    const int reference_node = this->FindNodeIndex(def->reference_node);
    const int x_axis_node    = this->FindNodeIndex(def->x_axis_node);
    const int y_axis_node    = this->FindNodeIndex(def->y_axis_node);
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
            def.get(), reference_node, x_axis_node, y_axis_node, rot, node_indices);

        if (flexbody == nullptr)
            return; // Error already logged

        if (def->camera_settings.mode == RigDef::CameraSettings::MODE_CINECAM)
            flexbody->setCameraMode(static_cast<int>(def->camera_settings.cinecam_index));
        else
            flexbody->setCameraMode(static_cast<int>(def->camera_settings.mode));

        m_rig->flexbodies[m_rig->free_flexbody] = flexbody;
        m_rig->free_flexbody++;        
    }
    catch (Ogre::Exception& e)
    {
        this->AddMessage(Message::TYPE_ERROR, 
            "Failed to create flexbody '" + def->mesh_name + "', reason:" + e.getFullDescription());
    }
}

void RigSpawner::ProcessProp(RigDef::Prop & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (! CheckPropLimit(1))
    {
        return;
    }

    prop_t & prop = m_rig->ar_props[m_rig->ar_num_props];
    int prop_index = m_rig->ar_num_props;
    memset(&prop, 0, sizeof(prop_t));

    prop.noderef         = GetNodeIndexOrThrow(def.reference_node);
    prop.nodex           = FindNodeIndex(def.x_axis_node);
    prop.nodey           = FindNodeIndex(def.y_axis_node);
    if (prop.nodex == -1 || prop.nodey == -1)
    {
        return;
    }
    prop.offsetx         = def.offset.x;
    prop.offsety         = def.offset.y;
    prop.offsetz         = def.offset.z;
    prop.orgoffsetX      = def.offset.x;
    prop.orgoffsetY      = def.offset.y;
    prop.orgoffsetZ      = def.offset.z;
    prop.rotaX           = def.rotation.x;
    prop.rotaY           = def.rotation.y;
    prop.rotaZ           = def.rotation.z;
    prop.rot             = Ogre::Quaternion(Ogre::Degree(def.rotation.z), Ogre::Vector3::UNIT_Z);
    prop.rot             = prop.rot * Ogre::Quaternion(Ogre::Degree(def.rotation.y), Ogre::Vector3::UNIT_Y);
    prop.rot             = prop.rot * Ogre::Quaternion(Ogre::Degree(def.rotation.x), Ogre::Vector3::UNIT_X);
    prop.cameramode      = def.camera_settings.mode; /* Handles default value */
    prop.wheelrotdegree  = 160.f;
    /* Set no animation by default */
    prop.animKey[0]      = -1;
    prop.animKeyState[0] = -1.f;

    /* SPECIAL PROPS */

    /* Rear view mirror (left) */
    if (def.special == RigDef::Prop::SPECIAL_MIRROR_LEFT)
    {
        prop.mirror = 1;
        m_curr_mirror_prop_type = CustomMaterial::MirrorPropType::MPROP_LEFT;
    }

    /* Rear view mirror (right) */
    if (def.special == RigDef::Prop::SPECIAL_MIRROR_RIGHT)
    {
        prop.mirror = -1;
        m_curr_mirror_prop_type = CustomMaterial::MirrorPropType::MPROP_RIGHT;
    }

    /* Custom steering wheel */
    Ogre::Vector3 steering_wheel_offset = Ogre::Vector3::ZERO;
    if (def.special == RigDef::Prop::SPECIAL_DASHBOARD_LEFT)
    {
        steering_wheel_offset = Ogre::Vector3(-0.67, -0.61,0.24);
    }
    if (def.special == RigDef::Prop::SPECIAL_DASHBOARD_RIGHT)
    {
        steering_wheel_offset = Ogre::Vector3(0.67, -0.61,0.24);
    }
    if (steering_wheel_offset != Ogre::Vector3::ZERO)
    {
        if (def.special_prop_dashboard._offset_is_set)
        {
            steering_wheel_offset = def.special_prop_dashboard.offset;
        }
        prop.wheelrotdegree = def.special_prop_dashboard.rotation_angle;
        prop.wheel = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        prop.wheelpos = steering_wheel_offset;
        const std::string instance_name = this->ComposeName("SteeringWheelPropEntity", prop_index);
        prop.wheelmo = new MeshObject(
            def.special_prop_dashboard.mesh_name,
            instance_name,
            prop.wheel,
            m_enable_background_loading
            );
        this->SetupNewEntity(prop.wheelmo->getEntity(), Ogre::ColourValue(0, 0.5, 0.5));
    }

    /* CREATE THE PROP */

    prop.scene_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    const std::string instance_name = this->ComposeName("PropEntity", prop_index);
    prop.mo = new MeshObject(def.mesh_name, instance_name, prop.scene_node, m_enable_background_loading);
    prop.mo->setCastShadows(true); // Orig code {{ prop.mo->setCastShadows(shadowmode != 0); }}, shadowmode has default value 1 and changes with undocumented directive 'set_shadows'
    prop.beacontype = 'n'; // Orig: hardcoded in BTS_PROPS

    if (def.special == RigDef::Prop::SPECIAL_SPINPROP)
    {
        prop.spinner = 1;
        prop.mo->setCastShadows(false);
        prop.scene_node->setVisible(false);
    }
    else if(def.special == RigDef::Prop::SPECIAL_PALE)
    {
        prop.pale = 1;
    }
    else if(def.special == RigDef::Prop::SPECIAL_DRIVER_SEAT)
    {
        //driver seat, used to position the driver and make the seat translucent at times
        if (m_rig->ar_driverseat_prop == nullptr)
        {
            m_rig->ar_driverseat_prop = & prop;
            prop.mo->setMaterialName("driversseat");
        }
        else
        {
            this->AddMessage(Message::TYPE_INFO, "Found more than one 'seat[2]' special props. Only the first one will be the driver's seat.");
        }
    }
    else if(def.special == RigDef::Prop::SPECIAL_DRIVER_SEAT_2)
    {
        // Same as DRIVER_SEAT, except it doesn't force the "driversseat" material
        if (m_rig->ar_driverseat_prop == nullptr)
        {
            m_rig->ar_driverseat_prop = & prop;
        }
        else
        {
            this->AddMessage(Message::TYPE_INFO, "Found more than one 'seat[2]' special props. Only the first one will be the driver's seat.");
        }
    }
    else if (m_rig->m_flares_mode != GfxFlaresMode::NONE)
    {
        if(def.special == RigDef::Prop::SPECIAL_BEACON)
        {
            prop.beacontype = 'b';
            prop.beacon_light_rotation_angle[0] = 2.0 * 3.14 * (std::rand() / RAND_MAX);
            prop.beacon_light_rotation_rate[0] = 4.0 * 3.14 + (std::rand() / RAND_MAX) - 0.5;
            /* the light */
            auto beacon_light = gEnv->sceneManager->createLight();
            beacon_light->setType(Ogre::Light::LT_SPOTLIGHT);
            beacon_light->setDiffuseColour(def.special_prop_beacon.color);
            beacon_light->setSpecularColour(def.special_prop_beacon.color);
            beacon_light->setAttenuation(50.0, 1.0, 0.3, 0.0);
            beacon_light->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
            beacon_light->setCastShadows(false);
            beacon_light->setVisible(false);
            /* the flare billboard */

            auto flare_scene_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
            auto flare_billboard_sys = gEnv->sceneManager->createBillboardSet(1); //(propname,1);
            flare_scene_node->attachObject(flare_billboard_sys);
            flare_billboard_sys->createBillboard(0,0,0);
            if (flare_billboard_sys)
            {
                flare_billboard_sys->setMaterialName(def.special_prop_beacon.flare_material_name);
                flare_billboard_sys->setVisibilityFlags(DEPTHMAP_DISABLED);
            }
            flare_scene_node->setVisible(false);
            flare_billboard_sys->setVisible(false);

            // Complete
            prop.beacon_flare_billboard_scene_node[0] = flare_scene_node;
            prop.beacon_flares_billboard_system[0] = flare_billboard_sys;
            prop.beacon_light[0] = beacon_light;
        }
        else if(def.special == RigDef::Prop::SPECIAL_REDBEACON)
        {
            prop.beacon_light_rotation_angle[0] = 0.f;
            prop.beacon_light_rotation_rate[0] = 1.0;
            prop.beacontype = 'r';
            //the light
            auto beacon_light=gEnv->sceneManager->createLight();//propname);
            beacon_light->setType(Ogre::Light::LT_POINT);
            beacon_light->setDiffuseColour( Ogre::ColourValue(1.0, 0.0, 0.0));
            beacon_light->setSpecularColour( Ogre::ColourValue(1.0, 0.0, 0.0));
            beacon_light->setAttenuation(50.0, 1.0, 0.3, 0.0);
            beacon_light->setCastShadows(false);
            beacon_light->setVisible(false);
            //the flare billboard
            auto flare_scene_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
            auto flare_billboard_sys =gEnv->sceneManager->createBillboardSet(1); //propname,1);
            flare_billboard_sys->createBillboard(0,0,0);
            flare_billboard_sys->setMaterialName("tracks/redbeaconflare");
            flare_billboard_sys->setVisibilityFlags(DEPTHMAP_DISABLED);
            flare_scene_node->attachObject(flare_billboard_sys);
            flare_scene_node->setVisible(false);
            flare_billboard_sys->setDefaultDimensions(1.0, 1.0);

            // Finalize
            prop.beacon_light[0] = beacon_light;
            prop.beacon_flare_billboard_scene_node[0] = flare_scene_node;
            prop.beacon_flares_billboard_system[0] = flare_billboard_sys;
            
        }
        else if(def.special == RigDef::Prop::SPECIAL_LIGHTBAR)
        {
            m_rig->ar_is_police = true;
            prop.beacontype='p';
            for (int k=0; k<4; k++)
            {
                // Randomize rotation speed and timing, 
                // IMPORTANT: Do not remove the (Ogre::Real) casts, they affect result!
                prop.beacon_light_rotation_angle[k]=2.0*3.14*((Ogre::Real)std::rand()/(Ogre::Real)RAND_MAX);
                prop.beacon_light_rotation_rate[k]=4.0*3.14+((Ogre::Real)std::rand()/(Ogre::Real)RAND_MAX)-0.5;
                prop.beacon_flares_billboard_system[k]=nullptr;
                //the light
                prop.beacon_light[k]=gEnv->sceneManager->createLight();
                prop.beacon_light[k]->setType(Ogre::Light::LT_SPOTLIGHT);
                if (k>1)
                {
                    prop.beacon_light[k]->setDiffuseColour( Ogre::ColourValue(1.0, 0.0, 0.0));
                    prop.beacon_light[k]->setSpecularColour( Ogre::ColourValue(1.0, 0.0, 0.0));
                }
                else
                {
                    prop.beacon_light[k]->setDiffuseColour( Ogre::ColourValue(0.0, 0.5, 1.0));
                    prop.beacon_light[k]->setSpecularColour( Ogre::ColourValue(0.0, 0.5, 1.0));
                }
                prop.beacon_light[k]->setAttenuation(50.0, 1.0, 0.3, 0.0);
                prop.beacon_light[k]->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
                prop.beacon_light[k]->setCastShadows(false);
                prop.beacon_light[k]->setVisible(false);
                //the flare billboard
                prop.beacon_flare_billboard_scene_node[k] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
                prop.beacon_flares_billboard_system[k]=gEnv->sceneManager->createBillboardSet(1);
                prop.beacon_flares_billboard_system[k]->createBillboard(0,0,0);
                if (prop.beacon_flares_billboard_system[k])
                {
                    if (k>1)
                    {
                        prop.beacon_flares_billboard_system[k]->setMaterialName("tracks/brightredflare");
                    }
                    else
                    {
                        prop.beacon_flares_billboard_system[k]->setMaterialName("tracks/brightblueflare");
                    }

                    prop.beacon_flares_billboard_system[k]->setVisibilityFlags(DEPTHMAP_DISABLED);
                    prop.beacon_flare_billboard_scene_node[k]->attachObject(prop.beacon_flares_billboard_system[k]);
                }
                prop.beacon_flare_billboard_scene_node[k]->setVisible(false);
            }
        }

        if (m_curr_mirror_prop_type != CustomMaterial::MirrorPropType::MPROP_NONE)
        {
            m_curr_mirror_prop_scenenode = prop.mo->GetSceneNode();
        }
        this->SetupNewEntity(prop.mo->getEntity(), Ogre::ColourValue(1.f, 1.f, 0.f));
    }

    ++m_rig->ar_num_props;
    m_curr_mirror_prop_scenenode = nullptr;
    m_curr_mirror_prop_type = CustomMaterial::MirrorPropType::MPROP_NONE;

    /* PROCESS ANIMATIONS */

    if (def.animations.size() > 10)
    {
        std::stringstream msg;
        msg << "Prop (mesh: " << def.mesh_name << ") has too many animations: " << def.animations.size() << " (max. is 10). Using first 10 ...";
        AddMessage(Message::TYPE_WARNING, msg.str());
    }

    std::list<RigDef::Animation>::iterator anim_itor = def.animations.begin();
    int anim_index = 0;
    while (anim_itor != def.animations.end() && anim_index < 10)
    {
        prop.animKeyState[anim_index] = -1.0f; // Orig: hardcoded in {add_animation}

        /* Arg #1: ratio */
        prop.animratio[anim_index] = anim_itor->ratio;
        if (anim_itor->ratio == 0) 
        {
            std::stringstream msg;
            msg << "Prop (mesh: " << def.mesh_name << ") has invalid animation ratio (0), using it anyway (compatibility)...";
            AddMessage(Message::TYPE_WARNING, msg.str());
        }

        /* Arg #2: option1 (lower limit) */
        prop.constraints[anim_index].lower_limit = anim_itor->lower_limit; /* Handles default */

        /* Arg #3: option2 (upper limit) */
        prop.constraints[anim_index].upper_limit = anim_itor->upper_limit; /* Handles default */

        /* Arg #4: source */
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_AIRSPEED)) { /* (NOTE: code formatting relaxed) */
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AIRSPEED);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_VERTICAL_VELOCITY)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_VVI);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ALTIMETER_100K)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ALTIMETER);
            prop.animOpt3[anim_index] = 1.f;
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ALTIMETER_10K)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ALTIMETER);
            prop.animOpt3[anim_index] = 2.f;
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ALTIMETER_1K)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ALTIMETER);
            prop.animOpt3[anim_index] = 3.f;
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ANGLE_OF_ATTACK)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AOA);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_FLAP)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_FLAP);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_AIR_BRAKE)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AIRBRAKE);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ROLL)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ROLL);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_PITCH)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_PITCH);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_BRAKES)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_BRAKE);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ACCEL)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ACCEL);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_CLUTCH)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_CLUTCH);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SPEEDO)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SPEEDO);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_TACHO)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_TACHO);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_TURBO)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_TURBO);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_PARKING)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_PBRAKE);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SHIFT_LEFT_RIGHT)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SHIFTER);
            prop.animOpt3[anim_index] = 1.0f;
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SHIFT_BACK_FORTH)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SHIFTER);
            prop.animOpt3[anim_index] = 2.0f;
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SEQUENTIAL_SHIFT)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SHIFTER);
            prop.animOpt3[anim_index] = 3.0f;
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SHIFTERLIN)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SHIFTER);
            prop.animOpt3[anim_index] = 4.0f;
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_TORQUE)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_TORQUE);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_HEADING)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_HEADING);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_DIFFLOCK)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_DIFFLOCK);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_STEERING_WHEEL)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_STEERING);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_AILERON)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AILERONS);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ELEVATOR)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ELEVATORS);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_AIR_RUDDER)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ARUDDER);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_BOAT_RUDDER)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_BRUDDER);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_BOAT_THROTTLE)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_BTHROTTLE);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_PERMANENT)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_PERMANENT);
        }
        if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_EVENT)) {
            BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_EVENT);
        }
        /* Motor-indexed sources */
        std::list<RigDef::Animation::MotorSource>::iterator source_itor = anim_itor->motor_sources.begin();
        for ( ; source_itor != anim_itor->motor_sources.end(); source_itor++)
        {
            if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_THROTTLE)) {
                BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_THROTTLE);
                prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
            }
            if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_RPM)) {
                BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_RPM);
                prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
            }
            if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_TORQUE)) {
                BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AETORQUE);
                prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
            }
            if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_PITCH)) {
                BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AEPITCH);
                prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
            }
            if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_STATUS)) {
                BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AESTATUS);
                prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
            }
        }
        if (prop.animFlags[anim_index] == 0)
        {
            AddMessage(Message::TYPE_ERROR, "Failed to identify animation source");
        }

        /* Anim modes */
        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_X)) {
            BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_ROTA_X);
        }
        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_Y)) {
            BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_ROTA_Y);
        }
        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_Z)) {
            BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_ROTA_Z);
        }
        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_X)) {
            BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_OFFSET_X);
        }
        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_Y)) {
            BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_OFFSET_Y);
        }
        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_Z)) {
            BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_OFFSET_Z);
        }
        if (prop.animMode[anim_index] == 0)
        {
            AddMessage(Message::TYPE_ERROR, "Failed to identify animation mode");
        }

        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_AUTO_ANIMATE)) 
        {
            BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_AUTOANIMATE);

            // Flag whether default lower and/or upper animation limit constraints are effective
            const bool use_default_lower_limit = (anim_itor->lower_limit == 0.f);
            const bool use_default_upper_limit = (anim_itor->upper_limit == 0.f);

            if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_X)) {
                prop.constraints[anim_index].lower_limit = (use_default_lower_limit) ? (-180.f) : (anim_itor->lower_limit + prop.rotaX);
                prop.constraints[anim_index].upper_limit = (use_default_upper_limit) ? ( 180.f) : (anim_itor->upper_limit + prop.rotaX);
            }
            if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_Y)) {
                prop.constraints[anim_index].lower_limit = (use_default_lower_limit) ? (-180.f) : (anim_itor->lower_limit + prop.rotaY);
                prop.constraints[anim_index].upper_limit = (use_default_upper_limit) ? ( 180.f) : (anim_itor->upper_limit + prop.rotaY);
            }
            if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_Z)) {
                prop.constraints[anim_index].lower_limit = (use_default_lower_limit) ? (-180.f) : (anim_itor->lower_limit + prop.rotaZ);
                prop.constraints[anim_index].upper_limit = (use_default_upper_limit) ? ( 180.f) : (anim_itor->upper_limit + prop.rotaZ);
            }
            if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_X)) {
                prop.constraints[anim_index].lower_limit = (use_default_lower_limit) ? (-10.f) : (anim_itor->lower_limit + prop.orgoffsetX);
                prop.constraints[anim_index].upper_limit = (use_default_upper_limit) ? ( 10.f) : (anim_itor->upper_limit + prop.orgoffsetX);
            }
            if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_Y)) {
                prop.constraints[anim_index].lower_limit = (use_default_lower_limit) ? (-10.f) : (anim_itor->lower_limit + prop.orgoffsetY);
                prop.constraints[anim_index].upper_limit = (use_default_upper_limit) ? ( 10.f) : (anim_itor->upper_limit + prop.orgoffsetY);
            }
            if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_Z)) {
                prop.constraints[anim_index].lower_limit = (use_default_lower_limit) ? (-10.f) : (anim_itor->lower_limit + prop.orgoffsetZ);
                prop.constraints[anim_index].upper_limit = (use_default_upper_limit) ? ( 10.f) : (anim_itor->upper_limit + prop.orgoffsetZ);
            }
        }
        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_NO_FLIP)) 
        {
            BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_NOFLIP);
        }
        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_BOUNCE)) 
        {
            BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_BOUNCE);
            prop.animOpt5[anim_index] = 1.f;
        }
        if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_EVENT_LOCK)) 
        {
            prop.animKeyState[anim_index] = 0.0f;
            prop.lastanimKS[anim_index] = 0.0f;
        }
        
        /* Parameter 'event:' */

        if (! anim_itor->event.empty())
        {
            // we are using keys as source
            prop.animFlags[anim_index] |= ANIM_FLAG_EVENT;

            int event_id = RoR::App::GetInputEngine()->resolveEventName(anim_itor->event);
            if (event_id == -1)
            {
                AddMessage(Message::TYPE_ERROR, "Unknown animation event: " + anim_itor->event);
            }
            else
            {
                prop.animKey[anim_index] = event_id;
            }
        }

        /* Advance */
        anim_itor++;
        anim_index++;
    }
}

void RigSpawner::ProcessFlare2(RigDef::Flare2 & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (m_rig->m_flares_mode == GfxFlaresMode::NONE) { return; }

    int blink_delay = def.blink_delay_milis;
    float size = def.size;

    /* Backwards compatibility */
    if (blink_delay == -2) 
    {
        if (def.type == RigDef::Flare2::TYPE_l_LEFT_BLINKER || def.type == RigDef::Flare2::TYPE_r_RIGHT_BLINKER)
        {
            blink_delay = -1; /* Default blink */
        }
        else
        {
            blink_delay = 0; /* Default no blink */
        }
    }
    
    if (size == -2.f && def.type == RigDef::Flare2::TYPE_f_HEADLIGHT)
    {
        size = 1.f;
    }
    else if ((size == -2.f && def.type != RigDef::Flare2::TYPE_f_HEADLIGHT) || size == -1.f)
    {
        size = 0.5f;
    }

    flare_t flare;
    flare.type                 = def.type;
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
    flare.snode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    std::string flare_name = this->ComposeName("Flare", static_cast<int>(m_rig->flares.size()));
    flare.bbs = gEnv->sceneManager->createBillboardSet(flare_name, 1);
    bool using_default_material = true;
    if (flare.bbs == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, "Failed to create flare: '" + flare_name + "', continuing without it (compatibility)...");
    }
    else
    {
        flare.bbs->createBillboard(0,0,0);
        flare.bbs->setVisibilityFlags(DEPTHMAP_DISABLED);
        
        if (def.material_name.length() == 0 || def.material_name == "default")
        {
            if (def.type == RigDef::Flare2::TYPE_b_BRAKELIGHT)
            {
                flare.bbs->setMaterialName("tracks/brakeflare");
            }
            else if (def.type == RigDef::Flare2::TYPE_l_LEFT_BLINKER || (def.type == RigDef::Flare2::TYPE_r_RIGHT_BLINKER))
            {
                flare.bbs->setMaterialName("tracks/blinkflare");
            }
            else
            {
                flare.bbs->setMaterialName("tracks/flare");
            }
        }
        else
        {
            using_default_material = false;
            flare.bbs->setMaterialName(def.material_name);
        }
        flare.snode->attachObject(flare.bbs);
    }
    flare.isVisible = true;
    flare.light = nullptr;

    if ((App::gfx_flares_mode.GetActive() >= GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY) && size > 0.001)
    {
        //if (type == 'f' && usingDefaultMaterial && flaresMode >=2 && size > 0.001)
        if (def.type == RigDef::Flare2::TYPE_f_HEADLIGHT && using_default_material )
        {
            /* front light */
            flare.light=gEnv->sceneManager->createLight(flare_name);
            flare.light->setType(Ogre::Light::LT_SPOTLIGHT);
            flare.light->setDiffuseColour( Ogre::ColourValue(1, 1, 1));
            flare.light->setSpecularColour( Ogre::ColourValue(1, 1, 1));
            flare.light->setAttenuation(400, 0.9, 0, 0);
            flare.light->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
            flare.light->setCastShadows(false);
        }
    }
    if ((App::gfx_flares_mode.GetActive() >= GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS) && size > 0.001)
    {
        if (def.type == RigDef::Flare2::TYPE_f_HEADLIGHT && ! using_default_material)
        {
            /* this is a quick fix for the red backlight when frontlight is switched on */
            flare.light=gEnv->sceneManager->createLight(flare_name);
            flare.light->setDiffuseColour( Ogre::ColourValue(1.0, 0, 0));
            flare.light->setSpecularColour( Ogre::ColourValue(1.0, 0, 0));
            flare.light->setAttenuation(10.0, 1.0, 0, 0);
        }
        else if (def.type == RigDef::Flare2::TYPE_R_REVERSE_LIGHT)
        {
            flare.light=gEnv->sceneManager->createLight(flare_name);
            flare.light->setDiffuseColour(Ogre::ColourValue(1, 1, 1));
            flare.light->setSpecularColour(Ogre::ColourValue(1, 1, 1));
            flare.light->setAttenuation(20.0, 1, 0, 0);
        }
        else if (def.type == RigDef::Flare2::TYPE_b_BRAKELIGHT)
        {
            flare.light=gEnv->sceneManager->createLight(flare_name);
            flare.light->setDiffuseColour( Ogre::ColourValue(1.0, 0, 0));
            flare.light->setSpecularColour( Ogre::ColourValue(1.0, 0, 0));
            flare.light->setAttenuation(10.0, 1.0, 0, 0);
        }
        else if (def.type == RigDef::Flare2::TYPE_l_LEFT_BLINKER || (def.type == RigDef::Flare2::TYPE_r_RIGHT_BLINKER))
        {
            flare.light=gEnv->sceneManager->createLight(flare_name);
            flare.light->setDiffuseColour( Ogre::ColourValue(1, 1, 0));
            flare.light->setSpecularColour( Ogre::ColourValue(1, 1, 0));
            flare.light->setAttenuation(10.0, 1, 1, 0);
        }
        //else if ((type == 'u') && flaresMode >= 4 && size > 0.001)
        else if (def.type == RigDef::Flare2::TYPE_u_USER)
        {
            /* user light always white (TODO: improve this) */
            flare.light=gEnv->sceneManager->createLight(flare_name);
            flare.light->setDiffuseColour( Ogre::ColourValue(1, 1, 1));
            flare.light->setSpecularColour( Ogre::ColourValue(1, 1, 1));
            flare.light->setAttenuation(50.0, 1.0, 1, 0.2);

            if (m_rig->m_net_custom_light_count < 4)
            {
                m_rig->m_net_custom_lights[m_rig->m_net_custom_light_count] = m_rig->flares.size();
                m_rig->m_net_custom_light_count++;
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
    m_rig->flares.push_back(flare);
}

Ogre::MaterialPtr RigSpawner::InstantiateManagedMaterial(Ogre::String const & source_name, Ogre::String const & clone_name)
{
    SPAWNER_PROFILE_SCOPED();

    Ogre::MaterialPtr src_mat = RoR::OgreSubsystem::GetMaterialByName(source_name);
    if (src_mat.isNull())
    {
        std::stringstream msg;
        msg << "Built-in material '" << source_name << "' missing! Skipping...";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return Ogre::MaterialPtr();
    }

    return src_mat->clone(clone_name, true, m_custom_resource_group);
}

void RigSpawner::ProcessManagedMaterial(RigDef::ManagedMaterial & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (m_managed_materials.find(def.name) != m_managed_materials.end())
    {
        this->AddMessage(Message::TYPE_ERROR, "Duplicate managed material name: '" + def.name + "'. Ignoring definition...");
        return;
    }

    // Create global placeholder
    // This is necessary to load meshes with original material names (= unchanged managed mat names)
    // - if not found, OGRE substitutes them with 'BaseWhite' which breaks subsequent processing.
    if (RoR::OgreSubsystem::GetMaterialByName(def.name).isNull())
    {
        m_placeholder_managedmat->clone(def.name);
    }

    std::string custom_name = def.name + ACTOR_ID_TOKEN + TOSTRING(m_rig->ar_instance_id);
    Ogre::MaterialPtr material;
    if (def.type == RigDef::ManagedMaterial::TYPE_FLEXMESH_STANDARD || def.type == RigDef::ManagedMaterial::TYPE_FLEXMESH_TRANSPARENT)
    {
        std::string mat_name_base
            = (def.type == RigDef::ManagedMaterial::TYPE_FLEXMESH_STANDARD)
            ? "managed/flexmesh_standard"
            : "managed/flexmesh_transparent";

        if (def.HasDamagedDiffuseMap())
        {
            if (def.HasSpecularMap())
            {
                /* FLEXMESH, damage, specular */
                material = this->InstantiateManagedMaterial(mat_name_base + "/speculardamage", custom_name);
                if (material.isNull())
                {
                    return;
                }
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Dmg_Diffuse_Map")->setTextureName(def.damaged_diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("SpecularMapping1")->getTextureUnitState("SpecularMapping1_Tex")->setTextureName(def.specular_map);
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
                material = this->InstantiateManagedMaterial(mat_name_base + "/specularonly", custom_name);
                if (material.isNull())
                {
                    return;
                }
                material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
                material->getTechnique("BaseTechnique")->getPass("SpecularMapping1")->getTextureUnitState("SpecularMapping1_Tex")->setTextureName(def.specular_map);
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
    else if (def.type == RigDef::ManagedMaterial::TYPE_MESH_STANDARD || def.type == RigDef::ManagedMaterial::TYPE_MESH_TRANSPARENT)
    {
        Ogre::String mat_name_base
            = (def.type == RigDef::ManagedMaterial::TYPE_MESH_STANDARD)
            ? "managed/mesh_standard"
            : "managed/mesh_transparent";

        if (def.HasSpecularMap())
        {
            /* MESH, specular */
            material = this->InstantiateManagedMaterial(mat_name_base + "/specular", custom_name);
            if (material.isNull())
            {
                return;
            }
            material->getTechnique("BaseTechnique")->getPass("BaseRender")->getTextureUnitState("Diffuse_Map")->setTextureName(def.diffuse_map);
            material->getTechnique("BaseTechnique")->getPass("SpecularMapping1")->getTextureUnitState("SpecularMapping1_Tex")->setTextureName(def.specular_map);
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

    if (def.type != RigDef::ManagedMaterial::TYPE_INVALID)
    {
        if (def.options.double_sided)
        {
            material->getTechnique("BaseTechnique")->getPass("BaseRender")->setCullingMode(Ogre::CULL_NONE);
            if (def.HasSpecularMap())
            {
                material->getTechnique("BaseTechnique")->getPass("SpecularMapping1")->setCullingMode(Ogre::CULL_NONE);
            }
        }
    }

    /* Finalize */

    material->compile();
    m_managed_materials.insert(std::make_pair(def.name, material));
}

void RigSpawner::ProcessCollisionBox(RigDef::CollisionBox & def)
{
    SPAWNER_PROFILE_SCOPED();

    auto itor = def.nodes.begin();
    auto end  = def.nodes.end();
    for ( ; itor != end; ++itor)
    {
        std::pair<unsigned int, bool> node_result = GetNodeIndex(*itor);
        if (! node_result.second)
        {
            std::stringstream msg;
            msg << "Invalid node '" << itor->ToString() << "'";
            continue;
        }
        m_rig->ar_nodes[node_result.first].collisionBoundingBoxID = static_cast<char>(m_rig->collisionBoundingBoxes.size());
    }

    m_rig->collisionBoundingBoxes.resize(m_rig->collisionBoundingBoxes.size() + 1);
}

bool RigSpawner::AssignWheelToAxle(int & _out_axle_wheel, node_t *axis_node_1, node_t *axis_node_2)
{
    SPAWNER_PROFILE_SCOPED();

    for (int i = 0; i < m_rig->free_wheel; i++)
    {
        wheel_t & wheel = m_rig->wheels[i];
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

void RigSpawner::ProcessAxle(RigDef::Axle & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (! CheckAxleLimit(1))
    {
        return;
    }

    node_t *wheel_1_node_1 = GetNodePointerOrThrow(def.wheels[0][0]);
    node_t *wheel_1_node_2 = GetNodePointerOrThrow(def.wheels[0][1]);
    node_t *wheel_2_node_1 = GetNodePointerOrThrow(def.wheels[1][0]);
    node_t *wheel_2_node_2 = GetNodePointerOrThrow(def.wheels[1][1]);
    Axle *axle = new Axle();

    if (! AssignWheelToAxle(axle->wheel_1, wheel_1_node_1, wheel_1_node_2))
    {
        std::stringstream msg;
        msg << "Couldn't find wheel with axis nodes '" << def.wheels[0][0].ToString()
            << "' and '" << def.wheels[0][1].ToString() << "'";
        AddMessage(Message::TYPE_WARNING, msg.str());
    }

    if (! AssignWheelToAxle(axle->wheel_2, wheel_2_node_1, wheel_2_node_2))
    {
        std::stringstream msg;
        msg << "Couldn't find wheel with axis nodes '" << def.wheels[1][0].ToString()
            << "' and '" << def.wheels[1][1].ToString() << "'";
        AddMessage(Message::TYPE_WARNING, msg.str());
    }

    if (def.options.size() == 0)
    {
        AddMessage(Message::TYPE_INFO, "No differential defined, defaulting to Open & Locked");
        axle->addDiffType(OPEN_DIFF);
        axle->addDiffType(LOCKED_DIFF);
    }
    else
    {
        auto end = def.options.end();
        for (auto itor = def.options.begin(); itor != end; ++itor)
        {
            switch (*itor)
            {
            case RigDef::Axle::OPTION_l_LOCKED:
                axle->addDiffType(LOCKED_DIFF);
                break;
            case RigDef::Axle::OPTION_o_OPEN:
                axle->addDiffType(OPEN_DIFF);
                break;
            case RigDef::Axle::OPTION_s_SPLIT:
                axle->addDiffType(SPLIT_DIFF);
                break;
            default:
                AddMessage(Message::TYPE_WARNING, "Unknown differential type: " + *itor);
                break;
            }
        }
    }

    m_rig->m_axles[m_rig->m_num_axles] = axle;
    m_rig->m_num_axles++;
}

void RigSpawner::ProcessSpeedLimiter(RigDef::SpeedLimiter & def)
{
    SPAWNER_PROFILE_SCOPED();

    m_rig->sl_enabled     = def.is_enabled;
    m_rig->sl_speed_limit = def.max_speed;
}

void RigSpawner::ProcessCruiseControl(RigDef::CruiseControl & def)
{
    SPAWNER_PROFILE_SCOPED();

    m_rig->cc_target_speed_lower_limit = def.min_speed;
    if (m_rig->cc_target_speed_lower_limit <= 0.f)
    {
        std::stringstream msg;
        msg << "Invalid parameter 'lower_limit' (" << m_rig->cc_target_speed_lower_limit 
            << ") must be positive nonzero number. Using it anyway (compatibility)";
    }
    m_rig->cc_can_brake = def.autobrake != 0;
}

void RigSpawner::ProcessTorqueCurve(RigDef::TorqueCurve & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (m_rig->ar_engine == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, "Section 'torquecurve' found but no 'engine' defined, skipping...");
        return;
    }

    TorqueCurve *target_torque_curve = m_rig->ar_engine->getTorqueCurve();

    if (def.predefined_func_name.length() != 0)
    {
        target_torque_curve->setTorqueModel(def.predefined_func_name);
    }
    else
    {
        target_torque_curve->CreateNewCurve(); /* Use default name for custom curve */
        std::vector<RigDef::TorqueCurve::Sample>::iterator itor = def.samples.begin();
        for ( ; itor != def.samples.end(); itor++)
        {
            target_torque_curve->AddCurveSample(itor->power, itor->torque_percent);
        }
    }
}

void RigSpawner::ProcessParticle(RigDef::Particle & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (App::gfx_particles_mode.GetActive() != 1)
    {
        return;
    }

    unsigned int particle_index = m_rig->free_cparticle;
    m_rig->free_cparticle++;
    cparticle_t & particle = m_rig->cparticles[particle_index];

    particle.emitterNode = GetNodeIndexOrThrow(def.emitter_node);
    particle.directionNode = GetNodeIndexOrThrow(def.reference_node);

    /* Setup visuals */
    particle.psys = gEnv->sceneManager->createParticleSystem(this->ComposeName("CustomParticles",0), def.particle_system_name);
    if (particle.psys == nullptr)
    {
        std::stringstream msg;
        msg << "Failed to create particle system '" << def.particle_system_name << "'";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return;
    }
    particle.psys->setVisibilityFlags(DEPTHMAP_DISABLED);
    particle.snode->attachObject(particle.psys);
    particle.snode->setPosition(GetNode(particle.emitterNode).AbsPosition);
    
    /* Shut down the emitters */
    particle.active = false; 
    for (unsigned int i = 0; i < particle.psys->getNumEmitters(); i++)
    {
        particle.psys->getEmitter(i)->setEnabled(false);
    }
}

void RigSpawner::ProcessRopable(RigDef::Ropable & def)
{
    SPAWNER_PROFILE_SCOPED();

    ropable_t ropable;
    ropable.node = GetNodePointerOrThrow(def.node);
    ropable.group = def.group;
    ropable.in_use = false;
    ropable.multilock = def.has_multilock;
    m_rig->ropables.push_back(ropable);
}

void RigSpawner::ProcessTie(RigDef::Tie & def)
{
    SPAWNER_PROFILE_SCOPED();

    node_t & node_1 = GetNodeOrThrow(def.root_node);
    node_t & node_2 = GetNode( (node_1.pos == 0) ? 1 : 0 );

    int beam_index = m_rig->ar_num_beams;
    beam_t & beam = AddBeam(node_1, node_2, def.beam_defaults, def.detacher_group);
    SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold());
    beam.k = def.beam_defaults->GetScaledSpringiness();
    beam.d = def.beam_defaults->GetScaledDamping();
    beam.bm_type = (def.is_invisible) ? BEAM_INVISIBLE_HYDRO : BEAM_HYDRO;
    beam.L = def.max_reach_length;
    beam.refL = def.max_reach_length;
    beam.Lhydro = def.max_reach_length;
    beam.bounded = ROPE;
    beam.bm_disabled = true;
    beam.commandRatioLong = def.auto_shorten_rate;
    beam.commandRatioShort = def.auto_shorten_rate;
    beam.commandShort = def.min_length;
    beam.commandLong = def.max_length;
    beam.maxtiestress = def.max_stress;
    CreateBeamVisuals(beam, beam_index, def.beam_defaults);

    /* Register tie */
    tie_t tie;
    tie.group = def.group;
    tie.tying = false;
    tie.tied = false;
    tie.beam = & beam;
    tie.commandValue = -1.f;
    m_rig->ties.push_back(tie);

    m_rig->m_has_command_beams = true;
}

void RigSpawner::ProcessRope(RigDef::Rope & def)
{
    SPAWNER_PROFILE_SCOPED();

    node_t & root_node = GetNodeOrThrow(def.root_node);
    node_t & end_node = GetNodeOrThrow(def.end_node);

    /* Add beam */
    beam_t & beam = AddBeam(root_node, end_node, def.beam_defaults, def.detacher_group);
    SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold());
    beam.k = def.beam_defaults->GetScaledSpringiness();
    beam.d = def.beam_defaults->GetScaledDamping();
    beam.bounded = ROPE;
    beam.bm_type = (def.invisible) ? BEAM_INVISIBLE_HYDRO : BEAM_HYDRO;

    /* Register rope */
    rope_t rope;
    rope.beam = & beam;
    rope.locked = UNLOCKED;
    rope.lockedto = & m_rig->ar_nodes[0]; // Orig: hardcoded in BTS_ROPES
    rope.lockedto_ropable = nullptr;
    rope.group = 0; // Orig: hardcoded in BTS_ROPES. TODO: To be used.
    m_rig->ropes.push_back(rope);
}

void RigSpawner::ProcessRailGroup(RigDef::RailGroup & def)
{
    SPAWNER_PROFILE_SCOPED();

    Rail *rail = CreateRail(def.node_list);
    if (rail == nullptr)
    {
        return;
    }
    RailGroup *rail_group = new RailGroup(rail, def.id);
    m_rig->m_railgroups.push_back(rail_group);
}

void RigSpawner::ProcessSlidenode(RigDef::SlideNode & def)
{
    SPAWNER_PROFILE_SCOPED();

    node_t & node = GetNodeOrThrow(def.slide_node);
    SlideNode slide_node(& node, nullptr);
    slide_node.setThreshold(def.tolerance);
    slide_node.setSpringRate(def.spring_rate);
    slide_node.setAttachmentRate(def.attachment_rate);
    if (def._break_force_set)
    {
        slide_node.setBreakForce(def.break_force);
    }
    slide_node.setAttachmentDistance(def.max_attachment_distance);

    /* Constraints */
    if (BITMASK_IS_1(def.constraint_flags, RigDef::SlideNode::CONSTRAINT_ATTACH_ALL))
    {
        slide_node.setAttachRule( ATTACH_ALL );
    }
    if (BITMASK_IS_1(def.constraint_flags, RigDef::SlideNode::CONSTRAINT_ATTACH_SELF))
    {
        slide_node.setAttachRule( ATTACH_SELF );
    }
    if (BITMASK_IS_1(def.constraint_flags, RigDef::SlideNode::CONSTRAINT_ATTACH_FOREIGN))
    {
        slide_node.setAttachRule( ATTACH_FOREIGN );
    }
    if (BITMASK_IS_1(def.constraint_flags, RigDef::SlideNode::CONSTRAINT_ATTACH_NONE))
    {
        slide_node.setAttachRule( ATTACH_NONE );
    }

    /* RailGroup */
    RailGroup *rail_group = nullptr;
    if (def._railgroup_id_set)
    {
        std::vector<RailGroup*>::iterator itor = m_rig->m_railgroups.begin();
        for ( ; itor != m_rig->m_railgroups.end(); itor++)
        {
            if ((*itor)->getID() == def.railgroup_id)
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
        Rail *rail = CreateRail(def.rail_node_ranges);
        if (rail == nullptr)
        {
            return;
        }
        rail_group = new RailGroup(rail);
        m_rig->m_railgroups.push_back(rail_group);
    }
    else
    {
        AddMessage(Message::TYPE_ERROR, "No RailGroup available for SlideNode, skipping...");
    }

    slide_node.setDefaultRail(rail_group);
    m_rig->m_slidenodes.push_back(slide_node);
}

int RigSpawner::FindNodeIndex(RigDef::Node::Ref & node_ref, bool silent /* Default: false */)
{
    SPAWNER_PROFILE_SCOPED();

    std::pair<unsigned int, bool> result = GetNodeIndex(node_ref, /* quiet */ true);
    if (result.second)
    {
        return static_cast<int>(result.first);
    }
    else
    {
        if (! silent)
        {
            std::stringstream msg;
            msg << "Failed to find node by reference: " << node_ref.ToString();
            AddMessage(Message::TYPE_ERROR, msg.str());
        }
        return -1; /* Node not found */
    }
}

bool RigSpawner::CollectNodesFromRanges(
    std::vector<RigDef::Node::Range> & node_ranges,
    std::vector<unsigned int> & out_node_indices
    )
{
    SPAWNER_PROFILE_SCOPED();

    std::vector<RigDef::Node::Range>::iterator itor = node_ranges.begin();
    for ( ; itor != node_ranges.end(); itor++)
    {
        if (itor->IsRange())
        {

            int result_a = FindNodeIndex(itor->start, /* silent */ false);
            int result_b = FindNodeIndex(itor->end,   /* silent */ true);

            unsigned int start = 0;
            unsigned int end = 0;

            if (result_a == -1)
            {
                return false;
            }
            else
            {
                start = static_cast<unsigned int>(result_a);
            }

            if (result_b == -1)
            {
                std::stringstream msg;
                msg << "Encountered non-existent node '" << itor->end.ToString() << "' in range [" << itor->start.ToString() << " - " << itor->end.ToString() << "], "
                    << "highest node index is '" << m_rig->ar_num_nodes - 1 << "'.";

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
            else
            {
                end = static_cast<unsigned int>(result_b);
            }

            if (end < start)
            {
                unsigned int swap = start;
                start = end;
                end = swap;
            }

            for (unsigned int i = start; i <= end; i++)
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

Rail *RigSpawner::CreateRail(std::vector<RigDef::Node::Range> & node_ranges)
{
    SPAWNER_PROFILE_SCOPED();

    /* Collect nodes */
    std::vector<unsigned int> node_indices;
    node_indices.reserve(100);

    CollectNodesFromRanges(node_ranges, node_indices);

    /* Find beams &build rail */
    RailBuilder rail_builder; /* rail builder allocates the memory for each rail, it will not free it */
    if (node_indices.front() == node_indices.back())
    {
        rail_builder.loopRail();
    }

    for (unsigned int i = 0; i < node_indices.size() - 1; i++)
    {
        beam_t *beam = FindBeamInRig(node_indices[i], node_indices[i + 1]);
        if (beam == nullptr)
        {
            std::stringstream msg;
            msg << "No beam between nodes indexed '" << node_indices[i] << "' and '" << node_indices[i + 1] << "'";
            AddMessage(Message::TYPE_ERROR, msg.str());
            return nullptr;
        }
        rail_builder.pushBack(beam);
    }

    return rail_builder.getCompletedRail(); /* Transfers memory ownership */
}

beam_t *RigSpawner::FindBeamInRig(unsigned int node_a_index, unsigned int node_b_index)
{
    SPAWNER_PROFILE_SCOPED();

    node_t *node_a = & m_rig->ar_nodes[node_a_index];
    node_t *node_b = & m_rig->ar_nodes[node_b_index];

    for (unsigned int i = 0; i < static_cast<unsigned int>(m_rig->ar_num_beams); i++)
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

void RigSpawner::ProcessHook(RigDef::Hook & def)
{
    SPAWNER_PROFILE_SCOPED();

    /* Find the node */
    node_t *node = GetNodePointer(def.node);
    if (node ==  nullptr)
    {
        return;
    }
    
    /* Find the hook */
    hook_t *hook = nullptr;
    std::vector <hook_t>::iterator itor = m_rig->hooks.begin();
    for (; itor != m_rig->hooks.end(); itor++)
    {
        if (itor->hookNode == node)
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
    hook->lockrange = def.option_hook_range;
    hook->lockspeed = def.option_speed_coef * HOOK_SPEED_DEFAULT;
    hook->maxforce  = def.option_max_force;
    hook->group     = def.option_hookgroup;
    hook->lockgroup = def.option_lockgroup;
    hook->timer     = 0.f; // Hardcoded in BTS_HOOKS
    hook->timer_preset = def.option_timer;
    hook->beam->commandShort = def.option_min_range_meters;
    hook->selflock = BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_SELF_LOCK);
    hook->nodisable = BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_NO_DISABLE);
    if (BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_AUTO_LOCK))
    {
        hook->autolock = true;
        if (hook->group == -1)
        {
            hook->group = -2; /* only overwrite hgroup when its still default (-1) */
        }
    }
    if (BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_NO_ROPE))
    {
        hook->beam->bounded = NOSHOCK;
    }
    if (!BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_VISIBLE))
    {
        hook->beam->bm_type = BEAM_INVISIBLE_HYDRO;
    }
}

void RigSpawner::ProcessLockgroup(RigDef::Lockgroup & lockgroup)
{
    SPAWNER_PROFILE_SCOPED();

    auto itor = lockgroup.nodes.begin();
    auto end  = lockgroup.nodes.end();
    for (; itor != end; ++itor)
    {
        GetNodeOrThrow(*itor).lockgroup = lockgroup.number;
    }
}

void RigSpawner::ProcessTrigger(RigDef::Trigger & def)
{
    SPAWNER_PROFILE_SCOPED();

    shock_t & shock = this->GetFreeShock();

    // Disable trigger on startup? (default enabled)
    shock.trigger_enabled = !def.HasFlag_x_StartDisabled();

    m_rig->commandkey[def.shortbound_trigger_action].trigger_cmdkeyblock_state = false;
    if (def.longbound_trigger_action != -1)
    {
        m_rig->commandkey[def.longbound_trigger_action].trigger_cmdkeyblock_state = false;
    }

    unsigned int hydro_type = BEAM_HYDRO;
    unsigned int shock_flags = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISTRIGGER;
    float short_limit = def.contraction_trigger_limit;
    float long_limit = def.expansion_trigger_limit;

    if (def.HasFlag_i_Invisible())
    {
        hydro_type = BEAM_INVISIBLE_HYDRO;
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_INVISIBLE);
    }
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

    int node_1_index = FindNodeIndex(def.nodes[0]);
    int node_2_index = FindNodeIndex(def.nodes[1]);
    if (node_1_index == -1 || node_2_index == -1 )
    {
        return;
    }
    int beam_index = m_rig->ar_num_beams;
    beam_t & beam = AddBeam(GetNode(node_1_index), GetNode(node_2_index), def.beam_defaults, def.detacher_group);
    beam.bm_type = hydro_type;
    SetBeamStrength(beam, def.beam_defaults->breaking_threshold);
    SetBeamSpring(beam, 0.f);
    SetBeamDamping(beam, 0.f);
    CalculateBeamLength(beam);
    beam.shortbound = short_limit;
    beam.longbound = long_limit;
    beam.bounded = SHOCK2;
    beam.shock = &shock;

    CreateBeamVisuals(beam, beam_index, def.beam_defaults);

    if (m_rig->m_trigger_debug_enabled)
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
        m_rig->commandkey[def.shortbound_trigger_action].trigger_cmdkeyblock_state = true;
        if (def.longbound_trigger_action != -1)
        {
            m_rig->commandkey[def.longbound_trigger_action].trigger_cmdkeyblock_state = true;
        }
    }

    shock.trigger_boundary_t = def.boundary_timer;
    shock.flags              = shock_flags;
    shock.sbd_spring         = def.beam_defaults->springiness;
    shock.sbd_damp           = def.beam_defaults->damping_constant;
    shock.last_debug_state   = 0;
    
}

void RigSpawner::ProcessContacter(RigDef::Node::Ref & node_ref)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int node_index = GetNodeIndexOrThrow(node_ref);
    m_rig->contacters[m_rig->free_contacter].nodeid = node_index;
    m_rig->free_contacter++;
};

void RigSpawner::ProcessRotator(RigDef::Rotator & def)
{
    SPAWNER_PROFILE_SCOPED();

    rotator_t & rotator = m_rig->ar_rotators[m_rig->ar_num_rotators];

    rotator.angle = 0;
    rotator.rate = def.rate;
    rotator.axis1 = GetNodeIndexOrThrow(def.axis_nodes[0]);
    rotator.axis2     = GetNodeIndexOrThrow(def.axis_nodes[1]);
    rotator.force     = ROTATOR_FORCE_DEFAULT;
    rotator.tolerance = ROTATOR_TOLERANCE_DEFAULT;
    rotator.rotatorEngineCoupling = def.engine_coupling;
    rotator.rotatorNeedsEngine = def.needs_engine;
    for (unsigned int i = 0; i < 4; i++)
    {
        rotator.nodes1[i] = GetNodeIndexOrThrow(def.base_plate_nodes[i]);
        rotator.nodes2[i] = GetNodeIndexOrThrow(def.rotating_plate_nodes[i]);
    }

    // Rotate left key
    m_rig->commandkey[def.spin_left_key].rotators.push_back(- (m_rig->ar_num_rotators + 1));
    m_rig->commandkey[def.spin_left_key].description = "Rotate_Left/Right";

    // Rotate right key
    m_rig->commandkey[def.spin_right_key].rotators.push_back(m_rig->ar_num_rotators + 1);

    _ProcessKeyInertia(m_rig->m_rotator_inertia, def.inertia, *def.inertia_defaults, def.spin_left_key, def.spin_right_key);

    m_rig->ar_num_rotators++;
    m_rig->m_has_command_beams = true;
}

void RigSpawner::ProcessRotator2(RigDef::Rotator2 & def)
{
    SPAWNER_PROFILE_SCOPED();

    rotator_t & rotator = m_rig->ar_rotators[m_rig->ar_num_rotators];

    rotator.angle = 0;
    rotator.rate = def.rate;
    rotator.axis1 = GetNodeIndexOrThrow(def.axis_nodes[0]);
    rotator.axis2     = GetNodeIndexOrThrow(def.axis_nodes[1]);
    rotator.force     = def.rotating_force; // Default value is set in constructor
    rotator.tolerance = def.tolerance; // Default value is set in constructor
    rotator.rotatorEngineCoupling = def.engine_coupling;
    rotator.rotatorNeedsEngine = def.needs_engine;
    for (unsigned int i = 0; i < 4; i++)
    {
        rotator.nodes1[i] = GetNodeIndexOrThrow(def.base_plate_nodes[i]);
        rotator.nodes2[i] = GetNodeIndexOrThrow(def.rotating_plate_nodes[i]);
    }

    // Rotate left key
    m_rig->commandkey[def.spin_left_key].rotators.push_back(- (m_rig->ar_num_rotators + 1));
    if (! def.description.empty())
    {
        m_rig->commandkey[def.spin_left_key].description = def.description;
    }
    else
    {
        m_rig->commandkey[def.spin_left_key].description = "Rotate_Left/Right";
    }

    // Rotate right key
    m_rig->commandkey[def.spin_right_key].rotators.push_back(m_rig->ar_num_rotators + 1);

    _ProcessKeyInertia(m_rig->m_rotator_inertia, def.inertia, *def.inertia_defaults, def.spin_left_key, def.spin_right_key);

    m_rig->ar_num_rotators++;
    m_rig->m_has_command_beams = true;
}

void RigSpawner::_ProcessKeyInertia(
    CmdKeyInertia * key_inertia,
    RigDef::Inertia & inertia,
    RigDef::Inertia & inertia_defaults,
    int contract_key, 
    int extend_key
)
{
    SPAWNER_PROFILE_SCOPED();

    if (key_inertia != nullptr)
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
            key_inertia->setCmdKeyDelay(
                contract_key,
                inertia.start_delay_factor,
                inertia.stop_delay_factor,
                start_function,
                stop_function
            );

            key_inertia->setCmdKeyDelay(
                extend_key,
                inertia.start_delay_factor,
                inertia.stop_delay_factor,
                start_function,
                stop_function
            );
        }
        else if (inertia_defaults.start_delay_factor > 0 || inertia_defaults.stop_delay_factor > 0)
        {
            key_inertia->setCmdKeyDelay(
                contract_key,
                inertia_defaults.start_delay_factor,
                inertia_defaults.stop_delay_factor,
                inertia_defaults.start_function,
                inertia_defaults.stop_function
            );

            key_inertia->setCmdKeyDelay(
                extend_key,
                inertia_defaults.start_delay_factor,
                inertia_defaults.stop_delay_factor,
                inertia_defaults.start_function,
                inertia_defaults.stop_function
            );
        }
    }
}

void RigSpawner::ProcessCommand(RigDef::Command2 & def)
{
    SPAWNER_PROFILE_SCOPED();

    int beam_index = m_rig->ar_num_beams;
    int node_1_index = FindNodeIndex(def.nodes[0]);
    int node_2_index = FindNodeIndex(def.nodes[1]);
    if (node_1_index == -1 || node_2_index == -1)
    {
        AddMessage(Message::TYPE_ERROR, "Failed to fetch node");
        return;
    }
    beam_t & beam = AddBeam(m_rig->ar_nodes[node_1_index], m_rig->ar_nodes[node_2_index], def.beam_defaults, def.detacher_group);
    CalculateBeamLength(beam);
    SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold()); /* Override settings from AddBeam() */
    SetBeamSpring(beam, def.beam_defaults->GetScaledSpringiness());
    SetBeamDamping(beam, def.beam_defaults->GetScaledDamping());
    beam.bm_type = BEAM_HYDRO;

    /* Options */
    if (def.option_i_invisible)     { beam.bm_type = BEAM_INVISIBLE_HYDRO; }
    if (def.option_r_rope)          { beam.bounded = ROPE; }
    if (def.option_p_1press)        { beam.isOnePressMode = 1; }
    if (def.option_o_1press_center) { beam.isOnePressMode = 2; }
    if (def.option_f_not_faster)    { beam.isForceRestricted = true; }
    if (def.option_c_auto_center)   { beam.isCentering = true; }

    beam.commandRatioShort     = def.shorten_rate;
    beam.commandRatioLong      = def.lengthen_rate;
    beam.commandShort          = def.max_contraction;
    beam.commandLong           = def.max_extension;
    beam.commandEngineCoupling = def.affect_engine;
    beam.commandNeedsEngine    = def.needs_engine;
    beam.playsSound            = def.plays_sound;

    /* set the middle of the command, so its not required to recalculate this everytime ... */
    if (def.max_extension > def.max_contraction)
    {
        beam.centerLength = (def.max_extension - def.max_contraction) / 2 + def.max_contraction;
    }
    else
    {
        beam.centerLength = (def.max_contraction - def.max_extension) / 2 + def.max_extension;
    }

    _ProcessKeyInertia(m_rig->m_command_inertia, def.inertia, *def.inertia_defaults, def.contract_key, def.extend_key);	

    /* Add keys */
    command_t* contract_command = &m_rig->commandkey[def.contract_key];
    contract_command->beams.push_back(-beam_index);
    if (contract_command->description.empty())
    {
        contract_command->description = def.description;
    }

    command_t* extend_command = &m_rig->commandkey[def.extend_key];
    extend_command->beams.push_back(beam_index);
    if (extend_command->description.empty())
    {
        extend_command->description = def.description;
    }

    CreateBeamVisuals(beam, beam_index, def.beam_defaults);

    m_rig->m_num_command_beams++;
    m_rig->m_has_command_beams = true;
}

void RigSpawner::ProcessAnimator(RigDef::Animator & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (! CheckHydroLimit(1)) // TODO: remove the limit! See `RigSpawner::CalcMemoryRequirements()` ~ only_a_ptr, 06/2017
    {
        return;
    }

    if (m_rig->m_hydro_inertia != nullptr)
    {
        if (def.inertia_defaults->start_delay_factor > 0 && def.inertia_defaults->stop_delay_factor > 0)
        {
            m_rig->m_hydro_inertia->setCmdKeyDelay(
                m_rig->free_hydro,
                def.inertia_defaults->start_delay_factor,
                def.inertia_defaults->stop_delay_factor,
                def.inertia_defaults->start_function,
                def.inertia_defaults->stop_function
            );
        }
    }

    unsigned int hydro_type = BEAM_HYDRO;
    unsigned int anim_flags = 0;
    float anim_option = 0;

    /* Options. '{' intentionally misplaced. */

    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_VISIBLE)) {
        hydro_type = BEAM_HYDRO;
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_INVISIBLE)) {
        hydro_type = BEAM_INVISIBLE_HYDRO;
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_AIRSPEED)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AIRSPEED);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_VERTICAL_VELOCITY)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_VVI);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ANGLE_OF_ATTACK)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AOA);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_FLAP)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_FLAP);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_AIR_BRAKE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AIRBRAKE);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ROLL))	{
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ROLL);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_PITCH)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_PITCH);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_BRAKES)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_BRAKE);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ACCEL)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ACCEL);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_CLUTCH)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_CLUTCH);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SPEEDO)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SPEEDO);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_TACHO)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_TACHO);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_TURBO)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_TURBO);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_PARKING)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_PBRAKE);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_TORQUE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_TORQUE);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_BOAT_THROTTLE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_BTHROTTLE);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_BOAT_RUDDER)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_BRUDDER);
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SHIFT_LEFT_RIGHT)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
        anim_option = 1.f;
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SHIFT_BACK_FORTH))	{
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
        anim_option = 2.f;
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SEQUENTIAL_SHIFT)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
        anim_option = 3.f;
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_GEAR_SELECT)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
        anim_option = 4.f;
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ALTIMETER_100K)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ALTIMETER);
        anim_option = 1.f;
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ALTIMETER_10K)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ALTIMETER);
        anim_option = 2.f;
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ALTIMETER_1K)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_ALTIMETER);
        anim_option = 3.f;
    }
    
    /* Aerial */
    if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_THROTTLE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_THROTTLE);
        anim_option = static_cast<float>(def.aero_animator.motor);
    }
    if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_RPM)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_RPM);
        anim_option = static_cast<float>(def.aero_animator.motor);
    }
    if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_TORQUE)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AETORQUE);
        anim_option = static_cast<float>(def.aero_animator.motor);
    }
    if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_PITCH)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AEPITCH);
        anim_option = static_cast<float>(def.aero_animator.motor);
    }
    if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_STATUS)) {
        BITMASK_SET_1(anim_flags, ANIM_FLAG_AESTATUS);
        anim_option = static_cast<float>(def.aero_animator.motor);
    }

    unsigned int beam_index = m_rig->ar_num_beams;
    beam_t & beam = AddBeam(GetNode(def.nodes[0]), GetNode(def.nodes[1]), def.beam_defaults, def.detacher_group);
    /* set the limits to something with sense by default */
    beam.shortbound = 0.99999f;
    beam.longbound = 1000000.0f;
    beam.bm_type = hydro_type;
    beam.hydroRatio = def.lenghtening_factor;
    beam.animFlags = anim_flags;
    beam.animOption = anim_option;
    CalculateBeamLength(beam);
    SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold());
    SetBeamSpring(beam, def.beam_defaults->GetScaledSpringiness());
    SetBeamDamping(beam, def.beam_defaults->GetScaledDamping());
    CreateBeamVisuals(beam, beam_index, def.beam_defaults);

    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SHORT_LIMIT)) 
    {
        beam.shortbound = def.short_limit;
    }
    if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_LONG_LIMIT)) 
    {
        beam.longbound = def.long_limit;
    }

    m_rig->hydro[m_rig->free_hydro] = beam_index;
    m_rig->free_hydro++;
}

beam_t & RigSpawner::AddBeam(
    node_t & node_1, 
    node_t & node_2, 
    std::shared_ptr<RigDef::BeamDefaults> & beam_defaults,
    int detacher_group
)
{
    SPAWNER_PROFILE_SCOPED();

    /* Init */
    beam_t & beam = GetAndInitFreeBeam(node_1, node_2);
    beam.detacher_group = detacher_group;
    beam.diameter = beam_defaults->visual_beam_diameter;
    beam.bm_disabled = false;

    /* Breaking threshold (strength) */
    float strength = beam_defaults->breaking_threshold;
    beam.strength = strength;

    /* Deformation */
    SetBeamDeformationThreshold(beam, beam_defaults);

    float plastic_coef = beam_defaults->plastic_deform_coef;
    beam.plastic_coef = plastic_coef;

    return beam;
}

void RigSpawner::SetBeamStrength(beam_t & beam, float strength)
{
    SPAWNER_PROFILE_SCOPED();

    beam.strength = strength;
}

void RigSpawner::ProcessHydro(RigDef::Hydro & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (! CheckHydroLimit(1)) // TODO: remove the limit! See `RigSpawner::CalcMemoryRequirements()` ~ only_a_ptr, 06/2017
    {
        return;
    }

    unsigned int hydro_type = BEAM_HYDRO;
    unsigned int hydro_flags = 0;

    // Parse options
    if (def.options.empty()) // Parse as if option 'n' (OPTION_n_NORMAL) was present
    {
        hydro_type = BEAM_HYDRO;
        hydro_flags |= HYDRO_FLAG_DIR;
    }
    else
    {
        for (unsigned int i = 0; i < def.options.length(); ++i)
        {
            const char c = def.options[i];
            switch (c)
            {
                case RigDef::Hydro::OPTION_i_INVISIBLE:  // i
                    hydro_type = BEAM_INVISIBLE_HYDRO;
                    break;
                case RigDef::Hydro::OPTION_n_NORMAL:  // n
                    hydro_type = BEAM_HYDRO;
                    hydro_flags |= HYDRO_FLAG_DIR;
                    break;
                case RigDef::Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED:  // 's': // speed changing hydro
                    hydro_flags |= HYDRO_FLAG_SPEED;
                    break;
                case RigDef::Hydro::OPTION_a_INPUT_AILERON:  // 'a':
                    hydro_flags |= HYDRO_FLAG_AILERON;
                    break;
                case RigDef::Hydro::OPTION_r_INPUT_RUDDER:  // 'r':
                    hydro_flags |= HYDRO_FLAG_RUDDER;
                    break;
                case RigDef::Hydro::OPTION_e_INPUT_ELEVATOR:  // 'e':
                    hydro_flags |= HYDRO_FLAG_ELEVATOR;
                    break;
                case RigDef::Hydro::OPTION_u_INPUT_AILERON_ELEVATOR:  // 'u':
                    hydro_flags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_ELEVATOR);
                    break;
                case RigDef::Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR:  // 'v':
                    hydro_flags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_ELEVATOR);
                    break;
                case RigDef::Hydro::OPTION_x_INPUT_AILERON_RUDDER:  // 'x':
                    hydro_flags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_RUDDER);
                    break;
                case RigDef::Hydro::OPTION_y_INPUT_InvAILERON_RUDDER:  // 'y':
                    hydro_flags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_RUDDER);
                    break;
                case RigDef::Hydro::OPTION_g_INPUT_ELEVATOR_RUDDER:  // 'g':
                    hydro_flags |= (HYDRO_FLAG_ELEVATOR | HYDRO_FLAG_RUDDER);
                    break;
                case RigDef::Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER:  // 'h':
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
            if (hydro_type == BEAM_INVISIBLE_HYDRO && !hydro_flags)
            {
                hydro_flags |= HYDRO_FLAG_DIR;
            }
        }
    }

    _ProcessKeyInertia(m_rig->m_hydro_inertia, def.inertia, *def.inertia_defaults, m_rig->free_hydro, m_rig->free_hydro);	

    node_t & node_1 = GetNode(def.nodes[0]);
    node_t & node_2 = GetNode(def.nodes[1]);

    int beam_index = m_rig->ar_num_beams;
    beam_t & beam = AddBeam(node_1, node_2, def.beam_defaults, def.detacher_group);
    SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold());
    CalculateBeamLength(beam);
    beam.bm_type              = hydro_type;
    beam.k                    = def.beam_defaults->GetScaledSpringiness();
    beam.d                    = def.beam_defaults->GetScaledDamping();
    beam.hydroFlags           = hydro_flags;
    beam.hydroRatio           = def.lenghtening_factor;

    CreateBeamVisuals(beam, beam_index, def.beam_defaults);

    m_rig->hydro[m_rig->free_hydro] = beam_index;
    m_rig->free_hydro++;
}

void RigSpawner::ProcessShock2(RigDef::Shock2 & def)
{
    SPAWNER_PROFILE_SCOPED();

    node_t & node_1 = GetNode(def.nodes[0]);
    node_t & node_2 = GetNode(def.nodes[1]);
    float short_bound = def.short_bound;
    float long_bound = def.long_bound;
    unsigned int hydro_type = BEAM_HYDRO;
    unsigned int shock_flags = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISSHOCK2;

    if (BITMASK_IS_1(def.options, RigDef::Shock2::OPTION_i_INVISIBLE))
    {
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_INVISIBLE);
        hydro_type = BEAM_INVISIBLE_HYDRO;
    }
    if (BITMASK_IS_1(def.options, RigDef::Shock2::OPTION_s_SOFT_BUMP_BOUNDS))
    {
        BITMASK_SET_0(shock_flags, SHOCK_FLAG_NORMAL); /* Not normal anymore */
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_SOFTBUMP);
    }
    if (BITMASK_IS_1(def.options, RigDef::Shock2::OPTION_m_METRIC))
    {
        float beam_length = node_1.AbsPosition.distance(node_2.AbsPosition);
        short_bound /= beam_length;
        long_bound /= beam_length;
    }
    if (BITMASK_IS_1(def.options, RigDef::Shock2::OPTION_M_ABSOLUTE_METRIC))
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
    
    int beam_index = m_rig->ar_num_beams;
    beam_t & beam = AddBeam(node_1, node_2, def.beam_defaults, def.detacher_group);
    SetBeamStrength(beam, def.beam_defaults->breaking_threshold * 4.f);
    beam.bm_type              = hydro_type;
    beam.bounded              = SHOCK2;
    beam.k                    = def.spring_in;
    beam.d                    = def.damp_in;
    beam.shortbound           = short_bound;
    beam.longbound            = long_bound;

    /* Length + pre-compression */
    CalculateBeamLength(beam);
    beam.L          *= def.precompression;
    beam.refL       *= def.precompression;
    beam.Lhydro     *= def.precompression;

    CreateBeamVisuals(beam, beam_index, def.beam_defaults);

    shock_t & shock  = GetFreeShock();
    shock.flags      = shock_flags;
    shock.sbd_spring = def.beam_defaults->springiness;
    shock.sbd_damp   = def.beam_defaults->damping_constant;
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

void RigSpawner::ProcessShock(RigDef::Shock & def)
{
    SPAWNER_PROFILE_SCOPED();

    node_t & node_1 = GetNode(def.nodes[0]);
    node_t & node_2 = GetNode(def.nodes[1]);
    float short_bound = def.short_bound;
    float long_bound = def.long_bound;
    unsigned int hydro_type = BEAM_HYDRO;
    unsigned int shock_flags = SHOCK_FLAG_NORMAL;

    if (BITMASK_IS_1(def.options, RigDef::Shock::OPTION_i_INVISIBLE))
    {
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_INVISIBLE);
        hydro_type = BEAM_INVISIBLE_HYDRO;
    }
    if (BITMASK_IS_1(def.options, RigDef::Shock::OPTION_L_ACTIVE_LEFT))
    {
        BITMASK_SET_0(shock_flags, SHOCK_FLAG_NORMAL); /* Not normal anymore */
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_LACTIVE);
        m_rig->ar_has_active_shocks = true;
    }
    if (BITMASK_IS_1(def.options, RigDef::Shock::OPTION_R_ACTIVE_RIGHT))
    {
        BITMASK_SET_0(shock_flags, SHOCK_FLAG_NORMAL); /* Not normal anymore */
        BITMASK_SET_1(shock_flags, SHOCK_FLAG_RACTIVE);
        m_rig->ar_has_active_shocks = true;
    }
    if (BITMASK_IS_1(def.options, RigDef::Shock::OPTION_m_METRIC))
    {
        float beam_length = node_1.AbsPosition.distance(node_2.AbsPosition);
        short_bound /= beam_length;
        long_bound /= beam_length;
    }
    
    int beam_index = m_rig->ar_num_beams;
    beam_t & beam = AddBeam(node_1, node_2, def.beam_defaults, def.detacher_group);
    beam.shortbound = short_bound;
    beam.longbound  = long_bound;
    beam.bounded    = SHOCK1;
    beam.bm_type    = hydro_type;
    beam.k          = def.spring_rate;
    beam.d          = def.damping;
    SetBeamStrength(beam, def.beam_defaults->breaking_threshold * 4.f);

    /* Length + pre-compression */
    CalculateBeamLength(beam);
    beam.L          *= def.precompression;
    beam.refL       *= def.precompression;
    beam.Lhydro     *= def.precompression;

    shock_t & shock  = GetFreeShock();
    shock.flags      = shock_flags;
    shock.sbd_spring = def.beam_defaults->springiness;
    shock.sbd_damp   = def.beam_defaults->damping_constant;

    /* Create beam visuals, but don't attach them to scene graph */
    /* Old parser did it like this, I don't know why ~ only_a_ptr 13-04-14 */
    CreateBeamVisuals(beam, beam_index, def.beam_defaults);

    beam.shock = & shock;
    shock.beamid = beam_index;
}

void RigSpawner::FetchAxisNodes(
    node_t* & axis_node_1, 
    node_t* & axis_node_2, 
    RigDef::Node::Ref const & axis_node_1_id,
    RigDef::Node::Ref const & axis_node_2_id
)
{
    SPAWNER_PROFILE_SCOPED();

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

void RigSpawner::ProcessFlexBodyWheel(RigDef::FlexBodyWheel & def)
{
    SPAWNER_PROFILE_SCOPED();

    // Check capacities
    CheckFlexbodyLimit(1); // TODO: remove the limit! See `RigSpawner::CalcMemoryRequirements()` ~ only_a_ptr, 06/2017

    unsigned int base_node_index = m_rig->ar_num_nodes;
    wheel_t & wheel = m_rig->wheels[m_rig->free_wheel];

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
    // NOTE: node.iswheel is not used for rim nodes
    for (unsigned int i = 0; i < def.num_rays; i++)
    {
        float node_mass = def.mass / (4.f * def.num_rays);

        // Outer ring
        Ogre::Vector3 ray_point = axis_node_1->RelPosition + rim_ray_vector;
        rim_ray_vector = rim_ray_rotator * rim_ray_vector;

        node_t & outer_node      = GetFreeNode();
        InitNode(outer_node, ray_point, def.node_defaults);

        outer_node.mass          = node_mass;
        outer_node.id            = -1; // Orig: hardcoded (addWheel2)
        outer_node.wheelid       = m_rig->free_wheel;
        outer_node.friction_coef = def.node_defaults->friction;
        AdjustNodeBuoyancy(outer_node, def.node_defaults);

        // Inner ring
        ray_point = axis_node_2->RelPosition + rim_ray_vector;
        rim_ray_vector = rim_ray_rotator * rim_ray_vector;

        node_t & inner_node      = GetFreeNode();
        InitNode(inner_node, ray_point, def.node_defaults);

        inner_node.mass          = node_mass;
        inner_node.id            = -1; // Orig: hardcoded (addWheel2)
        inner_node.wheelid       = m_rig->free_wheel;
        inner_node.friction_coef = def.node_defaults->friction;
        AdjustNodeBuoyancy(inner_node, def.node_defaults);

        // Wheel object
        wheel.wh_nodes[i * 2]       = & outer_node;
        wheel.wh_nodes[(i * 2) + 1] = & inner_node;
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
        outer_node.id            = -1; // Orig: hardcoded (addWheel3)
        outer_node.wheelid       = m_rig->free_wheel;
        outer_node.friction_coef = def.node_defaults->friction;
        outer_node.volume_coef   = def.node_defaults->volume;
        outer_node.surface_coef  = def.node_defaults->surface;
        outer_node.iswheel       = WHEEL_FLEXBODY;
        AdjustNodeBuoyancy(outer_node, def.node_defaults);

        contacter_t & outer_contacter = m_rig->contacters[m_rig->free_contacter];
        outer_contacter.nodeid        = outer_node.pos; /* Node index */
        m_rig->free_contacter++;

        // Inner ring
        ray_point = axis_node_2->RelPosition + tyre_ray_vector;
        tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

        node_t & inner_node = GetFreeNode();
        InitNode(inner_node, ray_point);
        inner_node.mass          = node_mass;
        inner_node.id            = -1; // Orig: hardcoded (addWheel3)
        inner_node.wheelid       = m_rig->free_wheel;
        inner_node.friction_coef = def.node_defaults->friction;
        inner_node.volume_coef   = def.node_defaults->volume;
        inner_node.surface_coef  = def.node_defaults->surface;
        inner_node.iswheel       = WHEEL_FLEXBODY;
        AdjustNodeBuoyancy(inner_node, def.node_defaults);

        contacter_t & inner_contacter = m_rig->contacters[m_rig->free_contacter];
        inner_contacter.nodeid        = inner_node.pos; // Node index
        m_rig->free_contacter++;

        // Wheel object
        wheel.wh_nodes[i * 2] = & outer_node;
        wheel.wh_nodes[(i * 2) + 1] = & inner_node;
    }

    // Beams
    float rim_spring = def.rim_springiness;
    float rim_damp = def.rim_damping;
    float tyre_spring = def.tyre_springiness;
    float tyre_damp = def.tyre_damping;
    float tread_spring = def.beam_defaults->springiness;
    float tread_damp = def.beam_defaults->damping_constant;

    for (unsigned int i = 0; i < def.num_rays; i++)
    {
        // --- Rim --- 

        // Rim axis to rim ring
        unsigned int rim_outer_node_index = base_node_index + (i * 2);
        node_t *rim_outer_node = & m_rig->ar_nodes[rim_outer_node_index];
        node_t *rim_inner_node = & m_rig->ar_nodes[rim_outer_node_index + 1];

        AddWheelBeam(axis_node_1, rim_outer_node, rim_spring, rim_damp, def.beam_defaults);
        AddWheelBeam(axis_node_2, rim_inner_node, rim_spring, rim_damp, def.beam_defaults);
        AddWheelBeam(axis_node_2, rim_outer_node, rim_spring, rim_damp, def.beam_defaults);
        AddWheelBeam(axis_node_1, rim_inner_node, rim_spring, rim_damp, def.beam_defaults);

        // Reinforcement rim ring
        unsigned int rim_next_outer_node_index = base_node_index + (((i + 1) % def.num_rays) * 2);
        node_t *rim_next_outer_node = & m_rig->ar_nodes[rim_next_outer_node_index];
        node_t *rim_next_inner_node = & m_rig->ar_nodes[rim_next_outer_node_index + 1];

        AddWheelBeam(rim_outer_node, rim_inner_node,      rim_spring, rim_damp, def.beam_defaults);
        AddWheelBeam(rim_outer_node, rim_next_outer_node, rim_spring, rim_damp, def.beam_defaults);
        AddWheelBeam(rim_inner_node, rim_next_inner_node, rim_spring, rim_damp, def.beam_defaults);
        AddWheelBeam(rim_inner_node, rim_next_outer_node, rim_spring, rim_damp, def.beam_defaults);
    }

    // Tyre beams
    // Quick&dirty port from original SerializedRig::addWheel3()
    for (unsigned int i = 0; i < def.num_rays; i++)
    {
        int rim_node_index    = base_node_index + i*2;
        int tyre_node_index   = base_node_index + i*2 + def.num_rays*2;
        node_t * rim_node     = & m_rig->ar_nodes[rim_node_index];

        AddWheelBeam(rim_node, & m_rig->ar_nodes[tyre_node_index], tyre_spring/2.f, tyre_damp, def.beam_defaults);

        int tyre_base_index = (i == 0) ? tyre_node_index + (def.num_rays * 2) : tyre_node_index;
        AddWheelBeam(rim_node, & m_rig->ar_nodes[tyre_base_index - 1], tyre_spring/2.f, tyre_damp, def.beam_defaults);
        AddWheelBeam(rim_node, & m_rig->ar_nodes[tyre_base_index - 2], tyre_spring/2.f, tyre_damp, def.beam_defaults);

        node_t * next_rim_node = & m_rig->ar_nodes[rim_node_index + 1];
        AddWheelBeam(next_rim_node, & m_rig->ar_nodes[tyre_node_index],     tyre_spring/2.f, tyre_damp, def.beam_defaults);
        AddWheelBeam(next_rim_node, & m_rig->ar_nodes[tyre_node_index + 1], tyre_spring/2.f, tyre_damp, def.beam_defaults);

        {
            int index = (i == 0) ? tyre_node_index + (def.num_rays * 2) - 1 : tyre_node_index - 1;
            node_t * tyre_node = & m_rig->ar_nodes[index];
            AddWheelBeam(next_rim_node, tyre_node, tyre_spring/2.f, tyre_damp, def.beam_defaults);
        }

        //reinforcement (tire tread)
        {
            // Very messy port :(
            // Aliases
            int rimnode = rim_node_index;
            int rays = def.num_rays;

            AddWheelBeam(&m_rig->ar_nodes[rimnode+rays*2], &m_rig->ar_nodes[base_node_index+i*2+1+rays*2], tread_spring, tread_damp, def.beam_defaults);
            AddWheelBeam(&m_rig->ar_nodes[rimnode+rays*2], &m_rig->ar_nodes[base_node_index+((i+1)%rays)*2+rays*2], tread_spring, tread_damp, def.beam_defaults);
            AddWheelBeam(&m_rig->ar_nodes[base_node_index+i*2+1+rays*2], &m_rig->ar_nodes[base_node_index+((i+1)%rays)*2+1+rays*2], tread_spring, tread_damp, def.beam_defaults);
            AddWheelBeam(&m_rig->ar_nodes[rimnode+1+rays*2], &m_rig->ar_nodes[base_node_index+((i+1)%rays)*2+rays*2], tread_spring, tread_damp, def.beam_defaults);

            if (rigidity_node != nullptr)
            {
                if (axis_node_closest_to_rigidity_node == axis_node_1)
                {
                    axis_node_closest_to_rigidity_node = & m_rig->ar_nodes[base_node_index+i*2+rays*2];
                } else
                {
                    axis_node_closest_to_rigidity_node = & m_rig->ar_nodes[base_node_index+i*2+1+rays*2];
                };
                unsigned int beam_index = AddWheelBeam(rigidity_node, axis_node_closest_to_rigidity_node, tyre_spring, tyre_damp, def.beam_defaults);
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

        beam_index = AddWheelBeam(axis_node_1, &m_rig->ar_nodes[tirenode],     tyre_spring/2.f, tyre_damp, def.beam_defaults);
        GetBeam(beam_index).shortbound = support_beams_short_bound;
        GetBeam(beam_index).longbound  = 0.f;
        GetBeam(beam_index).bounded = SHOCK1;

        beam_index = AddWheelBeam(axis_node_2, &m_rig->ar_nodes[tirenode + 1], tyre_spring/2.f, tyre_damp, def.beam_defaults);
        GetBeam(beam_index).shortbound = support_beams_short_bound;
        GetBeam(beam_index).longbound  = 0.f;
        GetBeam(beam_index).bounded = SHOCK1;
    }

    // Wheel object
    wheel.wh_braking = this->TranslateBrakingDef(def.braking);
    wheel.wh_propulsed = def.propulsion;
    wheel.wh_num_nodes = 2 * def.num_rays;
    wheel.wh_axis_node_0 = axis_node_1;
    wheel.wh_axis_node_1 = axis_node_2;
    wheel.wh_radius = def.tyre_radius;
    wheel.wh_arm_node = this->GetNodePointer(def.reference_arm_node);

    if (def.propulsion != RigDef::Wheels::PROPULSION_NONE)
    {
        // for inter-differential locking
        m_rig->m_num_proped_wheels++;
        m_rig->m_proped_wheel_pairs[m_rig->m_num_proped_wheels] = m_rig->free_wheel;
    }
    if (def.braking != RigDef::Wheels::BRAKING_NO)
    {
        m_rig->m_num_braked_wheels++;
    }

    // Find near attach
    Ogre::Real length_1 = (axis_node_1->RelPosition - wheel.wh_arm_node->RelPosition).length();
    Ogre::Real length_2 = (axis_node_2->RelPosition - wheel.wh_arm_node->RelPosition).length();
    wheel.wh_near_attach_node = (length_1 < length_2) ? axis_node_1 : axis_node_2;

    // Commit the wheel
    int wheel_index = m_rig->free_wheel;
    ++m_rig->free_wheel;

    // Create visuals
    BuildMeshWheelVisuals(
        wheel_index,
        base_node_index,
        axis_node_1->pos,
        axis_node_2->pos,
        def.num_rays,
        def.rim_mesh_name,
        "tracks/trans", // Rim material name. Original parser: was hardcoded in BTS_FLEXBODYWHEELS
        def.rim_radius,
        def.side != RigDef::MeshWheel::SIDE_RIGHT
        );

    const std::string flexwheel_name = this->ComposeName("FlexBodyWheel", m_rig->free_flexbody);

    int num_nodes = def.num_rays * 4;
    std::vector<unsigned int> node_indices;
    node_indices.reserve(num_nodes);
    for (int i = 0; i < num_nodes; ++i)
    {
        node_indices.push_back( base_node_index + i );
    }

    RigDef::Flexbody flexbody_def;
    flexbody_def.mesh_name = def.tyre_mesh_name;
    flexbody_def.offset = Ogre::Vector3(0.5,0,0);

    try
    {
        auto* flexbody = m_flex_factory.CreateFlexBody(
            &flexbody_def,
            axis_node_1->pos,
            axis_node_2->pos,
            static_cast<int>(base_node_index),
            Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y),
            node_indices
            );

        if (flexbody == nullptr)
            return; // Error already logged

        this->CreateWheelSkidmarks(static_cast<unsigned>(wheel_index));

        m_rig->flexbodies[m_rig->free_flexbody] = flexbody;
        m_rig->free_flexbody++;
    }
    catch (Ogre::Exception& e)
    {
        this->AddMessage(Message::TYPE_ERROR, 
            "Failed to create flexbodywheel '" + def.tyre_mesh_name + "', reason:" + e.getFullDescription());
    }
}

wheel_t::BrakeCombo RigSpawner::TranslateBrakingDef(RigDef::Wheels::Braking def)
{
    switch (def)
    {
    case RigDef::Wheels::Braking::BRAKING_NO:                return wheel_t::BrakeCombo::NONE;
    case RigDef::Wheels::Braking::BRAKING_YES:               return wheel_t::BrakeCombo::FOOT_HAND;
    case RigDef::Wheels::Braking::BRAKING_DIRECTIONAL_LEFT:  return wheel_t::BrakeCombo::FOOT_HAND_SKID_LEFT;
    case RigDef::Wheels::Braking::BRAKING_DIRECTIONAL_RIGHT: return wheel_t::BrakeCombo::FOOT_HAND_SKID_RIGHT;
    case RigDef::Wheels::Braking::BRAKING_ONLY_FOOT:         return wheel_t::BrakeCombo::FOOT_ONLY;
    default:                                                 return wheel_t::BrakeCombo::NONE;
    }
}

void RigSpawner::ProcessMeshWheel(RigDef::MeshWheel & meshwheel_def)
{
    if (meshwheel_def._is_meshwheel2)
    {
        this->ProcessMeshWheel2(meshwheel_def);
        return;
    }

    SPAWNER_PROFILE_SCOPED();

    unsigned int base_node_index = m_rig->ar_num_nodes;
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
        meshwheel_def.node_defaults,
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
        meshwheel_def.beam_defaults,
        meshwheel_def.rigidity_node
    );

    BuildMeshWheelVisuals(
        wheel_index, 
        base_node_index, 
        axis_node_1->pos,
        axis_node_2->pos,
        meshwheel_def.num_rays,
        meshwheel_def.mesh_name,
        meshwheel_def.material_name,
        meshwheel_def.rim_radius,
        meshwheel_def.side != RigDef::MeshWheel::SIDE_RIGHT
        );

    CreateWheelSkidmarks(wheel_index);
}

void RigSpawner::ProcessMeshWheel2(RigDef::MeshWheel & def)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int base_node_index = m_rig->ar_num_nodes;
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
        def.node_defaults,
        def.mass
    );

    /* --- Beams --- */
    /* Use data from directive 'set_beam_defaults' for the tiretread beams */
    float tyre_spring = def.spring;
    float tyre_damp = def.damping;
    float rim_spring = def.beam_defaults->springiness;
    float rim_damp = def.beam_defaults->damping_constant;

    BuildWheelBeams(
        def.num_rays,
        base_node_index,
        axis_node_1,
        axis_node_2,
        tyre_spring,
        tyre_damp,
        rim_spring,
        rim_damp,
        def.beam_defaults,
        def.rigidity_node,
        0.15 // max_extension
    );

    /* --- Visuals --- */
    BuildMeshWheelVisuals(
        wheel_index, 
        base_node_index, 
        axis_node_1->pos,
        axis_node_2->pos,
        def.num_rays,
        def.mesh_name,
        def.material_name,
        def.rim_radius,
        def.side != RigDef::MeshWheel::SIDE_RIGHT
        );

    CreateWheelSkidmarks(wheel_index);
}

void RigSpawner::BuildMeshWheelVisuals(
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
    SPAWNER_PROFILE_SCOPED();

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
        Ogre::SceneNode* scene_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        scene_node->attachObject(flexmesh_wheel->GetTireEntity());

        m_rig->vwheels[wheel_index].fm = flexmesh_wheel;
        m_rig->vwheels[wheel_index].cnode = scene_node;
    }
    catch (Ogre::Exception& e)
    {
        this->AddMessage(Message::TYPE_ERROR, "Failed to create meshwheel visuals, message: " + e.getFullDescription());
        return;
    }
}

unsigned int RigSpawner::BuildWheelObjectAndNodes( 
    unsigned int num_rays,
    node_t *axis_node_1,
    node_t *axis_node_2,
    node_t *reference_arm_node,
    unsigned int reserve_nodes,
    unsigned int reserve_beams,
    float wheel_radius,
    RigDef::Wheels::Propulsion propulsion,
    RigDef::Wheels::Braking braking,
    std::shared_ptr<RigDef::NodeDefaults> node_defaults,
    float wheel_mass,
    bool set_param_iswheel, /* Default: true */
    float wheel_width       /* Default: -1.f */
)
{
    SPAWNER_PROFILE_SCOPED();

    wheel_t & wheel = m_rig->wheels[m_rig->free_wheel];

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

    if (propulsion != RigDef::Wheels::PROPULSION_NONE)
    {
        /* for inter-differential locking */
        m_rig->m_num_proped_wheels++;
        m_rig->m_proped_wheel_pairs[m_rig->m_num_proped_wheels] = m_rig->free_wheel;
    }
    if (braking != RigDef::Wheels::BRAKING_NO)
    {
        m_rig->m_num_braked_wheels++;
    }
    
    /* Nodes */
    Ogre::Vector3 ray_vector = axis_vector.perpendicular() * wheel_radius;
    Ogre::Quaternion ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.0 / (num_rays * 2)), axis_vector);

#ifdef DEBUG_TRUCKPARSER2013
    // TRUCK PARSER 2013 DEBUG
    std::stringstream msg;
    msg << "\nDBG RigSpawner::BuildWheelObjectAndNodes()\nDBG nodebase:" << m_rig->ar_num_nodes <<", axis-node-0:"<<axis_node_1->pos <<", axis-node-1:"<<axis_node_2->pos<<"\n";
    msg << "DBG ==== Adding nodes ====";
    // END
#endif

    for (unsigned int i = 0; i < num_rays; i++)
    {
        /* Outer ring */
        Ogre::Vector3 ray_point = axis_node_1->RelPosition + ray_vector;
        ray_vector = ray_rotator * ray_vector;

        node_t & outer_node = GetFreeNode();
        InitNode(outer_node, ray_point, node_defaults);
        outer_node.mass    = wheel_mass / (2.f * num_rays);
        outer_node.iswheel = (set_param_iswheel) ? WHEEL_DEFAULT : NOWHEEL;
        outer_node.id      = -1; // Orig: hardcoded (BTS_WHEELS)
        outer_node.wheelid = m_rig->free_wheel;
        AdjustNodeBuoyancy(outer_node, node_defaults);

        contacter_t & outer_contacter = m_rig->contacters[m_rig->free_contacter];
        outer_contacter.nodeid        = outer_node.pos; /* Node index */
        m_rig->free_contacter++;

        /* Inner ring */
        ray_point = axis_node_2->RelPosition + ray_vector;
        ray_vector = ray_rotator * ray_vector;

        node_t & inner_node = GetFreeNode();
        InitNode(inner_node, ray_point, node_defaults);
        inner_node.mass    = wheel_mass / (2.f * num_rays);
        inner_node.iswheel = (set_param_iswheel) ? WHEEL_DEFAULT : NOWHEEL;
        inner_node.id      = -1; // Orig: hardcoded (BTS_WHEELS)
        inner_node.wheelid = m_rig->free_wheel; 
        AdjustNodeBuoyancy(inner_node, node_defaults);

        contacter_t & contacter = m_rig->contacters[m_rig->free_contacter];
        contacter.nodeid        = inner_node.pos; /* Node index */
        m_rig->free_contacter++;

        /* Wheel object */
        wheel.wh_nodes[i * 2] = & outer_node;
        wheel.wh_nodes[(i * 2) + 1] = & inner_node;

#ifdef DEBUG_TRUCKPARSER2013
        // TRUCK PARSER 2013 DEBUG
        int modifier = 0;
        msg << "\nDBG\tN1: index=" << outer_node.pos + modifier <<", wheelid=" << outer_node.wheelid << ", iswheel=" << outer_node.iswheel 
            <<", X=" << outer_node.AbsPosition.x <<", Y=" << outer_node.AbsPosition.y <<", Z=" << outer_node.AbsPosition.z << std::endl
            << "DBG\tN2: index=" << inner_node.pos + modifier <<", wheelid=" << inner_node.wheelid << ", iswheel=" << inner_node.iswheel 
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
    unsigned int wheel_index = m_rig->free_wheel;
    m_rig->free_wheel++;
    return wheel_index;
}

void RigSpawner::AdjustNodeBuoyancy(node_t & node, RigDef::Node & node_def, std::shared_ptr<RigDef::NodeDefaults> defaults)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int options = (defaults->options | node_def.options); // Merge flags
    node.buoyancy = BITMASK_IS_1(options, RigDef::Node::OPTION_b_EXTRA_BUOYANCY) ? 10000.f : m_rig->m_dry_mass/15.f;
}

void RigSpawner::AdjustNodeBuoyancy(node_t & node, std::shared_ptr<RigDef::NodeDefaults> defaults)
{
    SPAWNER_PROFILE_SCOPED();

    node.buoyancy = BITMASK_IS_1(defaults->options, RigDef::Node::OPTION_b_EXTRA_BUOYANCY) ? 10000.f : m_rig->m_dry_mass/15.f;
}

int RigSpawner::FindLowestNodeInRig()
{
    SPAWNER_PROFILE_SCOPED();

    int lowest_node_index = 0;
    float lowest_y = m_rig->ar_nodes[0].AbsPosition.y;

    for (int i = 0; i < m_rig->ar_num_nodes; i++)
    {
        float y = m_rig->ar_nodes[i].AbsPosition.y;
        if (y < lowest_y)
        {
            lowest_y = y;
            lowest_node_index = i;
        }
    }

    return lowest_node_index;
}

int RigSpawner::FindLowestContactingNodeInRig()
{
    SPAWNER_PROFILE_SCOPED();

    int lowest_node_index = FindLowestNodeInRig();
    float lowest_y = std::numeric_limits<float>::max();

    for (int i = 0; i < m_rig->ar_num_nodes; i++)
    {
        if (m_rig->ar_nodes[i].contactless) continue;
        float y = m_rig->ar_nodes[i].AbsPosition.y;
        if (y < lowest_y)
        {
            lowest_y = y;
            lowest_node_index = i;
        }
    }

    return lowest_node_index;
}

void RigSpawner::BuildWheelBeams(
    unsigned int num_rays,
    unsigned int base_node_index,
    node_t *axis_node_1,
    node_t *axis_node_2,
    float tyre_spring,
    float tyre_damping,
    float rim_spring,
    float rim_damping,
    std::shared_ptr<RigDef::BeamDefaults> beam_defaults,
    RigDef::Node::Ref const & rigidity_node_id,
    float max_extension // = 0.f
)
{
    SPAWNER_PROFILE_SCOPED();
    
#ifdef DEBUG_TRUCKPARSER2013
    // DEBUG
    std::stringstream msg;
    msg << "==== BEAMS ====";
    // END DEBUG
#endif

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
        node_t *outer_ring_node = & m_rig->ar_nodes[outer_ring_node_index];
        node_t *inner_ring_node = & m_rig->ar_nodes[outer_ring_node_index + 1];
        
        AddWheelBeam(axis_node_1, outer_ring_node, tyre_spring, tyre_damping, beam_defaults, 0.66f, max_extension);
        AddWheelBeam(axis_node_2, inner_ring_node, tyre_spring, tyre_damping, beam_defaults, 0.66f, max_extension);
        AddWheelBeam(axis_node_2, outer_ring_node, tyre_spring, tyre_damping, beam_defaults);
        AddWheelBeam(axis_node_1, inner_ring_node, tyre_spring, tyre_damping, beam_defaults);

        /* Reinforcement */
        unsigned int next_outer_ring_node_index = base_node_index + (((i + 1) % num_rays) * 2);
        node_t *next_outer_ring_node = & m_rig->ar_nodes[next_outer_ring_node_index];
        node_t *next_inner_ring_node = & m_rig->ar_nodes[next_outer_ring_node_index + 1];

        AddWheelBeam(outer_ring_node, inner_ring_node,      rim_spring, rim_damping, beam_defaults);
        AddWheelBeam(outer_ring_node, next_outer_ring_node, rim_spring, rim_damping, beam_defaults);
        AddWheelBeam(inner_ring_node, next_inner_ring_node, rim_spring, rim_damping, beam_defaults);
        AddWheelBeam(inner_ring_node, next_outer_ring_node, rim_spring, rim_damping, beam_defaults);

#ifdef DEBUG_TRUCKPARSER2013
        // TRUCK PARSER 2013 DEBUG
        int modifier = 0;
        msg<<"\nDBG\tBounded: ";
        msg<<"["<<axis_node_1->pos + modifier<<" "<<outer_ring_node->pos + modifier<<"] ";
        msg<<"["<<axis_node_2->pos + modifier<<" "<<inner_ring_node->pos + modifier<<"] ";
        msg<<"["<<axis_node_2->pos + modifier<<" "<<outer_ring_node->pos + modifier<<"] ";
        msg<<"["<<axis_node_1->pos + modifier<<" "<<inner_ring_node->pos + modifier<<"]";
        //reinforcement
        msg<<"\nDBG\tReinforcement: ";
        msg<<"["<<outer_ring_node->pos + modifier<<" "<<inner_ring_node->pos      + modifier<<"] ";
        msg<<"["<<outer_ring_node->pos + modifier<<" "<<next_outer_ring_node->pos + modifier<<"] ";
        msg<<"["<<inner_ring_node->pos + modifier<<" "<<next_inner_ring_node->pos + modifier<<"] ";
        msg<<"["<<inner_ring_node->pos + modifier<<" "<<next_outer_ring_node->pos + modifier<<"] ";
        // END
#endif

        /* Rigidity beams */
        if (rigidity_node != nullptr)
        {
            node_t *target_node = (rigidity_beam_side_1) ? outer_ring_node : inner_ring_node;
            unsigned int beam_index = AddWheelBeam(rigidity_node, target_node, tyre_spring, tyre_damping, beam_defaults, -1.f, -1.f, BEAM_VIRTUAL);
            m_rig->ar_beams[beam_index].bm_type = BEAM_VIRTUAL;

#ifdef DEBUG_TRUCKPARSER2013
            // DEBUG
            msg<<"\nDBG\tRigidityBeam: ["<<rigidity_node->pos + modifier<<" "<<target_node->pos + modifier<<"] ";
            msg<<((rigidity_beam_side_1) ? "(outer)" : "(inner)");
            // END
#endif
        }
    }
#ifdef DEBUG_TRUCKPARSER2013
    // TRUCK PARSER 2013 DEBUG
    LOG(msg.str());
    // END
#endif
}

unsigned int RigSpawner::AddWheel(RigDef::Wheel & wheel_def)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int base_node_index = m_rig->ar_num_nodes;
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
        wheel_def.node_defaults,
        wheel_def.mass,
        true,
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
        wheel_def.beam_defaults,
        wheel_def.rigidity_node
    );

    CreateWheelVisuals(wheel_index, wheel_def, base_node_index);

    CreateWheelSkidmarks(wheel_index);

    return wheel_index;
}

void RigSpawner::CreateWheelSkidmarks(unsigned int wheel_index)
{
    // Always create, even if disabled by config
    m_rig->m_skid_trails[wheel_index] = new RoR::Skidmark(
        RoR::App::GetSimController()->GetSkidmarkConf(), RoR::App::GetSimController(), &m_rig->wheels[wheel_index], m_rig->m_beam_visuals_parent_scenenode, 300, 20);
}

#if 0 // refactored into pieces
unsigned int RigSpawner::AddWheel(RigDef::Wheel & wheel_def)
{
    /* Check capacity */
    CheckNodeLimit(wheel_def.num_rays * 2);
    CheckBeamLimit(wheel_def.num_rays * 8);

    unsigned int base_node_index = m_rig->ar_num_nodes;
    wheel_t & wheel = m_rig->wheels[m_rig->free_wheel];

    /* Enforce the "second node must have a larger Z coordinate than the first" constraint */
    unsigned int axis_nodes[2];
    if (GetNode(wheel_def.nodes[0]).RelPosition.z < GetNode(wheel_def.nodes[1]).RelPosition.z)
    {
        axis_nodes[0] = GetNodeIndexOrThrow(wheel_def.nodes[0]);
        axis_nodes[1] = GetNodeIndexOrThrow(wheel_def.nodes[1]);
    }
    else
    {
        axis_nodes[0] = GetNodeIndexOrThrow(wheel_def.nodes[1]);
        axis_nodes[1] = GetNodeIndexOrThrow(wheel_def.nodes[0]);
    }
    node_t & axis_node_1 = m_rig->ar_nodes[axis_nodes[0]];
    node_t & axis_node_2 = m_rig->ar_nodes[axis_nodes[1]];

    Ogre::Vector3 axis_vector = axis_node_2.RelPosition - axis_node_1.RelPosition;
    axis_vector.normalise();
    Ogre::Vector3 ray_vector = axis_vector.perpendicular() * wheel_def.radius;
    Ogre::Quaternion ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.f / wheel_def.num_rays * 2), axis_vector);

    /* Nodes */
    for (unsigned int i = 0; i < wheel_def.num_rays; i++)
    {
        /* Outer ring */
        Ogre::Vector3 ray_point = axis_node_1.RelPosition + ray_vector;
        ray_vector = ray_rotator * ray_vector;

        node_t & outer_node = GetFreeNode();
        InitNode(outer_node, ray_point, wheel_def.node_defaults);
        outer_node.mass    = wheel_def.mass / (2.f * wheel_def.num_rays);
        outer_node.iswheel = (m_rig->free_wheel * 2) + 1;
        outer_node.id      = -1; // Orig: hardcoded (BTS_WHEELS)
        outer_node.wheelid = m_rig->free_wheel;

        contacter_t & contacter = m_rig->contacters[m_rig->free_contacter];
        contacter.nodeid        = outer_node.pos; /* Node index */
        m_rig->free_contacter++;

        /* Inner ring */
        ray_point = axis_node_2.RelPosition + ray_vector;
        ray_vector = ray_rotator * ray_vector;

        node_t & inner_node = GetFreeNode();
        InitNode(inner_node, ray_point, wheel_def.node_defaults);
        inner_node.mass    = wheel_def.mass / (2.f * wheel_def.num_rays);
        inner_node.iswheel = (m_rig->free_wheel * 2) + 2;
        inner_node.id      = -1; // Orig: hardcoded (BTS_WHEELS)
        inner_node.wheelid = m_rig->free_wheel; 

        contacter_t & contacter = m_rig->contacters[m_rig->free_contacter];
        contacter.nodeid        = inner_node.pos; /* Node index */
        m_rig->free_contacter++;

        /* Wheel object */
        wheel.ar_nodes[i * 2] = & outer_node;
        wheel.ar_nodes[(i * 2) + 1] = & inner_node;
    }

    /* Beams */
    for (unsigned int i = 0; i < wheel_def.num_rays; i++)
    {
        /* Bounded */
        unsigned int outer_ring_node_index = base_node_index + (i * 2);
        node_t *outer_ring_node = & m_rig->ar_nodes[outer_ring_node_index];
        node_t *inner_ring_node = & m_rig->ar_nodes[outer_ring_node_index + 1];
        
        unsigned int beam_index = SectionWheelsAddBeam(wheel_def, & axis_node_1, outer_ring_node);
        GetBeam(beam_index).shortbound = 0.66f;
        GetBeam(beam_index).longbound = 0.0f;
        beam_index = SectionWheelsAddBeam(wheel_def, & axis_node_2, inner_ring_node);
        GetBeam(beam_index).shortbound = 0.66f;
        GetBeam(beam_index).longbound = 0.0f;
        SectionWheelsAddBeam(wheel_def, & axis_node_2, outer_ring_node);
        SectionWheelsAddBeam(wheel_def, & axis_node_1, inner_ring_node);

        /* Reinforcement */
        unsigned int next_outer_ring_node_index = base_node_index + (((i + 1) % wheel_def.num_rays) * 2);
        node_t *next_outer_ring_node = & m_rig->ar_nodes[next_outer_ring_node_index];
        node_t *next_inner_ring_node = & m_rig->ar_nodes[next_outer_ring_node_index + 1];

        SectionWheelsAddBeam(wheel_def, outer_ring_node, inner_ring_node);
        SectionWheelsAddBeam(wheel_def, outer_ring_node, next_outer_ring_node);
        SectionWheelsAddBeam(wheel_def, inner_ring_node, next_inner_ring_node);
        SectionWheelsAddBeam(wheel_def, inner_ring_node, next_outer_ring_node);
    }

    /* Wheel object */
    wheel.braked    = wheel_def.braking;
    wheel.propulsed = wheel_def.propulsion;
    wheel.nbnodes   = 2 * wheel_def.num_rays;
    wheel.refnode0  = & axis_node_1;
    wheel.refnode1  = & axis_node_2;
    wheel.radius    = wheel_def.radius;
    wheel.width     = axis_vector.length(); /* wheel_def.width is ignored. */
    wheel.arm       = GetNodePointer(wheel_def.reference_arm_node);

    if (wheel_def.propulsion != RigDef::Wheels::PROPULSION_NONE)
    {
        /* for inter-differential locking */
        m_rig->m_num_proped_wheels++;
        m_rig->m_proped_wheel_pairs[m_rig->m_num_proped_wheels] = m_rig->free_wheel;
    }
    if (wheel_def.braking != RigDef::Wheels::BRAKING_NO)
    {
        m_rig->m_num_braked_wheels++;
    }

    /* Find near attach */
    Ogre::Real length_1 = (axis_node_1.RelPosition - wheel.arm->RelPosition).length();
    Ogre::Real length_2 = (axis_node_2.RelPosition - wheel.arm->RelPosition).length();
    wheel.near_attach = & ((length_1 < length_2) ? axis_node_1 : axis_node_2);

    /* Advance */
    unsigned int wheel_index = m_rig->free_wheel;
    m_rig->free_wheel++;
    return wheel_index;
}
#endif

unsigned int RigSpawner::AddWheel2(RigDef::Wheel2 & wheel_2_def)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int base_node_index = m_rig->ar_num_nodes;
    wheel_t & wheel = m_rig->wheels[m_rig->free_wheel];
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

    /* Rigidity node */
    node_t *axis_node_closest_to_rigidity_node = nullptr;
    if (wheel_2_def.rigidity_node.IsValidAnyState())
    {
        node_t & rigidity_node = GetNode(wheel_2_def.rigidity_node);
        Ogre::Real distance_1 = (rigidity_node.RelPosition - axis_node_1->RelPosition).length();
        Ogre::Real distance_2 = (rigidity_node.RelPosition - axis_node_2->RelPosition).length();
        axis_node_closest_to_rigidity_node = ((distance_1 < distance_2)) ? axis_node_1 : axis_node_2;
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

        node_t & outer_node = GetFreeNode();
        InitNode(outer_node, ray_point, wheel_2_def.node_defaults);
        outer_node.mass    = node_mass;
        outer_node.iswheel = WHEEL_2;
        outer_node.id      = -1; // Orig: hardcoded (addWheel2)
        outer_node.wheelid = m_rig->free_wheel;

        /* Inner ring */
        ray_point = axis_node_2->RelPosition + rim_ray_vector;

        node_t & inner_node = GetFreeNode();
        InitNode(inner_node, ray_point, wheel_2_def.node_defaults);
        inner_node.mass    = node_mass;
        inner_node.iswheel = WHEEL_2; 
        inner_node.id      = -1; // Orig: hardcoded (addWheel2)
        inner_node.wheelid = m_rig->free_wheel;

        /* Wheel object */
        wheel.wh_nodes[i * 2] = & outer_node;
        wheel.wh_nodes[(i * 2) + 1] = & inner_node;

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
        outer_node.iswheel       = WHEEL_2;
        outer_node.id            = -1; // Orig: hardcoded (addWheel2)
        outer_node.wheelid       = m_rig->free_wheel;
        outer_node.friction_coef = wheel.wh_width * WHEEL_FRICTION_COEF;
        outer_node.volume_coef   = wheel_2_def.node_defaults->volume;
        outer_node.surface_coef  = wheel_2_def.node_defaults->surface;

        contacter_t & contacter = m_rig->contacters[m_rig->free_contacter];
        contacter.nodeid        = outer_node.pos; /* Node index */
        m_rig->free_contacter++;

        /* Inner ring */
        ray_point = axis_node_2->RelPosition + tyre_ray_vector;

        node_t & inner_node = GetFreeNode();
        InitNode(inner_node, ray_point);
        inner_node.mass          = (0.33f * wheel_2_def.mass) / (2.f * wheel_2_def.num_rays);
        inner_node.iswheel       = WHEEL_2;
        inner_node.id            = -1; // Orig: hardcoded (addWheel2)
        inner_node.wheelid       = m_rig->free_wheel;
        inner_node.friction_coef = wheel.wh_width * WHEEL_FRICTION_COEF;
        inner_node.volume_coef   = wheel_2_def.node_defaults->volume;
        inner_node.surface_coef  = wheel_2_def.node_defaults->surface;

        contacter_t & inner_contacter = m_rig->contacters[m_rig->free_contacter];
        inner_contacter.nodeid        = inner_node.pos; /* Node index */
        m_rig->free_contacter++;

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
        node_t *rim_outer_node = & m_rig->ar_nodes[rim_outer_node_index];
        node_t *rim_inner_node = & m_rig->ar_nodes[rim_outer_node_index + 1];

        unsigned int beam_index;
        beam_index = AddWheelRimBeam(wheel_2_def, axis_node_1, rim_outer_node);
        GetBeam(beam_index).shortbound = 0.66;
        beam_index = AddWheelRimBeam(wheel_2_def, axis_node_2, rim_inner_node);
        GetBeam(beam_index).shortbound = 0.66;
        AddWheelRimBeam(wheel_2_def, axis_node_2, rim_outer_node);
        AddWheelRimBeam(wheel_2_def, axis_node_1, rim_inner_node);

        /* Reinforcement */
        unsigned int rim_next_outer_node_index = base_node_index + (((i + 1) % wheel_2_def.num_rays) * 2);
        node_t *rim_next_outer_node = & m_rig->ar_nodes[rim_next_outer_node_index];
        node_t *rim_next_inner_node = & m_rig->ar_nodes[rim_next_outer_node_index + 1];

        AddWheelRimBeam(wheel_2_def, axis_node_1, rim_outer_node);
        AddWheelRimBeam(wheel_2_def, rim_outer_node, rim_inner_node);
        AddWheelRimBeam(wheel_2_def, rim_outer_node, rim_next_outer_node);
        AddWheelRimBeam(wheel_2_def, rim_inner_node, rim_next_inner_node);
        AddWheelRimBeam(wheel_2_def, rim_outer_node, rim_next_inner_node);
        AddWheelRimBeam(wheel_2_def, rim_inner_node, rim_next_outer_node);
        if (axis_node_closest_to_rigidity_node != nullptr)
        {
            beam_t & beam = GetFreeBeam();
            InitBeam(beam, GetNodePointer(wheel_2_def.rigidity_node), axis_node_closest_to_rigidity_node);
            beam.bm_type = BEAM_VIRTUAL;
            beam.k = wheel_2_def.rim_springiness;
            beam.d = wheel_2_def.rim_damping;
            SetBeamStrength(beam, wheel_2_def.beam_defaults->breaking_threshold);
        }

        /* --- Tyre --- */

        unsigned int tyre_node_index = rim_outer_node_index + (2 * wheel_2_def.num_rays);
        node_t *tyre_outer_node = & m_rig->ar_nodes[tyre_node_index];
        node_t *tyre_inner_node = & m_rig->ar_nodes[tyre_node_index + 1];
        unsigned int tyre_next_node_index = rim_next_outer_node_index + (2 * wheel_2_def.num_rays);
        node_t *tyre_next_outer_node = & m_rig->ar_nodes[tyre_next_node_index];
        node_t *tyre_next_inner_node = & m_rig->ar_nodes[tyre_next_node_index + 1];

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
    wheel.wh_axis_node_0   = axis_node_1;
    wheel.wh_axis_node_1   = axis_node_2;
    wheel.wh_radius        = wheel_2_def.tyre_radius;
    wheel.wh_arm_node      = this->GetNodePointer(wheel_2_def.reference_arm_node);

    if (wheel_2_def.propulsion != RigDef::Wheels::PROPULSION_NONE)
    {
        /* for inter-differential locking */
        m_rig->m_num_proped_wheels++;
        m_rig->m_proped_wheel_pairs[m_rig->m_num_proped_wheels] = m_rig->free_wheel;
    }
    if (wheel_2_def.braking != RigDef::Wheels::BRAKING_NO)
    {
        m_rig->m_num_braked_wheels++;
    }

    /* Find near attach */
    Ogre::Real length_1 = (axis_node_1->RelPosition - wheel.wh_arm_node->RelPosition).length();
    Ogre::Real length_2 = (axis_node_2->RelPosition - wheel.wh_arm_node->RelPosition).length();
    wheel.wh_near_attach_node = (length_1 < length_2) ? axis_node_1 : axis_node_2;

    CreateWheelSkidmarks(static_cast<unsigned>(m_rig->free_wheel));

    /* Advance */
    unsigned int wheel_index = m_rig->free_wheel;
    m_rig->free_wheel++;
    return wheel_index;
}

void RigSpawner::CreateWheelVisuals(unsigned int wheel_index, RigDef::Wheel & wheel_def, unsigned int node_base_index)
{
    // Wrapper, not profiling

    CreateWheelVisuals(
        wheel_index, 
        node_base_index, 
        wheel_def.num_rays,
        wheel_def.face_material_name,
        wheel_def.band_material_name,
        false
        );
}

void RigSpawner::CreateWheelVisuals(unsigned int wheel_index, RigDef::Wheel2 & wheel_2_def, unsigned int node_base_index)
{
    // Wrapper, not profiling

    CreateWheelVisuals(
        wheel_index, 
        node_base_index, 
        wheel_2_def.num_rays,
        wheel_2_def.face_material_name,
        wheel_2_def.band_material_name,
        true,
        wheel_2_def.rim_radius / wheel_2_def.tyre_radius
        );
}

void RigSpawner::CreateWheelVisuals(
    unsigned int wheel_index, 
    unsigned int node_base_index,
    unsigned int num_rays,
    Ogre::String const & rim_material_name,
    Ogre::String const & band_material_name,
    bool separate_rim,
    float rim_ratio
)
{
    SPAWNER_PROFILE_SCOPED();

    wheel_t & wheel = m_rig->wheels[wheel_index];
    vwheel_t & visual_wheel = m_rig->vwheels[wheel_index];

    try
    {
        const std::string wheel_mesh_name = this->ComposeName("WheelMesh", wheel_index);
        visual_wheel.meshwheel = false;
        visual_wheel.fm = new FlexMesh(
            wheel_mesh_name,
            m_rig->ar_nodes,
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
        Ogre::Entity *ec = gEnv->sceneManager->createEntity(instance_name, wheel_mesh_name);
        this->SetupNewEntity(ec, Ogre::ColourValue(0, 0.5, 0.5));
        visual_wheel.cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        m_rig->deletion_Entities.emplace_back(ec);
        visual_wheel.cnode->attachObject(ec);
    }
    catch (Ogre::Exception& e)
    {
        AddMessage(Message::TYPE_ERROR, "Failed to create wheel visuals: " +  e.getFullDescription());
    }
}

unsigned int RigSpawner::AddWheelBeam(
    node_t *node_1, 
    node_t *node_2, 
    float spring, 
    float damping, 
    std::shared_ptr<RigDef::BeamDefaults> beam_defaults,
    float max_contraction,   /* Default: -1.f */
    float max_extension,     /* Default: -1.f */
    int type                 /* Default: BEAM_INVISIBLE */
)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int index = m_rig->ar_num_beams;
    beam_t & beam = AddBeam(*node_1, *node_2, beam_defaults, DEFAULT_DETACHER_GROUP); 
    beam.bm_type = type;
    beam.k = spring;
    beam.d = damping;
    if (max_contraction > 0.f)
    {
        beam.shortbound = max_contraction;
        beam.longbound = max_extension;
        beam.bounded = SHOCK1;
    }
    CalculateBeamLength(beam);

    if (type != BEAM_VIRTUAL)
    {
        /* Create visuals, but don't attach to scene-graph (compatibility with show-skeleton function) */
        CreateBeamVisuals(beam, index, beam_defaults);
    }

    return index;
}

unsigned int RigSpawner::AddWheelRimBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int beam_index = _SectionWheels2AddBeam(wheel_2_def, node_1, node_2);
    beam_t & beam = GetBeam(beam_index);
    beam.k = wheel_2_def.rim_springiness;
    beam.d = wheel_2_def.rim_damping;
    return beam_index;
}

unsigned int RigSpawner::AddTyreBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int beam_index = _SectionWheels2AddBeam(wheel_2_def, node_1, node_2);
    beam_t & beam = GetBeam(beam_index);
    beam.k = wheel_2_def.tyre_springiness;
    beam.d = wheel_2_def.tyre_damping;

    m_rig->pressure_beams[m_rig->free_pressure_beam] = beam_index;
    m_rig->free_pressure_beam++;

    return beam_index;
}

unsigned int RigSpawner::_SectionWheels2AddBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int index = m_rig->ar_num_beams;
    beam_t & beam = GetFreeBeam();
    InitBeam(beam, node_1, node_2);
    beam.bm_type = BEAM_INVISIBLE;
    SetBeamStrength(beam, wheel_2_def.beam_defaults->breaking_threshold);
    SetBeamDeformationThreshold(beam, wheel_2_def.beam_defaults);
    return index;
}

void RigSpawner::ProcessWheel2(RigDef::Wheel2 & def)
{
    SPAWNER_PROFILE_SCOPED();

    unsigned int node_base_index = m_rig->ar_num_nodes;
    unsigned int wheel_index = AddWheel2(def);
    CreateWheelVisuals(wheel_index, def, node_base_index);
};

void RigSpawner::ProcessWheel(RigDef::Wheel & def)
{
    AddWheel(def);
};

void RigSpawner::ProcessWheelDetacher(RigDef::WheelDetacher & def)
{
    SPAWNER_PROFILE_SCOPED();

    if (def.wheel_id > m_rig->free_wheel - 1)
    {
        AddMessage(Message::TYPE_ERROR, std::string("Invalid wheel_id: ") + TOSTRING(def.wheel_id));
        return;
    }

    m_rig->wheels[def.wheel_id].wh_detacher_group = def.detacher_group;
};

void RigSpawner::ProcessSlopeBrake(RigDef::SlopeBrake & def)
{
    SPAWNER_PROFILE_SCOPED();

    // #1: regulating_force
    float force = def.regulating_force;
    if (force < 0.f || force > 20.f)
    {
        std::stringstream msg;
        msg << "Clamping 'regulating_force' value '" << force << "' to allowed range <0 - 20>";
        AddMessage(Message::TYPE_INFO, msg.str());
        force = (force < 0.f) ? 0.f : 20.f;
    }
    m_rig->slopeBrakeFactor = force;

    // #2: attach_angle
    float attach_angle = def.attach_angle;
    if (attach_angle < 1.f || attach_angle > 45.f)
    {
        std::stringstream msg;
        msg << "Clamping 'attach_angle' value '" << force << "' to allowed range <1 - 45>";
        AddMessage(Message::TYPE_INFO, msg.str());
        attach_angle = (attach_angle < 1.f) ? 1.f : 45.f;
    }
    m_rig->slopeBrakeAttAngle = attach_angle;

    // #3: release_angle
    float release_angle = def.release_angle;
    if (release_angle < 1.f || release_angle > 45.f)
    {
        std::stringstream msg;
        msg << "Clamping 'release_angle' value '" << force << "' to allowed range <1 - 45>";
        AddMessage(Message::TYPE_INFO, msg.str());
        release_angle = (release_angle < 1.f) ? 1.f : 45.f;
    }
    m_rig->slopeBrakeRelAngle = release_angle + attach_angle;

    // Flag
    m_rig->has_slope_brake = true;
};

void RigSpawner::ProcessTractionControl(RigDef::TractionControl & def)
{
    SPAWNER_PROFILE_SCOPED();

    /* #1: regulating_force */
    float force = def.regulation_force;
    if (force < 0.f || force > 20.f)
    {
        std::stringstream msg;
        msg << "Clamping 'regulating_force' value '" << force << "' to allowed range <0 - 20>";
        AddMessage(Message::TYPE_INFO, msg.str());
        force = (force < 0.f) ? 0.f : 20.f;
    }
    m_rig->tc_ratio = force;

    /* #2: wheelslip */
    m_rig->tc_wheelslip = (def.wheel_slip < 0.f) ? 0.f : def.wheel_slip;

    /* #3: fade_speed */
    m_rig->tc_fade = (def.fade_speed < 0.1f) ? 0.1f : def.fade_speed;

    /* #4: pulse/sec */
    float pulse = def.pulse_per_sec;
    if (pulse <= 1.0f || pulse >= 2000.0f)
    {
        pulse = 2000.0f;
    } 
    m_rig->tc_pulse_time = 1 / pulse;

    /* #4: mode */
    m_rig->tc_mode = static_cast<int>(def.attr_is_on);
    m_rig->tc_present = def.attr_is_on;
    m_rig->tc_notoggle = def.attr_no_toggle;
    if (def.attr_no_dashboard) { m_rig->tc_present = false; } // Override
};

void RigSpawner::ProcessAntiLockBrakes(RigDef::AntiLockBrakes & def)
{
    SPAWNER_PROFILE_SCOPED();

    /* #1: regulating_force */
    float force = def.regulation_force;
    if (force < 0.f || force > 20.f)
    {
        std::stringstream msg;
        msg << "Clamping 'regulating_force' value '" << force << "' to allowed range <0 - 20>";
        AddMessage(Message::TYPE_INFO, msg.str());
        force = (force < 0.f) ? 0.f : 20.f;
    }
    m_rig->alb_ratio = force;

    /* #2: min_speed */
    /* Wheelspeed adaption: 60 sec * 60 mins / 1000(kilometer) = 3.6 to get meter per sec */
    float min_speed = def.min_speed / 3.6f;
    m_rig->alb_minspeed = std::max(0.5f, min_speed);

    /* #3: pulse_per_sec */
    float pulse = def.pulse_per_sec;
    if (pulse <= 1.0f || pulse >= 2000.0f)
    {
        pulse = 2000.0f;
    } 
    m_rig->alb_pulse_time = 1 / pulse;

    /* #4: mode */
    m_rig->alb_mode = static_cast<int>(def.attr_is_on);
    m_rig->alb_present = def.attr_is_on;
    m_rig->alb_notoggle = def.attr_no_toggle;
    if (def.attr_no_dashboard) { m_rig->alb_present = false; } // Override
}

void RigSpawner::ProcessBrakes(RigDef::Brakes & def)
{
    SPAWNER_PROFILE_SCOPED();

    m_rig->ar_brake_force = def.default_braking_force;
    m_rig->m_handbrake_force = 2.f * m_rig->ar_brake_force;
    if (def.parking_brake_force != -1.f)
    {
        m_rig->m_handbrake_force = def.parking_brake_force;
    }
};

void RigSpawner::ProcessEngturbo(RigDef::Engturbo & def)
{
    /* Is this a land vehicle? */
    if (m_rig->ar_engine == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, "Section 'engturbo' found but no engine defined. Skipping ...");
        return;
    }
    
        /* Find it */
    std::shared_ptr<RigDef::Engturbo> engturbo;
    std::list<std::shared_ptr<RigDef::File::Module>>::iterator module_itor = m_selected_modules.begin();
    for (; module_itor != m_selected_modules.end(); module_itor++)
    {
        if (module_itor->get()->engturbo != nullptr)
        {
            engturbo = module_itor->get()->engturbo;
        }
    }
    
        /* Process it */
    m_rig->ar_engine->setTurboOptions(engturbo->version, engturbo->tinertiaFactor, engturbo->nturbos, engturbo->param1, engturbo->param2, engturbo->param3, engturbo->param4, engturbo->param5, engturbo->param6, engturbo->param7, engturbo->param8, engturbo->param9, engturbo->param10, engturbo->param11);
};

void RigSpawner::ProcessEngoption(RigDef::Engoption & def)
{
    SPAWNER_PROFILE_SCOPED();

    /* Is this a land vehicle? */
    if (m_rig->ar_engine == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, "Section 'engoption' found but no engine defined. Skipping ...");
        return;
    }

    /* Find it */
    std::shared_ptr<RigDef::Engoption> engoption;
    std::list<std::shared_ptr<RigDef::File::Module>>::iterator module_itor = m_selected_modules.begin();
    for (; module_itor != m_selected_modules.end(); module_itor++)
    {
        if (module_itor->get()->engoption != nullptr)
        {
            engoption = module_itor->get()->engoption;
        }
    }

    /* Process it */
    m_rig->ar_engine->setOptions(
        engoption->inertia,
        engoption->type,
        engoption->clutch_force,
        engoption->shift_time,
        engoption->clutch_time,
        engoption->post_shift_time,
        engoption->idle_rpm,
        engoption->stall_rpm,
        engoption->max_idle_mixture,
        engoption->min_idle_mixture
    );
};

void RigSpawner::ProcessEngine(RigDef::Engine & def)
{
    SPAWNER_PROFILE_SCOPED();

    /* Process it */
    m_rig->ar_driveable = TRUCK;

    /* Process gear list to BeamEngine-compatible format */
    /* TODO: Move this to BeamEngine::BeamEngine() */
    std::vector<float> gears_compat;
    gears_compat.reserve(2 + def.gear_ratios.size());
    gears_compat.push_back(def.reverse_gear_ratio);
    gears_compat.push_back(def.neutral_gear_ratio);
    std::vector<float>::iterator itor = def.gear_ratios.begin();
    for (; itor < def.gear_ratios.end(); itor++)
    {
        gears_compat.push_back(*itor);
    }

    m_rig->ar_engine = new BeamEngine(
        def.shift_down_rpm,
        def.shift_up_rpm,
        def.torque,
        gears_compat,
        def.global_gear_ratio,
        m_rig
    );

    m_rig->ar_engine->setAutoMode(App::sim_gearbox_mode.GetActive());
};

void RigSpawner::ProcessHelp()
{
    SPAWNER_PROFILE_SCOPED();

    SetCurrentKeyword(RigDef::File::KEYWORD_HELP);
    unsigned int material_count = 0;

    std::list<std::shared_ptr<RigDef::File::Module>>::iterator module_itor = m_selected_modules.begin();
    for (; module_itor != m_selected_modules.end(); module_itor++)
    {
        auto module = module_itor->get();
        if (! module->help_panel_material_name.empty())
        {
            m_rig->ar_help_panel_material = module->help_panel_material_name;
            material_count++;
        }
    }

    if (material_count > 1)
    {
        std::stringstream msg;
        msg << "Multiple (" << material_count << ") 'help' sections found. Using the last one...";
        AddMessage(Message::TYPE_WARNING, msg.str());	
    }

    SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);
};

void RigSpawner::ProcessAuthors()
{
    SPAWNER_PROFILE_SCOPED();

    SetCurrentKeyword(RigDef::File::KEYWORD_FILEFORMATVERSION);

    std::vector<RigDef::Author>::iterator author_itor = m_file->authors.begin();
    for (; author_itor != m_file->authors.end(); author_itor++)
    {
        authorinfo_t author;
        author.type = author_itor->type;
        author.name = author_itor->name;
        author.email = author_itor->email;
        if (author_itor->_has_forum_account)
        {
            author.id = author_itor->forum_account_id;
        }
        m_rig->authors.push_back(author);
    }	

    SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);
};

unsigned int RigSpawner::GetNodeIndexOrThrow(RigDef::Node::Ref const & node_ref)
{
    SPAWNER_PROFILE_SCOPED();

    std::pair<unsigned int, bool> result = GetNodeIndex(node_ref);
    if (! result.second)
    {
        std::stringstream msg;
        msg << "Failed to retrieve required node: " << node_ref.ToString();
        throw Exception(msg.str());
    }
    return result.first;
}

node_t & RigSpawner::GetNodeOrThrow(RigDef::Node::Ref const & node_ref)
{
    SPAWNER_PROFILE_SCOPED();

    return m_rig->ar_nodes[GetNodeIndexOrThrow(node_ref)];
}

void RigSpawner::ProcessCamera(RigDef::Camera & def)
{
    SPAWNER_PROFILE_SCOPED();

    /* Center node */
    if (def.center_node.IsValidAnyState())
    {
        m_rig->ar_camera_node_pos[m_rig->ar_num_cameras] 
            = static_cast<int>(GetNodeIndexOrThrow(def.center_node));
    }
    else
    {
        m_rig->ar_camera_node_pos[m_rig->ar_num_cameras] = -1;
    }

    /* Direction node */
    if (def.back_node.IsValidAnyState())
    {
        m_rig->ar_camera_node_dir[m_rig->ar_num_cameras] 
            = static_cast<int>(GetNodeIndexOrThrow(def.back_node));
    }
    else
    {
        m_rig->ar_camera_node_dir[m_rig->ar_num_cameras] = -1;
    }

    /* Roll node */
    if (def.left_node.IsValidAnyState())
    {
        m_rig->ar_camera_node_roll[m_rig->ar_num_cameras] 
            = static_cast<int>(GetNodeIndexOrThrow(def.left_node));
    }
    else
    {
        m_rig->ar_camera_node_roll[m_rig->ar_num_cameras] = -1;
    }

    /* Advance */
    m_rig->ar_num_cameras++;
};

node_t* RigSpawner::GetBeamNodePointer(RigDef::Node::Ref const & node_ref)
{
    SPAWNER_PROFILE_SCOPED();

    node_t* node = GetNodePointer(node_ref);
    if (node != nullptr)
    {
        return node;
    }
    return nullptr;
}

void RigSpawner::ProcessBeam(RigDef::Beam & def)
{
    SPAWNER_PROFILE_SCOPED();

    // Nodes
    node_t* ar_nodes[] = {nullptr, nullptr};
    ar_nodes[0] = GetBeamNodePointer(def.nodes[0]);
    if (ar_nodes[0] == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, std::string("Ignoring beam, could not find node: ") + def.nodes[0].ToString());
        return;
    }
    ar_nodes[1] = GetBeamNodePointer(def.nodes[1]);
    if (ar_nodes[1] == nullptr)
    {
        AddMessage(Message::TYPE_WARNING, std::string("Ignoring beam, could not find node: ") + def.nodes[1].ToString());
        return;
    }

    // Beam
    int beam_index = m_rig->ar_num_beams;
    beam_t & beam = AddBeam(*ar_nodes[0], *ar_nodes[1], def.defaults, def.detacher_group);
    beam.bm_type = BEAM_NORMAL;
    beam.k = def.defaults->GetScaledSpringiness();
    beam.d = def.defaults->GetScaledDamping();
    beam.bounded = NOSHOCK; // Orig: if (shortbound) ... hardcoded in BTS_BEAMS

    /* Calculate length */
    // orig = precompression hardcoded to 1
    CalculateBeamLength(beam);

    /* Strength */
    float beam_strength = def.defaults->GetScaledBreakingThreshold();
    beam.strength  = beam_strength;

    /* Options */
    if (BITMASK_IS_1(def.options, RigDef::Beam::OPTION_i_INVISIBLE))
    {
        beam.bm_type = BEAM_INVISIBLE;
    }
    if (BITMASK_IS_1(def.options, RigDef::Beam::OPTION_r_ROPE))
    {
        beam.bounded = ROPE;
    }
    if (BITMASK_IS_1(def.options, RigDef::Beam::OPTION_s_SUPPORT))
    {
        beam.bounded = SUPPORTBEAM;
        beam.longbound = def.extension_break_limit;
    }

    CreateBeamVisuals(beam, beam_index, def.defaults);
}

void RigSpawner::SetBeamDeformationThreshold(beam_t & beam, std::shared_ptr<RigDef::BeamDefaults> beam_defaults)
{
    SPAWNER_PROFILE_SCOPED();

    /*
    ---------------------------------------------------------------------------
        Old parser logic
    ---------------------------------------------------------------------------

    VAR default_deform              = BEAM_DEFORM (400,000)
    VAR default_deform_scale        = 1
    VAR beam_creak                  = BEAM_CREAK_DEFAULT (100,000)
    VAR enable_advanced_deformation = false


    add_beam()
        IF default_deform < beam_creak
            default_deform = beam_creak
        END IF

        VAR beam;
        beam.default_deform = default_deform * default_deform_scale
    END

    
    enable_advanced_deformation:
        READ enable_advanced_deformation


    set_beam_defaults:
        READ default_deform
        VAR  default_deform_user_defined
        READ default_deform_scale
        VAR  plastic_coef_user_defined

        IF (!enable_advanced_deformation && default_deform < BEAM_DEFORM)
           default_deform = BEAM_DEFORM;
        END IF

        IF (plastic_coef_user_defined)
            beam_creak = 0
        END IF
  
    ---------------------------------------------------------------------------
        TruckParser2013
    ---------------------------------------------------------------------------    

    VAR beam_defaults
    {
        default_deform                = BEAM_DEFORM
        scale.default_deform          = 1
        _enable_advanced_deformation  = false
        _user_defined                 = false
        _default_deform_set           = false
        _plastic_coef_user_defined    = false
    }


    set_beam_defaults:
        READ beam_defaults


    add_beam:

        // Init

        VAR default_deform = BEAM_DEFORM;
        VAR beam_creak = BEAM_CREAK_DEFAULT;

        // Old 'set_beam_defaults'

        IF (beam_defaults._is_user_defined)

            default_deform = beam_defaults.default_deform
            IF (!beam_defaults._enable_advanced_deformation && default_deform < BEAM_DEFORM)
               default_deform = BEAM_DEFORM;
            END IF

            IF (beam_defaults._plastic_coef_user_defined && beam_defaults.plastic_coef >= 0)
                beam_creak = 0
            END IF

        END IF

        // Old 'add_beam'

        IF default_deform < beam_creak
            default_deform = beam_creak
        END IF

        VAR beam;
        beam.default_deform = default_deform * beam_defaults.scale.default_deform
    
    ---------------------------------------------------------------------------
    */

    // Old init
    float default_deform = BEAM_DEFORM; 
    float beam_creak = BEAM_CREAK_DEFAULT;

    // Old 'set_beam_defaults'
    if (beam_defaults->_is_user_defined)
    {
        default_deform = beam_defaults->deformation_threshold;
        if (!beam_defaults->_enable_advanced_deformation && default_deform < BEAM_DEFORM)
        {
            default_deform = BEAM_DEFORM;
        }

        if (beam_defaults->_is_plastic_deform_coef_user_defined && beam_defaults->plastic_deform_coef >= 0.f)
        {
            beam_creak = 0.f;
        }
    }

    // Old 'add_beam'
    if (default_deform < beam_creak)
    {
        default_deform = beam_creak;
    }

    float deformation_threshold = default_deform * beam_defaults->scale.deformation_threshold_constant;

    beam.minmaxposnegstress = deformation_threshold;
    beam.maxposstress       = deformation_threshold;
    beam.maxnegstress       = -(deformation_threshold);
}

void RigSpawner::CreateBeamVisuals(beam_t & beam, int beam_index, std::shared_ptr<RigDef::BeamDefaults> beam_defaults)
{
    SPAWNER_PROFILE_SCOPED();

    try
    {
        beam.mEntity = gEnv->sceneManager->createEntity(this->ComposeName("Beam", beam_index), "beam.mesh");
    }
    catch (...)
    {
        throw Exception("Failed to load file 'beam.mesh' (should come with RoR installation)");
    }
    m_rig->deletion_Entities.push_back(beam.mEntity);
    beam.mSceneNode = m_rig->m_beam_visuals_parent_scenenode->createChildSceneNode();
    beam.mSceneNode->setScale(beam.diameter, -1, beam.diameter);
    if (beam.bm_type == BEAM_HYDRO)
    {
        beam.mEntity->setMaterialName("tracks/Chrome");
    }
    else
    {
        beam.mEntity->setMaterialName(beam_defaults->beam_material_name);
    }
}

void RigSpawner::CalculateBeamLength(beam_t & beam)
{
    SPAWNER_PROFILE_SCOPED();

    float beam_length = (beam.p1->RelPosition - beam.p2->RelPosition).length();
    beam.L = beam_length;
    beam.Lhydro = beam_length;
    beam.refL = beam_length;
}

void RigSpawner::InitBeam(beam_t & beam, node_t *node_1, node_t *node_2)
{
    SPAWNER_PROFILE_SCOPED();

    beam.p1 = node_1;
    beam.p2 = node_2;

    /* Length */
    CalculateBeamLength(beam);
}

void RigSpawner::AddMessage(RigSpawner::Message::Type type,	Ogre::String const & text)
{
    SPAWNER_PROFILE_SCOPED();

    /* Add message to report */
    m_messages.push_back(Message(type, text, m_current_keyword));

    /* Log message immediately (to put other log messages in context) */
    std::stringstream report;
    report << " == RigSpawner: ";
    switch (type)
    {
        case (RigSpawner::Message::TYPE_INTERNAL_ERROR): 
            report << "INTERNAL ERROR"; 
            ++m_messages_num_errors;
            break;

        case (RigSpawner::Message::TYPE_ERROR):
            report << "ERROR";
            ++m_messages_num_errors; 
            break;

        case (RigSpawner::Message::TYPE_WARNING):
            report << "WARNING";
            ++m_messages_num_warnings; 
            break;

        default:
            report << "INFO";
            ++m_messages_num_other;
            break;
    }
    report << " (Keyword " << RigDef::File::KeywordToString(m_current_keyword) << ") " << text;
    Ogre::LogManager::getSingleton().logMessage(report.str());
}

std::pair<unsigned int, bool> RigSpawner::GetNodeIndex(RigDef::Node::Ref const & node_ref, bool quiet /* Default: false */)
{
    SPAWNER_PROFILE_SCOPED();

    if (!node_ref.IsValidAnyState())
    {
        if (! quiet)
        {
            AddMessage(Message::TYPE_ERROR, std::string("Attempt to resolve invalid node reference: ") + node_ref.ToString());
        }
        return std::make_pair(0, false);
    }
    bool is_imported = node_ref.GetImportState_IsValid();
    bool is_named = (is_imported ? node_ref.GetImportState_IsResolvedNamed() : node_ref.GetRegularState_IsNamed());
    if (is_named)
    {
        auto result = m_named_nodes.find(node_ref.Str());
        if (result != m_named_nodes.end())
        {
            return std::make_pair(result->second, true);
        }
        else if (! quiet)
        {
            std::stringstream msg;
            msg << "Failed to resolve node-ref (node not found):" << node_ref.ToString();
            AddMessage(Message::TYPE_ERROR, msg.str());
        }
        return std::make_pair(0, false);
    }
    else
    {
        // Imported nodes pass without check
        if (!is_imported && (node_ref.Num() >= static_cast<unsigned int>(m_rig->ar_num_nodes)))
        {
            if (! quiet)
            {
                std::stringstream msg;
                msg << "Failed to resolve node-ref (node index too big, node count is: "<<m_rig->ar_num_nodes<<"): " << node_ref.ToString();
                AddMessage(Message::TYPE_ERROR, msg.str());
            }
            return std::make_pair(0, false);
        }
        return std::make_pair(node_ref.Num(), true);
    }
}

node_t* RigSpawner::GetNodePointer(RigDef::Node::Ref const & node_ref)
{
    SPAWNER_PROFILE_SCOPED();

    std::pair<unsigned int, bool> result = GetNodeIndex(node_ref);
    if (result.second)
    {
        return & m_rig->ar_nodes[result.first];
    }
    else
    {
        return nullptr;
    }
}

node_t* RigSpawner::GetNodePointerOrThrow(RigDef::Node::Ref const & node_ref)
{
    SPAWNER_PROFILE_SCOPED();

    node_t *node = GetNodePointer(node_ref);
    if (node == nullptr)
    {
        std::stringstream msg;
        msg << "Required node not found: " << node_ref.ToString();
        throw Exception(msg.str());
    }
    return node;
}

std::pair<unsigned int, bool> RigSpawner::AddNode(RigDef::Node::Id & id)
{
    SPAWNER_PROFILE_SCOPED();

    if (!id.IsValid())
    {
        std::stringstream msg;
        msg << "Attempt to add node with 'INVALID' flag: " << id.ToString() << " (number of nodes at this point: " << m_rig->ar_num_nodes << ")";
        this->AddMessage(Message::TYPE_ERROR, msg.str());
        return std::make_pair(0, false);
    }

    if (id.IsTypeNamed())
    {
        unsigned int new_index = static_cast<unsigned int>(m_rig->ar_num_nodes);
        auto insert_result = m_named_nodes.insert(std::make_pair(id.Str(), new_index));
        if (! insert_result.second)
        {
            std::stringstream msg;
            msg << "Ignoring named node! Duplicate name: " << id.Str() << " (number of nodes at this point: " << m_rig->ar_num_nodes << ")";
            this->AddMessage(Message::TYPE_ERROR, msg.str());
            return std::make_pair(0, false);
        }
        m_rig->ar_num_nodes++;
        return std::make_pair(new_index, true);
    }
    if (id.IsTypeNumbered())
    {
        if (id.Num() < static_cast<unsigned int>(m_rig->ar_num_nodes))
        {
            std::stringstream msg;
            msg << "Duplicate node number, previous definition will be overriden! - " << id.ToString() << " (number of nodes at this point: " << m_rig->ar_num_nodes << ")";
            this->AddMessage(Message::TYPE_WARNING, msg.str());
        }
        unsigned int new_index = static_cast<unsigned int>(m_rig->ar_num_nodes);
        m_rig->ar_num_nodes++;
        return std::make_pair(new_index, true);
    }
    // Invalid node ID without type flag!
    throw Exception("Invalid Node::Id without type flags!");
}

void RigSpawner::ProcessNode(RigDef::Node & def)
{
    SPAWNER_PROFILE_SCOPED();

    std::pair<unsigned int, bool> inserted_node = AddNode(def.id);
    if (! inserted_node.second)
    {
        return;
    }

    node_t & node = m_rig->ar_nodes[inserted_node.first];
    node.pos = inserted_node.first; /* Node index */
    node.id = static_cast<int>(def.id.Num());

    /* Positioning */
    Ogre::Vector3 node_position = m_spawn_position + def.position;
    node.AbsPosition = node_position; 
    node.RelPosition = node_position - m_rig->ar_origin;
        
    node.wetstate = DRY; // orig = hardcoded (init_node)
    node.iswheel = NOWHEEL;
    node.wheelid = -1; // Hardcoded in orig (bts_nodes, call to init_node())
    node.friction_coef = def.node_defaults->friction;
    node.volume_coef = def.node_defaults->volume;
    node.surface_coef = def.node_defaults->surface;
    node.collisionBoundingBoxID = -1; // orig = hardcoded (init_node)

    /* Mass */
    if (def.node_defaults->load_weight >= 0.f) // The >= operator is in orig.
    {
        // orig = further override of hardcoded default.
        node.mass = def.node_defaults->load_weight; 
        node.overrideMass = true;
        node.loadedMass = true;
    }
    else
    {
        node.mass = 10; // Hardcoded in original (bts_nodes, call to init_node())
        node.loadedMass = false;
    }

    /* Lockgroup */
    node.lockgroup = (m_file->lockgroup_default_nolock) ? RigDef::Lockgroup::LOCKGROUP_NOLOCK : RigDef::Lockgroup::LOCKGROUP_DEFAULT;

    /* Options */
    unsigned int options = def.options | def.node_defaults->options; /* Merge bit flags */
    if (BITMASK_IS_1(options, RigDef::Node::OPTION_l_LOAD_WEIGHT))
    {
        node.loadedMass = true;
        if (def._has_load_weight_override)
        {
            node.overrideMass = true;
            node.mass = def.load_weight_override;
        }
        else
        {
            m_rig->m_masscount++;
        }
    }
    if (BITMASK_IS_1(options, RigDef::Node::OPTION_h_HOOK_POINT))
    {
        /* Link [current-node] -> [node-0] */
        /* If current node is 0, link [node-0] -> [node-1] */
        node_t & node_2 = (node.pos == 0) ? GetNode(1) : GetNode(0);
        unsigned int beam_index = m_rig->ar_num_beams;

        beam_t & beam = AddBeam(node, node_2, def.beam_defaults, def.detacher_group);
        SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold() * 100.f);
        beam.bm_type           = BEAM_HYDRO;
        beam.d                 = def.beam_defaults->GetScaledDamping() * 0.1f;
        beam.k                 = def.beam_defaults->GetScaledSpringiness();
        beam.bounded           = ROPE;
        beam.bm_disabled       = true;
        beam.L                 = HOOK_RANGE_DEFAULT;
        beam.Lhydro            = HOOK_RANGE_DEFAULT;
        beam.refL              = HOOK_RANGE_DEFAULT;
        beam.commandRatioShort = HOOK_SPEED_DEFAULT;
        beam.commandRatioLong  = HOOK_SPEED_DEFAULT;
        beam.commandShort      = 0.0f;
        beam.commandLong       = 1.0f;
        beam.maxtiestress      = HOOK_FORCE_DEFAULT;
        SetBeamDeformationThreshold(beam, def.beam_defaults);
        CreateBeamVisuals(beam, beam_index, def.beam_defaults);
            
        // Logic cloned from SerializedRig.cpp, section BTS_NODES
        hook_t hook;
        hook.hookNode          = & node;
        hook.group             = -1;
        hook.locked            = UNLOCKED;
        hook.lockNode          = 0;
        hook.lockTruck         = 0;
        hook.lockNodes         = true;
        hook.lockgroup         = -1;
        hook.beam              = & beam;
        hook.maxforce          = HOOK_FORCE_DEFAULT;
        hook.lockrange         = HOOK_RANGE_DEFAULT;
        hook.lockspeed         = HOOK_SPEED_DEFAULT;
        hook.selflock          = false;
        hook.nodisable         = false;
        hook.timer             = 0.0f;
        hook.timer_preset      = HOOK_LOCK_TIMER_DEFAULT;
        hook.autolock          = false;
        m_rig->hooks.push_back(hook);
    }
    AdjustNodeBuoyancy(node, def, def.node_defaults);
    node.contactless       = BITMASK_IS_1(options, RigDef::Node::OPTION_c_NO_GROUND_CONTACT);
    node.disable_particles = BITMASK_IS_1(options, RigDef::Node::OPTION_p_NO_PARTICLES);
    node.disable_sparks    = BITMASK_IS_1(options, RigDef::Node::OPTION_f_NO_SPARKS);
    node.no_mouse_grab     = BITMASK_IS_1(options, RigDef::Node::OPTION_m_NO_MOUSE_GRAB);

    m_rig->ar_exhaust_dir_node        = BITMASK_IS_1(options, RigDef::Node::OPTION_y_EXHAUST_DIRECTION) ? node.pos : 0;
    m_rig->ar_exhaust_pos_node         = BITMASK_IS_1(options, RigDef::Node::OPTION_x_EXHAUST_POINT) ? node.pos : 0;

    // Update "fusedrag" autocalc y & z span
    if (def.position.z < m_fuse_z_min) { m_fuse_z_min = def.position.z; }
    if (def.position.z > m_fuse_z_max) { m_fuse_z_max = def.position.z; }
    if (def.position.y < m_fuse_y_min) { m_fuse_y_min = def.position.y; }
    if (def.position.y > m_fuse_y_max) { m_fuse_y_max = def.position.y; }

#ifdef DEBUG_TRUCKPARSER2013
    // DEBUG
    std::stringstream msg;
    msg<<"DBG ProcessNode() pos="<<node.pos<<", id="<<node.id << ", X=" << node.AbsPosition.x << ", Y=" << node.AbsPosition.y << ", Z=" << node.AbsPosition.z;
    LOG(msg.str());
    // END
#endif
}

void RigSpawner::AddExhaust(
        unsigned int emitter_node_idx,
        unsigned int direction_node_idx,
        bool old_format,
        Ogre::String *user_material_name
    )
{
    SPAWNER_PROFILE_SCOPED();

    if (m_rig->disable_smoke)
    {
        return;
    }
    exhaust_t exhaust;
    exhaust.emitterNode = emitter_node_idx;
    exhaust.directionNode = direction_node_idx;
    exhaust.isOldFormat = old_format;

    Ogre::String material_name;
    if (user_material_name != nullptr)
    {
        material_name = *user_material_name;
    }
    else
    {
        material_name = "tracks/Smoke";
    }

    if (m_rig->m_used_skin != nullptr)
    {
        auto search = m_rig->m_used_skin->replace_materials.find(material_name);
        if (search != m_rig->m_used_skin->replace_materials.end())
        {
            material_name = search->second;
        }
    }

    exhaust.smoker = gEnv->sceneManager->createParticleSystem(this->ComposeName("Exhaust", static_cast<int>(m_rig->exhausts.size())), material_name);
    if (exhaust.smoker == nullptr)
    {
        AddMessage(Message::TYPE_INTERNAL_ERROR, "Failed to create exhaust");
        return;
    }
    exhaust.smoker->setVisibilityFlags(DEPTHMAP_DISABLED); // Disable particles in depthmap

    
    exhaust.smokeNode = m_parent_scene_node->createChildSceneNode();
    exhaust.smokeNode->attachObject(exhaust.smoker);
    exhaust.smokeNode->setPosition(m_rig->ar_nodes[exhaust.emitterNode].AbsPosition);

    m_rig->ar_nodes[emitter_node_idx].isHot = true;
    m_rig->ar_nodes[emitter_node_idx].isHot = true;

    m_rig->exhausts.push_back(exhaust);
}

bool RigSpawner::AddModule(Ogre::String const & module_name)
{
    SPAWNER_PROFILE_SCOPED();

    auto result = m_file->user_modules.find(module_name);

    if (result != m_file->user_modules.end())
    {
        m_selected_modules.push_back(result->second);
        LOG(" == RigSpawner: Module added to configuration: " + module_name);
        return true;
    }
    this->AddMessage(Message::TYPE_WARNING, "Selected module not found: " + module_name);
    return false;
}

void RigSpawner::ProcessCinecam(RigDef::Cinecam & def)
{
    SPAWNER_PROFILE_SCOPED();

    // Node
    Ogre::Vector3 node_pos = m_spawn_position + def.position;
    node_t & camera_node = GetAndInitFreeNode(node_pos);
    camera_node.contactless = true; // Orig: hardcoded in BTS_CINECAM
    camera_node.wheelid = -1;
    camera_node.friction_coef = NODE_FRICTION_COEF_DEFAULT; // Node defaults are ignored here.
    camera_node.id = -1;
    AdjustNodeBuoyancy(camera_node, def.node_defaults);
    camera_node.volume_coef   = def.node_defaults->volume;
    camera_node.surface_coef  = def.node_defaults->surface;
    // NOTE: Not applying the 'node_mass' value here for backwards compatibility - this node must go through initial `Beam::calc_masses2()` pass with default weight.

    m_rig->ar_cinecam_node[m_rig->ar_num_cinecams] = camera_node.pos;
    m_rig->ar_num_cinecams++;

    // Beams
    for (unsigned int i = 0; i < 8; i++)
    {
        int beam_index = m_rig->ar_num_beams;
        beam_t & beam = AddBeam(camera_node, GetNode(def.nodes[i]), def.beam_defaults, DEFAULT_DETACHER_GROUP);
        beam.bm_type = BEAM_INVISIBLE;
        CalculateBeamLength(beam);
        beam.k = def.spring;
        beam.d = def.damping;
        CreateBeamVisuals(beam, beam_index, def.beam_defaults);
    }
};

void RigSpawner::InitNode(node_t & node, Ogre::Vector3 const & position)
{
    SPAWNER_PROFILE_SCOPED();

    /* Position */
    node.AbsPosition = position;
    node.RelPosition = position - m_rig->ar_origin;

    /* Misc. */
    node.collisionBoundingBoxID = -1; // orig = hardcoded (init_node)
    node.wetstate = DRY; // orig = hardcoded (init_node)
}

void RigSpawner::InitNode(
    node_t & node, 
    Ogre::Vector3 const & position,
    std::shared_ptr<RigDef::NodeDefaults> node_defaults
)
{
    SPAWNER_PROFILE_SCOPED();

    InitNode(node, position);
    node.friction_coef = node_defaults->friction;
    node.volume_coef = node_defaults->volume;
    node.surface_coef = node_defaults->surface;
}

void RigSpawner::ProcessGlobals(RigDef::Globals & def)
{
    SPAWNER_PROFILE_SCOPED();

    m_rig->m_dry_mass = def.dry_mass;
    m_rig->m_load_mass = def.cargo_mass;

    // NOTE: Don't do any material pre-processing here; it'll be done on actual entities (via `SetupNewEntity()`).
    if (! def.material_name.empty())
    {
        Ogre::MaterialPtr mat = RoR::OgreSubsystem::GetMaterialByName(def.material_name); // Check if exists (compatibility)
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

bool RigSpawner::CheckHydroLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->ar_num_shocks + count) > MAX_HYDROS)
    {
        std::stringstream msg;
        msg << "Hydro limit (" << MAX_HYDROS << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckParticleLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->free_cparticle + count) > MAX_CPARTICLES)
    {
        std::stringstream msg;
        msg << "Particle limit (" << MAX_CPARTICLES << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckAxleLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->m_num_axles + count) > MAX_WHEELS/2)
    {
        std::stringstream msg;
        msg << "Axle limit (" << MAX_WHEELS/2 << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckPropLimit(unsigned int count)
{	
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->ar_num_props + count) > MAX_PROPS)
    {
        std::stringstream msg;
        msg << "Prop limit (" << MAX_PROPS << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckFlexbodyLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->free_flexbody + count) > MAX_FLEXBODIES)
    {
        std::stringstream msg;
        msg << "Flexbody limit (" << MAX_FLEXBODIES << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckSubmeshLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_oldstyle_cab_submeshes.size() + count) > MAX_SUBMESHES)
    {
        std::stringstream msg;
        msg << "Submesh limit (" << MAX_SUBMESHES << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckTexcoordLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();
    
    if ((m_oldstyle_cab_texcoords.size() + count) > MAX_TEXCOORDS)
    {
        std::stringstream msg;
        msg << "Texcoord limit (" << MAX_TEXCOORDS << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

/* Static version */
bool RigSpawner::CheckSoundScriptLimit(Beam *vehicle, unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    //return CheckSoundScriptLimit(m_rig, count);
    if ((vehicle->free_soundsource + count) > MAX_SOUNDSCRIPTS_PER_TRUCK)
    {
        std::stringstream msg;
        msg << "SoundScript limit (" << MAX_SOUNDSCRIPTS_PER_TRUCK << ") exceeded";
        LOG(msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckCabLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->free_cab + count) > MAX_CABS)
    {
        std::stringstream msg;
        msg << "Cab limit (" << MAX_CABS << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckCameraRailLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->free_camerarail + count) > MAX_CAMERARAIL)
    {
        std::stringstream msg;
        msg << "CameraRail limit (" << MAX_CAMERARAIL << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckAirBrakeLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->free_airbrake + count) > MAX_AIRBRAKES)
    {
        std::stringstream msg;
        msg << "AirBrake limit (" << MAX_AIRBRAKES << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckAeroEngineLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->free_aeroengine + count) > MAX_AEROENGINES)
    {
        std::stringstream msg;
        msg << "AeroEngine limit (" << MAX_AEROENGINES << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

bool RigSpawner::CheckScrewpropLimit(unsigned int count)
{
    SPAWNER_PROFILE_SCOPED();

    if ((m_rig->free_screwprop + count) > MAX_SCREWPROPS)
    {
        std::stringstream msg;
        msg << "Screwprop limit (" << MAX_SCREWPROPS << ") exceeded";
        AddMessage(Message::TYPE_ERROR, msg.str());
        return false;
    }
    return true;
}

node_t &RigSpawner:: GetNode(unsigned int node_index)
{
    return m_rig->ar_nodes[node_index];
}

void RigSpawner::InitNode(unsigned int node_index, Ogre::Vector3 const & position)
{
    SPAWNER_PROFILE_SCOPED();

    InitNode(m_rig->ar_nodes[node_index], position);
}

beam_t & RigSpawner::GetBeam(unsigned int index)
{
    return m_rig->ar_beams[index];
}

node_t & RigSpawner::GetFreeNode()
{
    SPAWNER_PROFILE_SCOPED();

    node_t & node = m_rig->ar_nodes[m_rig->ar_num_nodes];
    node.pos = m_rig->ar_num_nodes;
    m_rig->ar_num_nodes++;
    return node;
}

beam_t & RigSpawner::GetFreeBeam()
{
    SPAWNER_PROFILE_SCOPED();

    beam_t & beam = m_rig->ar_beams[m_rig->ar_num_beams];
    m_rig->ar_num_beams++;
    return beam;
}

shock_t & RigSpawner::GetFreeShock()
{
    SPAWNER_PROFILE_SCOPED();

    shock_t & shock = m_rig->ar_shocks[m_rig->ar_num_shocks];
    m_rig->ar_num_shocks++;
    return shock;
}

beam_t & RigSpawner::GetAndInitFreeBeam(node_t & node_1, node_t & node_2)
{
    SPAWNER_PROFILE_SCOPED();

    beam_t & beam = GetFreeBeam();
    beam.p1 = & node_1;
    beam.p2 = & node_2;
    return beam;
}

node_t & RigSpawner::GetAndInitFreeNode(Ogre::Vector3 const & position)
{
    SPAWNER_PROFILE_SCOPED();

    node_t & node = GetFreeNode();
    InitNode(node, position);
    return node;
}

void RigSpawner::SetBeamSpring(beam_t & beam, float spring)
{
    beam.k = spring;
}

void RigSpawner::SetBeamDamping(beam_t & beam, float damping)
{
    beam.d = damping;
}

void RigSpawner::RecalculateBoundingBoxes(Beam *rig)
{
    SPAWNER_PROFILE_SCOPED();

    Ogre::Vector3 node_0_pos = rig->ar_nodes[0].AbsPosition;
    rig->boundingBox.setExtents(
        node_0_pos.x, node_0_pos.y, node_0_pos.z, 
        node_0_pos.x, node_0_pos.y, node_0_pos.z
    );
    rig->collisionBoundingBoxes.clear();

    for (int i=0; i < rig->ar_num_nodes; i++)
    {
        node_t & node = rig->ar_nodes[i];
        Ogre::Vector3 node_position = node.AbsPosition;
        rig->boundingBox.merge(node_position);
        if (node.collisionBoundingBoxID >= 0)
        {
            if ((unsigned int) node.collisionBoundingBoxID >= rig->collisionBoundingBoxes.size())
            {
                rig->collisionBoundingBoxes.push_back(
                    Ogre::AxisAlignedBox(
                        node_position.x, node_position.y, node_position.z, 
                        node_position.x, node_position.y, node_position.z
                    )
                );
            } 
            else
            {
                rig->collisionBoundingBoxes[node.collisionBoundingBoxID].merge(node_position);
            }
        }
    }

    for (unsigned int i = 0; i < rig->collisionBoundingBoxes.size(); i++)
    {
        rig->collisionBoundingBoxes[i].setMinimum(rig->collisionBoundingBoxes[i].getMinimum() - Ogre::Vector3(0.05f, 0.05f, 0.05f));
        rig->collisionBoundingBoxes[i].setMaximum(rig->collisionBoundingBoxes[i].getMaximum() + Ogre::Vector3(0.05f, 0.05f, 0.05f));
    }

    rig->boundingBox.setMinimum(rig->boundingBox.getMinimum() - Ogre::Vector3(0.05f, 0.05f, 0.05f));
    rig->boundingBox.setMaximum(rig->boundingBox.getMaximum() + Ogre::Vector3(0.05f, 0.05f, 0.05f));

    rig->predictedBoundingBox = rig->boundingBox;
    rig->predictedCollisionBoundingBoxes = rig->collisionBoundingBoxes;
}


void RigSpawner::SetupDefaultSoundSources(Beam *vehicle)
{
    SPAWNER_PROFILE_SCOPED();

    int trucknum = vehicle->ar_instance_id;
    int ar_exhaust_pos_node = vehicle->ar_exhaust_pos_node;

#ifdef USE_OPENAL
    if (SoundScriptManager::getSingleton().isDisabled()) 
    {
        return;
    }

    //engine
    if (vehicle->ar_engine != nullptr) /* Land vehicle */
    {
        if (vehicle->ar_engine->type=='t')
        {
            AddSoundSourceInstance(vehicle, "tracks/default_diesel", ar_exhaust_pos_node);
            AddSoundSourceInstance(vehicle, "tracks/default_force", ar_exhaust_pos_node);
            AddSoundSourceInstance(vehicle, "tracks/default_brakes", 0);
            AddSoundSourceInstance(vehicle, "tracks/default_parkbrakes", 0);
            AddSoundSourceInstance(vehicle, "tracks/default_reverse_beep", 0);
        }
        if (vehicle->ar_engine->type=='c')
            AddSoundSourceInstance(vehicle, "tracks/default_car", ar_exhaust_pos_node);
        if (vehicle->ar_engine->hasTurbo())
        {
            if (vehicle->ar_engine->turboInertiaFactor >= 3)
                AddSoundSourceInstance(vehicle, "tracks/default_turbo_big", ar_exhaust_pos_node);
            else if (vehicle->ar_engine->turboInertiaFactor <= 0.5)
                AddSoundSourceInstance(vehicle, "tracks/default_turbo_small", ar_exhaust_pos_node);
            else
                AddSoundSourceInstance(vehicle, "tracks/default_turbo_mid", ar_exhaust_pos_node);

            AddSoundSourceInstance(vehicle, "tracks/default_turbo_bov", ar_exhaust_pos_node);
            AddSoundSourceInstance(vehicle, "tracks/default_wastegate_flutter", ar_exhaust_pos_node);
        }

        if (vehicle->ar_engine->hasair)
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
    if (vehicle->alb_present)
    {
        AddSoundSourceInstance(vehicle, "tracks/default_antilock", 0);
    }
    //tractioncontrol
    if (vehicle->tc_present)
    {
        AddSoundSourceInstance(vehicle, "tracks/default_tractioncontrol", 0);
    }
    //screetch
    if ((vehicle->ar_driveable==TRUCK || vehicle->ar_driveable==AIRPLANE) && vehicle->free_wheel != 0)
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
    for (int i=0; i<vehicle->free_aeroengine && i<8; i++)
    {
        int turbojet_node = vehicle->aeroengines[i]->getNoderef();
        Ogre::String index_str = TOSTRING(i+1);

        if (vehicle->aeroengines[i]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOJET)
        {
            AddSoundSourceInstance(vehicle, "tracks/default_turbojet_start" + index_str, turbojet_node);
            AddSoundSourceInstance(vehicle, "tracks/default_turbojet_lopower" + index_str, turbojet_node);
            AddSoundSourceInstance(vehicle, "tracks/default_turbojet_hipower" + index_str, turbojet_node);
            if (((Turbojet*)(vehicle->aeroengines[i]))->afterburnable)
            {
                AddSoundSourceInstance(vehicle, "tracks/default_turbojet_afterburner" + index_str, turbojet_node);
            }
        }
        else if (vehicle->aeroengines[i]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
        {
            if (((Turboprop*)vehicle->aeroengines[i])->is_piston)
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
        AddSoundSource(vehicle, SoundScriptManager::getSingleton().createInstance(Ogre::String("tracks/linked/default_command/extend"), trucknum, NULL, SL_COMMAND, i), 0);
        AddSoundSource(vehicle, SoundScriptManager::getSingleton().createInstance(Ogre::String("tracks/linked/default_command/retract"), trucknum, NULL, SL_COMMAND, -i), 0);
    }

#endif //OPENAL
}

void RigSpawner::UpdateCollcabContacterNodes()
{
    SPAWNER_PROFILE_SCOPED();

    for (int i=0; i<m_rig->free_collcab; i++)
    {
        int tmpv = m_rig->collcabs[i] * 3;
        m_rig->ar_nodes[m_rig->cabs[tmpv]].contacter = true;
        m_rig->ar_nodes[m_rig->cabs[tmpv+1]].contacter = true;
        m_rig->ar_nodes[m_rig->cabs[tmpv+2]].contacter = true;
    }
}

std::string RigSpawner::ProcessMessagesToString()
{
    SPAWNER_PROFILE_SCOPED();

    std::stringstream report;

    auto itor = m_messages.begin();
    auto end  = m_messages.end();
    for (; itor != end; ++itor)
    {
        switch (itor->type)
        {
            case (Message::TYPE_INTERNAL_ERROR): 
                report << "#FF3300 INTERNAL ERROR #FFFFFF"; 
                break;

            case (Message::TYPE_ERROR): 
                report << "#FF3300 ERROR #FFFFFF"; 
                break;

            case (Message::TYPE_WARNING): 
                report << "#FFFF00 WARNING #FFFFFF"; 
                break;

            default:
                report << "INFO"; 
                break;
        }
        report << "(Keyword " << RigDef::File::KeywordToString(itor->keyword) << ")" << std::endl;
        report << "\t" << itor->text << std::endl;
    }
    return report.str();
}

RigDef::MaterialFlareBinding* RigSpawner::FindFlareBindingForMaterial(std::string const & material_name)
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

RigDef::VideoCamera* RigSpawner::FindVideoCameraByMaterial(std::string const & material_name)
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

Ogre::MaterialPtr RigSpawner::FindOrCreateCustomizedMaterial(std::string mat_lookup_name)
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
            lookup_entry.material = RoR::OgreSubsystem::GetMaterialByName("mirror")->clone(new_mat_name, true, m_custom_resource_group);
            // Special case - register under generated name. This is because all mirrors use the same material 'mirror'
            m_material_substitutions.insert(std::make_pair(new_mat_name, lookup_entry));
            return lookup_entry.material; // Done!
        }

        // Query 'videocameras'
        RigDef::VideoCamera* videocam_def = this->FindVideoCameraByMaterial(mat_lookup_name);
        if (videocam_def != nullptr)
        {
            Ogre::MaterialPtr video_mat_shared = RoR::OgreSubsystem::GetMaterialByName(mat_lookup_name);
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
        RigDef::MaterialFlareBinding* mat_flare_def = this->FindFlareBindingForMaterial(mat_lookup_name);
        if (mat_flare_def != nullptr)
        {
            lookup_entry.material_flare_def = mat_flare_def;
        }

        // Query SkinZip materials
        if (m_rig->m_used_skin != nullptr)
        {
            auto skin_res = m_rig->m_used_skin->replace_materials.find(mat_lookup_name);
            if (skin_res != m_rig->m_used_skin->replace_materials.end())
            {
                Ogre::MaterialPtr skin_mat = RoR::OgreSubsystem::GetMaterialByName(skin_res->second);
                if (!skin_mat.isNull())
                {
                    std::stringstream name_buf;
                    name_buf << videocam_def->material_name << ACTOR_ID_TOKEN << m_rig->ar_instance_id;
                    lookup_entry.material = skin_mat->clone(name_buf.str(), true, m_custom_resource_group);
                    m_material_substitutions.insert(std::make_pair(mat_lookup_name, lookup_entry));
                    return lookup_entry.material;
                }
                else
                {
                    std::stringstream buf;
                    buf << "Material '" << skin_res->second << "' from skin '" << m_rig->m_used_skin->name << "' not found! Ignoring it...";
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
            Ogre::MaterialPtr orig_mat = RoR::OgreSubsystem::GetMaterialByName(mat_lookup_name);
            if (orig_mat.isNull())
            {
                std::stringstream buf;
                buf << "Material doesn't exist:" << mat_lookup_name;
                this->AddMessage(Message::TYPE_ERROR, buf.str());
                return Ogre::MaterialPtr(); // NULL
            }

            std::stringstream name_buf;
            name_buf << orig_mat->getName() << ACTOR_ID_TOKEN << m_rig->ar_instance_id;
            lookup_entry.material = orig_mat->clone(name_buf.str(), true, m_custom_resource_group);
        }

        // Finally, query SkinZip textures
        if (m_rig->m_used_skin != nullptr)
        {
            RoR::SkinManager::ReplaceMaterialTextures(m_rig->m_used_skin, lookup_entry.material->getName());
        }

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

Ogre::MaterialPtr RigSpawner::CreateSimpleMaterial(Ogre::ColourValue color)
{
    assert(!m_simple_material_base.isNull());

    static size_t simple_mat_counter = 0;
    char name_buf[300];
    snprintf(name_buf, 300, "SimpleMaterial-%u%s%d", simple_mat_counter, ACTOR_ID_TOKEN, m_rig->ar_instance_id);
    Ogre::MaterialPtr newmat = m_simple_material_base->clone(name_buf);
    ++simple_mat_counter;
    newmat->getTechnique(0)->getPass(0)->setAmbient(color);

    return newmat;
}

void RigSpawner::SetupNewEntity(Ogre::Entity* ent, Ogre::ColourValue simple_color)
{
    // Use simple materials if applicable
    if (m_apply_simple_materials)
    {
        Ogre::MaterialPtr mat = this->CreateSimpleMaterial(simple_color);

        const unsigned short num_sub_entities = ent->getNumSubEntities();
        for (unsigned short i = 0; i < num_sub_entities; i++)
        {
            Ogre::SubEntity* subent = ent->getSubEntity(i);
            subent->setMaterial(mat);
        }

        return; // Done!
    }

    // Create unique sub-entity (=instance of submesh) materials
    unsigned short subent_max = ent->getNumSubEntities();
    for (unsigned short i = 0; i < subent_max; ++i)
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

void RigSpawner::FinalizeGfxSetup()
{
    // Check and warn if there are unclaimed managed materials
    // TODO &*&*

    // Create the actor
    m_rig->m_gfx_actor = std::unique_ptr<RoR::GfxActor>(new RoR::GfxActor(m_rig, m_custom_resource_group));

    // Process special materials
    for (auto& entry: m_material_substitutions)
    {
        if (entry.second.material_flare_def != nullptr) // 'materialflarebindings'
        {
            m_rig->m_gfx_actor->AddMaterialFlare(
                entry.second.material_flare_def->flare_number, entry.second.material);
        }
        else if (entry.second.mirror_prop_type != CustomMaterial::MirrorPropType::MPROP_NONE) // special 'prop' - rear view mirror
        {
            this->CreateMirrorPropVideoCam(
                entry.second.material, entry.second.mirror_prop_type, entry.second.mirror_prop_scenenode);
        }
        else if (entry.second.video_camera_def != nullptr) // 'videocameras'
        {
            this->SetCurrentKeyword(RigDef::File::KEYWORD_VIDEOCAMERA); // Logging
            this->CreateVideoCamera(entry.second.video_camera_def);
            this->SetCurrentKeyword(RigDef::File::KEYWORD_INVALID); // Logging
        }
    }

    if (!App::gfx_enable_videocams.GetActive())
    {
        m_rig->m_gfx_actor->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_DISABLED);
    }

    // Process "emissive cab" materials
    if (m_rig->m_cab_entity != nullptr)
    {
        auto search_itor = m_material_substitutions.find(m_cab_material_name);
        m_rig->m_gfx_actor->RegisterCabMaterial(search_itor->second.material, m_cab_trans_material);
        m_rig->m_gfx_actor->SetCabLightsActive(false); // Reset emissive lights to "off" state
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
    Ogre::ManualObject* mo = gEnv->sceneManager->createManualObject(); // TODO: Eliminate gEnv
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

void RigSpawner::CreateVideoCamera(RigDef::VideoCamera* def)
{
    try
    {
        GfxActor::VideoCamera vcam;

        vcam.vcam_material = this->FindOrCreateCustomizedMaterial(def->material_name);
        if (vcam.vcam_material.isNull())
        {
            this->AddMessage(Message::TYPE_ERROR, "Failed to create VideoCamera with material: " + def->material_name);
            return;
        }

        switch (def->camera_role)
        {
        case -1: vcam.vcam_type = GfxActor::VideoCamType::VCTYPE_VIDEOCAM;       break;
        case  0: vcam.vcam_type = GfxActor::VideoCamType::VCTYPE_TRACKING_VIDEOCAM; break;
        case  1: vcam.vcam_type = GfxActor::VideoCamType::VCTYPE_MIRROR;         break;
        default:
            this->AddMessage(Message::TYPE_ERROR, "VideoCamera (mat: " + def->material_name + ") has invalid 'role': " + TOSTRING(def->camera_role));
            return;
        }

        vcam.vcam_node_center = &this->GetNodeOrThrow(def->reference_node);
        vcam.vcam_node_dir_y  = &this->GetNodeOrThrow(def->bottom_node);
        vcam.vcam_node_dir_z  = &this->GetNodeOrThrow(def->left_node);
        vcam.vcam_pos_offset  = def->offset;

        //rotate camera picture 180 degrees, skip for mirrors
        float rotation_z = (def->camera_role != 1) ? def->rotation.z + 180 : def->rotation.z;
        vcam.vcam_rotation
            = Ogre::Quaternion(Ogre::Degree(rotation_z), Ogre::Vector3::UNIT_Z)
            * Ogre::Quaternion(Ogre::Degree(def->rotation.y), Ogre::Vector3::UNIT_Y)
            * Ogre::Quaternion(Ogre::Degree(def->rotation.x), Ogre::Vector3::UNIT_X);

        // set alternative camposition (optional)
        vcam.vcam_node_alt_pos = &this->GetNodeOrThrow(
            def->alt_reference_node.IsValidAnyState() ? def->alt_reference_node : def->reference_node);

        // set alternative lookat position (optional)
        vcam.vcam_node_lookat = nullptr;
        if (def->alt_orientation_node.IsValidAnyState())
        {
            // This is a tracker camera
            vcam.vcam_type = GfxActor::VideoCamType::VCTYPE_TRACKING_VIDEOCAM;
            vcam.vcam_node_lookat = &this->GetNodeOrThrow(def->alt_orientation_node);
        }

        // TODO: Eliminate gEnv
        vcam.vcam_ogre_camera = gEnv->sceneManager->createCamera(vcam.vcam_material->getName() + "_camera");

        bool useExternalMirrorWindow = BSETTING("UseVideocameraWindows", false);
        bool fullscreenRW = BSETTING("VideoCameraFullscreen", false);

        // check if this vidcamera is also affected
        static int counter = 0;
        if (useExternalMirrorWindow && fullscreenRW)
        {
            int monitor = ISETTING("VideoCameraMonitor_" + TOSTRING(counter), 0);
            if (monitor < 0)
                useExternalMirrorWindow = false;
            // < 0 = fallback to texture
        }

        if (!useExternalMirrorWindow)
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
            Ogre::NameValuePairList misc;
            if (!SSETTING("VideoCameraFSAA", "").empty())
                misc["FSAA"] = SSETTING("VideoCameraFSAA", "");

            if (!SSETTING("VideoCameraColourDepth", "").empty())
                misc["colourDepth"] = SSETTING("VideoCameraColourDepth", "");
            else
                misc["colourDepth"] = "32";

            if (ISETTING("VideoCameraLeft_" + TOSTRING(counter), 0) > 0)
                misc["left"] = SSETTING("VideoCameraLeft_" + TOSTRING(counter), "");

            if (ISETTING("VideoCameraTop_" + TOSTRING(counter), 0) > 0)
                misc["top"] = SSETTING("VideoCameraTop_" + TOSTRING(counter), "");
            if (!SSETTING("VideoCameraWindowBorder", "").empty())
                misc["border"] = SSETTING("VideoCameraWindowBorder", ""); // fixes for windowed mode

            misc["outerDimensions"] = "true"; // fixes for windowed mode

            bool fullscreen = BSETTING("VideoCameraFullscreen", false);
            if (fullscreen)
            {
                int monitor = ISETTING("VideoCameraMonitor_" + TOSTRING(counter), 0);
                misc["monitorIndex"] = TOSTRING(monitor);
            }

            const std::string window_name = (!def->camera_name.empty()) ? def->camera_name : def->material_name;
            vcam.vcam_render_window = Ogre::Root::getSingleton().createRenderWindow(
                window_name, def->texture_width, def->texture_height, fullscreen, &misc);

            if (ISETTING("VideoCameraLeft_" + TOSTRING(counter), 0) > 0)
                vcam.vcam_render_window->reposition(ISETTING("VideoCameraLeft_" + TOSTRING(counter), 0), ISETTING("VideoCameraTop_" + TOSTRING(counter), 0));

            if (ISETTING("VideoCameraWidth_" + TOSTRING(counter), 0) > 0)
                vcam.vcam_render_window->resize(ISETTING("VideoCameraWidth_" + TOSTRING(counter), 0), ISETTING("VideoCameraHeight_" + TOSTRING(counter), 0));

            vcam.vcam_render_window->setAutoUpdated(false);
            fixRenderWindowIcon(vcam.vcam_render_window); // Function from 'Utils.h'
            vcam.vcam_render_window->setDeactivateOnFocusChange(false);
            // TODO: disable texture mirrors
        }

        vcam.vcam_ogre_camera->setNearClipDistance(def->min_clip_distance);
        vcam.vcam_ogre_camera->setFarClipDistance(def->max_clip_distance);
        vcam.vcam_ogre_camera->setFOVy(Ogre::Degree(def->field_of_view));
        const float aspect_ratio = static_cast<float>(def->texture_width) / static_cast<float>(def->texture_height);
        vcam.vcam_ogre_camera->setAspectRatio(aspect_ratio);
        vcam.vcam_material->getTechnique(0)->getPass(0)->setLightingEnabled(false);

        if (vcam.vcam_type == GfxActor::VideoCamType::VCTYPE_MIRROR)
        {
            vcam.vcam_off_tex_name = "Chrome.dds"; // Built-in gray texture
        }
        else
        {
            // The default "NO SIGNAL" noisy blue screen texture
            vcam.vcam_off_tex_name = vcam.vcam_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName();
        }

        if (vcam.vcam_render_target)
        {
            Ogre::Viewport* vp = vcam.vcam_render_target->addViewport(vcam.vcam_ogre_camera);
            vp->setClearEveryFrame(true);
            vp->setBackgroundColour(gEnv->mainCamera->getViewport()->getBackgroundColour());
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
            vp->setBackgroundColour(gEnv->mainCamera->getViewport()->getBackgroundColour());
            vp->setVisibilityMask(~HIDE_MIRROR);
            vp->setVisibilityMask(~DEPTHMAP_DISABLED);
            vp->setOverlaysEnabled(false);
            vcam.vcam_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(vcam.vcam_off_tex_name);
        }

        if (App::diag_videocameras.GetActive())
        {
            Ogre::ManualObject* mo = CreateVideocameraDebugMesh(); // local helper function
            vcam.vcam_debug_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
            vcam.vcam_debug_node->attachObject(mo);
        }

        m_rig->m_gfx_actor->m_videocameras.push_back(vcam);
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

void RigSpawner::CreateMirrorPropVideoCam(
    Ogre::MaterialPtr custom_mat, CustomMaterial::MirrorPropType type, Ogre::SceneNode* prop_scenenode)
{
    static size_t mprop_counter = 0;
    try
    {
        // Prepare videocamera entry
        GfxActor::VideoCamera vcam;
        vcam.vcam_off_tex_name = "mirror.dds";
        vcam.vcam_prop_scenenode = prop_scenenode;
        switch (type)
        {
        case CustomMaterial::MirrorPropType::MPROP_LEFT:
            vcam.vcam_type = GfxActor::VideoCamType::VCTYPE_MIRROR_PROP_LEFT;
            break;

        case CustomMaterial::MirrorPropType::MPROP_RIGHT:
            vcam.vcam_type = GfxActor::VideoCamType::VCTYPE_MIRROR_PROP_RIGHT;
            break;

        default:
            this->AddMessage(Message::TYPE_ERROR, "Cannot create mirror prop of type 'MPROP_NONE'");
            return;
        }

        // Create rendering texture
        const std::string mirror_tex_name = this->ComposeName("MirrorPropTexture-", mprop_counter);
        vcam.vcam_render_tex = Ogre::TextureManager::getSingleton().createManual(mirror_tex_name
            , m_custom_resource_group
            , Ogre::TEX_TYPE_2D
            , 128
            , 256
            , 0
            , Ogre::PF_R8G8B8
            , Ogre::TU_RENDERTARGET);

        // Create OGRE camera
        vcam.vcam_ogre_camera = gEnv->sceneManager->createCamera(this->ComposeName("MirrorPropCamera-", mprop_counter));
        vcam.vcam_ogre_camera->setNearClipDistance(0.2f);
        vcam.vcam_ogre_camera->setFarClipDistance(gEnv->mainCamera->getFarClipDistance());
        vcam.vcam_ogre_camera->setFOVy(Ogre::Degree(50));
        vcam.vcam_ogre_camera->setAspectRatio(
            (gEnv->mainCamera->getViewport()->getActualWidth() / gEnv->mainCamera->getViewport()->getActualHeight()) / 2.0f);

        // Setup rendering
        vcam.vcam_render_target = vcam.vcam_render_tex->getBuffer()->getRenderTarget();
        vcam.vcam_render_target->setActive(true);
        Ogre::Viewport* v = vcam.vcam_render_target->addViewport(vcam.vcam_ogre_camera);
        v->setClearEveryFrame(true);
        v->setBackgroundColour(gEnv->mainCamera->getViewport()->getBackgroundColour());
        v->setOverlaysEnabled(false);

        // Setup material
        vcam.vcam_material = custom_mat;
        vcam.vcam_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(vcam.vcam_render_tex->getName());
        vcam.vcam_material->getTechnique(0)->getPass(0)->setLightingEnabled(false);

        // Submit the videocamera
        m_rig->m_gfx_actor->m_videocameras.push_back(vcam);
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
