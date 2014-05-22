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
	@file   ForwardDeclarations.h
	@brief  Global forward declarations.
	@author Petr Ohlidal
	@date   12/2013
*/

#pragma once

namespace RoR
{
	class  Application;
	class  Console;
	class  ContentManager;
	class  GUIManager;
	class  MainThread;
	class  OgreSubsystem;
	struct PlatformUtils;
	class  SceneMouse;
}

struct node_t;
struct beam_t;
struct shock_t;
struct collcab_rate_t;
struct soundsource_t;
struct contacter_t;
struct rigidifier_t;
struct wheel_t;
struct vwheel_t;
struct ropable_t;
struct wing_t;
struct command_t;
struct rotator_t;
struct flare_t;
struct prop_t;
struct rope_t;
struct exhaust_t;
struct cparticle_t;
struct debugtext_t;
struct rig_t;
struct collision_box_t;
struct tie_t;
struct hook_t;
struct ground_model_t;
struct client_t;
struct authorinfo_t;

namespace MOC
{
	class CollisionTools;
}

namespace Ogre
{
	class MovableText;
	class TerrainGroup;
}

namespace MyGUI
{
	class OgrePlatform;
}

class AeroEngine;
class Airbrake;
class Airfoil;
class AppState;
class AppStateManager;
class Autopilot;
class Axle;
class Beam;
class BeamEngine;
class BeamThreadStats;
class Buoyance;
class CacheEntry;
class CameraManager;
class Character;
class ChatSystem;
class CmdKeyInertia;
class Collisions;
class ColoredTextAreaOverlayElement;
class Dashboard;
class DashBoard;
class DashBoardManager;
class DOFManager;
class DotSceneLoader;
class DustPool;
class Editor;
class Envmap;
class Flexable;
class FlexAirfoil;
class FlexBody;
class FlexMesh;
class FlexObj;
class ForceFeedback;
class HDRListener;
class HeatHaze;
class HeightFinder;
class ICameraBehavior;
class IHeightFinder;
class IThreadTask;
class IWater;
class Landusemap;
class MapTextureCreator;
class MaterialFunctionMapper;
class MaterialReplacer;
class MeshObject;
class Mirrors;
class Network;
class OverlayWrapper;
class PointColDetector;
class PositionStorage;
class ProceduralManager;
class Rail;
class RailGroup;
class Replay;
class RigsOfRods;
class RigSpawner;
class Road2;
class Road;
class RoRFrameListener;
class ScopeLog;
class Screwprop;
class Skidmark;
class Skin;
class SkyManager;
class SlideNode;
class ShadowManager;
class SoundManager;
class SoundScriptInstance;
class SoundScriptManager;
class Streamable;
class SurveyMapManager;
class SurveyMapEntity;
class TerrainGeometryManager;
class TerrainHeightFinder;
class TerrainManager;
class TerrainObjectManager;
class ThreadPool;
class ThreadWorker;
class TorqueCurve;
class TruckEditor;
class TruckHUD;
class Turbojet;
class Turboprop;
class VideoCamera;
class Water;

#ifdef USE_SOCKETW
class SWBaseSocket;
#endif // USE_SOCKETW
