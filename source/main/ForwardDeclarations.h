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


/// @file   ForwardDeclarations.h
/// @brief  Global forward declarations.
/// @author Petr Ohlidal
/// @date   12/2013


#pragma once

namespace RoR
{
    class  Actor;
    class  ActorManager;
    class  ActorSpawner;
    class  AeroEngine;
    class  Airbrake;
    class  Airfoil;
    class  AppContext;
    class  Autopilot;
    class  Buoyance;
    class  CacheEntry;
    class  CacheSystem;
    class  CameraManager;
    class  Character;
    class  Collisions;
    class  ConfigFile;
    class  Console;
    class  ContentManager;
    class  CVar;
    class  DashBoard;
    class  DashBoardManager;
    class  DashPanelOverlayElement;
    class  DashTextAreaOverlayElement;
    class  DustPool;
    class  DiscordRpc;
    class  EngineSim;
    class  Flexable;
    class  FlexAirfoil;
    class  FlexBody;
    class  FlexBodyFileIO;
    struct FlexBodyCacheData;
    class  FlexFactory;
    class  FlexMeshWheel;
    class  FlexObj;
    class  ForceFeedback;
    class  GameContext;
    class  GameScript;
    class  GfxActor;
    struct GfxCharacter;
    class  GfxEnvmap;
    class  GfxScene;
    class  GUIManager;
    struct GuiManagerImpl;
    class  HydraxWater;
    class  InputEngine;
    class  IWater;
    class  Landusemap;
    class  LanguageEngine;
    class  MovableText;
    class  MumbleIntegration;
    class  OutGauge;
    class  OverlayWrapper;
    class  Network;
    class  OgreSubsystem;
    struct PlatformUtils;
    class  PointColDetector;
    struct Prop;
    struct PropAnim;
    class  RailGroup;
    class  Renderdash;
    class  Replay;
    class  RigLoadingProfiler;
    class  Screwprop;
    class  ScriptEngine;
    class  ShadowManager;
    class  Skidmark;
    class  SkidmarkConfig;
    struct SkinDef;
    class  SkinManager;
    class  SkyManager;
    class  SkyXManager;
    class  SlideNode;
    class  Sound;
    class  SoundManager;
    class  SoundScriptInstance;
    class  SoundScriptManager;
    class  Task;
    class  TerrainEditor;
    class  TerrainGeometryManager;
    class  TerrainManager;
    class  TerrainObjectManager;
    struct Terrn2Author;
    struct Terrn2Def;
    class  Terrn2Parser;
    struct Terrn2Telepoint;
    class  TorqueCurve;
    class  ThreadPool;
    class  VehicleAI;
    class  VideoCamera;

    // SimData.h
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

    namespace GUI
    {
        class  ConsoleView;
        class  Dialog;
        class  FrictionSettings;
        class  GameMainMenu;
        class  LoadingWindow;
        class  MainSelector;
        class  MpClientList;
        class  MultiplayerSelector;
        class  DirectionArrow;
        class  SceneMouse;
        class  SimActorStats;
        class  SurveyMap;
        class  TopMenubar;
    }
} // namespace RoR

namespace RoRnet
{
    struct Header;
    struct UserInfo;
    struct StreamRegister;
    struct ActorStreamRegister;
    struct ServerInfo;
    struct VehicleState;
}

#ifdef USE_SOCKETW
class SWBaseSocket;
#endif // USE_SOCKETW
