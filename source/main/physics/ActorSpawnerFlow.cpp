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
    this->SetCurrentKeyword(RigDef::Keyword::INVALID);                   \
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
    this->SetCurrentKeyword(RigDef::Keyword::INVALID);                   \
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

    // 'minimass'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::MINIMASS, minimass, ProcessMinimass);

    // 'set_collision_range'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SET_COLLISION_RANGE, set_collision_range, ProcessCollisionRange);

    // Section 'authors'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::AUTHOR, author, ProcessAuthor);

    // Section 'guid' in root module: unused for gameplay
    if (m_file->root_module->guid.empty())
    {
        this->AddMessage(Message::TYPE_WARNING, "vehicle uses no GUID, skinning will be impossible");
    }

    // Section 'description'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::DESCRIPTION, description, ProcessDescription);

    // Section 'managedmaterials'
    // This prepares substitute materials -> MUST be processed before any meshes are loaded.
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::MANAGEDMATERIALS, managedmaterials, ProcessManagedMaterial);

    // Section 'gobals' in any module
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::GLOBALS, globals, ProcessGlobals);

    // Section 'help' in any module.
    // MUST be done before "guisettings" (overrides help panel material)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::HELP, help, ProcessHelp);

    // Section 'engine' in any module
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::ENGINE, engine, ProcessEngine);

    // Section 'engoption' in any module
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::ENGOPTION, engoption, ProcessEngoption);

    /* Section 'engturbo' in any module */
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::ENGTURBO, engturbo, ProcessEngturbo);

    // Section 'torquecurve' in any module.
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::TORQUECURVE, torquecurve, ProcessTorqueCurve);

    // Section 'brakes' in any module
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::BRAKES, brakes, ProcessBrakes);

    // Section 'guisettings' in any module
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::GUISETTINGS, guisettings, ProcessGuiSettings);

    // ---------------------------- User-defined nodes ----------------------------

    // Sections 'nodes' & 'nodes2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::NODES, nodes, ProcessNode);

    // Old-format exhaust (defined by flags 'x/y' in section 'nodes', one per vehicle)
    if (m_actor->ar_exhaust_pos_node != 0 && m_actor->ar_exhaust_dir_node != 0)
    {
        AddExhaust(m_actor->ar_exhaust_pos_node, m_actor->ar_exhaust_dir_node);
    }

    // ---------------------------- Node generating sections ----------------------------

    // Section 'cinecam'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::CINECAM, cinecam, ProcessCinecam);

    // ---------------------------- Wheels (also generate nodes) ----------------------------

    // Section 'wheels'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::WHEELS, wheels, ProcessWheel);

    // Section 'wheels2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::WHEELS2, wheels2, ProcessWheel2);

    // Sections 'meshwheels' and 'meshwheels2'
    for (auto& m : m_selected_modules)
    {
        for (auto& def : m->mesh_wheels)
        {
            if (def._is_meshwheel2)
            {
                this->SetCurrentKeyword(RigDef::Keyword::MESHWHEELS2);
                this->ProcessMeshWheel2(def);
            }
            else
            {
                this->SetCurrentKeyword(RigDef::Keyword::MESHWHEELS);
                this->ProcessMeshWheel(def);
            }
        }
    }

    // Section 'flexbodywheels'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::FLEXBODYWHEELS, flexbodywheels, ProcessFlexBodyWheel);

    // ---------------------------- WheelDetachers ----------------------------

    // Section 'wheeldetachers'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::WHEELDETACHERS, wheeldetachers, ProcessWheelDetacher);

    // ---------------------------- User-defined beams ----------------------------
    //              (may reference any generated/user-defined node)

    // Section 'beams'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::BEAMS, beams, ProcessBeam);

    // Section 'shocks'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SHOCKS, shocks, ProcessShock);

    // Section 'shocks2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SHOCKS2, shocks2, ProcessShock2);

    // Section 'shocks3'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SHOCKS3, shocks3, ProcessShock3);

    // Section 'commands' and 'commands2' (Use generated nodes)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::COMMANDS2, commands2, ProcessCommand);

    // Section 'hydros'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::HYDROS, hydros, ProcessHydro);

    // Section 'triggers'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::TRIGGERS, triggers, ProcessTrigger);

    // Section 'ropes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::ROPES, ropes, ProcessRope);

    // ---------------------------- Other ----------------------------

    // Section 'AntiLockBrakes' in any module.
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::ANTILOCKBRAKES, antilockbrakes, ProcessAntiLockBrakes);
    
    // Sections 'flares' and 'flares2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::FLARES2, flares2, ProcessFlare2);

    // Section 'axles'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::AXLES, axles, ProcessAxle);

    // Section 'transfercase'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::TRANSFERCASE, transfercase, ProcessTransferCase);

    // Section 'interaxles'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::INTERAXLES, interaxles, ProcessInterAxle);

    // Section 'submeshes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SUBMESH, submeshes, ProcessSubmesh);

    // Section 'contacters'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::CONTACTERS, contacters, ProcessContacter);

    // Section 'cameras'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::CAMERAS, cameras, ProcessCamera);

    // Section 'hooks'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::HOOKS, hooks, ProcessHook);	

    // Section 'ties'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::TIES, ties, ProcessTie);

    // Section 'ropables'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::ROPABLES, ropables, ProcessRopable);

    // Section 'animators'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::ANIMATORS, animators, ProcessAnimator);

    // Section 'fusedrag'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::FUSEDRAG, fusedrag, ProcessFusedrag);

    // Section 'turbojets'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::TURBOJETS, turbojets, ProcessTurbojet);

    // Create the built-in "renderdash" material for use in meshes.
    // Must be done before 'props' are processed because those traditionally use it.
    // Must be always created, there is no mechanism to declare the need for it. It can be acessed from any mesh, not only dashboard-prop. Example content: https://github.com/RigsOfRods/rigs-of-rods/files/3044343/45fc291a9d2aa5faaa36cca6df9571cd6d1f1869_Actros_8x8-englisch.zip
    // TODO: Move setup to GfxActor
    m_oldstyle_renderdash = new RoR::Renderdash(
        m_custom_resource_group, this->ComposeName("RenderdashTex", 0), this->ComposeName("RenderdashCam", 0));

    // Section 'props'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::PROPS, props, ProcessProp);

    // Section 'TractionControl' in any module.
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::TRACTIONCONTROL, tractioncontrol, ProcessTractionControl);

    // Section 'rotators'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::ROTATORS, rotators, ProcessRotator);

    // Section 'rotators2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::ROTATORS2, rotators2, ProcessRotator2);

    // Section 'lockgroups'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::LOCKGROUPS, lockgroups, ProcessLockgroup);

    // Section 'railgroups'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::RAILGROUPS, railgroups, ProcessRailGroup);

    // Section 'slidenodes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SLIDENODES, slidenodes, ProcessSlidenode);

    // Section 'particles'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::PARTICLES, particles, ProcessParticle);

    // Section 'cruisecontrol' in any module.
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::CRUISECONTROL, cruisecontrol, ProcessCruiseControl);

    // Section 'speedlimiter'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SPEEDLIMITER, speedlimiter, ProcessSpeedLimiter);

    // Section 'collisionboxes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::COLLISIONBOXES, collisionboxes, ProcessCollisionBox);

    // Section 'exhausts'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::EXHAUSTS, exhausts, ProcessExhaust);

    // Section 'extcamera'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::EXTCAMERA, extcamera, ProcessExtCamera);

    // Section 'camerarail'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::CAMERARAIL, camerarail, ProcessCameraRail);

    // Section 'pistonprops'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::PISTONPROPS, pistonprops, ProcessPistonprop);

    // Sections 'turboprops' and 'turboprops2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::TURBOPROPS2, turboprops2, ProcessTurboprop2);

    // Section 'screwprops'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SCREWPROPS, screwprops, ProcessScrewprop);

    // Section 'fixes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::FIXES, fixes, ProcessFixedNode);

    this->CreateGfxActor(); // Required in sections below

    // Section 'flexbodies' (Uses generated nodes; needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::FLEXBODIES, flexbodies, ProcessFlexbody);

    // Section 'wings' (needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::WINGS, wings, ProcessWing);

    // Section 'airbrakes' (needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::AIRBRAKES, airbrakes, ProcessAirbrake);

#ifdef USE_OPENAL

    // Section 'soundsources'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SOUNDSOURCES, soundsources, ProcessSoundSource);

    // Section 'soundsources2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::Keyword::SOUNDSOURCES2, soundsources2, ProcessSoundSource2);

#endif // USE_OPENAL

    this->FinalizeRig();
    this->FinalizeGfxSetup();

    // Pass ownership
    Actor *rig = m_actor;
    m_actor = nullptr;
    return rig;
}
