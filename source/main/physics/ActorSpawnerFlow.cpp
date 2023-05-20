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
#include "CacheSystem.h"
#include "GfxScene.h"
#include "Renderdash.h"

using namespace RoR;

#define PROCESS_ELEMENT(_KEYWORD_, _FIELD_, _FUNCTION_)                 \
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
    this->SetCurrentKeyword(RigDef::Keyword::INVALID);                  \
}

void ActorSpawner::ProcessNewActor(ActorPtr actor, ActorSpawnRequest rq, RigDef::DocumentPtr def)
{
    m_actor = actor;
    m_file = def;

    m_particles_parent_scenenode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    m_spawn_position = rq.asr_position;
    m_current_keyword = RigDef::Keyword::INVALID;
    m_wing_area = 0.f;
    m_fuse_z_min = 1000.0f;
    m_fuse_z_max = -1000.0f;
    m_fuse_y_min = 1000.0f;
    m_fuse_y_max = -1000.0f;
    m_first_wing_index = WINGID_INVALID;

    m_generate_wing_position_lights = true;

    if (m_file->root_module->engine.size() > 0) // Engine present => it's a land vehicle.
    {
        m_generate_wing_position_lights = false; // Disable aerial pos. lights for land vehicles.
    }

    // Get resource group name
    App::GetCacheSystem()->CheckResourceLoaded(m_actor->ar_filename, m_custom_resource_group);

    // Create the built-in "renderdash" material for use in meshes.
    // Must be done before 'props' are processed because those traditionally use it.
    // Must be always created, there is no mechanism to declare the need for it. It can be acessed from any mesh, not only dashboard-prop.
    // Example content: https://github.com/RigsOfRods/rigs-of-rods/files/3044343/45fc291a9d2aa5faaa36cca6df9571cd6d1f1869_Actros_8x8-englisch.zip
    // TODO: Move setup to GfxActor
    m_oldstyle_renderdash = new RoR::Renderdash(
        m_custom_resource_group, this->ComposeName("RenderdashTex", 0), this->ComposeName("RenderdashCam", 0));

    this->InitializeRig();

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

    PROCESS_ELEMENT(RigDef::Keyword::MINIMASS, minimass, ProcessMinimass);
    PROCESS_ELEMENT(RigDef::Keyword::SET_COLLISION_RANGE, set_collision_range, ProcessCollisionRange);
    PROCESS_ELEMENT(RigDef::Keyword::AUTHOR, author, ProcessAuthor);

    // 'guid': unused for gameplay
    if (m_file->root_module->guid.empty())
    {
        this->AddMessage(Message::TYPE_WARNING, "vehicle uses no GUID, skinning will be impossible");
    }

    PROCESS_ELEMENT(RigDef::Keyword::DESCRIPTION, description, ProcessDescription);
    // This prepares substitute materials -> MUST be processed before any meshes are loaded.
    PROCESS_ELEMENT(RigDef::Keyword::MANAGEDMATERIALS, managedmaterials, ProcessManagedMaterial);
    PROCESS_ELEMENT(RigDef::Keyword::GLOBALS, globals, ProcessGlobals);
    // MUST be done before "guisettings" (overrides help panel material)
    PROCESS_ELEMENT(RigDef::Keyword::HELP, help, ProcessHelp);
    PROCESS_ELEMENT(RigDef::Keyword::ENGINE, engine, ProcessEngine);
    PROCESS_ELEMENT(RigDef::Keyword::ENGOPTION, engoption, ProcessEngoption);
    PROCESS_ELEMENT(RigDef::Keyword::ENGTURBO, engturbo, ProcessEngturbo);
    PROCESS_ELEMENT(RigDef::Keyword::TORQUECURVE, torquecurve, ProcessTorqueCurve);
    PROCESS_ELEMENT(RigDef::Keyword::BRAKES, brakes, ProcessBrakes);
    PROCESS_ELEMENT(RigDef::Keyword::GUISETTINGS, guisettings, ProcessGuiSettings);

    // ---------------------------- User-defined nodes ----------------------------

    m_actor->m_gfx_actor = std::unique_ptr<RoR::GfxActor>(
        new RoR::GfxActor(m_actor, this, m_custom_resource_group, m_oldstyle_renderdash));

    PROCESS_ELEMENT(RigDef::Keyword::NODES, nodes, ProcessNode);

    // Old-format exhaust (defined by flags 'x/y' in section 'nodes', one per vehicle)
    if (m_actor->ar_exhaust_pos_node != 0 && m_actor->ar_exhaust_dir_node != 0)
    {
        AddExhaust(m_actor->ar_exhaust_pos_node, m_actor->ar_exhaust_dir_node);
    }

    // ---------------------------- Node generating sections ----------------------------

    PROCESS_ELEMENT(RigDef::Keyword::CINECAM, cinecam, ProcessCinecam);

    // ---------------------------- Wheels (also generate nodes) ----------------------------

    PROCESS_ELEMENT(RigDef::Keyword::WHEELS, wheels, ProcessWheel);
    PROCESS_ELEMENT(RigDef::Keyword::WHEELS2, wheels2, ProcessWheel2);
    PROCESS_ELEMENT(RigDef::Keyword::MESHWHEELS, meshwheels, ProcessMeshWheel);
    PROCESS_ELEMENT(RigDef::Keyword::MESHWHEELS2, meshwheels2, ProcessMeshWheel2);
    PROCESS_ELEMENT(RigDef::Keyword::FLEXBODYWHEELS, flexbodywheels, ProcessFlexBodyWheel);

    // ---------------------------- WheelDetachers ----------------------------

    PROCESS_ELEMENT(RigDef::Keyword::WHEELDETACHERS, wheeldetachers, ProcessWheelDetacher);

    // ---------------------------- User-defined beams ----------------------------
    //              (may reference any generated/user-defined node)

    PROCESS_ELEMENT(RigDef::Keyword::BEAMS, beams, ProcessBeam);
    PROCESS_ELEMENT(RigDef::Keyword::SHOCKS, shocks, ProcessShock);
    PROCESS_ELEMENT(RigDef::Keyword::SHOCKS2, shocks2, ProcessShock2);
    PROCESS_ELEMENT(RigDef::Keyword::SHOCKS3, shocks3, ProcessShock3);
    PROCESS_ELEMENT(RigDef::Keyword::COMMANDS2, commands2, ProcessCommand); // 'commands' are auto-imported as 'commands2'.
    PROCESS_ELEMENT(RigDef::Keyword::HYDROS, hydros, ProcessHydro);
    PROCESS_ELEMENT(RigDef::Keyword::TRIGGERS, triggers, ProcessTrigger);
    PROCESS_ELEMENT(RigDef::Keyword::ROPES, ropes, ProcessRope);

    // ---------------------------- Other ----------------------------

    PROCESS_ELEMENT(RigDef::Keyword::ANTILOCKBRAKES, antilockbrakes, ProcessAntiLockBrakes);
    PROCESS_ELEMENT(RigDef::Keyword::FLARES2, flares2, ProcessFlare2);
    PROCESS_ELEMENT(RigDef::Keyword::FLARES3, flares3, ProcessFlare3);
    PROCESS_ELEMENT(RigDef::Keyword::AXLES, axles, ProcessAxle);
    PROCESS_ELEMENT(RigDef::Keyword::TRANSFERCASE, transfercase, ProcessTransferCase);
    PROCESS_ELEMENT(RigDef::Keyword::INTERAXLES, interaxles, ProcessInterAxle);
    PROCESS_ELEMENT(RigDef::Keyword::SUBMESH, submeshes, ProcessSubmesh);
    PROCESS_ELEMENT(RigDef::Keyword::CONTACTERS, contacters, ProcessContacter);
    PROCESS_ELEMENT(RigDef::Keyword::CAMERAS, cameras, ProcessCamera);
    PROCESS_ELEMENT(RigDef::Keyword::HOOKS, hooks, ProcessHook);	
    PROCESS_ELEMENT(RigDef::Keyword::TIES, ties, ProcessTie);
    PROCESS_ELEMENT(RigDef::Keyword::ROPABLES, ropables, ProcessRopable);
    PROCESS_ELEMENT(RigDef::Keyword::ANIMATORS, animators, ProcessAnimator);
    PROCESS_ELEMENT(RigDef::Keyword::FUSEDRAG, fusedrag, ProcessFusedrag);
    PROCESS_ELEMENT(RigDef::Keyword::TURBOJETS, turbojets, ProcessTurbojet);
    PROCESS_ELEMENT(RigDef::Keyword::PROPS, props, ProcessProp);
    PROCESS_ELEMENT(RigDef::Keyword::TRACTIONCONTROL, tractioncontrol, ProcessTractionControl);
    PROCESS_ELEMENT(RigDef::Keyword::ROTATORS, rotators, ProcessRotator);
    PROCESS_ELEMENT(RigDef::Keyword::ROTATORS2, rotators2, ProcessRotator2);
    PROCESS_ELEMENT(RigDef::Keyword::LOCKGROUPS, lockgroups, ProcessLockgroup);
    PROCESS_ELEMENT(RigDef::Keyword::RAILGROUPS, railgroups, ProcessRailGroup);
    PROCESS_ELEMENT(RigDef::Keyword::SLIDENODES, slidenodes, ProcessSlidenode);
    PROCESS_ELEMENT(RigDef::Keyword::PARTICLES, particles, ProcessParticle);
    PROCESS_ELEMENT(RigDef::Keyword::CRUISECONTROL, cruisecontrol, ProcessCruiseControl);
    PROCESS_ELEMENT(RigDef::Keyword::SPEEDLIMITER, speedlimiter, ProcessSpeedLimiter);
    PROCESS_ELEMENT(RigDef::Keyword::COLLISIONBOXES, collisionboxes, ProcessCollisionBox);
    PROCESS_ELEMENT(RigDef::Keyword::EXHAUSTS, exhausts, ProcessExhaust);
    PROCESS_ELEMENT(RigDef::Keyword::EXTCAMERA, extcamera, ProcessExtCamera);
    PROCESS_ELEMENT(RigDef::Keyword::CAMERARAIL, camerarail, ProcessCameraRail);
    PROCESS_ELEMENT(RigDef::Keyword::PISTONPROPS, pistonprops, ProcessPistonprop);
    PROCESS_ELEMENT(RigDef::Keyword::TURBOPROPS2, turboprops2, ProcessTurboprop2); // 'turboprops' are auto-imported as 'turboprops2'.
    PROCESS_ELEMENT(RigDef::Keyword::SCREWPROPS, screwprops, ProcessScrewprop);
    PROCESS_ELEMENT(RigDef::Keyword::FIXES, fixes, ProcessFixedNode);
    PROCESS_ELEMENT(RigDef::Keyword::FLEXBODIES, flexbodies, ProcessFlexbody); // (needs GfxActor to exist)
    PROCESS_ELEMENT(RigDef::Keyword::WINGS, wings, ProcessWing); // (needs GfxActor to exist)
    PROCESS_ELEMENT(RigDef::Keyword::AIRBRAKES, airbrakes, ProcessAirbrake); // (needs GfxActor to exist)

#ifdef USE_OPENAL

    PROCESS_ELEMENT(RigDef::Keyword::SOUNDSOURCES, soundsources, ProcessSoundSource);
    PROCESS_ELEMENT(RigDef::Keyword::SOUNDSOURCES2, soundsources2, ProcessSoundSource2);

#endif // USE_OPENAL

    this->FinalizeRig();

    if (m_oldstyle_cab_texcoords.size() > 0 && m_actor->ar_num_cabs > 0)
    {
        this->CreateCabVisual();
    };

    this->FinalizeGfxSetup();
}
