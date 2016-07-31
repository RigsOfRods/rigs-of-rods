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

#define STR_PARSE_INT(_STR_)  Ogre::StringConverter::parseInt(_STR_)

#define STR_PARSE_REAL(_STR_) Ogre::StringConverter::parseReal(_STR_)

#define STR_PARSE_BOOL(_STR_) Ogre::StringConverter::parseBool(_STR_)

/// Parses line into "values[]" array and checks number of arguments
#define PARSE_UNSAFE(VAR_LINE, MIN_ARGS, _BLOCK_)                      \
    this->AddMessage(line, Message::TYPE_WARNING,                      \
        "Syntax check failed, parsing by legacy unsafe method...");    \
    const int min_args = MIN_ARGS;                                     \
    Ogre::StringVector values;                                         \
    if (this->_ParseArgs(VAR_LINE, values, min_args) < min_args)       \
    {                                                                  \
        std::stringstream msg;                                         \
        msg << "Too few arguments, required number is " << min_args;   \
        this->AddMessage(VAR_LINE, Message::TYPE_WARNING, msg.str());  \
    }                                                                  \
    else                                                               \
    {                                                                  \
        _BLOCK_                                                        \
    }


Parser::Parser():
    m_ror_minimass(0)
{
    // Push defaults 
    m_ror_default_inertia = std::shared_ptr<Inertia>(new Inertia);
    m_ror_beam_defaults = std::shared_ptr<BeamDefaults>(new BeamDefaults);
    m_ror_node_defaults = std::shared_ptr<NodeDefaults>(new NodeDefaults);
}

void Parser::ProcessCurrentLine()
{
    std::string line = m_current_line;

    bool line_finished = false;
    bool scan_for_keyword = true;

    // Check line type 
    std::smatch line_type_result;
    if (m_in_block_comment)
    {
        if (std::regex_match(line, Regexes::CHECK_BLOCK_COMMENT_END))
        {
            m_in_block_comment = false;
        }
        line_finished = true;
    }
    else if (m_in_description_section)
    {
        if (std::regex_match(line, Regexes::CHECK_END_DESCRIPTION))
        {
            m_in_description_section = false;
        }
        else
        {
            m_definition->description.push_back(line);
        }
        line_finished = true;
    }
    else
    {
        if (std::regex_search(line, line_type_result, Regexes::IDENTIFY_LINE_TYPE))
        {
            if (line_type_result[1].matched) // Block comment start 
            {
                m_in_block_comment = true;
                line_finished = true;
            }
            else if (line_type_result[2].matched || line_type_result[3].matched || line_type_result[4].matched) // Comment line || blank line 
            {
                m_num_contiguous_blank_lines++;
                line_finished = true;
            }
            else // Line has content 
            {
                m_num_contiguous_blank_lines = 0;
            }
        }
    }

    // Continue? 
    if (line_finished)
    {
        return;
    }

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
                ParseDirectiveAddAnimation(line);
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
                ParseAntiLockBrakes(line);
                line_finished = true;
                break;

            case (File::KEYWORD_AXLES):
                new_section = File::SECTION_AXLES;
                line_finished = true;
                break;

            case (File::KEYWORD_AUTHOR):
                ParseAuthor(line);
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
                ParseDirectiveDetacherGroup(line);
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
                ParseExtCamera(line);
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
                if (! current_module_is_root)
                {
                    AddMessage(line, Message::TYPE_WARNING, "Inline-section 'fileinfo' has global effect and should not appear in a module");
                }
                ParseFileinfo(line);
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
                ParseDirectiveFlexbodyCameraMode(line);
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
                ParseDirectivePropCameraMode(line);
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
                ParseDirectiveSetBeamDefaults(line);
                line_finished = true;
                break;

            case (File::KEYWORD_SET_BEAM_DEFAULTS_SCALE):
                ParseDirectiveSetBeamDefaultsScale(line);
                line_finished = true;
                break;

            case (File::KEYWORD_SET_COLLISION_RANGE):
                ParseSetCollisionRange(line);
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
                ParseSlopeBrake(line);
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
                ParseSpeedLimiter(line);
                line_finished = true;
                break;

            case (File::KEYWORD_SUBMESH_GROUNDMODEL):
                ParseSubmeshGroundModel(line);
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
                ParseTractionControl(line);
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
            ParseAirbrakes(line);
            line_finished = true;
            break;

        case (File::SECTION_ANIMATORS):
            ParseAnimator(line);
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
            ParseBeams(line);
            line_finished = true;
            break;

        case (File::SECTION_BRAKES):
            ParseBrakes(line);
            line_finished = true;
            break;

        case (File::SECTION_CAMERAS):
            ParseCameras(line);
            line_finished = true;
            break;

        case (File::SECTION_CAMERA_RAIL):
            ParseCameraRails(line);
            line_finished = true;
            break;

        case (File::SECTION_CINECAM):
            ParseCinecam(line);
            line_finished = true;
            break;

        case (File::SECTION_COMMANDS):
            ParseCommand(line);
            line_finished = true;
            break;

        case (File::SECTION_COMMANDS_2):
            ParseCommand2(line);
            line_finished = true;
            break;

        case (File::SECTION_COLLISION_BOXES):
            ParseCollisionBox(line);
            line_finished = true;
            break;

        case (File::SECTION_CONTACTERS):
            ParseContacter(line);
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
            ParseEngturbo(line);
            line_finished = true;
            break;

        case (File::SECTION_EXHAUSTS):
            ParseExhaust(line);
            line_finished = true;
            break;

        case (File::SECTION_FIXES):
            ParseFixes(line);
            line_finished = true;
            break;

        case (File::SECTION_FLARES):
            ParseFlare(line);
            line_finished = true;
            break;

        case (File::SECTION_FLARES_2):
            ParseFlare2(line);
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
            ParseFusedrag(line);
            line_finished = true;
            break;

        case (File::SECTION_GLOBALS):
            ParseGlobals(line);
            line_finished = true;
            break;

        case (File::SECTION_GUI_SETTINGS):
            ParseGuiSettings(line);
            line_finished = true;
            break;

        case (File::SECTION_HELP):
            ParseHelp(line);
            line_finished = true;
            break;

        case (File::SECTION_HOOKS):
            ParseHook(line);
            line_finished = true;
            break;

        case (File::SECTION_HYDROS):
            ParseHydros(line);
            line_finished = true;
            break;

        case (File::SECTION_LOCKGROUPS):
            ParseLockgroups(line);
            line_finished = true;
            break;

        case (File::SECTION_MANAGED_MATERIALS):
            ParseManagedMaterials(line);
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
            ParseProps(line);
            line_finished = true;
            break;

        case (File::SECTION_RAILGROUPS):
            ParseRailGroups(line);
            line_finished = true;
            break;

        case (File::SECTION_ROPABLES):
            ParseRopables(line);
            line_finished = true;
            break;

        case (File::SECTION_ROPES):
            ParseRopes(line);
            line_finished = true;
            break;

        case (File::SECTION_ROTATORS):
            ParseRotators(line);
            line_finished = true;
            break;

        case (File::SECTION_ROTATORS_2):
            ParseRotators2(line);
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
            ParseSoundsources(line);
            line_finished = true;
            break;

        case (File::SECTION_SOUNDSOURCES2):
            ParseSoundsources2(line);
            line_finished = true;
            break;

        case (File::SECTION_SUBMESH):
            ParseSubmesh(line);
            line_finished = true;
            break;

        case (File::SECTION_TIES):
            ParseTies(line);
            line_finished = true;
            break;

        case (File::SECTION_TORQUE_CURVE):
            ParseTorqueCurve(line);
            line_finished = true;
            break;

        case (File::SECTION_TRIGGERS):
            ParseTriggers(line);
            line_finished = true;
            break;

        case (File::SECTION_TURBOJETS):
            ParseTurbojets(line);
            line_finished = true;
            break;

        case (File::SECTION_TURBOPROPS):
            ParseTurboprops(line);
            line_finished = true;
            break;

        case (File::SECTION_VIDEO_CAMERA):
            ParseVideoCamera(line);
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
            ParseWing(line);
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

void Parser::ParseWing(Ogre::String const & line)
{
    
    std::smatch results;
    try
    {
        if (! std::regex_search(line, results, Regexes::SECTION_WINGS))
        {
            AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
            return;
        }
    }
    catch(std::runtime_error e)
    {
        std::stringstream msg;
        msg << "Wing not parsed, internal parser failure occured, message: " << e.what();
        AddMessage(line, Message::TYPE_FATAL_ERROR, msg.str());
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Wing wing;

    for (unsigned int i = 0; i < 8; i++)
    {
        wing.nodes[i] = _ParseNodeRef(results[i + 1]);
    }

    for (unsigned int i = 0; i < 8; i++)
    {
        wing.tex_coords[i] = STR_PARSE_REAL(results[i + 9]);
    }

    if (results[17].matched)
    {
        std::string control_surface_str = results[18];
        wing.control_surface = Wing::Control(control_surface_str.at(0));

        if (results[19].matched)
        {
            wing.chord_point = STR_PARSE_REAL(results[20]);

            if (results[21].matched)
            {
                wing.min_deflection = STR_PARSE_REAL(results[22]);

                if (results[23].matched)
                {
                    wing.max_deflection = STR_PARSE_REAL(results[24]);

                    if (results[25].matched)
                    {
                        wing.airfoil = results[26];

                        if (results[27].matched)
                        {
                            wing.efficacy_coef = STR_PARSE_REAL(results[28]);
                        }
                    }
                }
            }
        }
    }

    m_current_module->wings.push_back(wing);
}

void Parser::ParseSetCollisionRange(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_SET_COLLISION_RANGE))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (results[1].matched)
    {
        m_definition->collision_range = STR_PARSE_REAL(results[1]);
        m_definition->_collision_range_set = true;
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

void Parser::ParseTractionControl(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_TRACTION_CONTROL))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    TractionControl traction_control;
    traction_control.regulation_force = STR_PARSE_REAL(results[1]);
    traction_control.wheel_slip = STR_PARSE_REAL(results[2]);

    if (results[3].matched)
    {
        traction_control.fade_speed = STR_PARSE_REAL(results[4]);

        if (results[5].matched)
        {
            traction_control.pulse_per_sec = STR_PARSE_REAL(results[6]);

            if (results[7].matched)
            {
                // parse mode 
                Ogre::StringVector tokens = Ogre::StringUtil::split(results[8], "&");
                Ogre::StringVector::iterator iter = tokens.begin();
                for ( ; iter != tokens.end(); iter++)
                {
                    std::smatch results;
                    if (! std::regex_search(*iter, results, Regexes::TRACTION_CONTROL_MODE))
                    {
                        std::string invalid_keyword = *iter;
                        AddMessage(line, Message::TYPE_WARNING, "Ignoring invalid mode attribute: \"" + invalid_keyword + "\"");
                        continue;
                    }
                    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

                    if (results[1].matched)
                    {
                        BITMASK_SET_1(traction_control.mode, TractionControl::MODE_ON);
                        BITMASK_SET_0(traction_control.mode, TractionControl::MODE_OFF);
                    }
                    else if (results[2].matched)
                    {
                        BITMASK_SET_1(traction_control.mode, TractionControl::MODE_OFF);
                        BITMASK_SET_0(traction_control.mode, TractionControl::MODE_ON);
                    }
                    else if (results[3].matched)
                    {
                        BITMASK_SET_1(traction_control.mode, TractionControl::MODE_NO_DASHBOARD);
                    }
                    else if (results[4].matched)
                    {
                        BITMASK_SET_1(traction_control.mode, TractionControl::MODE_NO_TOGGLE);
                    }
                }
            }
        }
    }

    if (m_current_module->traction_control != nullptr)
    {
        AddMessage(line, Message::TYPE_WARNING, "Multiple inline-sections 'TractionControl' in a module, using last one ...");
    }
    m_current_module->traction_control = std::shared_ptr<TractionControl>( new TractionControl(traction_control) );
}

