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
#include <fmt/format.h>

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

void Parser::ProcessCurrentLine()
{
    // Ignore comment lines
    if ((m_current_line[0] == ';') || (m_current_line[0] == '/'))
    {
        return;
    }

    // First line in file (except blanks or comments) is the actor name
    if (m_document->name == "" && m_current_line != "")
    {
        m_document->name = m_current_line; // Already trimmed
        return;
    }

    // Split line to tokens
    if (m_current_block != Keyword::COMMENT &&
        m_current_block != Keyword::DESCRIPTION)
    {
        this->TokenizeCurrentLine();
    }

    // Detect keyword on current line 
    Keyword keyword = IdentifyKeywordInCurrentLine();
    m_log_keyword = keyword;
    switch (keyword)
    {
        // No keyword - Continue below to process current block.
        case Keyword::INVALID:
            break; // << NOT RETURN.

        // Directives without arguments: just record, do not change current block.
        case Keyword::DISABLEDEFAULTSOUNDS:
        case Keyword::ENABLE_ADVANCED_DEFORMATION:
        case Keyword::END_SECTION:
        case Keyword::FORWARDCOMMANDS:
        case Keyword::HIDEINCHOOSER:
        case Keyword::IMPORTCOMMANDS:
        case Keyword::LOCKGROUP_DEFAULT_NOLOCK:
        case Keyword::RESCUER:
        case Keyword::ROLLON:
        case Keyword::SLIDENODE_CONNECT_INSTANTLY:
        case Keyword::SUBMESH:
            m_document->lines.emplace_back(Line(keyword, DATAPOS_INVALID));
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
            this->EndBlock(keyword);
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

    // Parse current block, if any
    m_log_keyword = m_current_block;
    switch (m_current_block)
    {
        case Keyword::AIRBRAKES:            this->ParseAirbrakes();               return;
        case Keyword::ANIMATORS:            this->ParseAnimators();               return;
        case Keyword::AXLES:                this->ParseAxles();                   return;
        case Keyword::BEAMS:                this->ParseBeams();                   return;
        case Keyword::BRAKES:               this->ParseBrakes();                  return;
        case Keyword::CAMERAS:              this->ParseCameras();                 return;
        case Keyword::CAB:                  this->ParseCab();                     return;
        case Keyword::CAMERARAIL:           this->ParseCamerarails();             return;
        case Keyword::CINECAM:              this->ParseCinecam();                 return;
        case Keyword::COMMANDS:             this->ParseCommands();                return;
        case Keyword::COMMANDS2:            this->ParseCommands2();               return;
        case Keyword::COLLISIONBOXES:       this->ParseCollisionboxes();          return;
        case Keyword::CONTACTERS:           this->ParseContacters();              return;
        case Keyword::DESCRIPTION:          this->ParseDescription();             return;
        case Keyword::ENGINE:               this->ParseEngine();                  return;
        case Keyword::ENGOPTION:            this->ParseEngoption();               return;
        case Keyword::ENGTURBO:             this->ParseEngturbo();                return;
        case Keyword::EXHAUSTS:             this->ParseExhausts();                return;
        case Keyword::FIXES:                this->ParseFixes();                   return;
        case Keyword::FLARES:               this->ParseFlares();                  return;
        case Keyword::FLARES2:              this->ParseFlares2();                 return;
        case Keyword::FLARES3:              this->ParseFlares3();                 return;
        case Keyword::FLEXBODIES:           this->ParseFlexbodies();              return;
        case Keyword::FLEXBODYWHEELS:       this->ParseFlexbodywheels();          return;
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
        case Keyword::MESHWHEELS:           this->ParseMeshwheels();              return;
        case Keyword::MESHWHEELS2:          this->ParseMeshwheels2();             return;
        case Keyword::MINIMASS:             this->ParseMinimass();                return;
        case Keyword::NODES:                this->ParseNodes();                   return;
        case Keyword::NODES2:               this->ParseNodes2();                  return;
        case Keyword::PARTICLES:            this->ParseParticles();               return;
        case Keyword::PISTONPROPS:          this->ParsePistonprops();             return;
        case Keyword::PROPS:                this->ParseProps();                   return;
        case Keyword::RAILGROUPS:           this->ParseRailGroups();              return;
        case Keyword::ROPABLES:             this->ParseRopables();                return;
        case Keyword::ROPES:                this->ParseRopes();                   return;
        case Keyword::ROTATORS:             this->ParseRotators();                return;
        case Keyword::ROTATORS2:            this->ParseRotators2();               return;
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
        case Keyword::TURBOPROPS:           this->ParseTurboprops();              return;
        case Keyword::TURBOPROPS2:          this->ParseTurboprops2();             return;
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

    m_document->wings.push_back(wing);
    m_document->lines.emplace_back(Line(Keyword::WINGS, (int)m_document->wings.size() - 1));
}

void Parser::ParseSetCollisionRange()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg
    
    CollisionRange cr;
    cr.node_collision_range = this->GetArgFloat(1);
    
    m_document->set_collision_range.push_back(cr);
    m_document->lines.emplace_back(Line(Keyword::SET_COLLISION_RANGE, (int)m_document->set_collision_range.size() - 1));
}

void Parser::ParseWheel2()
{
    if (!this->CheckNumArguments(17)) { return; }

    Wheel2 wheel_2;

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

    m_document->wheels2.push_back(wheel_2);
    m_document->lines.emplace_back(Line(Keyword::WHEELS2, (int)m_document->wheels2.size() - 1));
}

void Parser::ParseWheel()
{
    if (! this->CheckNumArguments(14)) { return; }

    Wheel wheel;

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

    m_document->wheels.push_back(wheel);
    m_document->lines.emplace_back(Line(Keyword::WHEELS, (int)m_document->wheels.size() - 1));
}

void Parser::ParseWheelDetachers()
{
    if (! this->CheckNumArguments(2)) { return; }

    WheelDetacher wheeldetacher;

    wheeldetacher.wheel_id       = this->GetArgInt(0);
    wheeldetacher.detacher_group = this->GetArgInt(1);

    m_document->wheeldetachers.push_back(wheeldetacher);
    m_document->lines.emplace_back(Line(Keyword::WHEELDETACHERS, (int)m_document->wheeldetachers.size() - 1));
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
            this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "TractionControl Mode: missing");
            tc.attr_no_dashboard = false;
            tc.attr_no_toggle = false;
            tc.attr_is_on = true;
        }
    }

    m_document->tractioncontrol.push_back(tc);
    m_document->lines.emplace_back(Line(Keyword::TRACTIONCONTROL, (int)m_document->tractioncontrol.size() - 1));
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

    m_document->transfercase.push_back(tc);
    m_document->lines.emplace_back(Line(Keyword::TRANSFERCASE, (int)m_document->transfercase.size() - 1));
}

