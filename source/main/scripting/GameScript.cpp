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
#include <stdio.h>
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>
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
#include "Character.h"
#include "Collisions.h"
#include "GUI_GameConsole.h"
#include "GUIManager.h"
#include "Language.h"
#include "MainMenu.h"
#include "Network.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"
#include "SkyManager.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"
#include "Water.h"

#include <thread>

using namespace Ogre;
using namespace RoR;

/* class that implements the interface for the scripts */
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
    App::GetSimController()->GetBeamFactory()->WakeUpAllActors();
}

void GameScript::SetTrucksForcedAwake(bool forceActive)
{
    App::GetSimController()->GetBeamFactory()->SetTrucksForcedAwake(forceActive);
}

double GameScript::getTime()
{
    auto sim_controller = App::GetSimController();
    if (sim_controller == nullptr)
    {
        return 0.0;
    }
    else
    {
        return sim_controller->getTime();
    }
}

void GameScript::setPersonPosition(const Vector3& vec)
{
    if (gEnv->player)
        gEnv->player->setPosition(vec);
}

void GameScript::loadTerrain(const String& terrain)
{
    App::sim_terrain_name.SetPending(terrain.c_str());
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
    if (gEnv->player)
        gEnv->player->move(vec);
}

void GameScript::setPersonRotation(const Radian& rot)
{
    if (gEnv->player)
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
    if (App::GetSimTerrain())
    {
        App::GetSimTerrain()->getSkyManager()->SetSkyTimeFactor(value);
    }
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

float GameScript::stopTimer()
{
    return App::GetSimController()->StopRaceTimer();
}

void GameScript::startTimer()
{
    return App::GetSimController()->StartRaceTimer();
}

void GameScript::setWaterHeight(float value)
{
    if (App::GetSimTerrain() && App::GetSimTerrain()->getWater())
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
    return App::GetSimController()->GetPlayerActor();
}

float GameScript::getGravity()
{
    return App::GetSimTerrain()->getGravity();
}

void GameScript::setGravity(float value)
{
    App::GetSimTerrain()->setGravity(value);
}

Actor* GameScript::getTruckByNum(int num)
{
    // TODO: Do we have to add a 'GetActorByIndex' method to keep this backwards compatible?
    return App::GetSimController()->GetActorById(num);
}

int GameScript::getNumTrucks()
{
    return App::GetSimController()->GetBeamFactory()->GetActors().size();
}

int GameScript::getNumTrucksByFlag(int flag)
{
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
    Actor* actor = App::GetSimController()->GetPlayerActor();
    return (actor != nullptr) ? actor->ar_instance_id : -1;
}

void GameScript::registerForEvent(int eventValue)
{
    if (mse)
        mse->eventMask += eventValue;
}

void GameScript::flashMessage(String& txt, float time, float charHeight)
{

    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, "script_code_red.png");
    RoR::App::GetGuiManager()->PushNotification("Script:", txt);

}

void GameScript::message(String& txt, String& icon, float timeMilliseconds, bool forceVisible)
{
    //TODO: Notification system

    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, icon, timeMilliseconds, forceVisible);
    RoR::App::GetGuiManager()->PushNotification("Script:", txt);

}

void GameScript::UpdateDirectionArrow(String& text, Vector3& vec)
{
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
        ntype = LT_Heli;
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
    App::GetSimController()->GetBeamFactory()->RepairActor(gEnv->collisions, instance, box, keepPosition);
}

void GameScript::removeVehicle(const String& event_source_instance_name, const String& event_source_box_name)
{
    App::GetSimController()->RemoveActorByCollisionBox(event_source_instance_name, event_source_box_name);
}

void GameScript::destroyObject(const String& instanceName)
{
    if (App::GetSimTerrain() && App::GetSimTerrain()->getObjectManager())
    {
        App::GetSimTerrain()->getObjectManager()->unloadObject(instanceName);
    }
}

void GameScript::MoveTerrainObjectVisuals(const String& instanceName, const Vector3& pos)
{
    if (App::GetSimTerrain() && App::GetSimTerrain()->getObjectManager())
    {
        App::GetSimTerrain()->getObjectManager()->MoveObjectVisuals(instanceName, pos);
    }
}

