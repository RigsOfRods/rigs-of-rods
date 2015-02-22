/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#include "GameScript.h"

// AS addons start
#include "contextmgr/contextmgr.h"
#include "scriptany/scriptany.h"
#include "scriptarray/scriptarray.h"
#include "scripthelper/scripthelper.h"
#include "scriptmath/scriptmath.h"
#include "scriptstdstring/scriptstdstring.h"
#include "scriptstring/scriptstring.h"
// AS addons end

#ifdef USE_CURL
#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>
#endif //USE_CURL

#include "Application.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "Character.h"
#include "Console.h"
#include "IHeightFinder.h"
#include "Language.h"
#include "MainThread.h"
#include "Network.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"
#include "Settings.h"
#include "CaelumManager.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"
#include "Water.h"
#include "GUIManager.h"

using namespace Ogre;
using namespace RoR;

/* class that implements the interface for the scripts */
GameScript::GameScript(ScriptEngine *se) :
	  mse(se)
	, apiThread()
{
}

GameScript::~GameScript()
{
}

void GameScript::log(const String &msg)
{
	SLOG(msg);
}

double GameScript::getTime()
{
	double result = 0.0l;
	if (gEnv->frameListener) result = gEnv->frameListener->getTime();
	return result;
}

void GameScript::setPersonPosition(const Vector3 &vec)
{
	if (gEnv->player) gEnv->player->setPosition(vec);
}

void GameScript::loadTerrain(const String &terrain)
{
	if (gEnv->frameListener) RoR::Application::GetMainThreadLogic()->LoadTerrain(terrain);
}

Vector3 GameScript::getPersonPosition()
{
	Vector3 result(Vector3::ZERO);
	if (gEnv->player) result = gEnv->player->getPosition();
	return result;
}

void GameScript::movePerson(const Vector3 &vec)
{
	if (gEnv->player) gEnv->player->move(vec);
}

void GameScript::setPersonRotation(const Radian &rot)
{
	if (gEnv->player) gEnv->player->setRotation(rot);
}

Radian GameScript::getPersonRotation()
{
	Radian result(0);
	if (gEnv->player) result = gEnv->player->getRotation();
	return result;
}

String GameScript::getCaelumTime()
{
	String result = "";
#ifdef USE_CAELUM
	if (gEnv->terrainManager) result = gEnv->terrainManager->getCaelumManager()->getPrettyTime();
#endif // USE_CAELUM
	return result;
}

void GameScript::setCaelumTime(float value)
{
#ifdef USE_CAELUM
	if (gEnv->terrainManager) gEnv->terrainManager->getCaelumManager()->setTimeFactor(value);
#endif // USE_CAELUM
}

bool GameScript::getCaelumAvailable()
{
	bool result = false;
#ifdef USE_CAELUM
	if (gEnv->terrainManager) result = gEnv->terrainManager->getCaelumManager() != 0;
#endif // USE_CAELUM
	return result;
}

float GameScript::stopTimer()
{
	float result = 0.0f;
	if (gEnv->frameListener != nullptr)
	{
		result = gEnv->frameListener->StopRaceTimer();
	}
	return result;
}

void GameScript::startTimer()
{
	if (gEnv->frameListener != nullptr)
	{
		gEnv->frameListener->StartRaceTimer();
	}
}

void GameScript::setWaterHeight(float value)
{
	if (gEnv->terrainManager && gEnv->terrainManager->getWater())
	{
		IWater* water = gEnv->terrainManager->getWater();
		water->setCamera(gEnv->mainCamera);
		water->setHeight(value);
		water->update();
	}
}

float GameScript::getGroundHeight(Vector3 &v)
{
	float result = -1.0f;
	if (gEnv->terrainManager && gEnv->terrainManager->getHeightFinder()) result = gEnv->terrainManager->getHeightFinder()->getHeightAt(v.x, v.z);
	return result;
}

float GameScript::getWaterHeight()
{
	float result = 0.0f;
	if (gEnv->terrainManager && gEnv->terrainManager->getWater()) result = gEnv->terrainManager->getWater()->getHeight();
	return result;
}

