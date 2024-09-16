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

#include "GameScript.h"

#ifdef USE_CURL
#   include <curl/curl.h>
#   include <curl/easy.h>
#endif //USE_CURL

// AS addons start
#include "scriptany/scriptany.h"
#include "scriptarray/scriptarray.h"
#include "scripthelper/scripthelper.h"
#include "scriptmath/scriptmath.h"
#include "scriptstdstring/scriptstdstring.h"
// AS addons end

#include "AppContext.h"
#include "Actor.h"
#include "ActorManager.h"
#include "CacheSystem.h"
#include "Character.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "Console.h"
#include "CurlHelpers.h"
#include "EngineSim.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_TopMenubar.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "Network.h"
#include "RoRVersion.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"
#include "SkyManager.h"
#include "SoundScriptManager.h"
#include "Terrain.h"
#include "TerrainGeometryManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"
#include "VehicleAI.h"
#include "Water.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

using namespace Ogre;
using namespace RoR;
using namespace AngelScript;

// GUIDELINE: Make functions safe from invoking in wrong circumstances,
// i.e. when server script calls function using SimController while in main menu.
// --> Getter functions should silently return zero/empty value.
// --> Functions performing simulation changes should log warning and do nothing.



void GameScript::log(const String& msg)
{
    App::GetScriptEngine()->SLOG(msg);
}

void GameScript::logFormat(const char* format, ...)
{
    char buffer[4000] = {};
    sprintf(buffer, "[RoR|Script] "); // Length: 13 characters
    char* buffer_pos = buffer + 13;

    va_list args;
    va_start(args, format);
        vsprintf(buffer_pos, format, args);
    va_end(args);

    App::GetScriptEngine()->SLOG(buffer);
}

void GameScript::activateAllVehicles()
{
    App::GetGameContext()->GetActorManager()->WakeUpAllActors();
}

void GameScript::setTrucksForcedAwake(bool forceActive)
{
    App::GetGameContext()->GetActorManager()->SetTrucksForcedAwake(forceActive);
}

float GameScript::getTime()
{
    return App::GetGameContext()->GetActorManager()->GetTotalTime();
}

void GameScript::setPersonPosition(const Vector3& vec)
{
    if (!this->HavePlayerAvatar(__FUNCTION__))
        return;

    App::GetGameContext()->GetPlayerCharacter()->setPosition(vec);
}

void GameScript::loadTerrain(const String& terrain)
{
    App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, terrain));
}

Vector3 GameScript::getPersonPosition()
{
    Vector3 result(Vector3::ZERO);
    if (App::GetGameContext()->GetPlayerCharacter())
        result = App::GetGameContext()->GetPlayerCharacter()->getPosition();
    return result;
}

void GameScript::movePerson(const Vector3& vec)
{
    if (!this->HavePlayerAvatar(__FUNCTION__))
        return;

    App::GetGameContext()->GetPlayerCharacter()->move(vec);
}

void GameScript::setPersonRotation(const Radian& rot)
{
    if (!this->HavePlayerAvatar(__FUNCTION__))
        return;

    App::GetGameContext()->GetPlayerCharacter()->setRotation(rot);
}

Radian GameScript::getPersonRotation()
{
    Radian result(0);
    if (App::GetGameContext()->GetPlayerCharacter())
        result = App::GetGameContext()->GetPlayerCharacter()->getRotation();
    return result;
}

String GameScript::getCaelumTime()
{
    String result = "";
#ifdef USE_CAELUM
    if (App::GetGameContext()->GetTerrain())
    {
        result = App::GetGameContext()->GetTerrain()->getSkyManager()->GetPrettyTime();
    }
#endif // USE_CAELUM
    return result;
}

void GameScript::setCaelumTime(float value)
{
#ifdef USE_CAELUM
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    App::GetGameContext()->GetTerrain()->getSkyManager()->SetSkyTimeFactor(value);
#endif // USE_CAELUM
}

bool GameScript::getCaelumAvailable()
{
    bool result = false;
#ifdef USE_CAELUM
    if (App::GetGameContext()->GetTerrain())
        result = App::GetGameContext()->GetTerrain()->getSkyManager() != 0;
#endif // USE_CAELUM
    return result;
}

void GameScript::stopTimer()
{
    App::GetGameContext()->GetRaceSystem().StopRaceTimer();
}

void GameScript::startTimer(int id)
{
    App::GetGameContext()->GetRaceSystem().StartRaceTimer(id);
}

void GameScript::setTimeDiff(float diff)
{
    App::GetGameContext()->GetRaceSystem().SetRaceTimeDiff(diff);
}

void GameScript::setBestLapTime(float time)
{
    App::GetGameContext()->GetRaceSystem().SetRaceBestTime(time);
}

void GameScript::setWaterHeight(float value)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    if (App::GetGameContext()->GetTerrain()->getWater())
    {
        IWater* water = App::GetGameContext()->GetTerrain()->getWater();
        water->SetStaticWaterHeight(value);
        water->UpdateWater();
    }
}

float GameScript::getGroundHeight(Vector3& v)
{
    float result = -1.0f;
    if (App::GetGameContext()->GetTerrain())
        result = App::GetGameContext()->GetTerrain()->GetHeightAt(v.x, v.z);
    return result;
}

float GameScript::getWaterHeight()
{
    float result = 0.0f;
    if (App::GetGameContext()->GetTerrain() && App::GetGameContext()->GetTerrain()->getWater())
        result = App::GetGameContext()->GetTerrain()->getWater()->GetStaticWaterHeight();
    return result;
}

ActorPtr GameScript::getCurrentTruck()
{
    return App::GetGameContext()->GetPlayerActor();
}

float GameScript::getGravity()
{
    float result = 0.f;
    if (App::GetGameContext()->GetTerrain())
    {
        result = App::GetGameContext()->GetTerrain()->getGravity();
    }
    return result;
}

void GameScript::setGravity(float value)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    App::GetGameContext()->GetTerrain()->setGravity(value);
}

ActorPtr GameScript::getTruckByNum(int num)
{
    return App::GetGameContext()->GetActorManager()->GetActorById(num);
}

int GameScript::getNumTrucksByFlag(int flag)
{
    int result = 0;
    for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (!flag || static_cast<int>(actor->ar_state) == flag)
            result++;
    }
    return result;
}

int GameScript::getCurrentTruckNumber()
{
    ActorPtr actor = App::GetGameContext()->GetPlayerActor();
    return (actor != nullptr) ? actor->ar_instance_id : -1;
}

void GameScript::registerForEvent(int eventValue)
{
    if (App::GetScriptEngine())
    {
        ScriptUnitId_t unit_id = App::GetScriptEngine()->getCurrentlyExecutingScriptUnit();
        if (unit_id != SCRIPTUNITID_INVALID)
        {
            App::GetScriptEngine()->getScriptUnit(unit_id).eventMask |= eventValue;
        }
    }
}

void GameScript::unRegisterEvent(int eventValue)
{
    if (App::GetScriptEngine())
    {
        ScriptUnitId_t unit_id = App::GetScriptEngine()->getCurrentlyExecutingScriptUnit();
        if (unit_id != SCRIPTUNITID_INVALID)
        {
            App::GetScriptEngine()->getScriptUnit(unit_id).eventMask &= ~eventValue;
        }
    }
}

BitMask_t GameScript::getRegisteredEventsMask(ScriptUnitId_t nid)
{
    if (App::GetScriptEngine()->scriptUnitExists(nid))
        return App::GetScriptEngine()->getScriptUnit(nid).eventMask;
    else
        return BitMask_t(0);
}

void GameScript::setRegisteredEventsMask(ScriptUnitId_t nid, BitMask_t eventMask)
{
    if (App::GetScriptEngine()->scriptUnitExists(nid))
        App::GetScriptEngine()->getScriptUnit(nid).eventMask = eventMask;
}

