/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

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

#include "Actor.h"
#include "Application.h"
#include "SimConstants.h"
#include "CacheSystem.h"
#include "Console.h"
#include "RigDef_File.h"
#include "RigDef_Regexes.h"
#include "Utils.h"

#include <OgreException.h>
#include <OgreString.h>
#include <OgreStringVector.h>
#include <OgreStringConverter.h>

#include <algorithm>

using namespace RoR;

namespace RigDef
{

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

Parser::Parser()
{
    // Push defaults 
    m_ror_default_inertia = std::shared_ptr<Inertia>(new Inertia);
    m_ror_node_defaults = std::shared_ptr<NodeDefaults>(new NodeDefaults);
}

void Parser::ProcessCurrentLine()
{
    // Ignore comment lines
    if ((m_current_line[0] == ';') || (m_current_line[0] == '/'))
    {
        return;
    }

    // First line in file (except blanks or comments) is the actor name
    if (m_definition->name == "" && m_current_line != "")
    {
        m_definition->name = m_current_line; // Already trimmed
        return;
    }

    // Split line to tokens
    if (m_current_block != Keyword::COMMENT &&
        m_current_block != Keyword::DESCRIPTION)
    {
        this->TokenizeCurrentLine();
    }

    // Detect keyword on current line 
    Keyword keyword = Keyword::INVALID;
    // Quick check - keyword always starts with ASCII letter
    char c = tolower(m_current_line[0]); // Note: line comes in trimmed
    if (c >= 'a' && c <= 'z')
    {
        keyword = Parser::IdentifyKeyword(m_current_line);
    }
    m_log_keyword = keyword;
    switch (keyword)
    {
        // No keyword - Continue below to process current block.
        case Keyword::INVALID:
            break; // << NOT RETURN.

        // Directives without arguments: just record, do not change current block.
        case Keyword::DISABLEDEFAULTSOUNDS:
        case Keyword::ENABLE_ADVANCED_DEFORMATION:
        case Keyword::FORWARDCOMMANDS:
        case Keyword::HIDEINCHOOSER:
        case Keyword::IMPORTCOMMANDS:
        case Keyword::LOCKGROUP_DEFAULT_NOLOCK:
        case Keyword::RESCUER:
        case Keyword::ROLLON:
        case Keyword::SLIDENODE_CONNECT_INSTANTLY:
            this->ProcessGlobalDirective(keyword);
            return;
        case Keyword::END_SECTION:
            this->ProcessChangeModuleLine(keyword);
            return;

        // Directives with arguments: process immediately, do not change current block.
        case Keyword::ADD_ANIMATION:
            this->ParseDirectiveAddAnimation();
            return;
        case Keyword::ANTILOCKBRAKES:
            this->ParseAntiLockBrakes();
            return;
        case Keyword::AUTHOR:
            this->ParseAuthor();
            return;
        case Keyword::BACKMESH:
            this->ParseDirectiveBackmesh();
            return;
        case Keyword::CRUISECONTROL:
            this->ParseCruiseControl();
            return;
        case Keyword::DEFAULT_SKIN:
            this->ParseDirectiveDefaultSkin();
            return;
        case Keyword::DETACHER_GROUP:
            this->ParseDirectiveDetacherGroup();
            return;
        case Keyword::EXTCAMERA:
            this->ParseExtCamera();
            return;
        case Keyword::FILEFORMATVERSION:
            this->ParseFileFormatVersion();
            return;
        case Keyword::FILEINFO:
            this->ParseFileinfo();
            return;
        case Keyword::FLEXBODY_CAMERA_MODE:
            this->ParseDirectiveFlexbodyCameraMode();
            return;
        case Keyword::FORSET:
            this->ParseDirectiveForset();
            return;
        case Keyword::GUID:
            this->ParseGuid();
            return;
        case Keyword::PROP_CAMERA_MODE:
            this->ParseDirectivePropCameraMode();
            return;
        case Keyword::SECTION:
            this->ParseDirectiveSection();
            return;
        case Keyword::SET_BEAM_DEFAULTS:
            this->ParseDirectiveSetBeamDefaults();
            return;
        case Keyword::SET_BEAM_DEFAULTS_SCALE:
            this->ParseDirectiveSetBeamDefaultsScale();
            return;
        case Keyword::SET_COLLISION_RANGE:
            this->ParseSetCollisionRange();
            return;
        case Keyword::SET_DEFAULT_MINIMASS:
            this->ParseDirectiveSetDefaultMinimass();
            return;
        case Keyword::SET_INERTIA_DEFAULTS:
            this->ParseDirectiveSetInertiaDefaults();
            return;
        case Keyword::SET_MANAGEDMATERIALS_OPTIONS:
            this->ParseDirectiveSetManagedMaterialsOptions();
            return;
        case Keyword::SET_NODE_DEFAULTS:
            this->ParseDirectiveSetNodeDefaults();
            return;
        case Keyword::SET_SKELETON_SETTINGS:
            this->ParseSetSkeletonSettings();
            return;
        case Keyword::SPEEDLIMITER:
            this->ParseSpeedLimiter();
            return;
        case Keyword::SUBMESH:
            this->ParseDirectiveSubmesh();
            return;
        case Keyword::SUBMESH_GROUNDMODEL:
            this->ParseSubmeshGroundModel();
            return;
        case Keyword::TRACTIONCONTROL:
            this->ParseTractionControl();
            return;

        // Keywords which end current block:
        case Keyword::END_COMMENT:
        case Keyword::END_DESCRIPTION:
        case Keyword::END:
            this->BeginBlock(Keyword::INVALID);
            return;

        // Ignored keywords (obsolete):
        case Keyword::ENVMAP:
        case Keyword::HOOKGROUP:
        case Keyword::NODECOLLISION:
        case Keyword::RIGIDIFIERS:
            return;

        // Keywords which start new block:
        default:
            this->BeginBlock(keyword);
            return;
    }

    // Block extent tracking - assumes single 'nodes[2]' and 'beams' block in file.
    if ((keyword == Keyword::NODES || keyword == Keyword::NODES2) && m_current_module->_hint_nodes12_start_linenumber == -1)
    {
        m_current_module->_hint_nodes12_start_linenumber = (int)m_current_line_number;
    }
    else if (keyword == Keyword::BEAMS && m_current_module->_hint_beams_start_linenumber == -1)
    {
        m_current_module->_hint_beams_start_linenumber = (int)m_current_line_number;
    }
    else if (m_current_module->_hint_nodes12_end_linenumber == -1)
    {
        m_current_module->_hint_nodes12_start_linenumber = (int)m_current_line_number - 1;
    }
    else if (m_current_module->_hint_beams_end_linenumber == -1)
    {
        m_current_module->_hint_beams_start_linenumber = (int)m_current_line_number - 1;
    }

    // Parse current block, if any
    m_log_keyword = m_current_block;
    switch (m_current_block)
    {
        case Keyword::AIRBRAKES:            this->ParseAirbrakes();               return;
        case Keyword::ANIMATORS:            this->ParseAnimator();                return;
        case Keyword::ASSETPACKS:           this->ParseAssetpacks();              return;
        case Keyword::AXLES:                this->ParseAxles();                   return;
        case Keyword::BEAMS:                this->ParseBeams();                   return;
        case Keyword::BRAKES:               this->ParseBrakes();                  return;
        case Keyword::CAMERAS:              this->ParseCameras();                 return;
        case Keyword::CAB:                  this->ParseCab();                     return;
        case Keyword::CAMERARAIL:           this->ParseCameraRails();             return;
        case Keyword::CINECAM:              this->ParseCinecam();                 return;
        case Keyword::COMMANDS:
        case Keyword::COMMANDS2:            this->ParseCommandsUnified();         return;
        case Keyword::COLLISIONBOXES:       this->ParseCollisionBox();            return;
        case Keyword::CONTACTERS:           this->ParseContacter();               return;
        case Keyword::DESCRIPTION:          this->ParseDescription();             return;
        case Keyword::ENGINE:               this->ParseEngine();                  return;
        case Keyword::ENGOPTION:            this->ParseEngoption();               return;
        case Keyword::ENGTURBO:             this->ParseEngturbo();                return;
        case Keyword::EXHAUSTS:             this->ParseExhaust();                 return;
        case Keyword::FIXES:                this->ParseFixes();                   return;
        case Keyword::FLARES:
        case Keyword::FLARES2:              this->ParseFlaresUnified();           return;
        case Keyword::FLARES3:              this->ParseFlares3();                 return;
        case Keyword::FLEXBODIES:           this->ParseFlexbody();                return;
        case Keyword::FLEXBODYWHEELS:       this->ParseFlexBodyWheel();           return;
        case Keyword::FUSEDRAG:             this->ParseFusedrag();                return;
        case Keyword::GLOBALS:              this->ParseGlobals();                 return;
        case Keyword::GUISETTINGS:          this->ParseGuiSettings();             return;
        case Keyword::HELP:                 this->ParseHelp();                    return;
        case Keyword::HOOKS:                this->ParseHook();                    return;
        case Keyword::HYDROS:               this->ParseHydros();                  return;
        case Keyword::INTERAXLES:           this->ParseInterAxles();              return;
        case Keyword::LOCKGROUPS:           this->ParseLockgroups();              return;
        case Keyword::MANAGEDMATERIALS:     this->ParseManagedMaterials();        return;
        case Keyword::MATERIALFLAREBINDINGS:this->ParseMaterialFlareBindings();   return;
        case Keyword::MESHWHEELS:           this->ParseMeshWheel();               return;
        case Keyword::MESHWHEELS2:          this->ParseMeshWheel2();              return;
        case Keyword::MINIMASS:             this->ParseMinimass();                return;
        case Keyword::NODES:
        case Keyword::NODES2:               this->ParseNodesUnified();            return;
        case Keyword::PARTICLES:            this->ParseParticles();               return;
        case Keyword::PISTONPROPS:          this->ParsePistonprops();             return;
        case Keyword::PROPS:                this->ParseProps();                   return;
        case Keyword::RAILGROUPS:           this->ParseRailGroups();              return;
        case Keyword::ROPABLES:             this->ParseRopables();                return;
        case Keyword::ROPES:                this->ParseRopes();                   return;
        case Keyword::ROTATORS:
        case Keyword::ROTATORS2:            this->ParseRotatorsUnified();         return;
        case Keyword::SCREWPROPS:           this->ParseScrewprops();              return;
        case Keyword::SCRIPTS:              this->ParseScripts();                 return;
        case Keyword::SHOCKS:               this->ParseShock();                   return;
        case Keyword::SHOCKS2:              this->ParseShock2();                  return;
        case Keyword::SHOCKS3:              this->ParseShock3();                  return;
        case Keyword::SLIDENODES:           this->ParseSlidenodes();              return;
        case Keyword::SOUNDSOURCES:         this->ParseSoundsources();            return;
        case Keyword::SOUNDSOURCES2:        this->ParseSoundsources2();           return;
        case Keyword::TEXCOORDS:            this->ParseTexcoords();               return;
        case Keyword::TIES:                 this->ParseTies();                    return;
        case Keyword::TORQUECURVE:          this->ParseTorqueCurve();             return;
        case Keyword::TRANSFERCASE:         this->ParseTransferCase();            return;
        case Keyword::TRIGGERS:             this->ParseTriggers();                return;
        case Keyword::TURBOJETS:            this->ParseTurbojets();               return;
        case Keyword::TURBOPROPS:           
        case Keyword::TURBOPROPS2:          this->ParseTurbopropsUnified();       return;
        case Keyword::VIDEOCAMERA:          this->ParseVideoCamera();             return;
        case Keyword::WHEELDETACHERS:       this->ParseWheelDetachers();          return;
        case Keyword::WHEELS:               this->ParseWheel();                   return;
        case Keyword::WHEELS2:              this->ParseWheel2();                  return;
        case Keyword::WINGS:                this->ParseWing();                    return;
        default:;
    };
}

bool Parser::CheckNumArguments(int min_args)
{
    if (min_args > m_num_args)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Not enough arguments (got {}, {} needed), skipping line", m_num_args, min_args));
        return false;
    }
    return true;
}