void Parser::ParseSubmeshGroundModel(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_SUBMESH_GROUNDMODEL))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid format of inline-section 'submesh_groundmodel', ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    m_current_module->submeshes_ground_model_name = results[1];
}

void Parser::ParseSpeedLimiter(Ogre::String const & line)
{
    if (m_current_module->speed_limiter != nullptr)
    {
        AddMessage(line, Message::TYPE_WARNING, "Multiple inline-sections 'speedlimiter' in a module, using last one ...");
    }
    m_current_module->speed_limiter = std::shared_ptr<SpeedLimiter>( new SpeedLimiter() );

    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_SPEEDLIMITER))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    m_current_module->speed_limiter->max_speed = STR_PARSE_REAL(results[1]);
}

void Parser::ParseSlopeBrake(Ogre::String const & line)
{
    if (m_current_module->slope_brake != nullptr)
    {
        AddMessage(line, Message::TYPE_WARNING, "Multiple inline-sections 'SlopeBrake' in a module, using last one ...");
    }
    m_current_module->slope_brake = std::shared_ptr<SlopeBrake>( new SlopeBrake() );

    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_SLOPE_BRAKE))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (results[1].matched)
    {
        m_current_module->slope_brake->regulating_force = STR_PARSE_REAL(results[2]);
        
        if (results[3].matched)
        {
            m_current_module->slope_brake->attach_angle = STR_PARSE_REAL(results[4]);

            if (results[5].matched)
            {
                m_current_module->slope_brake->release_angle = STR_PARSE_REAL(results[6]);
            }
        }
    }
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

void Parser::ParseDirectiveSetBeamDefaults(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::DIRECTIVE_SET_BEAM_DEFAULTS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    m_user_beam_defaults = std::shared_ptr<BeamDefaults>( new BeamDefaults(*m_user_beam_defaults) );

    // What's the state of "enable_advanced_deformation" feature at this point?
    // Directive "enable_advanced_deformation" alters the effects of BeamDefaults
    // Since the old parser worked on-the-fly, only BeamDefaults defined after the directive were affected
    
    m_user_beam_defaults->_enable_advanced_deformation = m_definition->enable_advanced_deformation;

    //The "_enable_advanced_deformation" must only be aplied to BeamDefaults supplied by users, 
    //    not on the initial defaults used by parser.
    
    m_user_beam_defaults->_is_user_defined = true;

    // Springiness 
    float default_spring = STR_PARSE_REAL(results[1]);
    if (default_spring < 0) // NULL value => reset to default 
    {
        m_user_beam_defaults->springiness = m_ror_beam_defaults->springiness;
        BITMASK_SET_0(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_SPRINGINESS);
    }
    else
    {
        m_user_beam_defaults->springiness = default_spring;
        BITMASK_SET_1(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_SPRINGINESS);
    }

    // Damping constant 
    if (results[2].matched)
    {
        float default_damp = STR_PARSE_REAL(results[3]);
        if (default_damp < 0) // NULL value => reset to default 
        {
            m_user_beam_defaults->damping_constant = m_ror_beam_defaults->damping_constant;
            BITMASK_SET_0(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_DAMPING_CONSTANT);
        }
        else
        {
            m_user_beam_defaults->damping_constant = default_damp;
            BITMASK_SET_1(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_DAMPING_CONSTANT);
        }
    }
    else
    {
        return;
    }

    // Deformation threshold constant 
    if (results[4].matched)
    {
        float default_deform = STR_PARSE_REAL(results[5]);
        if (default_deform < 0) // NULL value => reset to default 
        {
            m_user_beam_defaults->deformation_threshold_constant = m_ror_beam_defaults->deformation_threshold_constant;
            BITMASK_SET_0(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_DEFORM_THRESHOLD_CONSTANT);
        }
        else
        {
            m_user_beam_defaults->deformation_threshold_constant = default_deform;
            BITMASK_SET_1(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_DEFORM_THRESHOLD_CONSTANT);
        }
    }
    else
    {
        return;
    }

    // Breaking threshold constant 
    if (results[6].matched)
    {
        float default_break = STR_PARSE_REAL(results[7]);
        if (default_break < 0) // NULL value => reset to default 
        {
            m_user_beam_defaults->breaking_threshold_constant = m_ror_beam_defaults->breaking_threshold_constant;
            BITMASK_SET_0(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_DEFORM_THRESHOLD_CONSTANT);
        }
        else
        {
            m_user_beam_defaults->breaking_threshold_constant = default_break;
            BITMASK_SET_1(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_BREAK_THRESHOLD_CONSTANT);
        }
    }
    else
    {
        return;
    }

    // Beam diameter 
    if (results[8].matched)
    {
        float default_beam_diameter = STR_PARSE_REAL(results[9]);
        if (default_beam_diameter < 0) // NULL value => reset to default 
        {
            m_user_beam_defaults->visual_beam_diameter = m_ror_beam_defaults->visual_beam_diameter;
            BITMASK_SET_0(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_BEAM_DIAMETER);
        }
        else
        {
            m_user_beam_defaults->visual_beam_diameter = default_beam_diameter;
            BITMASK_SET_1(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_BEAM_DIAMETER);
        }
    }
    else
    {
        return;	
    }

    if (results[10].matched)
    {
        m_user_beam_defaults->beam_material_name = results[11];
    }
    else
    {
        return;
    }

    // Plastic deformation coefficient 
    if (results[12].matched)
    {
        float default_plastic_coefficient = STR_PARSE_REAL(results[13]);
        if (default_plastic_coefficient < 0) // NULL value => reset to default 
        {
            m_user_beam_defaults->plastic_deformation_coefficient = m_ror_beam_defaults->plastic_deformation_coefficient;
            BITMASK_SET_0(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_PLASTIC_DEFORM_COEFFICIENT);
        }
        else
        {
            m_user_beam_defaults->plastic_deformation_coefficient = default_plastic_coefficient;
            BITMASK_SET_1(m_user_beam_defaults->_user_specified_fields, BeamDefaults::PARAM_PLASTIC_DEFORM_COEFFICIENT);
        }
    }

    return;
}

