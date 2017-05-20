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

/// @file
/// @author Petr Ohlidal
/// @date   12/2013

#pragma once

#include "RigDef_Prerequisites.h"
#include "RigDef_File.h"
#include "RigDef_SequentialImporter.h"

#include <memory>
#include <string>
#include <regex>

namespace RigDef
{

/**
    @class  Parser
    @author Petr Ohlidal

    @brief Checks the rig-def file syntax and pulls data to File object

    ARCHITECTURE:
        Every section/directive of the file should have it's own parser function. 
        If some sections are so similar that a single function could be used, 
        write a helper function and 2 wrappers for the sections.

    GENERAL RULE:
        The Parser should only check syntax and read data. Data transformation should be performed
        in another subsystem. There are cases when data need to be processed immediately, but those
        are exceptions. For example, the 'TractionControl/regulating_force' parameter has valid
        range 0-20 and any other values should be clamped to it - however, the parser should
        load the data as-is and leave the clamping to further processing.

    PRINCIPLE: 
        For every section/directive, there is a data-container struct defined in File.h.

        Every time a line of a particular section is parsed, an instance of the struct
        is saved into an array container in struct RigDef::File. There are exceptions to this rule.
        If the section is subject to adjustable defaults (like 'beams' to 'set_beam_defaults'),
        the default-data-container is attached to section-data-container.

        Optional sections (for example 'SlopeBrake') are represented by a dynamically allocated
        object. If the file doesn't contain the section, the pointer will be null.

        Whenever a directive is parsed, it's contents are stored to an instance of it's container
        struct, located in Parser.
        For every directive, there are two containers:
        * 'ror_*' represents game defaults as specified in documentation. Needed for resetting.
        * 'user_*' represent the last defaults specified in the .truck file.
        Again, there are exceptions to this rule.
*/

class Parser
{

public:

    static const int LINE_BUFFER_LENGTH = 2000;
    static const int LINE_MAX_ARGS = 100;

    struct Message
    {
        enum Type
        {
            TYPE_WARNING,
            TYPE_ERROR,
            TYPE_FATAL_ERROR,

            TYPE_INVALID = 0xFFFFFFFF
        };

        std::string      line;
        unsigned int     line_number;
        std::string      message;
        File::Section    section;
        File::Subsection subsection;
        Type             type;
        Ogre::String     module;
    };

    struct Token
    {
        const char* start;
        int         length;
    };

    Parser();

    void Prepare();
    void Finalize();
    void ProcessOgreStream(Ogre::DataStream* stream);
    void ProcessRawLine(const char* line);

    std::list<Message> const & GetMessages()
    {
        return m_messages;
    }

    std::shared_ptr<RigDef::File> GetFile()
    {
        return m_definition;
    }

    SequentialImporter* GetSequentialImporter() { return &m_sequential_importer; }

    std::string ProcessMessagesToString();

    int GetMessagesNumErrors()   const { return m_messages_num_errors;   }
    int GetMessagesNumWarnings() const { return m_messages_num_warnings; }
    int GetMessagesNumOther()    const { return m_messages_num_other;    }

protected:

// --------------------------------------------------------------------------
//  Directive parsers
// --------------------------------------------------------------------------

    void ParseDirectiveAddAnimation();

    void ParseDirectiveDetacherGroup();

    void ParseDirectiveFlexbodyCameraMode();

    void ParseDirectivePropCameraMode();

    void ParseDirectiveSetBeamDefaults();

    void ParseDirectiveSetBeamDefaultsScale();

    void ParseDirectiveSetInertiaDefaults();

    void ParseDirectiveSetManagedMaterialsOptions();

    void ParseDirectiveSetNodeDefaults();
    void LogParsedDirectiveSetNodeDefaultsData(float loadweight, float friction, float volume, float surface, unsigned int options);

// --------------------------------------------------------------------------
//  Section parsers
// --------------------------------------------------------------------------

    void ParseAirbrakes();

    void ParseAnimator();

    void ParseAntiLockBrakes();

    void ParseAuthor();

    void ParseAxles();

    void ParseBeams();

    void ParseBrakes();

    void ParseCameras();

    void ParseCameraRails();

    void ParseCinecam();

    void ParseCollisionBox();

    void ParseCommandsUnified();

    void ParseContacter();

    void ParseCruiseControl();

    void ParseEngine();

    void ParseEngoption();

    void ParseEngturbo();

    void ParseExhaust();

    void ParseExtCamera();

    void ParseFileFormatVersion();

    void ParseFileinfo();

    void ParseFixes();

    void ParseFlaresUnified();

    void ParseFlexbody();

    void ParseFlexBodyWheel();

    void ParseFusedrag();

    void ParseGlobals();

    void ParseGuid();

    void ParseGuiSettings();

    void ParseHelp();

    void ParseHook();

    void ParseHydros();

    void ParseLockgroups();

    void ParseManagedMaterials();

    void ParseMaterialFlareBindings();

    void ParseMeshWheelUnified();

    void ParseMinimass();

    void ParseNodesUnified();

    void ParseNodeCollision();

    void ParseParticles();

    void ParsePistonprops();

    void ParseProps();

    void ParseRailGroups();

    void ParseRopables();

    void ParseRopes();

    void ParseRotatorsUnified();

    void ParseScrewprops();

    void ParseSetCollisionRange();

    void ParseSetSkeletonSettings();

    void ParseShock();