// -------------------------------------------------------------------------- 
// Parsing individual keywords                                                
// -------------------------------------------------------------------------- 

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

    CollisionRange cr;
    cr.node_collision_range = this->GetArgFloat(1);

    m_current_module->set_collision_range.push_back(cr);
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
        m_sequential_importer.GenerateNodesForWheel(Keyword::WHEELS2, wheel_2.num_rays, wheel_2.rigidity_node.IsValidAnyState());
    }

    m_current_module->wheels2.push_back(wheel_2);
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
        m_sequential_importer.GenerateNodesForWheel(Keyword::WHEELS, wheel.num_rays, wheel.rigidity_node.IsValidAnyState());
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
    m_num_args = (int)tokens.size();
    if (! this->CheckNumArguments(2)) { return; }

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
            this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "missing mode");
            tc.attr_no_dashboard = false;
            tc.attr_no_toggle = false;
            tc.attr_is_on = true;
        }
    }

    m_current_module->tractioncontrol.push_back(tc);
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

    m_current_module->transfercase.push_back(tc);
}

void Parser::ParseSubmeshGroundModel()
{
    if (!this->CheckNumArguments(2)) { return; } // Items: keyword, arg

    m_current_module->submesh_groundmodel.push_back(this->GetArgStr(1));
}

void Parser::ParseSpeedLimiter()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    SpeedLimiter sl;
    sl.is_enabled = true;
    sl.max_speed = this->GetArgFloat(1);

    m_current_module->speedlimiter.push_back(sl);
}

void Parser::ParseSetSkeletonSettings()
{
    if (! this->CheckNumArguments(2)) { return; }

    if (m_current_module->set_skeleton_settings.size() == 0)
    {
        m_current_module->set_skeleton_settings.push_back(SkeletonSettings());
    }
    
    SkeletonSettings& skel = m_current_module->set_skeleton_settings[0];    
    skel.visibility_range_meters = this->GetArgFloat(1);
    if (m_num_args > 2) { skel.beam_thickness_meters = this->GetArgFloat(2); }
    
    // Defaults
    if (skel.visibility_range_meters < 0.f) { skel.visibility_range_meters = 150.f; }
    if (skel.beam_thickness_meters   < 0.f) { skel.beam_thickness_meters   = BEAM_SKELETON_DIAMETER; }
}

void Parser::ParseDirectiveSetNodeDefaults()
{
    if (!this->CheckNumArguments(2)) { return; }

    float load_weight   =                    this->GetArgFloat(1);
    float friction      = (m_num_args > 2) ? this->GetArgFloat(2) : -1;
    float volume        = (m_num_args > 3) ? this->GetArgFloat(3) : -1;
    float surface       = (m_num_args > 4) ? this->GetArgFloat(4) : -1;

    m_user_node_defaults = std::shared_ptr<NodeDefaults>( new NodeDefaults(*m_user_node_defaults) );

    m_user_node_defaults->load_weight = (load_weight < 0) ? m_ror_node_defaults->load_weight : load_weight;
    m_user_node_defaults->friction    = (friction    < 0) ? m_ror_node_defaults->friction    : friction;
    m_user_node_defaults->volume      = (volume      < 0) ? m_ror_node_defaults->volume      : volume;
    m_user_node_defaults->surface     = (surface     < 0) ? m_ror_node_defaults->surface     : surface;

    if (m_num_args > 5) m_user_node_defaults->options = this->GetArgNodeOptions(5);
}

void Parser::ParseDirectiveSetManagedMaterialsOptions()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    // Legacy behavior.
    m_current_managed_material_options.double_sided = (this->GetArgChar(1) != '0');
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
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    if (m_current_module->props.size() > 0)
    {
        m_current_module->props[m_current_module->props.size() - 1].camera_settings.mode = this->GetArgInt(1);
    }
    else
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "This line must come after a prop!");
    }
}

void Parser::ParseDirectiveSubmesh()
{
    this->BeginBlock(Keyword::INVALID); // flush the current submesh
    m_current_submesh = std::shared_ptr<Submesh>( new Submesh() );
}

void Parser::ParseDirectiveBackmesh()
{
    if (m_current_submesh)
    {
        m_current_submesh->backmesh = true;
    }
    else
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "must come after 'submesh'");
    }
}

void Parser::ProcessGlobalDirective(Keyword keyword)   // Directives that should only appear in root module
{
    switch (keyword)
    {
    case Keyword::DISABLEDEFAULTSOUNDS:      m_definition->disable_default_sounds = true;        return;
    case Keyword::ENABLE_ADVANCED_DEFORMATION:    m_definition->enable_advanced_deformation = true;   return;
    case Keyword::FORWARDCOMMANDS:           m_definition->forward_commands = true;              return;
    case Keyword::IMPORTCOMMANDS:           m_definition->import_commands = true;              return;
    case Keyword::HIDEINCHOOSER:           m_definition->hide_in_chooser = true;               return;
    case Keyword::LOCKGROUP_DEFAULT_NOLOCK:  m_definition->lockgroup_default_nolock = true;      return;
    case Keyword::RESCUER:                   m_definition->rescuer = true;                       return;
    case Keyword::ROLLON:                    m_definition->rollon = true;                        return;
    case Keyword::SLIDENODE_CONNECT_INSTANTLY: m_definition->slide_nodes_connect_instantly = true; return;

    default: return;
    }
}