void Parser::ParseDirectivePropCameraMode(Ogre::String const & line)
{
    assert(m_current_module != nullptr);

    if (m_current_module->props.size() == 0)
    {
        AddMessage(line, Message::TYPE_ERROR, "Directive 'prop_camera_mode' found but no 'prop' defined, ignoring...");
        return;
    }

    std::smatch results;
    if (! std::regex_search(line, results, Regexes::DIRECTIVE_PROP_CAMERA_MODE))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid format of directive 'prop_camera_mode', ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    _ParseCameraSettings(m_current_module->props.back().camera_settings, results[1]);
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

void Parser::ParseHook(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_HOOKS))
    {
        this->AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex.

    Hook hook;
    hook.node = this->_ParseNodeRef(results[1]);

    Ogre::StringVector tokens = Ogre::StringUtil::split(results[2], ",");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        std::smatch results;
        Ogre::String entry = *iter;
        Ogre::StringUtil::trim(entry);
        if (! std::regex_search(entry, results, Regexes::HOOKS_OPTIONS))
        {
            this->AddMessage(*iter, Message::TYPE_ERROR, "Invalid option of 'hooks', ignoring...");
            return;
        }

        if (! results[1].matched) 
        {
            this->AddMessage(*iter, Message::TYPE_ERROR, "Invalid option of 'hooks', ignoring...");
            return;
        }
        else if (results[2].matched) 
        {
            BITMASK_SET_1(hook.flags, Hook::FLAG_SELF_LOCK);
        }
        else if (results[3].matched) 
        {
            BITMASK_SET_1(hook.flags, Hook::FLAG_AUTO_LOCK);
        }
        else if (results[4].matched) 
        {
            BITMASK_SET_1(hook.flags, Hook::FLAG_NO_DISABLE);
        }
        else if (results[5].matched) 
        {
            BITMASK_SET_1(hook.flags, Hook::FLAG_NO_ROPE);
        }
        else if (results[6].matched) 
        {
            BITMASK_SET_1(hook.flags, Hook::FLAG_VISIBLE);
        }
        else if (results[9].matched) 
        {
            hook.option_hook_range = STR_PARSE_REAL(results[9]);
        }
        else if (results[12].matched) 
        {
            hook.option_max_force = STR_PARSE_REAL(results[12]);
        }
        else if (results[15].matched) 
        {
            hook.option_hookgroup = STR_PARSE_INT(results[15]);
        }
        else if (results[18].matched) 
        {
            hook.option_lockgroup = STR_PARSE_INT(results[18]);
        }
        else if (results[21].matched) 
        {
            hook.option_timer = STR_PARSE_REAL(results[21]);
        }
        else if (results[24].matched) 
        {
            hook.option_minimum_range_meters = STR_PARSE_REAL(results[24]);
        }
        else if (results[27].matched) 
        {
            hook.option_speed_coef = STR_PARSE_REAL(results[27]);
        }
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

void Parser::ParseGuiSettings(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_GUISETTINGS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (results[3].matched)
    {
        m_current_module->gui_settings->tacho_material = results[3];
    }
    else if (results[5].matched)
    {
        m_current_module->gui_settings->speedo_material = results[5];
    }
    else if (results[7].matched)
    {
        m_current_module->gui_settings->speedo_highest_kph = STR_PARSE_INT(results[7]);
    }
    else if (results[9].matched)
    {
        int input = STR_PARSE_INT(results[9]);
        if ( input != 0 && input != 1 ) // This is backwards compatible, legacy check was: 1. parse VALUE_STR as int 2. compare (VALUE_INT == 1)
        {
            std::stringstream msg;
            msg << "Param 'Use Max RPM' has invalid value '" << results[9] << "'. Valid values are '0' (no) or '1' (yes). Parsing as '0' (no). Please fix.";
            AddMessage(line, Message::TYPE_WARNING, msg.str());
        }
        m_current_module->gui_settings->use_max_rpm = (input == 1);
    }
    else if (results[11].matched)
    {
        m_current_module->gui_settings->help_material = results[11];
    }
    else if (results[12].matched)
    {
        if (results[14].matched)
        {
            m_current_module->gui_settings->interactive_overview_map_mode = GuiSettings::MAP_MODE_OFF;
        }
        else if (results[15].matched)
        {
            m_current_module->gui_settings->interactive_overview_map_mode = GuiSettings::MAP_MODE_SIMPLE;
        }
        else if (results[16].matched)
        {
            m_current_module->gui_settings->interactive_overview_map_mode = GuiSettings::MAP_MODE_ZOOM;
        }
    }
    else if (results[18].matched)
    {
        m_current_module->gui_settings->dashboard_layouts.push_back(results[18]);
    }
    else if (results[20].matched)
    {
        m_current_module->gui_settings->rtt_dashboard_layouts.push_back(results[20]);
    }
    else
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
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

void Parser::ParseFusedrag(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_FUSEDRAG))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Fusedrag fusedrag;
    fusedrag.front_node = _ParseNodeRef(results[1]);
    fusedrag.rear_node  = _ParseNodeRef(results[3]);

    if (results[6].matched)
    {
        fusedrag.approximate_width = STR_PARSE_REAL(results[6]);
        fusedrag.airfoil_name = results[8];

        m_current_module->fusedrag.push_back(fusedrag);
    }
    else if (results[9].matched)
    {
        fusedrag.autocalc = true;

        if (results[12].matched)
        {
            fusedrag.area_coefficient = STR_PARSE_REAL(results[12]);

            if (results[15].matched)
            {
                fusedrag.airfoil_name = results[15];
            }
        }

        m_current_module->fusedrag.push_back(fusedrag);
    }
    else
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
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

void Parser::ParseDirectiveFlexbodyCameraMode(Ogre::String const & line)
{
    if (m_last_flexbody == nullptr)
    {
        AddMessage(line, Message::TYPE_ERROR, "No flexbody to update, ignoring...");
        return;
    }

    std::smatch results;
    if (! std::regex_search(line, results, Regexes::DIRECTIVE_FLEXBODY_CAMERA_MODE))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    _ParseCameraSettings(m_last_flexbody->camera_settings, results[1]);
}

unsigned int Parser::_ParseCabOptions(Ogre::String const & options_str)
{
    unsigned int cab_options = 0;
    for (unsigned int i = 0; i < options_str.length(); i++)
    {
        switch (options_str.at(i))
        {
        case 'c':
            cab_options |= Cab::OPTION_c_CONTACT;
            break;
        case 'b':
            cab_options |= Cab::OPTION_b_BUOYANT;
            break;
        case 'D':
            cab_options |= (Cab::OPTION_c_CONTACT | Cab::OPTION_b_BUOYANT);
            break;
        case 'p':
            cab_options |= Cab::OPTION_p_10xTOUGHER;
            break;
        case 'u':
            cab_options |= Cab::OPTION_u_INVULNERABLE;
            break;
        case 'F':
            cab_options |= (Cab::OPTION_p_10xTOUGHER | Cab::OPTION_b_BUOYANT);
            break;
        case 'S':
            cab_options |= (Cab::OPTION_u_INVULNERABLE | Cab::OPTION_b_BUOYANT);
            break;
        case 'n':
            break; // Placeholder, does nothing 

        default:
            std::stringstream msg;
            msg << "Subsection 'submesh/cab': Invalid option '" << options_str.at(i) << "', ignoring...";
            this->AddMessage(options_str, Message::TYPE_WARNING, msg.str());
            break;
        }
    }
    return cab_options;
}

bool Parser::_TryParseCab(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SUBMESH_SUBSECTION_CAB))
    {
        return false;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Cab cab;
    cab.nodes[0] = _ParseNodeRef(results[1]);
    cab.nodes[1] = _ParseNodeRef(results[3]);
    cab.nodes[2] = _ParseNodeRef(results[5]);

    if (results[6].matched)
    {
        cab.options = this->_ParseCabOptions(results[8].str());
    }

    m_current_submesh->cab_triangles.push_back(cab);
    return true;
}

void Parser::ParseSubmeshUnsafe(Ogre::String const & line)
{
    if (m_current_subsection == File::SUBSECTION__SUBMESH__CAB)
    {
        PARSE_UNSAFE(line, 3,
        {
            Cab cab;
            cab.nodes[0] = this->_ParseNodeRef(values[0]);
            cab.nodes[1] = this->_ParseNodeRef(values[1]);
            cab.nodes[2] = this->_ParseNodeRef(values[2]);
            if (values.size() > 3)
            {
                cab.options = this->_ParseCabOptions(values[3]);
            }

            m_current_submesh->cab_triangles.push_back(cab);
        });
    }
    else if (m_current_subsection == File::SUBSECTION__SUBMESH__TEXCOORDS)
    {
        PARSE_UNSAFE(line, 3,
        {
            Texcoord texcoord;
            texcoord.node = this->_ParseNodeRef(values[0]);
            texcoord.u    =      STR_PARSE_REAL(values[1]);
            texcoord.v    =      STR_PARSE_REAL(values[2]);

            m_current_submesh->texcoords.push_back(texcoord);
        });
    }
}

void Parser::ParseSubmesh(Ogre::String const & line)
{
    if (m_current_subsection == File::SUBSECTION__SUBMESH__CAB)
    {
        if (!this->_TryParseCab(line))
        {
            this->ParseSubmeshUnsafe(line);
        }
    }
    else if (m_current_subsection == File::SUBSECTION__SUBMESH__TEXCOORDS)
    {
        std::smatch results;
        if (! std::regex_search(line, results, Regexes::SUBMESH_SUBSECTION_TEXCOORDS))
        {
            this->ParseSubmeshUnsafe(line);
            return;
        }
        // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

        Texcoord texcoord;
        texcoord.node = _ParseNodeRef(results[1]);
        texcoord.u = STR_PARSE_REAL(results[2]);
        texcoord.v = STR_PARSE_REAL(results[3]);

        m_current_submesh->texcoords.push_back(texcoord);
    }
    else if (this->_TryParseCab(line))
    {
        AddMessage(line, Message::TYPE_WARNING, "Section submesh has no subsection defined, but subsequent line matches 'cab' entry. Parsed as 'cab'.");
    }
    else
    {
        AddMessage(line, Message::TYPE_ERROR, "Section submesh has no subsection defined, line not parsed.");
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

void Parser::ParseFlare2(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_FLARES2))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring it...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Flare2 flare2;
    flare2.reference_node  = this->_ParseNodeRef(results[ 1]);
    flare2.node_axis_x     = this->_ParseNodeRef(results[ 3]);
    flare2.node_axis_y     = this->_ParseNodeRef(results[ 5]);
    flare2.offset.x             = STR_PARSE_REAL(results[ 7]);
    flare2.offset.y             = STR_PARSE_REAL(results[ 9]);
    flare2.offset.z             = STR_PARSE_REAL(results[11]);

    if (results[14].matched)
    {
        flare2.type = Flare2::Type(results[14].str().at(0));

        if (results[17].matched)
        {
            flare2.control_number = Flare2::Type(STR_PARSE_INT(results[17]));

            if (results[20].matched)
            {
                flare2.blink_delay_milis = STR_PARSE_INT(results[20]);

                if (results[23].matched)
                {
                    flare2.size = STR_PARSE_REAL(results[23]);

                    if (results[26].matched)
                    {
                        flare2.material_name = results[26];
                    }
                }
            }
        }
    }
    
    m_current_module->flares_2.push_back(flare2);
}

void Parser::ParseFlare(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_FLARES))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex.

    Flare2 flare;
    flare.reference_node = _ParseNodeRef(results[1]);
    flare.node_axis_x    = _ParseNodeRef(results[3]);
    flare.node_axis_y    = _ParseNodeRef(results[5]);

    flare.offset.x = STR_PARSE_REAL(results[7]);
    flare.offset.y = STR_PARSE_REAL(results[9]);

    if (results[12].matched)
    {
        char in = results[12].str().at(0);
        if (in != 'f' && in != 'b' && in != 'l' && in != 'r' && in != 'R' && in != 'u')
        {
            std::stringstream msg;
            msg << "Invalid flare type '" << in << "', falling back to type 'f' (front light)...";
            AddMessage(line, Message::TYPE_WARNING, msg.str());

            in = 'f';
        }
        flare.type = Flare2::Type(in);

        if (results[15].matched)
        {
            flare.control_number = Flare2::Type(STR_PARSE_INT(results[15]));

            if (results[18].matched)
            {
                flare.blink_delay_milis = STR_PARSE_INT(results[18]);

                if (results[21].matched)
                {
                    flare.size = STR_PARSE_REAL(results[21]);

                    if (results[24].matched)
                    {
                        flare.material_name = results[24];
                    }
                }
            }
        }
    }
    
    m_current_module->flares_2.push_back(flare);
}

void Parser::ParseFixes(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::NODE_LIST))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    m_current_module->fixes.push_back(_ParseNodeRef(results[1]));
}

void Parser::ParseExtCamera(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_EXTCAMERA))
    {
        AddMessage(line, Message::TYPE_ERROR, "Inline-section 'ext_camera' has incorrect format, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (m_current_module->ext_camera == nullptr)
    {
        m_current_module->ext_camera = std::shared_ptr<RigDef::ExtCamera>( new RigDef::ExtCamera() );
    }

    if (results[2].matched)
    {
        m_current_module->ext_camera->mode = ExtCamera::MODE_CLASSIC;
    }
    else if (results[3].matched)
    {
        m_current_module->ext_camera->mode = ExtCamera::MODE_CINECAM;
    }
    if (results[4].matched)
    {
        m_current_module->ext_camera->mode = ExtCamera::MODE_NODE;
        m_current_module->ext_camera->node = _ParseNodeRef(results[6]);
    }
}

void Parser::ParseExhaust(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_EXHAUSTS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Exhaust exhaust;
    exhaust.reference_node = _ParseNodeRef(results[1]);
    exhaust.direction_node = _ParseNodeRef(results[2]);
    
    if (results[3].matched)
    {
        // #4 Unused param 

        if (results[5].matched)
        {
            exhaust.material_name = results[6];
        }
    }

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

void Parser::ParseDirectiveDetacherGroup(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::DIRECTIVE_DETACHER_GROUP))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid directive 'detacher_group', ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (results[3].matched)
    {
        m_current_detacher_group = 0;
    }
    else if (results[2].matched)
    {
        m_current_detacher_group = STR_PARSE_INT(results[2]);
    }
    else
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid directive 'detacher_group', ignoring...");
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

void Parser::_ParseDirectiveAddAnimationMode(Animation & animation, Ogre::String mode_string)
{
    unsigned int mode = 0;

    Ogre::StringVector tokens = Ogre::StringUtil::split(mode_string, "|");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        std::smatch results;
        if (! std::regex_search(*iter, results, Regexes::IDENTIFY_ADD_ANIMATION_MODE))
        {
            AddMessage(*iter, Message::TYPE_ERROR, "Invalid mode for directive 'add_animation', ignoring...");
            return;
        }
        // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

        if (results[2].matched)
        {
            mode |= Animation::MODE_ROTATION_X;
        }
        else if (results[3].matched)
        {
            mode |= Animation::MODE_ROTATION_Y;
        }
        else if (results[4].matched)
        {
            mode |= Animation::MODE_ROTATION_Z;
        }
        else if (results[5].matched)
        {
            mode |= Animation::MODE_OFFSET_X;
        }
        else if (results[6].matched)
        {
            mode |= Animation::MODE_OFFSET_Y;
        }
        else if (results[7].matched)
        {
            mode |= Animation::MODE_OFFSET_Z;
        }
    }

    animation.mode = mode;
}