    void ParseShock2();

    void ParseSlidenodes();

    void ParseSlopeBrake();

    void ParseSoundsources();

    void ParseSoundsources2();

    void ParseSpeedLimiter();

    void ParseSubmesh();

    void ParseSubmeshGroundModel();

    void ParseTies();

    void ParseTorqueCurve();

    void ParseTractionControl();

    void ParseTriggers();

    void ParseTurbojets();

    void ParseTurbopropsUnified();

    void ParseVideoCamera();

    void ParseWheelDetachers();

    void ParseWheel();

    void ParseWheel2();

    void ParseWing();

// --------------------------------------------------------------------------
//  Utilities
// --------------------------------------------------------------------------

    void             ProcessCurrentLine();
    int              TokenizeCurrentLine();
    bool             CheckNumArguments(int num_required_args);

    std::string        GetArgStr          (int index);
    int                GetArgInt          (int index);
    unsigned           GetArgUint         (int index);
    long               GetArgLong         (int index);
    float              GetArgFloat        (int index);
    char               GetArgChar         (int index);
    bool               GetArgBool         (int index);
    Wheels::Propulsion GetArgPropulsion   (int index);
    Wheels::Braking    GetArgBraking      (int index);
    Node::Ref          GetArgNodeRef      (int index);
    Node::Ref          GetArgRigidityNode (int index);
    Node::Ref          GetArgNullableNode (int index);
    MeshWheel::Side    GetArgWheelSide    (int index);
    Wing::Control      GetArgWingSurface  (int index);
    Flare2::Type       GetArgFlareType    (int index);
    std::string        GetArgManagedTex   (int index);

    float              ParseArgFloat      (const char* str);
    int                ParseArgInt        (const char* str);
    unsigned           ParseArgUint       (const char* str);

    unsigned           ParseArgUint       (const std::string& s);
    float              ParseArgFloat      (const std::string& s);

    /// Attempts to parse cab line. Returns true if successful.
    bool _TryParseCab(Ogre::String const & line);

    void _CheckInvalidTrailingText(Ogre::String const & line, std::smatch const & results, unsigned int index);

    /// Keyword scan function. 
    File::Keyword IdentifyKeyword(Ogre::String const & line);

    /// Keyword scan utility function. 
    File::Keyword FindKeywordMatch(std::smatch& search_results);

    /// Adds a message to parser report.
    void AddMessage(std::string const & line, Message::Type type, std::string const & message);
    void AddMessage(Message::Type type, const char* msg)
    {
        this->AddMessage(m_current_line, type, msg);
    }
    void AddMessage(Message::Type type, std::string const & msg)
    {
        this->AddMessage(m_current_line, type, msg);
    }

    /// Print a log INFO message.
    void _PrintNodeDataForVerification(Ogre::String& line, Ogre::StringVector& args, int num_args, Node& node);

    static void _TrimTrailingComments(std::string const & line_in, std::string & line_out);

    Node::Ref _ParseNodeRef(std::string const & node_id_str);

    void _ParseCameraSettings(CameraSettings & camera_settings, Ogre::String input_str);

    void _ParseNodeOptions(unsigned int & options, const std::string & options_str);

    std::pair<bool, Ogre::String> GetModuleName(Ogre::String const & line);

    void SetCurrentModule(Ogre::String const & name);

    void _ExitSections(Ogre::String const & line);

    void RestoreRootModule();

    bool _IsCurrentModuleRoot()
    {
        return m_root_module == m_current_module;
    }

    void ParseOptionalInertia(Inertia& inertia, int index);

// --------------------------------------------------------------------------

    // RoR defaults

    std::shared_ptr<Inertia>             m_ror_default_inertia;
    std::shared_ptr<NodeDefaults>        m_ror_node_defaults;

    // Data from user directives
    // Each affected section-struct has a shared_ptr to it's respective defaults
    std::shared_ptr<Inertia>             m_user_default_inertia;
    std::shared_ptr<BeamDefaults>        m_user_beam_defaults;
    std::shared_ptr<NodeDefaults>        m_user_node_defaults;
    int                                  m_current_detacher_group;
    ManagedMaterialsOptions              m_current_managed_material_options;

    // Parser state
    std::shared_ptr<File::Module>        m_root_module;
    std::shared_ptr<File::Module>        m_current_module;

    unsigned int                         m_current_line_number;
    char                                 m_current_line[LINE_BUFFER_LENGTH];
    Token                                m_args[LINE_MAX_ARGS];    ///< Tokens of current line.
    int                                  m_num_args;               ///< Number of tokens on current line.
    File::Section                        m_current_section;        ///< Parser state.
    File::Subsection                     m_current_subsection;     ///< Parser state.
    bool                                 m_in_block_comment;       ///< Parser state.
    bool                                 m_in_description_section; ///< Parser state.
    bool                                 m_any_named_node_defined; ///< Parser state.
    std::shared_ptr<Submesh>             m_current_submesh;        ///< Parser state.
    std::shared_ptr<CameraRail>          m_current_camera_rail;    ///< Parser state.
    std::shared_ptr<Flexbody>            m_last_flexbody;

    SequentialImporter                   m_sequential_importer;

    std::shared_ptr<RigDef::File>        m_definition;
    std::list<Message>                   m_messages;
    int                                  m_messages_num_errors;
    int                                  m_messages_num_warnings;
    int                                  m_messages_num_other;
};

} // namespace RigDef