void Parser::_ParseBaseMeshWheel(BaseMeshWheel& mesh_wheel)
{
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
}

void Parser::ParseMeshWheel()
{
    if (! this->CheckNumArguments(16)) { return; }

    MeshWheel mesh_wheel;
    this->_ParseBaseMeshWheel(mesh_wheel);

    if (m_sequential_importer.IsEnabled())
    {
        m_sequential_importer.GenerateNodesForWheel(Keyword::MESHWHEELS, mesh_wheel.num_rays, mesh_wheel.rigidity_node.IsValidAnyState());
    }

    m_current_module->meshwheels.push_back(mesh_wheel);
}

void Parser::ParseMeshWheel2()
{
    if (! this->CheckNumArguments(16)) { return; }

    MeshWheel2 mesh_wheel;
    this->_ParseBaseMeshWheel(mesh_wheel);

    if (m_sequential_importer.IsEnabled())
    {
        m_sequential_importer.GenerateNodesForWheel(Keyword::MESHWHEELS2, mesh_wheel.num_rays, mesh_wheel.rigidity_node.IsValidAnyState());
    }

    m_current_module->meshwheels2.push_back(mesh_wheel);
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
            this->LogMessage(Console::CONSOLE_SYSTEM_WARNING, fmt::format("ignoring invalid option '{}'", attr));
        }
        i++;
    }

    m_current_module->hooks.push_back(hook);
}

void Parser::ParseHelp()
{
    Help h;
    h.material = m_current_line; // already trimmed
    m_current_module->help.push_back(h);
}

void Parser::ParseGuiSettings()
{
    if (! this->CheckNumArguments(2)) { return; }

    GuiSettings gs;
    gs.key = this->GetArgStr(0);
    gs.value = this->GetArgStr(1);

    m_current_module->guisettings.push_back(gs);
}

void Parser::ParseGuid()
{
    if (! this->CheckNumArguments(2)) { return; }

    Guid g;
    g.guid = this->GetArgStr(1);

    m_current_module->guid.push_back(g);
}

void Parser::ParseGlobals()
{
    if (! this->CheckNumArguments(2)) { return; }

    Globals globals;
    globals.dry_mass   = this->GetArgFloat(0);
    globals.cargo_mass = this->GetArgFloat(1);

    if (m_num_args > 2) { globals.material_name = this->GetArgStr(2); }

    m_current_module->globals.push_back(globals);
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

void Parser::ParseDirectiveFlexbodyCameraMode()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    if (m_current_module->flexbodies.size() > 0)
    {
        m_current_module->flexbodies[m_current_module->flexbodies.size() - 1].camera_settings.mode = this->GetArgInt(1);
    }
    else
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "This line must come after a flexbody!");
    }
}

void Parser::ParseCab()
{
    if (! this->CheckNumArguments(3)) { return; }

    if (!m_current_submesh)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "must come after 'submesh'");
        return;
    }

    Cab cab;
    cab.nodes[0] = this->GetArgNodeRef(0);
    cab.nodes[1] = this->GetArgNodeRef(1);
    cab.nodes[2] = this->GetArgNodeRef(2);
    if (m_num_args > 3) cab.options = this->GetArgCabOptions(3);

    m_current_submesh->cab_triangles.push_back(cab);
}

void Parser::ParseTexcoords()
{
    if (! this->CheckNumArguments(3)) { return; }

    if (!m_current_submesh)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "must come after 'submesh'");
        return;
    }

    Texcoord texcoord;
    texcoord.node = this->GetArgNodeRef(0);
    texcoord.u    = this->GetArgFloat  (1);
    texcoord.v    = this->GetArgFloat  (2);

    m_current_submesh->texcoords.push_back(texcoord);
}

void Parser::ParseFlexbody()
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

    m_current_module->flexbodies.push_back(flexbody);
}

void Parser::ParseDirectiveForset()
{
    if (m_current_module->flexbodies.size() == 0)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_WARNING, "ignoring 'forset': no matching flexbody!");
        return;
    }

    Parser::ProcessForsetLine(m_current_module->flexbodies.back(), m_current_line, m_current_line_number);
}

void Parser::ProcessForsetLine(RigDef::Flexbody& def, const std::string& _line, int line_number /*= -1*/)
{
    // --------------------------------------------------------------------------------------------
    // BEWARE OF QUIRKS in the following code (they must be preserved for backwards compatibility):
    // - a space between the 'forset' keyword and arguments is optional.
    // - garbage characters anywhere on the line will silently add node 0 to the set.
    // - a separator at the end of line will silently add node 0 to the set.
    // --------------------------------------------------------------------------------------------

    //parsing set definition
    std::string line = _line;
    char* pos = &line[0] + 6; // 'forset' = 6 characters
    while (*pos == ' ' || *pos == ':' || *pos == ',') { pos++; } // Skip any separators
    char* end = pos;
    char endwas = 'G';
    while (endwas != 0)
    {
        int val1, val2; // Node numbers
        end = pos;
        while (*end != '-' && *end != ',' && *end != 0) end++;
        endwas = *end;
        *end = 0;
        val1 = strtoul(pos, 0, 10);
        if (endwas == '-')
        {
            pos = end + 1;
            end = pos;
            while (*end != ',' && *end != 0) end++;
            endwas = *end;
            *end = 0;
            val2 = strtoul(pos, 0, 10);
            // Add interval [val1-val2]
            def.node_list_to_import.push_back(
                Node::Range(
                    Node::Ref(std::to_string(val1), val1, Node::Ref::IMPORT_STATE_IS_VALID, line_number),
                    Node::Ref(std::to_string(val2), val2, Node::Ref::IMPORT_STATE_IS_VALID, line_number)));
        }
        else
        {
            // Add interval [val1-val1]
            Node::Range range_a = Node::Range(Node::Ref(std::to_string(val1), val1, Node::Ref::IMPORT_STATE_IS_VALID, line_number));
            def.node_list_to_import.push_back(range_a);
        }
        pos = end + 1;
    }
}

void Parser::ParseFlaresUnified()
{
    const bool is_flares2 = (m_current_block == Keyword::FLARES2);
    if (! this->CheckNumArguments(is_flares2 ? 6 : 5)) { return; }

    Flare2 flare2;
    int pos = 0;
    flare2.reference_node = this->GetArgNodeRef(pos++);
    flare2.node_axis_x    = this->GetArgNodeRef(pos++);
    flare2.node_axis_y    = this->GetArgNodeRef(pos++);
    flare2.offset.x       = this->GetArgFloat  (pos++);
    flare2.offset.y       = this->GetArgFloat  (pos++);

    if (m_current_block == Keyword::FLARES2)
    {
        flare2.offset.z = this->GetArgFloat(pos++);
    }

    if (m_num_args > pos) { flare2.type = this->GetArgFlareType(pos++); }

    if (m_num_args > pos)
    {
        switch (flare2.type)
        {
            case FlareType::USER:      flare2.control_number = this->GetArgInt(pos); break;
            case FlareType::DASHBOARD: flare2.dashboard_link = this->GetArgStr(pos); break;
            default: break;
        }
        pos++;
    }

    if (m_num_args > pos) { flare2.blink_delay_milis = this->GetArgInt      (pos++); }
    if (m_num_args > pos) { flare2.size              = this->GetArgFloat    (pos++); }
    if (m_num_args > pos) { flare2.material_name     = this->GetArgStr      (pos++); }

    m_current_module->flares2.push_back(flare2);
}

void Parser::ParseFlares3()
{
    const bool is_flares2 = (m_current_block == Keyword::FLARES2);
    if (! this->CheckNumArguments(is_flares2 ? 6 : 5)) { return; }

    Flare3 flare3;
    flare3.inertia_defaults = m_user_default_inertia;

    flare3.reference_node = this->GetArgNodeRef(0);
    flare3.node_axis_x    = this->GetArgNodeRef(1);
    flare3.node_axis_y    = this->GetArgNodeRef(2);
    flare3.offset.x       = this->GetArgFloat(3);
    flare3.offset.y       = this->GetArgFloat(4);
    flare3.offset.z       = this->GetArgFloat(5);
    if (m_num_args > 6) { flare3.type = this->GetArgFlareType(6); }

    if (m_num_args > 7)
    {
        switch (flare3.type)
        {
            case FlareType::USER:      flare3.control_number = this->GetArgInt(7); break;
            case FlareType::DASHBOARD: flare3.dashboard_link = this->GetArgStr(7); break;
            default: break;
        }
    }

    if (m_num_args > 8) { flare3.blink_delay_milis = this->GetArgInt      (8); }
    if (m_num_args > 9) { flare3.size              = this->GetArgFloat    (9); }
    if (m_num_args > 10) { flare3.material_name    = this->GetArgStr      (10); }

    m_current_module->flares3.push_back(flare3);
}

