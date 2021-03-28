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

/*
    @file
    @brief  Implements part of ActorSpawner class. Code separated for easier debugging.
    @author Petr Ohlidal
    @date   12/2013
*/

#include "ActorSpawner.h"

#include "Actor.h"
#include "FlexBody.h"
#include "GfxData.h"
#include "SimData.h"
#include "Renderdash.h"

using namespace RoR;

#define PROCESS_SECTION_IN_ANY_MODULE(_KEYWORD_, _FIELD_, _FUNCTION_)   \
{                                                                       \
    this->SetCurrentKeyword(_KEYWORD_);                                 \
    for (auto& m: m_selected_modules)                                   \
    {                                                                   \
        if (m->_FIELD_ != nullptr)                                      \
        {                                                               \
            try {                                                       \
                _FUNCTION_(*(m->_FIELD_));                              \
            }                                                           \
            catch (...)                                                 \
            {                                                           \
                this->HandleException();                                \
            }                                                           \
            break;                                                      \
        }                                                               \
    }                                                                   \
    this->SetCurrentKeyword(Truck::KEYWORD_NONE);                   \
}

#define PROCESS_SECTION_IN_ALL_MODULES(_KEYWORD_, _FIELD_, _FUNCTION_)  \
{                                                                       \
    this->SetCurrentKeyword(_KEYWORD_);                                 \
    for (auto& m: m_selected_modules)                                   \
    {                                                                   \
        for (auto& entry: m->_FIELD_)                                   \
        {                                                               \
            try {                                                       \
                _FUNCTION_(entry);                                      \
            }                                                           \
            catch (...)                                                 \
            {                                                           \
                this->HandleException();                                \
            }                                                           \
        }                                                               \
    }                                                                   \
    this->SetCurrentKeyword(Truck::KEYWORD_NONE);                   \
}

