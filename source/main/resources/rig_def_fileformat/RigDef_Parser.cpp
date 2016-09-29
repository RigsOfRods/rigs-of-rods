/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
    @file   RigDef_Parser.cpp
    @author Petr Ohlidal
    @date   12/2013
*/

#include "RigDef_Parser.h"

#include "BeamConstants.h"
#include "RigDef_File.h"
#include "RigDef_Regexes.h"
#include "BitFlags.h"
#include "RoRPrerequisites.h"
#include "Utils.h"

#include <OgreException.h>
#include <OgreString.h>
#include <OgreStringVector.h>
#include <OgreStringConverter.h>

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

#define STR_PARSE_INT(_STR_)  Ogre::StringConverter::parseInt(_STR_)

#define STR_PARSE_REAL(_STR_) Ogre::StringConverter::parseReal(_STR_)

#define STR_PARSE_BOOL(_STR_) Ogre::StringConverter::parseBool(_STR_)

Parser::Parser():
    m_ror_minimass(0)
{
    // Push defaults 
    m_ror_default_inertia = std::shared_ptr<Inertia>(new Inertia);
    m_ror_node_defaults = std::shared_ptr<NodeDefaults>(new NodeDefaults);
}

void Parser::ProcessCurrentLine()
{
    // Check line type 
    std::smatch line_type_result;
    if (m_in_block_comment && StrEqualsNocase(m_current_line, "end_comment"))
    {
        m_in_block_comment = false;
        return;
    }
    else if (m_in_description_section && StrEqualsNocase(m_current_line, "end_description"))
    {
        m_in_description_section = false;
        return;
    }
    else if (m_in_description_section)
    {
        m_definition->description.push_back(m_current_line);
        return;
    }
    else if (StrEqualsNocase(m_current_line, "comment"))
    {
        m_in_block_comment = true;
        return;        
    }
    else if ((m_current_line[0] == ';') || (m_current_line[0] == '/')) // Comment line
    {
        return;
    }

    bool line_finished = false;
    bool scan_for_keyword = true;
    std::string line = m_current_line;

    // Prepare for switching file section 
    Ogre::String const & current_module_name = m_current_module->name;
    bool current_module_is_root = m_current_module == m_root_module;
    bool module_changed = false;
    Ogre::String new_module_name;
    bool new_module_is_root = false;

    File::Section new_section = File::SECTION_INVALID;
    File::Subsection new_subsection = File::SUBSECTION_INVALID;

    // Detect keywords on current line 
    // NOTE: Please maintain alphabetical order 
    if (scan_for_keyword)
    {
        File::Keyword keyword = IdentifyKeyword(line);
        switch (keyword)
        {
            case (File::KEYWORD_INVALID): // No new section 
                break;

            case (File::KEYWORD_ADD_ANIMATION):
                ParseDirectiveAddAnimation();
                line_finished = true;
                break;

            case (File::KEYWORD_AIRBRAKES):
                new_section = File::SECTION_AIRBRAKES;
                line_finished = true;
                break;

            case (File::KEYWORD_ANIMATORS):
                new_section = File::SECTION_ANIMATORS;
                line_finished = true;
                break;

            case (File::KEYWORD_ANTI_LOCK_BRAKES):
                ParseAntiLockBrakes();
                line_finished = true;
                break;

            case (File::KEYWORD_AXLES):
                new_section = File::SECTION_AXLES;
                line_finished = true;
                break;

            case (File::KEYWORD_AUTHOR):
                ParseAuthor();
                line_finished = true;
                break;

            case (File::KEYWORD_BACKMESH):
                if (m_current_section == File::SECTION_SUBMESH)
                {
                    m_current_submesh->backmesh = true;
                }
                else
                {
                    AddMessage(line, Message::TYPE_ERROR, "Misplaced sub-directive 'backmesh' (belongs in section 'submesh'), ignoring...");
                }
                line_finished = true;
                break;

            case (File::KEYWORD_BEAMS):
                new_section = File::SECTION_BEAMS;
                line_finished = true;
                break;

            case (File::KEYWORD_BRAKES):
                new_section = File::SECTION_BRAKES;
                line_finished = true;
                break;

            case (File::KEYWORD_CAB):
                new_subsection = File::SUBSECTION__SUBMESH__CAB;
                if (m_current_section != File::SECTION_SUBMESH)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Misplaced sub-section 'cab' (belongs in section 'submesh'), falling back to classic unsafe parsing method.");
                    new_section = File::SECTION_SUBMESH;
                }
                line_finished = true;
                break;

            case (File::KEYWORD_CAMERAS):
                new_section = File::SECTION_CAMERAS;
                line_finished = true;
                break;

            case (File::KEYWORD_CAMERARAIL):
                new_section = File::SECTION_CAMERA_RAIL;
                line_finished = true;
                break;

            case (File::KEYWORD_CINECAM):
                new_section = File::SECTION_CINECAM;
                line_finished = true;
                break;

            case (File::KEYWORD_COLLISIONBOXES):
                new_section = File::SECTION_COLLISION_BOXES;
                line_finished = true;
                break;

            case (File::KEYWORD_COMMANDS):
                new_section = File::SECTION_COMMANDS;
                line_finished = true;
                break;

            case (File::KEYWORD_COMMANDS2):
                new_section = File::SECTION_COMMANDS_2;
                line_finished = true;
                break;

            case (File::KEYWORD_CONTACTERS):
                new_section = File::SECTION_CONTACTERS;
                line_finished = true;
                break;

            case (File::KEYWORD_CRUISECONTROL):
                ParseCruiseControl(line);
                line_finished = true;
                break;

            case (File::KEYWORD_DESCRIPTION):
                m_in_description_section = true;
                line_finished = true;
                break;

            case (File::KEYWORD_DETACHER_GROUP):
                ParseDirectiveDetacherGroup();
                line_finished = true;
                break;

            case (File::KEYWORD_DISABLEDEFAULTSOUNDS):
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Directive 'disabledefaultsounds' has global effect and should not appear in a module");
                }
                m_definition->disable_default_sounds = true;
                line_finished = true;
                break;

            case (File::KEYWORD_ENABLE_ADVANCED_DEFORMATION):
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Directive 'enable_advanced_deformation' has global effect and should not appear in a module");
                }
                m_definition->enable_advanced_deformation = true;
                line_finished = true;
                break;

            case (File::KEYWORD_END):
                new_section = File::SECTION_NONE;
                line_finished = true;
                break;

            case (File::KEYWORD_END_SECTION):
                module_changed = true;
                new_module_is_root = true;
                line_finished = true;
                break;

            case (File::KEYWORD_ENGINE):
                new_section = File::SECTION_ENGINE;
                line_finished = true;
                break;

            case (File::KEYWORD_ENGOPTION):
                new_section = File::SECTION_ENGOPTION;
                line_finished = true;
                break;

            case (File::KEYWORD_ENGTURBO) :
                new_section = File::SECTION_ENGTURBO;
                line_finished = true;
                break;

            case (File::KEYWORD_ENVMAP):
                // Ignored 
                line_finished = true;
                break;

            case (File::KEYWORD_EXHAUSTS):
                new_section = File::SECTION_EXHAUSTS;
                line_finished = true;
                break;

            case (File::KEYWORD_EXTCAMERA):
                ParseExtCamera();
                line_finished = true;
                break;

            case (File::KEYWORD_FILEFORMATVERSION):
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Inline section 'fileformatversion' has global effect and should not appear in a module");
                }
                ParseFileFormatVersion(line);
                line_finished = true;
                break;

            case (File::KEYWORD_FILEINFO):
                ParseFileinfo();
                line_finished = true;
                break;

            case (File::KEYWORD_FIXES):
                new_section = File::SECTION_FIXES;
                line_finished = true;
                break;

            case (File::KEYWORD_FLARES):
                new_section = File::SECTION_FLARES;
                line_finished = true;
                break;

            case (File::KEYWORD_FLARES2):
                new_section = File::SECTION_FLARES_2;
                line_finished = true;
                break;

            case (File::KEYWORD_FLEXBODIES):
                new_section = File::SECTION_FLEXBODIES;
                line_finished = true;
                break;

            case (File::KEYWORD_FLEXBODY_CAMERA_MODE):
                ParseDirectiveFlexbodyCameraMode();
                line_finished = true;
                break;

            case (File::KEYWORD_FLEXBODYWHEELS):
                new_section = File::SECTION_FLEX_BODY_WHEELS;
                line_finished = true;
                break;

            case (File::KEYWORD_FORWARDCOMMANDS):
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Directive 'forwardcommands' has global effect and should not appear in a module");
                }
                m_definition->forward_commands = true;
                line_finished = true;
                break;

            case (File::KEYWORD_FUSEDRAG):
                new_section = File::SECTION_FUSEDRAG;
                line_finished = true;
                break;

            case (File::KEYWORD_GLOBALS):
                new_section = File::SECTION_GLOBALS;
                line_finished = true;
                break;

            case (File::KEYWORD_GUID):
                ParseGuid(line);
                line_finished = true;
                break;

            case (File::KEYWORD_GUISETTINGS):
                new_section = File::SECTION_GUI_SETTINGS;
                line_finished = true;
                break;

            case (File::KEYWORD_HELP):
                new_section = File::SECTION_HELP;
                line_finished = true;
                break;

            case (File::KEYWORD_HIDE_IN_CHOOSER):
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Directive 'hideInChooser' has global effect and should not appear in a module");
                }
                m_definition->hide_in_chooser = true;
                line_finished = true;
                break;

            case (File::KEYWORD_HOOKGROUP):
                // Not supported yet, ignore 
                line_finished = true;
                break;

            case (File::KEYWORD_HOOKS):
                new_section = File::SECTION_HOOKS;
                line_finished = true;
                break;

            case (File::KEYWORD_HYDROS):
                new_section = File::SECTION_HYDROS;
                line_finished = true;
                break;

            case (File::KEYWORD_IMPORTCOMMANDS):
                m_definition->import_commands = true;
                line_finished = true;
                break;

            case (File::KEYWORD_LOCKGROUPS):
                new_section = File::SECTION_LOCKGROUPS;
                line_finished = true;
                break;

            case (File::KEYWORD_LOCKGROUP_DEFAULT_NOLOCK):
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Directive 'lockgroup_default_nolock' has global effect and should not appear in a module");
                }
                m_definition->lockgroup_default_nolock = true;
                line_finished = true;
                break;

            case (File::KEYWORD_MANAGEDMATERIALS):
                new_section = File::SECTION_MANAGED_MATERIALS;
                line_finished = true;
                break;

            case (File::KEYWORD_MATERIALFLAREBINDINGS):
                new_section = File::SECTION_MATERIAL_FLARE_BINDINGS;
                line_finished = true;
                break;

            case (File::KEYWORD_MESHWHEELS):
                new_section = File::SECTION_MESH_WHEELS;
                line_finished = true;
                break;

            case (File::KEYWORD_MESHWHEELS2):
                new_section = File::SECTION_MESH_WHEELS_2;
                line_finished = true;
                break;

            case (File::KEYWORD_MINIMASS):
                new_section = File::SECTION_MINIMASS;
                line_finished = true;
                break;

            case (File::KEYWORD_NODECOLLISION):
                new_section = File::SECTION_NODE_COLLISION;
                line_finished = true;
                break;

            case (File::KEYWORD_NODES):
                new_section = File::SECTION_NODES;
                line_finished = true;
                break;

            case (File::KEYWORD_NODES2):
                new_section = File::SECTION_NODES_2;
                line_finished = true;
                break;

            case (File::KEYWORD_PARTICLES):
                new_section = File::SECTION_PARTICLES;
                line_finished = true;
                break;

            case (File::KEYWORD_PISTONPROPS):
                new_section = File::SECTION_PISTONPROPS;
                line_finished = true;
                break;

            case (File::KEYWORD_PROP_CAMERA_MODE):
                ParseDirectivePropCameraMode();
                line_finished = true;
                break;

            case (File::KEYWORD_PROPS):
                new_section = File::SECTION_PROPS;
                line_finished = true;
                break;

            case (File::KEYWORD_RAILGROUPS):
                new_section = File::SECTION_RAILGROUPS;
                line_finished = true;
                break;

            case (File::KEYWORD_RESCUER):
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Directive 'rescuer' has global effect and should not appear in a module");
                }
                m_definition->rescuer = true;
                line_finished = true;
                break;

            case (File::KEYWORD_RIGIDIFIERS):
                AddMessage(line, Message::TYPE_WARNING, "Rigidifiers are not supported, ignoring...");
                new_section = File::SECTION_NONE;
                line_finished = true;
                break;

            case (File::KEYWORD_ROLLON):
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Directive 'rollon' has global effect and should not appear in a module");
                }
                m_definition->rollon = true;
                line_finished = true;
                break;

            case (File::KEYWORD_ROPABLES):
                new_section = File::SECTION_ROPABLES;
                line_finished = true;
                break;

            case (File::KEYWORD_ROPES):
                new_section = File::SECTION_ROPES;
                line_finished = true;
                break;

            case (File::KEYWORD_ROTATORS):
                new_section = File::SECTION_ROTATORS;
                line_finished = true;
                break;

            case (File::KEYWORD_ROTATORS2):
                new_section = File::SECTION_ROTATORS_2;
                line_finished = true;
                break;

            case (File::KEYWORD_SCREWPROPS):
                new_section = File::SECTION_SCREWPROPS;
                line_finished = true;
                break;

            case (File::KEYWORD_SECTION):
                {
                    std::pair<bool, Ogre::String> result = GetModuleName(line);
                    if (result.first)
                    {
                        new_module_name = result.second;
                        module_changed = true;
                    }
                }
                line_finished = true;
                break;

            case (File::KEYWORD_SECTIONCONFIG):
                // Ignored 
                line_finished = true;
                break;

            case (File::KEYWORD_SET_BEAM_DEFAULTS):
                ParseDirectiveSetBeamDefaults();
                line_finished = true;
                break;

            case (File::KEYWORD_SET_BEAM_DEFAULTS_SCALE):
                ParseDirectiveSetBeamDefaultsScale(line);
                line_finished = true;
                break;

            case (File::KEYWORD_SET_COLLISION_RANGE):
                ParseSetCollisionRange();
                line_finished = true;
                break;

            case (File::KEYWORD_SET_INERTIA_DEFAULTS):
                ParseDirectiveSetInertiaDefaults(line);
                line_finished = true;
                break;

            case (File::KEYWORD_SET_MANAGEDMATERIALS_OPTIONS):
                ParseDirectiveSetManagedMaterialsOptions(line);
                line_finished = true;
                break;

            case (File::KEYWORD_SET_NODE_DEFAULTS):
                ParseDirectiveSetNodeDefaults();
                line_finished = true;
                break;

            case (File::KEYWORD_SET_SKELETON_SETTINGS):
                ParseSetSkeletonSettings(line);
                line_finished = true;
                break;

            case (File::KEYWORD_SHOCKS):
                new_section = File::SECTION_SHOCKS;
                line_finished = true;
                break;

            case (File::KEYWORD_SHOCKS2):
                new_section = File::SECTION_SHOCKS_2;
                line_finished = true;
                break;

            case (File::KEYWORD_SLIDENODE_CONNECT_INSTANTLY):
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Directive 'slidenode_connect_instantly' has global effect and should not appear in a module");
                }
                m_definition->slide_nodes_connect_instantly = true;
                line_finished = true;
                break;

            case (File::KEYWORD_SLIDENODES):
                new_section = File::SECTION_SLIDENODES;
                line_finished = true;
                break;

            case (File::KEYWORD_SLOPE_BRAKE):
                ParseSlopeBrake();
                line_finished = true;
                break;

            case (File::KEYWORD_SOUNDSOURCES):
                new_section = File::SECTION_SOUNDSOURCES;
                line_finished = true;
                break;

            case (File::KEYWORD_SOUNDSOURCES2):
                new_section = File::SECTION_SOUNDSOURCES2;
                line_finished = true;
                break;

