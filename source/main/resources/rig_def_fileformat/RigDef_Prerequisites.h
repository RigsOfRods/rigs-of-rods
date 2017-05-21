/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

namespace Ogre
{
    class DataStream;
}

namespace RigDef
{

// File structures declaration

struct CameraSettings;
struct NodeDefaults;
struct BeamDefaultsScale;
struct BeamDefaults;
struct Inertia;
struct ManagedMaterialsOptions;
struct ShadowOptions;
struct Globals;
struct GuiSettings;
struct Airbrake;
struct Animation;
struct Axle;
struct Beam;
struct Camera;
struct Cinecam;
struct CollisionBox;
struct CruiseControl;
struct Author;
struct Fileinfo;
struct Engine;
struct Engoption;
struct Engturbo;
struct Exhaust;
struct ExtCamera;
struct Brakes;
struct AntiLockBrakes;
struct TractionControl;
struct SlopeBrake;
struct WheelDetacher;
struct Wheel;
struct Wheel2;
struct MeshWheel;
struct Flare2;
struct Flexbody;
struct FlexBodyWheel;
struct Fusedrag;
struct Hook;
struct Shock;
struct Shock2;
struct SkeletonSettings;
struct Hydro;
struct AeroAnimator;
struct Animator;
struct Command2;
struct Rotator;
struct Rotator2;
struct Trigger;
struct Lockgroup;
struct ManagedMaterial;
struct MaterialFlareBinding;
struct NodeCollision;
struct Particle;
struct Pistonprop;
struct Prop;
struct RailGroup;
struct Ropable;
struct Rope;
struct Screwprop;
struct SlideNode;
struct SoundSource;
struct SoundSource2;
struct SpeedLimiter;
struct Cab;
struct Texcoord;
struct Submesh;
struct Tie;
struct TorqueCurve;
struct Turbojet;
struct Turboprop2;
struct VideoCamera;
struct Wing;
struct File;

// Parser classes

class Parser;
class Validator;
class SequentialImporter;

} // namespace RigDef
