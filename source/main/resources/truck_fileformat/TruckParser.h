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

/// @file
/// @author Petr Ohlidal
/// @date   12/2013

#pragma once

#include "TruckFileFormat.h"

#include <memory>
#include <string>
#include <regex>

namespace RoR {
namespace Truck {

/// @class  Parser
/// @author Petr Ohlidal
///
/// Loads "truck" file into a sequence of RoR::Truck::SeqSection tokens,
/// each optionally referencing a data array. The tokens are kept in
/// RoR::Truck::Module::sequence array.
class Parser
{
public:

    Parser();

    static const int LINE_BUFFER_LENGTH = 2000;
    static const int LINE_MAX_ARGS = 100;

    struct Message // TODO: remove, use console API directly
    {
        enum Type
        {
            TYPE_WARNING,
            TYPE_ERROR,
            TYPE_FATAL_ERROR,

            TYPE_INVALID = 0xFFFFFFFF
        };
    };

    struct Token
    {
        const char* start;
        int         length;
    };

    void ProcessOgreStream(Ogre::DataStream* stream, Ogre::String resource_group);
    void ProcessRawLine(const char* line);

    Truck::DocumentPtr GetFile()
    {
        return m_definition;
    }

private:

// --------------------------------------------------------------------------
//  Directives
// --------------------------------------------------------------------------

    void ParseSimpleDirective();
    void ParseDirectiveAddAnimation();
    void ParseDirectiveBackmesh();
    void ParseDirectiveDetacherGroup();
    void ParseDirectiveFlexbodyCameraMode();
    void ParseDirectivePropCameraMode();
    void ParseDirectiveSetBeamDefaults();
    void ParseDirectiveSetBeamDefaultsScale();
    void ParseDirectiveSetDefaultMinimass();
    void ParseDirectiveSetInertiaDefaults();
    void ParseDirectiveSetManagedMaterialsOptions();
    void ParseDirectiveSetNodeDefaults();

// --------------------------------------------------------------------------
//  Sections
// --------------------------------------------------------------------------

    void ParseActorNameLine();
    void ParseAirbrakes();
    void ParseAnimator();
    void ParseAntiLockBrakes();
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
    void ParseForset();
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
    void ParseSectionconfig();
    void ParseSetCollisionRange();
    void ParseSetSkeletonSettings();
    void ParseShock();
    void ParseShock2();
    void ParseShock3();
    void ParseSlidenodes();
    void ParseSlopeBrake();
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
    void             BeginSegment();
    void             EndSegment();

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

    void _CheckInvalidTrailingText(Ogre::String const & line, std::smatch const & results, unsigned int index);

    /// Keyword scan function. 
    Keyword IdentifyKeywordInCurrentLine();

    /// Keyword scan utility function. 
    Keyword FindKeywordMatch(std::smatch& search_results);

    /// Adds a message to console
    void AddMessage(std::string const & line, Message::Type type, std::string const & message);
    void AddMessage(Message::Type type, const char* msg)
    {
        this->AddMessage(m_current_line, type, msg);
    }
    void AddMessage(Message::Type type, std::string const & msg)
    {
        this->AddMessage(m_current_line, type, msg);
    }


    static void _TrimTrailingComments(std::string const & line_in, std::string & line_out);

    Node::Ref _ParseNodeRef(std::string const & node_id_str);

    void ProcessCommentLine();

    void ParseOptionalInertia(OptionalInertia& inertia, int index);

    int GetCurrentEditorGroup(); //!< Determines group ID for current line

// --------------------------------------------------------------------------

    // Parser state - reading
    unsigned int                         m_current_line_number;
    char                                 m_current_line[LINE_BUFFER_LENGTH];
    Token                                m_args[LINE_MAX_ARGS];    //!< Tokens of current line.
    int                                  m_num_args;               //!< Number of tokens on current line.

    // Parser state - writing
    Truck::ModulePtr                     m_current_segment;
    Keyword                              m_current_keyword;        //!< Any keyword other than INVALID. END stops the parsing.
    Keyword                              m_current_section;        //!< Only section keywords. INVALID means top of file (truck name).
    Keyword                              m_saved_section;          //!< Only section keywords. Used to resume after segment or comment.

    Ogre::String                         m_filename; // Logging
    Ogre::String                         m_resource_group;

    Truck::DocumentPtr                   m_definition;
};

} // namespace Truck
} // namespace RoR
