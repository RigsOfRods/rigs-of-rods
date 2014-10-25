/*

RIG INSPECTOR

Written 10/2014 by Petr "only_a_ptr" Ohlidal

This code logs all data contained within class Beam and associated data structs.

The purpose is to debug class RigSpawner, introduced in TruckParser2013 project 
	and replacing class SerializedRig.

*/

#pragma once

#include "RoRPrerequisites.h"
#include <iostream>

class RigInspector
{
public:
	
	static void InspectRig(Beam* rig, std::string const & out_path);

private:

	static void InspectFlexAirfoil(std::ofstream & f, FlexAirfoil & fa);

	static void InspectWings(std::ofstream & f, Beam* rig);

	static void InspectNodes(std::ofstream & f, Beam* rig);

	static void InspectBeams(std::ofstream & f, Beam* rig);

	static void InspectShocks(std::ofstream & f, Beam* rig);

	static void InspectWheels(std::ofstream & f, Beam* rig);

	static void InspectHooks(std::ofstream & f, Beam* rig);

	static void InspectCommandkey(std::ofstream & f, Beam* rig);

	static void InspectRotator(std::ofstream & f, Beam* rig);

	static void InspectProps(std::ofstream & f, Beam* rig);

	static void InspectAirbrakes(std::ofstream & f, Beam* rig);

	static void InspectEngine(std::ofstream & f, Beam* rig);

	static void InspectCollcabRate(std::ofstream & f, Beam* rig);

	static void PrintCollcabRate(std::ofstream & f, collcab_rate_t & data);

	static void InspectSoundsources(std::ofstream & f, Beam* rig);

	static void InspectRopables(std::ofstream & f, Beam* rig);

	static void PrintRopable(std::ofstream & f, ropable_t * ptr);

	static void InspectRopes(std::ofstream & f, Beam* rig);

	static void InspectTies(std::ofstream & f, Beam* rig);

	static void InspectContacters(std::ofstream & f, Beam* rig);

	static void InspectRigidifiers(std::ofstream & f, Beam* rig);

	static void InspectFlares(std::ofstream & f, Beam* rig);

	static void InspectExhausts(std::ofstream & f, Beam* rig);

	static void InspectAeroengines(std::ofstream & f, Beam* rig);

	static void PrintTurbojet(std::ofstream & f, Turbojet * ptr);

	static void InspectScrewprops(std::ofstream & f, Beam* rig);

	static void PrintFlexbody(std::ofstream & f, FlexBody* flexbody);

	static void InspectFlexbodies(std::ofstream & f, Beam* rig);

	static void InspectDashboardLayouts(std::ofstream & f, Beam* rig);

	static void InspectVideocameras(std::ofstream & f, Beam* rig);

	static void InspectAxles(std::ofstream & f, Beam* rig);

	static void PrintTurboprop(std::ofstream & f, Turboprop * ptr);

	// ========================================================

	static void InspectClassBeam(std::ofstream & f, Beam* rig);

	static void InspectStructRig(std::ofstream & f, Beam* rig);

};
