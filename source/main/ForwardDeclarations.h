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


/// @file   ForwardDeclarations.h
/// @brief  Global forward declarations.
/// @author Petr Ohlidal
/// @date   12/2013


#pragma once

namespace RoR
{
    class  ActorManager;
    class  CameraManager;
    class  ConfigFile;
    class  Console;
    class  ContentManager;
    class  FlexBodyFileIO;
    struct FlexBodyCacheData;
    class  FlexFactory;
    class  GfxActor;
    struct GfxCharacter;
    class  GfxScene;
    class  GUIManager;
    struct GuiManagerImpl;
    class  GuiManagerInterface;
    class  MainMenu;
    class  OgreSubsystem;
    struct PlatformUtils;
    class  RigLoadingProfiler;
    class  SceneMouse;
    class  Skidmark;
    class  SkidmarkConfig;
    struct SkinDef;
    class  SkinManager;
    struct Terrn2Author;
    struct Terrn2Def;
    class  Terrn2Parser;
    struct Terrn2Telepoint;

    namespace GUI
    {
        class  Dialog;
        class  FrictionSettings;
        class  GameMainMenu;
        class  GamePauseMenu;
        class  LoadingWindow;
        class  MainSelector;
        class  MpClientList;
        class  MultiplayerSelector;
        class  OpenSaveFileDialog;
        class  SimUtils;
        class  TeleportWindow;
        class  TopMenubar;
    }
} // namespace RoR

namespace MyGUI
{
    class  OgrePlatform;
}

namespace RoRnet
{
    struct Header;
    struct UserInfo;
    struct StreamRegister;
    struct ActorStreamRegister;
    struct ServerInfo;
    struct VehicleState;
}

struct node_t;
struct beam_t;
struct shock_t;
struct eventsource_t;
struct soundsource_t;
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
    class Camera;
    class ConfigFile;
    class MovableText;
    class Overlay;
    class RenderTarget;
    class TerrainGroup;
}

class AeroEngine;
class Airbrake;
class Airfoil;
class Autopilot;
class Actor;
class EngineSim;
class Buoyance;
class CacheEntry;
class CacheSystem;
class Character;
class ChatSystem;
class CmdKeyInertia;
class Collisions;
class ColoredTextAreaOverlayElement;
class Dashboard;
class DashBoard;
class DashBoardManager;
class Differential;
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
class HeightFinder;
class InputEngine;
class IWater;
class HydraxWater;
class Landusemap;
class MapTextureCreator;
class MeshObject;
class Mirror;
class MumbleIntegration;
class Network;
class OverlayWrapper;
class OutProtocol;
class PointColDetector;
class PositionStorage;
class ProceduralManager;
class RailSegment;
class RailGroup;
class Replay;
class ActorSpawner;
class Road2;
class Road;
class SimController;
class ScopeLog;
class Screwprop;
class SkyManager;
class SkyXManager;
class SlideNode;
class ShadowManager;
class Sound;
class SoundManager;
class SoundScriptInstance;
class SoundScriptManager;
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
class Turbojet;
class Turboprop;
class VideoCamera;
class VehicleAI;
class Water;

#ifdef USE_SOCKETW
class SWBaseSocket;
#endif // USE_SOCKETW