void Parser::_ParseDirectiveAddAnimationSource(Animation & animation, Ogre::String source_string)
{
    // Test event (must be solitary) 
    Ogre::StringUtil::trim(source_string);
    if (source_string == "event")
    {
        BITMASK_SET_1(animation.source, Animation::SOURCE_EVENT);
        return;
    }

    // Test other flags (can be combined) 
    Ogre::StringVector tokens = Ogre::StringUtil::split(source_string, "|");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        std::smatch results;
        if (! std::regex_search(*iter, results, Regexes::IDENTIFY_ADD_ANIMATION_SOURCE))
        {
            AddMessage(*iter, Message::TYPE_ERROR, "Invalid source for directive 'add_animation', ignoring...");
            return;
        }
        // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

        // Boolean options 
        if (results[2].matched)
        {
            animation.source |= Animation::SOURCE_AIRSPEED;
        }
        else if (results[3].matched)
        {
            animation.source |= Animation::SOURCE_VERTICAL_VELOCITY;
        }
        else if (results[4].matched)
        {
            animation.source |= Animation::SOURCE_ALTIMETER_100K;
        }
        else if (results[5].matched)
        {
            animation.source |= Animation::SOURCE_ALTIMETER_10K;
        }
        else if (results[6].matched)
        {
            animation.source |= Animation::SOURCE_ALTIMETER_1K;
        }
        else if (results[7].matched)
        {
            animation.source |= Animation::SOURCE_ANGLE_OF_ATTACK;
        }
        else if (results[8].matched)
        {
            animation.source |= Animation::SOURCE_FLAP;
        }
        else if (results[9].matched)
        {
            animation.source |= Animation::SOURCE_AIR_BRAKE;
        }
        else if (results[10].matched)
        {
            animation.source |= Animation::SOURCE_ROLL;
        }
        else if (results[11].matched)
        {
            animation.source |= Animation::SOURCE_PITCH;
        }
        else if (results[12].matched)
        {
            animation.source |= Animation::SOURCE_BRAKES;
        }
        else if (results[13].matched)
        {
            animation.source |= Animation::SOURCE_ACCEL;
        }
        else if (results[14].matched)
        {
            animation.source |= Animation::SOURCE_CLUTCH;
        }
        else if (results[15].matched)
        {
            animation.source |= Animation::SOURCE_SPEEDO;
        }
        else if (results[16].matched)
        {
            animation.source |= Animation::SOURCE_TACHO;
        }
        else if (results[17].matched)
        {
            animation.source |= Animation::SOURCE_TURBO;
        }
        else if (results[18].matched)
        {
            animation.source |= Animation::SOURCE_PARKING;
        }
        else if (results[19].matched)
        {
            animation.source |= Animation::SOURCE_SHIFT_LEFT_RIGHT;
        }
        else if (results[20].matched)
        {
            animation.source |= Animation::SOURCE_SHIFT_BACK_FORTH;
        }
        else if (results[21].matched)
        {
            animation.source |= Animation::SOURCE_SEQUENTIAL_SHIFT;
        }
        else if (results[22].matched)
        {
            animation.source |= Animation::SOURCE_SHIFTERLIN;
        }
        else if (results[23].matched)
        {
            animation.source |= Animation::SOURCE_TORQUE;
        }
        else if (results[24].matched)
        {
            animation.source |= Animation::SOURCE_HEADING;
        }
        else if (results[25].matched)
        {
            animation.source |= Animation::SOURCE_DIFFLOCK;
        }
        else if (results[26].matched)
        {
            animation.source |= Animation::SOURCE_BOAT_RUDDER;
        }
        else if (results[27].matched)
        {
            animation.source |= Animation::SOURCE_BOAT_THROTTLE;
        }
        else if (results[28].matched)
        {
            animation.source |= Animation::SOURCE_STEERING_WHEEL;
        }
        else if (results[29].matched)
        {
            animation.source |= Animation::SOURCE_AILERON;
        }
        else if (results[30].matched)
        {
            animation.source |= Animation::SOURCE_ELEVATOR;
        }
        else if (results[31].matched)
        {
            animation.source |= Animation::SOURCE_AIR_RUDDER;
        }
        else if (results[32].matched)
        {
            animation.source |= Animation::SOURCE_PERMANENT;
        }
        else if (results[33].matched)
        {
            Animation::MotorSource motor_source;
            motor_source.source = Animation::MotorSource::SOURCE_AERO_THROTTLE;
            motor_source.motor = STR_PARSE_INT(results[34]);
            animation.motor_sources.push_back(motor_source);
        }
        else if (results[35].matched)
        {
            Animation::MotorSource motor_source;
            motor_source.source = Animation::MotorSource::SOURCE_AERO_RPM;
            motor_source.motor = STR_PARSE_INT(results[36]);
            animation.motor_sources.push_back(motor_source);
        }
        else if (results[37].matched)
        {
            Animation::MotorSource motor_source;
            motor_source.source = Animation::MotorSource::SOURCE_AERO_TORQUE;
            motor_source.motor = STR_PARSE_INT(results[38]);
            animation.motor_sources.push_back(motor_source);
        }
        else if (results[39].matched)
        {
            Animation::MotorSource motor_source;
            motor_source.source = Animation::MotorSource::SOURCE_AERO_PITCH;
            motor_source.motor = STR_PARSE_INT(results[40]);
            animation.motor_sources.push_back(motor_source);
        }
        else if (results[41].matched)
        {
            Animation::MotorSource motor_source;
            motor_source.source = Animation::MotorSource::SOURCE_AERO_STATUS;
            motor_source.motor = STR_PARSE_INT(results[42]);
            animation.motor_sources.push_back(motor_source);
        }
    }
}

void Parser::ParseDirectiveAddAnimation(Ogre::String const & line)
{
    assert(m_current_module != nullptr);

    if (m_current_module->props.size() == 0)
    {
        AddMessage(line, Message::TYPE_ERROR, "Directive 'add_animation' has no prop to animate, ignoring...");
        return;
    }

    std::smatch results;
    if (! std::regex_search(line, results, Regexes::DIRECTIVE_ADD_ANIMATION))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid directive 'add_animation', ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex.

    Animation animation;
    animation.ratio       = STR_PARSE_REAL(results[2]);
    animation.lower_limit = STR_PARSE_REAL(results[4]);
    animation.upper_limit = STR_PARSE_REAL(results[6]);

    Ogre::StringVector tokens = Ogre::StringUtil::split(results[8], ",");

    for (Ogre::StringVector::iterator itor = tokens.begin(); itor != tokens.end(); itor++)
    {
        std::smatch token_results;
        if (! std::regex_search(*itor, token_results, Regexes::IDENTIFY_ADD_ANIMATION_TOKEN))
        {
            std::stringstream msg;
            msg << "Directive 'add_animation': Unrecognized/misplaced token '" << *itor << "', ignoring....";
            AddMessage(line, Message::TYPE_ERROR, msg.str());
            continue;
        }

        // NOTE: Positions in 'token_results' array match E_CAPTURE*() positions (starting with 1) in Regexes::IDENTIFY_ADD_ANIMATION_TOKEN. 

        if (token_results[2].matched) // autoanimate 
        {
            BITMASK_SET_1(animation.mode, Animation::MODE_AUTO_ANIMATE);
        }
        else if (token_results[3].matched) // noflip 
        {
            BITMASK_SET_1(animation.mode, Animation::MODE_NO_FLIP);
        }
        else if (token_results[4].matched) // bounce 
        {
            BITMASK_SET_1(animation.mode, Animation::MODE_BOUNCE);
        }
        else if (token_results[5].matched) // eventlock 
        {
            BITMASK_SET_1(animation.mode, Animation::MODE_EVENT_LOCK);
        }
        else if (token_results[6].matched) // Mode 
        {
            if (! token_results[7].matched)
            {
                AddMessage(line, Message::TYPE_ERROR, "Directive 'add_animation': Token 'mode' has invalid format, skipping....");
                continue;
            }
            else
            {
                _ParseDirectiveAddAnimationMode(animation, token_results[8]);
            }
        }
        else if (token_results[9].matched) // Source 
        {
            if (! token_results[10].matched)
            {
                AddMessage(line, Message::TYPE_ERROR, "Directive 'add_animation': Token 'source' has invalid format, skipping....");
                continue;
            }
            else
            {
                _ParseDirectiveAddAnimationSource(animation, token_results[11]);
            }
        }
        else if (token_results[12].matched) // Event 
        {
            if (! token_results[13].matched)
            {
                AddMessage(line, Message::TYPE_ERROR, "Directive 'add_animation': Token 'event' has invalid format, skipping....");
                continue;
            }
            else
            {
                animation.event = token_results[14];
                Ogre::StringUtil::toUpperCase(animation.event);
            }
        }
        else 
        {
            AddMessage(line, Message::TYPE_ERROR, "[Internal error] Directive 'add_animation': Token '" + *itor + "' passed syntax check, but wasn't recognized....");
        }
    }

    m_current_module->props.back().animations.push_back(animation);
}

