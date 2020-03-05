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

/*
    @file
    @brief  Implements part of ActorSpawner class. Code separated for easier debugging.
    @author Petr Ohlidal
    @date   12/2013
*/

#include "RigSpawner.h"

#include "Beam.h"
#include "Renderdash.h"

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
    this->SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);             \
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
    this->SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);             \
}

Actor *ActorSpawner::SpawnActor()
{
    InitializeRig();

    // Vehicle name
    m_actor->ar_design_name = m_file->name;

    // File hash
    m_actor->ar_filehash = m_file->hash;

    // Flags in root module
    m_actor->ar_forward_commands         = m_file->forward_commands;
    m_actor->ar_import_commands          = m_file->import_commands;
    m_actor->ar_rescuer_flag             = m_file->rescuer;
    m_actor->m_disable_default_sounds    = m_file->disable_default_sounds;
    m_actor->ar_hide_in_actor_list       = m_file->hide_in_chooser;
    m_actor->ar_collision_range          = m_file->collision_range;

    // Section 'authors' in root module
    ProcessAuthors();

    // Section 'guid' in root module: unused for gameplay
    if (m_file->guid.empty())
    {
        this->AddMessage(Message::TYPE_WARNING, "vehicle uses no GUID, skinning will be impossible");
    }

    // Section 'description'
    m_actor->description.assign(m_file->description.begin(), m_file->description.end());

    // Section 'managedmaterials'
    // This prepares substitute materials -> MUST be processed before any meshes are loaded.
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MANAGEDMATERIALS, managed_materials, ProcessManagedMaterial);

    // Section 'gobals' in any module
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_GLOBALS, globals, ProcessGlobals);

    // Section 'help' in any module.
    // NOTE: Must be done before "guisettings" (overrides help panel material)
    ProcessHelp();

    // Section 'engine' in any module
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGINE, engine, ProcessEngine);

    // Section 'engoption' in any module
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGOPTION, engoption, ProcessEngoption);

    /* Section 'engturbo' in any module */
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGTURBO, engturbo, ProcessEngturbo);

    // Section 'torquecurve' in any module.
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_TORQUECURVE, torque_curve, ProcessTorqueCurve);

    // Section 'brakes' in any module
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_BRAKES, brakes, ProcessBrakes);

    // Section 'guisettings' in any module
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_GUISETTINGS, gui_settings, ProcessGuiSettings);

    // ---------------------------- User-defined nodes ----------------------------

    // Sections 'nodes' & 'nodes2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_NODES, nodes, ProcessNode);

    // Old-format exhaust (defined by flags 'x/y' in section 'nodes', one per vehicle)
    if (m_actor->ar_exhaust_pos_node != 0 && m_actor->ar_exhaust_dir_node != 0)
    {
        AddExhaust(m_actor->ar_exhaust_pos_node, m_actor->ar_exhaust_dir_node);
    }

    // ---------------------------- Node generating sections ----------------------------

    // Section 'cinecam'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CINECAM, cinecam, ProcessCinecam);

    // ---------------------------- Wheels (also generate nodes) ----------------------------

    // Section 'wheels'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS, wheels, ProcessWheel);

    // Section 'wheels2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS2, wheels_2, ProcessWheel2);

    // Sections 'meshwheels' and 'meshwheels2'
    for (auto& m : m_selected_modules)
    {
        for (auto& def : m->mesh_wheels)
        {
            if (def._is_meshwheel2)
            {
                this->SetCurrentKeyword(RigDef::File::KEYWORD_MESHWHEELS2);
                this->ProcessMeshWheel2(def);
            }
            else
            {
                this->SetCurrentKeyword(RigDef::File::KEYWORD_MESHWHEELS);
                this->ProcessMeshWheel(def);
            }
        }
    }

    // Section 'flexbodywheels'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLEXBODYWHEELS, flex_body_wheels, ProcessFlexBodyWheel);

    // ---------------------------- WheelDetachers ----------------------------

    // Section 'wheeldetachers'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELDETACHERS, wheeldetachers, ProcessWheelDetacher);

    // ---------------------------- User-defined beams ----------------------------
    //              (may reference any generated/user-defined node)

    // Section 'beams'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_BEAMS, beams, ProcessBeam);

    // Section 'shocks'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SHOCKS, shocks, ProcessShock);

    // Section 'shocks2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SHOCKS2, shocks_2, ProcessShock2);

    // Section 'shocks3'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SHOCKS3, shocks_3, ProcessShock3);

    // Section 'commands' and 'commands2' (Use generated nodes)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_COMMANDS2, commands_2, ProcessCommand);

    // Section 'hydros'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_HYDROS, hydros, ProcessHydro);

    // Section 'triggers'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TRIGGERS, triggers, ProcessTrigger);

    // Section 'ropes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROPES, ropes, ProcessRope);

    // ---------------------------- Other ----------------------------

    // Section 'AntiLockBrakes' in any module.
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ANTI_LOCK_BRAKES, anti_lock_brakes, ProcessAntiLockBrakes);

    // Section 'SlopeBrake' in any module (feature removed).
    
    // Sections 'flares' and 'flares2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLARES2, flares_2, ProcessFlare2);

    // Section 'axles'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_AXLES, axles, ProcessAxle);

    // Section 'transfercase'
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_TRANSFER_CASE, transfer_case, ProcessTransferCase);

    // Section 'interaxles'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_INTERAXLES, interaxles, ProcessInterAxle);

    // Section 'submeshes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SUBMESH, submeshes, ProcessSubmesh);

    // Section 'contacters'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CONTACTERS, contacters, ProcessContacter);

    // Section 'cameras'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CAMERAS, cameras, ProcessCamera);

    // Section 'hooks'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_HOOKS, hooks, ProcessHook);	

    // Section 'ties'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TIES, ties, ProcessTie);

    // Section 'ropables'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROPABLES, ropables, ProcessRopable);

    // Section 'animators'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ANIMATORS, animators, ProcessAnimator);

    // Section 'fusedrag'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FUSEDRAG, fusedrag, ProcessFusedrag);

    // Section 'turbojets'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TURBOJETS, turbojets, ProcessTurbojet);

    // Create the built-in "renderdash" material for use in meshes.
    // Must be done before 'props' are processed because those traditionally use it.
    // Must be always created, there is no mechanism to declare the need for it. It can be acessed from any mesh, not only dashboard-prop. Example content: https://github.com/RigsOfRods/rigs-of-rods/files/3044343/45fc291a9d2aa5faaa36cca6df9571cd6d1f1869_Actros_8x8-englisch.zip
    // TODO: Move setup to GfxActor
    m_oldstyle_renderdash = new RoR::Renderdash(
        m_custom_resource_group, this->ComposeName("RenderdashTex", 0), this->ComposeName("RenderdashCam", 0));

    // Section 'props'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PROPS, props, ProcessProp);

    // Section 'TractionControl' in any module.
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_TRACTION_CONTROL, traction_control, ProcessTractionControl);

    // Section 'rotators'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS, rotators, ProcessRotator);

    // Section 'rotators_2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS2, rotators_2, ProcessRotator2);

    // Section 'lockgroups'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_LOCKGROUPS, lockgroups, ProcessLockgroup);

    // Section 'railgroups'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_RAILGROUPS, railgroups, ProcessRailGroup);

    // Section 'slidenodes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SLIDENODES, slidenodes, ProcessSlidenode);

    // Section 'particles'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PARTICLES, particles, ProcessParticle);

    // Section 'cruisecontrol' in any module.
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_CRUISECONTROL, cruise_control, ProcessCruiseControl);

    // Section 'speedlimiter' in any module.
    this->SetCurrentKeyword(RigDef::File::KEYWORD_SPEEDLIMITER);
    for (auto& m : m_selected_modules)
    {
        m_actor->sl_enabled = m->speed_limiter.is_enabled;
        m_actor->sl_speed_limit = m->speed_limiter.max_speed;
    }

    // Section 'collisionboxes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_COLLISIONBOXES, collision_boxes, ProcessCollisionBox);

    // Section 'exhausts'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_EXHAUSTS, exhausts, ProcessExhaust);

    // Section 'extcamera'
    PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_EXTCAMERA, ext_camera, ProcessExtCamera);

    // Section 'camerarail'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CAMERARAIL, camera_rails, ProcessCameraRail);

    // Section 'pistonprops'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PISTONPROPS, pistonprops, ProcessPistonprop);

    // Sections 'turboprops' and 'turboprops2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TURBOPROPS2, turboprops_2, ProcessTurboprop2);

    // Section 'screwprops'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SCREWPROPS, screwprops, ProcessScrewprop);

    // Section 'fixes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FIXES, fixes, ProcessFixedNode);

    this->CreateGfxActor(); // Required in sections below

    // Section 'flexbodies' (Uses generated nodes; needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLEXBODIES, flexbodies, ProcessFlexbody);

    // Section 'wings' (needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WINGS, wings, ProcessWing);

    // Section 'airbrakes' (needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_AIRBRAKES, airbrakes, ProcessAirbrake);

    // Section 'scripts' (needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SCRIPTS, scripts, ProcessScript);

#ifdef USE_OPENAL

    // Section 'soundsources'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SOUNDSOURCES, soundsources, ProcessSoundSource);

    // Section 'soundsources2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SOUNDSOURCES2, soundsources2, ProcessSoundSource2);

#endif // USE_OPENAL

    this->FinalizeRig();
    this->FinalizeGfxSetup();

    // Pass ownership
    Actor *rig = m_actor;
    m_actor = nullptr;
    return rig;
}
