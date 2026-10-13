/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

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

#include "../../autowrapper/aswrappedcall.h"
#include "Actor.h"
#include "AngelScriptBindings.h"
#include "GameScript.h"
#include "ScriptEngine.h"

#include <angelscript.h>

using namespace AngelScript;

// We need to manually define a wrap function for getScriptVariable
// since aswrappedcall.h has no support for functions with parameters
// of type ?&out.
static void getScriptVariable(asIScriptGeneric* gen)
{
    RoR::GameScript* obj = reinterpret_cast<RoR::GameScript*>(gen->GetObject());
    
    // We'll use the proxy from aswrappedcall.h for strings.
    const std::string& arg0 = static_cast<gw::Proxy<const std::string&>*>(gen->GetAddressOfArg(0))->value;
    void** arg1ref = reinterpret_cast<void**>(gen->GetArgAddress(1));
    int arg1id = gen->GetArgTypeId(1);
    int arg2 = gen->GetArgDWord(2);
    int ret = obj->getScriptVariable(arg0, arg1ref, arg1id, arg2);
    gen->SetReturnDWord(ret);
}

void RoR::RegisterGameScriptGeneric(asIScriptEngine *engine)
{
    int result;

    // class GameScript
    result = engine->RegisterObjectType("GameScriptClass", sizeof(GameScript), asOBJ_REF | asOBJ_NOCOUNT); ROR_ASSERT(result >= 0);
    
    // PLEASE maintain the same order as in GameScript.h!

    // > General
    result = engine->RegisterObjectMethod("GameScriptClass", "void log(const string &in)", WRAP_MFN(GameScript,log), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getTime()", WRAP_MFN(GameScript,getTime), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void backToMenu()", WRAP_MFN(GameScript, backToMenu), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void quitGame()", WRAP_MFN(GameScript, quitGame), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getFPS()", WRAP_MFN(GameScript, getFPS), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getAvgFPS()", WRAP_MFN(GameScript, getAvgFPS), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float rangeRandom(float, float)", WRAP_MFN(GameScript,rangeRandom), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int useOnlineAPI(const string &in, const dictionary &in, string &out)", WRAP_MFN(GameScript, useOnlineAPI), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void openUrlInDefaultBrowser(const string &in)", WRAP_MFN(GameScript, openUrlInDefaultBrowser), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool pushMessage(MsgType, dictionary@)", WRAP_MFN(GameScript, pushMessage), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool checkResourceExists(const string &in, const string &in)", WRAP_MFN(GameScript, checkResourceExists), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool deleteResource(const string &in, const string &in)", WRAP_MFN(GameScript, deleteResource), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string loadTextResourceAsString(const string &in, const string &in)", WRAP_MFN(GameScript, loadTextResourceAsString), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool createTextResourceFromString(const string &in, const string &in, const string &in, bool=false)", WRAP_MFN(GameScript, createTextResourceFromString), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "Ogre::SceneManager@ getSceneManager()", WRAP_MFN(GameScript, getSceneManager), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<dictionary>@ findResourceFileInfo(const string &in, const string &in, bool=false)", WRAP_MFN(GameScript, findResourceFileInfo), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void fetchUrlAsStringAsync(const string &in, const string &in)", WRAP_MFN(GameScript, fetchUrlAsStringAsync), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "Ogre::Image loadImageResource(const string &in, const string &in)", WRAP_MFN(GameScript, loadImageResource), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool serializeMeshResource(const string &in, const string &in, const Ogre::MeshPtr &in)", WRAP_MFN(GameScript, serializeMeshResource), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > GUI
    result = engine->RegisterObjectMethod("GameScriptClass", "void flashMessage(const string &in, float, float)", WRAP_MFN(GameScript, flashMessage), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void message(const string &in, const string &in, float, bool)", WRAP_MFN(GameScript, message), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getChatFontSize()", WRAP_MFN(GameScript, getChatFontSize), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setChatFontSize(int)", WRAP_MFN(GameScript, setChatFontSize), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void showMessageBox(string &in, string &in, bool button1, string &in, bool AllowClose, bool button2,string &in)", WRAP_MFN(GameScript, showMessageBox), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void showChooser(const string &in, const string &in, const string &in)", WRAP_MFN(GameScript, showChooser), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void updateDirectionArrow(const string &in, vector3 &in)", WRAP_MFN(GameScript, updateDirectionArrow), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void hideDirectionArrow()", WRAP_MFN(GameScript, hideDirectionArrow), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool getScreenPosFromWorldPos(const vector3&in, vector2&out)", WRAP_MFN(GameScript, getScreenPosFromWorldPos), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector2 getDisplaySize()", WRAP_MFN(GameScript, getDisplaySize), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector2 getMouseScreenPosition()", WRAP_MFN(GameScript, getMouseScreenPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > Script management
    result = engine->RegisterObjectMethod("GameScriptClass", "void registerForEvent(int)", WRAP_MFN(GameScript, registerForEvent), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void unRegisterEvent(int)", WRAP_MFN(GameScript, unRegisterEvent), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getRegisteredEventsMask(int)", WRAP_MFN(GameScript, getRegisteredEventsMask), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setRegisteredEventsMask(int, int)", WRAP_MFN(GameScript, setRegisteredEventsMask), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode addScriptFunction(const string &in, int = -2)", WRAP_MFN(GameScript, addScriptFunction), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode scriptFunctionExists(const string &in, int = -2)", WRAP_MFN(GameScript, scriptFunctionExists), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode deleteScriptFunction(const string &in, int = -2)", WRAP_MFN(GameScript, deleteScriptFunction), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode addScriptVariable(const string &in, int = -2)", WRAP_MFN(GameScript, addScriptVariable), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode scriptVariableExists(const string &in, int = -2)", WRAP_MFN(GameScript, scriptVariableExists), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode deleteScriptVariable(const string &in, int = -2)", WRAP_MFN(GameScript, deleteScriptVariable), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode getScriptVariable(const string &in, ?&out, int = -2)", asFUNCTION(getScriptVariable), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void clearEventCache()", WRAP_MFN(GameScript, clearEventCache), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int sendGameCmd(const string &in)", WRAP_MFN(GameScript, sendGameCmd), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<int>@ getRunningScripts()", WRAP_MFN(GameScript, getRunningScripts), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "dictionary@ getScriptDetails(int)", WRAP_MFN(GameScript, getScriptDetails), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > Terrain
    result = engine->RegisterObjectMethod("GameScriptClass", "void loadTerrain(const string &in)", WRAP_MFN(GameScript, loadTerrain), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getLoadedTerrain(string &out)", WRAP_MFN(GameScript, getLoadedTerrain), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool getCaelumAvailable()", WRAP_MFN(GameScript, getCaelumAvailable), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getCaelumTime()", WRAP_MFN(GameScript, getCaelumTime), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCaelumTime(float)", WRAP_MFN(GameScript, setCaelumTime), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getGravity()", WRAP_MFN(GameScript, getGravity), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setGravity(float)", WRAP_MFN(GameScript, setGravity), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getGroundHeight(vector3 &in)", WRAP_MFN(GameScript, getGroundHeight), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getWaterHeight()", WRAP_MFN(GameScript, getWaterHeight), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setWaterHeight(float)", WRAP_MFN(GameScript, setWaterHeight), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void spawnObject(const string &in, const string &in, vector3 &in, vector3 &in, const string &in, bool)", WRAP_MFN(GameScript, spawnObject), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void moveObjectVisuals(const string &in, vector3 &in)", WRAP_MFN(GameScript, moveObjectVisuals), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void destroyObject(const string &in)", WRAP_MFN(GameScript, destroyObject), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<TerrainEditorObjectClassPtr@>@ getEditorObjects()", WRAP_MFN(GameScript, getEditorObjects), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool getMousePositionOnTerrain(vector3 &out)", WRAP_MFN(GameScript, getMousePositionOnTerrain), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "TerrainClassPtr@ getTerrain()", WRAP_MFN(GameScript,getTerrain), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<Ogre::MovableObject@>@ getMousePointedMovableObjects()", WRAP_MFN(GameScript, getMousePointedMovableObjects), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > Character
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getPersonPosition()", WRAP_MFN(GameScript, getPersonPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonPosition(vector3 &in)", WRAP_MFN(GameScript, setPersonPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void movePerson(vector3 &in)", WRAP_MFN(GameScript, movePerson), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonRotation(radian &in)", WRAP_MFN(GameScript, setPersonRotation), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "radian getPersonRotation()", WRAP_MFN(GameScript, getPersonRotation), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > Actors
    result = engine->RegisterObjectMethod("GameScriptClass", "void activateAllVehicles()", WRAP_MFN(GameScript,activateAllVehicles), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setTrucksForcedActive(bool forceActive)", WRAP_MFN(GameScript,setTrucksForcedAwake), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void boostCurrentTruck(float)", WRAP_MFN(GameScript, boostCurrentTruck), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClassPtr @getCurrentTruck()", WRAP_MFN(GameScript, getCurrentTruck), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClassPtr @getTruckByNum(int)", WRAP_MFN(GameScript, getTruckByNum), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<BeamClass@> @getAllTrucks()", WRAP_MFN(GameScript, getAllTrucks), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getCurrentTruckNumber()", WRAP_MFN(GameScript, getCurrentTruckNumber), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClassPtr @spawnTruck(string &in, vector3 &in, vector3 &in)", WRAP_MFN(GameScript, spawnTruck), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void repairVehicle(const string &in, const string &in, bool)", WRAP_MFN(GameScript, repairVehicle), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void removeVehicle(const string &in, const string &in)", WRAP_MFN(GameScript, removeVehicle), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucksByFlag(int)", WRAP_MFN(GameScript, getNumTrucksByFlag), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getActorNextInstanceId()", WRAP_MFN(GameScript, getActorNextInstanceId), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClassPtr @getTruckRemotelyReceivingCommands()", WRAP_MFN(GameScript, getTruckRemotelyReceivingCommands), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > FreeForces
    result = engine->RegisterObjectMethod("GameScriptClass", "int getFreeForceNextId()", WRAP_MFN(GameScript, getFreeForceNextId), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getFreeBeamGfxNextId()", WRAP_MFN(GameScript, getFreeBeamGfxNextId), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > Waypoint AI for actors
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClassPtr @spawnTruckAI(string &in, vector3 &in, string &in, string &in, int x)", WRAP_MFN(GameScript, spawnTruckAI), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<vector3> @getWaypoints(int x)", WRAP_MFN(GameScript, getWaypoints), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<int> @getWaypointsSpeed()", WRAP_MFN(GameScript, getWaypointsSpeed), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void addWaypoint(vector3 &in)", WRAP_MFN(GameScript, addWaypoint), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIVehicleCount()", WRAP_MFN(GameScript, getAIVehicleCount), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIVehicleDistance()", WRAP_MFN(GameScript, getAIVehicleDistance), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIVehiclePositionScheme()", WRAP_MFN(GameScript, getAIVehiclePositionScheme), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIVehicleSpeed()", WRAP_MFN(GameScript, getAIVehicleSpeed), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getAIVehicleName(int x)", WRAP_MFN(GameScript, getAIVehicleName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getAIVehicleSectionConfig(int x)", WRAP_MFN(GameScript, getAIVehicleSectionConfig), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getAIVehicleSkin(int x)", WRAP_MFN(GameScript, getAIVehicleSkin), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIRepeatTimes()", WRAP_MFN(GameScript, getAIRepeatTimes), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIMode()", WRAP_MFN(GameScript, getAIMode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "VehicleAIClassPtr @getCurrentTruckAI()", WRAP_MFN(GameScript, getCurrentTruckAI), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "VehicleAIClassPtr @getTruckAIByNum(int)", WRAP_MFN(GameScript, getTruckAIByNum), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleCount(int count)", WRAP_MFN(GameScript, setAIVehicleCount), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleDistance(int dist)", WRAP_MFN(GameScript, setAIVehicleDistance), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehiclePositionScheme(int scheme)", WRAP_MFN(GameScript, setAIVehiclePositionScheme), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleSpeed(int speed)", WRAP_MFN(GameScript, setAIVehicleSpeed), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleName(int x, string name)", WRAP_MFN(GameScript, setAIVehicleName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleSectionConfig(int x, string config)", WRAP_MFN(GameScript, setAIVehicleSectionConfig), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleSkin(int x, string skin)", WRAP_MFN(GameScript, setAIVehicleSkin), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIRepeatTimes(int times)", WRAP_MFN(GameScript, setAIRepeatTimes), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIMode(int mode)", WRAP_MFN(GameScript, setAIMode), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > Camera
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraPosition(vector3 &in)", WRAP_MFN(GameScript, setCameraPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraDirection(vector3 &in)", WRAP_MFN(GameScript, setCameraDirection), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraOrientation(quaternion &in)", WRAP_MFN(GameScript, setCameraOrientation), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraRoll(float)", WRAP_MFN(GameScript, setCameraRoll), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraYaw(float)", WRAP_MFN(GameScript, setCameraYaw), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraPitch(float)", WRAP_MFN(GameScript, setCameraPitch), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraPosition()", WRAP_MFN(GameScript, getCameraPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraDirection()", WRAP_MFN(GameScript, getCameraDirection), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "quaternion getCameraOrientation()", WRAP_MFN(GameScript, getCameraOrientation), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void cameraLookAt(vector3 &in)", WRAP_MFN(GameScript, cameraLookAt), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > Race system
    result = engine->RegisterObjectMethod("GameScriptClass", "void setBestLapTime(float time)", WRAP_MFN(GameScript,setBestLapTime), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setTimeDiff(float diff)", WRAP_MFN(GameScript,setTimeDiff), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void startTimer(int id)", WRAP_MFN(GameScript,startTimer), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void stopTimer()", WRAP_MFN(GameScript,stopTimer), asCALL_GENERIC); ROR_ASSERT(result>=0);
    
    // > Material helpers
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialAmbient(const string &in, float, float, float)", WRAP_MFN(GameScript,setMaterialAmbient), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialDiffuse(const string &in, float, float, float, float)", WRAP_MFN(GameScript,setMaterialDiffuse), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialSpecular(const string &in, float, float, float, float)", WRAP_MFN(GameScript,setMaterialSpecular), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialEmissive(const string &in, float, float, float)", WRAP_MFN(GameScript,setMaterialEmissive), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureName(const string &in, int, int, int, const string &in)", WRAP_MFN(GameScript,setMaterialTextureName), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureRotate(const string &in, int, int, int, float)", WRAP_MFN(GameScript,setMaterialTextureRotate), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureScroll(const string &in, int, int, int, float, float)", WRAP_MFN(GameScript,setMaterialTextureScroll), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureScale(const string &in, int, int, int, float, float)", WRAP_MFN(GameScript,setMaterialTextureScale), asCALL_GENERIC); ROR_ASSERT(result>=0);

    // > Audio
    result = engine->RegisterObjectMethod("GameScriptClass", "array<SoundScriptTemplateClass@>@ getAllSoundScriptTemplates()", WRAP_MFN(GameScript, getAllSoundScriptTemplates), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "SoundScriptTemplateClassPtr@ getSoundScriptTemplate(const string &in)", WRAP_MFN(GameScript, getSoundScriptTemplate), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<SoundScriptInstanceClass@>@ getAllSoundScriptInstances()", WRAP_MFN(GameScript, getAllSoundScriptInstances), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "SoundClassPtr@ createSoundFromResource(const string &in filename, const string &in rg_name = string())", WRAP_MFN(GameScript, createSoundFromResource), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "SoundScriptInstanceClassPtr@ createSoundScriptInstance(const string &in template, int actor_instance_id = -1)", WRAP_MFN(GameScript, createSoundScriptInstance), asCALL_GENERIC); ROR_ASSERT(result >= 0);

}