#if 0 // Not supported yet
            case (File::KEYWORD_SOUNDSOURCES3):
                new_section = File::SECTION_;
                line_finished = true;
                break;
#endif
            case (File::KEYWORD_SPEEDLIMITER):
                ParseSpeedLimiter();
                line_finished = true;
                break;

            case (File::KEYWORD_SUBMESH_GROUNDMODEL):
                ParseSubmeshGroundModel();
                line_finished = true;
                break;

            case (File::KEYWORD_SUBMESH):
                new_section = File::SECTION_SUBMESH;
                line_finished = true;
                break;

            case (File::KEYWORD_TEXCOORDS):
                new_subsection = File::SUBSECTION__SUBMESH__TEXCOORDS;
                if (m_current_section != File::SECTION_SUBMESH)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Misplaced sub-section 'texcoords' (belongs in section 'submesh'), falling back to classic unsafe parsing method.");
                    new_section = File::SECTION_SUBMESH;
                }
                line_finished = true;
                break;

            case (File::KEYWORD_TIES):
                new_section = File::SECTION_TIES;
                line_finished = true;
                break;

            case (File::KEYWORD_TORQUECURVE):
                new_section = File::SECTION_TORQUE_CURVE;
                line_finished = true;
                break;

            case (File::KEYWORD_TRACTION_CONTROL):
                ParseTractionControl();
                line_finished = true;
                break;

            case (File::KEYWORD_TRIGGERS):
                new_section = File::SECTION_TRIGGERS;
                line_finished = true;
                break;

            case (File::KEYWORD_TURBOJETS):
                new_section = File::SECTION_TURBOJETS;
                line_finished = true;
                break;

            case (File::KEYWORD_TURBOPROPS):
                new_section = File::SECTION_TURBOPROPS;
                line_finished = true;
                break;

            case (File::KEYWORD_TURBOPROPS2):
                AddMessage(line, Message::TYPE_WARNING, "Turboprops2 are not supported, ignoring...");
                new_section = File::SECTION_NONE;
                line_finished = true;
                break;

            case (File::KEYWORD_VIDEOCAMERA):
                new_section = File::SECTION_VIDEO_CAMERA;
                line_finished = true;
                break;

            case (File::KEYWORD_WHEELS):
                new_section = File::SECTION_WHEELS;
                line_finished = true;
                break;

            case (File::KEYWORD_WHEELS2):
                new_section = File::SECTION_WHEELS_2;
                line_finished = true;
                break;

            case (File::KEYWORD_WINGS):
                new_section = File::SECTION_WINGS;
                line_finished = true;
                break;

        }
    }

    if (module_changed)
    {
        new_section = File::SECTION_NONE; // Make parser commit unfinished work, actual module switch happens afterwards. 	
    }

    // Section-specific switch logic 
    if (new_section != File::SECTION_INVALID)
    {
        // Exit sections 
        _ExitSections(line);
        
        // Enter sections 
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

        m_current_section = new_section;
    }
    if (new_subsection != File::SUBSECTION_INVALID)
    {
        m_current_subsection = new_subsection;
    }

    // Module-switching 
    if (module_changed)
    {
        m_last_flexbody.reset(); // Set to nullptr

        if (new_module_is_root)
        {
            if (current_module_is_root)
            {
                AddMessage(line, Message::TYPE_ERROR, "Misplaced keyword 'end_section', ignoring...");
            }
            else
            {
                RestoreRootModule();
            }
        }
        else
        {
            if (current_module_name == new_module_name)
            {
                AddMessage(line, Message::TYPE_ERROR, "Attempt to re-enter current module, ignoring...");
            }
            else
            {
                SetCurrentModule(new_module_name);
            }
        }
    }

    // Continue? 
    if (line_finished)
    {
        return;
    }

    this->TokenizeCurrentLine();

    // Parse current section, if any 
    // NOTE: Please maintain alphabetical order 
    switch (m_current_section)
    {

        case (File::SECTION_AIRBRAKES):
            ParseAirbrakes();
            line_finished = true;
            break;

        case (File::SECTION_ANIMATORS):
            ParseAnimator();
            line_finished = true;
            break;

        case (File::SECTION_AXLES):
            ParseAxles(line);
            line_finished = true;
            break;

        case (File::SECTION_TRUCK_NAME):
            m_definition->name = Ogre::String(line);
            Ogre::StringUtil::trim(m_definition->name);
            m_current_section = File::SECTION_NONE;
            line_finished = true;
            break;

        case (File::SECTION_BEAMS):
            ParseBeams();
            line_finished = true;
            break;

        case (File::SECTION_BRAKES):
            ParseBrakes();
            line_finished = true;
            break;

        case (File::SECTION_CAMERAS):
            ParseCameras();
            line_finished = true;
            break;

        case (File::SECTION_CAMERA_RAIL):
            ParseCameraRails();
            line_finished = true;
            break;

        case (File::SECTION_CINECAM):
            ParseCinecam();
            line_finished = true;
            break;

        case (File::SECTION_COMMANDS):
        case (File::SECTION_COMMANDS_2):
            ParseCommandsUnified();
            line_finished = true;
            break;

        case (File::SECTION_COLLISION_BOXES):
            ParseCollisionBox();
            line_finished = true;
            break;

        case (File::SECTION_CONTACTERS):
            ParseContacter();
            line_finished = true;
            break;

        case (File::SECTION_ENGINE):
            ParseEngine();
            line_finished = true;
            break;

        case (File::SECTION_ENGOPTION):
            ParseEngoption();
            line_finished = true;
            break;

        case (File::SECTION_ENGTURBO) :
            ParseEngturbo();
            line_finished = true;
            break;

        case (File::SECTION_EXHAUSTS):
            ParseExhaust();
            line_finished = true;
            break;

        case (File::SECTION_FIXES):
            ParseFixes();
            line_finished = true;
            break;

        case (File::SECTION_FLARES):
        case (File::SECTION_FLARES_2):
            ParseFlaresUnified();
            line_finished = true;
            break;

        case (File::SECTION_FLEXBODIES):
            ParseFlexbody();
            line_finished = true;
            break;

        case (File::SECTION_FLEX_BODY_WHEELS):
            ParseFlexBodyWheel();
            line_finished = true;
            break;

        case (File::SECTION_FUSEDRAG):
            ParseFusedrag();
            line_finished = true;
            break;

        case (File::SECTION_GLOBALS):
            ParseGlobals(line);
            line_finished = true;
            break;

        case (File::SECTION_GUI_SETTINGS):
            ParseGuiSettings();
            line_finished = true;
            break;

        case (File::SECTION_HELP):
            ParseHelp(line);
            line_finished = true;
            break;

        case (File::SECTION_HOOKS):
            ParseHook(); 
            line_finished = true;
            break;

        case (File::SECTION_HYDROS):
            ParseHydros();
            line_finished = true;
            break;

        case (File::SECTION_LOCKGROUPS):
            ParseLockgroups();
            line_finished = true;
            break;

        case (File::SECTION_MANAGED_MATERIALS):
            ParseManagedMaterials();
            line_finished = true;
            break;

        case (File::SECTION_MATERIAL_FLARE_BINDINGS):
            ParseMaterialFlareBindings(line);
            line_finished = true;
            break;

        case (File::SECTION_MESH_WHEELS):
        case (File::SECTION_MESH_WHEELS_2):
            ParseMeshWheelUnified();
            line_finished = true;
            break;

        case (File::SECTION_MINIMASS):
            ParseMinimass(line);
            line_finished = true;
            break;

        case (File::SECTION_NODE_COLLISION):
            ParseNodeCollision(line);
            line_finished = true;
            break;

        case (File::SECTION_NODES):
        case (File::SECTION_NODES_2):
            ParseNodesUnified();
            line_finished = true;
            break;

        case (File::SECTION_PARTICLES):
            ParseParticles(line);
            line_finished = true;
            break;

        case (File::SECTION_PISTONPROPS):
            ParsePistonprops(line);
            line_finished = true;
            break;

        case (File::SECTION_PROPS):
            ParseProps();
            line_finished = true;
            break;

        case (File::SECTION_RAILGROUPS):
            ParseRailGroups(line);
            line_finished = true;
            break;

        case (File::SECTION_ROPABLES):
            ParseRopables();
            line_finished = true;
            break;

        case (File::SECTION_ROPES):
            ParseRopes(line);
            line_finished = true;
            break;

        case (File::SECTION_ROTATORS):
        case (File::SECTION_ROTATORS_2):
            ParseRotatorsUnified();
            line_finished = true;
            break;

        case (File::SECTION_SCREWPROPS):
            ParseScrewprops(line);
            line_finished = true;
            break;

        case (File::SECTION_SHOCKS):
            ParseShock();
            line_finished = true;
            break;

        case (File::SECTION_SHOCKS_2):
            ParseShock2();
            line_finished = true;
            break;

        case (File::SECTION_SLIDENODES):
            ParseSlidenodes(line);
            line_finished = true;
            break;

        case (File::SECTION_SOUNDSOURCES):
            ParseSoundsources();
            line_finished = true;
            break;

        case (File::SECTION_SOUNDSOURCES2):
            ParseSoundsources2();
            line_finished = true;
            break;

        case (File::SECTION_SUBMESH):
            ParseSubmesh();
            line_finished = true;
            break;

        case (File::SECTION_TIES):
            ParseTies();
            line_finished = true;
            break;

        case (File::SECTION_TORQUE_CURVE):
            ParseTorqueCurve();
            line_finished = true;
            break;

        case (File::SECTION_TRIGGERS):
            ParseTriggers();
            line_finished = true;
            break;

        case (File::SECTION_TURBOJETS):
            ParseTurbojets();
            line_finished = true;
            break;

        case (File::SECTION_TURBOPROPS):
            ParseTurbopropsUnified();
            line_finished = true;
            break;

        case (File::SECTION_VIDEO_CAMERA):
            ParseVideoCamera();
            line_finished = true;
            break;

        case (File::SECTION_WHEELS):
            ParseWheel();
            line_finished = true;
            break;

        case (File::SECTION_WHEELS_2):
            ParseWheel2();
            line_finished = true;
            break;

        case (File::SECTION_WINGS):
            ParseWing();
            line_finished = true;
            break;

        default:
            break;
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

#if 0 // TEMPLATE 

void Parser::Parse()
{
    this->GetArg( 0);
    this->GetArg( 1);
    this->GetArg( 2);
    this->GetArg( 3);
    this->GetArg( 4);
    this->GetArg( 5);
    this->GetArg( 6);
    this->GetArg( 7);
    this->GetArg( 8);
    this->GetArg( 9);
    this->GetArg(10);
    this->GetArg(11);
    this->GetArg(12);
    this->GetArg(13);
    this->GetArg(14);
    this->GetArg(15);
}

#endif

void Parser::ParseWing()
{
    if (!this->CheckNumArguments(16)) { return; }

    Wing wing;

    for (int i = 0; i <  8; i++) { wing.nodes[i]        = this->GetArgNodeRef     (i);  }
    for (int i = 8; i < 16; i++) { wing.tex_coords[i]   = this->GetArgFloat       (i);  }

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
    this->TokenizeCurrentLine();
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

    if (tokens.size() > 4)
    {
        Ogre::StringVector attrs = Ogre::StringUtil::split(tokens[4], "&");
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

    if (m_current_module->traction_control != nullptr)
    {
        this->AddMessage(Message::TYPE_WARNING, "Multiple inline-sections 'TractionControl' in a module, using last one ...");
    }
    m_current_module->traction_control = std::shared_ptr<TractionControl>( new TractionControl(tc) );
}

void Parser::ParseSubmeshGroundModel()
{
    this->TokenizeCurrentLine();
    if (!this->CheckNumArguments(2)) { return; } // Items: keyword, arg

    m_current_module->submeshes_ground_model_name = this->GetArgStr(1);
}

void Parser::ParseSpeedLimiter()
{
    this->TokenizeCurrentLine();
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

void Parser::ParseSetSkeletonSettings(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_SET_SKELETON_DISPLAY))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (m_current_module->skeleton_settings == nullptr)
    {
        m_current_module->skeleton_settings = std::shared_ptr<RigDef::SkeletonSettings>(new SkeletonSettings());
    }

    float visibility_meters = STR_PARSE_REAL(results[1]);
    if (visibility_meters < 0)
    {
        m_current_module->skeleton_settings->visibility_range_meters = m_ror_skeleton_settings.visibility_range_meters;
    } 
    else
    {
        m_current_module->skeleton_settings->visibility_range_meters = visibility_meters;
    }

    if (results[2].matched)
    {
        float beam_thickness_meters = STR_PARSE_REAL(results[3]);
        if (beam_thickness_meters < 0)
        {
            m_current_module->skeleton_settings->beam_thickness_meters = m_ror_skeleton_settings.beam_thickness_meters;
        }
        else
        {
            m_current_module->skeleton_settings->beam_thickness_meters = beam_thickness_meters;
        }
    }
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
    this->TokenizeCurrentLine();
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

    if (!opt_str.empty())
    {
        this->_ParseNodeOptions(m_user_node_defaults->options, opt_str);
    }
    this->LogParsedDirectiveSetNodeDefaultsData(
        load_weight, friction, volume, surface, m_user_node_defaults->options);
}

void Parser::_ParseNodeOptions(unsigned int & options, const std::string & options_str)
{
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

void Parser::ParseDirectiveSetManagedMaterialsOptions(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::DIRECTIVE_SET_MANAGEDMATERIALS_OPTIONS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    // Param 1: double-sided 
    Ogre::String input = results[1];
    int double_sided = STR_PARSE_INT(input); // Let's do it the way old parser did, even though "input" can be any string 
    if (! std::regex_match(input, std::regex("0|1", std::regex::ECMAScript)))
    {
        AddMessage(
            line, 
            Message::TYPE_WARNING, 
            "Directive 'set_managedmaterials_options':"
                " Invalid value of parameter ~1: '" + input + "', should be only '0' or '1'."
                " Interpreting as '0' for backwards compatibility. Please fix."
            );
    }
    m_current_managed_material_options.double_sided = double_sided != 0;
}

void Parser::ParseDirectiveSetBeamDefaultsScale(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::DIRECTIVE_SET_BEAM_DEFAULTS_SCALE))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    m_user_beam_defaults = std::shared_ptr<BeamDefaults>( new BeamDefaults(*m_user_beam_defaults) );

    m_user_beam_defaults->scale.springiness = STR_PARSE_REAL(results[1]);

    if (results[2].matched)
    {
        m_user_beam_defaults->scale.damping_constant = STR_PARSE_REAL(results[3]);

        if (results[4].matched)
        {
            m_user_beam_defaults->scale.deformation_threshold_constant = STR_PARSE_REAL(results[5]);

            if (results[6].matched)
            {
                m_user_beam_defaults->scale.breaking_threshold_constant = STR_PARSE_REAL(results[7]);
            }
        }
    }
}

