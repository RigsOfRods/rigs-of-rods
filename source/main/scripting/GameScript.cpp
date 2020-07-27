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

#include "GameScript.h"

#ifdef USE_CURL
#   include <curl/curl.h>
#   include <curl/easy.h>
#endif //USE_CURL

#include "OgreSubsystem.h"

// AS addons start
#include "scriptany/scriptany.h"
#include "scriptarray/scriptarray.h"
#include "scripthelper/scripthelper.h"
#include "scriptmath/scriptmath.h"
#include "scriptstdstring/scriptstdstring.h"
// AS addons end

#include "Application.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "Character.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "Console.h"
#include "GUIManager.h"
#include "Language.h"
#include "MainMenu.h"
#include "Network.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"
#include "SkyManager.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"
#include "Water.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

using namespace Ogre;
using namespace RoR;

// GUIDELINE: Make functions safe from invoking in wrong circumstances,
// i.e. when server script calls function using SimController while in main menu.
// --> Getter functions should silently return zero/empty value.
// --> Functions performing simulation changes should log warning and do nothing.

GameScript::GameScript(ScriptEngine* se) :
    mse(se)
{
}

GameScript::~GameScript()
{
}

void GameScript::log(const String& msg)
{
    ScriptEngine::getSingleton().SLOG(msg);
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

    ScriptEngine::getSingleton().SLOG(buffer);
}

void GameScript::activateAllVehicles()
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->GetBeamFactory()->WakeUpAllActors();
}

void GameScript::SetTrucksForcedAwake(bool forceActive)
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->GetBeamFactory()->SetTrucksForcedAwake(forceActive);
}

float GameScript::getTime()
{
    float result = 0.f;
    if (App::GetSimController())
        result = App::GetSimController()->getTime();
    return result;
}

void GameScript::setPersonPosition(const Vector3& vec)
{
    if (!this->HavePlayerAvatar(__FUNCTION__))
        return;

    gEnv->player->setPosition(vec);
}

void GameScript::loadTerrain(const String& terrain)
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::sim_terrain_name->SetPendingStr(terrain);
    App::GetSimController()->LoadTerrain();
}

Vector3 GameScript::getPersonPosition()
{
    Vector3 result(Vector3::ZERO);
    if (gEnv->player)
        result = gEnv->player->getPosition();
    return result;
}

void GameScript::movePerson(const Vector3& vec)
{
    if (!this->HavePlayerAvatar(__FUNCTION__))
        return;

    gEnv->player->move(vec);
}

void GameScript::setPersonRotation(const Radian& rot)
{
    if (!this->HavePlayerAvatar(__FUNCTION__))
        return;

    gEnv->player->setRotation(rot);
}

Radian GameScript::getPersonRotation()
{
    Radian result(0);
    if (gEnv->player)
        result = gEnv->player->getRotation();
    return result;
}

String GameScript::getCaelumTime()
{
    String result = "";
#ifdef USE_CAELUM
    if (App::GetSimTerrain())
    {
        result = App::GetSimTerrain()->getSkyManager()->GetPrettyTime();
    }
#endif // USE_CAELUM
    return result;
}

void GameScript::setCaelumTime(float value)
{
#ifdef USE_CAELUM
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    App::GetSimTerrain()->getSkyManager()->SetSkyTimeFactor(value);
#endif // USE_CAELUM
}

bool GameScript::getCaelumAvailable()
{
    bool result = false;
#ifdef USE_CAELUM
    if (App::GetSimTerrain())
        result = App::GetSimTerrain()->getSkyManager() != 0;
#endif // USE_CAELUM
    return result;
}

void GameScript::stopTimer()
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->StopRaceTimer();
}

void GameScript::startTimer(int id)
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->StartRaceTimer(id);
}

void GameScript::setTimeDiff(float diff)
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->SetRaceTimeDiff(diff);
}

void GameScript::setBestLapTime(float time)
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->SetRaceBestTime(time);
}

void GameScript::setWaterHeight(float value)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    if (App::GetSimTerrain()->getWater())
    {
        IWater* water = App::GetSimTerrain()->getWater();
        water->WaterSetCamera(gEnv->mainCamera);
        water->SetStaticWaterHeight(value);
        water->UpdateWater();
    }
}