void Parser::ParseSubmeshGroundModel()
{
    if (!this->CheckNumArguments(2)) { return; } // Items: keyword, arg

    m_document->submesh_groundmodel.push_back(this->GetArgStr(1));
    m_document->lines.emplace_back(Line(Keyword::SUBMESH_GROUNDMODEL, (int)m_document->submesh_groundmodel.size() - 1));
}

void Parser::ParseSpeedLimiter()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    SpeedLimiter sl;
    sl.is_enabled = true;
    sl.max_speed = this->GetArgFloat(1);

    m_document->speedlimiter.push_back(sl);
    m_document->lines.emplace_back(Line(Keyword::SPEEDLIMITER, (int)m_document->speedlimiter.size() - 1));
}

void Parser::ParseSetSkeletonSettings()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    SkeletonSettings skel;
    skel.visibility_range_meters = this->GetArgFloat(1);
    if (m_num_args > 2) { skel.beam_thickness_meters = this->GetArgFloat(2); }
    
    // Defaults
    if (skel.visibility_range_meters < 0.f) { skel.visibility_range_meters = 150.f; }
    if (skel.beam_thickness_meters   < 0.f) { skel.beam_thickness_meters   = BEAM_SKELETON_DIAMETER; }

    m_document->set_skeleton_settings.push_back(skel);
    m_document->lines.emplace_back(Line(Keyword::SET_SKELETON_SETTINGS, (int)m_document->set_skeleton_settings.size() - 1));
}

void Parser::ParseDirectiveSetNodeDefaults()
{
    if (!this->CheckNumArguments(2)) { return; }

    NodeDefaults def;
    def._num_args = m_num_args;

                        def.load_weight = this->GetArgFloat(1);
    if (m_num_args > 2) def.friction   = this->GetArgFloat(2);
    if (m_num_args > 3) def.volume     = this->GetArgFloat(3);
    if (m_num_args > 4) def.surface    = this->GetArgFloat(4);
    if (m_num_args > 5) def.options    = this->GetArgNodeOptions(5);

    m_document->set_node_defaults.push_back(def);
    m_document->lines.emplace_back(Line(Keyword::SET_NODE_DEFAULTS, (int)m_document->set_node_defaults.size() - 1));
}

void Parser::ParseDirectiveSetManagedMaterialsOptions()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    ManagedMaterialsOptions mmo;

    // This is what v0.3x's parser did.
    char c = this->GetArgChar(1);
    mmo.double_sided = (c != '0');

    m_document->set_managedmaterials_options.push_back(mmo);
    m_document->lines.emplace_back(Line(Keyword::SET_MANAGEDMATERIALS_OPTIONS, (int)m_document->set_managedmaterials_options.size() - 1));
}

void Parser::ParseDirectiveSetBeamDefaultsScale()
{
    if (! this->CheckNumArguments(5)) { return; }

    BeamDefaultsScale scale;
    scale._num_args = m_num_args;

    scale.springiness = this->GetArgFloat(1);
    if (m_num_args > 2) { scale.damping_constant = this->GetArgFloat(2); }
    if (m_num_args > 3) { scale.deformation_threshold_constant = this->GetArgFloat(3); }
    if (m_num_args > 4) { scale.breaking_threshold_constant = this->GetArgFloat(4); }

    m_document->set_beam_defaults_scale.push_back(scale);
    m_document->lines.emplace_back(Line(Keyword::SET_BEAM_DEFAULTS_SCALE, (int)m_document->set_beam_defaults_scale.size() - 1));
}

void Parser::ParseDirectiveSetBeamDefaults()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    BeamDefaults d;
    d._num_args = m_num_args;

    d.springiness = this->GetArgFloat(1);
    if (m_num_args > 2) d.damping_constant = this->GetArgFloat(2);
    if (m_num_args > 3) d.deformation_threshold = this->GetArgFloat(3);
    if (m_num_args > 4) d.breaking_threshold = this->GetArgFloat(4);
    if (m_num_args > 5) d.visual_beam_diameter = this->GetArgFloat(5);
    if (m_num_args > 6) d.beam_material_name = this->GetArgStr(6);
    if (m_num_args > 7) d.plastic_deform_coef = this->GetArgFloat(7);

    m_document->set_beam_defaults.push_back(d);
    m_document->lines.emplace_back(Line(Keyword::SET_BEAM_DEFAULTS, (int)m_document->set_beam_defaults.size() - 1));
}

void Parser::ParseDirectivePropCameraMode()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    PropCameraMode def;
    def.mode = this->GetArgInt(1);

    m_document->prop_camera_mode.push_back(def);
    m_document->lines.emplace_back(Line(Keyword::PROP_CAMERA_MODE, (int)m_document->prop_camera_mode.size() - 1));
}

void Parser::ParseDirectiveSection()
{
    // syntax: "section version configname1, [configname2]..."
    Section section;
    // version is unused
    for (int i = 2; i < m_num_args; ++i)
    {
        section.configs.push_back(this->GetArgStr(i));
    }

    // Backwards compatibility: automatically add 'sectionconfig' where missing.
    for (std::string& new_config : section.configs)
    {
        bool exists = false;
        for (RigDef::SectionConfig& sc : m_document->sectionconfig)
        {
            exists = exists || (sc.name == new_config);
        }
        if (!exists)
        {
            SectionConfig new_sc;
            new_sc.name = new_config;
            m_document->sectionconfig.push_back(new_sc);
        }
    }

    m_document->section.push_back(section);
    m_document->lines.emplace_back(Line(Keyword::SECTION, (int)m_document->section.size() - 1));
}

void Parser::ParseDirectiveSectionConfig()
{
    // syntax: "sectionconfig version configname"
    SectionConfig sc;
    // version is unused
    sc.name = this->GetArgStr(2);

    m_document->sectionconfig.push_back(sc);
    m_document->lines.emplace_back(Line(Keyword::SECTIONCONFIG, (int)m_document->sectionconfig.size() - 1));
}

void Parser::ParseDirectiveBackmesh()
{
    m_document->lines.emplace_back(Line(Keyword::BACKMESH, -1));
}

void Parser::ParseMeshwheels()
{
    if (! this->CheckNumArguments(16)) { return; }

    MeshWheel mesh_wheel;

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

    m_document->meshwheels.push_back(mesh_wheel);
    m_document->lines.emplace_back(Line(Keyword::MESHWHEELS, (int)m_document->meshwheels.size() - 1));
}

