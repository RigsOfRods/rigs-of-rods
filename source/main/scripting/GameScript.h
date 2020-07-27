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

/// @file
/// @author Thomas Fischer
/// @date   24th of February 2009

#pragma once

#include "RoRPrerequisites.h"

#include "ScriptEngine.h"

#include <angelscript.h>

struct curlMemoryStruct
{
    char* memory;
    size_t size;
};

/**
 *  @brief Proxy class that can be called by script functions
 */
class GameScript : public ZeroedMemoryAllocator
{
public:
    /**
     * constructor
     * @param se pointer to the ScriptEngine instance
     */
    GameScript(ScriptEngine* se);

    /**
     * destructor
     */
    ~GameScript();

    /**
     * writes a message to the games log (RoR.log)
     * @param msg string to log
     */
    void log(const Ogre::String& msg);

    /**
     * writes a message to the games log (RoR.log)
     * @param msg string to log
     */
    void logFormat(const char* fmt, ...);

    /**
     * moves the person relative
     * @param vec translation vector
     */
    void activateAllVehicles();

    /**
     * moves the person relative
     * @param vec translation vector
     */
    void SetTrucksForcedAwake(bool forceActive);

    /**
     * returns the time in seconds since the game was started
     * @return time in seconds
     */
    float getTime();

    //anglescript test
    void boostCurrentTruck(float factor);

    /**
     * sets the character position
     * @param vec position vector on the terrain
     */
    void setPersonPosition(const Ogre::Vector3& vec);

    void loadTerrain(const Ogre::String& terrain);
    /**
     * moves the person relative
     * @param vec translation vector
     */
    void movePerson(const Ogre::Vector3& vec);

    /**
     * sets the character rotation
     * @param rot the character rotation
     */
    void setPersonRotation(const Ogre::Radian& rot);

    /**
     * gets the character rotation
     * @return character rotation
     */
    Ogre::Radian getPersonRotation();

    /**
     * gets the time of the day in seconds
     * @return string with HH::MM::SS format
     */
    Ogre::String getCaelumTime();

    /**
     * sets the time of the day in seconds
     * @param value day time in seconds
     */
    void setCaelumTime(float value);

    /**
     * returns the current base water level (without waves)
     * @return water height in meters
     */
    float getWaterHeight();

    float getGroundHeight(Ogre::Vector3& v);

    /**
     * sets the base water height
     * @param value base height in meters
     */
    void setWaterHeight(float value);

    /**
     * returns the current selected truck, 0 if in person mode
     * @return reference to Beam object that is currently in use
     */
    Actor* getCurrentTruck();

    /**
     * returns a truck by index, get max index by calling getNumTrucks
     * @return reference to Beam object that the selected slot
     */
    Actor* getTruckByNum(int num);

    /**
     * returns the current amount of loaded trucks
     * @return integer value representing the amount of loaded trucks
     */
    int getNumTrucks();

    /**
     * returns the current truck number. >=0 when using a truck, -1 when in person mode
     * @return integer truck number
     */
    int GetPlayerActorId();

    /**
     * returns the currently set upo gravity
     * @return float number describing gravity terrain wide.
     */
    float getGravity();

    /**
     * sets the gravity terrain wide. This is an expensive call, since the masses of all trucks are recalculated.
     * @param value new gravity terrain wide (default is -9.81)
     */
    void setGravity(float value);

    /**
     * registers for a new event to be received by the scripting system
     * @param eventValue \see enum scriptEvents
     */
    void registerForEvent(int eventValue);

    /**
     * DEPRECATED: use message
     * shows a message to the user
     */
    void flashMessage(Ogre::String& txt, float time, float charHeight);

    /**
     * shows a message to the user over the console system
     */
    void message(Ogre::String& txt, Ogre::String& icon, float timeMilliseconds, bool forceVisible);

    /**
     * set direction arrow
     * @param text text to be displayed. "" to hide the text
     */
    void UpdateDirectionArrow(Ogre::String& text, Ogre::Vector3& vec);

    /**
     * returns the size of the font used by the chat box
     * @return pixel size of the chat text
     */
    int getChatFontSize();

    /**
     * changes the font size of the chat box
     * @param size font size in pixels
     */
    void setChatFontSize(int size);

    /**
     * Sets the camera's position.
     * @param pos The new position of the camera.
     */
    void setCameraPosition(const Ogre::Vector3& pos);

    /**
     * Sets the camera's direction vector.
     * @param vec A vector representing the direction of the vector.
     */
    void setCameraDirection(const Ogre::Vector3& vec);

    /**
     * Sets the camera's orientation.
     * @param vec A vector representing the direction of the vector.
     */
    void setCameraOrientation(const Ogre::Quaternion& q);

    /**
     * Rolls the camera anticlockwise, around its local z axis.
     * @param angle The roll-angle
     */
    void setCameraRoll(float angle);

    /**
     * Rotates the camera anticlockwise around it's local y axis.
     * @param angle The yaw-angle
     */
    void setCameraYaw(float angle);

    /**
     * Pitches the camera up/down anticlockwise around it's local z axis.
     * @param angle The pitch-angle
     */
    void setCameraPitch(float angle);

    /**
      * Retrieves the camera's position.
      * @return The current position of the camera
     */
    Ogre::Vector3 getCameraPosition();

    /**
     * Gets the camera's direction.
     * @return A vector representing the direction of the camera
     */
    Ogre::Vector3 getCameraDirection();

    /**
     * Gets the camera's orientation.
     * @return A quaternion representing the orientation of the camera
     */
    Ogre::Quaternion getCameraOrientation();