float GameScript::getGroundHeight(Vector3& v)
{
    float result = -1.0f;
    if (App::GetSimTerrain())
        result = App::GetSimTerrain()->GetHeightAt(v.x, v.z);
    return result;
}

float GameScript::getWaterHeight()
{
    float result = 0.0f;
    if (App::GetSimTerrain() && App::GetSimTerrain()->getWater())
        result = App::GetSimTerrain()->getWater()->GetStaticWaterHeight();
    return result;
}

Actor* GameScript::getCurrentTruck()
{
    Actor* result = nullptr;
    if (App::GetSimController())
        result = App::GetSimController()->GetPlayerActor();
    return result;
}

float GameScript::getGravity()
{
    float result = 0.f;
    if (App::GetSimTerrain())
    {
        result = App::GetSimTerrain()->getGravity();
    }
    return result;
}

void GameScript::setGravity(float value)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    App::GetSimTerrain()->setGravity(value);
}

Actor* GameScript::getTruckByNum(int num)
{
    // TODO: Do we have to add a 'GetActorByIndex' method to keep this backwards compatible?
    Actor* result = nullptr;
    if (App::GetSimController())
        result = App::GetSimController()->GetActorById(num);
    return result;
}

int GameScript::getNumTrucks()
{
    int result = 0;
    if (App::GetSimController())
        result = static_cast<int>(App::GetSimController()->GetBeamFactory()->GetActors().size());
    return result;
}

int GameScript::getNumTrucksByFlag(int flag)
{
    if (App::GetSimController() == nullptr)
        return 0;

    int result = 0;
    for (auto actor : App::GetSimController()->GetActors())
    {
        if (!flag || static_cast<int>(actor->ar_sim_state) == flag)
            result++;
    }
    return result;
}

int GameScript::GetPlayerActorId()
{
    if (App::GetSimController() == nullptr)
        return -1;

    Actor* actor = App::GetSimController()->GetPlayerActor();
    return (actor != nullptr) ? actor->ar_instance_id : -1;
}

void GameScript::registerForEvent(int eventValue)
{
    if (mse)
    {
        mse->eventMask = -1;
        mse->eventMask += eventValue;
    }
}

void GameScript::flashMessage(String& txt, float time, float charHeight)
{
    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, "script_code_red.png");
}

void GameScript::message(String& txt, String& icon, float timeMilliseconds, bool forceVisible)
{
    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, icon, timeMilliseconds, forceVisible);
}

void GameScript::UpdateDirectionArrow(String& text, Vector3& vec)
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->UpdateDirectionArrow(const_cast<char*>(text.c_str()), Vector3(vec.x, vec.y, vec.z));
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
    if (!this->HaveSimController(__FUNCTION__))
        return;

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
        App::GetSimController()->ShowLoaderGUI(ntype, instance, box);
    }

}

void GameScript::repairVehicle(const String& instance, const String& box, bool keepPosition)
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->GetBeamFactory()->RepairActor(gEnv->collisions, instance, box, keepPosition);
}

void GameScript::removeVehicle(const String& event_source_instance_name, const String& event_source_box_name)
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->RemoveActorByCollisionBox(event_source_instance_name, event_source_box_name);
}

void GameScript::destroyObject(const String& instanceName)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    if (App::GetSimTerrain()->getObjectManager())
    {
        App::GetSimTerrain()->getObjectManager()->unloadObject(instanceName);
    }
}

void GameScript::MoveTerrainObjectVisuals(const String& instanceName, const Vector3& pos)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    if (App::GetSimTerrain()->getObjectManager())
    {
        App::GetSimTerrain()->getObjectManager()->MoveObjectVisuals(instanceName, pos);
    }
}

