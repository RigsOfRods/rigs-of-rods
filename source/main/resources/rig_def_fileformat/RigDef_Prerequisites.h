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

#include <memory> //shared_ptr

namespace Ogre
{
    class DataStream;
}

namespace RigDef {

// File structures declarations
// TODO: Complete list

struct Document;
typedef std::shared_ptr<Document> DocumentPtr;

struct AeroAnimator;
struct Airbrake;
struct Animation;
struct AntiLockBrakes;
struct Axle;
struct Beam;
struct BeamDefaults;
struct BeamDefaultsScale;
struct Brakes;
struct Cab;
struct CameraSettings;
struct Cinecam;
struct CollisionBox;
struct Command2;
struct CruiseControl;
struct DefaultMinimass;
struct Engine;
struct Engoption;
struct Engturbo;
struct ExtCamera;
struct Flare;
struct Flare2;
struct Flexbody;
struct FlexBodyWheel;
struct Fusedrag;
struct Globals;
struct GuiSettings;
struct Hook;
struct Hydro;
struct Inertia;
struct Lockgroup;
struct ManagedMaterialsOptions;
struct MeshWheel;
struct Node;
struct NodeDefaults;
struct Particle;
struct Pistonprop;
struct Prop;
struct RailGroup;
struct Ropable;
struct ShadowOptions;
struct VideoCamera;

// Parser classes

class Parser;
class Validator;
class SequentialImporter;

} // namespace RigDef