void GameScript::flashMessage(String& txt, float time, float charHeight)
{
    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, "script_code_red.png");
}

void GameScript::message(String& txt, String& icon)
{
    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, icon);
}

void GameScript::updateDirectionArrow(String& text, Vector3& vec)
{
    App::GetGameContext()->GetRaceSystem().UpdateDirectionArrow(const_cast<char*>(text.c_str()), Vector3(vec.x, vec.y, vec.z));
}

int GameScript::getChatFontSize()
{
    return 0; //NETCHAT.getFontSize();
}

void GameScript::setChatFontSize(int size)
{
    //NETCHAT.setFontSize(size);
}

void GameScript::showChooser(const String& type, const String& instance, const String& box)
{
    LoaderType ntype = LT_None;

    if (type == "airplane")
        ntype = LT_Airplane;
    if (type == "all")
        ntype = LT_AllBeam;
    if (type == "boat")
        ntype = LT_Boat;
    if (type == "car")
        ntype = LT_Car;
    if (type == "extension")
        ntype = LT_Extension;
    if (type == "heli")
        ntype = LT_Airplane;
    if (type == "load")
        ntype = LT_Load;
    if (type == "trailer")
        ntype = LT_Trailer;
    if (type == "train")
        ntype = LT_Train;
    if (type == "truck")
        ntype = LT_Truck;
    if (type == "vehicle")
        ntype = LT_Vehicle;

    if (ntype != LT_None)
    {
        App::GetGameContext()->ShowLoaderGUI(ntype, instance, box);
    }

}

void GameScript::repairVehicle(const String& instance, const String& box, bool keepPosition)
{
    App::GetGameContext()->GetActorManager()->RepairActor(App::GetGameContext()->GetTerrain()->GetCollisions(), instance, box, keepPosition);
}

void GameScript::removeVehicle(const String& event_source_instance_name, const String& event_source_box_name)
{
    ActorPtr actor = App::GetGameContext()->FindActorByCollisionBox(event_source_instance_name, event_source_box_name);
    if (actor)
    {
        App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
    }
}

void GameScript::destroyObject(const String& instanceName)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    if (App::GetGameContext()->GetTerrain()->getObjectManager())
    {
        App::GetGameContext()->GetTerrain()->getObjectManager()->unloadObject(instanceName);
    }
}

void GameScript::moveObjectVisuals(const String& instanceName, const Vector3& pos)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    if (App::GetGameContext()->GetTerrain()->getObjectManager())
    {
        App::GetGameContext()->GetTerrain()->getObjectManager()->MoveObjectVisuals(instanceName, pos);
    }
}

void GameScript::spawnObject(const String& objectName, const String& instanceName, const Vector3& pos, const Vector3& rot, const String& eventhandler, bool uniquifyMaterials)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    if ((App::GetGameContext()->GetTerrain()->getObjectManager() == nullptr))
    {
        this->logFormat("spawnObject(): Cannot spawn object, no terrain loaded!");
        return;
    }

    if (App::GetScriptEngine()->getTerrainScriptUnit() == -1)
    {
        this->logFormat("spawnObject(): Cannot spawn object, no terrain script loaded!");
        return;
    }

    try
    {
        AngelScript::asIScriptModule* module = App::GetScriptEngine()->getScriptUnit(App::GetScriptEngine()->getTerrainScriptUnit()).scriptModule;
        if (module == nullptr)
        {
            this->logFormat("spawnObject(): Failed to fetch/create script module");
            return;
        }

        int handler_func_id = -1; // no function
        if (!eventhandler.empty())
        {
            // Let script author know (via Angelscript.log) there's a better alternative.
            App::GetScriptEngine()->setForwardScriptLogToConsole(false);
            App::GetScriptEngine()->SLOG(
                "spawnObject(): Specifying event handler function in `game.spawnObject()` (or .TOBJ file) is obsolete and only works with terrain scripts;"
                " Use `eventCallbackEx()` with event `SE_EVENTBOX_ENTER` instead, it does the same job and works with any script."
                " Just pass an empty string to the `game.spawnObject()` parameter.");
            App::GetScriptEngine()->setForwardScriptLogToConsole(true);

            // Look up the function and log if not found or found with bad arguments (probably a typo).
            AngelScript::asIScriptFunction* handler_func = App::GetScriptEngine()->getFunctionByDeclAndLogCandidates(
                App::GetScriptEngine()->getTerrainScriptUnit(), GETFUNCFLAG_REQUIRED,
                eventhandler, GETFUNC_DEFAULTEVENTCALLBACK_SIGFMT);
            if (handler_func != nullptr)
            {
                handler_func_id = handler_func->GetId();
            }
        }

        const String type = "";
        App::GetGameContext()->GetTerrain()->getObjectManager()->LoadTerrainObject(objectName, pos, rot, instanceName, type, /*rendering_distance=*/0, true, handler_func_id, uniquifyMaterials);
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::spawnObject()");
        return;
    }
}

void GameScript::hideDirectionArrow()
{
    App::GetGameContext()->GetRaceSystem().UpdateDirectionArrow(0, Vector3::ZERO);
}

bool GameScript::getScreenPosFromWorldPos(Ogre::Vector3 const& world_pos, Ogre::Vector2& out_screen)
{
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    World2ScreenConverter world2screen(
        App::GetCameraManager()->GetCamera()->getViewMatrix(true), App::GetCameraManager()->GetCamera()->getProjectionMatrix(), Ogre::Vector2(screen_size.x, screen_size.y));
    Ogre::Vector3 pos_xyz = world2screen.Convert(world_pos);
    if (pos_xyz.z < 0.f)
    {
        out_screen.x = pos_xyz.x;
        out_screen.y = pos_xyz.y;
        return true;
    }
    return false;
}

Ogre::Vector2 GameScript::getDisplaySize()
{
    ImVec2 size = ImGui::GetIO().DisplaySize;
    return Vector2(size.x, size.y);
}

Ogre::Vector2 GameScript::getMouseScreenPosition()
{
    ImVec2 pos = ImGui::GetIO().MousePos;
    return Vector2(pos.x, pos.y);
}

int GameScript::setMaterialAmbient(const String& materialName, float red, float green, float blue)
{
    try
    {
        MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
        if (m.isNull())
            return 0;
        m->setAmbient(red, green, blue);
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::setMaterialAmbient()");
        return 0;
    }
    return 1;
}

int GameScript::setMaterialDiffuse(const String& materialName, float red, float green, float blue, float alpha)
{
    try
    {
        MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
        if (m.isNull())
            return 0;
        m->setDiffuse(red, green, blue, alpha);
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::setMaterialDiffuse()");
        return 0;
    }
    return 1;
}

int GameScript::setMaterialSpecular(const String& materialName, float red, float green, float blue, float alpha)
{
    try
    {
        MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
        if (m.isNull())
            return 0;
        m->setSpecular(red, green, blue, alpha);
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::setMaterialSpecular()");
        return 0;
    }
    return 1;
}

int GameScript::setMaterialEmissive(const String& materialName, float red, float green, float blue)
{
    try
    {
        MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
        if (m.isNull())
            return 0;
        m->setSelfIllumination(red, green, blue);
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::setMaterialEmissive()");
        return 0;
    }
    return 1;
}