void Parser::ParseMeshwheels2()
{
    if (! this->CheckNumArguments(16)) { return; }

    MeshWheel2 mesh_wheel;

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

    m_document->meshwheels2.push_back(mesh_wheel);
    m_document->lines.emplace_back(Line(Keyword::MESHWHEELS2, (int)m_document->meshwheels2.size() - 1));
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

    m_document->hooks.push_back(hook);
    m_document->lines.emplace_back(Line(Keyword::HOOKS, (int)m_document->hooks.size() - 1));
}

void Parser::ParseHelp()
{
    Help h;
    h.material = m_current_line; // already trimmed

    m_document->help.push_back(h);
    m_document->lines.emplace_back(Line(Keyword::HELP, (int)m_document->help.size() - 1));
}

void Parser::ParseGuiSettings()
{
    if (! this->CheckNumArguments(2)) { return; }
   
    GuiSettings gs;
    gs.key = this->GetArgStr(0);
    gs.value = this->GetArgStr(1);

    m_document->guisettings.push_back(gs);
    m_document->lines.emplace_back(Line(Keyword::GUISETTINGS, (int)m_document->guisettings.size() - 1));
}

void Parser::ParseGuid()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    Guid g;
    g.guid = this->GetArgStr(1);

    m_document->guid.push_back(g);
    m_document->lines.emplace_back(Line(Keyword::GUID, (int)m_document->guid.size() - 1));
}

void Parser::ParseGlobals()
{
    if (! this->CheckNumArguments(2)) { return; }

    Globals globals;
    globals.dry_mass   = this->GetArgFloat(0);
    globals.cargo_mass = this->GetArgFloat(1);

    if (m_num_args > 2) { globals.material_name = this->GetArgStr(2); }

    m_document->globals.push_back(globals);
    m_document->lines.emplace_back(Line(Keyword::GLOBALS, (int)m_document->globals.size() - 1));
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

    m_document->fusedrag.push_back(fusedrag);
    m_document->lines.emplace_back(Line(Keyword::FUSEDRAG, (int)m_document->fusedrag.size() - 1));
}

void Parser::ParseDirectiveFlexbodyCameraMode()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    FlexbodyCameraMode line;
    line.mode = this->GetArgInt(1);

    m_document->flexbody_camera_mode.push_back(line);
    m_document->lines.emplace_back(Line(Keyword::FLEXBODY_CAMERA_MODE, (int)m_document->flexbody_camera_mode.size() - 1));
}

void Parser::ParseCab()
{
    if (! this->CheckNumArguments(3)) { return; }

    Cab cab;
    cab.nodes[0] = this->GetArgNodeRef(0);
    cab.nodes[1] = this->GetArgNodeRef(1);
    cab.nodes[2] = this->GetArgNodeRef(2);
    if (m_num_args > 3) cab.options = this->GetArgCabOptions(3);

    m_document->cab.push_back(cab);
    m_document->lines.emplace_back(Line(Keyword::CAB, (int)m_document->cab.size() - 1));
}

void Parser::ParseTexcoords()
{
    if (! this->CheckNumArguments(3)) { return; }

    Texcoord texcoord;
    texcoord.node = this->GetArgNodeRef(0);
    texcoord.u    = this->GetArgFloat  (1);
    texcoord.v    = this->GetArgFloat  (2);

    m_document->texcoords.push_back(texcoord);
    m_document->lines.emplace_back(Line(Keyword::TEXCOORDS, (int)m_document->texcoords.size() - 1));
}

void Parser::ParseFlexbodies()
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

    m_document->flexbodies.push_back(flexbody);
    m_document->lines.emplace_back(Line(Keyword::FLEXBODIES, (int)m_document->flexbodies.size() - 1));
}

void Parser::ParseDirectiveForset()
{
    Forset def;

    // --------------------------------------------------------------------------------------------
    // BEWARE OF QUIRKS in the following code (they must be preserved for backwards compatibility):
    // - a space between the 'forset' keyword and arguments is optional.
    // - a separator at the end of line will silently add '0' to the node list.
    // --------------------------------------------------------------------------------------------

    //parsing set definition
    char* pos = m_current_line + 6; // 'forset' = 6 characters
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
            def.node_set.push_back(NodeInterval(val1, val2));
        }
        else
        {
            def.node_set.push_back(NodeInterval(val1, val1));
        }
        pos = end + 1;
    }

    m_document->forset.push_back(def);
    m_document->lines.emplace_back(Line(Keyword::FORSET, (int)m_document->forset.size() - 1));

}

void Parser::ParseFlares()
{
    if (! this->CheckNumArguments(5)) { return; }

    Flare2 flare;
    flare.reference_node = this->GetArgNodeRef(0);
    flare.node_axis_x    = this->GetArgNodeRef(1);
    flare.node_axis_y    = this->GetArgNodeRef(2);
    flare.offset.x       = this->GetArgFloat  (3);
    flare.offset.y       = this->GetArgFloat  (4);

    if (m_num_args > 5) { flare.type = this->GetArgFlareType(5); }

    if (m_num_args > 6)
    {
        switch (flare.type)
        {
            case FlareType::USER:      flare.control_number = this->GetArgInt(6); break;
            case FlareType::DASHBOARD: flare.dashboard_link = this->GetArgStr(6); break;
            default: break;
        }
    }

    if (m_num_args > 7) { flare.blink_delay_milis = this->GetArgInt      (7); }
    if (m_num_args > 8) { flare.size              = this->GetArgFloat    (8); }
    if (m_num_args > 9) { flare.material_name     = this->GetArgStr      (9); }

    m_document->flares.push_back(flare);
    m_document->lines.emplace_back(Line(Keyword::FLARES, (int)m_document->flares.size() - 1));
}

void Parser::ParseFlares2()
{
    if (! this->CheckNumArguments(6)) { return; }

    Flare2 flare2;
    flare2.reference_node = this->GetArgNodeRef(0);
    flare2.node_axis_x    = this->GetArgNodeRef(1);
    flare2.node_axis_y    = this->GetArgNodeRef(2);
    flare2.offset.x       = this->GetArgFloat  (3);
    flare2.offset.y       = this->GetArgFloat  (4);
    flare2.offset.z       = this->GetArgFloat  (5); //<< only difference from 'flares'

    if (m_num_args > 6) { flare2.type = this->GetArgFlareType(6); }

    if (m_num_args > 7)
    {
        switch (flare2.type)
        {
            case FlareType::USER:      flare2.control_number = this->GetArgInt(7); break;
            case FlareType::DASHBOARD: flare2.dashboard_link = this->GetArgStr(7); break;
            default: break;
        }
    }

    if (m_num_args > 8)  { flare2.blink_delay_milis = this->GetArgInt      (8); }
    if (m_num_args > 9)  { flare2.size              = this->GetArgFloat    (9); }
    if (m_num_args > 10) { flare2.material_name     = this->GetArgStr      (10); }

    m_document->flares2.push_back(flare2);
    m_document->lines.emplace_back(Line(Keyword::FLARES2, (int)m_document->flares2.size() - 1));
}