void GameScript::spawnObject(const String& objectName, const String& instanceName, const Vector3& pos, const Vector3& rot, const String& eventhandler, bool uniquifyMaterials)
{
    if ((App::GetSimTerrain() == nullptr) || (App::GetSimTerrain()->getObjectManager() == nullptr))
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
    catch (std::exception e)
    {
        this->logFormat("spawnObject(): An exception occurred, message: %s", e.what());
        return;
    }
}

void GameScript::hideDirectionArrow()
{
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
    catch (Exception e)
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
    catch (Exception e)
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
    catch (Exception e)
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
    catch (Exception e)
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
    catch (Exception e)
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
    if (gEnv->collisions)
        gEnv->collisions->clearEventCache();
}

void GameScript::setCameraPosition(const Vector3& pos)
{
    if (gEnv->mainCamera)
        gEnv->mainCamera->setPosition(Vector3(pos.x, pos.y, pos.z));
}

void GameScript::setCameraDirection(const Vector3& rot)
{
    if (gEnv->mainCamera)
        gEnv->mainCamera->setDirection(Vector3(rot.x, rot.y, rot.z));
}

void GameScript::setCameraOrientation(const Quaternion& q)
{
    if (gEnv->mainCamera)
        gEnv->mainCamera->setOrientation(Quaternion(q.w, q.x, q.y, q.z));
}

void GameScript::setCameraYaw(float rotX)
{
    if (gEnv->mainCamera)
        gEnv->mainCamera->yaw(Degree(rotX));
}

void GameScript::setCameraPitch(float rotY)
{
    if (gEnv->mainCamera)
        gEnv->mainCamera->pitch(Degree(rotY));
}

void GameScript::setCameraRoll(float rotZ)
{
    if (gEnv->mainCamera)
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
    if (gEnv->mainCamera)
        gEnv->mainCamera->lookAt(Vector3(pos.x, pos.y, pos.z));
}

#ifdef USE_CURL
//hacky hack to fill memory with data for curl
// from: http://curl.haxx.se/libcurl/c/getinmemory.html
static size_t curlWriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    size_t realsize = size * nmemb;
    struct curlMemoryStruct* mem = (struct curlMemoryStruct *)data;
    char* new_mem;

    new_mem = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (new_mem == NULL)
    {
        free(mem->memory);
        printf("Error (re)allocating memory\n");
        exit(EXIT_FAILURE);
    }
    mem->memory = new_mem;

    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}
#endif //USE_CURL