int GameScript::getTextureUnitState(TextureUnitState** tu, const String materialName, int techniqueNum, int passNum, int textureUnitNum)
{
    // Internal helper - propagate exceptions outside so that correct function names appear in exception-events
    // ========================================================================================================
    
    MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
    if (m.isNull())
        return 1;

    // verify technique
    if (techniqueNum < 0 || techniqueNum > m->getNumTechniques())
        return 2;
    Technique* t = m->getTechnique(techniqueNum);
    if (!t)
        return 2;

    //verify pass
    if (passNum < 0 || passNum > t->getNumPasses())
        return 3;
    Pass* p = t->getPass(passNum);
    if (!p)
        return 3;

    //verify texture unit
    if (textureUnitNum < 0 || textureUnitNum > p->getNumTextureUnitStates())
        return 4;
    TextureUnitState* tut = p->getTextureUnitState(textureUnitNum);
    if (!tut)
        return 4;

    *tu = tut;
    return 0;
}

int GameScript::setMaterialTextureName(const String& materialName, int techniqueNum, int passNum, int textureUnitNum, const String& textureName)
{
    try
    {
        TextureUnitState* tu = 0;
        int res = getTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
        if (res == 0 && tu != 0)
        {
            // finally, set it
            tu->setTextureName(textureName);
        }
        return res;
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::setMaterialTextureName()");
        return 0;
    }
}

int GameScript::setMaterialTextureRotate(const String& materialName, int techniqueNum, int passNum, int textureUnitNum, float rotation)
{
    try
    {
        TextureUnitState* tu = 0;
        int res = getTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
        if (res == 0 && tu != 0)
        {
            tu->setTextureRotate(Degree(rotation));
        }
        return res;
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::setMaterialTextureRotate()");
        return 0;
    }
}

int GameScript::setMaterialTextureScroll(const String& materialName, int techniqueNum, int passNum, int textureUnitNum, float sx, float sy)
{
    try
    {
        TextureUnitState* tu = 0;
        int res = getTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
        if (res == 0 && tu != 0)
        {
            tu->setTextureScroll(sx, sy);
        }
        return res;
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::setMaterialTextureScroll()");
        return 0;
    }
}

int GameScript::setMaterialTextureScale(const String& materialName, int techniqueNum, int passNum, int textureUnitNum, float u, float v)
{
    try
    {
        TextureUnitState* tu = 0;
        int res = getTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
        if (res == 0 && tu != 0)
        {
            tu->setTextureScale(u, v);
        }
        return res;
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::setMaterialTextureScale()");
        return 0;
    }
}

float GameScript::rangeRandom(float from, float to)
{
    return Math::RangeRandom(from, to);
}

int GameScript::getLoadedTerrain(String& result)
{
    String terrainName = "";

    if (App::GetGameContext()->GetTerrain())
    {
        terrainName = App::GetGameContext()->GetTerrain()->getTerrainName();
        result = terrainName;
    }

    return !terrainName.empty();
}

RoR::TerrainPtr GameScript::getTerrain()
{
    return App::GetGameContext()->GetTerrain();
}

void GameScript::clearEventCache()
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    if (App::GetGameContext()->GetTerrain()->GetCollisions() == nullptr)
    {
        this->logFormat("Cannot execute '%s', collisions not ready", __FUNCTION__);
        return;
    }

    App::GetGameContext()->GetTerrain()->GetCollisions()->clearEventCache();
}

void GameScript::setCameraPosition(const Vector3& pos)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    App::GetCameraManager()->GetCameraNode()->setPosition(Vector3(pos.x, pos.y, pos.z));
}

void GameScript::setCameraDirection(const Vector3& rot)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    App::GetCameraManager()->GetCameraNode()->setDirection(Vector3(rot.x, rot.y, rot.z), Ogre::Node::TS_WORLD);
}

void GameScript::setCameraOrientation(const Quaternion& q)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    App::GetCameraManager()->GetCameraNode()->setOrientation(Quaternion(q.w, q.x, q.y, q.z));
}

void GameScript::setCameraYaw(float rotX)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    App::GetCameraManager()->GetCameraNode()->yaw(Degree(rotX), Ogre::Node::TS_WORLD);
}

void GameScript::setCameraPitch(float rotY)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    App::GetCameraManager()->GetCameraNode()->pitch(Degree(rotY));
}

void GameScript::setCameraRoll(float rotZ)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    App::GetCameraManager()->GetCameraNode()->roll(Degree(rotZ));
}

Vector3 GameScript::getCameraPosition()
{
    Vector3 result(Vector3::ZERO);
    if (App::GetCameraManager()->GetCameraNode())
        result = App::GetCameraManager()->GetCameraNode()->getPosition();
    return result;
}

Vector3 GameScript::getCameraDirection()
{
    Vector3 result(Vector3::ZERO);
    if (App::GetCameraManager()->GetCameraNode())
    {
        // Direction points down -Z by default (adapted from Ogre::Camera)
        result = App::GetCameraManager()->GetCameraNode()->getOrientation() * -Ogre::Vector3::UNIT_Z;
    }
    return result;
}

Quaternion GameScript::getCameraOrientation()
{
    Quaternion result(Quaternion::ZERO);
    if (App::GetCameraManager()->GetCameraNode())
        result = App::GetCameraManager()->GetCameraNode()->getOrientation();
    return result;
}

void GameScript::cameraLookAt(const Vector3& pos)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    App::GetCameraManager()->GetCameraNode()->lookAt(Vector3(pos.x, pos.y, pos.z), Ogre::Node::TS_WORLD);
}

int GameScript::useOnlineAPI(const String& apiquery, const AngelScript::CScriptDictionary& dict, String& result)
{
    if (App::app_disable_online_api->getBool())
        return 0;

    ScriptUnitId_t unit_id = App::GetScriptEngine()->getCurrentlyExecutingScriptUnit();
    if (unit_id == SCRIPTUNITID_INVALID)
        return 2;

    ActorPtr player_actor = App::GetGameContext()->GetPlayerActor();

    if (player_actor == nullptr)
        return 1;

    std::string hashtok = Sha1Hash(App::mp_player_name->getStr());
    std::string url = App::mp_api_url->getStr() + apiquery;
    std::string user = std::string("RoR-Api-User: ") + App::mp_player_name->getStr();
    std::string token = std::string("RoR-Api-User-Token: ") + hashtok;

    std::string terrain_name = App::GetGameContext()->GetTerrain()->getTerrainName();

    std::string script_name = App::GetScriptEngine()->getScriptUnit(unit_id).scriptName;
    std::string script_hash = App::GetScriptEngine()->getScriptUnit(unit_id).scriptHash;

    rapidjson::Document j_doc;
    j_doc.SetObject();

    j_doc.AddMember("user-name", rapidjson::StringRef(App::mp_player_name->getStr().c_str()), j_doc.GetAllocator());
    j_doc.AddMember("user-country", rapidjson::StringRef(App::app_country->getStr().c_str()), j_doc.GetAllocator());
    j_doc.AddMember("user-token", rapidjson::StringRef(hashtok.c_str()), j_doc.GetAllocator());

    j_doc.AddMember("terrain-name", rapidjson::StringRef(terrain_name.c_str()), j_doc.GetAllocator());
    j_doc.AddMember("terrain-filename", rapidjson::StringRef(App::sim_terrain_name->getStr().c_str()), j_doc.GetAllocator());

    j_doc.AddMember("script-name", rapidjson::StringRef(script_name.c_str()), j_doc.GetAllocator());
    j_doc.AddMember("script-hash", rapidjson::StringRef(script_hash.c_str()), j_doc.GetAllocator());

    j_doc.AddMember("actor-name", rapidjson::StringRef(player_actor->ar_design_name.c_str()), j_doc.GetAllocator());
    j_doc.AddMember("actor-filename", rapidjson::StringRef(player_actor->ar_filename.c_str()), j_doc.GetAllocator());
    j_doc.AddMember("actor-hash", rapidjson::StringRef(player_actor->ar_filehash.c_str()), j_doc.GetAllocator());

    rapidjson::Value j_linked_actors(rapidjson::kArrayType);
    for (ActorPtr& actor : player_actor->ar_linked_actors)
    {
        rapidjson::Value j_actor(rapidjson::kObjectType);
        j_actor.AddMember("actor-name", rapidjson::StringRef(actor->ar_design_name.c_str()), j_doc.GetAllocator());
        j_actor.AddMember("actor-filename", rapidjson::StringRef(actor->ar_filename.c_str()), j_doc.GetAllocator());
        j_actor.AddMember("actor-hash", rapidjson::StringRef(actor->ar_filehash.c_str()), j_doc.GetAllocator());
        j_linked_actors.PushBack(j_actor, j_doc.GetAllocator());
    }
    j_doc.AddMember("linked-actors", j_linked_actors, j_doc.GetAllocator());

    j_doc.AddMember("avg-fps", getAvgFPS(), j_doc.GetAllocator());
    j_doc.AddMember("ror-version", rapidjson::StringRef(ROR_VERSION_STRING), j_doc.GetAllocator());

    for (auto item : dict)
    {
        const std::string& key = item.GetKey();
        const std::string* value = (std::string *)item.GetAddressOfValue();
        j_doc.AddMember(rapidjson::StringRef(key.c_str()), rapidjson::StringRef(value->c_str()), j_doc.GetAllocator());
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    j_doc.Accept(writer);
    std::string json = buffer.GetString();

#if USE_CURL
    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
            _L("using Online API..."), "information.png");

    LOG("[RoR|GameScript] Submitting race results to '" + url + "'");

    std::thread([url, user, token, json]()
        {
            long response_code = 0;

            struct curl_slist *slist = NULL;
            slist = curl_slist_append(slist, "Accept: application/json");
            slist = curl_slist_append(slist, "Content-Type: application/json");
            slist = curl_slist_append(slist, user.c_str());
            slist = curl_slist_append(slist, token.c_str());

            CURL *curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    slist);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    json.c_str());
            
            CURLcode curl_result = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            if (curl_result != CURLE_OK || response_code != 200)
            {
                Ogre::LogManager::getSingleton().stream()
                    << "[RoR|GameScript] `useOnlineAPI()` failed to submit data;"
                    << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;
            }

            curl_easy_cleanup(curl);
            curl = nullptr;
            curl_slist_free_all(slist);
            slist = NULL;
        }).detach();