void Parser::ParseDirectiveSetBeamDefaults()
{
    this->TokenizeCurrentLine();
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

    if (d.springiness           < 0.f) { d.springiness           = DEFAULT_SPRING;        }
    if (d.damping_constant      < 0.f) { d.damping_constant      = DEFAULT_DAMP;          }
    if (d.deformation_threshold < 0.f) { d.deformation_threshold = BEAM_DEFORM;           }
    if (d.breaking_threshold    < 0.f) { d.breaking_threshold    = BEAM_BREAK;            }
    if (d.visual_beam_diameter  < 0.f) { d.visual_beam_diameter  = DEFAULT_BEAM_DIAMETER; }

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

    this->TokenizeCurrentLine();
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    this->_ParseCameraSettings(m_current_module->props.back().camera_settings, this->GetArgStr(1));
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
        else if ((attr == "selflock") ||(attr == "self-lock") ||(attr == "self_lock") ) { BITMASK_SET_1(hook.flags, Hook::FLAG_SELF_LOCK);  }
        else if ((attr == "autolock") ||(attr == "auto-lock") ||(attr == "auto_lock") ) { BITMASK_SET_1(hook.flags, Hook::FLAG_AUTO_LOCK);  }
        else if ((attr == "nodisable")||(attr == "no-disable")||(attr == "no_disable")) { BITMASK_SET_1(hook.flags, Hook::FLAG_NO_DISABLE); }
        else if ((attr == "norope")   ||(attr == "no-rope")   ||(attr == "no_rope")   ) { BITMASK_SET_1(hook.flags, Hook::FLAG_NO_ROPE);    }
        else if ((attr == "visible")  ||(attr == "vis")                               ) { BITMASK_SET_1(hook.flags, Hook::FLAG_VISIBLE);    }
        else
        {
            std::string msg = "Ignoring invalid option: " + attr;
            this->AddMessage(Message::TYPE_ERROR, msg.c_str());
        }
        i++;
    }

    m_current_module->hooks.push_back(hook);
}

