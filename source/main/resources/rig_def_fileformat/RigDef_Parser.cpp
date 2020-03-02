/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal & contributors

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
/// @date   12/2013

#include "RigDef_Parser.h"

#include "Application.h"
#include "BeamConstants.h"
#include "CacheSystem.h"
#include "Console.h"
#include "RigDef_File.h"
#include "RigDef_Regexes.h"
#include "BitFlags.h"
#include "RoRPrerequisites.h"
#include "Utils.h"

#include <OgreException.h>
#include <OgreString.h>
#include <OgreStringVector.h>
#include <OgreStringConverter.h>

using namespace RoR;

namespace RigDef
{

using namespace RoR;

inline bool IsWhitespace(char c)
{
    return (c == ' ') || (c == '\t');
}

inline bool IsSeparator(char c)
{
    return IsWhitespace(c) || (c == ':') || (c == '|') || (c == ',');
}

inline bool StrEqualsNocase(std::string const & s1, std::string const & s2)
{
    if (s1.size() != s2.size()) { return false; }
    for (size_t i = 0; i < s1.size(); ++i)
    {
        if (tolower(s1[i]) != tolower(s2[i])) { return false; }
    }
    return true;
}

#define STR_PARSE_INT(_STR_)  Ogre::StringConverter::parseInt(_STR_)

#define STR_PARSE_REAL(_STR_) Ogre::StringConverter::parseReal(_STR_)

#define STR_PARSE_BOOL(_STR_) Ogre::StringConverter::parseBool(_STR_)

Parser::Parser()
{
    // Push defaults 
    m_ror_default_inertia = std::shared_ptr<Inertia>(new Inertia);
    m_ror_node_defaults = std::shared_ptr<NodeDefaults>(new NodeDefaults);
    m_ror_minimass = std::shared_ptr<MinimassPreset>(new MinimassPreset);
}

void Parser::ProcessCurrentLine()
{
    if (m_in_block_comment)
    {
        if (StrEqualsNocase(m_current_line, "end_comment"))
        {
            m_in_block_comment = false;
        }
        return;
    }
    else if (StrEqualsNocase(m_current_line, "comment"))
    {
        m_in_block_comment = true;
        return;
    }
    else if (m_in_description_section) // Enter logic is below in 'keywords'
    {
        if (StrEqualsNocase(m_current_line, "end_description"))
        {
            m_in_description_section = false;
        }
        else
        {
            m_definition->description.push_back(m_current_line);
        }
        return;
    }
    else if (StrEqualsNocase(m_current_line, "comment"))
    {
        m_in_block_comment = true;
        return;
    }
    else if ((m_current_line[0] == ';') || (m_current_line[0] == '/'))
    {
        this->ProcessCommentLine();
        return;
    }

    this->TokenizeCurrentLine();

    if (m_num_args == 0)
    {
        return; // Skip empty line
    }

    // Detect keywords on current line 
    File::Keyword keyword = IdentifyKeywordInCurrentLine();
    switch (keyword)
    {
        case File::KEYWORD_INVALID: break; // No new section  - carry on with processing data

        case File::KEYWORD_ADD_ANIMATION:            this->ParseDirectiveAddAnimation();                  return;
        case File::KEYWORD_AIRBRAKES:                this->ChangeSection(File::SECTION_AIRBRAKES);        return;
        case File::KEYWORD_ANIMATORS:                this->ChangeSection(File::SECTION_ANIMATORS);        return;
        case File::KEYWORD_ANTI_LOCK_BRAKES:         this->ParseAntiLockBrakes();                         return;
        case File::KEYWORD_AXLES:                    this->ChangeSection(File::SECTION_AXLES);            return;
        case File::KEYWORD_AUTHOR:                   this->ParseAuthor();                                 return;
        case File::KEYWORD_BACKMESH:                 this->ParseDirectiveBackmesh();                      return;
        case File::KEYWORD_BEAMS:                    this->ChangeSection(File::SECTION_BEAMS);            return;
        case File::KEYWORD_BRAKES:                   this->ChangeSection(File::SECTION_BRAKES);           return;
        case File::KEYWORD_CAB:                      this->ProcessKeywordCab();                           return;
        case File::KEYWORD_CAMERAS:                  this->ChangeSection(File::SECTION_CAMERAS);          return;
        case File::KEYWORD_CAMERARAIL:               this->ChangeSection(File::SECTION_CAMERA_RAIL);      return;
        case File::KEYWORD_CINECAM:                  this->ChangeSection(File::SECTION_CINECAM);          return;
        case File::KEYWORD_COLLISIONBOXES:           this->ChangeSection(File::SECTION_COLLISION_BOXES);  return;
        case File::KEYWORD_COMMANDS:                 this->ChangeSection(File::SECTION_COMMANDS);         return;
        case File::KEYWORD_COMMANDS2:                this->ChangeSection(File::SECTION_COMMANDS_2);       return;
        case File::KEYWORD_CONTACTERS:               this->ChangeSection(File::SECTION_CONTACTERS);       return;
        case File::KEYWORD_CRUISECONTROL:            this->ParseCruiseControl();                          return;
        case File::KEYWORD_DESCRIPTION:              this->ChangeSection(File::SECTION_NONE); m_in_description_section = true; return;
        case File::KEYWORD_DETACHER_GROUP:           this->ParseDirectiveDetacherGroup();                 return;
        case File::KEYWORD_DISABLEDEFAULTSOUNDS:     this->ProcessGlobalDirective(keyword);               return;
        case File::KEYWORD_ENABLE_ADVANCED_DEFORM:   this->ProcessGlobalDirective(keyword);               return;
        case File::KEYWORD_END:                      this->ChangeSection(File::SECTION_NONE);             return;
        case File::KEYWORD_END_SECTION:              this->ProcessChangeModuleLine(keyword);              return;
        case File::KEYWORD_ENGINE:                   this->ChangeSection(File::SECTION_ENGINE);           return;
        case File::KEYWORD_ENGOPTION:                this->ChangeSection(File::SECTION_ENGOPTION);        return;
        case File::KEYWORD_ENGTURBO:                 this->ChangeSection(File::SECTION_ENGTURBO);         return;
        case File::KEYWORD_ENVMAP:                   /* Ignored */                                        return;
        case File::KEYWORD_EXHAUSTS:                 this->ChangeSection(File::SECTION_EXHAUSTS);         return;
        case File::KEYWORD_EXTCAMERA:                this->ParseExtCamera();                              return;
        case File::KEYWORD_FILEFORMATVERSION:        this->ParseFileFormatVersion();                      return;
        case File::KEYWORD_FILEINFO:                 this->ParseFileinfo();                               return;
        case File::KEYWORD_FIXES:                    this->ChangeSection(File::SECTION_FIXES);            return;
        case File::KEYWORD_FLARES:                   this->ChangeSection(File::SECTION_FLARES);           return;
        case File::KEYWORD_FLARES2:                  this->ChangeSection(File::SECTION_FLARES_2);         return;
        case File::KEYWORD_FLEXBODIES:               this->ChangeSection(File::SECTION_FLEXBODIES);       return;
        case File::KEYWORD_FLEXBODY_CAMERA_MODE:     this->ParseDirectiveFlexbodyCameraMode();            return;
        case File::KEYWORD_FLEXBODYWHEELS:           this->ChangeSection(File::SECTION_FLEX_BODY_WHEELS); return;
        case File::KEYWORD_FORWARDCOMMANDS:          this->ProcessGlobalDirective(keyword);               return;
        case File::KEYWORD_FUSEDRAG:                 this->ChangeSection(File::SECTION_FUSEDRAG);         return;
        case File::KEYWORD_GLOBALS:                  this->ChangeSection(File::SECTION_GLOBALS);          return;
        case File::KEYWORD_GUID:                     this->ParseGuid();                                   return;
        case File::KEYWORD_GUISETTINGS:              this->ChangeSection(File::SECTION_GUI_SETTINGS);     return;
        case File::KEYWORD_HELP:                     this->ChangeSection(File::SECTION_HELP);             return;
        case File::KEYWORD_HIDE_IN_CHOOSER:          this->ProcessGlobalDirective(keyword);               return;
        case File::KEYWORD_HOOKGROUP:                /* Obsolete, ignored */                              return;
        case File::KEYWORD_HOOKS:                    this->ChangeSection(File::SECTION_HOOKS);            return;
        case File::KEYWORD_HYDROS:                   this->ChangeSection(File::SECTION_HYDROS);           return;
        case File::KEYWORD_IMPORTCOMMANDS:           m_definition->import_commands = true;                return;
        case File::KEYWORD_INTERAXLES:               this->ChangeSection(File::SECTION_INTERAXLES);       return;
        case File::KEYWORD_LOCKGROUPS:               this->ChangeSection(File::SECTION_LOCKGROUPS);       return;
        case File::KEYWORD_LOCKGROUP_DEFAULT_NOLOCK: this->ProcessGlobalDirective(keyword);               return;
        case File::KEYWORD_MANAGEDMATERIALS:         this->ChangeSection(File::SECTION_MANAGED_MATERIALS); return;
        case File::KEYWORD_MATERIALFLAREBINDINGS:    this->ChangeSection(File::SECTION_MAT_FLARE_BINDINGS); return;
        case File::KEYWORD_MESHWHEELS:               this->ChangeSection(File::SECTION_MESH_WHEELS);      return;
        case File::KEYWORD_MESHWHEELS2:              this->ChangeSection(File::SECTION_MESH_WHEELS_2);    return;
        case File::KEYWORD_MINIMASS:                 this->ChangeSection(File::SECTION_MINIMASS);         return;
        case File::KEYWORD_NODECOLLISION:            this->ChangeSection(File::SECTION_NODE_COLLISION);   return;
        case File::KEYWORD_NODES:                    this->ChangeSection(File::SECTION_NODES);            return;
        case File::KEYWORD_NODES2:                   this->ChangeSection(File::SECTION_NODES_2);          return;
        case File::KEYWORD_PARTICLES:                this->ChangeSection(File::SECTION_PARTICLES);        return;
        case File::KEYWORD_PISTONPROPS:              this->ChangeSection(File::SECTION_PISTONPROPS);      return;
        case File::KEYWORD_PROP_CAMERA_MODE:         this->ParseDirectivePropCameraMode();                return;
        case File::KEYWORD_PROPS:                    this->ChangeSection(File::SECTION_PROPS);            return;
        case File::KEYWORD_RAILGROUPS:               this->ChangeSection(File::SECTION_RAILGROUPS);       return;
        case File::KEYWORD_RESCUER:                  this->ProcessGlobalDirective(keyword);               return;
        case File::KEYWORD_RIGIDIFIERS:              this->AddMessage(Message::TYPE_WARNING, "Rigidifiers are not supported, ignoring..."); return;
        case File::KEYWORD_ROLLON:                   this->ProcessGlobalDirective(keyword);               return;
        case File::KEYWORD_ROPABLES:                 this->ChangeSection(File::SECTION_ROPABLES);         return;
        case File::KEYWORD_ROPES:                    this->ChangeSection(File::SECTION_ROPES);            return;
        case File::KEYWORD_ROTATORS:                 this->ChangeSection(File::SECTION_ROTATORS);         return;
        case File::KEYWORD_ROTATORS2:                this->ChangeSection(File::SECTION_ROTATORS_2);       return;
        case File::KEYWORD_SCREWPROPS:               this->ChangeSection(File::SECTION_SCREWPROPS);       return;
        case File::KEYWORD_SCRIPTS:                  this->ChangeSection(File::SECTION_SCRIPTS);          return;
        case File::KEYWORD_SECTION:                  this->ProcessChangeModuleLine(keyword);              return;
        case File::KEYWORD_SECTIONCONFIG:            /* Ignored */                                        return;
        case File::KEYWORD_SET_BEAM_DEFAULTS:        this->ParseDirectiveSetBeamDefaults();               return;
        case File::KEYWORD_SET_BEAM_DEFAULTS_SCALE:  this->ParseDirectiveSetBeamDefaultsScale();          return;
        case File::KEYWORD_SET_COLLISION_RANGE:      this->ParseSetCollisionRange();                      return;
        case File::KEYWORD_SET_DEFAULT_MINIMASS:     this->ParseDirectiveSetDefaultMinimass();            return;
        case File::KEYWORD_SET_INERTIA_DEFAULTS:     this->ParseDirectiveSetInertiaDefaults();            return;
        case File::KEYWORD_SET_MANAGEDMATS_OPTIONS:  this->ParseDirectiveSetManagedMaterialsOptions();    return;
        case File::KEYWORD_SET_NODE_DEFAULTS:        this->ParseDirectiveSetNodeDefaults();               return;
        case File::KEYWORD_SET_SKELETON_SETTINGS:    this->ParseSetSkeletonSettings();                    return;
        case File::KEYWORD_SHOCKS:                   this->ChangeSection(File::SECTION_SHOCKS);           return;
        case File::KEYWORD_SHOCKS2:                  this->ChangeSection(File::SECTION_SHOCKS_2);         return;
        case File::KEYWORD_SHOCKS3:                  this->ChangeSection(File::SECTION_SHOCKS_3);         return;
        case File::KEYWORD_SLIDENODE_CONNECT_INSTANT:this->ProcessGlobalDirective(keyword);               return;
        case File::KEYWORD_SLIDENODES:               this->ChangeSection(File::SECTION_SLIDENODES);       return;
        case File::KEYWORD_SLOPE_BRAKE:              this->ParseSlopeBrake();                             return;
        case File::KEYWORD_SOUNDSOURCES:             this->ChangeSection(File::SECTION_SOUNDSOURCES);     return;
        case File::KEYWORD_SOUNDSOURCES2:            this->ChangeSection(File::SECTION_SOUNDSOURCES2);    return;
        case File::KEYWORD_SPEEDLIMITER:             this->ParseSpeedLimiter();                           return;
        case File::KEYWORD_SUBMESH_GROUNDMODEL:      this->ParseSubmeshGroundModel();                     return;
        case File::KEYWORD_SUBMESH:                  this->ChangeSection(File::SECTION_SUBMESH);          return;
        case File::KEYWORD_TEXCOORDS:                this->ProcessKeywordTexcoords();                     return;
        case File::KEYWORD_TIES:                     this->ChangeSection(File::SECTION_TIES);             return;
        case File::KEYWORD_TORQUECURVE:              this->ChangeSection(File::SECTION_TORQUE_CURVE);     return;
        case File::KEYWORD_TRACTION_CONTROL:         this->ParseTractionControl();                        return;
        case File::KEYWORD_TRANSFER_CASE:            this->ChangeSection(File::SECTION_TRANSFER_CASE);    return;
        case File::KEYWORD_TRIGGERS:                 this->ChangeSection(File::SECTION_TRIGGERS);         return;
        case File::KEYWORD_TURBOJETS:                this->ChangeSection(File::SECTION_TURBOJETS);        return;
        case File::KEYWORD_TURBOPROPS:               this->ChangeSection(File::SECTION_TURBOPROPS);       return;
        case File::KEYWORD_TURBOPROPS2:              this->ChangeSection(File::SECTION_TURBOPROPS_2);     return;
        case File::KEYWORD_VIDEOCAMERA:              this->ChangeSection(File::SECTION_VIDEO_CAMERA);     return;
        case File::KEYWORD_WHEELDETACHERS:           this->ChangeSection(File::SECTION_WHEELDETACHERS);   return;
        case File::KEYWORD_WHEELS:                   this->ChangeSection(File::SECTION_WHEELS);           return;
        case File::KEYWORD_WHEELS2:                  this->ChangeSection(File::SECTION_WHEELS_2);         return;
        case File::KEYWORD_WINGS:                    this->ChangeSection(File::SECTION_WINGS);            return;
    }

    // Parse current section, if any 
    switch (m_current_section)
    {
        case (File::SECTION_AIRBRAKES):            this->ParseAirbrakes();               return;
        case (File::SECTION_ANIMATORS):            this->ParseAnimator();                return;
        case (File::SECTION_AXLES):                this->ParseAxles();                   return;
        case (File::SECTION_TRUCK_NAME):           this->ParseActorNameLine();           return; 
        case (File::SECTION_BEAMS):                this->ParseBeams();                   return;
        case (File::SECTION_BRAKES):               this->ParseBrakes();                  return;
        case (File::SECTION_CAMERAS):              this->ParseCameras();                 return;
        case (File::SECTION_CAMERA_RAIL):          this->ParseCameraRails();             return;
        case (File::SECTION_CINECAM):              this->ParseCinecam();                 return;
        case (File::SECTION_COMMANDS):
        case (File::SECTION_COMMANDS_2):           this->ParseCommandsUnified();         return;
        case (File::SECTION_COLLISION_BOXES):      this->ParseCollisionBox();            return;
        case (File::SECTION_CONTACTERS):           this->ParseContacter();               return;
        case (File::SECTION_ENGINE):               this->ParseEngine();                  return;
        case (File::SECTION_ENGOPTION):            this->ParseEngoption();               return;
        case (File::SECTION_ENGTURBO) :            this->ParseEngturbo();                return;
        case (File::SECTION_EXHAUSTS):             this->ParseExhaust();                 return;
        case (File::SECTION_FIXES):                this->ParseFixes();                   return;
        case (File::SECTION_FLARES):
        case (File::SECTION_FLARES_2):             this->ParseFlaresUnified();           return;
        case (File::SECTION_FLEXBODIES):           this->ParseFlexbody();                return;
        case (File::SECTION_FLEX_BODY_WHEELS):     this->ParseFlexBodyWheel();           return;
        case (File::SECTION_FUSEDRAG):             this->ParseFusedrag();                return;
        case (File::SECTION_GLOBALS):              this->ParseGlobals();                 return;
        case (File::SECTION_GUI_SETTINGS):         this->ParseGuiSettings();             return;
        case (File::SECTION_HELP):                 this->ParseHelp();                    return;
        case (File::SECTION_HOOKS):                this->ParseHook();                    return;
        case (File::SECTION_HYDROS):               this->ParseHydros();                  return;
        case (File::SECTION_INTERAXLES):           this->ParseInterAxles();              return;
        case (File::SECTION_LOCKGROUPS):           this->ParseLockgroups();              return;
        case (File::SECTION_MANAGED_MATERIALS):    this->ParseManagedMaterials();        return;
        case (File::SECTION_MAT_FLARE_BINDINGS):   this->ParseMaterialFlareBindings();   return;
        case (File::SECTION_MESH_WHEELS):
        case (File::SECTION_MESH_WHEELS_2):        this->ParseMeshWheelUnified();        return;
        case (File::SECTION_MINIMASS):             this->ParseMinimass();                return;
        case (File::SECTION_NODE_COLLISION):       this->ParseNodeCollision();           return;
        case (File::SECTION_NODES):
        case (File::SECTION_NODES_2):              this->ParseNodesUnified();            return;
        case (File::SECTION_PARTICLES):            this->ParseParticles();               return;
        case (File::SECTION_PISTONPROPS):          this->ParsePistonprops();             return;
        case (File::SECTION_PROPS):                this->ParseProps();                   return;
        case (File::SECTION_RAILGROUPS):           this->ParseRailGroups();              return;
        case (File::SECTION_ROPABLES):             this->ParseRopables();                return;
        case (File::SECTION_ROPES):                this->ParseRopes();                   return;
        case (File::SECTION_ROTATORS):
        case (File::SECTION_ROTATORS_2):           this->ParseRotatorsUnified();         return;
        case (File::SECTION_SCREWPROPS):           this->ParseScrewprops();              return;
        case (File::SECTION_SCRIPTS):              this->ParseScripts();                 return;
        case (File::SECTION_SHOCKS):               this->ParseShock();                   return;
        case (File::SECTION_SHOCKS_2):             this->ParseShock2();                  return;
        case (File::SECTION_SHOCKS_3):             this->ParseShock3();                  return;
        case (File::SECTION_SLIDENODES):           this->ParseSlidenodes();              return;
        case (File::SECTION_SOUNDSOURCES):         this->ParseSoundsources();            return;
        case (File::SECTION_SOUNDSOURCES2):        this->ParseSoundsources2();           return;
        case (File::SECTION_SUBMESH):              this->ParseSubmesh();                 return;
        case (File::SECTION_TIES):                 this->ParseTies();                    return;
        case (File::SECTION_TORQUE_CURVE):         this->ParseTorqueCurve();             return;
        case (File::SECTION_TRANSFER_CASE):        this->ParseTransferCase();            return;
        case (File::SECTION_TRIGGERS):             this->ParseTriggers();                return;
        case (File::SECTION_TURBOJETS):            this->ParseTurbojets();               return;
        case (File::SECTION_TURBOPROPS):           
        case (File::SECTION_TURBOPROPS_2):         this->ParseTurbopropsUnified();       return;
        case (File::SECTION_VIDEO_CAMERA):         this->ParseVideoCamera();             return;
        case (File::SECTION_WHEELDETACHERS):       this->ParseWheelDetachers();          return;
        case (File::SECTION_WHEELS):               this->ParseWheel();                   return;
        case (File::SECTION_WHEELS_2):             this->ParseWheel2();                  return;
        case (File::SECTION_WINGS):                this->ParseWing();                    return;
        default:;
    };
}

bool Parser::CheckNumArguments(int num_required_args)
{
    if (num_required_args > m_num_args)
    {
        char msg[200];
        snprintf(msg, 200, "Not enough arguments, %d required, got %d. Skipping line.", num_required_args, m_num_args);
        this->AddMessage(Message::TYPE_WARNING, msg);
        return false;
    }
    return true;
}

// -------------------------------------------------------------------------- 
// Parsing individual keywords                                                
// -------------------------------------------------------------------------- 

void Parser::ParseActorNameLine()
{
    m_definition->name = m_current_line; // Already trimmed
    this->ChangeSection(File::SECTION_NONE);
}

void Parser::ParseWing()
{
    if (!this->CheckNumArguments(16)) { return; }

    Wing wing;

    for (int i = 0; i <  8; i++) { wing.nodes[i]        = this->GetArgNodeRef     (i);  }
    for (int i = 8; i < 16; i++) { wing.tex_coords[i-8] = this->GetArgFloat       (i);  }

    if (m_num_args > 16)         { wing.control_surface = this->GetArgWingSurface (16); }
    if (m_num_args > 17)         { wing.chord_point     = this->GetArgFloat       (17); }
    if (m_num_args > 18)         { wing.min_deflection  = this->GetArgFloat       (18); }
    if (m_num_args > 19)         { wing.max_deflection  = this->GetArgFloat       (19); }
    if (m_num_args > 20)         { wing.airfoil         = this->GetArgStr         (20); }
    if (m_num_args > 21)         { wing.efficacy_coef   = this->GetArgFloat       (21); }

    m_current_module->wings.push_back(wing);
}

void Parser::ParseSetCollisionRange()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    float value = this->GetArgFloat(1);
    if (value >= 0)
    {
        m_definition->collision_range = value;
    }
}

