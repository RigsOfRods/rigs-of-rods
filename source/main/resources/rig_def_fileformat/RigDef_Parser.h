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
/// @brief Checks the rig-def file syntax and loads data to memory

#pragma once

#include "Console.h"
#include "RigDef_Prerequisites.h"
#include "RigDef_File.h"
#include "RigDef_SequentialImporter.h"

#include <memory>
#include <string>
#include <regex>

namespace RigDef
{

///  @class  Parser
///  @author Petr Ohlidal
///
///  @brief Checks the rig-def file syntax and pulls data to File object
///
///  For every section/directive, there is a data-container struct defined in File.h.
///  The Parser should preferably only read data as-is, without validation.
///
///  Every time a line of a particular section is parsed, an instance of the struct
///  is saved into an array container in struct RigDef::Document. There are exceptions to this rule.
///
///  Keywords 'set_[node|beam|inertia]_defaults' are 'presets' and are managed by dyn. allocated
///  objects. For every preset, there are 2 pointers:
///      * 'ror_*' represents game defaults as specified in documentation. Needed for resetting.
///      * 'user_*' represent the last defaults specified in the .truck file.
class Parser
{

public:

    static const int LINE_BUFFER_LENGTH = 2000;
    static const int LINE_MAX_ARGS = 100;

    struct Token
    {
        const char* start;
        int         length;
    };

    Parser();

    void Prepare();
    void Finalize();
    void ProcessOgreStream(Ogre::DataStream* stream, Ogre::String resource_group);
    void ProcessRawLine(const char* line);

    RigDef::DocumentPtr GetFile()
    {
        return m_definition;
    }

    SequentialImporter* GetSequentialImporter() { return &m_sequential_importer; }

    // Common for truckfiles and addonparts:
    static void     ProcessForsetLine(RigDef::Flexbody& def, const std::string& line, int line_number = -1);
    static Keyword  IdentifyKeyword(const std::string& line);
    static SpecialProp IdentifySpecialProp(const std::string& str);

private:

// --------------------------------------------------------------------------
//  Directive parsers
// --------------------------------------------------------------------------

    void ProcessGlobalDirective(Keyword keyword); //!< Directives that should only appear in root module
    void ParseDirectiveAddAnimation();
    void ParseDirectiveBackmesh();
    void ParseDirectiveDefaultSkin();
    void ParseDirectiveDetacherGroup();
    void ParseDirectiveFlexbodyCameraMode();
    void ParseDirectiveForset();
    void ParseDirectivePropCameraMode();
    void ParseDirectiveSection();
    void ParseDirectiveSectionConfig();
    void ParseDirectiveSetBeamDefaults();
    void ParseDirectiveSetBeamDefaultsScale();
    void ParseDirectiveSetDefaultMinimass();
    void ParseDirectiveSetInertiaDefaults();
    void ParseDirectiveSetManagedMaterialsOptions();
    void ParseDirectiveSetNodeDefaults();
    void ParseDirectiveSubmesh();

// --------------------------------------------------------------------------
//  Section parsers
// --------------------------------------------------------------------------

    void ParseAirbrakes();
    void ParseAnimator();
    void ParseAntiLockBrakes();
    void ParseAssetpacks();
    void ParseAuthor();
    void ParseAxles();
    void ParseBeams();
    void ParseBrakes();
    void ParseCab();
    void ParseCameras();
    void ParseCameraRails();
    void ParseCinecam();
    void ParseCollisionBox();
    void ParseCommandsUnified();
    void ParseContacter();
    void ParseCruiseControl();
    void ParseDescription();
    void ParseEngine();
    void ParseEngoption();
    void ParseEngturbo();
    void ParseExhaust();
    void ParseExtCamera();
    void ParseFileFormatVersion();
    void ParseFileinfo();
    void ParseFixes();
    void ParseFlaresUnified();
    void ParseFlares3();
    void ParseFlexbody();
    void ParseFlexBodyWheel();
    void ParseFusedrag();
    void ParseGlobals();
    void ParseGuid();
    void ParseGuiSettings();
    void ParseHelp();
    void ParseHook();
    void ParseHydros();
    void ParseInterAxles();
    void ParseLockgroups();
    void ParseManagedMaterials();
    void ParseMaterialFlareBindings();
    void ParseMeshWheel();
    void ParseMeshWheel2();
    void ParseMinimass();
    void ParseNodesUnified();
    void ParseParticles();
    void ParsePistonprops();
    void ParseProps();
    void ParseRailGroups();
    void ParseRopables();
    void ParseRopes();
    void ParseRotatorsUnified();
    void ParseScrewprops();
    void ParseScripts();
    void ParseSetCollisionRange();
    void ParseSetSkeletonSettings();
    void ParseShock();
    void ParseShock2();
    void ParseShock3();
    void ParseSlidenodes();
    void ParseSoundsources();
    void ParseSoundsources2();
    void ParseSpeedLimiter();
    void ParseSubmeshGroundModel();
    void ParseTexcoords();
    void ParseTies();
    void ParseTorqueCurve();
    void ParseTractionControl();
    void ParseTransferCase();
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
    void             BeginBlock(RigDef::Keyword keyword);
    void             ProcessChangeModuleLine(Keyword keyword);