void Parser::ParseHelp(Ogre::String const & line)
{
    if (! m_current_module->help_panel_material_name.empty())
    {
        AddMessage(line, Message::TYPE_WARNING, "Multiple lines of section 'help', using the last found...");
    }

    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_HELP))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    m_current_module->help_panel_material_name = results[1];
}

void Parser::ParseGuiSettings()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    GuiSettings* gui_settings = m_current_module->gui_settings.get();
   
    std::string key = this->GetArgStr(0);
    
    if (key == "debugBeams") {} // Obsolete, ignored
    
    else if (key == "tachoMaterial")  { gui_settings->tacho_material     =  this->GetArgStr(1); }
    else if (key == "speedoMaterial") { gui_settings->speedo_material    =  this->GetArgStr(1); }
    else if (key == "speedoMax")      { gui_settings->speedo_highest_kph =  this->GetArgInt(1); }
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

void Parser::ParseGuid(Ogre::String const & line)
{
    if (! m_definition->guid.empty())
    {
        AddMessage(line, Message::TYPE_WARNING, "Multiple sections 'guid', using the last defined...");
    }

    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_GUID))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid inline-section 'guid', ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    m_definition->guid = results[1];
}

void Parser::ParseGlobals(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_GLOBALS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }

    if (m_current_module->globals != nullptr)
    {
        AddMessage(line, Message::TYPE_WARNING, "Multiple sections 'globals' in one module, using the first one found.");
        return;
    }

    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Globals globals;
    globals.dry_mass = STR_PARSE_REAL(results[1]);
    globals.cargo_mass = STR_PARSE_REAL(results[2]);
    if (results[3].matched)
    {
        globals.material_name = results[4];
    }

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

    this->TokenizeCurrentLine();
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
        strncpy_s(setdef, m_current_line + 6, LINE_BUFFER_LENGTH); // Cut away "forset"
        const char* item = std::strtok(setdef, ",");

        // TODO: Add error reporting
        // It appears strtoul() sets no ERRNO for input 'x1' (parsed -> '0')

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
                    size_t length = std::max(a_end - item, 200);
                    a_text = std::string(item, length);
                }
                char* b_end = nullptr;
                const char* item2 = hyphen + 1;
                unsigned b = ::strtoul(item2, &b_end, 10);
                size_t length = std::max(b_end - item2, 200);
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
    if (m_num_args > 3) { exhaust.material_name = this->GetArgStr(3); }

    m_current_module->exhausts.push_back(exhaust);
}