void Parser::ParseWheel2()
{
    if (!this->CheckNumArguments(17)) { return; }

    Wheel2 wheel_2;
    wheel_2.node_defaults = m_user_node_defaults;
    wheel_2.beam_defaults = m_user_beam_defaults;

    wheel_2.rim_radius         = this->GetArgFloat        ( 0);
    wheel_2.tyre_radius        = this->GetArgFloat        ( 1);
    wheel_2.width              = this->GetArgFloat        ( 2);
    wheel_2.num_rays           = this->GetArgInt          ( 3);
    wheel_2.nodes[0]           = this->GetArgNodeRef      ( 4);
    wheel_2.nodes[1]           = this->GetArgNodeRef      ( 5);
    wheel_2.rigidity_node      = this->GetArgRigidityNode ( 6);
    wheel_2.braking            = this->GetArgBraking      ( 7);
    wheel_2.propulsion         = this->GetArgPropulsion   ( 8);
    wheel_2.reference_arm_node = this->GetArgNodeRef      ( 9);
    wheel_2.mass               = this->GetArgFloat        (10);
    wheel_2.rim_springiness    = this->GetArgFloat        (11);
    wheel_2.rim_damping        = this->GetArgFloat        (12);
    wheel_2.tyre_springiness   = this->GetArgFloat        (13);
    wheel_2.tyre_damping       = this->GetArgFloat        (14);
    wheel_2.face_material_name = this->GetArgStr          (15);
    wheel_2.band_material_name = this->GetArgStr          (16);

    if (m_sequential_importer.IsEnabled())
    {
        m_sequential_importer.GenerateNodesForWheel(File::KEYWORD_WHEELS2, wheel_2.num_rays, wheel_2.rigidity_node.IsValidAnyState());
    }

    m_current_module->wheels_2.push_back(wheel_2);
}

void Parser::ParseWheel()
{
    if (! this->CheckNumArguments(14)) { return; }

    Wheel wheel;
    wheel.node_defaults = m_user_node_defaults;
    wheel.beam_defaults = m_user_beam_defaults;

    wheel.radius             = this->GetArgFloat        ( 0);
    wheel.width              = this->GetArgFloat        ( 1);
    wheel.num_rays           = this->GetArgInt          ( 2);
    wheel.nodes[0]           = this->GetArgNodeRef      ( 3);
    wheel.nodes[1]           = this->GetArgNodeRef      ( 4);
    wheel.rigidity_node      = this->GetArgRigidityNode ( 5);
    wheel.braking            = this->GetArgBraking      ( 6);
    wheel.propulsion         = this->GetArgPropulsion   ( 7);
    wheel.reference_arm_node = this->GetArgNodeRef      ( 8);
    wheel.mass               = this->GetArgFloat        ( 9);
    wheel.springiness        = this->GetArgFloat        (10);
    wheel.damping            = this->GetArgFloat        (11);
    wheel.face_material_name = this->GetArgStr          (12);
    wheel.band_material_name = this->GetArgStr          (13);

    if (m_sequential_importer.IsEnabled())
    {
        m_sequential_importer.GenerateNodesForWheel(File::KEYWORD_WHEELS, wheel.num_rays, wheel.rigidity_node.IsValidAnyState());
    }

    m_current_module->wheels.push_back(wheel);
}

void Parser::ParseWheelDetachers()
{
    if (! this->CheckNumArguments(2)) { return; }

    WheelDetacher wheeldetacher;

    wheeldetacher.wheel_id       = this->GetArgInt(0);
    wheeldetacher.detacher_group = this->GetArgInt(1);

    m_current_module->wheeldetachers.push_back(wheeldetacher);
}

void Parser::ParseTractionControl()
{
    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 15, ","); // "TractionControl" = 15 characters
    if (tokens.size() < 2)
    {
        this->AddMessage(Message::TYPE_ERROR, "Too few arguments");
        return;
    }

    TractionControl tc;
                             tc.regulation_force = this->ParseArgFloat(tokens[0].c_str());
                             tc.wheel_slip       = this->ParseArgFloat(tokens[1].c_str());
    if (tokens.size() > 2) { tc.fade_speed       = this->ParseArgFloat(tokens[2].c_str()); }
    if (tokens.size() > 3) { tc.pulse_per_sec    = this->ParseArgFloat(tokens[3].c_str()); }

    for (unsigned int i=4; i<tokens.size(); i++)
    {
        Ogre::StringVector args2 = Ogre::StringUtil::split(tokens[i], ":");
        Ogre::StringUtil::trim(args2[0]);
        Ogre::StringUtil::toLowerCase(args2[0]);

        if (args2[0] == "mode" && args2.size() == 2)
        {
            Ogre::StringVector attrs = Ogre::StringUtil::split(args2[1], "&");
            auto itor = attrs.begin();
            auto endi = attrs.end();
            for (; itor != endi; ++itor)
            {
                std::string attr = *itor;
                Ogre::StringUtil::trim(attr);
                Ogre::StringUtil::toLowerCase(attr);
                     if (strncmp(attr.c_str(), "nodash", 6)   == 0) { tc.attr_no_dashboard = true;  }
                else if (strncmp(attr.c_str(), "notoggle", 8) == 0) { tc.attr_no_toggle    = true;  }
                else if (strncmp(attr.c_str(), "on", 2)       == 0) { tc.attr_is_on        = true;  }
                else if (strncmp(attr.c_str(), "off", 3)      == 0) { tc.attr_is_on        = false; }
            }
        }
        else
        {
            this->AddMessage(Message::TYPE_ERROR, "TractionControl Mode: missing");
            tc.attr_no_dashboard = false;
            tc.attr_no_toggle = false;
            tc.attr_is_on = true;
        }
    }

    if (m_current_module->traction_control != nullptr)
    {
        this->AddMessage(Message::TYPE_WARNING, "Multiple inline-sections 'TractionControl' in a module, using last one ...");
    }

    m_current_module->traction_control = std::shared_ptr<TractionControl>( new TractionControl(tc) );
}

void Parser::ParseTransferCase()
{
    if (! this->CheckNumArguments(2)) { return; }

    TransferCase tc;

    tc.a1 = this->GetArgInt(0) - 1;
    tc.a2 = this->GetArgInt(1) - 1;
    if (m_num_args > 2) { tc.has_2wd    = this->GetArgInt(2); }
    if (m_num_args > 3) { tc.has_2wd_lo = this->GetArgInt(3); }
    for (int i = 4; i < m_num_args; i++) { tc.gear_ratios.push_back(this->GetArgFloat(i)); }

    if (m_current_module->transfer_case != nullptr)
    {
        this->AddMessage(Message::TYPE_WARNING, "Multiple inline-sections 'transfercase' in a module, using last one ...");
    }

    m_current_module->transfer_case = std::shared_ptr<TransferCase>( new TransferCase(tc) );
}

void Parser::ParseSubmeshGroundModel()
{
    if (!this->CheckNumArguments(2)) { return; } // Items: keyword, arg

    m_current_module->submeshes_ground_model_name = this->GetArgStr(1);
}

void Parser::ParseSpeedLimiter()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    SpeedLimiter& sl = m_current_module->speed_limiter;
    if (sl.is_enabled)
    {
        this->AddMessage(Message::TYPE_WARNING, "Multiple inline-sections 'speedlimiter' in a module, using last one ...");
    }

    sl.is_enabled = true;
    sl.max_speed = this->GetArgFloat(1);
    if (sl.max_speed <= 0.f)
    {
        char msg[200];
        snprintf(msg, 200, "Invalid 'max_speed' (%f), must be > 0.0. Using it anyway (compatibility)", sl.max_speed);
        this->AddMessage(Message::TYPE_WARNING, msg);
    }
}

void Parser::ParseSlopeBrake()
{
    if (m_current_module->slope_brake != nullptr)
    {
        this->AddMessage(Message::TYPE_WARNING, "Multiple definitions 'SlopeBrake' in a module, using last one ...");
    }
    m_current_module->slope_brake = std::shared_ptr<SlopeBrake>( new SlopeBrake() );
    
    SlopeBrake* sb = m_current_module->slope_brake.get();
    if (m_num_args > 1) { sb->regulating_force = this->GetArgFloat(1); }
    if (m_num_args > 2) { sb->attach_angle     = this->GetArgFloat(2); }
    if (m_num_args > 3) { sb->release_angle    = this->GetArgFloat(3); }
}

void Parser::ParseSetSkeletonSettings()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    SkeletonSettings& skel = m_current_module->skeleton_settings;    
    skel.visibility_range_meters = this->GetArgFloat(1);
    if (m_num_args > 2) { skel.beam_thickness_meters = this->GetArgFloat(2); }
    
    // Defaults
    if (skel.visibility_range_meters < 0.f) { skel.visibility_range_meters = 150.f; }
    if (skel.beam_thickness_meters   < 0.f) { skel.beam_thickness_meters   = BEAM_SKELETON_DIAMETER; }
}

void Parser::LogParsedDirectiveSetNodeDefaultsData(float load_weight, float friction, float volume, float surface, unsigned int options)
{
    std::stringstream msg;
    msg << "Parsed data for verification:"
        << "\n\tLoadWeight: " << load_weight
        << "\n\t  Friction: " << friction
        << "\n\t    Volume: " << volume
        << "\n\t   Surface: " << surface
        << "\n\t   Options: ";
        
    if (BITMASK_IS_1(options, Node::OPTION_l_LOAD_WEIGHT)       )  { msg << " l_LOAD_WEIGHT"; }
    if (BITMASK_IS_1(options, Node::OPTION_n_MOUSE_GRAB)        )  { msg << " n_MOUSE_GRAB"; }
    if (BITMASK_IS_1(options, Node::OPTION_m_NO_MOUSE_GRAB)     )  { msg << " m_NO_MOUSE_GRAB"; }
    if (BITMASK_IS_1(options, Node::OPTION_f_NO_SPARKS)         )  { msg << " f_NO_SPARKS"; }
    if (BITMASK_IS_1(options, Node::OPTION_x_EXHAUST_POINT)     )  { msg << " x_EXHAUST_POINT"; }
    if (BITMASK_IS_1(options, Node::OPTION_y_EXHAUST_DIRECTION) )  { msg << " y_EXHAUST_DIRECTION"; }
    if (BITMASK_IS_1(options, Node::OPTION_c_NO_GROUND_CONTACT) )  { msg << " c_NO_GROUND_CONTACT"; }
    if (BITMASK_IS_1(options, Node::OPTION_h_HOOK_POINT)        )  { msg << " h_HOOK_POINT"; }
    if (BITMASK_IS_1(options, Node::OPTION_e_TERRAIN_EDIT_POINT))  { msg << " e_TERRAIN_EDIT_POINT"; }
    if (BITMASK_IS_1(options, Node::OPTION_b_EXTRA_BUOYANCY)    )  { msg << " b_EXTRA_BUOYANCY"; }
    if (BITMASK_IS_1(options, Node::OPTION_p_NO_PARTICLES)      )  { msg << " p_NO_PARTICLES"; }
    if (BITMASK_IS_1(options, Node::OPTION_L_LOG)               )  { msg << " L_LOG"; }

    this->AddMessage(m_current_line, Message::TYPE_WARNING, msg.str());
}

void Parser::ParseDirectiveSetNodeDefaults()
{
    if (!this->CheckNumArguments(2)) { return; }

    float load_weight   =                    this->GetArgFloat(1);
    float friction      = (m_num_args > 2) ? this->GetArgFloat(2) : -1;
    float volume        = (m_num_args > 3) ? this->GetArgFloat(3) : -1;
    float surface       = (m_num_args > 4) ? this->GetArgFloat(4) : -1;
    std::string opt_str = (m_num_args > 5) ? this->GetArgStr  (5) : "";

    m_user_node_defaults = std::shared_ptr<NodeDefaults>( new NodeDefaults(*m_user_node_defaults) );

    m_user_node_defaults->load_weight = (load_weight < 0) ? m_ror_node_defaults->load_weight : load_weight;
    m_user_node_defaults->friction    = (friction    < 0) ? m_ror_node_defaults->friction    : friction;
    m_user_node_defaults->volume      = (volume      < 0) ? m_ror_node_defaults->volume      : volume;
    m_user_node_defaults->surface     = (surface     < 0) ? m_ror_node_defaults->surface     : surface;

    this->_ParseNodeOptions(m_user_node_defaults->options, opt_str);

    // Disabled until someone wants it back
    //this->LogParsedDirectiveSetNodeDefaultsData(load_weight, friction, volume, surface, m_user_node_defaults->options);
}

void Parser::_ParseNodeOptions(unsigned int & options, const std::string & options_str)
{
    options = 0;

    for (unsigned int i = 0; i < options_str.length(); i++)
    {
        const char c = options_str.at(i);
        switch(c)
        {
            case 'l':
                BITMASK_SET_1(options, Node::OPTION_l_LOAD_WEIGHT);
                break;
            case 'n':
                BITMASK_SET_1(options, Node::OPTION_n_MOUSE_GRAB);
                BITMASK_SET_0(options, Node::OPTION_m_NO_MOUSE_GRAB);
                break;
            case 'm':
                BITMASK_SET_1(options, Node::OPTION_m_NO_MOUSE_GRAB);
                BITMASK_SET_0(options, Node::OPTION_n_MOUSE_GRAB);
                break;
            case 'f':
                BITMASK_SET_1(options, Node::OPTION_f_NO_SPARKS);
                break;
            case 'x':
                BITMASK_SET_1(options, Node::OPTION_x_EXHAUST_POINT);
                break;
            case 'y':
                BITMASK_SET_1(options, Node::OPTION_y_EXHAUST_DIRECTION);
                break;
            case 'c':
                BITMASK_SET_1(options, Node::OPTION_c_NO_GROUND_CONTACT);
                break;
            case 'h':
                BITMASK_SET_1(options, Node::OPTION_h_HOOK_POINT);
                break;
            case 'e':
                BITMASK_SET_1(options, Node::OPTION_e_TERRAIN_EDIT_POINT);
                break;
            case 'b':
                BITMASK_SET_1(options, Node::OPTION_b_EXTRA_BUOYANCY);
                break;
            case 'p':
                BITMASK_SET_1(options, Node::OPTION_p_NO_PARTICLES);
                break;
            case 'L':
                BITMASK_SET_1(options, Node::OPTION_L_LOG);
                break;

            default:
                this->AddMessage(options_str, Message::TYPE_WARNING, std::string("Ignoring invalid option: ") + c);
                break;
        }
    }
}

void Parser::ParseDirectiveSetManagedMaterialsOptions()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg
    
    // This is what v0.3x's parser did.
    char c = this->GetArgChar(1);
    m_current_managed_material_options.double_sided = (c != '0');

    if (c != '0' && c != '1')
    {
        this->AddMessage(Message::TYPE_WARNING,
            "Param 'doublesided' should be only 1 or 0, got '" + this->GetArgStr(1) + "', parsing as 0");
    }
}

void Parser::ParseDirectiveSetBeamDefaultsScale()
{
    if (! this->CheckNumArguments(5)) { return; }
    
    BeamDefaults* b = new BeamDefaults(*m_user_beam_defaults);
    b->scale.springiness = this->GetArgFloat(1);
    
    if (m_num_args > 2) { b->scale.damping_constant               = this->GetArgFloat(2); }
    if (m_num_args > 3) { b->scale.deformation_threshold_constant = this->GetArgFloat(3); }
    if (m_num_args > 4) { b->scale.breaking_threshold_constant    = this->GetArgFloat(4); }

    m_user_beam_defaults = std::shared_ptr<BeamDefaults>(b);
}

void Parser::ParseDirectiveSetBeamDefaults()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    BeamDefaults d(*m_user_beam_defaults);

    // What's the state of "enable_advanced_deformation" feature at this point?
    // Directive "enable_advanced_deformation" alters the effects of BeamDefaults
    // Since the old parser worked on-the-fly, only BeamDefaults defined after the directive were affected

    d._enable_advanced_deformation = m_definition->enable_advanced_deformation;

    d._is_user_defined = true; //The "_enable_advanced_deformation" must only be aplied to user-defined values, not defaults.
    d.springiness      = this->GetArgFloat(1);

    if (m_num_args > 2) { d.damping_constant       = this->GetArgFloat(2); }
    if (m_num_args > 3) { d.deformation_threshold  = this->GetArgFloat(3); }
    if (m_num_args > 4) { d.breaking_threshold     = this->GetArgFloat(4); }
    if (m_num_args > 5) { d.visual_beam_diameter   = this->GetArgFloat(5); }
    if (m_num_args > 6) { d.beam_material_name     = this->GetArgStr  (6); }
    if (m_num_args > 7) { d.plastic_deform_coef    = this->GetArgFloat(7); }

    if (m_num_args > 7 && d.plastic_deform_coef >= 0.0f) { d._is_plastic_deform_coef_user_defined = true; }

    if (d.springiness           < 0.f) { d.springiness           = DEFAULT_SPRING;                              }
    if (d.damping_constant      < 0.f) { d.damping_constant      = DEFAULT_DAMP;                                }
    if (d.deformation_threshold < 0.f) { d.deformation_threshold = BEAM_DEFORM;                                 }
    if (d.breaking_threshold    < 0.f) { d.breaking_threshold    = BEAM_BREAK;                                  }
    if (d.visual_beam_diameter  < 0.f) { d.visual_beam_diameter  = DEFAULT_BEAM_DIAMETER;                       }
    if (d.plastic_deform_coef   < 0.f) { d.plastic_deform_coef   = (*m_user_beam_defaults).plastic_deform_coef; }

    m_user_beam_defaults = std::shared_ptr<BeamDefaults>( new BeamDefaults(d) );
    return;
}