Beam *GameScript::getCurrentTruck()
{
	return BeamFactory::getSingleton().getCurrentTruck();
}

float GameScript::getGravity()
{
	float result = 0.0f;
	if (gEnv->frameListener) result = gEnv->terrainManager->getGravity();
	return result;
}

void GameScript::setGravity(float value)
{
	if (gEnv->terrainManager) gEnv->terrainManager->setGravity(value);
}

Beam *GameScript::getTruckByNum(int num)
{
	return BeamFactory::getSingleton().getTruck(num);
}

int GameScript::getNumTrucks()
{
	return BeamFactory::getSingleton().getTruckCount();
}

int GameScript::getNumTrucksByFlag(int flag)
{
	int result = 0;
	for (int i=0; i < BeamFactory::getSingleton().getTruckCount(); i++)
	{
		Beam *truck = BeamFactory::getSingleton().getTruck(i);
		if (!truck && !flag) result++;
		if (!truck) continue;
		if (truck->state == flag) result++;
	}
	return result;
}

int GameScript::getCurrentTruckNumber()
{
	return BeamFactory::getSingleton().getCurrentTruckNumber();
}

void GameScript::registerForEvent(int eventValue)
{
	if (mse) mse->eventMask += eventValue;
}

void GameScript::flashMessage(String &txt, float time, float charHeight)
{
#ifdef USE_MYGUI
	RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, "script_code_red.png");
	RoR::Application::GetGuiManager()->PushNotification("Script:", txt);
#endif // USE_MYGUI
}

void GameScript::message(String &txt, String &icon, float timeMilliseconds, bool forceVisible)
{
	//TODO: Notification system
#ifdef USE_MYGUI
	RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, icon, timeMilliseconds, forceVisible);
	RoR::Application::GetGuiManager()->PushNotification("Script:", txt);
#endif // USE_MYGUI
}

void GameScript::setDirectionArrow(String &text, Vector3 &vec)
{
	if (gEnv->frameListener) gEnv->frameListener->setDirectionArrow(const_cast<char*>(text.c_str()), Vector3(vec.x, vec.y, vec.z));
}

int GameScript::getChatFontSize()
{
	return 0; //NETCHAT.getFontSize();
}

void GameScript::setChatFontSize(int size)
{
	//NETCHAT.setFontSize(size);
}

void GameScript::showChooser(const String &type, const String &instance, const String &box)
{
#ifdef USE_MYGUI
	LoaderType ntype = LT_None;
	
	if (type == "airplane")    ntype = LT_Airplane;
	if (type == "all")         ntype = LT_AllBeam;
	if (type == "boat")        ntype = LT_Boat;
	if (type == "car")         ntype = LT_Car;
	if (type == "extension")   ntype = LT_Extension;
	if (type == "heli")        ntype = LT_Heli;
	if (type == "load")        ntype = LT_Load;
	if (type == "trailer")     ntype = LT_Trailer;
	if (type == "train")       ntype = LT_Train;
	if (type == "truck")       ntype = LT_Truck;
	if (type == "vehicle")     ntype = LT_Vehicle;
	
	if (ntype != LT_None)
	{
		if (gEnv->frameListener) gEnv->frameListener->showLoad(ntype, instance, box);
	}
#endif //USE_MYGUI
}

void GameScript::repairVehicle(const String &instance, const String &box, bool keepPosition)
{
	BeamFactory::getSingleton().repairTruck(gEnv->collisions, instance, box, keepPosition);
}

void GameScript::removeVehicle(const String &instance, const String &box)
{
	BeamFactory::getSingleton().removeTruck(gEnv->collisions, instance, box);
}

void GameScript::destroyObject(const String &instanceName)
{
	if (gEnv->terrainManager && gEnv->terrainManager->getObjectManager())
	{
		gEnv->terrainManager->getObjectManager()->unloadObject(instanceName);
	}
}