void Parser::ParseFileFormatVersion(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_FILE_FORMAT_VERSION))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    const unsigned version = STR_PARSE_INT(results[1]);
    m_definition->file_format_version = version;

    if (version >= 450)
    {
        m_sequential_importer.Disable();
    }
}

void Parser::ParseDirectiveDetacherGroup()
{
    this->TokenizeCurrentLine();
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

void Parser::ParseCruiseControl(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_CRUISECONTROL))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    CruiseControl cruise_control;
    cruise_control.min_speed = STR_PARSE_REAL(results[1]);
    cruise_control.autobrake = STR_PARSE_INT(results[2]);

    if (m_current_module->cruise_control != nullptr)
    {
        AddMessage(line, Message::TYPE_WARNING, "Found multiple inline sections 'cruise_control' in one module, using last one.");
    }
    m_current_module->cruise_control = std::shared_ptr<CruiseControl>( new CruiseControl(cruise_control) );
}

void Parser::ParseDirectiveAddAnimation()
{
    assert(m_current_module != nullptr);

    if (m_current_module->props.size() == 0)
    {
        AddMessage(Message::TYPE_ERROR, "Directive 'add_animation' has no prop to animate, ignoring...");
        return;
    }

    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 13); // "add_animation" = 13 characters

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
        Ogre::StringVector entry = Ogre::StringUtil::split(*itor);
        Ogre::StringUtil::trim(entry[0]);
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
            Ogre::StringVector values = Ogre::StringUtil::split(entry[1]);
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
                Ogre::StringUtil::trim(entry[1]);
                animation.event = entry[1];
            }
            else if (entry[0] == "source")
            {
                // Cannot be combined with "|"
                Ogre::StringUtil::trim(entry[1]);
                if (entry[1] == "event")
                {
                    animation.source |= Animation::SOURCE_EVENT;
                    continue;
                }

                for (auto itor = values.begin(); itor != values.end(); ++itor)
                {
                    std::string value = *itor;
                    Ogre::StringUtil::trim(value);

                         if (entry[1] == "airspeed")      { animation.source |= Animation::SOURCE_AIRSPEED;          }
                    else if (entry[1] == "vvi")           { animation.source |= Animation::SOURCE_VERTICAL_VELOCITY; }
                    else if (entry[1] == "altimeter100k") { animation.source |= Animation::SOURCE_ALTIMETER_100K;    }
                    else if (entry[1] == "altimeter10k")  { animation.source |= Animation::SOURCE_ALTIMETER_10K;     }
                    else if (entry[1] == "altimeter1k")   { animation.source |= Animation::SOURCE_ALTIMETER_1K;      }
                    else if (entry[1] == "aoa")           { animation.source |= Animation::SOURCE_ANGLE_OF_ATTACK;   }
                    else if (entry[1] == "flap")          { animation.source |= Animation::SOURCE_FLAP;              }
                    else if (entry[1] == "airbrake")      { animation.source |= Animation::SOURCE_AIR_BRAKE;         }
                    else if (entry[1] == "roll")          { animation.source |= Animation::SOURCE_ROLL;              }
                    else if (entry[1] == "pitch")         { animation.source |= Animation::SOURCE_PITCH;             }
                    else if (entry[1] == "brake")         { animation.source |= Animation::SOURCE_BRAKES;            }
                    else if (entry[1] == "accel")         { animation.source |= Animation::SOURCE_ACCEL;             }
                    else if (entry[1] == "clutch")        { animation.source |= Animation::SOURCE_CLUTCH;            }
                    else if (entry[1] == "speedo")        { animation.source |= Animation::SOURCE_SPEEDO;            }
                    else if (entry[1] == "tacho")         { animation.source |= Animation::SOURCE_TACHO;             }
                    else if (entry[1] == "turbo")         { animation.source |= Animation::SOURCE_TURBO;             }
                    else if (entry[1] == "pbrake")        { animation.source |= Animation::SOURCE_PARKING;           }
                    else if (entry[1] == "shifterman1")   { animation.source |= Animation::SOURCE_SHIFT_LEFT_RIGHT;  }
                    else if (entry[1] == "shifterman2")   { animation.source |= Animation::SOURCE_SHIFT_BACK_FORTH;  }
                    else if (entry[1] == "sequential")    { animation.source |= Animation::SOURCE_SEQUENTIAL_SHIFT;  }
                    else if (entry[1] == "shifterlin")    { animation.source |= Animation::SOURCE_SHIFTERLIN;        }
                    else if (entry[1] == "torque")        { animation.source |= Animation::SOURCE_TORQUE;            }
                    else if (entry[1] == "heading")       { animation.source |= Animation::SOURCE_HEADING;           }
                    else if (entry[1] == "difflock")      { animation.source |= Animation::SOURCE_DIFFLOCK;          }
                    else if (entry[1] == "rudderboat")    { animation.source |= Animation::SOURCE_BOAT_RUDDER;       }
                    else if (entry[1] == "throttleboat")  { animation.source |= Animation::SOURCE_BOAT_THROTTLE;     }
                    else if (entry[1] == "steeringwheel") { animation.source |= Animation::SOURCE_STEERING_WHEEL;    }
                    else if (entry[1] == "aileron")       { animation.source |= Animation::SOURCE_AILERON;           }
                    else if (entry[1] == "elevator")      { animation.source |= Animation::SOURCE_ELEVATOR;          }
                    else if (entry[1] == "rudderair")     { animation.source |= Animation::SOURCE_AIR_RUDDER;        }
                    else if (entry[1] == "permanent")     { animation.source |= Animation::SOURCE_PERMANENT;         }

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
    if (tokens.size() > 4)
    {
        Ogre::StringVector attrs = Ogre::StringUtil::split(tokens[4], "&");
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
    engturbo.param2         = this->GetArgFloat( 4);
    engturbo.param3         = this->GetArgFloat( 5);
    engturbo.param4         = this->GetArgFloat( 6);
    engturbo.param5         = this->GetArgFloat( 7);
    engturbo.param6         = this->GetArgFloat( 8);
    engturbo.param7         = this->GetArgFloat( 9);
    engturbo.param8         = this->GetArgFloat(10);
    engturbo.param9         = this->GetArgFloat(11);
    engturbo.param10        = this->GetArgFloat(12);
    engturbo.param11        = this->GetArgFloat(13);
    
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
    m_current_module->contacters.push_back(this->GetArgNodeRef(0));
}

void Parser::ParseCommandsUnified()
{
    const bool is_commands2 = (m_current_section == File::SECTION_COMMANDS_2);
    const int max_args = (is_commands2 ? 8 : 7);
    if (!this->CheckNumArguments(max_args))
    {
        return;
    }

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
    
    if (m_num_args > 10) { cinecam.spring  = this->GetArgFloat(11); }
    if (m_num_args > 11) { cinecam.damping = this->GetArgFloat(12); }
    
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

    m_current_module->brakes.default_braking_force = this->GetArgFloat(0);

    if (m_num_args > 1)
    {
        m_current_module->brakes.parking_brake_force = this->GetArgFloat(1);
    }
}

void Parser::ParseAxles(Ogre::String const & line)
{
    Axle axle;

    Ogre::StringVector tokens = Ogre::StringUtil::split(line, ",");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        std::smatch results;
        if (! std::regex_search(*iter, results, Regexes::SECTION_AXLES_PROPERTY))
        {
            AddMessage(line, Message::TYPE_ERROR, "Invalid property, ignoring whole line...");
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

                    default: // No check needed, regex takes care of that 
                        break;
                }
            }
        }
    }

    m_current_module->axles.push_back(axle);	
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
    if (! this->CheckNumArguments(8)) { return; }

    Turboprop2 turboprop;
    
    turboprop.reference_node     = this->GetArgNodeRef(0);
    turboprop.axis_node          = this->GetArgNodeRef(1);
    turboprop.blade_tip_nodes[0] = this->GetArgNodeRef(2);
    turboprop.blade_tip_nodes[1] = this->GetArgNodeRef(3);
    turboprop.blade_tip_nodes[2] = this->GetArgNodeRef(4);
    turboprop.blade_tip_nodes[3] = this->GetArgNodeRef(5);
    turboprop.turbine_power_kW   = this->GetArgFloat  (6);
    turboprop.airfoil            = this->GetArgStr    (7);
    
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
    if (m_num_args > 7) { trigger.boundary_timer = this->GetArgFloat(7); }

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
    
    if (m_num_args > 5) { tie.is_invisible = (this->GetArgChar  (5) == 'i'); }
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