void Parser::ParseDirectivePropCameraMode()
{
    assert(m_current_module != nullptr);
    if (m_current_module->props.size() == 0)
    {
        this->AddMessage(Message::TYPE_ERROR, "Directive 'prop_camera_mode' found but no 'prop' defined, ignoring...");
        return;
    }

    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    this->_ParseCameraSettings(m_current_module->props.back().camera_settings, this->GetArgStr(1));
}

void Parser::ParseDirectiveBackmesh()
{
    if (m_current_section == File::SECTION_SUBMESH)
    {
        m_current_submesh->backmesh = true;
    }
    else
    {
        this->AddMessage(Message::TYPE_ERROR, "Misplaced sub-directive 'backmesh' (belongs in section 'submesh'), ignoring...");
    }
}

void Parser::ProcessKeywordTexcoords()
{
    if (m_current_section == File::SECTION_SUBMESH)
    {
        m_current_subsection = File::SUBSECTION__SUBMESH__TEXCOORDS;
    }
    else
    {
        this->AddMessage(Message::TYPE_WARNING, "Misplaced sub-section 'texcoords' (belongs in section 'submesh'), falling back to classic unsafe parsing method.");
        this->ChangeSection(File::SECTION_SUBMESH);
    }
}

void Parser::ProcessKeywordCab()
{
    m_current_subsection = File::SUBSECTION__SUBMESH__CAB;
    if (m_current_section != File::SECTION_SUBMESH)
    {
        this->AddMessage(Message::TYPE_WARNING, "Misplaced sub-section 'cab' (belongs in section 'submesh')");
        this->ChangeSection(File::SECTION_SUBMESH);
    }
}

void Parser::ProcessGlobalDirective(File::Keyword keyword)   // Directives that should only appear in root module
{
    this->VerifyModuleIsRoot(keyword); // Reports warning message if we're not in root module

    switch (keyword)
    {
    case File::KEYWORD_DISABLEDEFAULTSOUNDS:      m_definition->disable_default_sounds = true;        return;
    case File::KEYWORD_ENABLE_ADVANCED_DEFORM:    m_definition->enable_advanced_deformation = true;   return;
    case File::KEYWORD_FORWARDCOMMANDS:           m_definition->forward_commands = true;              return;
    case File::KEYWORD_HIDE_IN_CHOOSER:           m_definition->hide_in_chooser = true;               return;
    case File::KEYWORD_LOCKGROUP_DEFAULT_NOLOCK:  m_definition->lockgroup_default_nolock = true;      return;
    case File::KEYWORD_RESCUER:                   m_definition->rescuer = true;                       return;
    case File::KEYWORD_ROLLON:                    m_definition->rollon = true;                        return;
    case File::KEYWORD_SLIDENODE_CONNECT_INSTANT: m_definition->slide_nodes_connect_instantly = true; return;

    default: this->AddMessage(Message::TYPE_ERROR, "INTERNAL ERROR: '"
                 + std::string(File::KeywordToString(keyword)) + "' is not a global directive");      return;
    }
}

void Parser::VerifyModuleIsRoot(File::Keyword keyword)
{
    if (m_current_module != m_root_module)
    {
        char buf[200];
        snprintf(buf, 200, "Keyword '%s' has global effect and should not appear in a module", File::KeywordToString(keyword));
        this->AddMessage(Message::TYPE_WARNING, buf);
    }
}

void Parser::ParseMeshWheelUnified()
{
    if (! this->CheckNumArguments(16)) { return; }

    MeshWheel mesh_wheel;
    mesh_wheel._is_meshwheel2     = (m_current_section == File::SECTION_MESH_WHEELS_2);
    mesh_wheel.node_defaults      = m_user_node_defaults;
    mesh_wheel.beam_defaults      = m_user_beam_defaults;

    mesh_wheel.tyre_radius        = this->GetArgFloat        ( 0);
    mesh_wheel.rim_radius         = this->GetArgFloat        ( 1);
    mesh_wheel.width              = this->GetArgFloat        ( 2);
    mesh_wheel.num_rays           = this->GetArgInt          ( 3);
    mesh_wheel.nodes[0]           = this->GetArgNodeRef      ( 4);
    mesh_wheel.nodes[1]           = this->GetArgNodeRef      ( 5);
    mesh_wheel.rigidity_node      = this->GetArgRigidityNode ( 6);
    mesh_wheel.braking            = this->GetArgBraking      ( 7);
    mesh_wheel.propulsion         = this->GetArgPropulsion   ( 8);
    mesh_wheel.reference_arm_node = this->GetArgNodeRef      ( 9);
    mesh_wheel.mass               = this->GetArgFloat        (10);
    mesh_wheel.spring             = this->GetArgFloat        (11);
    mesh_wheel.damping            = this->GetArgFloat        (12);
    mesh_wheel.side               = this->GetArgWheelSide    (13);
    mesh_wheel.mesh_name          = this->GetArgStr          (14);
    mesh_wheel.material_name      = this->GetArgStr          (15);

    if (m_sequential_importer.IsEnabled())
    {
        File::Keyword keyword = (mesh_wheel._is_meshwheel2)
            ? File::KEYWORD_MESHWHEELS2
            : File::KEYWORD_MESHWHEELS;
        m_sequential_importer.GenerateNodesForWheel(keyword, mesh_wheel.num_rays, mesh_wheel.rigidity_node.IsValidAnyState());
    }

    m_current_module->mesh_wheels.push_back(mesh_wheel);
}

void Parser::ParseHook()
{
    if (! this->CheckNumArguments(1)) { return; }

    Hook hook;
    hook.node = this->GetArgNodeRef(0);

    int i = 1;
    while (i < m_num_args)
    {
        std::string attr = this->GetArgStr(i);
        Ogre::StringUtil::trim(attr);
        const bool has_value = (i < (m_num_args - 1));

        // Values
             if (has_value && (attr == "hookrange")                          ) { hook.option_hook_range       = this->GetArgFloat(++i); }
        else if (has_value && (attr == "speedcoef")                          ) { hook.option_speed_coef       = this->GetArgFloat(++i); }
        else if (has_value && (attr == "maxforce")                           ) { hook.option_max_force        = this->GetArgFloat(++i); }
        else if (has_value && (attr == "timer")                              ) { hook.option_timer            = this->GetArgFloat(++i); }
        else if (has_value && (attr == "hookgroup"  || attr == "hgroup")     ) { hook.option_hookgroup        = this->GetArgInt  (++i); }
        else if (has_value && (attr == "lockgroup"  || attr == "lgroup")     ) { hook.option_lockgroup        = this->GetArgInt  (++i); }
        else if (has_value && (attr == "shortlimit" || attr == "short_limit")) { hook.option_min_range_meters = this->GetArgFloat(++i); }
        // Flags
        else if ((attr == "selflock") ||(attr == "self-lock") ||(attr == "self_lock") ) { hook.flag_self_lock  = true; }
        else if ((attr == "autolock") ||(attr == "auto-lock") ||(attr == "auto_lock") ) { hook.flag_auto_lock  = true; }
        else if ((attr == "nodisable")||(attr == "no-disable")||(attr == "no_disable")) { hook.flag_no_disable = true; }
        else if ((attr == "norope")   ||(attr == "no-rope")   ||(attr == "no_rope")   ) { hook.flag_no_rope    = true; }
        else if ((attr == "visible")  ||(attr == "vis")                               ) { hook.flag_visible    = true; }
        else
        {
            std::string msg = "Ignoring invalid option: " + attr;
            this->AddMessage(Message::TYPE_ERROR, msg.c_str());
        }
        i++;
    }

    m_current_module->hooks.push_back(hook);
}

void Parser::ParseHelp()
{
    if (! m_current_module->help_panel_material_name.empty())
    {
        this->AddMessage(Message::TYPE_WARNING, "Secton defined more than once.");
    }

    m_current_module->help_panel_material_name = m_current_line;
    Ogre::StringUtil::trim(m_current_module->help_panel_material_name);
}

void Parser::ParseGuiSettings()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    GuiSettings* gui_settings = m_current_module->gui_settings.get();
   
    std::string key = this->GetArgStr(0);
    
    if (key == "debugBeams") {} // Obsolete, ignored
    
    else if (key == "tachoMaterial")  { gui_settings->tacho_material     =  this->GetArgStr(1); }
    else if (key == "speedoMaterial") { gui_settings->speedo_material    =  this->GetArgStr(1); }
    else if (key == "ar_speedo_max_kph")      { gui_settings->speedo_highest_kph =  this->GetArgInt(1); }
    else if (key == "useMaxRPM")      { gui_settings->use_max_rpm        = (this->GetArgInt(1) == 1); }
    else if (key == "helpMaterial")   { gui_settings->help_material      =  this->GetArgStr(1); }
    
    else if (key == "dashboard")        { gui_settings->dashboard_layouts    .push_back(this->GetArgStr(1)); }
    else if (key == "texturedashboard") { gui_settings->rtt_dashboard_layouts.push_back(this->GetArgStr(1)); }
    
    else if (key == "interactiveOverviewMap")
    {
        std::string val = this->GetArgStr(1);
        
             if (val == "off"   ) { gui_settings->interactive_overview_map_mode = GuiSettings::MAP_MODE_OFF;    }
        else if (val == "simple") { gui_settings->interactive_overview_map_mode = GuiSettings::MAP_MODE_SIMPLE; }
        else if (val == "zoom"  ) { gui_settings->interactive_overview_map_mode = GuiSettings::MAP_MODE_ZOOM;   }
        
        else { this->AddMessage(Message::TYPE_ERROR, "Unknown map mode [" + val + "], ignoring..."); }
    }
    else
    {
        this->AddMessage(Message::TYPE_ERROR, "Unknown setting [" + key + "], ignoring...");
    }
}

void Parser::ParseGuid()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    if (! m_definition->guid.empty())
    {
        this->AddMessage(Message::TYPE_WARNING, "Guid defined multiple times.");
    }

    m_definition->guid = this->GetArgStr(1);
}

void Parser::ParseGlobals()
{
    if (! this->CheckNumArguments(2)) { return; }

    if (m_current_module->globals != nullptr)
    {
        this->AddMessage(Message::TYPE_WARNING, "Globals defined more than once.");
    }

    Globals globals;
    globals.dry_mass   = this->GetArgFloat(0);
    globals.cargo_mass = this->GetArgFloat(1);

    if (m_num_args > 2) { globals.material_name = this->GetArgStr(2); }

    m_current_module->globals = std::shared_ptr<Globals>( new Globals(globals) );
}

void Parser::ParseFusedrag()
{
    if (! this->CheckNumArguments(3)) { return; }

    Fusedrag fusedrag;
    fusedrag.front_node = this->GetArgNodeRef(0);
    fusedrag.rear_node  = this->GetArgNodeRef(1);

    if (this->GetArgStr(2) == "autocalc")
    {
        fusedrag.autocalc = true;

        // Fusedrag autocalculation from truck size
        if (m_num_args > 3) { fusedrag.area_coefficient = this->GetArgFloat(3); }
        if (m_num_args > 4) { fusedrag.airfoil_name     = this->GetArgStr  (4); }
    }
    else
    {
        // Original calculation
        fusedrag.approximate_width = this->GetArgFloat(2);
        
        if (m_num_args > 3) { fusedrag.airfoil_name = this->GetArgStr(3); }
    }

    m_current_module->fusedrag.push_back(fusedrag);
}

void Parser::_ParseCameraSettings(CameraSettings & camera_settings, Ogre::String input_str)
{
    int input = STR_PARSE_INT(input_str);
    if (input >= 0)
    {
        camera_settings.mode = CameraSettings::MODE_CINECAM;
        camera_settings.cinecam_index = input;
    }
    else if (input >= -2)
    {
        camera_settings.mode = CameraSettings::Mode(input);
    }
    else
    {
        AddMessage(input_str, Message::TYPE_ERROR, "Invalid value of camera setting, ignoring...");
        return;
    }
}

void Parser::ParseDirectiveFlexbodyCameraMode()
{
    if (m_last_flexbody == nullptr)
    {
        this->AddMessage(Message::TYPE_ERROR, "No flexbody to update, ignoring...");
        return;
    }

    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    this->_ParseCameraSettings(m_last_flexbody->camera_settings, this->GetArgStr(1));
}

void Parser::ParseSubmesh()
{
    if (m_current_subsection == File::SUBSECTION__SUBMESH__CAB)
    {
        if (! this->CheckNumArguments(3)) { return; }

        Cab cab;
        cab.nodes[0] = this->GetArgNodeRef(0);
        cab.nodes[1] = this->GetArgNodeRef(1);
        cab.nodes[2] = this->GetArgNodeRef(2);
        if (m_num_args > 3)
        {
            cab.options = 0;
            std::string options_str = this->GetArgStr(3);
            for (unsigned int i = 0; i < options_str.length(); i++)
            {
                switch (options_str.at(i))
                {
                case 'c': cab.options |=  Cab::OPTION_c_CONTACT;                               break;
                case 'b': cab.options |=  Cab::OPTION_b_BUOYANT;                               break;
                case 'D': cab.options |= (Cab::OPTION_c_CONTACT      | Cab::OPTION_b_BUOYANT); break;
                case 'p': cab.options |=  Cab::OPTION_p_10xTOUGHER;                            break;
                case 'u': cab.options |=  Cab::OPTION_u_INVULNERABLE;                          break;
                case 'F': cab.options |= (Cab::OPTION_p_10xTOUGHER   | Cab::OPTION_b_BUOYANT); break;
                case 'S': cab.options |= (Cab::OPTION_u_INVULNERABLE | Cab::OPTION_b_BUOYANT); break; 
                case 'n': break; // Placeholder, does nothing 

                default:
                    char msg[200] = "";
                    snprintf(msg, 200, "'submesh/cab' Ignoring invalid option '%c'...", options_str.at(i));
                    this->AddMessage(Message::TYPE_WARNING, msg);
                    break;
                }
            }
        }

        m_current_submesh->cab_triangles.push_back(cab);
    }
    else if (m_current_subsection == File::SUBSECTION__SUBMESH__TEXCOORDS)
    {
        if (! this->CheckNumArguments(3)) { return; }

        Texcoord texcoord;
        texcoord.node = this->GetArgNodeRef(0);
        texcoord.u    = this->GetArgFloat  (1);
        texcoord.v    = this->GetArgFloat  (2);

        m_current_submesh->texcoords.push_back(texcoord);
    }
    else
    {
        AddMessage(Message::TYPE_ERROR, "Section submesh has no subsection defined, line not parsed.");
    }
}

void Parser::ParseFlexbody()
{
    if (m_current_subsection == File::SUBSECTION__FLEXBODIES__PROPLIKE_LINE)
    {
        if (! this->CheckNumArguments(10)) { return; }

        Flexbody flexbody;
        flexbody.reference_node = this->GetArgNodeRef (0);
        flexbody.x_axis_node    = this->GetArgNodeRef (1);
        flexbody.y_axis_node    = this->GetArgNodeRef (2);
        flexbody.offset.x       = this->GetArgFloat   (3);
        flexbody.offset.y       = this->GetArgFloat   (4);
        flexbody.offset.z       = this->GetArgFloat   (5);
        flexbody.rotation.x     = this->GetArgFloat   (6);
        flexbody.rotation.y     = this->GetArgFloat   (7);
        flexbody.rotation.z     = this->GetArgFloat   (8);
        flexbody.mesh_name      = this->GetArgStr     (9);

        m_last_flexbody = std::shared_ptr<Flexbody>( new Flexbody(flexbody) );
        m_current_module->flexbodies.push_back(m_last_flexbody);

        // Switch subsection
        m_current_subsection =  File::SUBSECTION__FLEXBODIES__FORSET_LINE;
    }
    else if (m_current_subsection == File::SUBSECTION__FLEXBODIES__FORSET_LINE)
    {
        // Syntax: "forset", followed by space/comma, followed by ","-separated items.
        // Acceptable item forms:
        // * Single node number / node name
        // * Pair of node numbers:" 123 - 456 ". Whitespace is optional.

        char setdef[LINE_BUFFER_LENGTH] = ""; // strtok() is destructive, we need own buffer.
        strncpy(setdef, m_current_line + 6, LINE_BUFFER_LENGTH - 6); // Cut away "forset"
        const char* item = std::strtok(setdef, ",");

        // TODO: Add error reporting
        // It appears strtoul() sets no ERRNO for input 'x1' (parsed -> '0')

        const ptrdiff_t MAX_ITEM_LEN = 200;
        while (item != nullptr)
        {
            const char* hyphen = strchr(item, '-');
            if (hyphen != nullptr)
            {
                unsigned a = 0; 
                char* a_end = nullptr;
                std::string a_text;
                std::string b_text;
                if (hyphen != item)
                {
                    a = ::strtoul(item, &a_end, 10);
                    size_t length = std::min(a_end - item, MAX_ITEM_LEN);
                    a_text = std::string(item, length);
                }
                char* b_end = nullptr;
                const char* item2 = hyphen + 1;
                unsigned b = ::strtoul(item2, &b_end, 10);
                size_t length = std::min(b_end - item2, MAX_ITEM_LEN);
                b_text = std::string(item2, length);

                // Add interval [a-b]
                m_last_flexbody->node_list_to_import.push_back(
                    Node::Range(
                        Node::Ref(a_text, a, Node::Ref::IMPORT_STATE_IS_VALID, m_current_line_number),
                        Node::Ref(b_text, b, Node::Ref::IMPORT_STATE_IS_VALID, m_current_line_number)));
            }
            else
            {
                errno = 0;
                unsigned a = 0;
                a = ::strtoul(item, nullptr, 10);
                // Add interval [a-a]
                Node::Range range_a = Node::Range(Node::Ref(std::string(item), a, Node::Ref::IMPORT_STATE_IS_VALID, m_current_line_number));
                m_last_flexbody->node_list_to_import.push_back(range_a);
            }
            item = strtok(nullptr, ",");
        }

        // Switch subsection 
        m_current_subsection =  File::SUBSECTION__FLEXBODIES__PROPLIKE_LINE;
    }
    else
    {
        AddMessage(Message::TYPE_FATAL_ERROR, "Internal parser failure, section 'flexbodies' not parsed.");
    }
}

void Parser::ParseFlaresUnified()
{
    const bool is_flares2 = (m_current_section == File::SECTION_FLARES_2);
    if (! this->CheckNumArguments(is_flares2 ? 6 : 5)) { return; }

    Flare2 flare2;
    int pos = 0;
    flare2.reference_node = this->GetArgNodeRef(pos++);
    flare2.node_axis_x    = this->GetArgNodeRef(pos++);
    flare2.node_axis_y    = this->GetArgNodeRef(pos++);
    flare2.offset.x       = this->GetArgFloat  (pos++);
    flare2.offset.y       = this->GetArgFloat  (pos++);

    if (m_current_section == File::SECTION_FLARES_2)
    {
        flare2.offset.z = this->GetArgFloat(pos++);
    }

    if (m_num_args > pos) { flare2.type              = this->GetArgFlareType(pos++); }
    if (m_num_args > pos) { flare2.control_number    = this->GetArgInt      (pos++); }
    if (m_num_args > pos) { flare2.blink_delay_milis = this->GetArgInt      (pos++); }
    if (m_num_args > pos) { flare2.size              = this->GetArgFloat    (pos++); }
    if (m_num_args > pos) { flare2.material_name     = this->GetArgStr      (pos++); }

    m_current_module->flares_2.push_back(flare2);
}