void Parser::ParseFixes()
{
    m_current_module->fixes.push_back(this->GetArgNodeRef(0));
}

void Parser::ParseExtCamera()
{
    if (! this->CheckNumArguments(2)) { return; }

    ExtCamera extcam;
    extcam.mode = this->GetArgExtCameraMode(1);
    if (m_num_args > 2) { extcam.node = this->GetArgNodeRef(2); }

    m_current_module->extcamera.push_back(extcam);
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

    FileFormatVersion ffv;
    ffv.version = this->GetArgInt(1);

    m_current_module->fileformatversion.push_back(ffv);
    m_current_block = Keyword::INVALID;
}

void Parser::ParseDirectiveDefaultSkin()
{
    if (!this->CheckNumArguments(2)) { return; } // 2 items: keyword, param

    DefaultSkin data;
    data.skin_name = this->GetArgStr(1);
    std::replace(data.skin_name.begin(), data.skin_name.end(), '_', ' ');

    m_current_module->default_skin.push_back(data);
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

    m_current_module->cruisecontrol.push_back(cruise_control);
}

void Parser::ParseDescription()
{
    m_current_module->description.push_back(m_current_line); // Already trimmed
}

void Parser::ParseDirectiveAddAnimation()
{
    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 14, ","); // "add_animation " = 14 characters
    m_num_args = (int)tokens.size();
    if (! this->CheckNumArguments(4)) { return; }

    if (m_current_module->props.size() == 0)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_WARNING, "'add_animation' must come after prop, ignoring...");
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
        else if (entry.size() == 2 && (entry[0] == "mode" || entry[0] == "event" || entry[0] == "source" || entry[0] == "link"))
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
                animation.event_name = entry[1];
                Ogre::StringUtil::trim(animation.event_name);
                Ogre::StringUtil::toUpperCase(animation.event_name);
            }
            else if (entry[0] == "link")
            {
                animation.dash_link_name = entry[1];
                Ogre::StringUtil::trim(animation.dash_link_name);
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
                    else if (value == "autoshifterlin"){ animation.source |= Animation::SOURCE_AUTOSHIFTERLIN;    }
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
                    else if (value == "dashboard")     { animation.source |= Animation::SOURCE_DASHBOARD;         }
                    else if (value == "signalstalk")   { animation.source |= Animation::SOURCE_SIGNALSTALK;       }
                    else if (value == "gearreverse")   { animation.source |= Animation::SOURCE_GEAR_REVERSE;      }
                    else if (value == "gearneutral")   { animation.source |= Animation::SOURCE_GEAR_NEUTRAL;      }

                    else
                    {
                        Animation::MotorSource motor_source;
                        // aeroengines...
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
                        // gears... (hack)
                        else if (entry[1].compare(0, 4, "gear") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_GEAR_FORWARD;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(4));
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
            this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                fmt::format("Ignoring invalid token '{}' ({})", itor->c_str(), warn_msg));
        }
    }

    m_current_module->props.back().animations.push_back(animation);
}

void Parser::ParseAntiLockBrakes()
{
    AntiLockBrakes alb;
    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 15, ","); // "AntiLockBrakes " = 15 characters
    m_num_args = (int)tokens.size();
    if (! this->CheckNumArguments(2)) { return; }

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
            this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "missing mode");
            alb.attr_no_dashboard = false;
            alb.attr_no_toggle = false;
            alb.attr_is_on = true;
        }
    }

    m_current_module->antilockbrakes.push_back(alb);
}

void Parser::ParseEngoption()
{
    if (! this->CheckNumArguments(1)) { return; }

    Engoption engoption;
    engoption.inertia = this->GetArgFloat(0);

    if (m_num_args > 1) { engoption.type             = this->GetArgEngineType(1); }
    if (m_num_args > 2) { engoption.clutch_force     = this->GetArgFloat(2); }
    if (m_num_args > 3) { engoption.shift_time       = this->GetArgFloat(3); }
    if (m_num_args > 4) { engoption.clutch_time      = this->GetArgFloat(4); }
    if (m_num_args > 5) { engoption.post_shift_time  = this->GetArgFloat(5); }
    if (m_num_args > 6) { engoption.stall_rpm        = this->GetArgFloat(6); }
    if (m_num_args > 7) { engoption.idle_rpm         = this->GetArgFloat(7); }
    if (m_num_args > 8) { engoption.max_idle_mixture = this->GetArgFloat(8); }
    if (m_num_args > 9) { engoption.min_idle_mixture = this->GetArgFloat(9); }
    if (m_num_args > 10){ engoption.braking_torque   = this->GetArgFloat(10);}

    m_current_module->engoption.push_back(engoption);
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
        this->LogMessage(Console::CONSOLE_SYSTEM_WARNING, "You cannot have more than 4 turbos. Fallback: using 4 instead.");
        engturbo.nturbos = 4;
    }

    m_current_module->engturbo.push_back(engturbo);
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

    // Forward gears
    for (int i = 6; i < m_num_args; i++)
    {
        float ratio = this->GetArgFloat(i);
        if (ratio < 0.f)
        {
            break; // Optional terminator argument
        }
        engine.gear_ratios.push_back(ratio);   
    }

    if (engine.gear_ratios.size() == 0)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "no forward gear");
        return;
    }

    m_current_module->engine.push_back(engine);
}

void Parser::ParseContacter()
{
    if (! this->CheckNumArguments(1)) { return; }

    m_current_module->contacters.push_back(this->GetArgNodeRef(0));
}

void Parser::ParseCommandsUnified()
{
    const bool is_commands2 = (m_current_block == Keyword::COMMANDS2);
    const int max_args = (is_commands2 ? 8 : 7);
    if (! this->CheckNumArguments(max_args)) { return; }

    Command2 command2;
    command2.beam_defaults     = m_user_beam_defaults;
    command2.detacher_group    = m_current_detacher_group;
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
        m_current_module->commands2.push_back(command2);
        return;
    }

    // Parse options
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
            this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                fmt::format("ignoring unknown flag '{}'", c));
        }
    }

    // Resolve option conflicts
    if (command2.option_c_auto_center && winner != 'c' && winner != 0)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_WARNING, "Command cannot be one-pressed and self centering at the same time, ignoring flag 'c'");
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
        this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
            "Command cannot be one-pressed and self centering at the same time, ignoring flag '%c'");
    }
    else if (ignored != 0 && (winner == 'o' || winner == 'p'))
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
            "Command already has a one-pressed c.mode, ignoring flag '%c'");
    }

    if (m_num_args > pos) { command2.description   = this->GetArgStr  (pos++);}

    if (m_num_args > pos) { ParseOptionalInertia(command2.inertia, pos); pos += 4; }

    if (m_num_args > pos) { command2.affect_engine = this->GetArgFloat(pos++);}
    if (m_num_args > pos) { command2.needs_engine  = this->GetArgBool (pos++);}
    if (m_num_args > pos) { command2.plays_sound   = this->GetArgBool (pos++);}

    m_current_module->commands2.push_back(command2);
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

    m_current_module->collisionboxes.push_back(collisionbox);
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
        m_sequential_importer.AddGeneratedNode(Keyword::CINECAM);
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

    Brakes brakes;
    brakes.default_braking_force = this->GetArgFloat(0);
    if (m_num_args > 1)
    {
        brakes.parking_brake_force = this->GetArgFloat(1);
    }
    m_current_module->brakes.push_back(brakes);
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
            this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "Invalid property, ignoring whole line...");
            return;
        }
        // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

        if (results[1].matched)
        {
            unsigned int wheel_index = PARSEINT(results[2]) - 1;
            axle.wheels[wheel_index][0] = _ParseNodeRef(results[3]);
            axle.wheels[wheel_index][1] = _ParseNodeRef(results[4]);
        }
        else if (results[5].matched)
        {
            this->_ParseDifferentialTypes(axle.options, results[6].str());
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
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "Invalid property, ignoring whole line...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (results[5].matched)
    {
        this->_ParseDifferentialTypes(interaxle.options, results[6].str());
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

void Parser::ParseAssetpacks()
{
    if (! this->CheckNumArguments(1)) { return; }

    Assetpack assetpack;

    assetpack.filename = this->GetArgStr(0);

    m_current_module->assetpacks.push_back(assetpack);
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
    bool is_turboprop_2 = m_current_block == Keyword::TURBOPROPS2;

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
    
    m_current_module->turboprops2.push_back(turboprop);
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
    trigger.shortbound_trigger_action = this->GetArgInt    (4); 
    trigger.longbound_trigger_action  = this->GetArgInt    (5); 
    if (m_num_args > 6) trigger.options = this->GetArgTriggerOptions(6);
    if (m_num_args > 7) trigger.boundary_timer = this->GetArgFloat(7);

    m_current_module->triggers.push_back(trigger);
}

void Parser::ParseTorqueCurve()
{
    if (m_current_module->torquecurve.size() == 0)
    {
        m_current_module->torquecurve.push_back(TorqueCurve());
    }

    Ogre::StringVector args = Ogre::StringUtil::split(m_current_line, ",");
    
    if (args.size() == 1u)
    {
        m_current_module->torquecurve[0].predefined_func_name = args[0];
    }
    else if (args.size() == 2u)
    {
        TorqueCurve::Sample sample;
        sample.power          = this->ParseArgFloat(args[0].c_str());
        sample.torque_percent = this->ParseArgFloat(args[1].c_str());
        m_current_module->torquecurve[0].samples.push_back(sample);  
    }
    else
    {
        // Consistent with 0.38's parser.
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "too many arguments, skipping");
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
            case (char)TieOption::n_DUMMY:
            case (char)TieOption::v_DUMMY:
                break;

            case (char)TieOption::i_INVISIBLE:
                tie.options |= Tie::OPTION_i_INVISIBLE;
                break;

            case (char)TieOption::s_NO_SELF_LOCK:
                tie.options |= Tie::OPTION_s_DISABLE_SELF_LOCK;
                break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid option '{}'", c));
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
    soundsource2.mode              = this->GetArgInt(1);
    soundsource2.sound_script_name = this->GetArgStr(2);

    if (soundsource2.mode < -2)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("invalid mode {}, falling back to default -2", soundsource2.mode));
        soundsource2.mode = -2;
    }

    m_current_module->soundsources2.push_back(soundsource2);
}