void GameScript::spawnObject(const String& objectName, const String& instanceName, const Vector3& pos, const Vector3& rot, const String& eventhandler, bool uniquifyMaterials)
{
    if (!this->HaveSimTerrain(__FUNCTION__))
        return;

    if ((App::GetSimTerrain()->getObjectManager() == nullptr))
    {
        this->logFormat("spawnObject(): Cannot spawn object, no terrain loaded!");
        return;
    }

    try
    {
        AngelScript::asIScriptModule* module = mse->getEngine()->GetModule(mse->moduleName, AngelScript::asGM_ONLY_IF_EXISTS);
        if (module == nullptr)
        {
            this->logFormat("spawnObject(): Failed to fetch/create script module '%s'", mse->moduleName);
            return;
        }

        int handler_func_id = -1; // no function
        if (!eventhandler.empty())
        {
            AngelScript::asIScriptFunction* handler_func = module->GetFunctionByName(eventhandler.c_str());
            if (handler_func != nullptr)
            {
                handler_func_id = handler_func->GetId();
            }
            else
            {
                this->logFormat("spawnObject(): Warning; Failed to find handler function '%s' in script module '%s'",
                    eventhandler.c_str(), mse->moduleName);
            }
        }

        SceneNode* bakeNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        const String type = "";
        App::GetSimTerrain()->getObjectManager()->LoadTerrainObject(objectName, pos, rot, bakeNode, instanceName, type, true, handler_func_id, uniquifyMaterials);
    }
    catch (std::exception& e)
    {
        this->logFormat("spawnObject(): An exception occurred, message: %s", e.what());
        return;
    }
}

void GameScript::hideDirectionArrow()
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    App::GetSimController()->UpdateDirectionArrow(0, Vector3::ZERO);
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
    catch (Exception& e)
    {
        this->log("Exception in setMaterialAmbient(): " + e.getFullDescription());
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
    catch (Exception& e)
    {
        this->log("Exception in setMaterialDiffuse(): " + e.getFullDescription());
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
    catch (Exception& e)
    {
        this->log("Exception in setMaterialSpecular(): " + e.getFullDescription());
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
    catch (Exception& e)
    {
        this->log("Exception in setMaterialEmissive(): " + e.getFullDescription());
        return 0;
    }
    return 1;
}

int GameScript::getSafeTextureUnitState(TextureUnitState** tu, const String materialName, int techniqueNum, int passNum, int textureUnitNum)
{
    try
    {
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
    catch (Exception& e)
    {
        this->log("Exception in getSafeTextureUnitState(): " + e.getFullDescription());
    }
    return 1;
}

int GameScript::setMaterialTextureName(const String& materialName, int techniqueNum, int passNum, int textureUnitNum, const String& textureName)
{
    TextureUnitState* tu = 0;
    int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
    if (res == 0 && tu != 0)
    {
        // finally, set it
        tu->setTextureName(textureName);
    }
    return res;
}

int GameScript::setMaterialTextureRotate(const String& materialName, int techniqueNum, int passNum, int textureUnitNum, float rotation)
{
    TextureUnitState* tu = 0;
    int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
    if (res == 0 && tu != 0)
    {
        tu->setTextureRotate(Degree(rotation));
    }
    return res;
}

int GameScript::setMaterialTextureScroll(const String& materialName, int techniqueNum, int passNum, int textureUnitNum, float sx, float sy)
{
    TextureUnitState* tu = 0;
    int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
    if (res == 0 && tu != 0)
    {
        tu->setTextureScroll(sx, sy);
    }
    return res;
}

int GameScript::setMaterialTextureScale(const String& materialName, int techniqueNum, int passNum, int textureUnitNum, float u, float v)
{
    TextureUnitState* tu = 0;
    int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
    if (res == 0 && tu != 0)
    {
        tu->setTextureScale(u, v);
    }
    return res;
}

float GameScript::rangeRandom(float from, float to)
{
    return Math::RangeRandom(from, to);
}

int GameScript::getLoadedTerrain(String& result)
{
    String terrainName = "";

    if (App::GetSimTerrain())
    {
        terrainName = App::GetSimTerrain()->getTerrainName();
        result = terrainName;
    }

    return !terrainName.empty();
}

void GameScript::clearEventCache()
{
    if (gEnv->collisions == nullptr)
    {
        this->logFormat("Cannot execute '%s', collisions not ready", __FUNCTION__);
        return;
    }

    gEnv->collisions->clearEventCache();
}

void GameScript::setCameraPosition(const Vector3& pos)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    gEnv->mainCamera->setPosition(Vector3(pos.x, pos.y, pos.z));
}

void GameScript::setCameraDirection(const Vector3& rot)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    gEnv->mainCamera->setDirection(Vector3(rot.x, rot.y, rot.z));
}

void GameScript::setCameraOrientation(const Quaternion& q)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    gEnv->mainCamera->setOrientation(Quaternion(q.w, q.x, q.y, q.z));
}