void Parser::ParseFixes()
{
    m_current_module->fixes.push_back(this->GetArgNodeRef(0));
}

void Parser::ParseExtCamera()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    if (m_current_module->ext_camera == nullptr)
    {
        m_current_module->ext_camera = std::shared_ptr<RigDef::ExtCamera>( new RigDef::ExtCamera() );
    }
    ExtCamera* extcam = m_current_module->ext_camera.get();
    
    auto mode_str = this->GetArgStr(1);
    if (mode_str == "classic")
    {
        extcam->mode = ExtCamera::MODE_CLASSIC;
    }
    else if (mode_str == "cinecam")
    {
        extcam->mode = ExtCamera::MODE_CINECAM;
    }
    else if ((mode_str == "node") && (m_num_args > 2))
    {
        extcam->mode = ExtCamera::MODE_NODE;
        extcam->node = this->GetArgNodeRef(2);
    }
}

void Parser::ParseExhaust()
{
    if (! this->CheckNumArguments(2)) { return; }

    Exhaust exhaust;
    exhaust.reference_node = this->GetArgNodeRef(0);
    exhaust.direction_node = this->GetArgNodeRef(1);
    
    // Param [2] is unused
    if (m_num_args > 3) { exhaust.particle_name = this->GetArgStr(3); }

    m_current_module->exhausts.push_back(exhaust);
}

void Parser::ParseFileFormatVersion()
{
    if (! this->CheckNumArguments(2)) { return; }

    if (m_current_module != m_root_module)
    {
        this->AddMessage(Message::TYPE_WARNING, "Inline section 'fileformatversion' has global effect and should not appear in a module");
    }

    m_definition->file_format_version = this->GetArgUint(1);

    if (m_definition->file_format_version >= 450)
    {
        m_sequential_importer.Disable();
    }

    this->ChangeSection(File::SECTION_NONE);
}

void Parser::ParseDirectiveDetacherGroup()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, param

    if (this->GetArgStr(1) == "end")
    {
        m_current_detacher_group = 0;
    }
    else
    {
        m_current_detacher_group = this->GetArgInt(1);
    }
}

void Parser::ParseCruiseControl()
{
    if (! this->CheckNumArguments(3)) { return; } // keyword + 2 params

    CruiseControl cruise_control;
    cruise_control.min_speed = this->GetArgFloat(1);
    cruise_control.autobrake = this->GetArgInt(2);

    if (m_current_module->cruise_control != nullptr)
    {
        this->AddMessage(Message::TYPE_WARNING, "Directive 'CruiseControl' used multiple times.");
    }
    m_current_module->cruise_control = std::shared_ptr<CruiseControl>( new CruiseControl(cruise_control) );
}

void Parser::ParseDirectiveAddAnimation()
{
    if (m_current_module->props.size() == 0)
    {
        AddMessage(Message::TYPE_ERROR, "Directive 'add_animation' has no prop to animate, ignoring...");
        return;
    }

    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 14, ","); // "add_animation " = 14 characters

    if (tokens.size() < 4)
    {
        AddMessage(Message::TYPE_ERROR, "Not enough arguments, skipping...");
        return;
    }

    Animation animation;
    animation.ratio       = this->ParseArgFloat(tokens[0].c_str());
    animation.lower_limit = this->ParseArgFloat(tokens[1].c_str());
    animation.upper_limit = this->ParseArgFloat(tokens[2].c_str());

    for (auto itor = tokens.begin() + 3; itor != tokens.end(); ++itor)
    {
        Ogre::StringVector entry = Ogre::StringUtil::split(*itor, ":");
        Ogre::StringUtil::trim(entry[0]);
        if (entry.size() > 1) Ogre::StringUtil::trim(entry[1]); 

        const int WARN_LEN = 500;
        char warn_msg[WARN_LEN] = "";

        if (entry.size() == 1) // Single keyword
        {
            if      (entry[0] == "autoanimate") { animation.mode |= Animation::MODE_AUTO_ANIMATE; }
            else if (entry[0] == "noflip")      { animation.mode |= Animation::MODE_NO_FLIP; }
            else if (entry[0] == "bounce")      { animation.mode |= Animation::MODE_BOUNCE; }
            else if (entry[0] == "eventlock")   { animation.mode |= Animation::MODE_EVENT_LOCK; }

            else { snprintf(warn_msg, WARN_LEN, "Invalid keyword: %s", entry[0].c_str()); }
        }
        else if (entry.size() == 2 && (entry[0] == "mode" || entry[0] == "event" || entry[0] == "source"))
        {
            Ogre::StringVector values = Ogre::StringUtil::split(entry[1], "|");
            if (entry[0] == "mode")
            {
                for (auto itor = values.begin(); itor != values.end(); ++itor)
                {
                    std::string value = *itor;
                    Ogre::StringUtil::trim(value);

                         if (value == "x-rotation") { animation.mode |= Animation::MODE_ROTATION_X; }
                    else if (value == "y-rotation") { animation.mode |= Animation::MODE_ROTATION_Y; }
                    else if (value == "z-rotation") { animation.mode |= Animation::MODE_ROTATION_Z; }
                    else if (value == "x-offset"  ) { animation.mode |= Animation::MODE_OFFSET_X;   }
                    else if (value == "y-offset"  ) { animation.mode |= Animation::MODE_OFFSET_Y;   }
                    else if (value == "z-offset"  ) { animation.mode |= Animation::MODE_OFFSET_Z;   }

                    else { snprintf(warn_msg, WARN_LEN, "Invalid 'mode': %s, ignoring...", entry[1].c_str()); }
                }
            }
            else if (entry[0] == "event")
            {
                animation.event = entry[1];
                Ogre::StringUtil::trim(animation.event);
                Ogre::StringUtil::toUpperCase(animation.event);
            }
            else if (entry[0] == "source")
            {
                for (auto itor = values.begin(); itor != values.end(); ++itor)
                {
                    std::string value = *itor;
                    Ogre::StringUtil::trim(value);

                         if (value == "airspeed")      { animation.source |= Animation::SOURCE_AIRSPEED;          }
                    else if (value == "vvi")           { animation.source |= Animation::SOURCE_VERTICAL_VELOCITY; }
                    else if (value == "altimeter100k") { animation.source |= Animation::SOURCE_ALTIMETER_100K;    }
                    else if (value == "altimeter10k")  { animation.source |= Animation::SOURCE_ALTIMETER_10K;     }
                    else if (value == "altimeter1k")   { animation.source |= Animation::SOURCE_ALTIMETER_1K;      }
                    else if (value == "aoa")           { animation.source |= Animation::SOURCE_ANGLE_OF_ATTACK;   }
                    else if (value == "flap")          { animation.source |= Animation::SOURCE_FLAP;              }
                    else if (value == "airbrake")      { animation.source |= Animation::SOURCE_AIR_BRAKE;         }
                    else if (value == "roll")          { animation.source |= Animation::SOURCE_ROLL;              }
                    else if (value == "pitch")         { animation.source |= Animation::SOURCE_PITCH;             }
                    else if (value == "brakes")        { animation.source |= Animation::SOURCE_BRAKES;            }
                    else if (value == "accel")         { animation.source |= Animation::SOURCE_ACCEL;             }
                    else if (value == "clutch")        { animation.source |= Animation::SOURCE_CLUTCH;            }
                    else if (value == "speedo")        { animation.source |= Animation::SOURCE_SPEEDO;            }
                    else if (value == "tacho")         { animation.source |= Animation::SOURCE_TACHO;             }
                    else if (value == "turbo")         { animation.source |= Animation::SOURCE_TURBO;             }
                    else if (value == "parking")       { animation.source |= Animation::SOURCE_PARKING;           }
                    else if (value == "shifterman1")   { animation.source |= Animation::SOURCE_SHIFT_LEFT_RIGHT;  }
                    else if (value == "shifterman2")   { animation.source |= Animation::SOURCE_SHIFT_BACK_FORTH;  }
                    else if (value == "sequential")    { animation.source |= Animation::SOURCE_SEQUENTIAL_SHIFT;  }
                    else if (value == "shifterlin")    { animation.source |= Animation::SOURCE_SHIFTERLIN;        }
                    else if (value == "torque")        { animation.source |= Animation::SOURCE_TORQUE;            }
                    else if (value == "heading")       { animation.source |= Animation::SOURCE_HEADING;           }
                    else if (value == "difflock")      { animation.source |= Animation::SOURCE_DIFFLOCK;          }
                    else if (value == "rudderboat")    { animation.source |= Animation::SOURCE_BOAT_RUDDER;       }
                    else if (value == "throttleboat")  { animation.source |= Animation::SOURCE_BOAT_THROTTLE;     }
                    else if (value == "steeringwheel") { animation.source |= Animation::SOURCE_STEERING_WHEEL;    }
                    else if (value == "aileron")       { animation.source |= Animation::SOURCE_AILERON;           }
                    else if (value == "elevator")      { animation.source |= Animation::SOURCE_ELEVATOR;          }
                    else if (value == "rudderair")     { animation.source |= Animation::SOURCE_AIR_RUDDER;        }
                    else if (value == "permanent")     { animation.source |= Animation::SOURCE_PERMANENT;         }
                    else if (value == "event")         { animation.source |= Animation::SOURCE_EVENT;             }

                    else
                    {
                        Animation::MotorSource motor_source;
                        if (entry[1].compare(0, 8, "throttle") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_THROTTLE;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(8));
                        }
                        else if (entry[1].compare(0, 3, "rpm") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_RPM;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(3));
                        }
                        else if (entry[1].compare(0, 8, "aerotorq") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_TORQUE;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(8));
                        }
                        else if (entry[1].compare(0, 7, "aeropit") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_PITCH;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(7));
                        }
                        else if (entry[1].compare(0, 10, "aerostatus") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_STATUS;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(10));
                        }
                        else
                        {
                            snprintf(warn_msg, WARN_LEN, "Invalid 'source': %s, ignoring...", entry[1].c_str());
                            continue;
                        }
                        animation.motor_sources.push_back(motor_source);
                    }
                }
            }
            else
            {
                snprintf(warn_msg, WARN_LEN, "Invalid keyword: %s, ignoring...", entry[0].c_str());
            }
        }
        else
        {
            snprintf(warn_msg, WARN_LEN, "Invalid item: %s, ignoring...", entry[0].c_str());
        }

        if (warn_msg[0] != '\0')
        {
            char msg[WARN_LEN + 100];
            snprintf(msg, WARN_LEN + 100, "Invalid token: %s (%s) ignoring....", itor->c_str(), warn_msg);
            this->AddMessage(Message::TYPE_WARNING, msg);
        }
    }

    m_current_module->props.back().animations.push_back(animation);
}

void Parser::ParseAntiLockBrakes()
{
    AntiLockBrakes alb;
    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 15, ","); // "AntiLockBrakes " = 15 characters
    if (tokens.size() < 2)
    {
        this->AddMessage(Message::TYPE_ERROR, "Too few arguments for `AntiLockBrakes`");
        return;
    }

    alb.regulation_force = this->ParseArgFloat(tokens[0].c_str());
    alb.min_speed        = this->ParseArgInt  (tokens[1].c_str());

    if (tokens.size() > 3) { alb.pulse_per_sec = this->ParseArgFloat(tokens[2].c_str()); }

    for (unsigned int i=3; i<tokens.size(); i++)
    {
        Ogre::StringVector args2 = Ogre::StringUtil::split(tokens[i], ":");
        Ogre::StringUtil::trim(args2[0]);
        Ogre::StringUtil::toLowerCase(args2[0]);
        if (args2[0] == "mode" && args2.size() == 2)
        {
            Ogre::StringVector attrs = Ogre::StringUtil::split(args2[1], "&");
            auto itor = attrs.begin();
            auto endi = attrs.end();
            for (; itor != endi; ++itor)
            {
                std::string attr = *itor;
                Ogre::StringUtil::trim(attr);
                Ogre::StringUtil::toLowerCase(attr);
                     if (strncmp(attr.c_str(), "nodash", 6)   == 0) { alb.attr_no_dashboard = true;  }
                else if (strncmp(attr.c_str(), "notoggle", 8) == 0) { alb.attr_no_toggle    = true;  }
                else if (strncmp(attr.c_str(), "on", 2)       == 0) { alb.attr_is_on        = true;  }
                else if (strncmp(attr.c_str(), "off", 3)      == 0) { alb.attr_is_on        = false; }
            }
        }
        else
        {
            this->AddMessage(Message::TYPE_ERROR, "Antilockbrakes Mode: missing");
            alb.attr_no_dashboard = false;
            alb.attr_no_toggle = false;
            alb.attr_is_on = true;
        }
    }

    if (m_current_module->anti_lock_brakes != nullptr)
    {
        this->AddMessage(Message::TYPE_WARNING, "Found multiple sections 'AntiLockBrakes' in one module, using last one.");
    }
    m_current_module->anti_lock_brakes = std::shared_ptr<AntiLockBrakes>( new AntiLockBrakes(alb) );
}

void Parser::ParseEngoption()
{
    if (! this->CheckNumArguments(1)) { return; }

    Engoption engoption;
    engoption.inertia = this->GetArgFloat(0);

    if (m_num_args > 1)
    {
        engoption.type = Engoption::EngineType(this->GetArgChar(1));
    }

    if (m_num_args > 2) { engoption.clutch_force     = this->GetArgFloat(2); }
    if (m_num_args > 3) { engoption.shift_time       = this->GetArgFloat(3); }
    if (m_num_args > 4) { engoption.clutch_time      = this->GetArgFloat(4); }
    if (m_num_args > 5) { engoption.post_shift_time  = this->GetArgFloat(5); }
    if (m_num_args > 6) { engoption.stall_rpm        = this->GetArgFloat(6); }
    if (m_num_args > 7) { engoption.idle_rpm         = this->GetArgFloat(7); }
    if (m_num_args > 8) { engoption.max_idle_mixture = this->GetArgFloat(8); }
    if (m_num_args > 9) { engoption.min_idle_mixture = this->GetArgFloat(9); }
    if (m_num_args > 10){ engoption.braking_torque   = this->GetArgFloat(10);}

    m_current_module->engoption = std::shared_ptr<Engoption>( new Engoption(engoption) );
}

void Parser::ParseEngturbo()
{
    if (! this->CheckNumArguments(4)) { return; }

    Engturbo engturbo;
    engturbo.version        = this->GetArgInt  ( 0);
    engturbo.tinertiaFactor = this->GetArgFloat( 1);
    engturbo.nturbos        = this->GetArgInt  ( 2);
    engturbo.param1         = this->GetArgFloat( 3);

    if (m_num_args >  4) { engturbo.param2  = this->GetArgFloat( 4); }
    if (m_num_args >  5) { engturbo.param3  = this->GetArgFloat( 5); }
    if (m_num_args >  6) { engturbo.param4  = this->GetArgFloat( 6); }
    if (m_num_args >  7) { engturbo.param5  = this->GetArgFloat( 7); }
    if (m_num_args >  8) { engturbo.param6  = this->GetArgFloat( 8); }
    if (m_num_args >  9) { engturbo.param7  = this->GetArgFloat( 9); }
    if (m_num_args > 10) { engturbo.param8  = this->GetArgFloat(10); }
    if (m_num_args > 11) { engturbo.param9  = this->GetArgFloat(11); }
    if (m_num_args > 12) { engturbo.param10 = this->GetArgFloat(12); }
    if (m_num_args > 13) { engturbo.param11 = this->GetArgFloat(13); }

    if (engturbo.nturbos > 4)
    {
        this->AddMessage(Message::TYPE_WARNING, "You cannot have more than 4 turbos. Fallback: using 4 instead.");
        engturbo.nturbos = 4;
    }

    m_current_module->engturbo = std::shared_ptr<Engturbo>(new Engturbo(engturbo));
}

void Parser::ParseEngine()
{
    if (! this->CheckNumArguments(6)) { return; }

    Engine engine;
    engine.shift_down_rpm     = this->GetArgFloat(0);
    engine.shift_up_rpm       = this->GetArgFloat(1);
    engine.torque             = this->GetArgFloat(2);
    engine.global_gear_ratio  = this->GetArgFloat(3);
    engine.reverse_gear_ratio = this->GetArgFloat(4);
    engine.neutral_gear_ratio = this->GetArgFloat(5);

    // Forward gears (max 21)
    int gear_index = 0;
    while ((gear_index < 21) && (m_num_args > (6 + gear_index)))
    {
        float ratio = this->GetArgFloat(gear_index + 6);
        if (ratio < 0.f) { break; } // Optional terminator argument
        engine.gear_ratios.push_back(ratio);
        ++gear_index;
    }

    if (engine.gear_ratios.size() == 0)
    {
        AddMessage(Message::TYPE_ERROR, "Engine has no forward gear, ignoring...");
        return;
    }

    m_current_module->engine = std::shared_ptr<Engine>( new Engine(engine) );
}

void Parser::ParseContacter()
{
    if (! this->CheckNumArguments(1)) { return; }

    m_current_module->contacters.push_back(this->GetArgNodeRef(0));
}

void Parser::ParseCommandsUnified()
{
    const bool is_commands2 = (m_current_section == File::SECTION_COMMANDS_2);
    const int max_args = (is_commands2 ? 8 : 7);
    if (! this->CheckNumArguments(max_args)) { return; }

    Command2 command2;
    command2.beam_defaults     = m_user_beam_defaults;
    command2.detacher_group    = m_current_detacher_group;
    command2._format_version   = (is_commands2) ? 2 : 1;
    command2.inertia_defaults  = m_user_default_inertia;

    int pos = 0;
    command2.nodes[0]          = this->GetArgNodeRef(pos++);
    command2.nodes[1]          = this->GetArgNodeRef(pos++);
    command2.shorten_rate      = this->GetArgFloat  (pos++);

    if (is_commands2)
    {
        command2.lengthen_rate = this->GetArgFloat(pos++);
    }
    else
    {
        command2.lengthen_rate = command2.shorten_rate;
    }

    command2.max_contraction = this->GetArgFloat(pos++);
    command2.max_extension   = this->GetArgFloat(pos++);
    command2.contract_key    = this->GetArgInt  (pos++);
    command2.extend_key      = this->GetArgInt  (pos++);

    if (m_num_args <= max_args) // No more args?
    {
        m_current_module->commands_2.push_back(command2);
        return;
    }

    // Parse options
    const int WARN_LEN = 200;
    char warn_msg[WARN_LEN] = "";
    std::string options_str = this->GetArgStr(pos++);
    char winner = 0;
    for (auto itor = options_str.begin(); itor != options_str.end(); ++itor)
    {
        const char c = *itor;
        if ((winner == 0) && (c == 'o' || c == 'p' || c == 'c')) { winner = c; }
        
             if (c == 'n') {} // Filler, does nothing
        else if (c == 'i') { command2.option_i_invisible     = true; }
        else if (c == 'r') { command2.option_r_rope          = true; }
        else if (c == 'f') { command2.option_f_not_faster    = true; }
        else if (c == 'c') { command2.option_c_auto_center   = true; }
        else if (c == 'p') { command2.option_p_1press        = true; }
        else if (c == 'o') { command2.option_o_1press_center = true; }
        else
        {
            snprintf(warn_msg, WARN_LEN, "Ignoring unknown flag '%c'", c);
            this->AddMessage(Message::TYPE_WARNING, warn_msg);
        }
    }

    // Resolve option conflicts
    if (command2.option_c_auto_center && winner != 'c' && winner != 0)
    {
        AddMessage(Message::TYPE_WARNING, "Command cannot be one-pressed and self centering at the same time, ignoring flag 'c'");
        command2.option_c_auto_center = false;
    }
    char ignored = '\0';
    if (command2.option_o_1press_center && winner != 'o' && winner != 0)
    {
        command2.option_o_1press_center = false;
        ignored = 'o';
    }
    else if (command2.option_p_1press && winner != 'p' && winner != 0)
    {
        command2.option_p_1press = false;
        ignored = 'p';
    }

    // Report conflicts
    if (ignored != 0 && winner == 'c')
    {
        snprintf(warn_msg, WARN_LEN, "Command cannot be one-pressed and self centering at the same time, ignoring flag '%c'", ignored);
        AddMessage(Message::TYPE_WARNING, warn_msg);
    }
    else if (ignored != 0 && (winner == 'o' || winner == 'p'))
    {
        snprintf(warn_msg, WARN_LEN, "Command already has a one-pressed c.mode, ignoring flag '%c'", ignored);
        AddMessage(Message::TYPE_WARNING, warn_msg);
    }

    if (m_num_args > pos) { command2.description   = this->GetArgStr  (pos++);}

    if (m_num_args > pos) { ParseOptionalInertia(command2.inertia, pos); pos += 4; }

    if (m_num_args > pos) { command2.affect_engine = this->GetArgFloat(pos++);}
    if (m_num_args > pos) { command2.needs_engine  = this->GetArgBool (pos++);}
    if (m_num_args > pos) { command2.plays_sound   = this->GetArgBool (pos++);}

    m_current_module->commands_2.push_back(command2);
}