void Parser::ParseSlidenodes()
{
    Ogre::StringVector args = Ogre::StringUtil::split(m_current_line, ", ");
    m_num_args = (int)args.size();
    if (! this->CheckNumArguments(2)) { return; }

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
            slidenode._spring_rate_set = true;
            in_rail_node_list = false;
            break;
        case 'B':
            slidenode.break_force = this->ParseArgFloat(itor->substr(1));
            slidenode._break_force_set = true;
            in_rail_node_list = false;
            break;
        case 'T':
            slidenode.tolerance = this->ParseArgFloat(itor->substr(1));
            slidenode._tolerance_set = true;
            in_rail_node_list = false;
            break;
        case 'R':
            slidenode.attachment_rate = this->ParseArgFloat(itor->substr(1));
            slidenode._attachment_rate_set = true;
            in_rail_node_list = false;
            break;
        case 'G':
            slidenode.railgroup_id = this->ParseArgFloat(itor->substr(1));
            slidenode._railgroup_id_set = true;
            in_rail_node_list = false;
            break;
        case 'D':
            slidenode.max_attach_dist = this->ParseArgFloat(itor->substr(1));
            slidenode._max_attach_dist_set = true;
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
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("Ignoring invalid option '{}'", itor->at(1)));
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

    if (m_num_args > 15) shock_3.options = this->GetArgShock3Options(15);

    m_current_module->shocks3.push_back(shock_3);
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

    if (m_num_args > 13) shock_2.options = this->GetArgShock2Options(13);

    m_current_module->shocks2.push_back(shock_2);
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
    if (m_num_args > 7) shock.options = this->GetArgShockOptions(7);

    m_current_module->shocks.push_back(shock);
}