void Parser::ParseFlares3()
{
    if (! this->CheckNumArguments(6)) { return; }

    Flare3 flare3;
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

    m_document->flares3.push_back(flare3);
    m_document->lines.emplace_back(Line(Keyword::FLARES3, (int)m_document->flares3.size() - 1));
}

void Parser::ParseFixes()
{
    m_document->fixes.push_back(this->GetArgNodeRef(0));
    m_document->lines.emplace_back(Line(Keyword::FIXES, (int)m_document->fixes.size() - 1));
}

void Parser::ParseExtCamera()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    ExtCamera extcam;
    extcam.mode = this->GetArgExtCameraMode(1);
    if (m_num_args > 2) { extcam.node = this->GetArgNodeRef(2); }
    
    m_document->extcamera.push_back(extcam);
    m_document->lines.emplace_back(Line(Keyword::EXTCAMERA, (int)m_document->extcamera.size() - 1));
}

void Parser::ParseExhausts()
{
    if (! this->CheckNumArguments(2)) { return; }

    Exhaust exhaust;
    exhaust.reference_node = this->GetArgNodeRef(0);
    exhaust.direction_node = this->GetArgNodeRef(1);
    
    // Param [2] is unused
    if (m_num_args > 3) { exhaust.particle_name = this->GetArgStr(3); }

    m_document->exhausts.push_back(exhaust);
    m_document->lines.emplace_back(Line(Keyword::EXHAUSTS, (int)m_document->exhausts.size() - 1));
}

void Parser::ParseFileFormatVersion()
{
    if (! this->CheckNumArguments(2)) { return; }

    FileFormatVersion ffv;
    ffv.version = this->GetArgInt(1);

    m_document->fileformatversion.push_back(ffv);
    m_document->lines.emplace_back(Line(Keyword::FILEFORMATVERSION, (int)m_document->fileformatversion.size() - 1));
    m_current_block = Keyword::INVALID;
}

void Parser::ParseDirectiveDefaultSkin()
{
    if (!this->CheckNumArguments(2)) { return; } // 2 items: keyword, param

    DefaultSkin data;
    data.skin_name = this->GetArgStr(1);
    std::replace(data.skin_name.begin(), data.skin_name.end(), '_', ' ');

    m_document->default_skin.push_back(data);\
    m_document->lines.emplace_back(Line(Keyword::DEFAULT_SKIN, (int)m_document->default_skin.size() - 1));
}

void Parser::ParseDirectiveDetacherGroup()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, param

    DetacherGroup dg;
    if (this->GetArgStr(1) == "end")
    {
        dg.end = true;
    }
    else
    {
        dg.number = this->GetArgInt(1);
    }

    m_document->detacher_group.push_back(dg);
    m_document->lines.emplace_back(Line(Keyword::DETACHER_GROUP, (int)m_document->detacher_group.size() - 1));
}

void Parser::ParseCruiseControl()
{
    if (! this->CheckNumArguments(3)) { return; } // keyword + 2 params

    CruiseControl cruise_control;
    cruise_control.min_speed = this->GetArgFloat(1);
    cruise_control.autobrake = this->GetArgInt(2);

    m_document->cruisecontrol.push_back(cruise_control);
    m_document->lines.emplace_back(Line(Keyword::CRUISECONTROL, (int)m_document->cruisecontrol.size() - 1));
}

void Parser::ParseDescription()
{
    m_document->description.push_back(m_current_line); // Already trimmed
    m_document->lines.emplace_back(Line(Keyword::DESCRIPTION, (int)m_document->description.size() - 1));
}

void Parser::ParseDirectiveAddAnimation()
{
    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 14, ","); // "add_animation " = 14 characters
    m_num_args = (int)tokens.size();
    if (! this->CheckNumArguments(4)) { return; }

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
                animation.event_name = entry[1];
                Ogre::StringUtil::trim(animation.event_name);
                Ogre::StringUtil::toUpperCase(animation.event_name);
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
            this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                fmt::format("Ignoring invalid token '{}' ({})", itor->c_str(), warn_msg));
        }
    }

    m_document->add_animation.push_back(animation);
    m_document->lines.emplace_back(Line(Keyword::ADD_ANIMATION, (int)m_document->add_animation.size() - 1));
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
            this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "Antilockbrakes Mode: missing");
            alb.attr_no_dashboard = false;
            alb.attr_no_toggle = false;
            alb.attr_is_on = true;
        }
    }

    m_document->antilockbrakes.push_back(alb);
    m_document->lines.emplace_back(Line(Keyword::ANTILOCKBRAKES, (int)m_document->antilockbrakes.size() - 1));
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

    m_document->engoption.push_back(engoption);
    m_document->lines.emplace_back(Line(Keyword::ENGOPTION, (int)m_document->engoption.size() - 1));
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

    m_document->engturbo.push_back(engturbo);
    m_document->lines.emplace_back(Line(Keyword::ENGTURBO, (int)m_document->engturbo.size() - 1));
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
        LogMessage(Console::CONSOLE_SYSTEM_ERROR, "Engine has no forward gear, ignoring...");
        return;
    }

    m_document->engine.push_back(engine);
    m_document->lines.emplace_back(Line(Keyword::ENGINE, (int)m_document->engine.size() - 1));
}

void Parser::ParseContacters()
{
    if (! this->CheckNumArguments(1)) { return; }

    m_document->contacters.push_back(this->GetArgNodeRef(0));
    m_document->lines.emplace_back(Line(Keyword::CONTACTERS, (int)m_document->contacters.size() - 1));
}