void Parser::ParseCollisionBox()
{
    CollisionBox collisionbox;

    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line, ",");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        collisionbox.nodes.push_back( this->_ParseNodeRef(*iter) );
    }

    m_current_module->collision_boxes.push_back(collisionbox);
}

void Parser::ParseCinecam()
{
    if (! this->CheckNumArguments(11)) { return; }

    Cinecam cinecam;
    cinecam.beam_defaults = m_user_beam_defaults;
    cinecam.node_defaults = m_user_node_defaults;

    // Required arguments
    cinecam.position.x = this->GetArgFloat  ( 0);
    cinecam.position.y = this->GetArgFloat  ( 1);
    cinecam.position.z = this->GetArgFloat  ( 2);
    cinecam.nodes[0]   = this->GetArgNodeRef( 3);
    cinecam.nodes[1]   = this->GetArgNodeRef( 4);
    cinecam.nodes[2]   = this->GetArgNodeRef( 5);
    cinecam.nodes[3]   = this->GetArgNodeRef( 6);
    cinecam.nodes[4]   = this->GetArgNodeRef( 7);
    cinecam.nodes[5]   = this->GetArgNodeRef( 8);
    cinecam.nodes[6]   = this->GetArgNodeRef( 9);
    cinecam.nodes[7]   = this->GetArgNodeRef(10);

    // Optional arguments
    if (m_num_args > 11) { cinecam.spring    = this->GetArgFloat(11); }
    if (m_num_args > 12) { cinecam.damping   = this->GetArgFloat(12); }

    if (m_num_args > 13)
    {
        float value = this->GetArgFloat(13);
        if (value > 0.f) // Invalid input (for example illegal trailing ";pseudo-comment") parses as 0
            cinecam.node_mass = value;
    }

    if (m_sequential_importer.IsEnabled())
    {
        m_sequential_importer.AddGeneratedNode(File::KEYWORD_CINECAM);
    }

    m_current_module->cinecam.push_back(cinecam);
}

void Parser::ParseCameraRails()
{
    m_current_camera_rail->nodes.push_back( this->GetArgNodeRef(0) );
}

void Parser::ParseBrakes()
{
    if (!this->CheckNumArguments(1)) { return; }

    if (m_current_module->brakes == nullptr)
    {
        m_current_module->brakes = std::shared_ptr<Brakes>( new Brakes() );
    }

    m_current_module->brakes->default_braking_force = this->GetArgFloat(0);

    if (m_num_args > 1)
    {
        m_current_module->brakes->parking_brake_force = this->GetArgFloat(1);
    }
}

void Parser::ParseAxles()
{
    Axle axle;

    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line, ",");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        std::smatch results;
        if (! std::regex_search(*iter, results, Regexes::SECTION_AXLES_PROPERTY))
        {
            this->AddMessage(Message::TYPE_ERROR, "Invalid property, ignoring whole line...");
            return;
        }
        // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

        if (results[1].matched)
        {
            unsigned int wheel_index = STR_PARSE_INT(results[2]) - 1;
            axle.wheels[wheel_index][0] = _ParseNodeRef(results[3]);
            axle.wheels[wheel_index][1] = _ParseNodeRef(results[4]);
        }
        else if (results[5].matched)
        {
            std::string options_str = results[6].str();
            for (unsigned int i = 0; i < options_str.length(); i++)
            {
                switch(options_str.at(i))
                {
                    case 'o':
                        axle.options.push_back(Axle::OPTION_o_OPEN);
                        break;
                    case 'l':
                        axle.options.push_back(Axle::OPTION_l_LOCKED);
                        break;
                    case 's':
                        axle.options.push_back(Axle::OPTION_s_SPLIT);
                        break;
                    case 'v':
                        axle.options.push_back(Axle::OPTION_s_VISCOUS);
                        break;

                    default: // No check needed, regex takes care of that 
                        break;
                }
            }
        }
    }

    m_current_module->axles.push_back(axle);	
}

void Parser::ParseInterAxles()
{
    auto args = Ogre::StringUtil::split(m_current_line, ",");
    if (args.size() < 2) { return; }

    InterAxle interaxle;

    interaxle.a1 = this->ParseArgInt(args[0].c_str()) - 1;
    interaxle.a2 = this->ParseArgInt(args[1].c_str()) - 1;

    std::smatch results;
    if (! std::regex_search(args[2], results, Regexes::SECTION_AXLES_PROPERTY))
    {
        this->AddMessage(Message::TYPE_ERROR, "Invalid property, ignoring whole line...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (results[5].matched)
    {
        std::string options_str = results[6].str();
        for (unsigned int i = 0; i < options_str.length(); i++)
        {
            switch(options_str.at(i))
            {
                case 'o':
                    interaxle.options.push_back(Axle::OPTION_o_OPEN);
                    break;
                case 'l':
                    interaxle.options.push_back(Axle::OPTION_l_LOCKED);
                    break;
                case 's':
                    interaxle.options.push_back(Axle::OPTION_s_SPLIT);
                    break;
                case 'v':
                    interaxle.options.push_back(Axle::OPTION_s_VISCOUS);
                    break;

                default: // No check needed, regex takes care of that 
                    break;
            }
        }
    }

    m_current_module->interaxles.push_back(interaxle);	
}

void Parser::ParseAirbrakes()
{
    if (! this->CheckNumArguments(14)) { return; }

    Airbrake airbrake;
    airbrake.reference_node        = this->GetArgNodeRef( 0);
    airbrake.x_axis_node           = this->GetArgNodeRef( 1);
    airbrake.y_axis_node           = this->GetArgNodeRef( 2);
    airbrake.aditional_node        = this->GetArgNodeRef( 3);
    airbrake.offset.x              = this->GetArgFloat  ( 4);
    airbrake.offset.y              = this->GetArgFloat  ( 5);
    airbrake.offset.z              = this->GetArgFloat  ( 6);
    airbrake.width                 = this->GetArgFloat  ( 7);
    airbrake.height                = this->GetArgFloat  ( 8);
    airbrake.max_inclination_angle = this->GetArgFloat  ( 9);
    airbrake.texcoord_x1           = this->GetArgFloat  (10);
    airbrake.texcoord_y1           = this->GetArgFloat  (11);
    airbrake.texcoord_x2           = this->GetArgFloat  (12);
    airbrake.texcoord_y2           = this->GetArgFloat  (13);

    m_current_module->airbrakes.push_back(airbrake);
}

void Parser::ParseVideoCamera()
{
    if (! this->CheckNumArguments(19)) { return; }

    VideoCamera videocamera;

    videocamera.reference_node       = this->GetArgNodeRef     ( 0);
    videocamera.left_node            = this->GetArgNodeRef     ( 1);
    videocamera.bottom_node          = this->GetArgNodeRef     ( 2);
    videocamera.alt_reference_node   = this->GetArgNullableNode( 3);
    videocamera.alt_orientation_node = this->GetArgNullableNode( 4);
    videocamera.offset.x             = this->GetArgFloat       ( 5);
    videocamera.offset.y             = this->GetArgFloat       ( 6);
    videocamera.offset.z             = this->GetArgFloat       ( 7);
    videocamera.rotation.x           = this->GetArgFloat       ( 8);
    videocamera.rotation.y           = this->GetArgFloat       ( 9);
    videocamera.rotation.z           = this->GetArgFloat       (10);
    videocamera.field_of_view        = this->GetArgFloat       (11);
    videocamera.texture_width        = this->GetArgInt         (12);
    videocamera.texture_height       = this->GetArgInt         (13);
    videocamera.min_clip_distance    = this->GetArgFloat       (14);
    videocamera.max_clip_distance    = this->GetArgFloat       (15);
    videocamera.camera_role          = this->GetArgInt         (16);
    videocamera.camera_mode          = this->GetArgInt         (17);
    videocamera.material_name        = this->GetArgStr         (18);

    if (m_num_args > 19) { videocamera.camera_name = this->GetArgStr(19); }

    m_current_module->videocameras.push_back(videocamera);
}

void Parser::ParseCameras()
{
    if (! this->CheckNumArguments(3)) { return; }

    Camera camera;
    camera.center_node = this->GetArgNodeRef(0);
    camera.back_node   = this->GetArgNodeRef(1);
    camera.left_node   = this->GetArgNodeRef(2);

    m_current_module->cameras.push_back(camera);
}

void Parser::ParseTurbopropsUnified()
{
    bool is_turboprop_2 = m_current_section == File::SECTION_TURBOPROPS_2;

    if (! this->CheckNumArguments(is_turboprop_2 ? 9 : 8)) { return; }

    Turboprop2 turboprop;
    
    turboprop.reference_node     = this->GetArgNodeRef(0);
    turboprop.axis_node          = this->GetArgNodeRef(1);
    turboprop.blade_tip_nodes[0] = this->GetArgNodeRef(2);
    turboprop.blade_tip_nodes[1] = this->GetArgNodeRef(3);
    turboprop.blade_tip_nodes[2] = this->GetArgNullableNode(4);
    turboprop.blade_tip_nodes[3] = this->GetArgNullableNode(5);

    int offset = 0;

    if (is_turboprop_2)
    {
        turboprop.couple_node = this->GetArgNullableNode(6);

        offset = 1;
    }

    turboprop.turbine_power_kW   = this->GetArgFloat  (6 + offset);
    turboprop.airfoil            = this->GetArgStr    (7 + offset);
    
    m_current_module->turboprops_2.push_back(turboprop);
}

void Parser::ParseTurbojets()
{
    if (! this->CheckNumArguments(9)) { return; }

    Turbojet turbojet;
    turbojet.front_node     = this->GetArgNodeRef(0);
    turbojet.back_node      = this->GetArgNodeRef(1);
    turbojet.side_node      = this->GetArgNodeRef(2);
    turbojet.is_reversable  = this->GetArgInt    (3);
    turbojet.dry_thrust     = this->GetArgFloat  (4);
    turbojet.wet_thrust     = this->GetArgFloat  (5);
    turbojet.front_diameter = this->GetArgFloat  (6);
    turbojet.back_diameter  = this->GetArgFloat  (7);
    turbojet.nozzle_length  = this->GetArgFloat  (8);

    m_current_module->turbojets.push_back(turbojet);
}

void Parser::ParseTriggers()
{
    if (! this->CheckNumArguments(6)) { return; }

    Trigger trigger;
    trigger.beam_defaults             = m_user_beam_defaults;
    trigger.detacher_group            = m_current_detacher_group;
    trigger.nodes[0]                  = this->GetArgNodeRef(0);
    trigger.nodes[1]                  = this->GetArgNodeRef(1);
    trigger.contraction_trigger_limit = this->GetArgFloat  (2);
    trigger.expansion_trigger_limit   = this->GetArgFloat  (3);
    
    int shortbound_trigger_action = this->GetArgInt(4); 
    int longbound_trigger_action  = this->GetArgInt(5); 
    if (m_num_args > 6)
    {
        std::string options_str = this->GetArgStr(6);
        for (unsigned int i = 0; i < options_str.length(); i++)
        {
            switch(options_str.at(i))
            {
                case 'i': trigger.options |= Trigger::OPTION_i_INVISIBLE;             break;
                case 'c': trigger.options |= Trigger::OPTION_c_COMMAND_STYLE;         break;
                case 'x': trigger.options |= Trigger::OPTION_x_START_OFF;             break;
                case 'b': trigger.options |= Trigger::OPTION_b_BLOCK_KEYS;            break;
                case 'B': trigger.options |= Trigger::OPTION_B_BLOCK_TRIGGERS;        break;
                case 'A': trigger.options |= Trigger::OPTION_A_INV_BLOCK_TRIGGERS;    break;
                case 's': trigger.options |= Trigger::OPTION_s_SWITCH_CMD_NUM;        break;
                case 'h': trigger.options |= Trigger::OPTION_h_UNLOCK_HOOKGROUPS_KEY; break;
                case 'H': trigger.options |= Trigger::OPTION_H_LOCK_HOOKGROUPS_KEY;   break;
                case 't': trigger.options |= Trigger::OPTION_t_CONTINUOUS;            break;
                case 'E': trigger.options |= Trigger::OPTION_E_ENGINE_TRIGGER;        break;

                default:
                    this->AddMessage(Message::TYPE_WARNING, Ogre::String("Invalid trigger option: " + options_str.at(i)));
            }
        }
    }

    if (m_num_args > 7)
    {
        float boundary_timer = this->GetArgFloat(7);
        if (boundary_timer > 0.0f)
            trigger.boundary_timer = boundary_timer;
    }

    // Handle actions
    if (trigger.IsHookToggleTrigger())
    {
        Trigger::HookToggleTrigger hook_toggle;
        hook_toggle.contraction_trigger_hookgroup_id = shortbound_trigger_action;
        hook_toggle.extension_trigger_hookgroup_id = longbound_trigger_action;
        trigger.SetHookToggleTrigger(hook_toggle);
    }
    else if (trigger.HasFlag_E_EngineTrigger())
    {
        Trigger::EngineTrigger engine_trigger;
        engine_trigger.function = Trigger::EngineTrigger::Function(shortbound_trigger_action);
        engine_trigger.motor_index = longbound_trigger_action;
        trigger.SetEngineTrigger(engine_trigger);
    }
    else
    {
        Trigger::CommandKeyTrigger command_keys;
        command_keys.contraction_trigger_key = shortbound_trigger_action;
        command_keys.extension_trigger_key   = longbound_trigger_action;
        trigger.SetCommandKeyTrigger(command_keys);
    }

    m_current_module->triggers.push_back(trigger);
}

void Parser::ParseTorqueCurve()
{
    if (m_current_module->torque_curve == nullptr)
    {
        m_current_module->torque_curve = std::shared_ptr<RigDef::TorqueCurve>(new RigDef::TorqueCurve());
    }

    Ogre::StringVector args = Ogre::StringUtil::split(m_current_line, ",");
    
    if (args.size() == 1u)
    {
        m_current_module->torque_curve->predefined_func_name = args[0];
    }
    else if (args.size() == 2u)
    {
        TorqueCurve::Sample sample;
        sample.power          = this->ParseArgFloat(args[0].c_str());
        sample.torque_percent = this->ParseArgFloat(args[1].c_str());
        m_current_module->torque_curve->samples.push_back(sample);  
    }
    else
    {
        // Consistent with 0.38's parser.
        this->AddMessage(Message::TYPE_ERROR, "Invalid line, too many arguments");   
    }
}

void Parser::ParseTies()
{
    if (! this->CheckNumArguments(5)) { return; }

    Tie tie;
    tie.beam_defaults     = m_user_beam_defaults;
    tie.detacher_group    = m_current_detacher_group;

    tie.root_node         = this->GetArgNodeRef(0);
    tie.max_reach_length  = this->GetArgFloat  (1);
    tie.auto_shorten_rate = this->GetArgFloat  (2);
    tie.min_length        = this->GetArgFloat  (3);
    tie.max_length        = this->GetArgFloat  (4);

    if (m_num_args > 5)
    {
        for (char c: this->GetArgStr(5))
        {
            switch (c)
            {
            case Tie::OPTION_n_FILLER:
            case Tie::OPTION_v_FILLER:
                break;

            case Tie::OPTION_i_INVISIBLE:
                tie.is_invisible = true;
                break;

            case Tie::OPTION_s_NO_SELF_LOCK:
                tie.disable_self_lock = true;
                break;

            default:
                this->AddMessage(Message::TYPE_WARNING, std::string("Invalid option: ") + c + ", ignoring...");
                break;
            }
        }
    }

    if (m_num_args > 6) { tie.max_stress   =  this->GetArgFloat (6); }
    if (m_num_args > 7) { tie.group        =  this->GetArgInt   (7); }

    m_current_module->ties.push_back(tie);
}

void Parser::ParseSoundsources()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    SoundSource soundsource;
    soundsource.node              = this->GetArgNodeRef(0);
    soundsource.sound_script_name = this->GetArgStr(1);

    m_current_module->soundsources.push_back(soundsource);
}

void Parser::ParseSoundsources2()
{
    if (! this->CheckNumArguments(3)) { return; }
    
    SoundSource2 soundsource2;
    soundsource2.node              = this->GetArgNodeRef(0);
    soundsource2.sound_script_name = this->GetArgStr(2);

    int mode = this->GetArgInt(1);
    if (mode < 0)
    {
        if (mode < -2)
        {
            std::string msg = this->GetArgStr(1) + " is invalid soundsources2.mode, falling back to default -2";
            this->AddMessage(Message::TYPE_ERROR, msg.c_str());
            mode = -2;
        }
        soundsource2.mode = SoundSource2::Mode(mode);
    }
    else
    {
        soundsource2.mode = SoundSource2::MODE_CINECAM;
        soundsource2.cinecam_index = mode;
    }

    m_current_module->soundsources2.push_back(soundsource2);
}

void Parser::ParseSlidenodes()
{
    Ogre::StringVector args = Ogre::StringUtil::split(m_current_line, ", ");
    if (args.size() < 2u)
    {
        this->AddMessage(Message::TYPE_ERROR, "Too few arguments");
    }

    SlideNode slidenode;
    slidenode.slide_node = this->_ParseNodeRef(args[0]);
    
    bool in_rail_node_list = true;

    for (auto itor = args.begin() + 1; itor != args.end(); ++itor)
    {
        char c = toupper(itor->at(0));
        switch (c)
        {
        case 'S':
            slidenode.spring_rate = this->ParseArgFloat(itor->substr(1));
            in_rail_node_list = false;
            break;
        case 'B':
            slidenode.break_force = this->ParseArgFloat(itor->substr(1));
            slidenode._break_force_set = true;
            in_rail_node_list = false;
            break;
        case 'T':
            slidenode.tolerance = this->ParseArgFloat(itor->substr(1));
            in_rail_node_list = false;
            break;
        case 'R':
            slidenode.attachment_rate = this->ParseArgFloat(itor->substr(1));
            in_rail_node_list = false;
            break;
        case 'G':
            slidenode.railgroup_id = this->ParseArgFloat(itor->substr(1));
            slidenode._railgroup_id_set = true;
            in_rail_node_list = false;
            break;
        case 'D':
            slidenode.max_attachment_distance = this->ParseArgFloat(itor->substr(1));
            in_rail_node_list = false;
            break;
        case 'C':
            switch (itor->at(1))
            {
            case 'a':
                BITMASK_SET_1(slidenode.constraint_flags, SlideNode::CONSTRAINT_ATTACH_ALL);
                break;
            case 'f':
                BITMASK_SET_1(slidenode.constraint_flags, SlideNode::CONSTRAINT_ATTACH_FOREIGN);
                break;
            case 's':
                BITMASK_SET_1(slidenode.constraint_flags, SlideNode::CONSTRAINT_ATTACH_SELF);
                break;
            case 'n':
                BITMASK_SET_1(slidenode.constraint_flags, SlideNode::CONSTRAINT_ATTACH_NONE);
                break;
            default:
                this->AddMessage(Message::TYPE_WARNING, std::string("Ignoring invalid option: ") + itor->at(1));
                break;
            }
            in_rail_node_list = false;
            break;
        default:
            if (in_rail_node_list)
                slidenode.rail_node_ranges.push_back( _ParseNodeRef(*itor));
            break;
        }
    }
    
    m_current_module->slidenodes.push_back(slidenode);
}

void Parser::ParseShock3()
{
    if (! this->CheckNumArguments(15)) { return; }

    Shock3 shock_3;
    shock_3.beam_defaults  = m_user_beam_defaults;
    shock_3.detacher_group = m_current_detacher_group;

    shock_3.nodes[0]       = this->GetArgNodeRef( 0);
    shock_3.nodes[1]       = this->GetArgNodeRef( 1);
    shock_3.spring_in      = this->GetArgFloat  ( 2);
    shock_3.damp_in        = this->GetArgFloat  ( 3);
    shock_3.damp_in_slow   = this->GetArgFloat  ( 4);
    shock_3.split_vel_in   = this->GetArgFloat  ( 5);
    shock_3.damp_in_fast   = this->GetArgFloat  ( 6);
    shock_3.spring_out     = this->GetArgFloat  ( 7);
    shock_3.damp_out       = this->GetArgFloat  ( 8);
    shock_3.damp_out_slow  = this->GetArgFloat  ( 9);
    shock_3.split_vel_out  = this->GetArgFloat  (10);
    shock_3.damp_out_fast  = this->GetArgFloat  (11);
    shock_3.short_bound    = this->GetArgFloat  (12);
    shock_3.long_bound     = this->GetArgFloat  (13);
    shock_3.precompression = this->GetArgFloat  (14);

    shock_3.options = 0u;
    if (m_num_args > 15)
    {
        std::string options_str = this->GetArgStr(15);
        auto itor = options_str.begin();
        auto endi = options_str.end();
        while (itor != endi)
        {
            char c = *itor++; // ++
            switch (c)
            {
                case 'n': 
                case 'v': 
                    break; // Placeholder, does nothing.
                case 'i': BITMASK_SET_1(shock_3.options, Shock3::OPTION_i_INVISIBLE);
                    break;
                case 'm': BITMASK_SET_1(shock_3.options, Shock3::OPTION_m_METRIC);
                    break;
                case 'M': BITMASK_SET_1(shock_3.options, Shock3::OPTION_M_ABSOLUTE_METRIC);
                    break;
                default: {
                        char msg[100] = "";
                        snprintf(msg, 100, "Invalid option: '%c', ignoring...", c);
                        AddMessage(Message::TYPE_WARNING, msg);
                    }
                    break;
            }
        }
    }

    m_current_module->shocks_3.push_back(shock_3);
}

void Parser::ParseShock2()
{
    if (! this->CheckNumArguments(13)) { return; }

    Shock2 shock_2;
    shock_2.beam_defaults  = m_user_beam_defaults;
    shock_2.detacher_group = m_current_detacher_group;

    shock_2.nodes[0]                   = this->GetArgNodeRef( 0);
    shock_2.nodes[1]                   = this->GetArgNodeRef( 1);
    shock_2.spring_in                  = this->GetArgFloat  ( 2);
    shock_2.damp_in                    = this->GetArgFloat  ( 3);
    shock_2.progress_factor_spring_in  = this->GetArgFloat  ( 4);
    shock_2.progress_factor_damp_in    = this->GetArgFloat  ( 5);
    shock_2.spring_out                 = this->GetArgFloat  ( 6);
    shock_2.damp_out                   = this->GetArgFloat  ( 7);
    shock_2.progress_factor_spring_out = this->GetArgFloat  ( 8);
    shock_2.progress_factor_damp_out   = this->GetArgFloat  ( 9);
    shock_2.short_bound                = this->GetArgFloat  (10);
    shock_2.long_bound                 = this->GetArgFloat  (11);
    shock_2.precompression             = this->GetArgFloat  (12);

    shock_2.options = 0u;
    if (m_num_args > 13)
    {
        std::string options_str = this->GetArgStr(13);
        auto itor = options_str.begin();
        auto endi = options_str.end();
        while (itor != endi)
        {
            char c = *itor++; // ++
            switch (c)
            {
                case 'n': 
                case 'v': 
                    break; // Placeholder, does nothing.
                case 'i': BITMASK_SET_1(shock_2.options, Shock2::OPTION_i_INVISIBLE);
                    break;
                case 'm': BITMASK_SET_1(shock_2.options, Shock2::OPTION_m_METRIC);
                    break;
                case 'M': BITMASK_SET_1(shock_2.options, Shock2::OPTION_M_ABSOLUTE_METRIC);
                    break;
                case 's': BITMASK_SET_1(shock_2.options, Shock2::OPTION_s_SOFT_BUMP_BOUNDS);
                    break;
                default: {
                        char msg[100] = "";
                        snprintf(msg, 100, "Invalid option: '%c', ignoring...", c);
                        AddMessage(Message::TYPE_WARNING, msg);
                    }
                    break;
            }
        }
    }

    m_current_module->shocks_2.push_back(shock_2);
}

void Parser::ParseShock()
{
    if (! this->CheckNumArguments(7)) { return; }

    Shock shock;
    shock.beam_defaults  = m_user_beam_defaults;
    shock.detacher_group = m_current_detacher_group;

    shock.nodes[0]       = this->GetArgNodeRef(0);
    shock.nodes[1]       = this->GetArgNodeRef(1);
    shock.spring_rate    = this->GetArgFloat  (2);
    shock.damping        = this->GetArgFloat  (3);
    shock.short_bound    = this->GetArgFloat  (4);
    shock.long_bound     = this->GetArgFloat  (5);
    shock.precompression = this->GetArgFloat  (6);

    shock.options = 0u;
    if (m_num_args > 7)
    {
        std::string options_str = this->GetArgStr(7);
        auto itor = options_str.begin();
        auto endi = options_str.end();
        while (itor != endi)
        {
            char c = *itor++;
            switch (c)
            {
                case 'n':
                case 'v':
                    break; // Placeholder, does nothing.
                case 'i': BITMASK_SET_1(shock.options, Shock::OPTION_i_INVISIBLE);
                    break;
                case 'm': BITMASK_SET_1(shock.options, Shock::OPTION_m_METRIC);
                    break;
                case 'r':
                case 'R': BITMASK_SET_1(shock.options, Shock::OPTION_R_ACTIVE_RIGHT);
                    break;
                case 'l':
                case 'L': BITMASK_SET_1(shock.options, Shock::OPTION_L_ACTIVE_LEFT);
                    break;
                default: {
                    char msg[100] = "";
                    snprintf(msg, 100, "Invalid option: '%c', ignoring...", c);
                    AddMessage(Message::TYPE_WARNING, msg);
                }
                break;
            }
        }
    }
    m_current_module->shocks.push_back(shock);
}

void Parser::_CheckInvalidTrailingText(Ogre::String const & line, std::smatch const & results, unsigned int index)
{
    if (results[index].matched) // Invalid trailing text 
    {
        std::stringstream msg;
        msg << "Invalid text after parameters: '" << results[index] << "'. Please remove. Ignoring...";
        AddMessage(line, Message::TYPE_WARNING, msg.str());
    }
}

Node::Ref Parser::_ParseNodeRef(std::string const & node_id_str)
{
    if (m_sequential_importer.IsEnabled())
    {
        // Import of legacy fileformatversion
        int node_id_num = STR_PARSE_INT(node_id_str);
        if (node_id_num < 0)
        {
            Str<2000> msg;
            msg << "Invalid negative node number " << node_id_num << ", parsing as " << (node_id_num*-1) << " for backwards compatibility";
            AddMessage(node_id_str, Message::TYPE_WARNING, msg.ToCStr());
            node_id_num *= -1;
        }
        // Since fileformatversion is not known from the beginning of parsing, 2 states must be kept 
        // at the same time: IMPORT_STATE and REGULAR_STATE. The outer logic must make the right pick.
        unsigned int flags = Node::Ref::IMPORT_STATE_IS_VALID |                                     // Import state
                             Node::Ref::REGULAR_STATE_IS_VALID | Node::Ref::REGULAR_STATE_IS_NAMED; // Regular state (fileformatversion >= 450)
        if (m_any_named_node_defined)
        {
            flags |= Node::Ref::IMPORT_STATE_MUST_CHECK_NAMED_FIRST;
        }
        return Node::Ref(node_id_str, node_id_num, flags, m_current_line_number);
    }
    else
    {
        // fileformatversion >= 450, use named-only nodes
        return Node::Ref(node_id_str, 0, Node::Ref::REGULAR_STATE_IS_VALID | Node::Ref::REGULAR_STATE_IS_NAMED, m_current_line_number);
    }
}

void Parser::ParseDirectiveSetDefaultMinimass()
{
    if (! this->CheckNumArguments(2)) { return; } // Directive name + parameter

    m_user_minimass = std::shared_ptr<MinimassPreset>(new MinimassPreset(this->GetArgFloat(1)));
}

void Parser::ParseDirectiveSetInertiaDefaults()
{
    if (! this->CheckNumArguments(2)) { return; }

    float start_delay = this->GetArgFloat(1);
    float stop_delay = 0;
    if (m_num_args > 2) { stop_delay = this->GetArgFloat(2); }

    if (start_delay < 0 || stop_delay < 0)
    {
        m_user_default_inertia = m_ror_default_inertia; // Reset and return
        return;
    }

    // Create
    Inertia* i = new Inertia(*m_user_default_inertia.get());
    i->start_delay_factor = start_delay;
    i->stop_delay_factor = stop_delay;
    
    if (m_num_args > 3) { i->start_function = this->GetArgStr(3); }
    if (m_num_args > 4) { i->stop_function  = this->GetArgStr(4); }
    
    m_user_default_inertia = std::shared_ptr<Inertia>(i);
}

void Parser::ParseScrewprops()
{
    if (! this->CheckNumArguments(4)) { return; }
    
    Screwprop screwprop;

    screwprop.prop_node = this->GetArgNodeRef(0);
    screwprop.back_node = this->GetArgNodeRef(1);
    screwprop.top_node  = this->GetArgNodeRef(2);
    screwprop.power     = this->GetArgFloat  (3);

    m_current_module->screwprops.push_back(screwprop);
}

void Parser::ParseScripts()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    Script script;

    std::string type_str(this->GetArgStr(0));
    if (type_str == "frame-step")
    {
        script.type = Script::TYPE_FRAMESTEP;
    }
    else if (type_str == "sim-step")
    {
        script.type = Script::TYPE_SIMSTEP;
    }
    else
    {
        this->AddMessage(type_str, Message::TYPE_ERROR, _L("Invalid param #0 (type), ignoring line"));
        return;
    }

    script.filename = this->GetArgStr(1);

    for (int i = 2; i < m_num_args; ++i)
    {
        if (i > 2)
        {
            script.arguments += " ";
        }
        script.arguments += this->GetArgStr(i);
    }

    m_current_module->scripts.push_back(script);
}