Node::Ref Parser::_ParseNodeRef(std::string const & node_id_str)
{
    if (m_sequential_importer.IsEnabled())
    {
        // Import of legacy fileformatversion
        int node_id_num = PARSEINT(node_id_str);
        if (node_id_num < 0)
        {
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

    m_set_default_minimass = std::shared_ptr<DefaultMinimass>(new DefaultMinimass());
    m_set_default_minimass->min_mass_Kg = this->GetArgFloat(1);
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
    if (!this->CheckNumArguments(1)) { return; }

    Script script;

    script.filename = this->GetArgStr(0);

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

    if (m_current_block == Keyword::ROTATORS2)
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

    if (m_current_block == Keyword::ROTATORS2)
    {
        m_current_module->rotators2.push_back(rotator);
    }
    else
    {
        m_current_module->rotators.push_back(rotator);
    }
}

void Parser::ParseFileinfo()
{
    if (! this->CheckNumArguments(2)) { return; }

    Fileinfo fileinfo;

    fileinfo.unique_id = this->GetArgStr(1);
    Ogre::StringUtil::trim(fileinfo.unique_id);

    if (m_num_args > 2) { fileinfo.category_id  = this->GetArgInt(2); }
    if (m_num_args > 3) { fileinfo.file_version = this->GetArgInt(3); }

    m_current_module->fileinfo.push_back(fileinfo);

    m_current_block = Keyword::INVALID;
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
    m_num_args = (int)args.size();
    if (! this->CheckNumArguments(3)) { return; }

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
    prop.mesh_name      = this->GetArgStr(9);
    prop.special        = Parser::IdentifySpecialProp(prop.mesh_name);

    if ((prop.special == SpecialProp::BEACON) && (m_num_args >= 14))
    {
        prop.special_prop_beacon.flare_material_name = this->GetArgStr(10);
        Ogre::StringUtil::trim(prop.special_prop_beacon.flare_material_name);

        prop.special_prop_beacon.color = Ogre::ColourValue(
            this->GetArgFloat(11), this->GetArgFloat(12), this->GetArgFloat(13));
    }
    else if (prop.special == SpecialProp::DASHBOARD_LEFT ||
             prop.special == SpecialProp::DASHBOARD_RIGHT)
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

void Parser::ParseNodesUnified()
{
    if (! this->CheckNumArguments(4)) { return; }

    Node node;
    node.node_defaults = m_user_node_defaults;
    node.beam_defaults = m_user_beam_defaults;
    node.default_minimass = m_set_default_minimass;
    node.detacher_group = m_current_detacher_group;

    if (m_current_block == Keyword::NODES2)
    {
        std::string node_name = this->GetArgStr(0);
        node.id.setStr(node_name);
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
        node.options = this->GetArgNodeOptions(4);
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
            this->LogMessage(Console::CONSOLE_SYSTEM_WARNING, 
                "Node has load-weight-override value specified, but option 'l' is not present. Ignoring value...");
        }
    }

    m_current_module->nodes.push_back(node);
}

void Parser::ParseMinimass()
{
    if (! this->CheckNumArguments(1)) { return; }

    Minimass mm;
    mm.global_min_mass_Kg = this->GetArgFloat(0);
    if (m_num_args > 1) { mm.option = this->GetArgMinimassOption(1); }

    m_current_module->minimass.push_back(mm);
    m_current_block = Keyword::INVALID;
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
        m_sequential_importer.GenerateNodesForWheel(Keyword::FLEXBODYWHEELS, flexbody_wheel.num_rays, flexbody_wheel.rigidity_node.IsValidAnyState());
    }

    m_current_module->flexbodywheels.push_back(flexbody_wheel);
}

void Parser::ParseMaterialFlareBindings()
{
    if (! this->CheckNumArguments(2)) { return; }

    MaterialFlareBinding binding;
    binding.flare_number  = this->GetArgInt(0);
    binding.material_name = this->GetArgStr(1);
    
    m_current_module->materialflarebindings.push_back(binding);
}

void Parser::ParseManagedMaterials()
{
    if (! this->CheckNumArguments(2)) { return; }

    ManagedMaterial managed_mat;
    managed_mat.options = m_current_managed_material_options;
    managed_mat.name    = this->GetArgStr(0);
    managed_mat.type    = this->GetArgManagedMatType(1);

    if (managed_mat.type != ManagedMaterialType::INVALID)
    {
        if (! this->CheckNumArguments(3)) { return; }

        managed_mat.diffuse_map = this->GetArgStr(2);

        if (managed_mat.type == ManagedMaterialType::MESH_STANDARD ||
            managed_mat.type == ManagedMaterialType::MESH_TRANSPARENT)
        {
            if (m_num_args > 3) { managed_mat.specular_map = this->GetArgManagedTex(3); }
        }
        else if (managed_mat.type == ManagedMaterialType::FLEXMESH_STANDARD ||
            managed_mat.type == ManagedMaterialType::FLEXMESH_TRANSPARENT)
        {
            if (m_num_args > 3) { managed_mat.damaged_diffuse_map = this->GetArgManagedTex(3); }
            if (m_num_args > 4) { managed_mat.specular_map        = this->GetArgManagedTex(4); }
        }

        m_current_module->managedmaterials.push_back(managed_mat);
    }
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
    
    if (m_num_args > 3) { hydro.options = this->GetArgHydroOptions(3); }

    if (!hydro.options)
    {
        hydro.options |= Hydro::OPTION_n_INPUT_NORMAL;
    }
    
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

void Parser::_ParseDifferentialTypes(DifferentialTypeVec& diff_types, std::string const& options_str)
{
    for (char c: options_str)
    {
        switch(c)
        {
            case (char)DifferentialType::o_OPEN:
            case (char)DifferentialType::l_LOCKED:
            case (char)DifferentialType::s_SPLIT:
            case (char)DifferentialType::v_VISCOUS:
                diff_types.push_back(DifferentialType(c));
                break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid differential type '{}'", c));
                break;
        }
    }
}

void Parser::ParseBeams()
{
    if (! this->CheckNumArguments(2)) { return; }

    Beam beam;
    beam.defaults       = m_user_beam_defaults;
    beam.detacher_group = m_current_detacher_group;

    beam.nodes[0] = this->GetArgNodeRef(0);
    beam.nodes[1] = this->GetArgNodeRef(1);
    if (m_num_args > 2) beam.options = this->GetArgBeamOptions(2);

    if ((m_num_args > 3) && BITMASK_IS_1(beam.options, Beam::OPTION_s_SUPPORT))
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

            animator.aero_animator.engine_idx = this->ParseArgUint(results[2].str().c_str()) - 1;
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
    if (! this->CheckNumArguments(2)) { return; }

    Author author;
    if (m_num_args > 1) { author.type             = this->GetArgStr(1); }
    if (m_num_args > 2) { author.forum_account_id = this->GetArgInt(2); author._has_forum_account = true; }
    if (m_num_args > 3) { author.name             = this->GetArgStr(3); }
    if (m_num_args > 4) { author.email            = this->GetArgStr(4); }

    m_current_module->author.push_back(author);
    m_current_block = Keyword::INVALID;
}

// -------------------------------------------------------------------------- 
//  Utilities
// -------------------------------------------------------------------------- 

void Parser::LogMessage(Console::MessageType type, std::string const& msg)
{
    App::GetConsole()->putMessage(
        Console::CONSOLE_MSGTYPE_ACTOR,
        type,
        fmt::format("{}:{} ({}): {}",
            m_filename, m_current_line_number, KeywordToString(m_log_keyword), msg));
}

Keyword Parser::IdentifyKeyword(const std::string& line)
{
    // Search and ignore lettercase
    std::smatch results;
    std::regex_search(line, results, Regexes::IDENTIFY_KEYWORD_IGNORE_CASE); // Always returns true.

    // The 'results' array contains a complete match at positon [0] and sub-matches starting with [1], 
    //    so we get exact positions in Regexes::IDENTIFY_KEYWORD, which again match Keyword enum members
    for (unsigned int i = 1; i < results.size(); i++)
    {
        std::ssub_match sub  = results[i];
        if (sub.matched)
        {
            // Build enum value directly from result offset
            return Keyword(i);
        }
    }

    return Keyword::INVALID;
}

void Parser::Prepare()
{
    m_current_block = Keyword::INVALID;
    m_current_line_number = 1;
    m_definition = RigDef::DocumentPtr(new Document());
    m_any_named_node_defined = false;
    m_current_detacher_group = 0; // Global detacher group 

    m_user_default_inertia = m_ror_default_inertia;
    m_user_node_defaults   = m_ror_node_defaults;
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

void Parser::BeginBlock(Keyword keyword)
{
    if (keyword == Keyword::INVALID) // also means 'end'
    {
        // flush staged submesh, if any
        if (m_current_submesh != nullptr)
        {
            m_current_module->submeshes.push_back(*m_current_submesh);
            m_current_submesh.reset(); // Set to nullptr
        }

        // flush staged camerarail, if any
        if (m_current_camera_rail != nullptr)
        {
            if (m_current_camera_rail->nodes.size() == 0)
            {
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING, "Empty section 'camerarail', ignoring...");
            }
            else
            {
                m_current_module->camerarail.push_back(*m_current_camera_rail);
                m_current_camera_rail.reset();
            }
        }
    }
    else if (keyword == Keyword::CAMERARAIL)
    {
        this->BeginBlock(Keyword::INVALID); // flush staged rail
        m_current_camera_rail = std::shared_ptr<CameraRail>( new CameraRail() );
    }
    m_current_block = keyword;
}

void Parser::ProcessChangeModuleLine(Keyword keyword)
{
    // Determine and verify new module
    std::string new_module_name;
    if (keyword == Keyword::END_SECTION)
    {
        if (m_current_module == m_root_module)
        {
            this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "Misplaced keyword 'end_section' (already in root module), ignoring...");
            return;
        }
        new_module_name = ROOT_MODULE_NAME;
    }
    else if (keyword == Keyword::SECTION)
    {
        if (!this->CheckNumArguments(3)) // Syntax: "section VERSION NAME"; VERSION is unused
        {
            return; // Error already reported
        }

        new_module_name = this->GetArgStr(2);
        if (new_module_name == m_current_module->name)
        {
            this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "Attempt to re-enter current module, ignoring...");
            return;
        }
    }

    // Perform the switch
    this->BeginBlock(Keyword::INVALID);

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
        m_current_module = std::make_shared<Document::Module>(new_module_name);
        m_definition->user_modules.insert(std::make_pair(new_module_name, m_current_module));
    }
}

void Parser::ParseDirectiveSection()
{
    this->ProcessChangeModuleLine(Keyword::SECTION);
}

void Parser::ParseDirectiveSectionConfig()
{
    // FIXME: restore this, see branch 'retro-0407'
}

void Parser::Finalize()
{
    this->BeginBlock(Keyword::INVALID);

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

WheelSide Parser::GetArgWheelSide(int index)
{
    char c = this->GetArgChar(index);
    switch (c)
    {
        case (char)WheelSide::RIGHT:
        case (char)WheelSide::LEFT:
            return WheelSide(c);

        default:
            this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                fmt::format("Bad arg~{} 'side' (value: {}), parsing as 'l' for backwards compatibility.", index + 1, c));
            return WheelSide::LEFT;
    }
}

long Parser::GetArgLong(int index)
{
    errno = 0;
    char* out_end = nullptr;
    long res = std::strtol(m_args[index].start, &out_end, 10);
    if (errno != 0)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Cannot parse argument [{}] as integer, errno: {}", index + 1, errno));
        return 0; // Compatibility
    }
    if (out_end == m_args[index].start)
    {
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Argument [{}] is not valid integer", index + 1));
        return 0; // Compatibility
    }
    else if (out_end != (m_args[index].start + m_args[index].length))
    {;
        this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Integer argument [{}] has invalid trailing characters", index + 1));
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

WheelPropulsion Parser::GetArgPropulsion(int index)
{
    int p = this->GetArgInt(index);
    switch (p)
    {
        case (int)WheelPropulsion::NONE:
        case (int)WheelPropulsion::FORWARD:
        case (int)WheelPropulsion::BACKWARD:
            return WheelPropulsion(p);

        default:
            this->LogMessage(Console::CONSOLE_SYSTEM_ERROR,
                fmt::format("Bad value of param ~{} (propulsion), using 0 (no propulsion)", index + 1));
            return WheelPropulsion::NONE;
    }
}

WheelBraking Parser::GetArgBraking(int index)
{
    int b = this->GetArgInt(index);
    switch (b)
    {
        case (int)WheelBraking::NONE:
        case (int)WheelBraking::FOOT_HAND:
        case (int)WheelBraking::FOOT_HAND_SKID_LEFT:
        case (int)WheelBraking::FOOT_HAND_SKID_RIGHT:
        case (int)WheelBraking::FOOT_ONLY:
            return WheelBraking(b);

        default:
            this->LogMessage(Console::CONSOLE_SYSTEM_ERROR,
                fmt::format("Bad value of param ~{} (braking), using 0 (not braked)", index + 1));
            return WheelBraking::NONE;
    }
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

FlareType Parser::GetArgFlareType(int index)
{
    char in = this->GetArgChar(index);
    switch (in)
    {
        // Front lights
        case (char)FlareType::HEADLIGHT: 
        case (char)FlareType::HIGH_BEAM:
        case (char)FlareType::FOG_LIGHT:
        // Rear lighs
        case (char)FlareType::TAIL_LIGHT:
        case (char)FlareType::BRAKE_LIGHT:
        case (char)FlareType::REVERSE_LIGHT:
        // Special lights
        case (char)FlareType::SIDELIGHT:
        case (char)FlareType::BLINKER_LEFT:
        case (char)FlareType::BLINKER_RIGHT:
        case (char)FlareType::USER:
        case (char)FlareType::DASHBOARD:
            return FlareType(in);

        default:
            this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                fmt::format("Invalid flare type '{}', falling back to type 'f' (front light)...", in));
            return FlareType::HEADLIGHT;
    }
}

ExtCameraMode Parser::GetArgExtCameraMode(int index)
{
    std::string str = this->GetArgStr(index);
    if (str == "classic") return ExtCameraMode::CLASSIC;
    if (str == "cinecam") return ExtCameraMode::CINECAM;
    if (str == "node")    return ExtCameraMode::NODE;

    this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
        fmt::format("Invalid ExtCameraMode '{}', falling back to type 'classic'...", str));
    return ExtCameraMode::CLASSIC;
}

