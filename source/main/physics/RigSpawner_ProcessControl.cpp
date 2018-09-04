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

#define PROCESS_SECTION_IN_ANY_MODULE(_KEYWORD_, _FIELD_, _FUNCTION_)                      \
{                                                                                          \
    SetCurrentKeyword(_KEYWORD_);                                                          \
    auto module_itor = m_selected_modules.begin();                                         \
    auto module_end  = m_selected_modules.end();                                           \
    for (; module_itor != module_end; ++module_itor)                                       \
    {                                                                                      \
        auto* def = module_itor->get()->_FIELD_.get();                                     \
        if (def != nullptr)                                                                \
        {                                                                                  \
            try {                                                                          \
                _FUNCTION_(*def);                                                          \
            }                                                                              \
            catch (Exception ex)                                                           \
            {                                                                              \
                AddMessage(Message::TYPE_ERROR, ex.what());                                \
            }                                                                              \
            catch (...)                                                                    \
            {                                                                              \
                AddMessage(Message::TYPE_ERROR, "An unknown exception has occured");       \
            }                                                                              \
            break;                                                                         \
        }                                                                                  \
    }                                                                                      \
    SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);                                      \
}

#define PROCESS_MSTRUCT_IN_ANY_MODULE(_KEYWORD_, _FIELD_, _FUNCTION_)                      \
{                                                                                          \
    SetCurrentKeyword(_KEYWORD_);                                                          \
    auto module_itor = m_selected_modules.begin();                                         \
    auto module_end  = m_selected_modules.end();                                           \
    for (; module_itor != module_end; ++module_itor)                                       \
    {                                                                                      \
        try {                                                                              \
            _FUNCTION_(module_itor->get()->_FIELD_);                                       \
        }                                                                                  \
        catch (Exception ex)                                                               \
        {                                                                                  \
            AddMessage(Message::TYPE_ERROR, ex.what());                                    \
        }                                                                                  \
        catch (...)                                                                        \
        {                                                                                  \
            AddMessage(Message::TYPE_ERROR, "An unknown exception has occured");           \
        }                                                                                  \
    }                                                                                      \
    SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);                                      \
}

#define PROCESS_SECTION_IN_ALL_MODULES(_KEYWORD_, _FIELD_, _FUNCTION_)                     \
{                                                                                          \
    SetCurrentKeyword(_KEYWORD_);                                                          \
    auto module_itor = m_selected_modules.begin();                                         \
    auto module_end  = m_selected_modules.end();                                           \
    for (; module_itor != module_end; ++module_itor)                                       \
    {                                                                                      \
        auto& field = module_itor->get()->_FIELD_;                                         \
        auto section_itor = field.begin();                                                 \
        auto section_end  = field.end();                                                   \
        for (; section_itor != section_end; ++section_itor)                                \
        {                                                                                  \
            try {                                                                          \
                _FUNCTION_(*section_itor);                                                 \
            }                                                                              \
            catch (ActorSpawner::Exception & ex)                                             \
            {                                                                              \
                AddMessage(Message::TYPE_ERROR, ex.what());                                \
            }                                                                              \
            catch (...)                                                                    \
            {                                                                              \
                AddMessage(Message::TYPE_ERROR, "An unknown exception has occured");       \
            }                                                                              \
        }                                                                                  \
    }                                                                                      \
    SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);                                      \
}

#define PROCESS_SECTION_IN_ALL_MODULES_COND(_KEYWORD_, _FIELD_, _COND_, _FUNCTION_)        \
{                                                                                          \
    SetCurrentKeyword(_KEYWORD_);                                                          \
    auto module_itor=m_selected_modules.begin();                                           \
    for(; module_itor != m_selected_modules.end(); ++module_itor)                          \
    {                                                                                      \
        auto itor = module_itor->get()->_FIELD_.begin();                                   \
        auto endi = module_itor->get()->_FIELD_.end();                                     \
        for (; itor != endi; ++itor)                                                       \
        {                                                                                  \
            if (_COND_)                                                                    \
            {                                                                              \
                try{                                                                       \
                    this->_FUNCTION_(*itor);                                               \
                }                                                                          \
                catch (Exception& ex)                                                      \
                {                                                                          \
                    AddMessage(Message::TYPE_ERROR,ex.what());                             \
                }                                                                          \
                catch (...)                                                                \
                {                                                                          \
                    AddMessage(Message::TYPE_ERROR, "An unknown exception has occured");   \
                }                                                                          \
            }                                                                              \
        }                                                                                  \
    }                                                                                      \
    SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);                                      \
}

Actor *ActorSpawner::SpawnActor()
{
    InitializeRig();

    // Vehicle name
    m_actor->ar_design_name = m_file->name;

    // Flags in root module
    m_actor->ar_forward_commands         = m_file->forward_commands;
    m_actor->ar_import_commands          = m_file->import_commands;
    m_actor->ar_rescuer_flag             = m_file->rescuer;
    m_actor->m_disable_default_sounds    = m_file->disable_default_sounds;
    m_actor->ar_hide_in_actor_list       = m_file->hide_in_chooser;
    m_actor->m_slidenodes_connect_on_spawn  = m_file->slide_nodes_connect_instantly;

    // Section 'authors' in root module
    ProcessAuthors();

    // Section 'guid' in root module: unused for gameplay
    if (m_file->guid.empty())
    {
        this->AddMessage(Message::TYPE_WARNING, "vehicle uses no GUID, skinning will be impossible");
    }

    // Section 'minimass' in root module
    if (m_file->_minimum_mass_set)
    {
        m_actor->m_minimass = m_file->minimum_mass;
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
        AddExhaust(m_actor->ar_exhaust_pos_node, m_actor->ar_exhaust_dir_node, true, nullptr);
    }

    // ---------------------------- Node generating sections ----------------------------

    // Section 'cinecam'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CINECAM, cinecam, ProcessCinecam);

    // ---------------------------- Wheels (also generate nodes) ----------------------------

    // Section 'wheels'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS, wheels, ProcessWheel);

    // Section 'wheels2'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS2, wheels_2, ProcessWheel2);

    // Section 'meshwheels'
    PROCESS_SECTION_IN_ALL_MODULES_COND(RigDef::File::KEYWORD_MESHWHEELS, mesh_wheels, (!itor->_is_meshwheel2), ProcessMeshWheel);

    // Section 'meshwheels2'
    PROCESS_SECTION_IN_ALL_MODULES_COND(RigDef::File::KEYWORD_MESHWHEELS2, mesh_wheels, (itor->_is_meshwheel2), ProcessMeshWheel2);

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

    // Section 'wings'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WINGS, wings, ProcessWing);

    // Section 'animators'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ANIMATORS, animators, ProcessAnimator);

    // Section 'airbrakes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_AIRBRAKES, airbrakes, ProcessAirbrake);

    // Section 'fusedrag'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FUSEDRAG, fusedrag, ProcessFusedrag);

    // Section 'turbojets'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TURBOJETS, turbojets, ProcessTurbojet);

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
    PROCESS_MSTRUCT_IN_ANY_MODULE(RigDef::File::KEYWORD_SPEEDLIMITER, speed_limiter, ProcessSpeedLimiter);

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

    this->CreateGfxActor(); // Required in the 'flexbodies' section

    // Section 'flexbodies' (Uses generated nodes; needs GfxActor to exist)
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLEXBODIES, flexbodies, ProcessFlexbody);

    // Section 'fixes'
    PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FIXES, fixes, ProcessFixedNode);

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