#else // USE_CURL
    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
                                       _L("Cannot use Online API in this build (CURL not available)"));
#endif // USE_CURL

    return 0;
}

void GameScript::openUrlInDefaultBrowser(const std::string& url)
{
    RoR::OpenUrlInDefaultBrowser(url); // PlatformUtils.h
}

void GameScript::fetchUrlAsStringAsync(const std::string& url, const std::string& display_filename)
{
#if defined(USE_CURL)
    CurlTaskContext task;
    task.ctc_url = url;
    task.ctc_displayname = display_filename;
    // Messages to post                                     // See `RoR::angelScriptThreadStatus`
    task.ctc_msg_progress = MSG_APP_SCRIPT_THREAD_STATUS;  // `RoR::ASTHREADSTATUS_CURLSTRING_PROGRESS`
    task.ctc_msg_success = MSG_APP_SCRIPT_THREAD_STATUS;   // `RoR::ASTHREADSTATUS_CURLSTRING_SUCCESS`
    task.ctc_msg_failure = MSG_APP_SCRIPT_THREAD_STATUS;   // `RoR::ASTHREADSTATUS_CURLSTRING_FAILURE`

    std::packaged_task<void(CurlTaskContext)> pktask(GetUrlAsStringMQ);
    std::thread(std::move(pktask), task).detach();
#endif // defined(USE_CURL)
}

void GameScript::boostCurrentTruck(float factor)
{
    ActorPtr actor = App::GetGameContext()->GetPlayerActor();
    if (actor && actor->ar_engine)
    {
        float rpm = actor->ar_engine->getRPM();
        rpm += 2000.0f * factor;
        actor->ar_engine->setRPM(rpm);
    }
}

int GameScript::addScriptFunction(const String& arg)
{
    return App::GetScriptEngine()->addFunction(arg);
}

int GameScript::scriptFunctionExists(const String& arg)
{
    return App::GetScriptEngine()->functionExists(arg);
}

int GameScript::deleteScriptFunction(const String& arg)
{
    return App::GetScriptEngine()->deleteFunction(arg);
}

int GameScript::addScriptVariable(const String& arg)
{
    return App::GetScriptEngine()->addVariable(arg);
}

int GameScript::deleteScriptVariable(const String& arg)
{
    return App::GetScriptEngine()->deleteVariable(arg);
}

int GameScript::sendGameCmd(const String& message)
{
#ifdef USE_SOCKETW
    if (RoR::App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        App::GetNetwork()->AddPacket(0, RoRnet::MSG2_GAME_CMD, (int)message.size(), const_cast<char*>(message.c_str()));
        return 0;
    }
#endif // USE_SOCKETW

    return -11;
}

AngelScript::CScriptArray* GameScript::getRunningScripts()
{
    std::vector<ScriptUnitId_t> ids;
    for (auto& pair: App::GetScriptEngine()->getScriptUnits())
        ids.push_back(pair.first);

    return VectorToScriptArray(ids, "int");
}


AngelScript::CScriptDictionary* GameScript::getScriptDetails(ScriptUnitId_t nid)
{
    if (!App::GetScriptEngine()->scriptUnitExists(nid))
        return nullptr;

    ScriptUnit& info = App::GetScriptEngine()->getScriptUnit(nid);
    AngelScript::CScriptDictionary* dict = AngelScript::CScriptDictionary::Create(App::GetScriptEngine()->getEngine());
    int stringTypeid = App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("string");
    int scriptCategoryTypeid = App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("ScriptCategory");

    dict->Set("uniqueId", (asINT64)info.uniqueId);
    dict->Set("scriptName", new std::string(info.scriptName), stringTypeid);
    dict->Set("scriptCategory", &info.scriptCategory, scriptCategoryTypeid);
    dict->Set("eventMask", (asINT64)info.eventMask);
    dict->Set("scriptBuffer", new std::string(info.scriptBuffer), stringTypeid);

    // TBD Some other time...
    //AngelScript::asIScriptModule* scriptModule = nullptr;
    //AngelScript::asIScriptFunction* frameStepFunctionPtr = nullptr; //!< script function pointer to the frameStep function
    //AngelScript::asIScriptFunction* eventCallbackFunctionPtr = nullptr; //!< script function pointer to the event callback function
    //AngelScript::asIScriptFunction* eventCallbackExFunctionPtr = nullptr; //!< script function pointer to the event callback function
    //AngelScript::asIScriptFunction* defaultEventCallbackFunctionPtr = nullptr; //!< script function pointer for spawner events
    //ActorPtr associatedActor; //!< For ScriptCategory::ACTOR
    //Ogre::String scriptHash;

    return dict;
}

VehicleAIPtr GameScript::getCurrentTruckAI()
{
    VehicleAIPtr result = nullptr;
    if (App::GetGameContext()->GetPlayerActor())
    {
        result = App::GetGameContext()->GetPlayerActor()->ar_vehicle_ai;
    }
    return result;
}

VehicleAIPtr GameScript::getTruckAIByNum(int num)
{
    VehicleAIPtr result = nullptr;
    ActorPtr actor = App::GetGameContext()->GetActorManager()->GetActorById(num);
    if (actor != nullptr)
    {
        result = actor->ar_vehicle_ai;
    }
    return result;
}

ActorPtr GameScript::spawnTruck(Ogre::String& truckName, Ogre::Vector3& pos, Ogre::Vector3& rot)
{
    ActorSpawnRequest rq;
    rq.asr_position = pos;
    rq.asr_rotation = Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z);
    rq.asr_filename = truckName;
    return App::GetGameContext()->SpawnActor(rq);
}