void Parser::ParseCommands()
{
    if (! this->CheckNumArguments(7)) { return; }

    Command command;

    command.nodes[0]          = this->GetArgNodeRef(0);
    command.nodes[1]          = this->GetArgNodeRef(1);
    command.rate              = this->GetArgFloat  (2);
    command.max_contraction = this->GetArgFloat(3);
    command.max_extension   = this->GetArgFloat(4);
    command.contract_key    = this->GetArgInt  (5);
    command.extend_key      = this->GetArgInt  (6);

    if (m_num_args > 7) { this->ParseCommandOptions(command, this->GetArgStr(7)); }
    if (m_num_args > 8) { command.description   = this->GetArgStr  (8);}

    if (m_num_args > 9) { this->ParseOptionalInertia(command.inertia, 9); } // 4 args

    if (m_num_args > 13) { command.affect_engine = this->GetArgFloat(13);}
    if (m_num_args > 14) { command.needs_engine  = this->GetArgBool (14);}
    if (m_num_args > 15) { command.plays_sound   = this->GetArgBool (15);}

    m_document->commands.push_back(command);
    m_document->lines.emplace_back(Line(Keyword::COMMANDS, (int)m_document->commands.size() - 1));
}

void Parser::ParseCommands2()
{
    if (! this->CheckNumArguments(8)) { return; }

    Command2 command;

    command.nodes[0]          = this->GetArgNodeRef(0);
    command.nodes[1]          = this->GetArgNodeRef(1);
    command.shorten_rate      = this->GetArgFloat  (2); // <- different from 'commands'
    command.lengthen_rate     = this->GetArgFloat  (3); // <- different from 'commands'
    command.max_contraction = this->GetArgFloat(4);
    command.max_extension   = this->GetArgFloat(5);
    command.contract_key    = this->GetArgInt  (6);
    command.extend_key      = this->GetArgInt  (7);

    if (m_num_args > 8) { this->ParseCommandOptions(command, this->GetArgStr(8)); }
    if (m_num_args > 9) { command.description   = this->GetArgStr  (9);}

    if (m_num_args > 10) { this->ParseOptionalInertia(command.inertia, 10); } // 4 args

    if (m_num_args > 14) { command.affect_engine = this->GetArgFloat(14);}
    if (m_num_args > 15) { command.needs_engine  = this->GetArgBool (15);}
    if (m_num_args > 16) { command.plays_sound   = this->GetArgBool (16);}

    m_document->commands2.push_back(command);
    m_document->lines.emplace_back(Line(Keyword::COMMANDS2, (int)m_document->commands2.size() - 1));
}

void Parser::ParseCommandOptions(CommandCommon& command2, std::string const& options_str)
{
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
}

void Parser::ParseCollisionboxes()
{
    CollisionBox collisionbox;

    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line, ",");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        collisionbox.nodes.push_back( this->_ParseNodeRef(*iter) );
    }

    m_document->collisionboxes.push_back(collisionbox);
    m_document->lines.emplace_back(Line(Keyword::COLLISIONBOXES, (int)m_document->collisionboxes.size() - 1));
}

void Parser::ParseCinecam()
{
    if (! this->CheckNumArguments(11)) { return; }

    Cinecam cinecam;

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

    m_document->cinecam.push_back(cinecam);
    m_document->lines.emplace_back(Line(Keyword::CINECAM, (int)m_document->cinecam.size() - 1));
}

void Parser::ParseCamerarails()
{
    m_document->camerarails.push_back( this->GetArgNodeRef(0) );
    m_document->lines.emplace_back(Line(Keyword::CAMERARAIL, (int)m_document->camerarails.size() - 1));
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

    m_document->brakes.push_back(brakes);
    m_document->lines.emplace_back(Line(Keyword::BRAKES, (int)m_document->brakes.size() - 1));
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

    m_document->axles.push_back(axle);
    m_document->lines.emplace_back(Line(Keyword::AXLES, (int)m_document->axles.size() - 1));
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

    m_document->interaxles.push_back(interaxle);
    m_document->lines.emplace_back(Line(Keyword::INTERAXLES, (int)m_document->interaxles.size() - 1));
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

    m_document->airbrakes.push_back(airbrake);
    m_document->lines.emplace_back(Line(Keyword::AIRBRAKES, (int)m_document->airbrakes.size() - 1));
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

    m_document->videocameras.push_back(videocamera);
    m_document->lines.emplace_back(Line(Keyword::VIDEOCAMERA, (int)m_document->videocameras.size() - 1));
}

void Parser::ParseCameras()
{
    if (! this->CheckNumArguments(3)) { return; }

    Camera camera;
    camera.center_node = this->GetArgNodeRef(0);
    camera.back_node   = this->GetArgNodeRef(1);
    camera.left_node   = this->GetArgNodeRef(2);

    m_document->cameras.push_back(camera);
    m_document->lines.emplace_back(Line(Keyword::CAMERAS, (int)m_document->cameras.size() - 1));
}

void Parser::ParseTurboprops()
{
    if (! this->CheckNumArguments(8)) { return; }

    Turboprop2 turboprop;

    turboprop.reference_node     = this->GetArgNodeRef(0);
    turboprop.axis_node          = this->GetArgNodeRef(1);
    turboprop.blade_tip_nodes[0] = this->GetArgNodeRef(2);
    turboprop.blade_tip_nodes[1] = this->GetArgNodeRef(3);
    turboprop.blade_tip_nodes[2] = this->GetArgNullableNode(4);
    turboprop.blade_tip_nodes[3] = this->GetArgNullableNode(5);
    turboprop.turbine_power_kW   = this->GetArgFloat  (6);
    turboprop.airfoil            = this->GetArgStr    (7);
    
    m_document->turboprops.push_back(turboprop);
    m_document->lines.emplace_back(Line(Keyword::TURBOPROPS, (int)m_document->turboprops.size() - 1));
}

void Parser::ParseTurboprops2()
{
    if (! this->CheckNumArguments(9)) { return; }

    Turboprop2 turboprop;
    
    turboprop.reference_node     = this->GetArgNodeRef(0);
    turboprop.axis_node          = this->GetArgNodeRef(1);
    turboprop.blade_tip_nodes[0] = this->GetArgNodeRef(2);
    turboprop.blade_tip_nodes[1] = this->GetArgNodeRef(3);
    turboprop.blade_tip_nodes[2] = this->GetArgNullableNode(4);
    turboprop.blade_tip_nodes[3] = this->GetArgNullableNode(5);
    turboprop.couple_node        = this->GetArgNullableNode(6);
    turboprop.turbine_power_kW   = this->GetArgFloat  (7);
    turboprop.airfoil            = this->GetArgStr    (8);
    
    m_document->turboprops2.push_back(turboprop);
    m_document->lines.emplace_back(Line(Keyword::TURBOPROPS2, (int)m_document->turboprops2.size() - 1));
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

    m_document->turbojets.push_back(turbojet);
    m_document->lines.emplace_back(Line(Keyword::TURBOJETS, (int)m_document->turbojets.size() - 1));
}