void Parser::ParseRotatorsUnified()
{
    if (! this->CheckNumArguments(13)) { return; }

    Rotator2 rotator;
    rotator.inertia_defaults = m_user_default_inertia;
    
    rotator.axis_nodes[0]           = this->GetArgNodeRef( 0);
    rotator.axis_nodes[1]           = this->GetArgNodeRef( 1);
    rotator.base_plate_nodes[0]     = this->GetArgNodeRef( 2);
    rotator.base_plate_nodes[1]     = this->GetArgNodeRef( 3);
    rotator.base_plate_nodes[2]     = this->GetArgNodeRef( 4);
    rotator.base_plate_nodes[3]     = this->GetArgNodeRef( 5);
    rotator.rotating_plate_nodes[0] = this->GetArgNodeRef( 6);
    rotator.rotating_plate_nodes[1] = this->GetArgNodeRef( 7);
    rotator.rotating_plate_nodes[2] = this->GetArgNodeRef( 8);
    rotator.rotating_plate_nodes[3] = this->GetArgNodeRef( 9);
    rotator.rate                    = this->GetArgFloat  (10);
    rotator.spin_left_key           = this->GetArgInt    (11);
    rotator.spin_right_key          = this->GetArgInt    (12);
    
    int offset = 0;

    if (m_current_section == File::SECTION_ROTATORS_2)
    {
        if (! this->CheckNumArguments(16)) { return; }
        if (m_num_args > 13) { rotator.rotating_force  = this->GetArgFloat(13); }
        if (m_num_args > 14) { rotator.tolerance       = this->GetArgFloat(14); }
        if (m_num_args > 15) { rotator.description     = this->GetArgStr  (15); }

        offset = 3;
    }

    this->ParseOptionalInertia(rotator.inertia, 13 + offset);
    if (m_num_args > 17 + offset) { rotator.engine_coupling = this->GetArgFloat(17 + offset); }
    if (m_num_args > 18 + offset) { rotator.needs_engine    = this->GetArgBool (18 + offset); }

        if (m_current_section == File::SECTION_ROTATORS_2)
        {
            m_current_module->rotators_2.push_back(rotator);
        }
        else
        {
            m_current_module->rotators.push_back(rotator);
        }
}

void Parser::ParseFileinfo()
{
    if (! this->CheckNumArguments(2)) { return; }

    if (m_current_module != m_root_module)
    {
        this->AddMessage(Message::TYPE_WARNING, "Inline-section 'fileinfo' has global effect and should not appear in a module");
    }

    Fileinfo fileinfo;

    fileinfo.unique_id = this->GetArgStr(1);
    Ogre::StringUtil::trim(fileinfo.unique_id);

    if (m_num_args > 2) { fileinfo.category_id  = this->GetArgInt(2); }
    if (m_num_args > 3) { fileinfo.file_version = this->GetArgInt(3); }

    m_definition->file_info = std::shared_ptr<Fileinfo>( new Fileinfo(fileinfo) );

    this->ChangeSection(File::SECTION_NONE);
}

void Parser::ParseRopes()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    Rope rope;
    rope.beam_defaults  = m_user_beam_defaults;
    rope.detacher_group = m_current_detacher_group;
    rope.root_node      = this->GetArgNodeRef(0);
    rope.end_node       = this->GetArgNodeRef(1);
    
    if (m_num_args > 2) { rope.invisible  = (this->GetArgChar(2) == 'i'); }

    m_current_module->ropes.push_back(rope);
}

void Parser::ParseRopables()
{
    if (! this->CheckNumArguments(1)) { return; }

    Ropable ropable;
    ropable.node = this->GetArgNodeRef(0);
    
    if (m_num_args > 1) { ropable.group         =  this->GetArgInt(1); }
    if (m_num_args > 2) { ropable.has_multilock = (this->GetArgInt(2) == 1); }

    m_current_module->ropables.push_back(ropable);
}

void Parser::ParseRailGroups()
{
    Ogre::StringVector args = Ogre::StringUtil::split(m_current_line, ",");
    if (args.size() < 3u)
    {
        this->AddMessage(Message::TYPE_ERROR, "Not enough parameters");
        return;
    }

    RailGroup railgroup;
    railgroup.id = this->ParseArgInt(args[0].c_str());

    for (auto itor = args.begin() + 1; itor != args.end(); itor++)
    {
        railgroup.node_list.push_back( this->_ParseNodeRef(*itor));
    }

    m_current_module->railgroups.push_back(railgroup);
}

void Parser::ParseProps()
{
    if (! this->CheckNumArguments(10)) { return; }

    Prop prop;
    prop.reference_node = this->GetArgNodeRef(0);
    prop.x_axis_node    = this->GetArgNodeRef(1);
    prop.y_axis_node    = this->GetArgNodeRef(2);
    prop.offset.x       = this->GetArgFloat  (3);
    prop.offset.y       = this->GetArgFloat  (4);
    prop.offset.z       = this->GetArgFloat  (5);
    prop.rotation.x     = this->GetArgFloat  (6);
    prop.rotation.y     = this->GetArgFloat  (7);
    prop.rotation.z     = this->GetArgFloat  (8);
    prop.mesh_name      = this->GetArgStr    (9);

    bool is_dash = false;
         if (prop.mesh_name.find("leftmirror"  ) != std::string::npos) { prop.special = Prop::SPECIAL_MIRROR_LEFT; }
    else if (prop.mesh_name.find("rightmirror" ) != std::string::npos) { prop.special = Prop::SPECIAL_MIRROR_RIGHT; }
    else if (prop.mesh_name.find("dashboard-rh") != std::string::npos) { prop.special = Prop::SPECIAL_DASHBOARD_RIGHT; is_dash = true; }
    else if (prop.mesh_name.find("dashboard"   ) != std::string::npos) { prop.special = Prop::SPECIAL_DASHBOARD_LEFT;  is_dash = true; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "spinprop", false) ) { prop.special = Prop::SPECIAL_AERO_PROP_SPIN; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "pale", false)     ) { prop.special = Prop::SPECIAL_AERO_PROP_BLADE; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "seat", false)     ) { prop.special = Prop::SPECIAL_DRIVER_SEAT; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "seat2", false)    ) { prop.special = Prop::SPECIAL_DRIVER_SEAT_2; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "beacon", false)   ) { prop.special = Prop::SPECIAL_BEACON; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "redbeacon", false)) { prop.special = Prop::SPECIAL_REDBEACON; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "lightb", false)   ) { prop.special = Prop::SPECIAL_LIGHTBAR; } // Previously: 'strncmp("lightbar", meshname, 6)'

    if ((prop.special == Prop::SPECIAL_BEACON) && (m_num_args >= 14))
    {
        prop.special_prop_beacon.flare_material_name = this->GetArgStr(10);
        Ogre::StringUtil::trim(prop.special_prop_beacon.flare_material_name);

        prop.special_prop_beacon.color = Ogre::ColourValue(
            this->GetArgFloat(11), this->GetArgFloat(12), this->GetArgFloat(13));
    }
    else if (is_dash)
    {
        if (m_num_args > 10) prop.special_prop_dashboard.mesh_name = this->GetArgStr(10);
        if (m_num_args > 13)
        {
            prop.special_prop_dashboard.offset = Ogre::Vector3(this->GetArgFloat(11), this->GetArgFloat(12), this->GetArgFloat(13));
            prop.special_prop_dashboard._offset_is_set = true;
        }
        if (m_num_args > 14) prop.special_prop_dashboard.rotation_angle = this->GetArgFloat(14);
    }

    m_current_module->props.push_back(prop);
}

void Parser::ParsePistonprops()
{
    if (!this->CheckNumArguments(10)) { return; }

    Pistonprop pistonprop;
    pistonprop.reference_node     = this->GetArgNodeRef     (0);
    pistonprop.axis_node          = this->GetArgNodeRef     (1);
    pistonprop.blade_tip_nodes[0] = this->GetArgNodeRef     (2);
    pistonprop.blade_tip_nodes[1] = this->GetArgNodeRef     (3);
    pistonprop.blade_tip_nodes[2] = this->GetArgNullableNode(4);
    pistonprop.blade_tip_nodes[3] = this->GetArgNullableNode(5);
    pistonprop.couple_node        = this->GetArgNullableNode(6);
    pistonprop.turbine_power_kW   = this->GetArgFloat       (7);
    pistonprop.pitch              = this->GetArgFloat       (8);
    pistonprop.airfoil            = this->GetArgStr         (9);

    m_current_module->pistonprops.push_back(pistonprop);

}