ActorPtr GameScript::spawnTruckAI(Ogre::String& truckName, Ogre::Vector3& pos, Ogre::String& truckSectionConfig, std::string& truckSkin, int x)
{
    try
    {
        ActorSpawnRequest rq;
        rq.asr_position = pos;

        // Set rotation based on first two waypoints
        std::vector<Ogre::Vector3> waypoints;
        for (int i = 0; i < App::GetGuiManager()->TopMenubar.ai_waypoints.size(); i++)
        {
            waypoints.push_back(App::GetGuiManager()->TopMenubar.ai_waypoints[i].position);
        }
        if (App::GetGuiManager()->TopMenubar.ai_mode == 3 && x == 1) // Crash driving mode
        {
            std::reverse(waypoints.begin(), waypoints.end());
        }

        // Check if we have enough waypoints
        Ogre::Vector3 dir = Ogre::Vector3::ZERO;
        if (waypoints.size() >= 2)
            dir = waypoints[0] - waypoints[1];
        else if (waypoints.size() >= 1)
            dir = waypoints[0];
        dir.y = 0;
        rq.asr_rotation = Ogre::Vector3::UNIT_X.getRotationTo(dir, Ogre::Vector3::UNIT_Y);

        rq.asr_filename = truckName;
        rq.asr_config = truckSectionConfig;
        rq.asr_skin_entry = App::GetCacheSystem()->FetchSkinByName(truckSkin);
        rq.asr_origin = ActorSpawnRequest::Origin::AI;
        return App::GetGameContext()->SpawnActor(rq);
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::setMaterialTextureScale()");
        return ActorPtr();
    }
}

AngelScript::CScriptArray* GameScript::getWaypoints(int x)
{
    std::vector<Ogre::Vector3> vec;
    for (int i = 0; i < App::GetGuiManager()->TopMenubar.ai_waypoints.size(); i++)
    {
        vec.push_back(App::GetGuiManager()->TopMenubar.ai_waypoints[i].position);
    }
    if (App::GetGuiManager()->TopMenubar.ai_mode == 3 && x == 1) // Crash driving mode
    {
        std::reverse(vec.begin(), vec.end());
    }

    AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(AngelScript::asGetActiveContext()->GetEngine()->GetTypeInfoByDecl("array<vector3>"), vec.size());

    for(AngelScript::asUINT i = 0; i < arr->GetSize(); i++)
    {
        // Set the value of each element
        arr->SetValue(i, &vec[i]);
    }

    return arr;
}

AngelScript::CScriptArray* GameScript::getAllTrucks()
{
    ActorPtrVec& actors = App::GetGameContext()->GetActorManager()->GetActors();
    AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(AngelScript::asGetActiveContext()->GetEngine()->GetTypeInfoByDecl("array<BeamClass@>"), actors.size());

    for (AngelScript::asUINT i = 0; i < arr->GetSize(); i++)
    {
        // Set the value of each element
        arr->SetValue(i, &actors[i]);
    }

    return arr;
}

void GameScript::addWaypoint(const Ogre::Vector3& pos)
{
    std::vector<Ogre::Vector3> waypoints;
    for (int i = 0; i < App::GetGuiManager()->TopMenubar.ai_waypoints.size(); i++)
    {
        waypoints.push_back(App::GetGuiManager()->TopMenubar.ai_waypoints[i].position);
    }
}

AngelScript::CScriptArray* GameScript::getWaypointsSpeed()
{
    std::vector<int> vec;
    for (int i = 0; i < App::GetGuiManager()->TopMenubar.ai_waypoints.size(); i++)
    {
        vec.push_back(App::GetGuiManager()->TopMenubar.ai_waypoints[i].speed);
    }

    AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(AngelScript::asGetActiveContext()->GetEngine()->GetTypeInfoByDecl("array<int>"), vec.size());

    for(AngelScript::asUINT i = 0; i < arr->GetSize(); i++)
    {
        // Set the value of each element
        arr->SetValue(i, &vec[i]);
    }

    return arr;
}

int GameScript::getAIVehicleCount()
{
    int num = App::GetGuiManager()->TopMenubar.ai_num;
    return num;
}

int GameScript::getAIVehicleDistance()
{
    int dist = App::GetGuiManager()->TopMenubar.ai_distance;
    return dist;
}

int GameScript::getAIVehiclePositionScheme()
{
    int scheme = App::GetGuiManager()->TopMenubar.ai_position_scheme;
    return scheme;
}

int GameScript::getAIVehicleSpeed()
{
    int speed = App::GetGuiManager()->TopMenubar.ai_speed;
    return speed;
}

Ogre::String GameScript::getAIVehicleName(int x)
{
    if ((App::GetGuiManager()->TopMenubar.ai_mode == 2 || App::GetGuiManager()->TopMenubar.ai_mode == 3) && x == 1) // Drag Race or Crash driving mode
    {
        Ogre::String name = App::GetGuiManager()->TopMenubar.ai_fname2;
        return name;
    }
    else
    {
        Ogre::String name = App::GetGuiManager()->TopMenubar.ai_fname;
        return name;
    }
}

Ogre::String GameScript::getAIVehicleSectionConfig(int x)
{
    if ((App::GetGuiManager()->TopMenubar.ai_mode == 2 || App::GetGuiManager()->TopMenubar.ai_mode == 3) && x == 1) // Drag Race or Crash driving mode
    {
        Ogre::String config = App::GetGuiManager()->TopMenubar.ai_sectionconfig2;
        return config;
    }
    else
    {
        Ogre::String config = App::GetGuiManager()->TopMenubar.ai_sectionconfig;
        return config;
    }
}

std::string GameScript::getAIVehicleSkin(int x)
{
    if ((App::GetGuiManager()->TopMenubar.ai_mode == 2 || App::GetGuiManager()->TopMenubar.ai_mode == 3) && x == 1) // Drag Race or Crash driving mode
    {
        std::string skin = App::GetGuiManager()->TopMenubar.ai_skin2;
        return skin;
    }
    else
    {
        std::string skin = App::GetGuiManager()->TopMenubar.ai_skin;
        return skin;
    }
}

int GameScript::getAIRepeatTimes()
{
    int times = App::GetGuiManager()->TopMenubar.ai_times;
    return times;
}

int GameScript::getAIMode()
{
    int mode = App::GetGuiManager()->TopMenubar.ai_mode;
    return mode;
}

// AI: set

void GameScript::setAIVehicleCount(int num)
{
    App::GetGuiManager()->TopMenubar.ai_num = num;
}

void GameScript::setAIVehicleDistance(int dist)
{
    App::GetGuiManager()->TopMenubar.ai_distance = dist;
}

void GameScript::setAIVehiclePositionScheme(int scheme)
{
    App::GetGuiManager()->TopMenubar.ai_position_scheme = scheme;
}

void GameScript::setAIVehicleSpeed(int speed)
{
    App::GetGuiManager()->TopMenubar.ai_speed = speed;
}

void GameScript::setAIVehicleName(int x, std::string name)
{
    if ((App::GetGuiManager()->TopMenubar.ai_mode == 2 || App::GetGuiManager()->TopMenubar.ai_mode == 3) && x == 1) // Drag Race or Crash driving mode
    {
        App::GetGuiManager()->TopMenubar.ai_fname2 = name;
    }
    else
    {
        App::GetGuiManager()->TopMenubar.ai_fname = name;
    }
}

void GameScript::setAIVehicleSectionConfig(int x, std::string config)
{
    switch (x)
    {
    case 0:
        App::GetGuiManager()->TopMenubar.ai_sectionconfig = config;
        break;
    case 1:
        App::GetGuiManager()->TopMenubar.ai_sectionconfig2 = config;
        break;
    default:
        this->log(fmt::format("setAIVehicleSectionConfig: ERROR, valid 'x' is 0 or 1, got {}", x));
        break;
    }
}