void Parser::ParseTriggers()
{
    if (! this->CheckNumArguments(6)) { return; }

    Trigger trigger;
    trigger.nodes[0]                  = this->GetArgNodeRef(0);
    trigger.nodes[1]                  = this->GetArgNodeRef(1);
    trigger.contraction_trigger_limit = this->GetArgFloat  (2);
    trigger.expansion_trigger_limit   = this->GetArgFloat  (3);
    
    int shortbound_trigger_action = this->GetArgInt(4); 
    int longbound_trigger_action  = this->GetArgInt(5); 
    if (m_num_args > 6) trigger.options = this->GetArgTriggerOptions(6);

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
    else if (BITMASK_IS_1(trigger.options, RigDef::Trigger::OPTION_E_ENGINE_TRIGGER))
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

    m_document->triggers.push_back(trigger);
    m_document->lines.emplace_back(Line(Keyword::TRIGGERS, (int)m_document->triggers.size() - 1));
}

void Parser::ParseTorqueCurve()
{
    TorqueCurve torque_curve;

    Ogre::StringVector args = Ogre::StringUtil::split(m_current_line, ",");
    bool valid = true;
    if (args.size() == 1u)
    {
        torque_curve.predefined_func_name = args[0];
    }
    else if (args.size() == 2u)
    {
        torque_curve.sample.power          = this->ParseArgFloat(args[0].c_str());
        torque_curve.sample.torque_percent = this->ParseArgFloat(args[1].c_str());
    }
    else
    {
        // Consistent with 0.38's parser.
        this->LogMessage(Console::CONSOLE_SYSTEM_ERROR, "Invalid line, too many arguments");
        valid = false;
    }

    if (valid)
    {
        m_document->torquecurve.push_back(torque_curve);
        m_document->lines.emplace_back(Line(Keyword::TORQUECURVE, (int)m_document->torquecurve.size() - 1));
    }
}

void Parser::ParseTies()
{
    if (! this->CheckNumArguments(5)) { return; }

    Tie tie;

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

    m_document->ties.push_back(tie);
    m_document->lines.emplace_back(Line(Keyword::TIES, (int)m_document->ties.size() - 1));
}

void Parser::ParseSoundsources()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    SoundSource soundsource;
    soundsource.node              = this->GetArgNodeRef(0);
    soundsource.sound_script_name = this->GetArgStr(1);

    m_document->soundsources.push_back(soundsource);
    m_document->lines.emplace_back(Line(Keyword::SOUNDSOURCES, (int)m_document->soundsources.size() - 1));
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

    m_document->soundsources2.push_back(soundsource2);
    m_document->lines.emplace_back(Line(Keyword::SOUNDSOURCES2, (int)m_document->soundsources2.size() - 1));
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
    
    m_document->slidenodes.push_back(slidenode);
    m_document->lines.emplace_back(Line(Keyword::SLIDENODES, (int)m_document->slidenodes.size() - 1));
}

void Parser::ParseShock3()
{
    if (! this->CheckNumArguments(15)) { return; }

    Shock3 shock_3;

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

    m_document->shocks3.push_back(shock_3);
    m_document->lines.emplace_back(Line(Keyword::SHOCKS3, (int)m_document->shocks3.size() - 1));
}

void Parser::ParseShock2()
{
    if (! this->CheckNumArguments(13)) { return; }

    Shock2 shock_2;

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

    m_document->shocks2.push_back(shock_2);
    m_document->lines.emplace_back(Line(Keyword::SHOCKS2, (int)m_document->shocks2.size() - 1));
}

void Parser::ParseShock()
{
    if (! this->CheckNumArguments(7)) { return; }

    Shock shock;

    shock.nodes[0]       = this->GetArgNodeRef(0);
    shock.nodes[1]       = this->GetArgNodeRef(1);
    shock.spring_rate    = this->GetArgFloat  (2);
    shock.damping        = this->GetArgFloat  (3);
    shock.short_bound    = this->GetArgFloat  (4);
    shock.long_bound     = this->GetArgFloat  (5);
    shock.precompression = this->GetArgFloat  (6);
    if (m_num_args > 7) shock.options = this->GetArgShockOptions(7);

    m_document->shocks.push_back(shock);
    m_document->lines.emplace_back(Line(Keyword::SHOCKS, (int)m_document->shocks.size() - 1));
}

NodeRef_t Parser::_ParseNodeRef(std::string const & node_id_str)
{
    return NodeRef_t(node_id_str);
}

void Parser::ParseDirectiveSetDefaultMinimass()
{
    if (! this->CheckNumArguments(2)) { return; } // Directive name + parameter

    DefaultMinimass dm;
    dm.min_mass_Kg = this->GetArgFloat(1);

    m_document->set_default_minimass.push_back(dm);
    m_document->lines.emplace_back(Line(Keyword::SET_DEFAULT_MINIMASS, (int)m_document->set_default_minimass.size() - 1));
}

void Parser::ParseDirectiveSetInertiaDefaults()
{
    if (! this->CheckNumArguments(2)) { return; }

    InertiaDefaults inertia;
    inertia._num_args = m_num_args;
    inertia.start_delay_factor = this->GetArgFloat(1);
    if (m_num_args > 2) { inertia.stop_delay_factor = this->GetArgFloat(2); }
    if (m_num_args > 3) { inertia.start_function = this->GetArgStr(3); }
    if (m_num_args > 4) { inertia.stop_function  = this->GetArgStr(4); }
    
    m_document->set_inertia_defaults.push_back(inertia);
    m_document->lines.emplace_back(Line(Keyword::SET_INERTIA_DEFAULTS, (int)m_document->set_inertia_defaults.size() - 1));
}

void Parser::ParseScrewprops()
{
    if (! this->CheckNumArguments(4)) { return; }
    
    Screwprop screwprop;

    screwprop.prop_node = this->GetArgNodeRef(0);
    screwprop.back_node = this->GetArgNodeRef(1);
    screwprop.top_node  = this->GetArgNodeRef(2);
    screwprop.power     = this->GetArgFloat  (3);

    m_document->screwprops.push_back(screwprop);
    m_document->lines.emplace_back(Line(Keyword::SCREWPROPS, (int)m_document->screwprops.size() - 1));
}

void Parser::ParseScripts()
{
    if (!this->CheckNumArguments(1)) { return; }

    Script script;

    script.filename = this->GetArgStr(0);

    m_document->scripts.push_back(script);
    m_document->lines.emplace_back(Line(Keyword::SCRIPTS, (int)m_document->scripts.size() - 1));
}