float Parser::GetArgFloat(int index)
{
    return (float) Ogre::StringConverter::parseReal(this->GetArgStr(index), 0.f);
}

float Parser::ParseArgFloat(const char* str)
{
    return (float) Ogre::StringConverter::parseReal(str, 0.f);
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
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Cannot parse argument '{}' as int, errno: {}", str, errno));
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

WingControlSurface Parser::GetArgWingSurface(int index)
{
    char c = this->GetArgChar(index);
    switch (c)
    {
        case (char)WingControlSurface::n_NONE:
        case (char)WingControlSurface::a_RIGHT_AILERON:
        case (char)WingControlSurface::b_LEFT_AILERON:
        case (char)WingControlSurface::f_FLAP:
        case (char)WingControlSurface::e_ELEVATOR:
        case (char)WingControlSurface::r_RUDDER:
        case (char)WingControlSurface::S_RIGHT_HAND_STABILATOR:
        case (char)WingControlSurface::T_LEFT_HAND_STABILATOR:
        case (char)WingControlSurface::c_RIGHT_ELEVON:
        case (char)WingControlSurface::d_LEFT_ELEVON:
        case (char)WingControlSurface::g_RIGHT_FLAPERON:
        case (char)WingControlSurface::h_LEFT_FLAPERON:
        case (char)WingControlSurface::U_RIGHT_HAND_TAILERON:
        case (char)WingControlSurface::V_LEFT_HAND_TAILERON:
        case (char)WingControlSurface::i_RIGHT_RUDDERVATOR:
        case (char)WingControlSurface::j_LEFT_RUDDERVATOR:
            return WingControlSurface(c);

        default:
            fmt::format("invalid WingControlSurface '{}', falling back to 'n' (none)", c);
            return WingControlSurface::n_NONE;
    }
}

std::string Parser::GetArgManagedTex(int index)
{
    std::string tex_name = this->GetArgStr(index);
    return (tex_name.at(0) != '-') ? tex_name : "";
}

MinimassOption Parser::GetArgMinimassOption(int index)
{
    switch (this->GetArgStr(index)[0])
    {
        case (char)MinimassOption::l_SKIP_LOADED:
            return MinimassOption::l_SKIP_LOADED;

        case (char)MinimassOption::n_DUMMY:
            return MinimassOption::n_DUMMY;

        default:
            this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                fmt::format("Not a valid minimass option: {}, falling back to 'n' (dummy)", this->GetArgStr(index)));
            return MinimassOption::n_DUMMY;
    }
}

BitMask_t Parser::GetArgCabOptions(int index)
{
    BitMask_t ret = 0;
    for (char c: this->GetArgStr(index))
    {
        switch (c)
        {
            case (char)CabOption::c_CONTACT:              ret |= Cab::OPTION_c_CONTACT             ; break;
            case (char)CabOption::b_BUOYANT:              ret |= Cab::OPTION_b_BUOYANT             ; break;
            case (char)CabOption::p_10xTOUGHER:           ret |= Cab::OPTION_p_10xTOUGHER          ; break;
            case (char)CabOption::u_INVULNERABLE:         ret |= Cab::OPTION_u_INVULNERABLE        ; break;
            case (char)CabOption::s_BUOYANT_NO_DRAG:      ret |= Cab::OPTION_s_BUOYANT_NO_DRAG     ; break;
            case (char)CabOption::r_BUOYANT_ONLY_DRAG:    ret |= Cab::OPTION_r_BUOYANT_ONLY_DRAG   ; break;
            case (char)CabOption::D_CONTACT_BUOYANT:      ret |= Cab::OPTION_D_CONTACT_BUOYANT     ; break;
            case (char)CabOption::F_10xTOUGHER_BUOYANT:   ret |= Cab::OPTION_F_10xTOUGHER_BUOYANT  ; break;
            case (char)CabOption::S_INVULNERABLE_BUOYANT: ret |= Cab::OPTION_S_INVULNERABLE_BUOYANT; break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid flag '{}'", c));
        }
    }
    return ret;
}

BitMask_t Parser::GetArgTriggerOptions(int index)
{
    BitMask_t ret = 0;
    for (char c: this->GetArgStr(index))
    {
        switch(c)
        {
            case (char)TriggerOption::i_INVISIBLE          : ret |= Trigger::OPTION_i_INVISIBLE;           break;
            case (char)TriggerOption::c_COMMAND_STYLE      : ret |= Trigger::OPTION_c_COMMAND_STYLE;       break;
            case (char)TriggerOption::x_START_DISABLED     : ret |= Trigger::OPTION_x_START_DISABLED;      break;
            case (char)TriggerOption::b_KEY_BLOCKER        : ret |= Trigger::OPTION_b_KEY_BLOCKER;         break;
            case (char)TriggerOption::B_TRIGGER_BLOCKER    : ret |= Trigger::OPTION_B_TRIGGER_BLOCKER;     break;
            case (char)TriggerOption::A_INV_TRIGGER_BLOCKER: ret |= Trigger::OPTION_A_INV_TRIGGER_BLOCKER; break;
            case (char)TriggerOption::s_CMD_NUM_SWITCH     : ret |= Trigger::OPTION_s_CMD_NUM_SWITCH;      break;
            case (char)TriggerOption::h_UNLOCKS_HOOK_GROUP : ret |= Trigger::OPTION_h_UNLOCKS_HOOK_GROUP;  break;
            case (char)TriggerOption::H_LOCKS_HOOK_GROUP   : ret |= Trigger::OPTION_H_LOCKS_HOOK_GROUP;    break;
            case (char)TriggerOption::t_CONTINUOUS         : ret |= Trigger::OPTION_t_CONTINUOUS;          break;
            case (char)TriggerOption::E_ENGINE_TRIGGER     : ret |= Trigger::OPTION_E_ENGINE_TRIGGER;      break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid option '{}'", c));
        }
    }
    return ret;
}

BitMask_t Parser::GetArgBeamOptions(int index)
{
    BitMask_t ret = 0;
    for (char c: this->GetArgStr(index))
    {
        switch (c)
        {
            case (char)BeamOption::i_INVISIBLE: ret |= Beam::OPTION_i_INVISIBLE; break;
            case (char)BeamOption::r_ROPE     : ret |= Beam::OPTION_r_ROPE     ; break;
            case (char)BeamOption::s_SUPPORT  : ret |= Beam::OPTION_s_SUPPORT  ; break;

            case (char)BeamOption::v_DUMMY: break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid option '{}'", c));
        }
    }
    return ret;
}

BitMask_t Parser::GetArgHydroOptions (int index)
{
    BitMask_t ret = 0;
    for (char c: this->GetArgStr(index))
    {
        switch (c)
        {
            case (char)HydroOption::j_INVISIBLE                : ret |= Hydro::OPTION_j_INVISIBLE                ; break;
            case (char)HydroOption::s_DISABLE_ON_HIGH_SPEED    : ret |= Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED    ; break;
            case (char)HydroOption::a_INPUT_AILERON            : ret |= Hydro::OPTION_a_INPUT_AILERON            ; break;
            case (char)HydroOption::r_INPUT_RUDDER             : ret |= Hydro::OPTION_r_INPUT_RUDDER             ; break;
            case (char)HydroOption::e_INPUT_ELEVATOR           : ret |= Hydro::OPTION_e_INPUT_ELEVATOR           ; break;
            case (char)HydroOption::u_INPUT_AILERON_ELEVATOR   : ret |= Hydro::OPTION_u_INPUT_AILERON_ELEVATOR   ; break;
            case (char)HydroOption::v_INPUT_InvAILERON_ELEVATOR: ret |= Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR; break;
            case (char)HydroOption::x_INPUT_AILERON_RUDDER     : ret |= Hydro::OPTION_x_INPUT_AILERON_RUDDER     ; break;
            case (char)HydroOption::y_INPUT_InvAILERON_RUDDER  : ret |= Hydro::OPTION_y_INPUT_InvAILERON_RUDDER  ; break;
            case (char)HydroOption::g_INPUT_ELEVATOR_RUDDER    : ret |= Hydro::OPTION_g_INPUT_ELEVATOR_RUDDER    ; break;
            case (char)HydroOption::h_INPUT_InvELEVATOR_RUDDER : ret |= Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER ; break;
            case (char)HydroOption::n_INPUT_NORMAL             : ret |= Hydro::OPTION_n_INPUT_NORMAL             ; break;

            case (char)HydroOption::i_INVISIBLE_INPUT_NORMAL:
                if (ret == 0)
                {
                    // Original intent: when using 'i' flag alone, also force 'n' (steering input).
                    // For backward compatibility, do it every time 'i' comes first, even if not alone.
                    ret |= Hydro::OPTION_n_INPUT_NORMAL;
                }
                ret |= Hydro::OPTION_j_INVISIBLE;
                break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid option '{}'", c));
        }
    }

    return ret;
}