void GameScript::setCameraYaw(float rotX)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    gEnv->mainCamera->yaw(Degree(rotX));
}

void GameScript::setCameraPitch(float rotY)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    gEnv->mainCamera->pitch(Degree(rotY));
}

void GameScript::setCameraRoll(float rotZ)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    gEnv->mainCamera->roll(Degree(rotZ));
}

Vector3 GameScript::getCameraPosition()
{
    Vector3 result(Vector3::ZERO);
    if (gEnv->mainCamera)
        result = gEnv->mainCamera->getPosition();
    return result;
}

Vector3 GameScript::getCameraDirection()
{
    Vector3 result(Vector3::ZERO);
    if (gEnv->mainCamera)
        result = gEnv->mainCamera->getDirection();
    return result;
}

Quaternion GameScript::getCameraOrientation()
{
    Quaternion result(Quaternion::ZERO);
    if (gEnv->mainCamera)
        result = gEnv->mainCamera->getOrientation();
    return result;
}

void GameScript::cameraLookAt(const Vector3& pos)
{
    if (!this->HaveMainCamera(__FUNCTION__))
        return;

    gEnv->mainCamera->lookAt(Vector3(pos.x, pos.y, pos.z));
}

int GameScript::useOnlineAPI(const String& apiquery, const AngelScript::CScriptDictionary& dict, String& result)
{
    if (!this->HaveSimController(__FUNCTION__))
        return 1;

    if (App::app_disable_online_api->GetActiveVal<bool>())
        return 0;

    Actor* player_actor = App::GetSimController()->GetPlayerActor();

    if (player_actor == nullptr)
        return 1;

    std::string hashtok = Utils::Sha1Hash(App::mp_player_name->GetActiveStr());
    std::string url = App::mp_api_url->GetActiveStr() + apiquery;
    std::string user = std::string("RoR-Api-User: ") + App::mp_player_name->GetActiveStr();
    std::string token = std::string("RoR-Api-User-Token: ") + hashtok;

    std::string terrain_name = App::GetSimTerrain()->getTerrainName();

    std::string script_name = mse->getScriptName();
    std::string script_hash = mse->getScriptHash();

    rapidjson::Document j_doc;
    j_doc.SetObject();

    j_doc.AddMember("user-name", rapidjson::StringRef(App::mp_player_name->GetActiveStr().c_str()), j_doc.GetAllocator());
    j_doc.AddMember("user-country", rapidjson::StringRef(App::app_country->GetActiveStr().c_str()), j_doc.GetAllocator());
    j_doc.AddMember("user-token", rapidjson::StringRef(hashtok.c_str()), j_doc.GetAllocator());

    j_doc.AddMember("terrain-name", rapidjson::StringRef(terrain_name.c_str()), j_doc.GetAllocator());
    j_doc.AddMember("terrain-filename", rapidjson::StringRef(App::sim_terrain_name->GetActiveStr().c_str()), j_doc.GetAllocator());

    j_doc.AddMember("script-name", rapidjson::StringRef(script_name.c_str()), j_doc.GetAllocator());
    j_doc.AddMember("script-hash", rapidjson::StringRef(script_hash.c_str()), j_doc.GetAllocator());

    j_doc.AddMember("actor-name", rapidjson::StringRef(player_actor->ar_design_name.c_str()), j_doc.GetAllocator());
    j_doc.AddMember("actor-filename", rapidjson::StringRef(player_actor->ar_filename.c_str()), j_doc.GetAllocator());
    j_doc.AddMember("actor-hash", rapidjson::StringRef(player_actor->ar_filehash.c_str()), j_doc.GetAllocator());

    rapidjson::Value j_linked_actors(rapidjson::kArrayType);
    for (auto actor : player_actor->GetAllLinkedActors())
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

    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
            _L("using Online API..."), "information.png", 2000);

    LOG("[RoR|GameScript] Submitting race results to '" + url + "'");

    std::thread([url, user, token, json]()
        {
            struct curl_slist *slist = NULL;
            slist = curl_slist_append(slist, "Accept: application/json");
            slist = curl_slist_append(slist, "Content-Type: application/json");
            slist = curl_slist_append(slist, user.c_str());
            slist = curl_slist_append(slist, token.c_str());

            CURL *curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    slist);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    json.c_str());
            curl_easy_perform(curl);

            curl_easy_cleanup(curl);
            curl = nullptr;
            curl_slist_free_all(slist);
            slist = NULL;
        }).detach();

    return 0;
}