void GameScript::spawnObject(const String &objectName, const String &instanceName, const Vector3 &pos, const Vector3 &rot, const String &eventhandler, bool uniquifyMaterials)
{
	AngelScript::asIScriptModule *mod = 0;
	try
	{
		mod = mse->getEngine()->GetModule(mse->moduleName, AngelScript::asGM_ONLY_IF_EXISTS);
	}catch(std::exception e)
	{
		SLOG("Exception in spawnObject(): " + String(e.what()));
		return;
	}
	if (!mod) return;
	int functionPtr = mod->GetFunctionIdByName(eventhandler.c_str());

	// trying to create the new object
	SceneNode *bakeNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	if (gEnv->terrainManager && gEnv->terrainManager->getObjectManager())
	{
		const String type = "";
		gEnv->terrainManager->getObjectManager()->loadObject(objectName, pos, rot, bakeNode, instanceName, type, true, functionPtr, uniquifyMaterials);
	}
}

void GameScript::hideDirectionArrow()
{
	if (gEnv->frameListener) gEnv->frameListener->setDirectionArrow(0, Vector3::ZERO);
}

int GameScript::setMaterialAmbient(const String &materialName, float red, float green, float blue)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if (m.isNull()) return 0;
		m->setAmbient(red, green, blue);
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialAmbient(): " + e.getFullDescription());
		return 0;
	}
	return 1;
}

int GameScript::setMaterialDiffuse(const String &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if (m.isNull()) return 0;
		m->setDiffuse(red, green, blue, alpha);
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialDiffuse(): " + e.getFullDescription());
		return 0;
	}
	return 1;
}

int GameScript::setMaterialSpecular(const String &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if (m.isNull()) return 0;
		m->setSpecular(red, green, blue, alpha);
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialSpecular(): " + e.getFullDescription());
		return 0;
	}
	return 1;
}

int GameScript::setMaterialEmissive(const String &materialName, float red, float green, float blue)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if (m.isNull()) return 0;
		m->setSelfIllumination(red, green, blue);
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialEmissive(): " + e.getFullDescription());
		return 0;
	}
	return 1;
}

int GameScript::getSafeTextureUnitState(TextureUnitState **tu, const String materialName, int techniqueNum, int passNum, int textureUnitNum)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if (m.isNull()) return 1;

		// verify technique
		if (techniqueNum < 0 || techniqueNum > m->getNumTechniques()) return 2;
		Technique *t = m->getTechnique(techniqueNum);
		if (!t) return 2;

		//verify pass
		if (passNum < 0 || passNum > t->getNumPasses()) return 3;
		Pass *p = t->getPass(passNum);
		if (!p) return 3;

		//verify texture unit
		if (textureUnitNum < 0 || textureUnitNum > p->getNumTextureUnitStates()) return 4;
		TextureUnitState *tut = p->getTextureUnitState(textureUnitNum);
		if (!tut) return 4;

		*tu = tut;
		return 0;
	} catch(Exception e)
	{
		SLOG("Exception in getSafeTextureUnitState(): " + e.getFullDescription());
	}
	return 1;
}

int GameScript::setMaterialTextureName(const String &materialName, int techniqueNum, int passNum, int textureUnitNum, const String &textureName)
{
	TextureUnitState *tu = 0;
	int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
	if (res == 0 && tu != 0)
	{
		// finally, set it
		tu->setTextureName(textureName);
	}
	return res;
}

int GameScript::setMaterialTextureRotate(const String &materialName, int techniqueNum, int passNum, int textureUnitNum, float rotation)
{
	TextureUnitState *tu = 0;
	int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
	if (res == 0 && tu != 0)
	{
		tu->setTextureRotate(Degree(rotation));
	}
	return res;
}

int GameScript::setMaterialTextureScroll(const String &materialName, int techniqueNum, int passNum, int textureUnitNum, float sx, float sy)
{
	TextureUnitState *tu = 0;
	int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
	if (res == 0 && tu != 0)
	{
		tu->setTextureScroll(sx, sy);
	}
	return res;
}

