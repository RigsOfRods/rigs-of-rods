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

#include "AngelScriptBindings.h"
#include "GameScript.h"
#include "ScriptEngine.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterGameScript(asIScriptEngine *engine)
{
    int result;

    // enum ScriptCategory ~ for `game.pushMessage(MSG_APP_LOAD_SCRIPT_REQUESTED ...)`
    result = engine->RegisterEnum("ScriptCategory"); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("ScriptCategory", "SCRIPT_CATEGORY_INVALID", static_cast<int>(ScriptCategory::INVALID)); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptCategory", "SCRIPT_CATEGORY_ACTOR", static_cast<int>(ScriptCategory::ACTOR)); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptCategory", "SCRIPT_CATEGORY_TERRAIN", static_cast<int>(ScriptCategory::TERRAIN)); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptCategory", "SCRIPT_CATEGORY_CUSTOM", static_cast<int>(ScriptCategory::CUSTOM)); ROR_ASSERT(result >= 0);

    // `enum ScriptRetCode` ~ Common return codes for script manipulation funcs (add/get/delete | funcs/variables)
    result = engine->RegisterEnum("ScriptRetCode"); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_ERROR", SCRIPTRETCODE_AS_ERROR ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CONTEXT_ACTIVE", SCRIPTRETCODE_AS_CONTEXT_ACTIVE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CONTEXT_NOT_FINISHED", SCRIPTRETCODE_AS_CONTEXT_NOT_FINISHED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CONTEXT_NOT_PREPARED", SCRIPTRETCODE_AS_CONTEXT_NOT_PREPARED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_ARG", SCRIPTRETCODE_AS_INVALID_ARG ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NO_FUNCTION", SCRIPTRETCODE_AS_NO_FUNCTION ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NOT_SUPPORTED", SCRIPTRETCODE_AS_NOT_SUPPORTED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_NAME", SCRIPTRETCODE_AS_INVALID_NAME ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NAME_TAKEN", SCRIPTRETCODE_AS_NAME_TAKEN ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_DECLARATION", SCRIPTRETCODE_AS_INVALID_DECLARATION ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_OBJECT", SCRIPTRETCODE_AS_INVALID_OBJECT ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_TYPE", SCRIPTRETCODE_AS_INVALID_TYPE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_ALREADY_REGISTERED", SCRIPTRETCODE_AS_ALREADY_REGISTERED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_MULTIPLE_FUNCTIONS", SCRIPTRETCODE_AS_MULTIPLE_FUNCTIONS ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NO_MODULE", SCRIPTRETCODE_AS_NO_MODULE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NO_GLOBAL_VAR", SCRIPTRETCODE_AS_NO_GLOBAL_VAR ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_CONFIGURATION", SCRIPTRETCODE_AS_INVALID_CONFIGURATION ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_INTERFACE", SCRIPTRETCODE_AS_INVALID_INTERFACE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CANT_BIND_ALL_FUNCTIONS", SCRIPTRETCODE_AS_CANT_BIND_ALL_FUNCTIONS ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_LOWER_ARRAY_DIMENSION_NOT_REGISTERED", SCRIPTRETCODE_AS_LOWER_ARRAY_DIMENSION_NOT_REGISTERED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_WRONG_CONFIG_GROUP", SCRIPTRETCODE_AS_WRONG_CONFIG_GROUP ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CONFIG_GROUP_IS_IN_USE", SCRIPTRETCODE_AS_CONFIG_GROUP_IS_IN_USE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_ILLEGAL_BEHAVIOUR_FOR_TYPE", SCRIPTRETCODE_AS_ILLEGAL_BEHAVIOUR_FOR_TYPE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_WRONG_CALLING_CONV", SCRIPTRETCODE_AS_WRONG_CALLING_CONV ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_BUILD_IN_PROGRESS", SCRIPTRETCODE_AS_BUILD_IN_PROGRESS ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INIT_GLOBAL_VARS_FAILED", SCRIPTRETCODE_AS_INIT_GLOBAL_VARS_FAILED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_OUT_OF_MEMORY", SCRIPTRETCODE_AS_OUT_OF_MEMORY ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_MODULE_IS_IN_USE", SCRIPTRETCODE_AS_MODULE_IS_IN_USE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_UNSPECIFIED_ERROR", SCRIPTRETCODE_UNSPECIFIED_ERROR); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_ENGINE_NOT_CREATED", SCRIPTRETCODE_ENGINE_NOT_CREATED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_CONTEXT_NOT_CREATED", SCRIPTRETCODE_CONTEXT_NOT_CREATED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_SCRIPTUNIT_NOT_EXISTS", SCRIPTRETCODE_SCRIPTUNIT_NOT_EXISTS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_SCRIPTUNIT_NO_MODULE", SCRIPTRETCODE_SCRIPTUNIT_NO_MODULE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_FUNCTION_NOT_EXISTS", SCRIPTRETCODE_FUNCTION_NOT_EXISTS); ROR_ASSERT(result >= 0);

    // class GameScript
    result = engine->RegisterObjectType("GameScriptClass", sizeof(GameScript), asOBJ_REF | asOBJ_NOCOUNT); ROR_ASSERT(result >= 0);
    
    // PLEASE maintain the same order as in GameScript.h!

    // > General
    result = engine->RegisterObjectMethod("GameScriptClass", "void log(const string &in)", asMETHOD(GameScript,log), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getTime()", asMETHOD(GameScript,getTime), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void backToMenu()", asMETHOD(GameScript, backToMenu), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void quitGame()", asMETHOD(GameScript, quitGame), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getFPS()", asMETHOD(GameScript, getFPS), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getAvgFPS()", asMETHOD(GameScript, getAvgFPS), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float rangeRandom(float, float)", asMETHOD(GameScript,rangeRandom), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int useOnlineAPI(const string &in, const dictionary &in, string &out)", asMETHOD(GameScript, useOnlineAPI), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void openUrlInDefaultBrowser(const string &in)", asMETHOD(GameScript, openUrlInDefaultBrowser), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool pushMessage(MsgType, dictionary@)", asMETHOD(GameScript, pushMessage), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool checkResourceExists(const string &in, const string &in)", asMETHOD(GameScript, checkResourceExists), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool deleteResource(const string &in, const string &in)", asMETHOD(GameScript, deleteResource), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string loadTextResourceAsString(const string &in, const string &in)", asMETHOD(GameScript, loadTextResourceAsString), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool createTextResourceFromString(const string &in, const string &in, const string &in, bool=false)", asMETHOD(GameScript, createTextResourceFromString), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "Ogre::SceneManager@ getSceneManager()", asMETHOD(GameScript, getSceneManager), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<dictionary>@ findResourceFileInfo(const string &in, const string &in, bool=false)", asMETHOD(GameScript, findResourceFileInfo), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void fetchUrlAsStringAsync(const string &in, const string &in)", asMETHOD(GameScript, fetchUrlAsStringAsync), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "Ogre::Image loadImageResource(const string &in, const string &in)", asMETHOD(GameScript, loadImageResource), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool serializeMeshResource(const string &in, const string &in, const Ogre::MeshPtr &in)", asMETHOD(GameScript, serializeMeshResource), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // > GUI
    result = engine->RegisterObjectMethod("GameScriptClass", "void flashMessage(const string &in, float, float)", asMETHOD(GameScript, flashMessage), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void message(const string &in, const string &in, float, bool)", asMETHOD(GameScript, message), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getChatFontSize()", asMETHOD(GameScript, getChatFontSize), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setChatFontSize(int)", asMETHOD(GameScript, setChatFontSize), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void showMessageBox(string &in, string &in, bool button1, string &in, bool AllowClose, bool button2,string &in)", asMETHOD(GameScript, showMessageBox), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void showChooser(const string &in, const string &in, const string &in)", asMETHOD(GameScript, showChooser), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void updateDirectionArrow(const string &in, vector3 &in)", asMETHOD(GameScript, updateDirectionArrow), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void hideDirectionArrow()", asMETHOD(GameScript, hideDirectionArrow), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool getScreenPosFromWorldPos(const vector3&in, vector2&out)", AngelScript::asMETHOD(GameScript, getScreenPosFromWorldPos), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector2 getDisplaySize()", AngelScript::asMETHOD(GameScript, getDisplaySize), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector2 getMouseScreenPosition()", AngelScript::asMETHOD(GameScript, getMouseScreenPosition), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // > Script management
    result = engine->RegisterObjectMethod("GameScriptClass", "void registerForEvent(int)", asMETHOD(GameScript, registerForEvent), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void unRegisterEvent(int)", asMETHOD(GameScript, unRegisterEvent), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getRegisteredEventsMask(int)", asMETHOD(GameScript, getRegisteredEventsMask), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setRegisteredEventsMask(int, int)", asMETHOD(GameScript, setRegisteredEventsMask), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode addScriptFunction(const string &in, int = -2)", asMETHOD(GameScript, addScriptFunction), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode scriptFunctionExists(const string &in, int = -2)", asMETHOD(GameScript, scriptFunctionExists), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode deleteScriptFunction(const string &in, int = -2)", asMETHOD(GameScript, deleteScriptFunction), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode addScriptVariable(const string &in, int = -2)", asMETHOD(GameScript, addScriptVariable), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode scriptVariableExists(const string &in, int = -2)", asMETHOD(GameScript, scriptVariableExists), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode deleteScriptVariable(const string &in, int = -2)", asMETHOD(GameScript, deleteScriptVariable), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "ScriptRetCode getScriptVariable(const string &in, ?&out, int = -2)", asMETHOD(GameScript, getScriptVariable), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void clearEventCache()", asMETHOD(GameScript, clearEventCache), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int sendGameCmd(const string &in)", asMETHOD(GameScript, sendGameCmd), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<int>@ getRunningScripts()", asMETHOD(GameScript, getRunningScripts), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "dictionary@ getScriptDetails(int)", asMETHOD(GameScript, getScriptDetails), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // > Terrain
    result = engine->RegisterObjectMethod("GameScriptClass", "void loadTerrain(const string &in)", asMETHOD(GameScript, loadTerrain), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getLoadedTerrain(string &out)", asMETHOD(GameScript, getLoadedTerrain), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool getCaelumAvailable()", asMETHOD(GameScript, getCaelumAvailable), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getCaelumTime()", asMETHOD(GameScript, getCaelumTime), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCaelumTime(float)", asMETHOD(GameScript, setCaelumTime), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getGravity()", asMETHOD(GameScript, getGravity), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setGravity(float)", asMETHOD(GameScript, setGravity), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getGroundHeight(vector3 &in)", asMETHOD(GameScript, getGroundHeight), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getWaterHeight()", asMETHOD(GameScript, getWaterHeight), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setWaterHeight(float)", asMETHOD(GameScript, setWaterHeight), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void spawnObject(const string &in, const string &in, vector3 &in, vector3 &in, const string &in, bool)", asMETHOD(GameScript, spawnObject), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void moveObjectVisuals(const string &in, vector3 &in)", asMETHOD(GameScript, moveObjectVisuals), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void destroyObject(const string &in)", asMETHOD(GameScript, destroyObject), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<TerrainEditorObjectClassPtr@>@ getEditorObjects()", asMETHOD(GameScript, getEditorObjects), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool getMousePositionOnTerrain(vector3 &out)", AngelScript::asMETHOD(GameScript, getMousePositionOnTerrain), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "TerrainClassPtr@ getTerrain()", AngelScript::asMETHOD(GameScript,getTerrain), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<Ogre::MovableObject@>@ getMousePointedMovableObjects()", AngelScript::asMETHOD(GameScript, getMousePointedMovableObjects), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // > Character
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getPersonPosition()", asMETHOD(GameScript, getPersonPosition), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonPosition(vector3 &in)", asMETHOD(GameScript, setPersonPosition), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void movePerson(vector3 &in)", asMETHOD(GameScript, movePerson), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonRotation(radian &in)", asMETHOD(GameScript, setPersonRotation), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "radian getPersonRotation()", asMETHOD(GameScript, getPersonRotation), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // > Actors
    result = engine->RegisterObjectMethod("GameScriptClass", "void activateAllVehicles()", asMETHOD(GameScript,activateAllVehicles), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setTrucksForcedActive(bool forceActive)", asMETHOD(GameScript,setTrucksForcedAwake), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void boostCurrentTruck(float)", asMETHOD(GameScript, boostCurrentTruck), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClassPtr @getCurrentTruck()", asMETHOD(GameScript, getCurrentTruck), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClassPtr @getTruckByNum(int)", asMETHOD(GameScript, getTruckByNum), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<BeamClass@> @getAllTrucks()", AngelScript::asMETHOD(GameScript, getAllTrucks), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getCurrentTruckNumber()", asMETHOD(GameScript, getCurrentTruckNumber), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClassPtr @spawnTruck(string &in, vector3 &in, vector3 &in)", asMETHOD(GameScript, spawnTruck), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void repairVehicle(const string &in, const string &in, bool)", asMETHOD(GameScript, repairVehicle), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void removeVehicle(const string &in, const string &in)", asMETHOD(GameScript, removeVehicle), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucksByFlag(int)", asMETHOD(GameScript, getNumTrucksByFlag), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getActorNextInstanceId()", asMETHOD(GameScript, getActorNextInstanceId), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // > FreeForces
    result = engine->RegisterObjectMethod("GameScriptClass", "int getFreeForceNextId()", asMETHOD(GameScript, getFreeForceNextId), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // > Waypoint AI for actors
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClassPtr @spawnTruckAI(string &in, vector3 &in, string &in, string &in, int x)", AngelScript::asMETHOD(GameScript, spawnTruckAI), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<vector3> @getWaypoints(int x)", AngelScript::asMETHOD(GameScript, getWaypoints), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<int> @getWaypointsSpeed()", AngelScript::asMETHOD(GameScript, getWaypointsSpeed), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void addWaypoint(vector3 &in)", asMETHOD(GameScript, addWaypoint), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIVehicleCount()", AngelScript::asMETHOD(GameScript, getAIVehicleCount), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIVehicleDistance()", AngelScript::asMETHOD(GameScript, getAIVehicleDistance), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIVehiclePositionScheme()", AngelScript::asMETHOD(GameScript, getAIVehiclePositionScheme), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIVehicleSpeed()", AngelScript::asMETHOD(GameScript, getAIVehicleSpeed), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getAIVehicleName(int x)", AngelScript::asMETHOD(GameScript, getAIVehicleName), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getAIVehicleSectionConfig(int x)", AngelScript::asMETHOD(GameScript, getAIVehicleSectionConfig), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getAIVehicleSkin(int x)", AngelScript::asMETHOD(GameScript, getAIVehicleSkin), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIRepeatTimes()", AngelScript::asMETHOD(GameScript, getAIRepeatTimes), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getAIMode()", AngelScript::asMETHOD(GameScript, getAIMode), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "VehicleAIClassPtr @getCurrentTruckAI()", asMETHOD(GameScript, getCurrentTruckAI), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "VehicleAIClassPtr @getTruckAIByNum(int)", asMETHOD(GameScript, getTruckAIByNum), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleCount(int count)", AngelScript::asMETHOD(GameScript, setAIVehicleCount), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleDistance(int dist)", AngelScript::asMETHOD(GameScript, setAIVehicleDistance), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehiclePositionScheme(int scheme)", AngelScript::asMETHOD(GameScript, setAIVehiclePositionScheme), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleSpeed(int speed)", AngelScript::asMETHOD(GameScript, setAIVehicleSpeed), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleName(int x, string name)", AngelScript::asMETHOD(GameScript, setAIVehicleName), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleSectionConfig(int x, string config)", AngelScript::asMETHOD(GameScript, setAIVehicleSectionConfig), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIVehicleSkin(int x, string skin)", AngelScript::asMETHOD(GameScript, setAIVehicleSkin), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIRepeatTimes(int times)", AngelScript::asMETHOD(GameScript, setAIRepeatTimes), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setAIMode(int mode)", AngelScript::asMETHOD(GameScript, setAIMode), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // > Camera
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraPosition(vector3 &in)", asMETHOD(GameScript, setCameraPosition), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraDirection(vector3 &in)", asMETHOD(GameScript, setCameraDirection), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraOrientation(quaternion &in)", asMETHOD(GameScript, setCameraOrientation), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraRoll(float)", asMETHOD(GameScript, setCameraRoll), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraYaw(float)", asMETHOD(GameScript, setCameraYaw), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraPitch(float)", asMETHOD(GameScript, setCameraPitch), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraPosition()", asMETHOD(GameScript, getCameraPosition), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraDirection()", asMETHOD(GameScript, getCameraDirection), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "quaternion getCameraOrientation()", asMETHOD(GameScript, getCameraOrientation), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void cameraLookAt(vector3 &in)", asMETHOD(GameScript, cameraLookAt), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // > Race system
    result = engine->RegisterObjectMethod("GameScriptClass", "void setBestLapTime(float time)", asMETHOD(GameScript,setBestLapTime), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setTimeDiff(float diff)", asMETHOD(GameScript,setTimeDiff), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void startTimer(int id)", asMETHOD(GameScript,startTimer), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void stopTimer()", asMETHOD(GameScript,stopTimer), asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    // > Material helpers
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialAmbient(const string &in, float, float, float)", asMETHOD(GameScript,setMaterialAmbient), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialDiffuse(const string &in, float, float, float, float)", asMETHOD(GameScript,setMaterialDiffuse), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialSpecular(const string &in, float, float, float, float)", asMETHOD(GameScript,setMaterialSpecular), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialEmissive(const string &in, float, float, float)", asMETHOD(GameScript,setMaterialEmissive), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureName(const string &in, int, int, int, const string &in)", asMETHOD(GameScript,setMaterialTextureName), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureRotate(const string &in, int, int, int, float)", asMETHOD(GameScript,setMaterialTextureRotate), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureScroll(const string &in, int, int, int, float, float)", asMETHOD(GameScript,setMaterialTextureScroll), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureScale(const string &in, int, int, int, float, float)", asMETHOD(GameScript,setMaterialTextureScale), asCALL_THISCALL); ROR_ASSERT(result>=0);

    // > Audio
    result = engine->RegisterObjectMethod("GameScriptClass", "array<SoundScriptTemplateClass@>@ getAllSoundScriptTemplates()", asMETHOD(GameScript, getAllSoundScriptTemplates), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "SoundScriptTemplateClassPtr@ getSoundScriptTemplate(const string &in)", asMETHOD(GameScript, getSoundScriptTemplate), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "array<SoundScriptInstanceClass@>@ getAllSoundScriptInstances()", asMETHOD(GameScript, getAllSoundScriptInstances), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "SoundClassPtr@ createSoundFromResource(const string &in filename, const string &in rg_name = string())", asMETHOD(GameScript, createSoundFromResource), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "SoundScriptInstanceClassPtr@ createSoundScriptInstance(const string &in template, int actor_instance_id = -1)", asMETHOD(GameScript, createSoundScriptInstance), asCALL_THISCALL); ROR_ASSERT(result >= 0);

}