void GameScript::boostCurrentTruck(float factor)
{
    if (!this->HaveSimController(__FUNCTION__))
        return;

    Actor* actor = App::GetSimController()->GetPlayerActor();
    if (actor && actor->ar_engine)
    {
        float rpm = actor->ar_engine->GetEngineRpm();
        rpm += 2000.0f * factor;
        actor->ar_engine->SetEngineRpm(rpm);
    }
}

int GameScript::addScriptFunction(const String& arg)
{
    return mse->addFunction(arg);
}

int GameScript::scriptFunctionExists(const String& arg)
{
    return mse->functionExists(arg);
}

int GameScript::deleteScriptFunction(const String& arg)
{
    return mse->deleteFunction(arg);
}

int GameScript::addScriptVariable(const String& arg)
{
    return mse->addVariable(arg);
}

int GameScript::deleteScriptVariable(const String& arg)
{
    return mse->deleteVariable(arg);
}

int GameScript::sendGameCmd(const String& message)
{
#ifdef USE_SOCKETW
    if (RoR::App::mp_state->GetActiveEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        RoR::Networking::AddPacket(0, RoRnet::MSG2_GAME_CMD, (int)message.size(), const_cast<char*>(message.c_str()));
        return 0;
    }
#endif // USE_SOCKETW

    return -11;
}

VehicleAI* GameScript::getCurrentTruckAI()
{
    VehicleAI* result = nullptr;
    if (App::GetSimController())
    {
        Actor* actor = App::GetSimController()->GetPlayerActor();
        if (actor != nullptr)
            result = actor->ar_vehicle_ai;
    }
    return result;
}

VehicleAI* GameScript::getTruckAIByNum(int num)
{
    VehicleAI* result = nullptr;
    if (App::GetSimController())
    {
        Actor* actor = App::GetSimController()->GetActorById(num);
        if (actor != nullptr)
            result = actor->ar_vehicle_ai;
    }
    return result;
}

Actor* GameScript::spawnTruck(const std::string& truckName, const Ogre::Vector3& pos, const Ogre::Vector3& rot, const std::string& skinName)
{
    if (!this->HaveSimController(__FUNCTION__))
        return nullptr;

    ActorSpawnRequest rq;
    rq.asr_position = pos;
    rq.asr_rotation = Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z);
    rq.asr_filename = truckName;
    rq.asr_skin_entry = App::GetCacheSystem()->FetchSkinByName(skinName);
    return App::GetSimController()->SpawnActorDirectly(rq);
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
    App::app_state->SetPendingVal((int)AppState::MAIN_MENU);
}

void GameScript::quitGame()
{
    RoR::App::app_state->SetPendingVal((int)RoR::AppState::SHUTDOWN);
}

float GameScript::getFPS()
{
    return App::GetOgreSubsystem()->GetRenderWindow()->getStatistics().lastFPS;
}

float GameScript::getAvgFPS()
{
    return App::GetOgreSubsystem()->GetRenderWindow()->getStatistics().avgFPS;
}

bool GameScript::HaveSimController(const char* func_name)
{
    if (App::GetSimController() == nullptr)
    {
        this->logFormat("Cannot execute '%s', simulation not ready", func_name);
        return false;
    }
    return true;
}

bool GameScript::HaveSimTerrain(const char* func_name)
{
    if (App::GetSimTerrain() == nullptr)
    {
        this->logFormat("Cannot execute '%s', terrain not ready", func_name);
        return false;
    }
    return true;
}

bool GameScript::HavePlayerAvatar(const char* func_name)
{
    if (gEnv->player == nullptr)
    {
        this->logFormat("Cannot execute '%s', player avatar not ready", func_name);
        return false;
    }
    return true;
}

bool GameScript::HaveMainCamera(const char* func_name)
{
    if (gEnv->mainCamera == nullptr)
    {
        this->logFormat("Cannot execute '%s', main camera not ready", func_name);
        return false;
    }
    return true;
}