void Parser::ParseSlidenodes(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_SLIDENODES))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    SlideNode slidenode;
    slidenode.slide_node = _ParseNodeRef(results[1]);

    bool in_rail_node_list = true;
    Ogre::StringVector tokens = Ogre::StringUtil::split(results[2], ",");
    for (Ogre::StringVector::iterator itor = tokens.begin(); itor != tokens.end(); ++itor)
    {
        std::smatch token_results;
        if (std::regex_search(*itor, token_results, Regexes::SLIDENODES_IDENTIFY_OPTION))
        {
            in_rail_node_list = false;

            if (token_results[1].matched)
            {
                slidenode.spring_rate = STR_PARSE_REAL(token_results[2]);
            }
            else if (token_results[3].matched)
            {
                slidenode.break_force = STR_PARSE_REAL(token_results[4]);
                slidenode._break_force_set = true;
            }
            else if (token_results[5].matched)
            {
                slidenode.tolerance = STR_PARSE_REAL(token_results[6]);
            }
            else if (token_results[7].matched)
            {
                slidenode.attachment_rate = STR_PARSE_REAL(token_results[8]);
            }
            else if (token_results[9].matched)
            {
                slidenode.railgroup_id = STR_PARSE_INT(token_results[10]);
                slidenode._railgroup_id_set = true;
            }
            else if (token_results[11].matched)
            {
                slidenode.max_attachment_distance = STR_PARSE_REAL(token_results[12]);
            }
            // #13, #14 Quantity is ignored 
            else if (token_results[15].matched)
            {
                char option = token_results[16].str().at(0);
                switch (option)
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
                        this->AddMessage(line, Message::TYPE_WARNING, std::string("Ignoring invalid option: ") + option);
                        break;
                }
            }
        }
        else
        {
            if (in_rail_node_list)
            {
                slidenode.rail_node_ranges.push_back( _ParseNodeRef(*itor));
            }
            else
            {
                std::stringstream msg;
                msg << "Invalid token: '" << *itor << "', ignoring...";
                AddMessage(line, Message::TYPE_WARNING, msg.str());
            }
        }
    }
    
    m_current_module->slidenodes.push_back(slidenode);
}

void Parser::ParseShock2()
{
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
                case 'n': break; // Placeholder, does nothing.
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
            std::stringstream msg;
            msg << "Encountered node with illegal negative number: '" << node_id_num 
                << "', parsing as positive '" << node_id_num * -1 << "' for backwards compatibility. Please fix as soon as possible.";
            AddMessage(node_id_str, Message::TYPE_WARNING, msg.str());
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

void Parser::ParseDirectiveSetInertiaDefaults(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::DIRECTIVE_SET_INERTIA_DEFAULTS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    float start_delay = STR_PARSE_REAL(results[1]);
    float stop_delay = 0;
    if (results[4].matched)
    {
        stop_delay = STR_PARSE_REAL(results[4]);
    }
    if (start_delay < 0 || stop_delay < 0)
    {
        // Reset
        m_user_default_inertia = m_ror_default_inertia;
    }
    else
    {
        // Create
        m_user_default_inertia = std::shared_ptr<Inertia>(new Inertia(*m_user_default_inertia.get()));

        // Update
        m_user_default_inertia->start_delay_factor = start_delay;
        m_user_default_inertia->stop_delay_factor = stop_delay;

        if (results[7].matched)
        {
            m_user_default_inertia->start_function = results[7];

            if (results[10].matched)
            {
                m_user_default_inertia->stop_function = results[10];
            }
        }
    }

    if (results[11].matched)
    {
        std::stringstream msg;
        msg << "Illegal text after parameters: \"" << results[11] << "\", ignoring...";
        AddMessage(line, Message::TYPE_WARNING, msg.str());
    }
}

void Parser::ParseScrewprops(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_SCREWPROPS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Screwprop screwprop;

    screwprop.prop_node = _ParseNodeRef(results[1]);
    screwprop.back_node = _ParseNodeRef(results[2]);
    screwprop.top_node = _ParseNodeRef(results[3]);
    screwprop.power = STR_PARSE_REAL(results[4]);

    m_current_module->screwprops.push_back(screwprop);
}

void Parser::ParseRotatorsUnified()
{
    if (! this->CheckNumArguments(13)) { return; }

    Rotator rotator;
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
    
    if (m_current_section == File::SECTION_ROTATORS_2)
    {
        Rotator2* rotator2 = static_cast<Rotator2*>(&rotator);

        if (m_num_args > 13) { rotator2->rotating_force  = this->GetArgFloat(13); }
        if (m_num_args > 14) { rotator2->tolerance       = this->GetArgFloat(14); }
        if (m_num_args > 15) { rotator2->description     = this->GetArgStr  (15); }
        this->ParseOptionalInertia(rotator2->inertia, 16);
        if (m_num_args > 20) { rotator2->engine_coupling = this->GetArgFloat(20); }
        if (m_num_args > 21) { rotator2->needs_engine    = this->GetArgBool (21); }
        
        m_current_module->rotators_2.push_back(*rotator2);
    }
    else
    {
        this->ParseOptionalInertia(rotator.inertia, 13);
        if (m_num_args > 17) { rotator.engine_coupling = this->GetArgFloat(17); }
        if (m_num_args > 18) { rotator.needs_engine    = this->GetArgBool (18); }
        
        m_current_module->rotators.push_back(rotator);
    }
}

void Parser::ParseFileinfo()
{
    this->TokenizeCurrentLine();
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
}

void Parser::ParseRopes(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_ROPES))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Rope rope;
    rope.beam_defaults = m_user_beam_defaults;
    rope.detacher_group = m_current_detacher_group;
    rope.root_node = _ParseNodeRef(results[1]);
    rope.end_node = _ParseNodeRef(results[2]);

    if (results[4].matched)
    {
        std::string options_str = results[4].str();
        const char options_char = options_str.at(0);
        if (options_char == 'i')
        {
            rope.invisible = true;
            rope._has_invisible_set = true;
        }
        else
        {
            this->AddMessage(line, Message::TYPE_WARNING, std::string("Ignoring invalid flag: ") + options_char);
        }
    }

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