void Parser::ParseAntiLockBrakes(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_ANTI_LOCK_BRAKES))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    AntiLockBrakes anti_lock_brakes;
    anti_lock_brakes.regulation_force = STR_PARSE_REAL(results[1]);
    anti_lock_brakes.min_speed = static_cast<unsigned int>(STR_PARSE_REAL(results[2]));

    if (results[4].matched)
    {
        anti_lock_brakes.pulse_per_sec = STR_PARSE_REAL(results[4]);
        anti_lock_brakes._pulse_per_sec_set = true;

        if (results[6].matched)
        {
            // parse mode 
            Ogre::StringVector tokens = Ogre::StringUtil::split(results[6], "&");
            Ogre::StringVector::iterator iter = tokens.begin();
            for ( ; iter != tokens.end(); iter++)
            {
                std::smatch results;
                if (! std::regex_search(*iter, results, Regexes::ANTI_LOCK_BRAKES_MODE))
                {
                    std::string invalid_keyword = *iter;
                    AddMessage(line, Message::TYPE_WARNING, "Ignoring invalid mode attribute: \"" + invalid_keyword + "\"");
                    continue;
                }
                // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

                if (results[2].matched)
                {
                    BITMASK_SET_1(anti_lock_brakes.mode, AntiLockBrakes::MODE_ON);
                    BITMASK_SET_0(anti_lock_brakes.mode, AntiLockBrakes::MODE_OFF);
                }
                else if (results[3].matched)
                {
                    BITMASK_SET_1(anti_lock_brakes.mode, AntiLockBrakes::MODE_OFF);
                    BITMASK_SET_0(anti_lock_brakes.mode, AntiLockBrakes::MODE_ON);
                }
                else if (results[4].matched)
                {
                    BITMASK_SET_1(anti_lock_brakes.mode, AntiLockBrakes::MODE_NO_DASHBOARD);
                }
                else if (results[5].matched)
                {
                    BITMASK_SET_1(anti_lock_brakes.mode, AntiLockBrakes::MODE_NO_TOGGLE);
                }
            }
        }
    }

    if (m_current_module->anti_lock_brakes != nullptr)
    {
        AddMessage(line, Message::TYPE_WARNING, "Found multiple sections 'AntiLockBrakes' in one module, using last one.");
    }
    m_current_module->anti_lock_brakes = std::shared_ptr<AntiLockBrakes>( new AntiLockBrakes(anti_lock_brakes) );
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

void Parser::ParseEngturbo(Ogre::String const & line)
{
    std::smatch results;
    if (!std::regex_search(line, results, Regexes::SECTION_ENGTURBO))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }

    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 
    Engturbo engturbo;
    engturbo.version = STR_PARSE_REAL(results[1]);
    engturbo.tinertiaFactor = STR_PARSE_REAL(results[2]);
    engturbo.nturbos = STR_PARSE_REAL(results[3]);

    if (STR_PARSE_REAL(results[3]) > 4)
    {
        AddMessage(line, Message::TYPE_WARNING, "You cannot have more than 4 turbos. Fallback: using 4 instead.");
        engturbo.nturbos = 4;
    }

    engturbo.param1 = STR_PARSE_REAL(results[4]);
    engturbo.param2 = STR_PARSE_REAL(results[6]);
    engturbo.param3 = STR_PARSE_REAL(results[8]);
    engturbo.param4 = STR_PARSE_REAL(results[10]);
    engturbo.param5 = STR_PARSE_REAL(results[12]);
    engturbo.param6 = STR_PARSE_REAL(results[14]);
    engturbo.param7 = STR_PARSE_REAL(results[16]);
    engturbo.param8 = STR_PARSE_REAL(results[18]);
    engturbo.param9 = STR_PARSE_REAL(results[20]);
    engturbo.param10 = STR_PARSE_REAL(results[22]);
    engturbo.param11 = STR_PARSE_REAL(results[24]);

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

void Parser::ParseContacter(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::NODE_LIST))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    m_current_module->contacters.push_back( _ParseNodeRef(results[1]) );
}

void Parser::ParseCommand(Ogre::String const & line)
{
    _ParseSectionsCommandsCommands2(line, Regexes::SECTION_COMMANDS, 1);
}

void Parser::ParseCommand2(Ogre::String const & line)
{
    _ParseSectionsCommandsCommands2(line, Regexes::SECTION_COMMANDS_2, 2);
}

void Parser::_ParseSectionsCommandsCommands2(Ogre::String const & line, std::regex const & regex, unsigned int format_version)
{
    std::smatch results;
    if (! std::regex_search(line, results, regex))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Command2 command2;
    command2.beam_defaults     = m_user_beam_defaults;
    command2.detacher_group    = m_current_detacher_group;
    command2._format_version   = format_version;
    command2.inertia_defaults  = m_user_default_inertia;
    command2.nodes[0]          = _ParseNodeRef(results[1]);
    command2.nodes[1]          = _ParseNodeRef(results[3]);

    // Shorten/lenghten rate 
    command2.shorten_rate = STR_PARSE_REAL(results[5]);
    unsigned int result_index;
    if (format_version == 2)
    {
        command2.lengthen_rate = STR_PARSE_REAL(results[7]);
        result_index = 9;
    }
    else
    {
        command2.lengthen_rate = command2.shorten_rate;
        result_index = 7;
    }
    
    command2.max_contraction = STR_PARSE_REAL(results[result_index]);
    result_index +=2;
    command2.max_extension   = STR_PARSE_REAL(results[result_index]);
    result_index +=2;
    command2.contract_key    = STR_PARSE_INT(results[result_index]);
    result_index +=2;
    command2.extend_key      = STR_PARSE_INT(results[result_index]);

    // Options 
    result_index += 4;
    if (results[result_index].matched)
    {
        std::string options_str = results[result_index].str();
        bool centering = false;
        bool one_press_mode = false;
        for (unsigned int i = 0; i < options_str.length(); i++)
        {
            const char c = options_str.at(i);
            switch(c)
            {
                case 'n': // Filler 
                    break;

                case 'i':
                    command2.options |= Command2::OPTION_i_INVISIBLE;
                    break;

                case 'r':
                    command2.options |= Command2::OPTION_r_ROPE;
                    break;

                case 'c':
                    if (! one_press_mode)
                    {
                        command2.options |= Command2::OPTION_c_AUTO_CENTER;
                        centering = true;
                    }
                    else
                    {
                        AddMessage(line, Message::TYPE_WARNING, "Command cannot be one-pressed and self centering at the same time, ignoring flag 'c'");
                    }
                    break;

                case 'f':
                    command2.options |= Command2::OPTION_f_NOT_FASTER;
                    break;

                case 'p':
                    if (centering)
                    {
                        AddMessage(line, Message::TYPE_WARNING, "Command cannot be one-pressed and self centering at the same time, ignoring flag 'p'");
                    }
                    else if (one_press_mode)
                    {
                        AddMessage(line, Message::TYPE_WARNING, "Command already has a one-pressed c.mode, ignoring flag 'p'");
                    }
                    else
                    {
                        command2.options |= Command2::OPTION_p_PRESS_ONCE;
                        one_press_mode = true;
                    }
                    break;

                case 'o':
                    if (centering)
                    {
                        AddMessage(line, Message::TYPE_WARNING, "Command cannot be one-pressed and self centering at the same time, ignoring flag 'o'");
                    }
                    else if (one_press_mode)
                    {
                        AddMessage(line, Message::TYPE_WARNING, "Command already has a one-pressed c.mode, ignoring flag 'o'");
                    }
                    else
                    {
                        command2.options |= Command2::OPTION_o_PRESS_ONCE_CENTER;
                        one_press_mode = true;
                    }
                    break;
                
                default:
                    this->AddMessage(options_str, Message::TYPE_WARNING, std::string("Ignoring invalid option: ") + c);
                    break;
            }
        }

        result_index += 4;
        if (results[result_index].matched)
        {
            command2.description = results[result_index];

            if (format_version == 1)
            {
                result_index += 6;
                if (_ParseOptionalInertia(command2.inertia, results, result_index))
                {
                    result_index += 12;
            
                    if (results[result_index].matched)
                    {
                        command2.affect_engine = STR_PARSE_REAL(results[result_index]);

                        result_index += 3;
                        if (results[result_index].matched)
                        {
                            command2.needs_engine = STR_PARSE_BOOL(results[result_index]);
                        }
                    }
                }
            }
            else if (format_version == 2)
            {
                result_index += 3;
                if (results[result_index].matched)
                {
                    std::string rest_of_line = results[result_index];
                    std::smatch rest_of_results;
                    result_index = 0;
                    if (std::regex_search(rest_of_line, rest_of_results, Regexes::SECTION_COMMANDS2_INERTIA_ENGINE_PART))
                    {
                        if (_ParseOptionalInertia(command2.inertia, rest_of_results, 2))
                        {
                            result_index += 12;
            
                            if (rest_of_results[result_index].matched)
                            {
                                command2.affect_engine = STR_PARSE_REAL(rest_of_results[result_index]);

                                result_index += 3;
                                if (rest_of_results[result_index].matched)
                                {
                                    command2.needs_engine = STR_PARSE_BOOL(rest_of_results[result_index]);
                                }
                            }
                        }
                    }
                    else
                    {
                        Ogre::StringUtil::trim(rest_of_line);
                        if (! rest_of_line.empty()) // Could happen with invalid trailing delimiter and I don't want to look like an idiot
                        {
                            std::stringstream msg;
                            msg << "Please remove: Invalid string after parameter \"description\": \"" << rest_of_line << "\""; 
                            AddMessage(line, Message::TYPE_WARNING, msg.str());
                        }
                    }
                }
            }
        }
    }

    m_current_module->commands_2.push_back(command2);
}

void Parser::ParseCollisionBox(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_COLLISIONBOXES))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    CollisionBox collisionbox;

    Ogre::StringVector tokens = Ogre::StringUtil::split(line, ",");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        collisionbox.nodes.push_back( _ParseNodeRef(*iter) );
    }

    m_current_module->collision_boxes.push_back(collisionbox);
}

void Parser::ParseCinecam(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_CINECAM))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Cinecam cinecam;
    cinecam.beam_defaults = m_user_beam_defaults;
    cinecam.node_defaults = m_user_node_defaults;

    cinecam.position.x = STR_PARSE_REAL(results[1]);
    cinecam.position.y = STR_PARSE_REAL(results[3]);
    cinecam.position.z = STR_PARSE_REAL(results[5]);
    if (m_sequential_importer.IsEnabled())
    {
        m_sequential_importer.AddGeneratedNode(File::KEYWORD_CINECAM);
    }
    cinecam.nodes[0] = _ParseNodeRef(results[ 7]);
    cinecam.nodes[1] = _ParseNodeRef(results[ 9]);
    cinecam.nodes[2] = _ParseNodeRef(results[11]);
    cinecam.nodes[3] = _ParseNodeRef(results[13]);
    cinecam.nodes[4] = _ParseNodeRef(results[15]);
    cinecam.nodes[5] = _ParseNodeRef(results[17]);
    cinecam.nodes[6] = _ParseNodeRef(results[19]);
    cinecam.nodes[7] = _ParseNodeRef(results[21]);

    if (results[24].matched)
    {
        cinecam.spring = STR_PARSE_REAL(results[24]);
        
        if (results[27].matched)
        {
            cinecam.damping = STR_PARSE_REAL(results[27]);
        }
    }

    m_current_module->cinecam.push_back(cinecam);
}

