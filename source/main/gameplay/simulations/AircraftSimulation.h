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
	@file   AircraftSimulation.h
	@date   06/2014
*/

#pragma once

#include "ForwardDeclarations.h"

namespace RoR
{

/**
* Stateless class which provides simulation logic.
*/
struct AircraftSimulation
{
	/**
	* SIM-CORE: 1x per frame. Logic: input, GUI, vehicle state.
	*/
	static void UpdateVehicle(Beam* vehicle, float seconds_since_last_frame);
};

} // namespace RoR