int GameScript::useOnlineAPIDirectly(OnlineAPIParams_t params)
{
    // ### Disabled until new multiplayer portal supports it ##

#if 0 
//#ifdef USE_CURL
    struct curlMemoryStruct chunk;

    chunk.memory = (char *)malloc(1); /* will be grown as needed by the realloc above */
    chunk.size = 0; /* no data at this point */

    // construct post fields
    struct curl_httppost* formpost = NULL;
    struct curl_httppost* lastptr = NULL;

    std::map<String, AngelScript::CScriptDictionary::valueStruct>::const_iterator it;
    for (it = params.dict->dict.begin(); it != params.dict->dict.end(); it++)
    {
        int typeId = it->second.typeId;
        if (typeId == mse->getEngine()->GetTypeIdByDecl("string"))
        {
            // its a String
            String* str = (String *)it->second.valueObj;
            curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, str->c_str(), CURLFORM_END);
        }
        else if (typeId == AngelScript::asTYPEID_INT8
            || typeId == AngelScript::asTYPEID_INT16
            || typeId == AngelScript::asTYPEID_INT32
            || typeId == AngelScript::asTYPEID_INT64)
        {
            // its an integer
            curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((int)it->second.valueInt).c_str(), CURLFORM_END);
        }
        else if (typeId == AngelScript::asTYPEID_UINT8
            || typeId == AngelScript::asTYPEID_UINT16
            || typeId == AngelScript::asTYPEID_UINT32
            || typeId == AngelScript::asTYPEID_UINT64)
        {
            // its an unsigned integer
            curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((unsigned int)it->second.valueInt).c_str(), CURLFORM_END);
        }
        else if (typeId == AngelScript::asTYPEID_FLOAT || typeId == AngelScript::asTYPEID_DOUBLE)
        {
            // its a float or double
            curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((float)it->second.valueFlt).c_str(), CURLFORM_END);
        }
    }

    // add some hard coded values
    //curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_Name", CURLFORM_COPYCONTENTS, gEnv->frameListener->terrainName.c_str(), CURLFORM_END);
    //curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_FileName", CURLFORM_COPYCONTENTS, gEnv->frameListener->terrainFileName.c_str(), CURLFORM_END);
    //curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_FileHash", CURLFORM_COPYCONTENTS, gEnv->frameListener->terrainFileHash.c_str(), CURLFORM_END);
    //curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ModHash", CURLFORM_COPYCONTENTS, gEnv->frameListener->terrainModHash.c_str(), CURLFORM_END);
    int port = App::GetMpServerPort();
    std::string server_port_str = "-";
    if (port != 0)
    {
        server_port_str = TOSTRING(port);
    }
    const bool mp_connected = (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ScriptName", CURLFORM_COPYCONTENTS, mse->getScriptName().c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ScriptHash", CURLFORM_COPYCONTENTS, mse->getScriptHash().c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_NickName", CURLFORM_COPYCONTENTS, App::mp_player_name.GetActive().c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Language", CURLFORM_COPYCONTENTS, App::app_language.GetActive().c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Token", CURLFORM_COPYCONTENTS, SSETTING("User Token Hash", "-").c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionString", CURLFORM_COPYCONTENTS, ROR_VERSION_STRING, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_ProtocolVersion", CURLFORM_COPYCONTENTS, RORNET_VERSION, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_BinaryHash", CURLFORM_COPYCONTENTS, "-", CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_GUID", CURLFORM_COPYCONTENTS, "-", CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerName", CURLFORM_COPYCONTENTS, App::GetMpServerHost().c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerPort", CURLFORM_COPYCONTENTS, server_port_str.c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_NetworkEnabled", CURLFORM_COPYCONTENTS, (mp_connected) ? "Yes" : "No", CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "APIProtocolVersion", CURLFORM_COPYCONTENTS, "2", CURLFORM_END);

    if (App::GetSimController()->GetBeamFactory()->GetPlayerActorInternal())
    {
        Beam* truck = App::GetSimController()->GetBeamFactory()->GetPlayerActorInternal();
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_Name", CURLFORM_COPYCONTENTS, truck->GetActorDesignName().c_str(), CURLFORM_END);
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_FileName", CURLFORM_COPYCONTENTS, truck->GetActorFileName().c_str(), CURLFORM_END);

        // look for any locked trucks
        int i = 0;
        for (std::vector<hook_t>::iterator it = truck->hooks.begin(); it != truck->hooks.end(); it++ , i++)
        {
            Beam* trailer = it->hk_locked_actor;
            if (trailer && trailer->getTruckName() != trailer->GetActorDesignName())
            {
                String name = "Trailer_" + TOSTRING(i) + "_Name";
                curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, name.c_str(), CURLFORM_COPYCONTENTS, trailer->GetActorDesignName().c_str(), CURLFORM_END);
                String filename = "Trailer_" + TOSTRING(i) + "_FileName";
                curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, filename.c_str(), CURLFORM_COPYCONTENTS, trailer->GetActorFileName().c_str(), CURLFORM_END);
            }
        }
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Trailer_Count", CURLFORM_COPYCONTENTS, TOSTRING(i).c_str(), CURLFORM_END);
    }

    const RenderTarget::FrameStats& stats = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getStatistics();
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "AVG_FPS", CURLFORM_COPYCONTENTS, TOSTRING(stats.avgFPS).c_str(), CURLFORM_END);

    CURLcode res;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        LOG("ERROR: failed to init curl");
        return 1;
    }

    char* curl_err_str[CURL_ERROR_SIZE];
    memset(curl_err_str, 0, CURL_ERROR_SIZE);

    String url = "http://" + String(REPO_SERVER) + params.apiquery;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    /* send all data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    // set post options
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    // logging stuff
    //curl_easy_setopt(curl, CURLOPT_STDERR,           LogManager::getsin InstallerLog::getSingleton()->getLogFilePtr());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_str[0]);

    // http related settings
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); // follow redirects
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1); // set the Referrer: field in requests where it follows a Location: redirect.
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 20);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RoR");
    curl_easy_setopt(curl, CURLOPT_FILETIME, 1);

    // TO BE DONE: ADD SSL
    // see: http://curl.haxx.se/libcurl/c/simplessl.html
    // curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    //printf("%lu bytes retrieved\n", (long)chunk.size);

    curl_formfree(formpost);

    String result;

    if (chunk.memory)
    {
        // convert memory into String now
        result = String(chunk.memory);

        // then free
        free(chunk.memory);
    }

    /* we're done with libcurl, so clean it up */
    curl_global_cleanup();

    if (res != CURLE_OK)
    {
        const char* errstr = curl_easy_strerror(res);
        result = "ERROR: " + String(errstr);
    }

    LOG("online API result: " + result);


    Console* con = RoR::App::GetConsole();
    if (con)
    {
        con->putMessage(Console::CONSOLE_MSGTYPE_HIGHSCORE, Console::CONSOLE_SYSTEM_NOTICE, ANSI_TO_UTF(result));
        RoR::App::GetGuiManager()->PushNotification("Script:", ANSI_TO_UTF(result));
    }