void Parser::ParseRailGroups(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_RAILGROUPS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    RailGroup railgroup;
    railgroup.id = STR_PARSE_INT(results[1]);

    // Node list 
    // Just use the split/trim/compare method 
    Ogre::StringVector tokens = Ogre::StringUtil::split(results[2].str(), ",");
    for (auto itor = tokens.begin(); itor != tokens.end(); itor++)
    {
        railgroup.node_list.push_back( _ParseNodeRef(*itor));
    }

    m_current_module->railgroups.push_back(railgroup);
}

void Parser::ParseProps()
{
    if (!this->CheckNumArguments(10)) { return; }

    Prop prop;
    prop.reference_node = this->GetArgNodeRef( 0);
    prop.x_axis_node    = this->GetArgNodeRef( 1);
    prop.y_axis_node    = this->GetArgNodeRef( 2);
    prop.offset.x       = this->GetArgFloat  ( 3);
    prop.offset.y       = this->GetArgFloat  ( 4);
    prop.offset.z       = this->GetArgFloat  ( 5);
    prop.rotation.x     = this->GetArgFloat  ( 6);
    prop.rotation.y     = this->GetArgFloat  ( 7);
    prop.rotation.z     = this->GetArgFloat  ( 8);
    prop.mesh_name      = this->GetArgStr    ( 9);

    bool is_dash = false;
         if (prop.mesh_name.find("leftmirror"  )) { prop.special = Prop::SPECIAL_MIRROR_LEFT; }
    else if (prop.mesh_name.find("rightmirror" )) { prop.special = Prop::SPECIAL_MIRROR_RIGHT; }
    else if (prop.mesh_name.find("lightbar"    )) { prop.special = Prop::SPECIAL_LIGHTBAR; }
    else if (prop.mesh_name.find("seat"        )) { prop.special = Prop::SPECIAL_DRIVER_SEAT; }
    else if (prop.mesh_name.find("seat2"       )) { prop.special = Prop::SPECIAL_DRIVER_SEAT_2; }
    else if (prop.mesh_name.find("redbeacon"   )) { prop.special = Prop::SPECIAL_REDBEACON; }
    else if (prop.mesh_name.find("pale"        )) { prop.special = Prop::SPECIAL_PALE; }
    else if (prop.mesh_name.find("spinprop"    )) { prop.special = Prop::SPECIAL_SPINPROP; }
    else if (prop.mesh_name.find("beacon"      )) { prop.special = Prop::SPECIAL_BEACON; }
    else if (prop.mesh_name.find("dashboard-rh")) { prop.special = Prop::SPECIAL_DASHBOARD_RIGHT; is_dash = true; }
    else if (prop.mesh_name.find("dashboard"   )) { prop.special = Prop::SPECIAL_DASHBOARD_LEFT;  is_dash = true; }

    if ((prop.special == Prop::SPECIAL_BEACON) && (m_num_args >= 14))
    {
        prop.special_prop_beacon.flare_material_name = this->GetArgStr(10);
        Ogre::StringUtil::trim(prop.special_prop_beacon.flare_material_name);

        prop.special_prop_beacon.color = Ogre::ColourValue(
            this->GetArgFloat(11), this->GetArgFloat(12), this->GetArgFloat(13));
    }
    else if (is_dash)
    {
        prop.special_prop_dashboard.offset = (prop.special == Prop::SPECIAL_DASHBOARD_LEFT)
            ? Ogre::Vector3(-0.67, -0.61, 0.24)
            : Ogre::Vector3( 0.67, -0.61, 0.24); // Defaults
        if (m_num_args >= 14)
        {
            prop.special_prop_dashboard.mesh_name = this->GetArgStr(10);
            prop.special_prop_dashboard.offset = Ogre::Vector3(
                this->GetArgFloat(11), this->GetArgFloat(12), this->GetArgFloat(13));
        }
        if (m_num_args >= 15)
        {
            prop.special_prop_dashboard.rotation_angle = this->GetArgFloat(14);
        }
    }

    m_current_module->props.push_back(prop);
}

void Parser::ParsePistonprops(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_PISTONPROPS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Pistonprop pistonprop;
    pistonprop.reference_node = _ParseNodeRef(results[1]);
    pistonprop.axis_node = _ParseNodeRef(results[2]);
    pistonprop.blade_tip_nodes[0] = _ParseNodeRef(results[3]);
    pistonprop.blade_tip_nodes[1] = _ParseNodeRef(results[4]);
    pistonprop.blade_tip_nodes[2] = _ParseNodeRef(results[5]);
    pistonprop.blade_tip_nodes[3] = _ParseNodeRef(results[6]);

    // Couplenode 
    if (results[9].matched)
    {
        pistonprop.couple_node = _ParseNodeRef(results[9]);
        pistonprop._couple_node_set = true;
    }

    pistonprop.turbine_power_kW = STR_PARSE_REAL(results[10]);
    pistonprop.pitch = STR_PARSE_REAL(results[11]);
    pistonprop.airfoil = results[12];

    m_current_module->pistonprops.push_back(pistonprop);

}

void Parser::ParseParticles(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_PARTICLES))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Particle particle;
    particle.emitter_node = _ParseNodeRef(results[1]);
    particle.reference_node = _ParseNodeRef(results[2]);
    particle.particle_system_name = results[3];

    m_current_module->particles.push_back(particle);
}

// Static
void Parser::_TrimTrailingComments(std::string const & line_in, std::string & line_out)
{
    // Trim trailing comment
    // We need to handle a case of lines as [keyword 1, 2, 3 ;;///// Comment!]
    int comment_start = line_in.find_first_of(";");
    if (comment_start != Ogre::String::npos)
    {
        line_out = line_in.substr(0, comment_start);
        return;
    }
    // The [//Comment] is harder - the '/' character may also be present in DESCRIPTION arguments!
    comment_start = line_in.find_last_of("/");
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
    node.detacher_group = m_current_detacher_group;

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

void Parser::ParseNodeCollision(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_NODECOLLISION))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    NodeCollision node_collision;
    node_collision.node = _ParseNodeRef(results[1]);
    node_collision.radius = STR_PARSE_REAL(results[2]);
    m_current_module->node_collisions.push_back(node_collision);
}