void Parser::ParseCameraRails(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_CAMERARAILS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    m_current_camera_rail->nodes.push_back( _ParseNodeRef(results[1]) );
}

void Parser::ParseBrakes(Ogre::String line)
{
    std::smatch results;
    bool result = std::regex_search(line, results, Regexes::SECTION_BRAKES);
    if (! result)
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }

    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (m_current_module->brakes == nullptr)
    {
        m_current_module->brakes = std::shared_ptr<Brakes>( new Brakes() );
    }

    m_current_module->brakes->default_braking_force = STR_PARSE_REAL(results[1]);

    if (results[3].matched)
    {
        m_current_module->brakes->parking_brake_force = STR_PARSE_REAL(results[3]); 
        m_current_module->brakes->_parking_brake_force_set = true;
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

void Parser::ParseAirbrakes(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_AIRBRAKES))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex.

    Airbrake airbrake;
    airbrake.reference_node        =  _ParseNodeRef(results[ 1]);
    airbrake.x_axis_node           =  _ParseNodeRef(results[ 3]);
    airbrake.y_axis_node           =  _ParseNodeRef(results[ 5]);
    airbrake.aditional_node        =  _ParseNodeRef(results[ 7]);
    airbrake.offset.x              = STR_PARSE_REAL(results[ 9]);
    airbrake.offset.y              = STR_PARSE_REAL(results[11]);
    airbrake.offset.z              = STR_PARSE_REAL(results[13]);
    airbrake.width                 = STR_PARSE_REAL(results[15]);
    airbrake.height                = STR_PARSE_REAL(results[17]);
    airbrake.max_inclination_angle = STR_PARSE_REAL(results[19]);
    airbrake.texcoord_x1           = STR_PARSE_REAL(results[21]);
    airbrake.texcoord_y1           = STR_PARSE_REAL(results[23]);
    airbrake.texcoord_x2           = STR_PARSE_REAL(results[25]);
    airbrake.texcoord_y2           = STR_PARSE_REAL(results[26]);

    m_current_module->airbrakes.push_back(airbrake);
}

void Parser::ParseVideoCamera(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_VIDEOCAMERA))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    VideoCamera videocamera;

    videocamera.reference_node       = _ParseNodeRef(results[1]);
    videocamera.left_node            = _ParseNodeRef(results[2]);
    videocamera.bottom_node          = _ParseNodeRef(results[3]);

    std::string param_4 = results[4];
    if (! std::regex_match(param_4, Regexes::MINUS_ONE_REAL))
    {
        videocamera.alt_reference_node = _ParseNodeRef(param_4);
    }

    std::string param_5 = results[5];
    if (! std::regex_match(param_5, Regexes::MINUS_ONE_REAL))
    {
        videocamera.alt_orientation_node = _ParseNodeRef(param_5);
    }

    videocamera.offset.x = STR_PARSE_REAL(results[6]);
    videocamera.offset.y = STR_PARSE_REAL(results[7]);
    videocamera.offset.z = STR_PARSE_REAL(results[8]);

    videocamera.rotation.x = STR_PARSE_REAL(results[9]);
    videocamera.rotation.y = STR_PARSE_REAL(results[10]);
    videocamera.rotation.z = STR_PARSE_REAL(results[11]);
    
    videocamera.field_of_view = STR_PARSE_REAL(results[12]);

    videocamera.texture_width  = STR_PARSE_INT(results[13]);
    videocamera.texture_height = STR_PARSE_INT(results[14]);

    videocamera.min_clip_distance = STR_PARSE_REAL(results[15]);
    videocamera.max_clip_distance = STR_PARSE_REAL(results[16]);

    videocamera.camera_role = STR_PARSE_INT(results[17]);
    videocamera.camera_mode = STR_PARSE_INT(results[18]);
    videocamera.material_name = results[19];
    if (results[21].matched)
    {
        videocamera.camera_name = results[21];
    }

    m_current_module->videocameras.push_back(videocamera);
}

void Parser::ParseCamerasUnsafe(Ogre::String const & line)
{
    PARSE_UNSAFE(line, 3,
    {
        Camera camera;
        camera.center_node = this->_ParseNodeRef(values[0]);
        camera.back_node   = this->_ParseNodeRef(values[1]);
        camera.left_node   = this->_ParseNodeRef(values[2]);

        m_current_module->cameras.push_back(camera);
    });
}

void Parser::ParseCameras(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_CAMERAS))
    {
        this->ParseCamerasUnsafe(line);
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Camera camera;
    camera.center_node = _ParseNodeRef(results[1]);
    camera.back_node   = _ParseNodeRef(results[2]);
    camera.left_node   = _ParseNodeRef(results[3]);

    m_current_module->cameras.push_back(camera);
}

void Parser::ParseTurboprops(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_TURBOPROPS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Turboprop2 turboprop;
    turboprop.reference_node = _ParseNodeRef(results[1]);
    turboprop.axis_node = _ParseNodeRef(results[2]);
    turboprop.blade_tip_nodes[0] = _ParseNodeRef(results[3]);
    turboprop.blade_tip_nodes[1] = _ParseNodeRef(results[4]);
    turboprop.blade_tip_nodes[2] = _ParseNodeRef(results[5]);
    turboprop.blade_tip_nodes[3] = _ParseNodeRef(results[6]);
    turboprop.turbine_power_kW = STR_PARSE_REAL(results[7]);
    turboprop.airfoil = results[8];

    m_current_module->turboprops_2.push_back(turboprop);
}

void Parser::ParseTurbojets(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_TURBOJETS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Turbojet turbojet;
    turbojet.front_node     = _ParseNodeRef(results[1]);
    turbojet.back_node      = _ParseNodeRef(results[2]);
    turbojet.side_node      = _ParseNodeRef(results[3]);
    turbojet.is_reversable  = STR_PARSE_INT(results[4]);
    turbojet.dry_thrust     = STR_PARSE_REAL(results[5]);
    turbojet.wet_thrust     = STR_PARSE_REAL(results[6]);
    turbojet.front_diameter = STR_PARSE_REAL(results[7]);
    turbojet.back_diameter  = STR_PARSE_REAL(results[8]);
    turbojet.nozzle_length  = STR_PARSE_REAL(results[9]);

    m_current_module->turbojets.push_back(turbojet);
}

void Parser::ParseTriggers(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_TRIGGERS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Trigger trigger;
    trigger.beam_defaults = m_user_beam_defaults;
    trigger.detacher_group = m_current_detacher_group;
    trigger.nodes[0] = _ParseNodeRef(results[1]);
    trigger.nodes[1] = _ParseNodeRef(results[2]);
    trigger.contraction_trigger_limit = STR_PARSE_REAL(results[3]);
    trigger.expansion_trigger_limit = STR_PARSE_REAL(results[4]);
    int shortbound_trigger_action = STR_PARSE_INT(results[5]);
    int longbound_trigger_action = STR_PARSE_INT(results[6]);

    if (results[8].matched)
    {
        std::string options_str = results[8].str();
        for (unsigned int i = 0; i < options_str.length(); i++)
        {
            switch(options_str.at(i))
            {
                case 'i':
                    trigger.options |= Trigger::OPTION_i_INVISIBLE;
                    break;
                case 'c':
                    trigger.options |= Trigger::OPTION_c_COMMAND_STYLE;
                    break;
                case 'x':
                    trigger.options |= Trigger::OPTION_x_START_OFF;
                    break;
                case 'b':
                    trigger.options |= Trigger::OPTION_b_BLOCK_KEYS;
                    break;
                case 'B':
                    trigger.options |= Trigger::OPTION_B_BLOCK_TRIGGERS;
                    break;
                case 'A':
                    trigger.options |= Trigger::OPTION_A_INV_BLOCK_TRIGGERS;
                    break;
                case 's':
                    trigger.options |= Trigger::OPTION_s_SWITCH_CMD_NUM;
                    break;
                case 'h':
                    trigger.options |= Trigger::OPTION_h_UNLOCK_HOOKGROUPS_KEY;
                    break;
                case 'H':
                    trigger.options |= Trigger::OPTION_H_LOCK_HOOKGROUPS_KEY;
                    break;
                case 't':
                    trigger.options |= Trigger::OPTION_t_CONTINUOUS;
                    break;
                case 'E':
                    trigger.options |= Trigger::OPTION_E_ENGINE_TRIGGER;
                    break;

                default:
                    AddMessage(line, Message::TYPE_WARNING, Ogre::String("Invalid trigger option: " + options_str.at(i)));
                    break;
            }
        }
    }
    if (results[10].matched)
    {
        trigger.boundary_timer = STR_PARSE_REAL(results[10]);
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

void Parser::ParseTorqueCurve(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_TORQUECURVE))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (m_current_module->torque_curve == nullptr)
    {
        m_current_module->torque_curve = std::shared_ptr<RigDef::TorqueCurve>(new RigDef::TorqueCurve());
    }

    if (results[1].matched)
    {
        TorqueCurve::Sample sample;
        sample.power = STR_PARSE_REAL(results[2]);
        sample.torque_percent = STR_PARSE_REAL(results[3]);
        m_current_module->torque_curve->samples.push_back(sample);
    }
    else
    {
        m_current_module->torque_curve->predefined_func_name = results[4];
    }
}

void Parser::ParseTies(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_TIES))
    {
        this->AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex.

    Tie tie;
    tie.beam_defaults     = m_user_beam_defaults;
    tie.detacher_group    = m_current_detacher_group;

    tie.root_node         = this->_ParseNodeRef(results[1]);
    tie.max_reach_length  =      STR_PARSE_REAL(results[3]);
    tie.auto_shorten_rate =      STR_PARSE_REAL(results[5]);
    tie.min_length        =      STR_PARSE_REAL(results[7]);
    tie.max_length        =      STR_PARSE_REAL(results[9]);

    if (results[12].matched)
    {
        std::string tie_options = results[12];
        for (unsigned i = 0; i < tie_options.size(); ++i)
        {
            const char tie_char = tie_options.at(i);
            switch (tie_char)
            {
            case 'n':
                tie.options = Tie::OPTIONS_VISIBLE;
                break;
            case 'i':
                tie.options = Tie::OPTIONS_INVISIBLE;
                break;
            default:
                this->AddMessage(line, Message::TYPE_WARNING, std::string("Ignoring invalid option: ") + tie_char);
                break;
            }
        }

        if (results[15].matched)
        {
            tie.max_stress = STR_PARSE_REAL(results[15]);

            if (results[18].matched)
            {
                tie.group = STR_PARSE_INT(results[18]);
            }
        }
    }

    m_current_module->ties.push_back(tie);
}

