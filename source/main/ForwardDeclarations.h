/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal & contributors

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
    class  CVar;
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
    struct ProjectEntry;
    struct Prop;
    struct PropAnim;
    class  Renderdash;
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
        class  ConsoleView;
        class  Dialog;
        class  FrictionSettings;
        class  GameMainMenu;
        class  GamePauseMenu;
        class  LoadingWindow;
        class  MainSelector;
        class  MpClientList;
        class  MultiplayerSelector;
        class  SimActorStats;
        class  SurveyMap;
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

class Actor;
class ActorSpawner;
class AeroEngine;
class Airbrake;
class Airfoil;
class Autopilot;
class Buoyance;
class CacheEntry;
class CacheSystem;
class Character;
class ChatSystem;
class CmdKeyInertia;
class Collisions;
class ColoredTextAreaOverlayElement;
class DashBoard;
class DashBoardManager;
class Differential;
class DotSceneLoader;
class DustPool;
class Editor;
class EngineSim;
class Envmap;
class FlexAirfoil;
class FlexBody;
class FlexMesh;
class FlexObj;
class Flexable;
class ForceFeedback;
class HDRListener;
class HeightFinder;
class HydraxWater;
class IWater;
class InputEngine;
class Landusemap;
class MapTextureCreator;
class MeshObject;
class Mirror;
class MumbleIntegration;
class Network;
class OutProtocol;
class OverlayWrapper;
class PointColDetector;
class ProceduralManager;
class RailGroup;
class RailSegment;
class Replay;
class Road2;
class Road;
class ScopeLog;
class Screwprop;
class ShadowManager;
class SimController;
class SkyManager;
class SkyXManager;
class SlideNode;
class Sound;
class SoundManager;
class SoundScriptInstance;
class SoundScriptManager;
class SurveyMapManager;
class Task;
class TerrainGeometryManager;
class TerrainHeightFinder;
class TerrainManager;
class TerrainObjectManager;
class ThreadPool;
class ThreadWorker;
class TorqueCurve;
class TransferCase;
class TruckEditor;
class Turbojet;
class Turboprop;
class VehicleAI;
class VideoCamera;
class Water;

#ifdef USE_SOCKETW
class SWBaseSocket;
#endif // USE_SOCKETW