void Parser::ParseRotators()
{
    if (! this->CheckNumArguments(13)) { return; }

    Rotator rotator;
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

    this->ParseOptionalInertia(rotator.inertia, 13);
    if (m_num_args > 17) { rotator.engine_coupling = this->GetArgFloat(17); }
    if (m_num_args > 18) { rotator.needs_engine    = this->GetArgBool (18); }

    m_document->rotators.push_back(rotator);
    m_document->lines.emplace_back(Line(Keyword::ROTATORS, (int)m_document->rotators.size() - 1));
}

void Parser::ParseRotators2()
{
    if (! this->CheckNumArguments(13)) { return; }

    Rotator2 rotator;
    
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

    // Extra args for rotators2
    if (m_num_args > 13) { rotator.rotating_force  = this->GetArgFloat(13); }
    if (m_num_args > 14) { rotator.tolerance       = this->GetArgFloat(14); }
    if (m_num_args > 15) { rotator.description     = this->GetArgStr  (15); }

    this->ParseOptionalInertia(rotator.inertia, 16); // 4 args
    if (m_num_args > 20) { rotator.engine_coupling = this->GetArgFloat(20); }
    if (m_num_args > 21) { rotator.needs_engine    = this->GetArgBool (21); }

    m_document->rotators2.push_back(rotator);
    m_document->lines.emplace_back(Line(Keyword::ROTATORS2, (int)m_document->rotators2.size() - 1));

}

void Parser::ParseFileinfo()
{
    if (! this->CheckNumArguments(2)) { return; }

    Fileinfo fileinfo;

    fileinfo.unique_id = this->GetArgStr(1);
    Ogre::StringUtil::trim(fileinfo.unique_id);

    if (m_num_args > 2) { fileinfo.category_id  = this->GetArgInt(2); }
    if (m_num_args > 3) { fileinfo.file_version = this->GetArgInt(3); }

    m_current_block = Keyword::INVALID;
    m_document->fileinfo.push_back(fileinfo);
    m_document->lines.emplace_back(Line(Keyword::FILEINFO, (int)m_document->fileinfo.size() - 1));
}

void Parser::ParseRopes()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    Rope rope;
    rope.root_node      = this->GetArgNodeRef(0);
    rope.end_node       = this->GetArgNodeRef(1);
    
    if (m_num_args > 2) { rope.invisible  = (this->GetArgChar(2) == 'i'); }

    m_document->ropes.push_back(rope);
    m_document->lines.emplace_back(Line(Keyword::ROPES, (int)m_document->ropes.size() - 1));
}

void Parser::ParseRopables()
{
    if (! this->CheckNumArguments(1)) { return; }

    Ropable ropable;
    ropable.node = this->GetArgNodeRef(0);
    
    if (m_num_args > 1) { ropable.group         =  this->GetArgInt(1); }
    if (m_num_args > 2) { ropable.has_multilock = (this->GetArgInt(2) == 1); }

    m_document->ropables.push_back(ropable);
    m_document->lines.emplace_back(Line(Keyword::ROPABLES, (int)m_document->ropables.size() - 1));
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

    m_document->railgroups.push_back(railgroup);
    m_document->lines.emplace_back(Line(Keyword::RAILGROUPS, (int)m_document->railgroups.size() - 1));
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
    // Attention - arg 9 evaluated twice!
    prop.mesh_name      = this->GetArgStr    (9);
    prop.special        = this->GetArgSpecialProp(9);

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

    m_document->props.push_back(prop);
    m_document->lines.emplace_back(Line(Keyword::PROPS, (int)m_document->props.size() - 1));
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

    m_document->pistonprops.push_back(pistonprop);
    m_document->lines.emplace_back(Line(Keyword::PISTONPROPS, (int)m_document->pistonprops.size() - 1));

}

void Parser::ParseParticles()
{
    if (!this->CheckNumArguments(3)) { return; }

    Particle particle;
    particle.emitter_node         = this->GetArgNodeRef(0);
    particle.reference_node       = this->GetArgNodeRef(1);
    particle.particle_system_name = this->GetArgStr    (2);

    m_document->particles.push_back(particle);
    m_document->lines.emplace_back(Line(Keyword::PARTICLES, (int)m_document->particles.size() - 1));
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

void Parser::ParseNodes()
{
    if (! this->CheckNumArguments(4)) { return; }

    Node node;
    node._num_args = m_num_args;
    node.num = NodeNum_t(this->GetArgInt(0));
    node.position.x = this->GetArgFloat(1);
    node.position.y = this->GetArgFloat(2);
    node.position.z = this->GetArgFloat(3);
    if (m_num_args > 4)
    {
        node.options = this->GetArgNodeOptions(4);
    }
    if (m_num_args > 5)
    {
        // Only used on spawn if 'l' flag is present
        node.loadweight_override = this->GetArgFloat(5);
    }

    m_document->nodes.push_back(node);
    m_document->lines.emplace_back(Line(Keyword::NODES, (int)m_document->nodes.size() - 1));
}

void Parser::ParseNodes2()
{
    if (! this->CheckNumArguments(4)) { return; }

    Node2 node;
    node._num_args = m_num_args;
    node.name = this->GetArgStr(0);
    node.position.x = this->GetArgFloat(1);
    node.position.y = this->GetArgFloat(2);
    node.position.z = this->GetArgFloat(3);
    node.options = this->GetArgNodeOptions(4);
    if (m_num_args > 5)
    {
        // Only used on spawn if 'l' flag is present
        node.loadweight_override = this->GetArgFloat(5);
    }

    m_document->nodes2.push_back(node);
    m_document->lines.emplace_back(Line(Keyword::NODES2, (int)m_document->nodes2.size() - 1));
}

void Parser::ParseMinimass()
{
    if (! this->CheckNumArguments(1)) { return; }

    Minimass mm;
    mm.global_min_mass_Kg = this->GetArgFloat(0);
    if (m_num_args > 1) { mm.option = this->GetArgMinimassOption(1); }

    m_current_block = Keyword::INVALID;

    m_document->minimass.push_back(mm);
    m_document->lines.emplace_back(Line(Keyword::MINIMASS, (int)m_document->minimass.size() - 1));
}

void Parser::ParseFlexbodywheels()
{
    if (! this->CheckNumArguments(16)) { return; }

    FlexBodyWheel flexbody_wheel;

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

    m_document->flexbodywheels.push_back(flexbody_wheel);
    m_document->lines.emplace_back(Line(Keyword::FLEXBODYWHEELS, (int)m_document->flexbodywheels.size() - 1));
}

void Parser::ParseMaterialFlareBindings()
{
    if (! this->CheckNumArguments(2)) { return; }

    MaterialFlareBinding binding;
    binding.flare_number  = this->GetArgInt(0);
    binding.material_name = this->GetArgStr(1);
    
    m_document->materialflarebindings.push_back(binding);
    m_document->lines.emplace_back(Line(Keyword::MATERIALFLAREBINDINGS, (int)m_document->materialflarebindings.size() - 1));
}

void Parser::ParseManagedMaterials()
{
    if (! this->CheckNumArguments(2)) { return; }

    ManagedMaterial managed_mat;
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
    }

    m_document->managedmaterials.push_back(managed_mat);
    m_document->lines.emplace_back(Line(Keyword::MANAGEDMATERIALS, (int)m_document->managedmaterials.size() - 1));
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
    
    m_document->lockgroups.push_back(lockgroup);
    m_document->lines.emplace_back(Line(Keyword::LOCKGROUPS, (int)m_document->lockgroups.size() - 1));
}

