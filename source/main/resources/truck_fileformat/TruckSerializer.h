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

#include "TruckFileFormat.h"

namespace RoR {
namespace Truck{

/// @class  Serializer
/// @author Petr Ohlidal
///
/// @brief Serializes the `Truck::Document` data structure to string.
class Serializer
{
public:
    Serializer(Truck::DocumentPtr truck);
    void Serialize();
    std::string GetOutput() const { return m_stream.str(); }

private:
    void SerializeModule(Truck::ModulePtr m);

    void ProcessAuthors();
    void ProcessGlobals(Module* module);
    void ProcessDescription();
    void ProcessGuid();
    void ProcessFileinfo();
    void WriteFlags();
    void ProcessHelp(Module* module);

    // Audio video
    void ProcessCinecam(Module*);

    // Structure
    void ProcessNodes(Module*);
    void ProcessNode(Node & node);
    void ProcessNodeDefaults(NodeDefaults* node_defaults);
    void ProcessNodeOptions(unsigned int options);
    void ProcessMinimassPreset(MinimassPreset* minimass_preset);
    
    void ProcessBeams(Module*);
    void ProcessBeamDefaults(BeamDefaults* beam_defaults);
    void ProcessBeam(Beam & beam);

    void ProcessShocks(Module*);
    void ProcessShocks2(Module*);
    void ProcessShocks3(Module*);
    void ProcessShock(Shock & def);
    void ProcessShock2(Shock2 & def);
    void ProcessShock3(Shock3 & def);

    void ProcessHydros(Module*);
    void ProcessHydro(Hydro & def);
    void ProcessRotators(Module* module);
    void ProcessRotators2(Module* module);

    void ProcessCommands2(Module*);
    void ProcessCommand2(Command2 & def);
    void ProcessSlideNodes(Module* module);
    void ProcessRopes(Module* module);
    void ProcessFixes(Module* module);
    void ProcessTies(Module* module);

    void ProcessCameras(Module* module);

    // Land vehicle
    void ProcessEngine(Module* module);
    void ProcessEngoption(Module* module);
    void ProcessBrakes(Module* module);
    void ProcessAntiLockBrakes(Module* module);
    void ProcessTractionControl(Module* module);
    void ProcessSlopeBrake(Module* module);
    void ProcessTorqueCurve(Module* module);
    void ProcessCruiseControl(Module* module);
    void ProcessSpeedLimiter(Module* module);
    void ProcessAxles(Module* module);
    void ProcessTransferCase(Module* module);
    void ProcessInterAxles(Module* module);

    // Wheels
    void ProcessMeshWheels(Module* module); // And meshwheels2
    void ProcessWheels(Module* module);
    void ProcessWheels2(Module* module);
    void ProcessFlexBodyWheels(Module* module);

    // Features
    void ProcessAnimators(Module* module);
    void ProcessContacters(Module* module);
    void ProcessTriggers(Module* module);
    void ProcessLockgroups(Module* module);
    void ProcessHooks(Module* module);
    void ProcessRailGroups(Module* module);
    void ProcessRopables(Module* module);
    void ProcessParticles(Module* module);
    void ProcessCollisionBoxes(Module* module);
    void ProcessManagedMaterialsAndOptions(Module* module);
    void ProcessFlares2(Module* module);
    void ProcessMaterialFlareBindings(Module* module);
    void ProcessPropsAndAnimations(Module* module);
    void ProcessFlexbodies(Module* module);
    void ProcessDirectiveAddAnimation(Truck::Animation & anim);
    /* TODO: 
    5.5.17 Camerarail
    5.5.8 Flexbodies
        5.5.8.3 (sub-directive) disable_flexbody_shadow
        5.5.8.4 (sub-directive) flexbody_camera_mode
    */

    void ProcessSubmesh(Module* module);
    void ProcessSubmeshGroundmodel(Module* module);
    void ProcessExhausts(Module* module);
    void ProcessGuiSettings(Module* module);
    void ProcessSetSkeletonSettings(Module* module);
    void ProcessVideocamera(Module* module);
    void ProcessExtCamera(Module* module);
    void ProcessSoundsources(Module* module);
    void ProcessSoundsources2(Module* module);

    // Aerial
    void ProcessWings(Module* module);
    void ProcessAirbrakes(Module* module);
    void ProcessTurboprops(Module* module);
    void ProcessFusedrag(Module* module);
    void ProcessTurbojets(Module* module);
    void ProcessPistonprops(Module* module);

    // Marine
    void ProcessScrewprops(Module* module);

protected:

    std::string         RigidityNodeToStr(Node::Ref node) { return (node.IsValidAnyState()) ? node.Str() : "9999"; }

    // Presets = `set_[node/beam]_defaults`, `set_default_minimass`
    void                ResetPresets();
    void                UpdatePresets(BeamDefaults* beam, NodeDefaults* node, MinimassPreset* minimass);

    // Groups = `;grp:NAME` comments
    void                ResetGroup();
    void                UpdateGroup(Module* module, int group_id);

    std::stringstream  m_stream;
    Truck::DocumentPtr m_truck;
    // Settings
    int                m_float_precision;
    int                m_float_width;
    int                m_bool_width;
    int                m_node_id_width;
    int                m_command_key_width;
    int                m_inertia_function_width;
    // State
    BeamDefaults*      m_current_beam_preset = nullptr;
    NodeDefaults*      m_current_node_preset = nullptr;
    MinimassPreset*    m_current_minimass_preset = nullptr;
    int                m_current_editor_group = -1;
};

} // namespace Truck
} // namespace RoR