void Parser::ParseMinimass(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_MINIMASS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (m_definition->_minimum_mass_set)
    {
        AddMessage(line, Message::TYPE_WARNING, "Duplicate section 'minimass', ignoring...");
        return;
    }

    m_definition->minimum_mass = STR_PARSE_REAL(results[1]);
    m_definition->_minimum_mass_set = true;
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

void Parser::ParseMaterialFlareBindings(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_MATERIALFLAREBINDINGS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    MaterialFlareBinding binding;
    binding.flare_number = STR_PARSE_INT(results[1]);
    if (results[2].matched)
    {
        std::stringstream msg;
        msg << "Invalid character(s) '" << results[2] << "' after parameter ~1 'Flare index', ingoring...";
        AddMessage(line, Message::TYPE_WARNING, msg.str());
    }
    binding.material_name = results[4];
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
        managed_mat.type = (type_str == "mesh_standard")
            ? ManagedMaterial::TYPE_MESH_STANDARD
            : ManagedMaterial::TYPE_MESH_TRANSPARENT;
        
        managed_mat.diffuse_map = this->GetArgStr(2);
        
        if (m_num_args > 3) { managed_mat.specular_map = this->GetArgManagedTex(3); }
    }
    else if (type_str == "flexmesh_standard" || type_str == "flexmesh_transparent")
    {
        managed_mat.type = (type_str == "flexmesh_standard")
            ? ManagedMaterial::TYPE_FLEXMESH_STANDARD
            : ManagedMaterial::TYPE_FLEXMESH_TRANSPARENT;
            
        managed_mat.diffuse_map = this->GetArgStr(2);
        
        if (m_num_args > 3) { managed_mat.damaged_diffuse_map = this->GetArgManagedTex(3); }
        if (m_num_args > 4) { managed_mat.specular_map        = this->GetArgManagedTex(4); }
    }
    else
    {
        this->AddMessage(Message::TYPE_ERROR, type_str + " is an invalid effect");
        return;
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
    
    beam.nodes[0] = this->GetArgNodeRef(0);
    beam.nodes[1] = this->GetArgNodeRef(1);

    // Flags 
    if (m_num_args > 2)
    {
        std::string options_str = this->GetArgStr(2);
        for (auto itor = options_str.begin(); itor != options_str.end(); ++itor)
        {
                 if (*itor == 'v') { continue; } // Dummy flag
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
        beam._has_extension_break_limit = true;
        beam.extension_break_limit = this->GetArgFloat(3);        
    }

    m_current_module->beams.push_back(beam);
}

void Parser::ParseAnimator()
{
    if (! this->CheckNumArguments(4)) { return; }

    Animator animator;
    animator.inertia_defaults   = m_user_default_inertia;
    animator.beam_defaults      = m_user_beam_defaults;
    animator.detacher_group     = m_current_detacher_group;

    animator.nodes[0]           = this->GetArgNodeRef(0);
    animator.nodes[1]           = this->GetArgNodeRef(1);
    animator.lenghtening_factor = this->GetArgFloat  (2);

    // Parse options 
    // Just use the split/trim/compare method 
    Ogre::StringVector attrs = Ogre::StringUtil::split(this->GetArgStr(3), "|");

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
    if (! this->_IsCurrentModuleRoot())
    {
        this->AddMessage(Message::TYPE_WARNING, "Inline-section 'author' has global effect and should not appear in a module");
    }

    this->TokenizeCurrentLine();
    if (! this->CheckNumArguments(2)) { return; }

    Author author;
    if (m_num_args > 1) { author.type             = this->GetArgStr(1); }
    if (m_num_args > 2) { author.forum_account_id = this->GetArgInt(2); author._has_forum_account = true; }
    if (m_num_args > 3) { author.name             = this->GetArgStr(3); }
    if (m_num_args > 4) { author.email            = this->GetArgStr(4); }
    m_definition->authors.push_back(author);
}

// -------------------------------------------------------------------------- 
//	Utilities                                                                 
// -------------------------------------------------------------------------- 

std::pair<bool, Ogre::String> Parser::GetModuleName(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::DIRECTIVE_SECTION))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return std::make_pair(false, "");
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    return std::make_pair(true, results[2]);
}

void Parser::SetCurrentModule(Ogre::String const & name)
{

    std::map< Ogre::String, std::shared_ptr<File::Module> >::iterator itor
        = m_definition->modules.find(name);

    if (itor != m_definition->modules.end())
    {
        m_current_module = itor->second;
    }
    else
    {
        m_current_module = std::shared_ptr<File::Module>( new File::Module(name) );
        m_definition->modules.insert( 
                std::pair< Ogre::String, std::shared_ptr<File::Module> >(name, m_current_module) 
            );
    }
}

void Parser::RestoreRootModule()
{
    m_current_module = m_root_module;
}

void Parser::AddMessage(std::string const & line, Message::Type type, std::string const & message)
{
    // Push and then access to avoid copy-constructing strings 
    m_messages.push_back(Message());

    m_messages.back().line = line;
    m_messages.back().line_number = m_current_line_number;
    m_messages.back().message = message;
    m_messages.back().type = type;
    m_messages.back().section = m_current_section;
    m_messages.back().subsection = m_current_subsection;
    m_messages.back().module = m_current_module->name;

    switch (type)
    {
    case Message::TYPE_ERROR: 
    case Message::TYPE_FATAL_ERROR: 
        ++m_messages_num_errors; 
        break;
    case Message::TYPE_WARNING: 
        ++m_messages_num_warnings; 
        break;
    default:
        ++m_messages_num_other;
        break;
    }
}

File::Keyword Parser::IdentifyKeyword(Ogre::String const & line)
{
    // Search with correct lettercase
    std::smatch results;
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
    m_user_node_defaults = m_ror_node_defaults;
    m_current_managed_material_options = ManagedMaterialsOptions();

    m_user_beam_defaults = std::shared_ptr<BeamDefaults>(new BeamDefaults);
    m_user_beam_defaults->springiness           = DEFAULT_SPRING;       
    m_user_beam_defaults->damping_constant      = DEFAULT_DAMP;         
    m_user_beam_defaults->deformation_threshold = BEAM_DEFORM;          
    m_user_beam_defaults->breaking_threshold    = BEAM_BREAK;           
    m_user_beam_defaults->visual_beam_diameter  = DEFAULT_BEAM_DIAMETER;

    m_root_module = std::shared_ptr<File::Module>( new File::Module("_Root_") );
    m_definition->root_module = m_root_module;
    m_current_module = m_root_module;

    m_sequential_importer.Init(true); // Enabled=true

    m_messages_num_errors = 0;
    m_messages_num_warnings = 0;
    m_messages_num_other = 0;
}

void Parser::_ExitSections(Ogre::String const & line)
{
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
            AddMessage(line, Message::TYPE_WARNING, "Empty section 'camerarail', ignoring...");
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
}

void Parser::Finalize()
{
    Ogre::String line;
    _ExitSections(line);

    if (m_sequential_importer.IsEnabled())
    {
        m_sequential_importer.Process( m_definition );
    }
}

std::string Parser::ProcessMessagesToString()
{
    if (this->GetMessages().size() == 0)
    {
        std::string msg(" == Parsing done OK, nothing to report");
        return msg;
    }

    std::stringstream report;
    report << " == Parsing done, report:" << std::endl <<std::endl;

    auto itor = m_messages.begin();
    auto end  = m_messages.end();
    for (; itor != end; ++itor)
    {
        switch (itor->type)
        {
            case (RigDef::Parser::Message::TYPE_FATAL_ERROR): 
                report << "#FF3300 FATAL_ERROR #FFFFFF"; 
                break;

            case (RigDef::Parser::Message::TYPE_ERROR): 
                report << "#FF3300 ERROR #FFFFFF"; 
                break;

            case (RigDef::Parser::Message::TYPE_WARNING): 
                report << "#FFFF00 WARNING #FFFFFF"; 
                break;

            default:
                report << "INFO"; 
                break;
        }
        report << " (Section " << RigDef::File::SectionToString(itor->section) << ")" << std::endl;
        report << "\tLine (" << itor->line_number << "): " << itor->line << std::endl;
        report << "\tMessage: " << itor->message << std::endl;
    }

    return report.str();
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

void Parser::ProcessOgreStream(Ogre::DataStream* stream)
{
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
