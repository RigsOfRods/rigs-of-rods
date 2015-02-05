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
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
	@file   RigDef_Prerequisites.h
	@author Petr Ohlidal
	@date   12/2013
*/

namespace RigDef
{

/* File structures declarations */
/* TODO: Complete list */

struct Airbrake;
struct Animation;
struct Axle;
struct Beam;
struct BeamDefaults;
struct BeamDefaultsScale;
struct CameraSettings;
struct Engine;
struct Engoption;
struct ExtCamera;
struct File;
struct Globals;
struct GuiSettings;
struct Inertia;
struct ManagedMaterialsOptions;
struct Node;
struct NodeDefaults;
struct OptionalInertia;
struct ShadowOptions;
struct VideoCamera;

/* Parser classes */

class Parser;
class Validator;

} /* namespace RigDef */