void Parser::ParseParticles()
{
    if (!this->CheckNumArguments(3)) { return; }

    Particle particle;
    particle.emitter_node         = this->GetArgNodeRef(0);
    particle.reference_node       = this->GetArgNodeRef(1);
    particle.particle_system_name = this->GetArgStr    (2);

    m_current_module->particles.push_back(particle);
}

// Static
void Parser::_TrimTrailingComments(std::string const & line_in, std::string & line_out)
{
    // Trim trailing comment
    // We need to handle a case of lines as [keyword 1, 2, 3 ;;///// Comment!]
    int comment_start = static_cast<int>(line_in.find_first_of(";"));
    if (comment_start != Ogre::String::npos)
    {
        line_out = line_in.substr(0, comment_start);
        return;
    }
    // The [//Comment] is harder - the '/' character may also be present in DESCRIPTION arguments!
    comment_start = static_cast<int>(line_in.find_last_of("/"));
    if (comment_start != Ogre::String::npos)
    {
        while (comment_start >= 0)
        {
            char c = line_in[comment_start - 1];
            if (c != '/' && c != ' ' && c != '\t')
            {
                break; // Start of comment found
            }
            --comment_start;
        }
        line_out = line_in.substr(0, comment_start);
        return;
    }
    // No comment found
    line_out = line_in;
}

void Parser::_PrintNodeDataForVerification(Ogre::String& line, Ogre::StringVector& args, int num_args, Node& node)
{
    std::stringstream msg;
    msg << "Data print for verification:";
    msg << "\n\tPosition X: " << node.position.x << " (input text: \"" << args[1] << "\"";
    msg << "\n\tPosition Y: " << node.position.y << " (input text: \"" << args[2] << "\"";
    msg << "\n\tPosition Z: " << node.position.z << " (input text: \"" << args[3] << "\"";
    if (num_args > 4) // Has options?
    {
        msg << "\n\tOptions: ";
        if (BITMASK_IS_1(node.options, Node::OPTION_l_LOAD_WEIGHT))          { msg << "l_LOAD_WEIGHT ";        }
        if (BITMASK_IS_1(node.options, Node::OPTION_n_MOUSE_GRAB))           { msg << "n_MOUSE_GRAB ";         }
        if (BITMASK_IS_1(node.options, Node::OPTION_m_NO_MOUSE_GRAB))        { msg << "m_NO_MOUSE_GRAB ";      }
        if (BITMASK_IS_1(node.options, Node::OPTION_f_NO_SPARKS))            { msg << "f_NO_SPARKS ";          }
        if (BITMASK_IS_1(node.options, Node::OPTION_x_EXHAUST_POINT))        { msg << "x_EXHAUST_POINT ";      }
        if (BITMASK_IS_1(node.options, Node::OPTION_y_EXHAUST_DIRECTION))    { msg << "y_EXHAUST_DIRECTION ";  }
        if (BITMASK_IS_1(node.options, Node::OPTION_c_NO_GROUND_CONTACT))    { msg << "c_NO_GROUND_CONTACT ";  }
        if (BITMASK_IS_1(node.options, Node::OPTION_h_HOOK_POINT))           { msg << "h_HOOK_POINT ";         }
        if (BITMASK_IS_1(node.options, Node::OPTION_e_TERRAIN_EDIT_POINT))   { msg << "e_TERRAIN_EDIT_POINT "; }
        if (BITMASK_IS_1(node.options, Node::OPTION_b_EXTRA_BUOYANCY))       { msg << "b_EXTRA_BUOYANCY ";     }
        if (BITMASK_IS_1(node.options, Node::OPTION_p_NO_PARTICLES))         { msg << "p_NO_PARTICLES ";       }
        if (BITMASK_IS_1(node.options, Node::OPTION_L_LOG))                  { msg << "L_LOG ";                }
        msg << "(input text:\"" << args[4] << "\"";
    }
    if (num_args > 5) // Has load weight override?
    {
        msg << "\n\tLoad weight overide: " << node.load_weight_override << " (input text: \"" << args[5] << "\"";
    }
    if (num_args > 6) // Is there invalid trailing text?
    {
        msg << "\n\t~Invalid trailing text: ";
        for (int i = 6; i < num_args; ++i)
        {
            msg << args[i];
        }
    }
    this->AddMessage(line, Message::TYPE_WARNING, msg.str());
}

void Parser::ParseNodesUnified()
{
    if (! this->CheckNumArguments(4)) { return; }

    Node node;
    node.node_defaults = m_user_node_defaults;
    node.beam_defaults = m_user_beam_defaults;
    node.node_minimass = m_user_minimass;
    node.detacher_group = m_current_detacher_group;
    node.editor_group_id = (int)m_current_module->node_editor_groups.size() - 1; // Empty -> -1 (none), otherwise last index.

    if (m_current_section == File::SECTION_NODES_2)
    {
        std::string node_name = this->GetArgStr(0);
        node.id.SetStr(node_name);
        if (m_sequential_importer.IsEnabled())
        {
            m_sequential_importer.AddNamedNode(node_name);
        }
        m_any_named_node_defined = true; // For import logic
    }
    else
    {
        const unsigned int node_num = this->GetArgUint(0);
        node.id.SetNum(node_num);
        if (m_sequential_importer.IsEnabled())
        {
            m_sequential_importer.AddNumberedNode(node_num);
        }
    }

    node.position.x = this->GetArgFloat(1);
    node.position.y = this->GetArgFloat(2);
    node.position.z = this->GetArgFloat(3);
    if (m_num_args > 4)
    {
        this->_ParseNodeOptions(node.options, this->GetArgStr(4));
    }
    if (m_num_args > 5)
    {
        if (node.options & Node::OPTION_l_LOAD_WEIGHT)
        {
            node.load_weight_override = this->GetArgFloat(5);
            node._has_load_weight_override = true;
        }
        else
        {
            this->AddMessage(Message::TYPE_WARNING, 
                "Node has load-weight-override value specified, but option 'l' is not present. Ignoring value...");
        }
    }

    m_current_module->nodes.push_back(node);
}

void Parser::ParseNodeCollision()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    NodeCollision node_collision;
    node_collision.node   = this->GetArgNodeRef(0);
    node_collision.radius = this->GetArgFloat  (1);
    
    m_current_module->node_collisions.push_back(node_collision);
}

void Parser::ParseMinimass()
{
    const float minimass = this->GetArgFloat(0);
    if (m_num_args > 1)
    {
        const std::string options_str = this->GetArgStr(1);
        for (char c: options_str)
        {
            switch (c)
            {
            case '\0': // Terminator NULL character
            case (MinimassPreset::OPTION_n_FILLER):
                break;

            case (MinimassPreset::OPTION_l_SKIP_LOADED):
                m_definition->minimass_skip_loaded_nodes = true;
                break;

            default:
                this->AddMessage(Message::TYPE_WARNING, std::string("Unknown option: ") + c);
                break;
            }
        }
    }

    m_ror_minimass->min_mass = minimass;

    this->ChangeSection(File::SECTION_NONE);
}

void Parser::ParseFlexBodyWheel()
{
    if (! this->CheckNumArguments(16)) { return; }

    FlexBodyWheel flexbody_wheel;
    flexbody_wheel.node_defaults = m_user_node_defaults;
    flexbody_wheel.beam_defaults = m_user_beam_defaults;

    flexbody_wheel.tyre_radius        = this->GetArgFloat        ( 0);
    flexbody_wheel.rim_radius         = this->GetArgFloat        ( 1);
    flexbody_wheel.width              = this->GetArgFloat        ( 2);
    flexbody_wheel.num_rays           = this->GetArgInt          ( 3);
    flexbody_wheel.nodes[0]           = this->GetArgNodeRef      ( 4);
    flexbody_wheel.nodes[1]           = this->GetArgNodeRef      ( 5);
    flexbody_wheel.rigidity_node      = this->GetArgRigidityNode ( 6);
    flexbody_wheel.braking            = this->GetArgBraking      ( 7);
    flexbody_wheel.propulsion         = this->GetArgPropulsion   ( 8);
    flexbody_wheel.reference_arm_node = this->GetArgNodeRef      ( 9);
    flexbody_wheel.mass               = this->GetArgFloat        (10);
    flexbody_wheel.tyre_springiness   = this->GetArgFloat        (11);
    flexbody_wheel.tyre_damping       = this->GetArgFloat        (12);
    flexbody_wheel.rim_springiness    = this->GetArgFloat        (13);
    flexbody_wheel.rim_damping        = this->GetArgFloat        (14);
    flexbody_wheel.side               = this->GetArgWheelSide    (15);

    if (m_num_args > 16) { flexbody_wheel.rim_mesh_name  = this->GetArgStr(16); }
    if (m_num_args > 17) { flexbody_wheel.tyre_mesh_name = this->GetArgStr(17); }

    if (m_sequential_importer.IsEnabled())
    {
        m_sequential_importer.GenerateNodesForWheel(File::KEYWORD_FLEXBODYWHEELS, flexbody_wheel.num_rays, flexbody_wheel.rigidity_node.IsValidAnyState());
    }

    m_current_module->flex_body_wheels.push_back(flexbody_wheel);
}

void Parser::ParseMaterialFlareBindings()
{
    if (! this->CheckNumArguments(2)) { return; }

    MaterialFlareBinding binding;
    binding.flare_number  = this->GetArgInt(0);
    binding.material_name = this->GetArgStr(1);
    
    m_current_module->material_flare_bindings.push_back(binding);
}

void Parser::ParseManagedMaterials()
{
    if (! this->CheckNumArguments(2)) { return; }

    ManagedMaterial managed_mat;
    
    managed_mat.options = m_current_managed_material_options;
    managed_mat.name    = this->GetArgStr(0);

    const std::string type_str = this->GetArgStr(1);
    if (type_str == "mesh_standard" || type_str == "mesh_transparent")
    {
        if (! this->CheckNumArguments(3)) { return; }

        managed_mat.type = (type_str == "mesh_standard")
            ? ManagedMaterial::TYPE_MESH_STANDARD
            : ManagedMaterial::TYPE_MESH_TRANSPARENT;
        
        managed_mat.diffuse_map = this->GetArgStr(2);
        
        if (m_num_args > 3) { managed_mat.specular_map = this->GetArgManagedTex(3); }
    }
    else if (type_str == "flexmesh_standard" || type_str == "flexmesh_transparent")
    {
        if (! this->CheckNumArguments(3)) { return; }

        managed_mat.type = (type_str == "flexmesh_standard")
            ? ManagedMaterial::TYPE_FLEXMESH_STANDARD
            : ManagedMaterial::TYPE_FLEXMESH_TRANSPARENT;
            
        managed_mat.diffuse_map = this->GetArgStr(2);
        
        if (m_num_args > 3) { managed_mat.damaged_diffuse_map = this->GetArgManagedTex(3); }
        if (m_num_args > 4) { managed_mat.specular_map        = this->GetArgManagedTex(4); }
    }
    else
    {
        this->AddMessage(Message::TYPE_WARNING, type_str + " is an unkown effect");
        return;
    }

    Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

    if (!rgm.resourceExists(m_resource_group, managed_mat.diffuse_map))
    {
        this->AddMessage(Message::TYPE_WARNING, "Missing texture file: " + managed_mat.diffuse_map);
        return;
    }
    if (managed_mat.HasDamagedDiffuseMap() && !rgm.resourceExists(m_resource_group, managed_mat.damaged_diffuse_map))
    {
        this->AddMessage(Message::TYPE_WARNING, "Missing texture file: " + managed_mat.damaged_diffuse_map);
        managed_mat.damaged_diffuse_map = "-";
    }
    if (managed_mat.HasSpecularMap() && !rgm.resourceExists(m_resource_group, managed_mat.specular_map))
    {
        this->AddMessage(Message::TYPE_WARNING, "Missing texture file: " + managed_mat.specular_map);
        managed_mat.specular_map = "-";
    }

    m_current_module->managed_materials.push_back(managed_mat);
}

void Parser::ParseLockgroups()
{
    if (! this->CheckNumArguments(2)) { return; } // Lockgroup num. + at least 1 node...

    Lockgroup lockgroup;
    lockgroup.number = this->GetArgInt(0);
    
    for (int i = 1; i < m_num_args; ++i)
    {
        lockgroup.nodes.push_back(this->GetArgNodeRef(i));
    }
    
    m_current_module->lockgroups.push_back(lockgroup);
}

void Parser::ParseHydros()
{
    if (! this->CheckNumArguments(3)) { return; }

    Hydro hydro;
    hydro.inertia_defaults   = m_user_default_inertia;
    hydro.detacher_group     = m_current_detacher_group;
    hydro.beam_defaults      = m_user_beam_defaults;
    
    hydro.nodes[0]           = this->GetArgNodeRef(0);
    hydro.nodes[1]           = this->GetArgNodeRef(1);
    hydro.lenghtening_factor = this->GetArgFloat  (2);
    
    if (m_num_args > 3) { hydro.options = this->GetArgStr(3); }
    
    this->ParseOptionalInertia(hydro.inertia, 4);

    m_current_module->hydros.push_back(hydro);
}

void Parser::ParseOptionalInertia(Inertia & inertia, int index)
{
    if (m_num_args > index) { inertia.start_delay_factor = this->GetArgFloat(index++); }
    if (m_num_args > index) { inertia.stop_delay_factor  = this->GetArgFloat(index++); }
    if (m_num_args > index) { inertia.start_function     = this->GetArgStr  (index++); }
    if (m_num_args > index) { inertia.stop_function      = this->GetArgStr  (index++); }
}

void Parser::ParseBeams()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    Beam beam;
    beam.defaults       = m_user_beam_defaults;
    beam.detacher_group = m_current_detacher_group;
    beam.editor_group_id = m_current_module->beam_editor_groups.size() - 1; // Empty -> -1 (none), otherwise last index.
    
    beam.nodes[0] = this->GetArgNodeRef(0);
    beam.nodes[1] = this->GetArgNodeRef(1);

    // Flags 
    if (m_num_args > 2)
    {
        std::string options_str = this->GetArgStr(2);
        for (auto itor = options_str.begin(); itor != options_str.end(); ++itor)
        {
                 if (*itor == 'v') { continue; } // Dummy flag
            else if (*itor == 'n') { continue; } // Dummy flag
            else if (*itor == 'i') { beam.options |= Beam::OPTION_i_INVISIBLE; }
            else if (*itor == 'r') { beam.options |= Beam::OPTION_r_ROPE; }
            else if (*itor == 's') { beam.options |= Beam::OPTION_s_SUPPORT; }
            else
            {
                char msg[200] = "";
                sprintf(msg, "Invalid flag: %c", *itor);
                this->AddMessage(Message::TYPE_WARNING, msg);
            }
        }
    }
    
    if ((m_num_args > 3) && (beam.options & Beam::OPTION_s_SUPPORT))
    {
        float support_break_limit = 0.0f;
        float support_break_factor = this->GetArgInt(3);
        if (support_break_factor > 0.0f)
        {
            support_break_limit = support_break_factor;
        }
        beam.extension_break_limit = support_break_limit;
        beam._has_extension_break_limit = true;
    }

    m_current_module->beams.push_back(beam);
}

void Parser::ParseAnimator()
{
    auto args = Ogre::StringUtil::split(m_current_line, ",");
    if (args.size() < 4) { return; }

    Animator animator;
    animator.inertia_defaults   = m_user_default_inertia;
    animator.beam_defaults      = m_user_beam_defaults;
    animator.detacher_group     = m_current_detacher_group;

    animator.nodes[0]           = this->_ParseNodeRef(args[0]);
    animator.nodes[1]           = this->_ParseNodeRef(args[1]);
    animator.lenghtening_factor = this->ParseArgFloat(args[2]);

    // Parse options; Just use the split/trim/compare method
    Ogre::StringVector attrs = Ogre::StringUtil::split(args[3], "|");

    auto itor = attrs.begin();
    auto endi = attrs.end();
    for (; itor != endi; ++itor)
    {
        Ogre::String token = *itor;
        Ogre::StringUtil::trim(token);
        std::smatch results;
        bool is_shortlimit = false;

        // Numbered keywords 
        if (std::regex_search(token, results, Regexes::PARSE_ANIMATORS_NUMBERED_KEYWORD))
        {
                 if (results[1] == "throttle")   animator.aero_animator.flags |= AeroAnimator::OPTION_THROTTLE;
            else if (results[1] == "rpm")        animator.aero_animator.flags |= AeroAnimator::OPTION_RPM;
            else if (results[1] == "aerotorq")   animator.aero_animator.flags |= AeroAnimator::OPTION_TORQUE;
            else if (results[1] == "aeropit")    animator.aero_animator.flags |= AeroAnimator::OPTION_PITCH;
            else if (results[1] == "aerostatus") animator.aero_animator.flags |= AeroAnimator::OPTION_STATUS;

            animator.aero_animator.motor = this->ParseArgInt(results[2].str().c_str());
        }
        else if ((is_shortlimit = (token.compare(0, 10, "shortlimit") == 0)) || (token.compare(0, 9, "longlimit") == 0))
        {
            Ogre::StringVector fields = Ogre::StringUtil::split(token, ":");
            if (fields.size() > 1)
            {
                if (is_shortlimit)
                {
                    animator.short_limit = std::strtod(fields[1].c_str(), nullptr);
                    animator.flags |= Animator::OPTION_SHORT_LIMIT;
                }
                else
                {
                    animator.long_limit = std::strtod(fields[1].c_str(), nullptr);
                    animator.flags |= Animator::OPTION_LONG_LIMIT;
                }
            }
        }
        else
        {
            // Standalone keywords 
                 if (token == "vis")           animator.flags |= Animator::OPTION_VISIBLE;
            else if (token == "inv")           animator.flags |= Animator::OPTION_INVISIBLE;
            else if (token == "airspeed")      animator.flags |= Animator::OPTION_AIRSPEED;
            else if (token == "vvi")           animator.flags |= Animator::OPTION_VERTICAL_VELOCITY;
            else if (token == "altimeter100k") animator.flags |= Animator::OPTION_ALTIMETER_100K;
            else if (token == "altimeter10k")  animator.flags |= Animator::OPTION_ALTIMETER_10K;
            else if (token == "altimeter1k")   animator.flags |= Animator::OPTION_ALTIMETER_1K;
            else if (token == "aoa")           animator.flags |= Animator::OPTION_ANGLE_OF_ATTACK;
            else if (token == "flap")          animator.flags |= Animator::OPTION_FLAP;
            else if (token == "airbrake")      animator.flags |= Animator::OPTION_AIR_BRAKE;
            else if (token == "roll")          animator.flags |= Animator::OPTION_ROLL;
            else if (token == "pitch")         animator.flags |= Animator::OPTION_PITCH;
            else if (token == "brakes")        animator.flags |= Animator::OPTION_BRAKES;
            else if (token == "accel")         animator.flags |= Animator::OPTION_ACCEL;
            else if (token == "clutch")        animator.flags |= Animator::OPTION_CLUTCH;
            else if (token == "speedo")        animator.flags |= Animator::OPTION_SPEEDO;
            else if (token == "tacho")         animator.flags |= Animator::OPTION_TACHO;
            else if (token == "turbo")         animator.flags |= Animator::OPTION_TURBO;
            else if (token == "parking")       animator.flags |= Animator::OPTION_PARKING;
            else if (token == "shifterman1")   animator.flags |= Animator::OPTION_SHIFT_LEFT_RIGHT;
            else if (token == "shifterman2")   animator.flags |= Animator::OPTION_SHIFT_BACK_FORTH;
            else if (token == "sequential")    animator.flags |= Animator::OPTION_SEQUENTIAL_SHIFT;
            else if (token == "shifterlin")    animator.flags |= Animator::OPTION_GEAR_SELECT;
            else if (token == "torque")        animator.flags |= Animator::OPTION_TORQUE;
            else if (token == "difflock")      animator.flags |= Animator::OPTION_DIFFLOCK;
            else if (token == "rudderboat")    animator.flags |= Animator::OPTION_BOAT_RUDDER;
            else if (token == "throttleboat")  animator.flags |= Animator::OPTION_BOAT_THROTTLE;
        }
    }

    m_current_module->animators.push_back(animator);
}