    /**
     * Points the camera at a location in worldspace.
     * @remarks
     *      This is a helper method to automatically generate the
     *      direction vector for the camera, based on it's current position
     *      and the supplied look-at point.
     * @param targetPoint A vector specifying the look at point.
    */
    void cameraLookAt(const Ogre::Vector3& targetPoint);

    /**
     * Adds a global function to the script
     * (Wrapper for ScriptEngine::addFunction)
     * @param arg A declaration for the function.
    */
    int addScriptFunction(const Ogre::String& arg);

    /**
     * Checks if a global function exists in the script
     * (Wrapper for ScriptEngine::functionExists)
     * @param arg A declaration for the function.
    */
    int scriptFunctionExists(const Ogre::String& arg);

    /**
     * Deletes a global function from the script
     * (Wrapper for ScriptEngine::deleteFunction)
     * @param arg A declaration for the function.
    */
    int deleteScriptFunction(const Ogre::String& arg);

    /**
     * Adds a global variable to the script
     * (Wrapper for ScriptEngine::addVariable)
     * @param arg A declaration for the variable.
    */
    int addScriptVariable(const Ogre::String& arg);

    /**
     * Deletes a global variable from the script
     * (Wrapper for ScriptEngine::deleteVariable)
     * @param arg A declaration for the variable.
    */
    int deleteScriptVariable(const Ogre::String& arg);

    /**
    * This spawns an object
    * @param objectName The name of the object (~the name of the odef file, but without the .odef extension)
    * @param instanceName A unique name for this object (you can choose one, but make sure that you don't use the same name twice)
    * @param pos The position where the object should be spawned
    * @param rot The rotation in which the object should be spawned
    * @param eventhandler A name of a function that should be called when an event happens (events, as defined in the object definition file)
    * @param uniquifyMaterials Set this to true if you need to uniquify the materials
    */
    void spawnObject(const Ogre::String& objectName, const Ogre::String& instanceName, const Ogre::Vector3& pos, const Ogre::Vector3& rot, const Ogre::String& eventhandler, bool uniquifyMaterials);
    /**
    * This destroys an object
    * @param instanceName The unique name that you chose when spawning this object
    * @see spawnObject
    */
    void destroyObject(const Ogre::String& instanceName);
    /**
    * This moves an object to a new position
    * @note This doesn't update the collision box!
    * @param instanceName The unique name that you chose when spawning this object
    * @param pos The position where the object should be moved to
    */
    void MoveTerrainObjectVisuals(const Ogre::String& instanceName, const Ogre::Vector3& pos);

    // new things, not documented yet
    void showChooser(const Ogre::String& type, const Ogre::String& instance, const Ogre::String& box);
    void repairVehicle(const Ogre::String& instance, const Ogre::String& box, bool keepPosition);
    void removeVehicle(const Ogre::String& instance, const Ogre::String& box);

    int getNumTrucksByFlag(int flag);
    bool getCaelumAvailable();
    void stopTimer();
    void startTimer(int id);
    void setTimeDiff(float diff);
    void setBestLapTime(float time);
    Ogre::String getSetting(const Ogre::String& str);
    void hideDirectionArrow();
    int setMaterialAmbient(const Ogre::String& materialName, float red, float green, float blue);
    int setMaterialDiffuse(const Ogre::String& materialName, float red, float green, float blue, float alpha);
    int setMaterialSpecular(const Ogre::String& materialName, float red, float green, float blue, float alpha);
    int setMaterialEmissive(const Ogre::String& materialName, float red, float green, float blue);
    int getSafeTextureUnitState(Ogre::TextureUnitState** tu, const Ogre::String materialName, int techniqueNum, int passNum, int textureUnitNum);
    int setMaterialTextureName(const Ogre::String& materialName, int techniqueNum, int passNum, int textureUnitNum, const Ogre::String& textureName);
    int setMaterialTextureRotate(const Ogre::String& materialName, int techniqueNum, int passNum, int textureUnitNum, float rotation);
    int setMaterialTextureScroll(const Ogre::String& materialName, int techniqueNum, int passNum, int textureUnitNum, float sx, float sy);
    int setMaterialTextureScale(const Ogre::String& materialName, int techniqueNum, int passNum, int textureUnitNum, float u, float v);

    float rangeRandom(float from, float to);
    int useOnlineAPI(const Ogre::String& apiquery, const AngelScript::CScriptDictionary& dict, Ogre::String& result);

    int getLoadedTerrain(Ogre::String& result);
    Ogre::Vector3 getPersonPosition();

    void clearEventCache();
    int sendGameCmd(const Ogre::String& message);

    VehicleAI* getCurrentTruckAI();
    VehicleAI* getTruckAIByNum(int num);

    Actor* spawnTruck(const std::string& truckName, const Ogre::Vector3& pos, const Ogre::Vector3& rot, const std::string& skinName);

    void showMessageBox(Ogre::String& title, Ogre::String& text, bool use_btn1, Ogre::String& btn1_text, bool allow_close, bool use_btn2, Ogre::String& btn2_text);
    void backToMenu();
    void quitGame();
    float getFPS();
    float getAvgFPS();

private:

    bool HaveSimController(const char* func_name); //!< Helper; Check if SimController instance exists, log warning if not.
    bool HaveSimTerrain(const char* func_name); //!< Helper; Check if SimController instance exists, log warning if not.
    bool HavePlayerAvatar(const char* func_name); //!< Helper; Check if local Character instance exists, log warning if not.
    bool HaveMainCamera(const char* func_name); //!< Helper; Check if main camera exists, log warning if not.

    ScriptEngine* mse; //!< local script engine pointer, used as proxy mostly
};