void Parser::ParseSoundsources(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_SOUNDSOURCES))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    SoundSource soundsource;
    soundsource.node = _ParseNodeRef(results[1]);
    soundsource.sound_script_name = results[2];

    m_current_module->soundsources.push_back(soundsource);
}

void Parser::ParseSoundsources2(Ogre::String const & line)
{

    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_SOUNDSOURCES2))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    SoundSource2 soundsource2;
    soundsource2.node = _ParseNodeRef(results[1]);
    soundsource2.sound_script_name = results[5];

    // Mode 
    int mode = 0;
    Ogre::String mode_str = results[3];
    if (! std::regex_match(mode_str, Regexes::DECIMAL_NUMBER) )
    {
        AddMessage(line, Message::TYPE_WARNING, "Invalid value of parameter ~2 'mode': '" + mode_str + "', parsing as '0' for backwards compatibility. Please fix.");
    }
    else
    {
        mode = STR_PARSE_INT(mode_str);
    }
    if (mode < 0)
    {
        if (mode < -2)
        {
            AddMessage(line, Message::TYPE_ERROR, "Invalid soundsources2.mode setting, ignoring whole line...");
            return;
        }
        soundsource2.mode = SoundSource2::Mode(mode);
    }
    else
    {
        soundsource2.mode = SoundSource2::MODE_CINECAM;
        soundsource2.cinecam_index = mode;
    }

    _CheckInvalidTrailingText(line, results, 6);

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
        m_user_default_inertia->_start_delay_factor_set = true;
        m_user_default_inertia->stop_delay_factor = stop_delay;
        m_user_default_inertia->_stop_delay_factor_set = true;

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

void Parser::_ParseRotatorsCommon(Rotator & rotator, std::smatch & results, unsigned int inertia_start_index)
{
    rotator.axis_nodes[0] = _ParseNodeRef(results[1]);
    rotator.axis_nodes[1] = _ParseNodeRef(results[3]);

    rotator.base_plate_nodes[0] = _ParseNodeRef(results[ 5]);
    rotator.base_plate_nodes[1] = _ParseNodeRef(results[ 7]);
    rotator.base_plate_nodes[2] = _ParseNodeRef(results[ 9]);
    rotator.base_plate_nodes[3] = _ParseNodeRef(results[11]);

    rotator.rotating_plate_nodes[0] = _ParseNodeRef(results[13]);
    rotator.rotating_plate_nodes[1] = _ParseNodeRef(results[15]);
    rotator.rotating_plate_nodes[2] = _ParseNodeRef(results[17]);
    rotator.rotating_plate_nodes[3] = _ParseNodeRef(results[19]);

    rotator.rate = STR_PARSE_REAL(results[21]);

    rotator.spin_left_key  = STR_PARSE_INT(results[23]);
    rotator.spin_right_key = STR_PARSE_INT(results[25]);

    // Inertia part 

    if (this->_ParseOptionalInertia(rotator.inertia, results, inertia_start_index))
    {
        unsigned int i = inertia_start_index + 12;

        if (results[i].matched)
        {
            rotator.engine_coupling = STR_PARSE_REAL(results[i]);
            i += 3;

            if (results[i].matched)
            {
                rotator.needs_engine = STR_PARSE_BOOL(results[i]);
            }
        }
    }
}

void Parser::ParseRotators(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_ROTATORS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex.

    Rotator rotator;
    rotator.inertia_defaults = m_user_default_inertia;

    const unsigned int inertia_start_index = 29;
    this->_ParseRotatorsCommon(rotator, results, inertia_start_index);

    // Backwards compatibility check
    // Parameter "inertia_start_delay" must accept garbage
    if (results[inertia_start_index].matched)
    {
        std::string start_delay_str = results[inertia_start_index];
        if (! std::regex_match(start_delay_str, Regexes::REAL_NUMBER))
        {
            float result = STR_PARSE_REAL(start_delay_str);
            std::stringstream msg;
            msg << "Invalid value of parameter ~14 'inertia_start_delay': '" << start_delay_str 
                << "', parsing as '" << result << "' for backwards compatibility. Please fix.";
            AddMessage(line, Message::TYPE_WARNING, msg.str());
        }
    }

    m_current_module->rotators.push_back(rotator);
}

void Parser::ParseRotators2(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_ROTATORS2))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex.

    Rotator2 rotator;
    rotator.inertia_defaults = m_user_default_inertia;

    const unsigned int inertia_start_index = 38;
    this->_ParseRotatorsCommon(rotator, results, inertia_start_index);

    if (results[28].matched)
    {
        rotator.rotating_force = STR_PARSE_REAL(results[28]);

        if (results[31].matched)
        {
            rotator.tolerance = STR_PARSE_REAL(results[31]);

            if (results[34].matched)
            {
                rotator.description = results[34];
            }
        }
    }

    m_current_module->rotators_2.push_back(rotator);
}

void Parser::ParseFileinfo(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_FILEINFO))
    {
        // Do exactly what legacy parser would.
        PARSE_UNSAFE(line, 2,
        {
            Fileinfo fileinfo;

            std::string unique_id = values[1];
            Ogre::StringUtil::trim(unique_id);
            std::stringstream report;
            report << "Check:";
            if (unique_id.length() > 0)
            {
                fileinfo.unique_id = unique_id;
                fileinfo._has_unique_id = true;
                report << "\n\tUID: " << unique_id;
            }
            else
            {
                report << "\n\tUID: [empty]";
            }
            if (values.size() > 2)
            {
                fileinfo.category_id = PARSEINT(values[2]);
                fileinfo._has_category_id = true;
                report << "\n\tCategoryID: " << fileinfo.category_id;
            }
            else
            {
                report << "\n\tCategoryID: [not set]";
            }
            if (values.size() > 3)
            {
                fileinfo.file_version = PARSEINT(values[3]);
                fileinfo._has_file_version_set = true;
                report << "\n\tFile version:: " << fileinfo.file_version;
            }
            else
            {
                report << "\n\tFile version: [not set]";
            }
            this->AddMessage(line, Message::TYPE_INVALID, report.str());
            m_definition->file_info = std::shared_ptr<Fileinfo>( new Fileinfo(fileinfo) );
        })
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Fileinfo fileinfo;
    bool go_next = false;

    // UID 
    if (results[3].matched)
    {
        fileinfo.unique_id = results[3];
        fileinfo._has_unique_id = true;
        go_next = true;
    }
    else if (results[2].matched)
    {
        go_next = true;
    }

    if (!go_next)
    {
        AddMessage(line, Message::TYPE_ERROR, "Inline-section 'fileinfo' has invalid UID, ignoring line...");
    }
    go_next = false;

    // category id 
    if (results[6].matched)
    {
        fileinfo.category_id = STR_PARSE_INT(results[6]);
        fileinfo._has_category_id = true;
        go_next = true;
    }
    else if (results[7].matched)
    {
        go_next = true;
    }

    if (!go_next)
    {
        AddMessage(line, Message::TYPE_ERROR, "Inline-section 'fileinfo' has invalid category id, ignoring line...");
    }
    go_next = false;

    // file version 
    int version = -1;
    if (results[10].matched)
    {
        // Integer input
        version = STR_PARSE_INT(results[10]);
    }
    else if (results[11].matched)
    {
        // Float input, silently parse as int.
        version = STR_PARSE_INT(results[11]);
    }

    if (version >= 0)
    {
        fileinfo.file_version = static_cast<unsigned int>(version);
        fileinfo._has_file_version_set = true;
    }

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

void Parser::ParseRopables(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_ROPABLES))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Ropable ropable;
    ropable.node = _ParseNodeRef(results[1]);

    if (results[3].matched)
    {
        ropable.group = STR_PARSE_INT(results[3]);
        ropable._has_group_set = true;

        if (results[5].matched)
        {
            unsigned int multilock = STR_PARSE_INT(results[5]);
            ropable.multilock = multilock > 0;
            ropable._has_multilock_set = true;
        }
    }

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