void Parser::ParseAuthor()
{
    if (m_current_module != m_root_module)
    {
        this->AddMessage(Message::TYPE_WARNING, "Inline-section 'author' has global effect and should not appear in a module");
    }

    if (! this->CheckNumArguments(2)) { return; }

    Author author;
    if (m_num_args > 1) { author.type             = this->GetArgStr(1); }
    if (m_num_args > 2) { author.forum_account_id = this->GetArgInt(2); }
    if (m_num_args > 3) { author.name             = this->GetArgStr(3); }
    if (m_num_args > 4) { author.email            = this->GetArgStr(4); }
    m_definition->authors.push_back(author);

    this->ChangeSection(File::SECTION_NONE);
}

// -------------------------------------------------------------------------- 
//  Utilities
// -------------------------------------------------------------------------- 

void Parser::AddMessage(std::string const & line, Message::Type type, std::string const & message)
{
    RoR::Str<4000> txt;

    if (!m_definition->name.empty())
    {
        txt << m_definition->name;
    }
    else
    {
        txt << m_filename;
    }

    txt << " (line " << (size_t)m_current_line_number;
    if (m_current_section != File::Section::SECTION_INVALID)
    {
        txt << " '" << RigDef::File::SectionToString(m_current_section);
        if (m_current_subsection != File::Subsection::SUBSECTION_NONE)
        {
            txt << "/" << RigDef::File::SubsectionToString(m_current_subsection);
        }
        txt << "'";
    }

    if (m_current_module != m_root_module)
    {
        txt << ", module: " << m_current_module->name;
    }

    txt << "): " << message;

    RoR::Console::MessageType cm_type;
    switch (type)
    {
    case Message::TYPE_FATAL_ERROR:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_ERROR;
        break;

    case Message::TYPE_ERROR:
    case Message::TYPE_WARNING:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_WARNING;
        break;

    default:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_NOTICE;
        break;
    }

    RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_ACTOR, cm_type, txt.ToCStr());
}

File::Keyword Parser::IdentifyKeywordInCurrentLine()
{
    // Quick check - keyword always starts with ASCII letter
    char c = tolower(m_current_line[0]); // Note: line comes in trimmed
    if (c > 'z' || c < 'a')
    {
        return File::KEYWORD_INVALID;
    }

    // Search with correct lettercase
    std::smatch results;
    std::string line(m_current_line);
    std::regex_search(line, results, Regexes::IDENTIFY_KEYWORD_RESPECT_CASE); // Always returns true.
    File::Keyword keyword = FindKeywordMatch(results);
    if (keyword != File::KEYWORD_INVALID)
    {
        return keyword;
    }

    // Search and ignore lettercase
    std::regex_search(line, results, Regexes::IDENTIFY_KEYWORD_IGNORE_CASE); // Always returns true.
    keyword = FindKeywordMatch(results);
    if (keyword != File::KEYWORD_INVALID)
    {
        this->AddMessage(line, Message::TYPE_WARNING,
            "Keyword has invalid lettercase. Correct form is: " + std::string(File::KeywordToString(keyword)));
    }
    return keyword;
}

File::Keyword Parser::FindKeywordMatch(std::smatch& search_results)
{
    // The 'results' array contains a complete match at positon [0] and sub-matches starting with [1], 
    //    so we get exact positions in Regexes::IDENTIFY_KEYWORD, which again match File::Keyword enum members

    for (unsigned int i = 1; i < search_results.size(); i++)
    {
        std::ssub_match sub  = search_results[i];
        if (sub.matched)
        {
            // Build enum value directly from result offset
            return File::Keyword(i);
        }
    }
    return File::KEYWORD_INVALID;
}

void Parser::Prepare()
{
    m_current_section = File::SECTION_TRUCK_NAME;
    m_current_subsection = File::SUBSECTION_NONE;
    m_current_line_number = 1;
    m_definition = std::shared_ptr<File>(new File());
    m_in_block_comment = false;
    m_in_description_section = false;
    m_any_named_node_defined = false;
    m_last_flexbody.reset(); // Set to nullptr
    m_current_detacher_group = 0; // Global detacher group 

    m_user_default_inertia = m_ror_default_inertia;
    m_user_node_defaults   = m_ror_node_defaults;
    m_user_minimass        = m_ror_minimass;
    m_current_managed_material_options = ManagedMaterialsOptions();

    m_user_beam_defaults = std::shared_ptr<BeamDefaults>(new BeamDefaults);
    m_user_beam_defaults->springiness           = DEFAULT_SPRING;
    m_user_beam_defaults->damping_constant      = DEFAULT_DAMP;
    m_user_beam_defaults->deformation_threshold = BEAM_DEFORM;
    m_user_beam_defaults->breaking_threshold    = BEAM_BREAK;
    m_user_beam_defaults->visual_beam_diameter  = DEFAULT_BEAM_DIAMETER;

    m_root_module = m_definition->root_module;
    m_current_module = m_definition->root_module;

    m_sequential_importer.Init(true); // Enabled=true
}

void Parser::ChangeSection(RigDef::File::Section new_section)
{
    // ## Section-specific switch logic ##

    if (m_current_submesh != nullptr)
    {
        m_current_module->submeshes.push_back(*m_current_submesh);
        m_current_submesh.reset(); // Set to nullptr
        m_current_subsection = File::SUBSECTION_NONE;
    }

    if (m_current_camera_rail != nullptr)
    {
        if (m_current_camera_rail->nodes.size() == 0)
        {
            this->AddMessage(Message::TYPE_WARNING, "Empty section 'camerarail', ignoring...");
        }
        else
        {
            m_current_module->camera_rails.push_back(*m_current_camera_rail);
            m_current_camera_rail.reset();
        }
    }

    if (m_current_section == File::SECTION_FLEXBODIES)
    {
        m_current_subsection = File::SUBSECTION_NONE;
    }

    // Enter sections
    m_current_section = new_section;
    if (new_section == File::SECTION_SUBMESH)
    {
        m_current_submesh = std::shared_ptr<Submesh>( new Submesh() );
    }
    else if (new_section == File::SECTION_CAMERA_RAIL)
    {
        m_current_camera_rail = std::shared_ptr<CameraRail>( new CameraRail() );
    }
    else if (new_section == File::SECTION_FLEXBODIES)
    {
        m_current_subsection = File::SUBSECTION__FLEXBODIES__PROPLIKE_LINE;
    }
    else if (new_section == File::SECTION_GUI_SETTINGS)
    {
        if (m_current_module->gui_settings == nullptr)
        {
            m_current_module->gui_settings = std::shared_ptr<GuiSettings> ( new GuiSettings() );
        }
    }
}

void Parser::ProcessChangeModuleLine(File::Keyword keyword)
{
    // Determine and verify new module
    std::string new_module_name;
    if (keyword == File::KEYWORD_END_SECTION)
    {
        if (m_current_module == m_root_module)
        {
            this->AddMessage(Message::TYPE_ERROR, "Misplaced keyword 'end_section' (already in root module), ignoring...");
            return;
        }
        new_module_name = ROOT_MODULE_NAME;
    }
    else if (keyword == File::KEYWORD_SECTION)
    {
        if (!this->CheckNumArguments(3)) // Syntax: "section VERSION NAME"; VERSION is unused
        {
            return; // Error already reported
        }

        new_module_name = this->GetArgStr(2);
        if (new_module_name == m_current_module->name)
        {
            this->AddMessage(Message::TYPE_ERROR, "Attempt to re-enter current module, ignoring...");
            return;
        }
    }

    // Perform the switch
    this->ChangeSection(RigDef::File::SECTION_NONE);
    m_last_flexbody.reset(); // Set to nullptr

    if (new_module_name == ROOT_MODULE_NAME)
    {
        m_current_module = m_root_module;
        return;
    }

    auto search_itor = m_definition->user_modules.find(new_module_name);
    if (search_itor != m_definition->user_modules.end())
    {
        m_current_module = search_itor->second;
    }
    else
    {
        m_current_module = std::make_shared<File::Module>(new_module_name);
        m_definition->user_modules.insert(std::make_pair(new_module_name, m_current_module));
    }
}

void Parser::Finalize()
{
    this->ChangeSection(File::SECTION_NONE);
    m_definition->global_minimass = m_ror_minimass;

    if (m_sequential_importer.IsEnabled())
    {
        m_sequential_importer.Process( m_definition );
    }
}

std::string Parser::GetArgStr(int index)
{
    return std::string(m_args[index].start, m_args[index].length);
}

char Parser::GetArgChar(int index)
{
    return *(m_args[index].start);
}

MeshWheel::Side Parser::GetArgWheelSide(int index)
{
    char side_char = this->GetArgChar(index);
    if (side_char != 'r')
    {
        if (side_char != 'l')
        {
            char msg[200] = "";
            snprintf(msg, 200, "Bad arg~%d 'side' (value: %c), parsing as 'l' for backwards compatibility.", index + 1, side_char);
            this->AddMessage(Message::TYPE_WARNING, msg);
        }
        return MeshWheel::SIDE_LEFT;
    }
    return MeshWheel::SIDE_RIGHT;
}

long Parser::GetArgLong(int index)
{
    errno = 0;
    char* out_end = nullptr;
    const int MSG_LEN = 200;
    char msg[MSG_LEN];
    long res = std::strtol(m_args[index].start, &out_end, 10);
    if (errno != 0)
    {
        snprintf(msg, MSG_LEN, "Cannot parse argument [%d] as integer, errno: %d", index + 1, errno);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return 0; // Compatibility
    }
    if (out_end == m_args[index].start)
    {
        snprintf(msg, MSG_LEN, "Argument [%d] is not valid integer", index + 1);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return 0; // Compatibility
    }
    else if (out_end != (m_args[index].start + m_args[index].length))
    {
        snprintf(msg, MSG_LEN, "Integer argument [%d] has invalid trailing characters", index + 1);
        this->AddMessage(Message::TYPE_WARNING, msg);
    }
    return res;
}

int Parser::GetArgInt(int index)
{
    return static_cast<int>(this->GetArgLong(index));
}

Node::Ref Parser::GetArgRigidityNode(int index)
{
    std::string rigidity_node = this->GetArgStr(index);
    if (rigidity_node != "9999") // Special null value
    {
        return this->GetArgNodeRef(index);
    }
    return Node::Ref(); // Defaults to invalid ref
}

Wheels::Propulsion Parser::GetArgPropulsion(int index)
{
    int propulsion = this->GetArgInt(index);
    if (propulsion < 0 || propulsion > 2)
    {
        char msg[100] = "";
        snprintf(msg, 100, "Bad value of param ~%d (propulsion), using 0 (no propulsion)", index + 1);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return Wheels::PROPULSION_NONE;
    }
    return Wheels::Propulsion(propulsion);
}

Wheels::Braking Parser::GetArgBraking(int index)
{
    int braking = this->GetArgInt(index);
    if (braking < 0 || braking > 4)
    {
        char msg[100] = "";
        snprintf(msg, 100, "Bad value of param ~%d (braking), using 0 (no braking)", index + 1);
        return Wheels::BRAKING_NO;
    }
    return Wheels::Braking(braking);
}

Node::Ref Parser::GetArgNodeRef(int index)
{
    return this->_ParseNodeRef(this->GetArgStr(index));
}

Node::Ref Parser::GetArgNullableNode(int index)
{
    if (! (Ogre::StringConverter::parseReal(this->GetArgStr(index)) == -1.f))
    {
        return this->GetArgNodeRef(index);
    }
    return Node::Ref(); // Defaults to empty ref.
}

unsigned Parser::GetArgUint(int index)
{
    return static_cast<unsigned>(this->GetArgLong(index));
}

Flare2::Type Parser::GetArgFlareType(int index)
{
    char in = this->GetArgChar(index);
    if (in != 'f' && in != 'b' && in != 'l' && in != 'r' && in != 'R' && in != 'u')
    {
        char msg[100];
        snprintf(msg, 100, "Invalid flare type '%c', falling back to type 'f' (front light)...", in);
        AddMessage(Message::TYPE_WARNING, msg);

        in = 'f';
    }
    return Flare2::Type(in);
}

float Parser::GetArgFloat(int index)
{
    errno = 0;
    char* out_end = nullptr;
    float res = std::strtod(m_args[index].start, &out_end);
    const int MSG_LEN = LINE_BUFFER_LENGTH +100;
    char msg_buf[MSG_LEN];
    if (errno != 0)
    {
        snprintf(msg_buf, MSG_LEN, "Cannot parse argument [%d] as float, errno: %d", index + 1, errno);
        this->AddMessage(Message::TYPE_ERROR, msg_buf);
        return 0.f; // Compatibility
    }
    if (out_end == m_args[index].start)
    {
        char arg[LINE_BUFFER_LENGTH] = "";
        strncpy(arg, m_args[index].start, m_args[index].length);
        snprintf(msg_buf, MSG_LEN, "Argument [%d] (\"%s\") is not valid float", index + 1, arg);
        this->AddMessage(Message::TYPE_ERROR, msg_buf);
        return 0.f; // Compatibility
    }
    else if (out_end != (m_args[index].start + m_args[index].length))
    {
        char arg[LINE_BUFFER_LENGTH] = "";
        ptrdiff_t offset = (out_end - m_args[index].start);
        strncpy(arg, out_end, m_args[index].length - offset);
        snprintf(msg_buf, MSG_LEN, "Argument [%d] (type: float) has invalid trailing characters (\"%s\")", index + 1, arg);
        this->AddMessage(Message::TYPE_WARNING, msg_buf);
    }
    return static_cast<float>(res);
}

float Parser::ParseArgFloat(const char* str)
{
    errno = 0;
    float res = std::strtod(str, nullptr);
    if (errno != 0)
    {
        char msg[100];
        snprintf(msg, 100, "Cannot parse argument '%s' as float, errno: %d", str, errno);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return 0.f; // Compatibility
    }
    return static_cast<float>(res);
}

float Parser::ParseArgFloat(std::string const & str)
{
    return this->ParseArgFloat(str.c_str());
}

unsigned Parser::ParseArgUint(const char* str)
{
    errno = 0;
    long res = std::strtol(str, nullptr, 10);
    if (errno != 0)
    {
        char msg[200];
        snprintf(msg, 200, "Cannot parse argument '%s' as int, errno: %d", str, errno);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return 0.f; // Compatibility
    }
    return static_cast<unsigned>(res);
}

unsigned Parser::ParseArgUint(std::string const & str)
{
    return this->ParseArgUint(str.c_str());
}

int Parser::ParseArgInt(const char* str)
{
    return static_cast<int>(this->ParseArgUint(str));
}

bool Parser::GetArgBool(int index)
{
    return Ogre::StringConverter::parseBool(this->GetArgStr(index));
}

Wing::Control Parser::GetArgWingSurface(int index)
{
    std::string str = this->GetArgStr(index);
    size_t bad_pos = str.find_first_not_of(Wing::CONTROL_LEGAL_FLAGS);
    const int MSG_LEN = 300;
    char msg_buf[MSG_LEN] = "";
    if (bad_pos == 0)
    {
        snprintf(msg_buf, MSG_LEN, "Invalid argument ~%d 'control surface' (value: %s), allowed are: <%s>, ignoring...",
            index + 1, str.c_str(), Wing::CONTROL_LEGAL_FLAGS.c_str());
        this->AddMessage(Message::TYPE_ERROR, msg_buf);
        return Wing::CONTROL_n_NONE;
    }
    if (str.size() > 1)
    {
        snprintf(msg_buf, MSG_LEN, "Argument ~%d 'control surface' (value: %s), should be only 1 letter.", index, str.c_str());
        this->AddMessage(Message::TYPE_WARNING, msg_buf);
    }
    return Wing::Control(str.at(0));
}

std::string Parser::GetArgManagedTex(int index)
{
    std::string tex_name = this->GetArgStr(index);
    return (tex_name.at(0) != '-') ? tex_name : "";
}

int Parser::TokenizeCurrentLine()
{
    int cur_arg = 0;
    const char* cur_char = m_current_line;
    int arg_len = 0;
    while ((*cur_char != '\0') && (cur_arg < Parser::LINE_MAX_ARGS))
    {
        const bool is_arg = !IsSeparator(*cur_char);
        if ((arg_len == 0) && is_arg)
        {
            m_args[cur_arg].start = cur_char;
            arg_len = 1;
        }
        else if ((arg_len > 0) && !is_arg)
        {
            m_args[cur_arg].length = arg_len;
            arg_len = 0;
            ++cur_arg;
        }
        else if (is_arg)
        {
            ++arg_len;
        }
        ++cur_char;
    }
    if (arg_len > 0)
    {
        m_args[cur_arg].length = arg_len;
        ++cur_arg;
    }

    m_num_args = cur_arg;
    return cur_arg;
}

void Parser::ProcessOgreStream(Ogre::DataStream* stream, Ogre::String resource_group)
{
    m_resource_group = resource_group;
    m_filename = stream->getName();

    char raw_line_buf[LINE_BUFFER_LENGTH];
    while (!stream->eof())
    {
        try
        {
            stream->readLine(raw_line_buf, LINE_BUFFER_LENGTH);
        }
        catch (Ogre::Exception &ex)
        {
            std::string msg = "Error reading truckfile! Message:\n";
            msg += ex.getFullDescription();
            this->AddMessage(Message::TYPE_FATAL_ERROR, msg.c_str());
            break;
        }

        this->ProcessRawLine(raw_line_buf);
    }
}

void Parser::ProcessRawLine(const char* raw_line_buf)
{
    const char* raw_start = raw_line_buf;
    const char* raw_end = raw_line_buf + strnlen(raw_line_buf, LINE_BUFFER_LENGTH);

    // Trim leading whitespace
    while (IsWhitespace(*raw_start) && (raw_start != raw_end))
    {
        ++raw_start;
    }

    // Sanitize UTF-8
    memset(m_current_line, 0, LINE_BUFFER_LENGTH);
    char* out_start = m_current_line;
    utf8::replace_invalid(raw_start, raw_end, out_start, '?');

    // Process
    this->ProcessCurrentLine();
    ++m_current_line_number;
}

void Parser::ProcessCommentLine()
{
    if (m_current_line[0] != ';')
    {
        return;
    }

    Str<100> name;
    if ((strlen(m_current_line) >= 5) &&
        (m_current_line[1] == 'g')    &&
        (m_current_line[2] == 'r')    &&
        (m_current_line[3] == 'p')    &&
        (m_current_line[4] == ':'))
    {
        name = &m_current_line[5];
    }
    else if (App::diag_import_grp_loose->GetActiveVal<bool>())
    {
        name = &m_current_line[1];
    }
    else
    {
        return;
    }

    switch (m_current_section)
    {
    case File::SECTION_NODES:
    case File::SECTION_NODES_2:
        m_current_module->node_editor_groups.push_back(File::EditorGroup(name.ToCStr()));
        break;

    case File::SECTION_BEAMS:
        m_current_module->beam_editor_groups.push_back(File::EditorGroup(name.ToCStr()));
        break;

    default:
        break;
    }
}

} // namespace RigDef