#endif //USE_CURL
    return 0;
}

int GameScript::useOnlineAPI(const String& apiquery, const AngelScript::CScriptDictionary& d, String& result)
{
#if 0 // ========================== disabled until new multiplayer portal supports it =============================
      // At the moment, the call to "useOnlineAPIDirectly()" is dummy, making this whole function dummy.

    // malloc this, so we are safe from this function scope
    OnlineAPIParams_t* params = (OnlineAPIParams_t *)malloc(sizeof(OnlineAPIParams_t));
    if (!params)
    {
        free(params);
        return 1;
    }
    params->cls = this;
    strncpy(params->apiquery, apiquery.c_str(), 2048);

    //wrap a new dict around this, as we dont know if or when the script will release it
    AngelScript::CScriptDictionary* newDict = new AngelScript::CScriptDictionary(mse->getEngine());
    // copy over the dict, the main data
    newDict->dict = d.dict;
    // assign it to the data container
    params->dict = newDict;
    // tell the script that there will be no direct feedback
    result = "asynchronous";


    Console* con = RoR::App::GetConsole();
    if (con)
        con->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("using Online API..."), "information.png", 2000);
    RoR::App::GetGuiManager()->PushNotification("Notice:", _L("using Online API..."));


    // fix the String objects in the dict
    // why we need to do this: when we copy the std::map (dict) over, we calso jsut copy the pointers to String in it.
    // when this continues and forks, AS releases the strings.
    // so we will allocate new strings that are persistent.
    std::map<String, AngelScript::CScriptDictionary::valueStruct>::iterator it;
    for (it = params->dict->dict.begin(); it != params->dict->dict.end(); it++)
    {
        int typeId = it->second.typeId;
        if (typeId == mse->getEngine()->GetTypeIdByDecl("string"))
        {
            // its a String, copy it over
            String* str = (String *)it->second.valueObj;
            it->second.valueObj = (void *)new String(*str);
        }
    }

    // create the thread
    LOG("creating thread for online API usage...");

    std::thread([params]()
        {
            // call the function
            params->cls->useOnlineAPIDirectly(*params);

            // free the params
            params->dict->Release();
            free(params);
        }).detach();

#endif // #if 0 =============== END disabled block of code ========================

    return 0;
}

void GameScript::boostCurrentTruck(float factor)
{
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
    if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
    {
        RoR::Networking::AddPacket(0, RoRnet::MSG2_GAME_CMD, (int)message.size(), const_cast<char*>(message.c_str()));
        return 0;
    }
#endif // USE_SOCKETW

    return -11;
}

VehicleAI* GameScript::getCurrentTruckAI()
{
    Actor* actor = App::GetSimController()->GetPlayerActor();
    if (actor != nullptr)
        return actor->ar_vehicle_ai;
    return nullptr;
}

VehicleAI* GameScript::getTruckAIByNum(int num)
{
    Actor* b = App::GetSimController()->GetActorById(num);
    if (b)
        return b->ar_vehicle_ai;
    return nullptr;
}

Actor* GameScript::spawnTruck(Ogre::String& truckName, Ogre::Vector3& pos, Ogre::Vector3& rot)
{
    ActorSpawnRequest rq;
    rq.asr_position = pos;
    rq.asr_rotation = Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z);
    rq.asr_filename = truckName;
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
    App::app_state.SetPending(AppState::MAIN_MENU);
}

void GameScript::quitGame()
{
    RoR::App::app_state.SetPending(RoR::AppState::SHUTDOWN);
}

float GameScript::getFPS()
{
    return App::GetOgreSubsystem()->GetRenderWindow()->getStatistics().lastFPS;
}