void GameScript::setAIVehicleSkin(int x, std::string skin)
{
    switch (x)
    {
    case 0:
        App::GetGuiManager()->TopMenubar.ai_skin = skin;
        break;
    case 1:
        App::GetGuiManager()->TopMenubar.ai_skin2 = skin;
        break;
    default:
        this->log(fmt::format("setAIVehicleSkin: ERROR, valid 'x' is 0 or 1, got {}", x));
        break;
    }
}

void GameScript::setAIRepeatTimes(int times)
{
    App::GetGuiManager()->TopMenubar.ai_times = times;
}

void GameScript::setAIMode(int mode)
{
    App::GetGuiManager()->TopMenubar.ai_mode = mode;
}

void GameScript::showMessageBox(Ogre::String& title, Ogre::String& text, bool use_btn1, Ogre::String& btn1_text, bool allow_close, bool use_btn2, Ogre::String& btn2_text)
{
    // Sanitize inputs
    const char* btn1_cstr = nullptr; // = Button disabled
    const char* btn2_cstr = nullptr;

    if (use_btn1)
    {
        btn1_cstr = (btn1_text.empty() ? "~1~" : btn1_text.c_str());
    }
    if (use_btn2)
    {
        btn2_cstr = (btn2_text.empty() ? "~2~" : btn2_text.c_str());
    }

    RoR::App::GetGuiManager()->ShowMessageBox(title.c_str(), text.c_str(), allow_close, btn1_cstr, btn2_cstr);
}

void GameScript::backToMenu()
{
    App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
    App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
}

void GameScript::quitGame()
{
    App::GetGameContext()->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
}

float GameScript::getFPS()
{
    return App::GetAppContext()->GetRenderWindow()->getStatistics().lastFPS;
}

float GameScript::getAvgFPS()
{
    return App::GetAppContext()->GetRenderWindow()->getStatistics().avgFPS;
}

bool GameScript::getMousePositionOnTerrain(Ogre::Vector3& out_pos)
{
    if (!HaveSimTerrain(__FUNCTION__))
        return false;

    Ogre::Vector2 mouse_npos = App::GetInputEngine()->getMouseNormalizedScreenPos();
    Ogre::Ray ray = App::GetCameraManager()->GetCamera()->getCameraToViewportRay(mouse_npos.x, mouse_npos.y);
    Ogre::TerrainGroup::RayResult ray_result = App::GetGameContext()->GetTerrain()->getGeometryManager()->getTerrainGroup()->rayIntersects(ray);
    if (ray_result.hit)
    {
        out_pos = ray_result.position;
    }
    return ray_result.hit;
}

Ogre::SceneManager* GameScript::getSceneManager()
{
    return App::GetGfxScene()->GetSceneManager();
}

