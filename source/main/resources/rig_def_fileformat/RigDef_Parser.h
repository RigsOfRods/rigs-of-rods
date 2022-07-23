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

    void Prepare();
    void Finalize();
    void ProcessOgreStream(Ogre::DataStream* stream, Ogre::String resource_group);
    void ProcessRawLine(const char* line);

    RigDef::DocumentPtr GetFile()
    {
        return m_document;
    }

private:

// --------------------------------------------------------------------------
//  Directive parsers
// --------------------------------------------------------------------------

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

// --------------------------------------------------------------------------
//  Section parsers
// --------------------------------------------------------------------------

    void ParseAirbrakes();
    void ParseAnimators();
    void ParseAntiLockBrakes();
    void ParseAuthor();
    void ParseAxles();
    void ParseBeams();
    void ParseBrakes();
    void ParseCab();
    void ParseCameras();
    void ParseCamerarails();
    void ParseCinecam();
    void ParseCollisionboxes();
    void ParseCommands();
    void ParseCommands2();
    void ParseContacters();
    void ParseCruiseControl();
    void ParseDescription();
    void ParseEngine();
    void ParseEngoption();
    void ParseEngturbo();
    void ParseExhausts();
    void ParseExtCamera();
    void ParseFileFormatVersion();
    void ParseFileinfo();
    void ParseFixes();
    void ParseFlares();
    void ParseFlares2();
    void ParseFlares3();
    void ParseFlexbodies();
    void ParseFlexbodywheels();
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
    void ParseMeshwheels();
    void ParseMeshwheels2();
    void ParseMinimass();
    void ParseNodes();
    void ParseNodes2();
    void ParseParticles();
    void ParsePistonprops();
    void ParseProps();
    void ParseRailGroups();
    void ParseRopables();
    void ParseRopes();
    void ParseRotators();
    void ParseRotators2();
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
    void ParseTurboprops();
    void ParseTurboprops2();
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
    Keyword          IdentifyKeywordInCurrentLine();
    bool             CheckNumArguments(int num_required_args);
    void             BeginBlock(RigDef::Keyword keyword);
    void             EndBlock(RigDef::Keyword keyword);

    std::string        GetArgStr          (int index);
    int                GetArgInt          (int index);
    unsigned           GetArgUint         (int index);
    long               GetArgLong         (int index);
    float              GetArgFloat        (int index);
    char               GetArgChar         (int index);
    bool               GetArgBool         (int index);
    WheelPropulsion    GetArgPropulsion   (int index);
    WheelBraking       GetArgBraking      (int index);
    NodeRef_t          GetArgNodeRef      (int index);
    NodeRef_t          GetArgRigidityNode (int index);
    NodeRef_t          GetArgNullableNode (int index);
    WheelSide          GetArgWheelSide    (int index);
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
    SpecialProp        GetArgSpecialProp  (int index);
    EngineType         GetArgEngineType   (int index);
    ManagedMaterialType GetArgManagedMatType(int index);

    float              ParseArgFloat      (const char* str);
    int                ParseArgInt        (const char* str);
    unsigned           ParseArgUint       (const char* str);
    unsigned           ParseArgUint       (const std::string& s);
    float              ParseArgFloat      (const std::string& s);

    /// Keyword scan utility function. 
    Keyword FindKeywordMatch(std::smatch& search_results);

    /// Adds a message to console
    void LogMessage(RoR::Console::MessageType type, std::string const& msg);

    static void _TrimTrailingComments(std::string const & line_in, std::string & line_out);

    NodeRef_t _ParseNodeRef(std::string const & node_id_str);
    void _ParseDifferentialTypes(DifferentialTypeVec& diff_types, std::string const& options_str);


    void ParseOptionalInertia(Inertia& inertia, int index);

    void ParseCommandOptions(CommandCommon& command2, std::string const& options_str);

// --------------------------------------------------------------------------



    unsigned int                         m_current_line_number;
    char                                 m_current_line[LINE_BUFFER_LENGTH];
    Token                                m_args[LINE_MAX_ARGS];    //!< Tokens of current line.
    int                                  m_num_args;               //!< Number of tokens on current line.
    Keyword                              m_current_block = Keyword::INVALID;
    Keyword                              m_log_keyword = Keyword::INVALID;



    Ogre::String                         m_filename; // Logging
    Ogre::String                         m_resource_group;

    RigDef::DocumentPtr                  m_document;
};

} // namespace RigDef
