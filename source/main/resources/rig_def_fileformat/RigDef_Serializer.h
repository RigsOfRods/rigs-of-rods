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
/// @date   10/2014

#pragma once

#include "RigDef_File.h"

namespace RigDef
{

/// @class  Serializer
/// @author Petr Ohlidal
///
/// @brief Serializes the `RigDef::File` data structure to string.
class Serializer
{
public:
    Serializer(RigDef::DocumentPtr rig_def);
    void Serialize();
    std::string GetOutput() const { return m_stream.str(); }

private:
    void SerializeModule(std::shared_ptr<RigDef::Document::Module> m);

    void ProcessAuthors(Document::Module* module);
    void ProcessGlobals(Document::Module* module);
    void ProcessDescription(Document::Module* module);
    void ProcessGuid(Document::Module* module);
    void ProcessFileinfo(Document::Module* module);
    void WriteFlags();
    void ProcessHelp(Document::Module* module);

    // Audio video
    void ProcessCinecam(Document::Module*);

    // Structure
    void ProcessNodes(Document::Module*);
    void ProcessNode(Node & node);
    void ProcessNodeDefaults(NodeDefaults* node_defaults);
    void ProcessNodeOptions(unsigned int options);
    void ProcessDefaultMinimass(DefaultMinimass* default_minimass);
    
    void ProcessBeams(Document::Module*);
    void ProcessBeamDefaults(BeamDefaults* beam_defaults);
    void ProcessBeam(Beam & beam);

    void ProcessShocks(Document::Module*);
    void ProcessShocks2(Document::Module*);
    void ProcessShocks3(Document::Module*);
    void ProcessShock(Shock & def);
    void ProcessShock2(Shock2 & def);
    void ProcessShock3(Shock3 & def);

    void ProcessHydros(Document::Module*);
    void ProcessHydro(Hydro & def);
    void ProcessRotators(Document::Module* module);
    void ProcessRotators2(Document::Module* module);

    void ProcessCommands2(Document::Module*);
    void ProcessCommand2(Command2 & def);
    void ProcessSlideNodes(Document::Module* module);
    void ProcessRopes(Document::Module* module);
    void ProcessFixes(Document::Module* module);
    void ProcessTies(Document::Module* module);

    void ProcessCameras(Document::Module* module);

    // Land vehicle
    void ProcessEngine(Document::Module* module);
    void ProcessEngoption(Document::Module* module);
    void ProcessBrakes(Document::Module* module);
    void ProcessAntiLockBrakes(Document::Module* module);
    void ProcessTractionControl(Document::Module* module);
    void ProcessTorqueCurve(Document::Module* module);
    void ProcessCruiseControl(Document::Module* module);
    void ProcessSpeedLimiter(Document::Module* module);
    void ProcessAxles(Document::Module* module);
    void ProcessTransferCase(Document::Module* module);
    void ProcessInterAxles(Document::Module* module);

    // Wheels
    void ProcessMeshWheels(Document::Module* module);
    void ProcessMeshWheels2(Document::Module* module);
    void ProcessWheels(Document::Module* module);
    void ProcessWheels2(Document::Module* module);
    void ProcessFlexBodyWheels(Document::Module* module);

    // Features
    void ProcessAnimators(Document::Module* module);
    void ProcessContacters(Document::Module* module);
    void ProcessTriggers(Document::Module* module);
    void ProcessLockgroups(Document::Module* module);
    void ProcessHooks(Document::Module* module);
    void ProcessRailGroups(Document::Module* module);
    void ProcessRopables(Document::Module* module);
    void ProcessParticles(Document::Module* module);
    void ProcessCollisionBoxes(Document::Module* module);
    void ProcessManagedMaterialsAndOptions(Document::Module* module);
    void ProcessFlares2(Document::Module* module);
    void ProcessMaterialFlareBindings(Document::Module* module);
    void ProcessPropsAndAnimations(Document::Module* module);
    void ProcessFlexbodies(Document::Module* module);
    void ProcessDirectiveAddAnimation(RigDef::Animation & anim);
    /* TODO: 
    5.5.17 Camerarail
    5.5.8 Flexbodies
        5.5.8.3 (sub-directive) disable_flexbody_shadow
        5.5.8.4 (sub-directive) flexbody_camera_mode
    */

    void ProcessSubmesh(Document::Module* module);
    void ProcessSubmeshGroundmodel(Document::Module* module); // STUB!!!
    void ProcessExhausts(Document::Module* module);
    void ProcessGuiSettings(Document::Module* module);
    void ProcessSetSkeletonSettings(Document::Module* module);
    void ProcessVideocamera(Document::Module* module);
    void ProcessExtCamera(Document::Module* module);
    void ProcessSoundsources(Document::Module* module);
    void ProcessSoundsources2(Document::Module* module);

    // Aerial
    void ProcessWings(Document::Module* module);
    void ProcessAirbrakes(Document::Module* module);
    void ProcessTurboprops(Document::Module* module);
    void ProcessFusedrag(Document::Module* module); // STUB!!!
    void ProcessTurbojets(Document::Module* module);
    void ProcessPistonprops(Document::Module* module);

    // Marine
    void ProcessScrewprops(Document::Module* module);

protected:

    std::string         RigidityNodeToStr(Node::Ref node) { return (node.IsValidAnyState()) ? node.Str() : "9999"; }

    void ExportBaseMeshWheel(BaseMeshWheel& def);

    // Presets, i.e. `set_[node/beam]_defaults`, `set_default_minimass`
    void                ResetPresets();
    void                UpdatePresets(BeamDefaults* beam, NodeDefaults* node, DefaultMinimass* minimass);

    std::stringstream                 m_stream;
    RigDef::DocumentPtr               m_rig_def;
    // Settings
    int                               m_float_precision;
    int                               m_float_width;
    int                               m_bool_width;
    int                               m_node_id_width;
    int                               m_command_key_width;
    int                               m_inertia_function_width;
    // State
    BeamDefaults*     m_current_beam_defaults = nullptr;
    NodeDefaults*     m_current_node_defaults = nullptr;
    DefaultMinimass*  m_current_default_minimass = nullptr;
};

} // namespace RigDef