int GameScript::setMaterialTextureScale(const String &materialName, int techniqueNum, int passNum, int textureUnitNum, float u, float v)
{
	TextureUnitState *tu = 0;
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

int GameScript::getLoadedTerrain(String &result)
{
	String terrainName = "";

	if (gEnv->terrainManager)
	{
		terrainName = gEnv->terrainManager->getTerrainName();
		result = terrainName;
	}

	return !terrainName.empty();
}

void GameScript::clearEventCache()
{
	if (gEnv->collisions) gEnv->collisions->clearEventCache();
}

void GameScript::setCameraPosition(const Vector3 &pos)
{
	if (gEnv->mainCamera) gEnv->mainCamera->setPosition(Vector3(pos.x, pos.y, pos.z));
}

void GameScript::setCameraDirection(const Vector3 &rot)
{
	if (gEnv->mainCamera) gEnv->mainCamera->setDirection(Vector3(rot.x, rot.y, rot.z));
}

void GameScript::setCameraOrientation(const Quaternion &q)
{
	if (gEnv->mainCamera) gEnv->mainCamera->setOrientation(Quaternion(q.w, q.x, q.y, q.z));
}

void GameScript::setCameraYaw(float rotX)
{
	if (gEnv->mainCamera) gEnv->mainCamera->yaw(Degree(rotX));
}

void GameScript::setCameraPitch(float rotY)
{
	if (gEnv->mainCamera) gEnv->mainCamera->pitch(Degree(rotY));
}

void GameScript::setCameraRoll(float rotZ)
{
	if (gEnv->mainCamera) gEnv->mainCamera->roll(Degree(rotZ));
}

Vector3 GameScript::getCameraPosition()
{
	Vector3 result(Vector3::ZERO);
	if (gEnv->mainCamera) result = gEnv->mainCamera->getPosition();
	return result;
}

Vector3 GameScript::getCameraDirection()
{
	Vector3 result(Vector3::ZERO);
	if (gEnv->mainCamera) result = gEnv->mainCamera->getDirection();
	return result;
}

Quaternion GameScript::getCameraOrientation()
{
	Quaternion result(Quaternion::ZERO);
	if (gEnv->mainCamera) result = gEnv->mainCamera->getOrientation();
	return result;
}

void GameScript::cameraLookAt(const Vector3 &pos)
{
	if (gEnv->mainCamera) gEnv->mainCamera->lookAt(Vector3(pos.x, pos.y, pos.z));
}

#ifdef USE_CURL
//hacky hack to fill memory with data for curl
// from: http://curl.haxx.se/libcurl/c/getinmemory.html
static size_t curlWriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct curlMemoryStruct *mem = (struct curlMemoryStruct *)data;
  char* new_mem;

  new_mem = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if (new_mem == NULL) {
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

void *onlineAPIThread(void *_params)
{
	// copy over params
	GameScript::OnlineAPIParams_t params = *(GameScript::OnlineAPIParams_t *)_params;

	// call the function
	params.cls->useOnlineAPIDirectly(params);

	// free the params
	params.dict->Release();
	free(_params);
	_params = NULL;

	pthread_exit(NULL);
	return NULL;
}

int GameScript::useOnlineAPIDirectly(OnlineAPIParams_t params)
{
#ifdef USE_CURL
	struct curlMemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	// construct post fields
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	curl_global_init(CURL_GLOBAL_ALL);

	std::map<String, AngelScript::CScriptDictionary::valueStruct>::const_iterator it;
	for (it = params.dict->dict.begin(); it != params.dict->dict.end(); it++)
	{
		int typeId = it->second.typeId;
		if (typeId == mse->getEngine()->GetTypeIdByDecl("string"))
		{
			// its a String
			String *str = (String *)it->second.valueObj;
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, str->c_str(), CURLFORM_END);
		}
		else if (typeId == AngelScript::asTYPEID_INT8 \
			|| typeId == AngelScript::asTYPEID_INT16 \
			|| typeId == AngelScript::asTYPEID_INT32 \
			|| typeId == AngelScript::asTYPEID_INT64)
		{
			// its an integer
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((int)it->second.valueInt).c_str(), CURLFORM_END);
		}
		else if (typeId == AngelScript::asTYPEID_UINT8 \
			|| typeId == AngelScript::asTYPEID_UINT16 \
			|| typeId == AngelScript::asTYPEID_UINT32 \
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
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ScriptName", CURLFORM_COPYCONTENTS, mse->getScriptName().c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ScriptHash", CURLFORM_COPYCONTENTS, mse->getScriptHash().c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_NickName", CURLFORM_COPYCONTENTS, SSETTING("Nickname", "Anonymous").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Language", CURLFORM_COPYCONTENTS, SSETTING("Language", "English").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Token", CURLFORM_COPYCONTENTS, SSETTING("User Token Hash", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionString", CURLFORM_COPYCONTENTS, ROR_VERSION_STRING, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVN", CURLFORM_COPYCONTENTS, SVN_REVISION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVNID", CURLFORM_COPYCONTENTS, SVN_ID, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_ProtocolVersion", CURLFORM_COPYCONTENTS, RORNET_VERSION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_BinaryHash", CURLFORM_COPYCONTENTS, SSETTING("BinaryHash", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_GUID", CURLFORM_COPYCONTENTS, SSETTING("GUID", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerName", CURLFORM_COPYCONTENTS, SSETTING("Server name", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerPort", CURLFORM_COPYCONTENTS, SSETTING("Server port", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_NetworkEnabled", CURLFORM_COPYCONTENTS, SSETTING("Network enable", "No").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "APIProtocolVersion", CURLFORM_COPYCONTENTS, "2", CURLFORM_END);

	if (BeamFactory::getSingleton().getCurrentTruck())
	{
		Beam *truck = BeamFactory::getSingleton().getCurrentTruck();
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_Name",     CURLFORM_COPYCONTENTS, truck->getTruckName().c_str(), CURLFORM_END);
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_FileName", CURLFORM_COPYCONTENTS, truck->getTruckFileName().c_str(), CURLFORM_END);
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_Hash",     CURLFORM_COPYCONTENTS, truck->getTruckHash().c_str(), CURLFORM_END);

		// look for any locked trucks
		int i = 0;
		for (std::vector<hook_t>::iterator it = truck->hooks.begin(); it != truck->hooks.end(); it++, i++)
		{
			Beam *trailer = it->lockTruck;
			if (trailer && trailer->getTruckName() != trailer->getTruckName())
			{
				String name = "Trailer_" + TOSTRING(i) + "_Name";
				curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, name.c_str(),     CURLFORM_COPYCONTENTS, trailer->getTruckName().c_str(), CURLFORM_END);
				String filename = "Trailer_" + TOSTRING(i) + "_FileName";
				curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, filename.c_str(), CURLFORM_COPYCONTENTS, trailer->getTruckFileName().c_str(), CURLFORM_END);
				String hash = "Trailer_" + TOSTRING(i) + "_Hash";
				curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, hash.c_str(),     CURLFORM_COPYCONTENTS, trailer->getTruckHash().c_str(), CURLFORM_END);
			}
		}
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Trailer_Count",     CURLFORM_COPYCONTENTS, TOSTRING(i).c_str(), CURLFORM_END);
	}

	const RenderTarget::FrameStats& stats = RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getStatistics();
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "AVG_FPS", CURLFORM_COPYCONTENTS, TOSTRING(stats.avgFPS).c_str(), CURLFORM_END);



	CURLcode res;
	CURL *curl = curl_easy_init();
	if (!curl)
	{
		LOG("ERROR: failed to init curl");
		return 1;
	}

	char *curl_err_str[CURL_ERROR_SIZE];
	memset(curl_err_str, 0, CURL_ERROR_SIZE);

	String url = "http://" + String(REPO_SERVER) + params.apiquery;
	curl_easy_setopt(curl, CURLOPT_URL,              url.c_str());

	/* send all data to this function  */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	// set post options
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

	// logging stuff
	//curl_easy_setopt(curl, CURLOPT_STDERR,           LogManager::getsin InstallerLog::getSingleton()->getLogFilePtr());
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,      curl_err_str[0]);

	// http related settings
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,   1); // follow redirects
	curl_easy_setopt(curl, CURLOPT_AUTOREFERER,      1); // set the Referrer: field in requests where it follows a Location: redirect.
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS,        20);
	curl_easy_setopt(curl, CURLOPT_USERAGENT,        "RoR");
	curl_easy_setopt(curl, CURLOPT_FILETIME,         1);

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
		const char *errstr = curl_easy_strerror(res);
		result = "ERROR: " + String(errstr);
	}

	LOG("online API result: " + result);