Actor *ActorSpawner::SpawnActor(std::string const& config)
{
    m_selected_config = config;
    this->EvaluateSectionConfig();

    InitializeRig();

    // Vehicle name
    m_actor->ar_design_name = m_file->name;

    // File hash
    m_actor->ar_filehash = m_file->hash;

    // Section 'managedmaterials'
    // This prepares substitute materials -> MUST be processed before any meshes are loaded.
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_MANAGEDMATERIALS, managed_materials, ProcessManagedMaterial);

    // Sequential elements - loop by module, then by sequence
    for (Truck::ModulePtr modul: m_selected_modules)
    {
        m_cur_module = modul;
        for (Truck::SeqSection const& elem: modul->sequence)
        {
            this->SetCurrentKeyword(elem.section);
            switch (elem.section)
            {
                // Global settings:
            case Truck::KEYWORD_FORWARDCOMMANDS:
                m_actor->ar_forward_commands = true;
                break;
            case Truck::KEYWORD_IMPORTCOMMANDS:
                m_actor->ar_import_commands = true;
                break;
            case Truck::KEYWORD_RESCUER:
                m_actor->ar_rescuer_flag = true;
                break;
            case Truck::KEYWORD_HIDE_IN_CHOOSER:
                m_actor->ar_hide_in_actor_list  = true;
                break;
            case Truck::KEYWORD_DISABLEDEFAULTSOUNDS:
                m_actor->m_disable_default_sounds = true;
                break;
            case Truck::KEYWORD_GLOBALS:
                this->ProcessGlobals(modul->globals[elem.index]);
                break;
            case Truck::KEYWORD_SLIDENODE_CONNECT_INSTANT:
                m_actor->ar_slidenodes_connect_instantly = true;
                break;
            case Truck::KEYWORD_HELP:
                m_state.helpmat = modul->help[elem.index];
                break;
            case Truck::KEYWORD_AUTHOR:
                this->ProcessAuthor(elem.index);
                break;
            case Truck::KEYWORD_MINIMASS:
                m_state.global_minimass = modul->minimass[elem.index].min_mass;
                if (modul->minimass[elem.index].option_skip_loaded_nodes == 1)
                    m_actor->ar_minimass_skip_loaded_nodes = true;
                break;

                // Presets:
            case Truck::KEYWORD_LOCKGROUP_DEFAULT_NOLOCK:
                m_state.lockgroup_default = NODE_LOCKGROUP_NOLOCK;
                break;
            case Truck::KEYWORD_SET_NODE_DEFAULTS:
                this->ProcessNodeDefaults(elem.index);
                break;
            case Truck::KEYWORD_SET_INERTIA_DEFAULTS:
                this->ProcessInertiaDefaults(elem.index);
                break;
            case Truck::KEYWORD_SET_BEAM_DEFAULTS:
                this->ProcessBeamDefaults(elem.index);
                break;
            case Truck::KEYWORD_SET_BEAM_DEFAULTS_SCALE:
                this->ProcessBeamDefaultsScale(elem.index);
                break;
            case Truck::KEYWORD_SET_COLLISION_RANGE:
                this->ProcessCollisionRangePreset(elem.index);
                break;
            case Truck::KEYWORD_DETACHER_GROUP:
                m_state.detacher_group_state = m_cur_module->detacher_group_preset[elem.index].detacher_group;
                break;
            case Truck::KEYWORD_SET_DEFAULT_MINIMASS:
                m_state.default_minimass = m_cur_module->default_minimass[elem.index];
                break;
            case Truck::KEYWORD_ENABLE_ADVANCED_DEFORMATION:
                m_state.enable_advanced_deformation = true;
                break;
            case Truck::KEYWORD_SET_MANAGEDMATERIALS_OPTIONS:
                this->ProcessManagedMatOptions(elem.index);
                break;
            case Truck::KEYWORD_SET_SKELETON_SETTINGS:
                this->ProcessSkeletonSettings(elem.index);
                break;

                // Nodes:
            case Truck::KEYWORD_NODES:
            case Truck::KEYWORD_NODES2:
                this->ProcessNode(modul->nodes[elem.index]);
                break;

                // Beams:
            case Truck::KEYWORD_ANIMATORS:
                this->ProcessAnimator(modul->animators[elem.index]);
                break;
            case Truck::KEYWORD_BEAMS:
                this->ProcessBeam(modul->beams[elem.index]);
                break;
            case Truck::KEYWORD_COMMANDS:
            case Truck::KEYWORD_COMMANDS2:
                this->ProcessCommand(modul->commands_2[elem.index]);
                break;
            case Truck::KEYWORD_HYDROS:
                this->ProcessHydro(modul->hydros[elem.index]);
                break;
            case Truck::KEYWORD_ROPES:
                this->ProcessRope(modul->ropes[elem.index]);
                break;
            case Truck::KEYWORD_SHOCKS:
                this->ProcessShock(modul->shocks[elem.index]);
                break;
            case Truck::KEYWORD_SHOCKS2:
                this->ProcessShock2(modul->shocks_2[elem.index]);
                break;
            case Truck::KEYWORD_SHOCKS3:
                this->ProcessShock3(modul->shocks_3[elem.index]);
                break;
            case Truck::KEYWORD_TIES:
                this->ProcessTie(modul->ties[elem.index]);
                break;
            case Truck::KEYWORD_TRIGGERS:
                this->ProcessTrigger(modul->triggers[elem.index]);
                break;

                // Generators:
            case Truck::KEYWORD_WHEELS:
                this->ProcessWheel(modul->wheels[elem.index]);
                break;
            case Truck::KEYWORD_WHEELS2:
                this->ProcessWheel2(modul->wheels_2[elem.index]);
                break;
            case Truck::KEYWORD_MESHWHEELS:
                this->ProcessMeshWheel(modul->mesh_wheels[elem.index]);
                break;
            case Truck::KEYWORD_MESHWHEELS2:
                this->ProcessMeshWheel2(modul->mesh_wheels[elem.index]); // Uses the same array as meshwheels
                break;
            case Truck::KEYWORD_FLEXBODYWHEELS:
                this->ProcessFlexBodyWheel(modul->flex_body_wheels[elem.index]);
                break;
            case Truck::KEYWORD_CINECAM:
                this->ProcessCinecam(modul->cinecam[elem.index]);
                break;

                // Drivetrain:
            case Truck::KEYWORD_BRAKES:
                this->ProcessBrakes(modul->brakes[elem.index]);
                break;
            case Truck::KEYWORD_ENGINE:
                this->ProcessEngine(modul->engine[elem.index]);
                break;
            case Truck::KEYWORD_ENGOPTION:
                this->ProcessEngoption(modul->engoption[elem.index]);
                break;
            case Truck::KEYWORD_ENGTURBO:
                this->ProcessEngturbo(modul->engturbo[elem.index]);
                break;
            case Truck::KEYWORD_TORQUECURVE:
                this->ProcessTorqueCurve(modul->torque_curve[elem.index]);
                break;

                // Visuals
            case Truck::KEYWORD_TEXCOORDS:
                m_state.texcoords.push_back({
                    this->ResolveNodeRef(modul->texcoords[elem.index].node),
                    modul->texcoords[elem.index].u,
                    modul->texcoords[elem.index].v});
                break;
            case Truck::KEYWORD_CAB:
                this->ProcessCab(modul->cab[elem.index]);
                break;
            case Truck::KEYWORD_SUBMESH:
                this->ProcessSubmesh();
                break;
            case Truck::KEYWORD_BACKMESH:
                this->ProcessBackmesh();
                break;

            default:
                break;
            }
        }
    }
    m_cur_module = nullptr;

    //                          *************************
    //                         CODE BELOW TO BE CLEANED UP
    //                          *************************


    // Section 'guid' in root module: unused for gameplay
    if (m_file->guid.empty())
    {
        this->AddMessage(Message::TYPE_WARNING, "vehicle uses no GUID, skinning will be impossible");
    }

    // Section 'description'
 //FIXME   m_actor->description.assign(m_file->description.begin(), m_file->description.end());

    // Section 'guisettings' in any module
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_GUISETTINGS, gui_settings, ProcessGuiSettings);

    // ---------------------------- User-defined nodes ----------------------------



    // Old-format exhaust (defined by flags 'x/y' in section 'nodes', one per vehicle)
    if (m_actor->ar_exhaust_pos_node != 0 && m_actor->ar_exhaust_dir_node != 0)
    {
        AddExhaust(m_actor->ar_exhaust_pos_node, m_actor->ar_exhaust_dir_node);
    }




    // ---------------------------- WheelDetachers ----------------------------

    // Section 'wheeldetachers'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_WHEELDETACHERS, wheeldetachers, ProcessWheelDetacher);

    // ---------------------------- Other ----------------------------

    // Section 'AntiLockBrakes' in any module.
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_ANTI_LOCK_BRAKES, anti_lock_brakes, ProcessAntiLockBrakes);

    // Section 'SlopeBrake' in any module (feature removed).
    
    // Sections 'flares' and 'flares2'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_FLARES2, flares_2, ProcessFlare2);

    // Section 'axles'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_AXLES, axles, ProcessAxle);

    // Section 'transfercase'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_TRANSFER_CASE, transfer_case, ProcessTransferCase);

    // Section 'interaxles'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_INTERAXLES, interaxles, ProcessInterAxle);

    // Section 'contacters'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_CONTACTERS, contacters, ProcessContacter);

    // Section 'cameras'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_CAMERAS, cameras, ProcessCamera);

    // Section 'hooks'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_HOOKS, hooks, ProcessHook);	

    // Section 'ties'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_TIES, ties, ProcessTie);

    // Section 'ropables'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_ROPABLES, ropables, ProcessRopable);

    // Section 'animators'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_ANIMATORS, animators, ProcessAnimator);

    // Section 'fusedrag'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_FUSEDRAG, fusedrag, ProcessFusedrag);

    // Section 'turbojets'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_TURBOJETS, turbojets, ProcessTurbojet);

    // Create the built-in "renderdash" material for use in meshes.
    // Must be done before 'props' are processed because those traditionally use it.
    // Must be always created, there is no mechanism to declare the need for it. It can be acessed from any mesh, not only dashboard-prop. Example content: https://github.com/RigsOfRods/rigs-of-rods/files/3044343/45fc291a9d2aa5faaa36cca6df9571cd6d1f1869_Actros_8x8-englisch.zip
    // TODO: Move setup to GfxActor
    m_oldstyle_renderdash = new RoR::Renderdash(
        m_custom_resource_group, this->ComposeName("RenderdashTex", 0), this->ComposeName("RenderdashCam", 0));

    // Section 'props'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_PROPS, props, ProcessProp);

    // Section 'TractionControl' in any module.
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_TRACTION_CONTROL, traction_control, ProcessTractionControl);

    // Section 'rotators'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_ROTATORS, rotators, ProcessRotator);

    // Section 'rotators_2'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_ROTATORS2, rotators_2, ProcessRotator2);

    // Section 'lockgroups'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_LOCKGROUPS, lockgroups, ProcessLockgroup);

    // Section 'railgroups'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_RAILGROUPS, railgroups, ProcessRailGroup);

    // Section 'slidenodes'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_SLIDENODES, slidenodes, ProcessSlidenode);

    // Section 'particles'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_PARTICLES, particles, ProcessParticle);

    // Section 'cruisecontrol'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_CRUISECONTROL, cruise_control, ProcessCruiseControl);

    // Section 'speedlimiter' in any module.
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_SPEEDLIMITER, speed_limiter, ProcessSpeedLimiter);

    // Section 'collisionboxes'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_COLLISIONBOXES, collision_boxes, ProcessCollisionBox);

    // Section 'exhausts'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_EXHAUSTS, exhausts, ProcessExhaust);

    // Section 'extcamera'
    this->SetCurrentKeyword(Truck::KEYWORD_EXTCAMERA);
    for (auto& m : m_selected_modules)
    {
        if (m->extcamera.size() > 0)
        {
            this->ProcessExtCamera(m->extcamera.back());
        }
    }

    // Section 'camerarail'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_CAMERARAIL, camera_rails, ProcessCameraRail);

    // Section 'pistonprops'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_PISTONPROPS, pistonprops, ProcessPistonprop);

    // Sections 'turboprops' and 'turboprops2'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_TURBOPROPS2, turboprops_2, ProcessTurboprop2);

    // Section 'screwprops'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_SCREWPROPS, screwprops, ProcessScrewprop);

    // Section 'fixes'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_FIXES, fixes, ProcessFixedNode);

    // --------------------------------------------------------------------
    // VISUALS SETUP - done separately so it can be parallelized in the future.
    // --------------------------------------------------------------------

    this->CreateGfxActor(); // Required in sections below

    // Some things need to be processed in order

    for (Truck::ModulePtr modul: m_selected_modules)
    {
        m_cur_module = modul;
        for (Truck::SeqSection const& elem: modul->sequence)
        {
            switch (elem.section)
            {
            case Truck::KEYWORD_FLEXBODIES:
                if (m_next_flexbody != -1)
                {
                    this->AddMessage(Message::TYPE_WARNING, "Missing 'forset' after 'flexbody'.");
                }
                m_next_flexbody = elem.index;
                break;
            case Truck::KEYWORD_FLEXBODY_CAMERA_MODE:
                if (m_last_flexbody)
                {
                    m_last_flexbody->setCameraMode(modul->flexbody_camera_mode[elem.index]);
                }
                break;
            case Truck::KEYWORD_FORSET:
                this->ProcessFlexbodyForset(elem.index); // needs GfxActor to exist
                break;

            case Truck::KEYWORD_PROPS:
                this->ProcessProp(modul->props[elem.index]);
                break;
            case Truck::KEYWORD_PROP_CAMERA_MODE:
                if (m_props.size() > 0)
                {
                    m_props.back().pp_camera_mode = modul->prop_camera_mode[elem.index];
                }
                break;
            }
        }
    }



    // Section 'wings' (needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_WINGS, wings, ProcessWing);

    // Section 'airbrakes' (needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_AIRBRAKES, airbrakes, ProcessAirbrake);

#ifdef USE_OPENAL

    // Section 'soundsources'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_SOUNDSOURCES, soundsources, ProcessSoundSource);

    // Section 'soundsources2'
    PROCESS_SECTION_IN_ALL_MODULES(Truck::KEYWORD_SOUNDSOURCES2, soundsources2, ProcessSoundSource2);

#endif // USE_OPENAL

    this->FinalizeRig();
    this->FinalizeGfxSetup();

    // Pass ownership
    Actor *rig = m_actor;
    m_actor = nullptr;
    return rig;
}