    std::string        GetArgStr          (int index);
    int                GetArgInt          (int index);
    unsigned           GetArgUint         (int index);
    long               GetArgLong         (int index);
    float              GetArgFloat        (int index);
    char               GetArgChar         (int index);
    bool               GetArgBool         (int index);
    RoR::WheelPropulsion GetArgPropulsion (int index);
    RoR::WheelBraking  GetArgBraking      (int index);
    Node::Ref          GetArgNodeRef      (int index);
    Node::Ref          GetArgRigidityNode (int index);
    Node::Ref          GetArgNullableNode (int index);
    RoR::WheelSide     GetArgWheelSide    (int index);
    WingControlSurface GetArgWingSurface  (int index);
    RoR::FlareType     GetArgFlareType    (int index);
    RoR::ExtCameraMode GetArgExtCameraMode(int index);
    std::string        GetArgManagedTex   (int index);
    MinimassOption     GetArgMinimassOption(int index);
    BitMask_t          GetArgCabOptions   (int index);
    BitMask_t          GetArgTriggerOptions(int index);
    BitMask_t          GetArgBeamOptions  (int index);
    BitMask_t          GetArgHydroOptions (int index);
    BitMask_t          GetArgShockOptions (int index);
    BitMask_t          GetArgShock2Options(int index);
    BitMask_t          GetArgShock3Options(int index);
    BitMask_t          GetArgNodeOptions  (int index);
    EngineType         GetArgEngineType   (int index);
    ManagedMaterialType GetArgManagedMatType(int index);

    float              ParseArgFloat      (const char* str);
    int                ParseArgInt        (const char* str);
    unsigned           ParseArgUint       (const char* str);
    unsigned           ParseArgUint       (const std::string& s);
    float              ParseArgFloat      (const std::string& s);

    /// Adds a message to console
    void LogMessage(RoR::Console::MessageType type, std::string const& msg);

    static void _TrimTrailingComments(std::string const & line_in, std::string & line_out);

    Node::Ref _ParseNodeRef(std::string const & node_id_str);
    void _ParseDifferentialTypes(DifferentialTypeVec& diff_types, std::string const& options_str);
    void _ParseBaseMeshWheel(BaseMeshWheel& mesh_wheel);

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
    std::shared_ptr<DefaultMinimass>     m_set_default_minimass;
    int                                  m_current_detacher_group;
    ManagedMaterialsOptions              m_current_managed_material_options;

    // Parser state
    std::shared_ptr<Document::Module>        m_root_module;
    std::shared_ptr<Document::Module>        m_current_module;

    unsigned int                         m_current_line_number;
    char                                 m_current_line[LINE_BUFFER_LENGTH];
    Token                                m_args[LINE_MAX_ARGS];    //!< Tokens of current line.
    int                                  m_num_args;               //!< Number of tokens on current line.
    Keyword                              m_current_block = Keyword::INVALID;
    Keyword                              m_log_keyword = Keyword::INVALID;
    bool                                 m_any_named_node_defined; //!< Parser state.
    std::shared_ptr<Submesh>             m_current_submesh;        //!< Parser state.
    std::shared_ptr<CameraRail>          m_current_camera_rail;    //!< Parser state.

    SequentialImporter                   m_sequential_importer;

    Ogre::String                         m_filename; // Logging
    Ogre::String                         m_resource_group;

    RigDef::DocumentPtr        m_definition;
};

} // namespace RigDef