#ifdef USE_MYGUI
	Console *con = RoR::Application::GetConsole();
	if (con)
	{
		con->putMessage(Console::CONSOLE_MSGTYPE_HIGHSCORE, Console::CONSOLE_SYSTEM_NOTICE, ANSI_TO_UTF(result));
		RoR::Application::GetGuiManager()->PushNotification("Script:", ANSI_TO_UTF(result));
	}
#endif // USE_MYGUI
#endif //USE_CURL
	return 0;
}

int GameScript::useOnlineAPI(const String &apiquery, const AngelScript::CScriptDictionary &d, String &result)
{
	// malloc this, so we are safe from this function scope
	OnlineAPIParams_t *params = (OnlineAPIParams_t *)malloc(sizeof(OnlineAPIParams_t));
	if (!params)
	{
		free(params);
		return 1;
	}
	params->cls      = this;
	strncpy(params->apiquery, apiquery.c_str(), 2048);

	//wrap a new dict around this, as we dont know if or when the script will release it
	AngelScript::CScriptDictionary *newDict = new AngelScript::CScriptDictionary(mse->getEngine());
	// copy over the dict, the main data
	newDict->dict    = d.dict;
	// assign it to the data container
	params->dict     = newDict;
	// tell the script that there will be no direct feedback
	result           = "asynchronous";

#ifdef USE_MYGUI
	Console *con = RoR::Application::GetConsole();
	if (con) con->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("using Online API..."), "information.png", 2000);
	RoR::Application::GetGuiManager()->PushNotification("Notice:", _L("using Online API...") + TOSTRING(""));