bool GameScript::pushMessage(MsgType type, AngelScript::CScriptDictionary* dict)
{
    Message m(type);
    std::string log_msg = fmt::format("`pushMessage({})`", MsgTypeToString(type));

    switch (type)
    {
        // -- NOT ALLOWED --

        // Application
    case MSG_APP_MODCACHE_LOAD_REQUESTED:
        // Networking
    case MSG_NET_CONNECT_STARTED:
    case MSG_NET_CONNECT_PROGRESS:
    case MSG_NET_CONNECT_SUCCESS:
    case MSG_NET_CONNECT_FAILURE:
    case MSG_NET_SERVER_KICK:
    case MSG_NET_USER_DISCONNECT:
    case MSG_NET_RECV_ERROR:
    case MSG_NET_REFRESH_SERVERLIST_SUCCESS:
    case MSG_NET_REFRESH_SERVERLIST_FAILURE:
    case MSG_NET_REFRESH_REPOLIST_SUCCESS:
    case MSG_NET_OPEN_RESOURCE_SUCCESS:
    case MSG_NET_REFRESH_REPOLIST_FAILURE:
    case MSG_NET_FETCH_AI_PRESETS_SUCCESS:
    case MSG_NET_FETCH_AI_PRESETS_FAILURE:
        // GUI
    case MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED:
    case MSG_GUI_DOWNLOAD_PROGRESS:
    case MSG_GUI_DOWNLOAD_FINISHED:
    case MSG_GUI_OPEN_SELECTOR_REQUESTED:
        // Editing
    case MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED:
        this->log(fmt::format("{} is not allowed.", log_msg));
        return false;


        // -- SOME ASSEMBLY REQUIRED --

        // Application
    case MSG_APP_LOAD_SCRIPT_REQUESTED:         //!< Payload = RoR::LoadScriptRequest* (owner)
    {
        LoadScriptRequest* rq = new LoadScriptRequest();
        bool has_filename = GetValueFromScriptDict(log_msg, dict, /*required:*/false, "filename", "string", rq->lsr_filename);
        bool has_buffer = GetValueFromScriptDict(log_msg, dict, /*required:*/false, "buffer", "string", rq->lsr_buffer);
        if (!has_filename && !has_buffer)
        {
            this->log(fmt::format("{}: ERROR, either 'filename' or 'buffer' must be set!", log_msg));
            delete rq;
            return false;
        }
        GetValueFromScriptDict(log_msg, dict, /*required:*/false, "category", "ScriptCategory", rq->lsr_category);
        if (rq->lsr_category == ScriptCategory::ACTOR)
        {
            int64_t instance_id; // AngelScript's `Dictionary` converts all ints int `int64`
            if (!GetValueFromScriptDict(log_msg, dict, /*required:*/true, "associated_actor", "int64", instance_id))
            {
                this->log(fmt::format("{}: WARNING, category 'ACTOR' specified but 'associated_actor' not given.", log_msg, rq->lsr_filename));
                delete rq;
                return false;
            }
        }
        m.payload = rq;
        break;
    }

    case MSG_APP_UNLOAD_SCRIPT_REQUESTED:       //!< Payload = RoR::ScriptUnitId_t* (owner)
    {
        int64_t id; // AngelScript's `Dictionary` converts all ints int `int64`
        if (!GetValueFromScriptDict(log_msg, dict, /*required:*/true, "id", "int64", id))
        {
            return false;
        }
        m.payload = new ScriptUnitId_t(static_cast<ScriptUnitId_t>(id));
        break;
    }

        // Simulation
    case MSG_SIM_LOAD_TERRN_REQUESTED:
        if (!GetValueFromScriptDict(log_msg, dict, /*required:*/true, "filename", "string", m.description))
        {
            return false;
        }
        break;

    case MSG_SIM_LOAD_SAVEGAME_REQUESTED:
        if (!GetValueFromScriptDict(log_msg, dict, /*required:*/true, "filename", "string", m.description))
        {
            return false;
        }
        break;

    case MSG_SIM_SPAWN_ACTOR_REQUESTED:         //!< Payload = RoR::ActorSpawnRequest* (owner)
    {
        ActorSpawnRequest* rq = new ActorSpawnRequest();

        // Get required params
        if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "filename", "string", rq->asr_filename) &&
            GetValueFromScriptDict(log_msg, dict, /*required:*/true, "position", "vector3", rq->asr_position) &&
            GetValueFromScriptDict(log_msg, dict, /*required:*/true, "rotation", "quaternion", rq->asr_rotation))
        {
            rq->asr_cache_entry = App::GetCacheSystem()->FindEntryByFilename(LT_AllBeam, /*partial=*/true, rq->asr_filename);
            if (!rq->asr_cache_entry)
            {
                this->log(fmt::format("{}: WARNING, vehicle '{}' is not installed.", log_msg, rq->asr_filename));
                delete rq;
                return false;
            }

            // Set sectionconfig
            GetValueFromScriptDict(log_msg, dict, /*required:*/false, "config", "string", rq->asr_config);
            // Make sure config exists
            if (rq->asr_config != "")
            {
                auto result = std::find(rq->asr_cache_entry->sectionconfigs.begin(), rq->asr_cache_entry->sectionconfigs.end(), rq->asr_config);
                if (result == rq->asr_cache_entry->sectionconfigs.end())
                {
                    this->log(fmt::format("{}: WARNING, configuration '{}' does not exist in '{}'.", log_msg, rq->asr_config, rq->asr_filename));
                    rq->asr_config = "";
                }
            }
            // If no config given (or was invalid), use the first available (classic behavior).
            if (rq->asr_config == "" && rq->asr_cache_entry->sectionconfigs.size() > 0)
            {
                rq->asr_config = rq->asr_cache_entry->sectionconfigs[0];
            }

            // Enter or not?
            GetValueFromScriptDict(log_msg, dict, /*required:*/false, "enter", "bool", rq->asr_enter);

            // Get skin
            std::string skin_name;
            if (GetValueFromScriptDict(log_msg, dict, /*required:*/false, "skin", "string", skin_name))
            {
                rq->asr_skin_entry = App::GetCacheSystem()->FetchSkinByName(skin_name);
                if (!rq->asr_skin_entry)
                    this->log(fmt::format("{}: WARNING, skin '{}' is not installed.", log_msg, skin_name));
            }

            m.payload = rq;
        }
        else
        {
            delete rq;
            return false;
        }
        break;
    }

    case MSG_SIM_MODIFY_ACTOR_REQUESTED:        //!< Payload = RoR::ActorModifyRequest* (owner)
    {
        ActorModifyRequest::Type modify_type;
        // `dictionary` converts all primitives to `double` or `int64`, see 'scriptdictionary.cpp', function `Set()`
        int64_t instance_id = -1;
        if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "type", "ActorModifyRequestType", modify_type) &&
            GetValueFromScriptDict(log_msg, dict, /*required:*/true, "instance_id", "int64", instance_id))
        {
            ActorModifyRequest* rq = new ActorModifyRequest();
            rq->amr_type = modify_type;
            rq->amr_actor = static_cast<ActorInstanceID_t>(instance_id);
            m.payload = rq;
        }
        else
        {
            return false;
        }
        break;
    }

    case MSG_SIM_DELETE_ACTOR_REQUESTED:        //!< Payload = RoR::ActorPtr* (owner)
    case MSG_SIM_HIDE_NET_ACTOR_REQUESTED:      //!< Payload = ActorPtr* (owner)
    case MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED:    //!< Payload = ActorPtr* (owner)
    {
        // `dictionary` converts all primitives to `double` or `int64`, see 'scriptdictionary.cpp', function `Set()`
        int64_t instance_id = -1;
        if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "instance_id", "int64", instance_id))
        {
            ActorPtr actor = App::GetGameContext()->GetActorManager()->GetActorById(instance_id);
            if (actor)
            {
                m.payload = new ActorPtr(actor);
            }
            else
            {
                this->log(fmt::format("{}: Actor with instance ID '{}' not found!", log_msg, instance_id));
                return false;
            }
        }
        else
        {
            return false;
        }
        break;
    }
    
    case MSG_SIM_SEAT_PLAYER_REQUESTED:         //!< Payload = RoR::ActorPtr (owner) | nullptr
    {
        // `dictionary` converts all primitives to `double` or `int64`, see 'scriptdictionary.cpp', function `Set()`
        int64_t instance_id = -1;
        ActorPtr actor;
        if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "instance_id", "int64", instance_id)
            && instance_id > -1)
        {
            actor = App::GetGameContext()->GetActorManager()->GetActorById(instance_id);
        }
        m.payload = new ActorPtr(actor);
        break;
    }

    case MSG_SIM_TELEPORT_PLAYER_REQUESTED:     //!< Payload = Ogre::Vector3* (owner)
    {
        Ogre::Vector3 position;
        if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "position", "vector3", position))
        {
            m.payload = new Ogre::Vector3(position);
        }
        else
        {
            return false;
        }
        break;
    }

    case MSG_SIM_ADD_FREEFORCE_REQUESTED:
    case MSG_SIM_MODIFY_FREEFORCE_REQUESTED:
    {
        // `dictionary` converts all primitives to `double` or `int64`, see 'scriptdictionary.cpp', function `Set()`
        FreeForceRequest* rq = new FreeForceRequest();
        if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "id", "int64", rq->ffr_id) &&
            GetValueFromScriptDict(log_msg, dict, /*required:*/true, "type", "FreeForceType", rq->ffr_type) &&
            GetValueFromScriptDict(log_msg, dict, /*required:*/true, "force_magnitude", "double", rq->ffr_force_magnitude) &&
            GetValueFromScriptDict(log_msg, dict, /*required:*/true, "base_actor", "int64", rq->ffr_base_actor) &&
            GetValueFromScriptDict(log_msg, dict, /*required:*/true, "base_node", "int64", rq->ffr_base_node))
        {
            switch (rq->ffr_type)
            {
                case (int64_t)FreeForceType::CONSTANT:
                    if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "force_const_direction", "vector3", rq->ffr_force_const_direction))
                    {
                        m.payload = rq;
                    }
                    else
                    {
                        delete rq;
                        return false;
                    }
                    break;

                case (int64_t)FreeForceType::TOWARDS_COORDS:
                    if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "target_coords", "vector3", rq->ffr_target_coords))
                    {
                        m.payload = rq;
                    }
                    else
                    {
                        delete rq;
                        return false;
                    }
                    break;

                case (int64_t)FreeForceType::TOWARDS_NODE:
                    if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "target_actor", "int64", rq->ffr_target_actor) &&
                        GetValueFromScriptDict(log_msg, dict, /*required:*/true, "target_node", "int64", rq->ffr_target_node))
                    {
                        m.payload = rq;
                    }
                    else
                    {
                        delete rq;
                        return false;
                    }
                    break;

                default:
                    this->log(fmt::format("{}: ERROR, invalid 'free force type' value '{}'", log_msg, rq->ffr_type));
                    delete rq;
                    return false;
            }
            m.payload = rq;
        }
        else
        {
            delete rq;
            return false;
        }
        break;
    }

    case MSG_SIM_REMOVE_FREEFORCE_REQUESTED:
    {
        // `dictionary` converts all primitives to `double` or `int64`, see 'scriptdictionary.cpp', function `Set()`
        int64_t id = -1;
        if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "id", "int64", id))
        {
            m.payload = new FreeForceID_t(id);
        }
        else
        {
            return false;
        }
        break;
    }

    case MSG_EDI_LOAD_BUNDLE_REQUESTED:        //!< Payload = RoR::CacheEntryPtr* (owner)
    case MSG_EDI_RELOAD_BUNDLE_REQUESTED:      //!< Payload = RoR::CacheEntryPtr* (owner)
    case MSG_EDI_UNLOAD_BUNDLE_REQUESTED:      //!< Payload = RoR::CacheEntryPtr* (owner)
    {
        CacheEntryPtr entry;
        if (GetValueFromScriptDict(log_msg, dict, /*required*/true, "cache_entry", "CacheEntryClass@", entry))
        {
            m.payload = new CacheEntryPtr(entry);
        }
        else
        {
            return false;
        }
        break;
    }

    case MSG_EDI_CREATE_PROJECT_REQUESTED:     //!< Payload = RoR::CreateProjectRequest* (owner)
    {
        CreateProjectRequest* request = new CreateProjectRequest();
        if (GetValueFromScriptDict(log_msg, dict, /*required:*/true, "name", "string", request->cpr_name) &&
            GetValueFromScriptDict(log_msg, dict, /*required:*/true, "source_entry", "CacheEntryClass@", request->cpr_source_entry))
        {
            m.payload = request;
        }
        else
        {
            delete request;
            return false;
        }
        break;
    }
    
    default:;
    }

    App::GetGameContext()->PushMessage(m);
    return true;
}

