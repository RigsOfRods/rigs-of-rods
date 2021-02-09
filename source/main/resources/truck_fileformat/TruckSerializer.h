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

namespace Truck{

/// @class  Serializer
/// @author Petr Ohlidal
///
/// @brief Serializes the `Truck::File` data structure to string.
class Serializer
{
public:
    Serializer(std::shared_ptr<Truck::File> rig_def);
    void Serialize();
    std::string GetOutput() const { return m_stream.str(); }

private:
    void SerializeModule(std::shared_ptr<Truck::File::Module> m);

    void ProcessAuthors();
    void ProcessGlobals(File::Module* module);
    void ProcessDescription();
    void ProcessGuid();
    void ProcessFileinfo();
    void WriteFlags();
    void ProcessHelp(File::Module* module);

    // Audio video
    void ProcessCinecam(File::Module*);

    // Structure
    void ProcessNodes(File::Module*);
    void ProcessNode(Node & node);
    void ProcessNodeDefaults(NodeDefaults* node_defaults);
    void ProcessNodeOptions(unsigned int options);
    void ProcessMinimassPreset(MinimassPreset* minimass_preset);
    
    void ProcessBeams(File::Module*);
    void ProcessBeamDefaults(BeamDefaults* beam_defaults);
    void ProcessBeam(Beam & beam);

    void ProcessShocks(File::Module*);
    void ProcessShocks2(File::Module*);
    void ProcessShocks3(File::Module*);
    void ProcessShock(Shock & def);
    void ProcessShock2(Shock2 & def);
    void ProcessShock3(Shock3 & def);

    void ProcessHydros(File::Module*);
    void ProcessHydro(Hydro & def);
    void ProcessRotators(File::Module* module);
    void ProcessRotators2(File::Module* module);

    void ProcessCommands2(File::Module*);
    void ProcessCommand2(Command2 & def);
    void ProcessSlideNodes(File::Module* module);
    void ProcessRopes(File::Module* module);
    void ProcessFixes(File::Module* module);
    void ProcessTies(File::Module* module);

    void ProcessCameras(File::Module* module);

    // Land vehicle
    void ProcessEngine(File::Module* module);
    void ProcessEngoption(File::Module* module);
    void ProcessBrakes(File::Module* module);
    void ProcessAntiLockBrakes(File::Module* module);
    void ProcessTractionControl(File::Module* module);
    void ProcessSlopeBrake(File::Module* module);
    void ProcessTorqueCurve(File::Module* module);
    void ProcessCruiseControl(File::Module* module);
    void ProcessSpeedLimiter(File::Module* module);
    void ProcessAxles(File::Module* module);
    void ProcessTransferCase(File::Module* module);
    void ProcessInterAxles(File::Module* module);

    // Wheels
    void ProcessMeshWheels(File::Module* module); // And meshwheels2
    void ProcessWheels(File::Module* module);
    void ProcessWheels2(File::Module* module);
    void ProcessFlexBodyWheels(File::Module* module);

    // Features
    void ProcessAnimators(File::Module* module);
    void ProcessContacters(File::Module* module);
    void ProcessTriggers(File::Module* module);
    void ProcessLockgroups(File::Module* module);
    void ProcessHooks(File::Module* module);
    void ProcessRailGroups(File::Module* module);
    void ProcessRopables(File::Module* module);
    void ProcessParticles(File::Module* module);
    void ProcessCollisionBoxes(File::Module* module);
    void ProcessManagedMaterialsAndOptions(File::Module* module);
    void ProcessFlares2(File::Module* module);
    void ProcessMaterialFlareBindings(File::Module* module);
    void ProcessPropsAndAnimations(File::Module* module);
    void ProcessFlexbodies(File::Module* module);
    void ProcessDirectiveAddAnimation(Truck::Animation & anim);
    /* TODO: 
    5.5.17 Camerarail
    5.5.8 Flexbodies
        5.5.8.3 (sub-directive) disable_flexbody_shadow
        5.5.8.4 (sub-directive) flexbody_camera_mode
    */

    void ProcessSubmesh(File::Module* module);
    void ProcessSubmeshGroundmodel(File::Module* module); // STUB!!!
    void ProcessExhausts(File::Module* module);
    void ProcessGuiSettings(File::Module* module);
    void ProcessSetSkeletonSettings(File::Module* module);
    void ProcessVideocamera(File::Module* module);
    void ProcessExtCamera(File::Module* module);
    void ProcessSoundsources(File::Module* module);
    void ProcessSoundsources2(File::Module* module);

    // Aerial
    void ProcessWings(File::Module* module);
    void ProcessAirbrakes(File::Module* module);
    void ProcessTurboprops(File::Module* module);
    void ProcessFusedrag(File::Module* module); // STUB!!!
    void ProcessTurbojets(File::Module* module);
    void ProcessPistonprops(File::Module* module);

    // Marine
    void ProcessScrewprops(File::Module* module);

protected:

    std::string         RigidityNodeToStr(Node::Ref node) { return (node.IsValidAnyState()) ? node.Str() : "9999"; }

    // Presets = `set_[node/beam]_defaults`, `set_default_minimass`
    void                ResetPresets();
    void                UpdatePresets(BeamDefaults* beam, NodeDefaults* node, MinimassPreset* minimass);

    // Groups = `;grp:NAME` comments
    void                ResetGroup();
    void                UpdateGroup(File::Module* module, int group_id);

    std::stringstream                 m_stream;
    std::shared_ptr<Truck::File>     m_rig_def;
    // Settings
    int                               m_float_precision;
    int                               m_float_width;
    int                               m_bool_width;
    int                               m_node_id_width;
    int                               m_command_key_width;
    int                               m_inertia_function_width;
    // State
    BeamDefaults*     m_current_beam_preset = nullptr;
    NodeDefaults*     m_current_node_preset = nullptr;
    MinimassPreset*   m_current_minimass_preset = nullptr;
    int               m_current_editor_group = -1;
};

} // namespace Truck