void Parser::ParseHydros()
{
    if (! this->CheckNumArguments(3)) { return; }

    Hydro hydro;
    
    hydro.nodes[0]           = this->GetArgNodeRef(0);
    hydro.nodes[1]           = this->GetArgNodeRef(1);
    hydro.lenghtening_factor = this->GetArgFloat  (2);
    
    if (m_num_args > 3) { hydro.options = this->GetArgHydroOptions(3); }

    if (!hydro.options)
    {
        hydro.options |= Hydro::OPTION_n_INPUT_NORMAL;
    }
    
    this->ParseOptionalInertia(hydro.inertia, 4);

    m_document->hydros.push_back(hydro);
    m_document->lines.emplace_back(Line(Keyword::HYDROS, (int)m_document->hydros.size() - 1));
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
    
    beam.nodes[0] = this->GetArgNodeRef(0);
    beam.nodes[1] = this->GetArgNodeRef(1);
    if (m_num_args > 2) beam.options = this->GetArgBeamOptions(2);

    if ((m_num_args > 3) && BITMASK_IS_1(beam.options, Beam::OPTION_s_SUPPORT))
    {
        float support_break_factor = this->GetArgFloat(3);
        if (support_break_factor > 0.0f)
        {
            beam.support_break_limit = support_break_factor;
        }
    }

    m_document->beams.push_back(beam);
    m_document->lines.emplace_back(Line(Keyword::BEAMS, (int)m_document->beams.size() - 1));
}

void Parser::ParseAnimators()
{
    auto args = Ogre::StringUtil::split(m_current_line, ",");
    if (args.size() < 4) { return; }

    Animator animator;

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

    m_document->animators.push_back(animator);
    m_document->lines.emplace_back(Line(Keyword::ANIMATORS, (int)m_document->animators.size() - 1));
}

void Parser::ParseAuthor()
{
    if (! this->CheckNumArguments(2)) { return; }

    Author author;
    if (m_num_args > 1) { author.type             = this->GetArgStr(1); }
    if (m_num_args > 2) { author.forum_account_id = this->GetArgInt(2); author._has_forum_account = true; }
    if (m_num_args > 3) { author.name             = this->GetArgStr(3); }
    if (m_num_args > 4) { author.email            = this->GetArgStr(4); }

    m_current_block = Keyword::INVALID;
    m_document->author.push_back(author);
    m_document->lines.emplace_back(Line(Keyword::AUTHOR, (int)m_document->author.size() - 1));
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

Keyword Parser::IdentifyKeywordInCurrentLine()
{
    // Quick check - keyword always starts with ASCII letter
    char c = tolower(m_current_line[0]); // Note: line comes in trimmed
    if (c > 'z' || c < 'a')
    {
        return Keyword::INVALID;
    }

    // Search with correct lettercase
    std::smatch results;
    std::string line(m_current_line);
    std::regex_search(line, results, Regexes::IDENTIFY_KEYWORD_RESPECT_CASE); // Always returns true.
    Keyword keyword = FindKeywordMatch(results);
    if (keyword != Keyword::INVALID)
    {
        return keyword;
    }

    // Search and ignore lettercase
    std::regex_search(line, results, Regexes::IDENTIFY_KEYWORD_IGNORE_CASE); // Always returns true.
    keyword = FindKeywordMatch(results);
    return keyword;
}

Keyword Parser::FindKeywordMatch(std::smatch& search_results)
{
    // The 'results' array contains a complete match at positon [0] and sub-matches starting with [1], 
    //    so we get exact positions in Regexes::IDENTIFY_KEYWORD, which again match Keyword enum members

    for (unsigned int i = 1; i < search_results.size(); i++)
    {
        std::ssub_match sub  = search_results[i];
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
    m_document = RigDef::DocumentPtr(new Document());
 
}

void Parser::BeginBlock(Keyword keyword)
{
    m_current_block = keyword;
    if (keyword != Keyword::INVALID)
    {
        Line line(keyword, DATAPOS_INVALID);
        line.block_boundary = true;
        m_document->lines.push_back(line);
    }
}

void Parser::EndBlock(Keyword keyword)
{
    m_current_block = Keyword::INVALID;
    if (keyword != Keyword::INVALID)
    {
        Line line(keyword, DATAPOS_INVALID);
        line.block_boundary = true;
        m_document->lines.push_back(line);
    }
}

void Parser::Finalize()
{
    this->BeginBlock(Keyword::INVALID);
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

NodeRef_t Parser::GetArgRigidityNode(int index)
{
    std::string rigidity_node = this->GetArgStr(index);
    if (rigidity_node != "9999") // Special null value
    {
        return this->GetArgNodeRef(index);
    }
    return NODEREF_INVALID;
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

NodeRef_t Parser::GetArgNodeRef(int index)
{
    return this->_ParseNodeRef(this->GetArgStr(index));
}

NodeRef_t Parser::GetArgNullableNode(int index)
{
    if (! (Ogre::StringConverter::parseReal(this->GetArgStr(index)) == -1.f))
    {
        return this->GetArgNodeRef(index);
    }
    return NODEREF_INVALID;
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

            case '\0': break;

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
            
            case '\0': break;
            
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
            case '\0': break;

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
            case '\0': break;
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
            case '\0': break;

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
            case '\0': break;

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
            case '\0': break;

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
            case '\0': break;

            default:
                this->LogMessage(Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format("ignoring invalid option '{}'", c));
        }
    }
    return ret;
}

SpecialProp Parser::GetArgSpecialProp(int index)
{
    std::string str = this->GetArgStr(index);

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
