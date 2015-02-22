/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   LandVehicleSimulation.h
	@date   05/2014
*/

#pragma once

#include "ForwardDeclarations.h"

namespace RoR
{

/**
* Stateless class which provides simulation logic.
*/
struct LandVehicleSimulation
{
	/**
	* @param dt Delta time in seconds.
	*/
	static void UpdateCruiseControl(Beam* curr_truck, float dt);

	/**
	* @param dt Delta time in seconds.
	*/
	static void CheckSpeedLimit(Beam* curr_truck, float dt);

	/**
	* Logic: input, sound, vehicle state
	*/
	static void UpdateVehicle(Beam* curr_truck, float seconds_since_last_frame);
};

} // namespace RoR