BitMask_t Parser::GetArgShockOptions(int index)
{
    BitMask_t ret = 0;
    for (char c: this->GetArgStr(index))
    {
        switch (c)
        {
            case (char)ShockOption::i_INVISIBLE    : ret |= Shock::OPTION_i_INVISIBLE   ; break;
            case (char)ShockOption::L_ACTIVE_LEFT  : ret |= Shock::OPTION_L_ACTIVE_LEFT ; break;
            case (char)ShockOption::R_ACTIVE_RIGHT : ret |= Shock::OPTION_R_ACTIVE_RIGHT; break;
            case (char)ShockOption::m_METRIC       : ret |= Shock::OPTION_m_METRIC      ; break;

            case (char)ShockOption::n_DUMMY: break;
            case (char)ShockOption::v_DUMMY: break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid option '{}'", c));
        }
    }
    return ret;
}

BitMask_t Parser::GetArgShock2Options(int index)
{
    BitMask_t ret = 0;
    for (char c: this->GetArgStr(index))
    {
        switch (c)
        {
            case (char)Shock2Option::i_INVISIBLE:        ret |= Shock2::OPTION_i_INVISIBLE;        break;
            case (char)Shock2Option::s_SOFT_BUMP_BOUNDS: ret |= Shock2::OPTION_s_SOFT_BUMP_BOUNDS; break;
            case (char)Shock2Option::m_METRIC:           ret |= Shock2::OPTION_m_METRIC;           break;
            case (char)Shock2Option::M_ABSOLUTE_METRIC:  ret |= Shock2::OPTION_M_ABSOLUTE_METRIC;  break;

            case (char)Shock2Option::n_DUMMY: break;
            case (char)Shock2Option::v_DUMMY: break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid option '{}'", c));
        }
    }
    return ret;
}

BitMask_t Parser::GetArgShock3Options(int index)
{
    BitMask_t ret = 0;
    for (char c: this->GetArgStr(index))
    {
        switch (c)
        {
            case (char)Shock3Option::i_INVISIBLE:        ret |= Shock3::OPTION_i_INVISIBLE;        break;
            case (char)Shock3Option::m_METRIC:           ret |= Shock3::OPTION_m_METRIC;           break;
            case (char)Shock3Option::M_ABSOLUTE_METRIC:  ret |= Shock3::OPTION_M_ABSOLUTE_METRIC;  break;

            case (char)Shock3Option::n_DUMMY: break;
            case (char)Shock3Option::v_DUMMY: break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid option '{}'", c));
        }
    }
    return ret;
}

BitMask_t Parser::GetArgNodeOptions(int index)
{
    BitMask_t ret = 0;
    for (char c: this->GetArgStr(index))
    {
        switch (c)
        {
            case (char)NodeOption::m_NO_MOUSE_GRAB     : ret |= Node::OPTION_m_NO_MOUSE_GRAB     ; break;
            case (char)NodeOption::f_NO_SPARKS         : ret |= Node::OPTION_f_NO_SPARKS         ; break;
            case (char)NodeOption::x_EXHAUST_POINT     : ret |= Node::OPTION_x_EXHAUST_POINT     ; break;
            case (char)NodeOption::y_EXHAUST_DIRECTION : ret |= Node::OPTION_y_EXHAUST_DIRECTION ; break;
            case (char)NodeOption::c_NO_GROUND_CONTACT : ret |= Node::OPTION_c_NO_GROUND_CONTACT ; break;
            case (char)NodeOption::h_HOOK_POINT        : ret |= Node::OPTION_h_HOOK_POINT        ; break;
            case (char)NodeOption::e_TERRAIN_EDIT_POINT: ret |= Node::OPTION_e_TERRAIN_EDIT_POINT; break;
            case (char)NodeOption::b_EXTRA_BUOYANCY    : ret |= Node::OPTION_b_EXTRA_BUOYANCY    ; break;
            case (char)NodeOption::p_NO_PARTICLES      : ret |= Node::OPTION_p_NO_PARTICLES      ; break;
            case (char)NodeOption::L_LOG               : ret |= Node::OPTION_L_LOG               ; break;
            case (char)NodeOption::l_LOAD_WEIGHT       : ret |= Node::OPTION_l_LOAD_WEIGHT       ; break;

            case (char)NodeOption::n_DUMMY: break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid option '{}'", c));
        }
    }
    return ret;
}

SpecialProp Parser::IdentifySpecialProp(const std::string& str)
{
    if (str.find("leftmirror"  ) != std::string::npos)         { return SpecialProp::MIRROR_LEFT; }
    if (str.find("rightmirror" ) != std::string::npos)         { return SpecialProp::MIRROR_RIGHT; }
    if (str.find("dashboard-rh") != std::string::npos)         { return SpecialProp::DASHBOARD_RIGHT; }
    if (str.find("dashboard"   ) != std::string::npos)         { return SpecialProp::DASHBOARD_LEFT; }
    if (Ogre::StringUtil::startsWith(str, "spinprop", false) ) { return SpecialProp::AERO_PROP_SPIN; }
    if (Ogre::StringUtil::startsWith(str, "pale", false)     ) { return SpecialProp::AERO_PROP_BLADE; }
    if (Ogre::StringUtil::startsWith(str, "seat", false)     ) { return SpecialProp::DRIVER_SEAT; }
    if (Ogre::StringUtil::startsWith(str, "seat2", false)    ) { return SpecialProp::DRIVER_SEAT_2; }
    if (Ogre::StringUtil::startsWith(str, "beacon", false)   ) { return SpecialProp::BEACON; }
    if (Ogre::StringUtil::startsWith(str, "redbeacon", false)) { return SpecialProp::REDBEACON; }
    if (Ogre::StringUtil::startsWith(str, "lightb", false)   ) { return SpecialProp::LIGHTBAR; }
    return SpecialProp::NONE;
}

EngineType Parser::GetArgEngineType(int index)
{
    char c = this->GetArgChar(index);
    switch (c)
    {
        case (char)EngineType::t_TRUCK:
        case (char)EngineType::c_CAR:
        case (char)EngineType::e_ECAR:
            return EngineType(c);

        default:
            fmt::format("invalid EngineType '{}', falling back to 't' (truck)", c);
            return EngineType::t_TRUCK;
    }
}

ManagedMaterialType Parser::GetArgManagedMatType(int index)
{
    std::string str = this->GetArgStr(index);
    if (str == "mesh_standard")        return ManagedMaterialType::MESH_STANDARD;
    if (str == "mesh_transparent")     return ManagedMaterialType::MESH_TRANSPARENT;
    if (str == "flexmesh_standard")    return ManagedMaterialType::FLEXMESH_STANDARD;
    if (str == "flexmesh_transparent") return ManagedMaterialType::FLEXMESH_TRANSPARENT;

    this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
        fmt::format("Not a valid ManagedMaterialType: '{}'", str));
    return ManagedMaterialType::INVALID;
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
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_ERROR,
                fmt::format("Could not read truck file: {}", ex.getFullDescription()));
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

    // Skip empty/comment lines
    if ((raw_start == raw_end) || (*raw_start == ';') || (*raw_start == '/'))
    {
        ++m_current_line_number;
        return;
    }

    // Sanitize UTF-8
    memset(m_current_line, 0, LINE_BUFFER_LENGTH);
    char* out_start = m_current_line;
    utf8::replace_invalid(raw_start, raw_end, out_start, '?');

    // Process
    this->ProcessCurrentLine();
    ++m_current_line_number;
}

} // namespace RigDef