#endif // USE_MYGUI

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
			String *str = (String *)it->second.valueObj;
			it->second.valueObj = (void *)new String(*str);
		}
	}

	// create the thread
	LOG("creating thread for online API usage...");
	int rc = pthread_create(&apiThread, NULL, onlineAPIThread, (void *)params);
	if (rc)
	{
		LOG("useOnlineAPI/pthread error code: " + TOSTRING(rc));
		return 1;
	}

	return 0;
}

void GameScript::boostCurrentTruck(float factor)
{
    // add C++ code here
	Beam *b = BeamFactory::getSingleton().getCurrentTruck();
	if (b && b->engine)
	{
		float rpm = b->engine->getRPM();
		rpm += 2000.0f * factor;
		b->engine->setRPM(rpm);
	}
}

int GameScript::addScriptFunction(const String &arg)
{
	return mse->addFunction(arg);
}

int GameScript::scriptFunctionExists(const String &arg)
{
	return mse->functionExists(arg);
}

int GameScript::deleteScriptFunction(const String &arg)
{
	return mse->deleteFunction(arg);
}

int GameScript::addScriptVariable(const String &arg)
{
	return mse->addVariable(arg);
}

int GameScript::deleteScriptVariable(const String &arg)
{
	return mse->deleteVariable(arg);
}

int GameScript::sendGameCmd(const String& message)
{
	if (!gEnv->network)
	{
		return -11;
	} else 
	{
		return gEnv->network->sendScriptMessage(const_cast<char*>(message.c_str()), (unsigned int)message.size());
	}
}