FreeForceID_t GameScript::getFreeForceNextId()
{
    return App::GetGameContext()->GetActorManager()->GetFreeForceNextId();
}

// --------------------------------
// Audio

CScriptArray* GameScript::getAllSoundScriptTemplates()
{
    return MapToScriptArray(App::GetSoundScriptManager()->getAllTemplates(), "SoundScriptTemplateClass@");
}

SoundScriptTemplatePtr GameScript::getSoundScriptTemplate(const std::string& name)
{
    return App::GetSoundScriptManager()->getTemplate(name);
}

AngelScript::CScriptArray* GameScript::getAllSoundScriptInstances()
{
    return VectorToScriptArray(App::GetSoundScriptManager()->getAllInstances(), "SoundScriptInstanceClass@");
}

SoundPtr GameScript::createSoundFromResource(const std::string& filename, const std::string& resource_group_name)
{
    return App::GetSoundScriptManager()->getSoundManager()->createSound(filename, resource_group_name);
}

SoundScriptInstancePtr GameScript::createSoundScriptInstance(const std::string& template_name, int actor_instance_id = SoundScriptInstance::ACTOR_ID_UNKNOWN)
{
    return App::GetSoundScriptManager()->createInstance(template_name, actor_instance_id);
}

bool GameScript::checkResourceExists(const std::string& filename, const std::string& resource_group)
{
    try
    {
        std::string resource_name = this->CheckFileAccess("checkResourceExists()", filename, resource_group);
        if (resource_name == "")
            return false; // Access denied - error already logged

        // Actually check for the resource
        return Ogre::ResourceGroupManager::getSingleton().resourceExists(resource_group, resource_name);
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::checkResourceExists()");
        return false;
    }
}

bool GameScript::deleteResource(const std::string& filename, const std::string& resource_group)
{
    try
    {
        std::string resource_name = this->CheckFileAccess("deleteResource()", filename, resource_group);
        if (resource_name == "")
            return false; // Access denied - error already logged

        // Actually delete the resource
        Ogre::ResourceGroupManager::getSingleton().deleteResource(resource_name, resource_group);
        return true;
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::deleteResource()");
        return false;
    }
}

std::string GameScript::loadTextResourceAsString(const std::string& filename, const std::string& resource_group)
{
    try
    {
        std::string resource_name = this->CheckFileAccess("loadTextResourceAsString()", filename, resource_group);
        if (resource_name == "")
            return ""; // Access denied - error already logged

        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(resource_name, resource_group);

        if (stream.isNull() || !stream->isReadable())
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_ERROR,
                fmt::format("loadTextResourceAsString() could not read resource '{}' in group '{}'",
                resource_name, resource_group));
            return "";
        }

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        // WORKAROUND: apparently `getAsString()` has some Linux-x64 issues (`eof()` triggers SIGINT):
        // https://discord.com/channels/136544456244461568/189904947649708032/1155952230130778262
        // Observed with OGRE 1.11.6
        std::string str;
        const size_t BUF_LEN = 4000;
        char buf[BUF_LEN] = {};
        bool eof = false;
        while (!eof)
        {
            size_t read_len = stream->read(buf, BUF_LEN);
            if (read_len < BUF_LEN)
            {
                buf[read_len] = 0;
                eof = true;
            }
            str.append(buf, read_len);
        }
        return str;
#else
        return stream->getAsString();
#endif
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::loadTextResourceAsString()");
        return "";
    }
}

bool GameScript::createTextResourceFromString(const std::string& data, const std::string& filename, const std::string& resource_group, bool overwrite/*=false*/)
{
    try
    {
        std::string resource_name = this->CheckFileAccess("createTextResourceFromString()", filename, resource_group);
        if (resource_name == "")
            return false; // Access denied - error already logged

        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().createResource(resource_name, resource_group, overwrite);

        if (stream.isNull() || !stream->isWriteable())
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_ERROR,
                fmt::format("createTextResourceFromString() could not create resource '{}' in group '{}'",
                resource_name, resource_group));
            return false;
        }

        stream->write(data.data(), data.size());
        return true;
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::createTextResourceFromString()");
        return false;
    }
}

AngelScript::CScriptArray* GameScript::findResourceFileInfo(const std::string& resource_group, const std::string& pattern, bool dirs /*= false*/)
{
    try
    {
        // Search the file system
        Ogre::FileInfoListPtr fileInfoList 
            = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo(resource_group, pattern, dirs);

        // Put results to array
        AngelScript::asITypeInfo* typeinfo = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl("array<dictionary>");
        AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(typeinfo);
        int stringTypeid = App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("string");
        for (const Ogre::FileInfo& fileinfo: *fileInfoList)
        {
            AngelScript::CScriptDictionary* dict = AngelScript::CScriptDictionary::Create(App::GetScriptEngine()->getEngine());
            dict->Set("filename", new std::string(fileinfo.filename), stringTypeid);
            dict->Set("basename", new std::string(fileinfo.basename), stringTypeid);
            dict->Set("compressedSize", (asINT64)fileinfo.compressedSize);
            dict->Set("uncompressedSize", (asINT64)fileinfo.uncompressedSize);

            arr->InsertLast(dict);
        }
        return arr;
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::findResourceFileInfo()");
        return nullptr;
    }
}

Ogre::Image GameScript::loadImageResource(const std::string& filename, const std::string& resource_group)
{
    try
    {
        std::string resource_name = this->CheckFileAccess("loadImageResource()", filename, resource_group);
        if (resource_name == "")
            return Ogre::Image(); // Access denied - error already logged

        Ogre::Image img;
        return img.load(resource_name, resource_group);
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::loadImageResource()");
        return Ogre::Image();
    }
}

bool GameScript::serializeMeshResource(const std::string& filename, const std::string& resource_group, const Ogre::MeshPtr& mesh)
{
    try
    {
        std::string resource_name = this->CheckFileAccess("serializeMeshResource()", filename, resource_group);
        if (resource_name == "")
            return false; // Access denied - error already logged

        Ogre::MeshSerializer ser;
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().createResource(resource_name, resource_group);
        ser.exportMesh(mesh.get(), stream);
        return true;
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("GameScript::serializeMeshResource()");
        return false;
    }
}

// ------------------------
// Helpers:

bool GameScript::HaveSimTerrain(const char* func_name)
{
    if (App::GetGameContext()->GetTerrain() == nullptr)
    {
        this->logFormat("Cannot execute '%s', terrain not ready", func_name);
        return false;
    }
    return true;
}

bool GameScript::HavePlayerAvatar(const char* func_name)
{
    if (App::GetGameContext()->GetPlayerCharacter() == nullptr)
    {
        this->logFormat("Cannot execute '%s', player avatar not ready", func_name);
        return false;
    }
    return true;
}

bool GameScript::HaveMainCamera(const char* func_name)
{
    if (App::GetCameraManager()->GetCamera() == nullptr)
    {
        this->logFormat("Cannot execute '%s', main camera not ready", func_name);
        return false;
    }
    return true;
}

std::string GameScript::CheckFileAccess(const char* func_name, const std::string& filename, const std::string& resource_group)
{
    // Extract filename and extension from the input, because OGRE allows absolute paths in resource system.
    // -----------------------------------------------------------------------------------------------------

    std::string basename, extension, path;
    Ogre::StringUtil::splitFullFilename(filename, basename, extension, path);
    if (path != "")
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("{}: access denied to '{}' with group '{}' - file paths are not allowed",
            func_name, filename, resource_group));
        return "";
    }
    else
    {
        return basename + "." + extension;
    }
}