void Parser::ParseProps(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_PROPS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Prop prop;
    prop.reference_node = _ParseNodeRef(results[1]);
    prop.x_axis_node = _ParseNodeRef(results[3]);
    prop.y_axis_node = _ParseNodeRef(results[5]);

    prop.offset.x = STR_PARSE_REAL(results[7]);
    prop.offset.y = STR_PARSE_REAL(results[9]);
    prop.offset.z = STR_PARSE_REAL(results[11]);

    prop.rotation.x = STR_PARSE_REAL(results[13]);
    prop.rotation.y = STR_PARSE_REAL(results[15]);
    prop.rotation.z = STR_PARSE_REAL(results[17]);

    prop.mesh_name = results[19];

    // Special props 
    std::smatch special_results;
    if (std::regex_search(prop.mesh_name, special_results, Regexes::SPECIAL_PROPS))
    {
        for (int i = 1; i <= 11; i++)
        {
            if (special_results[i].matched)
            {
                if (i == 3 || i == 4) // Custom steering wheel 
                {
                    prop.special = Prop::Special(i);

                    std::string special_params = results[21];
                    std::smatch dashboard_results;
                    if (std::regex_search(special_params, dashboard_results, Regexes::SPECIAL_PROP_DASHBOARD))
                    {
                        if ( dashboard_results[4].matched )
                        {
                            prop.special_prop_steering_wheel.offset.x = STR_PARSE_REAL(dashboard_results[4]);

                            if ( dashboard_results[7].matched )
                            {
                                prop.special_prop_steering_wheel.offset.y = STR_PARSE_REAL(dashboard_results[7]);

                                if ( dashboard_results[10].matched )
                                {
                                    prop.special_prop_steering_wheel.offset.z = STR_PARSE_REAL(dashboard_results[10]);
                                    prop.special_prop_steering_wheel._offset_is_set = true;
                                    prop.special_prop_steering_wheel.mesh_name = dashboard_results[1];

                                    if ( dashboard_results[13].matched )
                                    {
                                        prop.special_prop_steering_wheel.rotation_angle = STR_PARSE_REAL(dashboard_results[13]);
                                    }
                                }
                            }
                        }
                    }
                }
                else if (i == 9) // Beacon 
                {
                    prop.special = Prop::Special(i);
                    std::string special_params = results[21];
                    std::smatch beacon_results;
                    if (std::regex_search(special_params, beacon_results, Regexes::SPECIAL_PROP_BEACON))
                    {
                        // Flare material name 
                        prop.special_prop_beacon.flare_material_name = beacon_results[1];
                        Ogre::StringUtil::trim(prop.special_prop_beacon.flare_material_name);
                        // Color 
                        prop.special_prop_beacon.color = Ogre::ColourValue(
                            STR_PARSE_REAL(beacon_results[3]),
                            STR_PARSE_REAL(beacon_results[5]),
                            STR_PARSE_REAL(beacon_results[7])
                        );
                    }
                }
                else
                {
                    prop.special = Prop::Special(i);
                }
                break;
            }
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

void Parser::ParseManagedMaterials(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_MANAGEDMATERIALS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    ManagedMaterial managed_material;
    managed_material.options = m_current_managed_material_options;
    managed_material.name = results[1].str();

    managed_material.type = (results[4].matched) ? ManagedMaterial::TYPE_MESH_STANDARD        : managed_material.type;
    managed_material.type = (results[5].matched) ? ManagedMaterial::TYPE_MESH_TRANSPARENT     : managed_material.type;
    managed_material.type = (results[6].matched) ? ManagedMaterial::TYPE_FLEXMESH_STANDARD    : managed_material.type;
    managed_material.type = (results[7].matched) ? ManagedMaterial::TYPE_FLEXMESH_TRANSPARENT : managed_material.type;

    managed_material.diffuse_map = results[9];

    if	(	managed_material.type == ManagedMaterial::TYPE_MESH_STANDARD
        ||	managed_material.type == ManagedMaterial::TYPE_MESH_TRANSPARENT)
    {
        if (results[12].matched)
        {
            Ogre::String input = results[12];
            Ogre::StringUtil::trim(input);

            //    According to documentation, '-' is a placeholder for no texture.
            //    However, for compatibility with old parser, we need to only check the first letter and ignore the rest.
            
            if (input.at(0) != '-')
            {
                managed_material.specular_map = input;
            }
        }
    }
    else
    {
        if (results[12].matched)
        {
            Ogre::String input = results[12];

            //    According to documentation, '-' is a placeholder for no texture.
            //    However, for compatibility with old parser, we need to only check the first letter and ignore the rest.
            
            if (input.at(0) != '-')
            {
                managed_material.damaged_diffuse_map = input;
            }
        }
        if (results[15].matched)
        {
            Ogre::String input = results[15];

            //    According to documentation, '-' is a placeholder for no texture.
            //    However, for compatibility with old parser, we need to only check the first letter and ignore the rest.
            
            if (input.at(0) != '-')
            {
                managed_material.specular_map = input;
            }
        }
    }

    m_current_module->managed_materials.push_back(managed_material);
}

void Parser::ParseLockgroups(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_LOCKGROUPS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Lockgroup lockgroup;
    lockgroup.number = STR_PARSE_INT(results[1]);

    // Node list 
    Ogre::StringVector tokens = Ogre::StringUtil::split(results[2].str(), ",");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        Ogre::String token = *iter;
        lockgroup.nodes.push_back(_ParseNodeRef(token));
    }

    _CheckInvalidTrailingText(line, results, 5);
    
    m_current_module->lockgroups.push_back(lockgroup);
}

void Parser::ParseHydros(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_HYDROS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Hydro hydro;
    hydro.inertia_defaults   = m_user_default_inertia;
    hydro.detacher_group     = m_current_detacher_group;
    hydro.beam_defaults      = m_user_beam_defaults;
    hydro.nodes[0]           = _ParseNodeRef(results[1]);
    hydro.nodes[1]           = _ParseNodeRef(results[3]);
    hydro.lenghtening_factor = STR_PARSE_REAL(results[5]);

    // Flags 
    if (results[8].matched)
    {
        hydro.options = results[8];
    }

    // Inertia part 
    _ParseOptionalInertia(hydro.inertia, results, 11);

    m_current_module->hydros.push_back(hydro);
}

bool Parser::_ParseOptionalInertia(Inertia & inertia, std::smatch & results, unsigned int start_index)
{
    unsigned int result_index = start_index;

    if (results[result_index].matched)
    {
        Ogre::String start_delay_str = results[result_index];
        inertia.start_delay_factor = STR_PARSE_REAL(start_delay_str);
        inertia._start_delay_factor_set = true;

        result_index += 3;
        if (results[result_index].matched)
        {
            Ogre::String stop_delay_str = results[result_index];
            inertia.stop_delay_factor = STR_PARSE_REAL(stop_delay_str);
            inertia._stop_delay_factor_set = true;

            result_index +=3;
            if (results[result_index].matched)
            {
                inertia.start_function = results[result_index];

                result_index += 3;
                if (results[result_index].matched)
                {
                    inertia.stop_function = results[result_index];
                    return true;
                }
            }
        }
    }
    return false;
}

void Parser::ParseBeams(Ogre::String const & _line)
{
    const int FLAGS_INDEX     = 7;
    const int EXT_LIMIT_INDEX = 11;

    std::string line;
    Parser::_TrimTrailingComments(_line, line);

    // Parse arguments
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_BEAMS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Beam beam;
    beam.defaults = m_user_beam_defaults;
    beam.detacher_group = m_current_detacher_group;
    beam.nodes[0] = _ParseNodeRef(results[1]);
    beam.nodes[1] = _ParseNodeRef(results[3]);

    // Flags 
    if (results[FLAGS_INDEX].matched)
    {
        std::string const & flags_str = results[FLAGS_INDEX];
        for (unsigned int i = 0; i < flags_str.length(); i++)
        {
            if (flags_str[i] == 'v') 
            {
                continue; // Dummy flag
            }
            if (flags_str[i] == 'i') 
            {
                beam.options |= Beam::OPTION_i_INVISIBLE;
            }
            else if (flags_str[i] == 'r')
            {
                beam.options |= Beam::OPTION_r_ROPE;
            }
            else if (flags_str[i] == 's')
            {
                beam.options |= Beam::OPTION_s_SUPPORT;
                if (results[EXT_LIMIT_INDEX].matched)
                {
                    beam._has_extension_break_limit = true;
                    beam.extension_break_limit = STR_PARSE_REAL(results[EXT_LIMIT_INDEX]);
                }
            }
            else
            {
                AddMessage(line, Message::TYPE_WARNING, std::string("Invalid flag: ") + flags_str[i]);
            }
        }
    }

    m_current_module->beams.push_back(beam);
}

void Parser::ParseAnimator(Ogre::String const & line)
{
    std::smatch results;
    if (! std::regex_search(line, results, Regexes::SECTION_ANIMATORS))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    Animator animator;
    animator.inertia_defaults   = m_user_default_inertia;
    animator.beam_defaults      = m_user_beam_defaults;
    animator.detacher_group     = m_current_detacher_group;
    animator.nodes[0]           = _ParseNodeRef(results[1]);
    animator.nodes[1]           = _ParseNodeRef(results[2]);
    animator.lenghtening_factor = STR_PARSE_REAL(results[3]);

    // Parse options 
    // Just use the split/trim/compare method 
    Ogre::StringVector tokens = Ogre::StringUtil::split(results[4].str(), "|");
    Ogre::StringVector::iterator iter = tokens.begin();
    while (iter != tokens.end())
    {
        Ogre::String token = *iter;
        Ogre::StringUtil::trim(token);

        // Numbered keywords 
        if (std::regex_search(token, results, Regexes::PARSE_ANIMATORS_NUMBERED_KEYWORD))
        {
            AeroAnimator aero_animator;
            
                 if (results[1] == "throttle")   animator.aero_animator.flags |= AeroAnimator::OPTION_THROTTLE;
            else if (results[1] == "rpm")        animator.aero_animator.flags |= AeroAnimator::OPTION_RPM;
            else if (results[1] == "aerotorq")   animator.aero_animator.flags |= AeroAnimator::OPTION_TORQUE;
            else if (results[1] == "aeropit")    animator.aero_animator.flags |= AeroAnimator::OPTION_PITCH;
            else if (results[1] == "aerostatus") animator.aero_animator.flags |= AeroAnimator::OPTION_STATUS;

            animator.aero_animator.motor = STR_PARSE_INT(results[2]);
        }
        else if (std::regex_search(token, results, Regexes::PARSE_ANIMATORS_KEY_COLON_VALUE))
        {
            if (results[1] == "shortlimit")
            {
                animator.short_limit = STR_PARSE_REAL(results[2]);
                animator.flags |= Animator::OPTION_SHORT_LIMIT;
            }
            else if(results[1] == "longlimit")
            {
                animator.long_limit = STR_PARSE_REAL(results[2]);
                animator.flags |= Animator::OPTION_LONG_LIMIT;
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

        iter++;
    }

    m_current_module->animators.push_back(animator);
}

void Parser::ParseAuthor(Ogre::String const & line)
{
    if (! _IsCurrentModuleRoot())
    {
        AddMessage(line, Message::TYPE_WARNING, "Inline-section 'author' has global effect and should not appear in a module");
    }

    std::smatch results;
    if (! std::regex_search(line, results, Regexes::INLINE_SECTION_AUTHOR))
    {
        AddMessage(line, Message::TYPE_ERROR, "Invalid line, ignoring...");
        return;
    }

    Author author;

    if (results[1].matched)
    {
        author.type = results[2].str();
    }
    else
    {
        AddMessage(line, Message::TYPE_WARNING, "Inline-section 'author' has no parameters specified, ignoring...");
        return;
    }

    // Forum account id 
    if (results[3].matched)
    {
        int user_id = STR_PARSE_INT(results[4]);
        if (user_id != -1)
        {
            author._has_forum_account = true;
            author.forum_account_id = user_id;
        }

        // Name 
        if (results[5].matched)
        {
            author.name = results[6].str();

            // Email 
            if (results[7].matched)
            {
                author.email = results[8].str();
            }
        }
    }

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
    m_num_contiguous_blank_lines = 0;
    m_definition = std::shared_ptr<File>(new File());
    m_in_block_comment = false;
    m_in_description_section = false;
    m_any_named_node_defined = false;
    m_last_flexbody.reset(); // Set to nullptr
    m_current_detacher_group = 0; // Global detacher group 

    m_user_default_inertia = m_ror_default_inertia;
    m_user_beam_defaults = m_ror_beam_defaults;
    m_user_node_defaults = m_ror_node_defaults;
    m_current_managed_material_options = ManagedMaterialsOptions();

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

int Parser::_ParseArgs(std::string const & line, Ogre::StringVector &args, unsigned min_num_args)
{
    // Note: This splitting silently ignores multiple consecutive separators.
    //       For example "A,|,B" will produce results[0] = "A", results[1] = "B"
    try
    {
        args = Ogre::StringUtil::split(line, ":|, \t");
        if (args.size() < min_num_args)
        {
            std::stringstream msg;
            msg << "Too few arguments, minimum is: " << min_num_args;
            this->AddMessage(line, Message::TYPE_ERROR, msg.str());
            args.clear();
            return -1;
        }
        return args.size();
    } 
    catch(Ogre::Exception &e)
    {
        this->AddMessage(line, Message::TYPE_ERROR, "Unexpected error while parsing line: "+e.getFullDescription());
        args.clear();
        return -1;
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

unsigned Parser::GetArgUint(int index)
{
    return static_cast<unsigned>(this->GetArgLong(index));
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
