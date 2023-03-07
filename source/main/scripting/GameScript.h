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

/// @file
/// @author Thomas Fischer
/// @date   24th of February 2009

#pragma once

#include "Application.h"

#include <angelscript.h>
#include <scriptdictionary/scriptdictionary.h>

namespace RoR {

/// @addtogroup Scripting
/// @{

/**
 *  @brief Proxy class that can be called by script functions
 */
class GameScript : public ZeroedMemoryAllocator
{
public:
    // PLEASE maintain the same order as in GameScriptAngelscript.cpp!

    /// @name General
    /// @{

    /**
     * writes a message to the games log (RoR.log)
     * @param msg string to log
     */
    void log(const Ogre::String& msg);

    /**
     * returns the time in seconds since the game was started
     * @return time in seconds
     */
    float getTime();

    void backToMenu();
    void quitGame();
    float getFPS();
    float getAvgFPS();
    float rangeRandom(float from, float to);

    int useOnlineAPI(const Ogre::String& apiquery, const AngelScript::CScriptDictionary& dict, Ogre::String& result);

    /**
    * Pushes a message to internal message queue. Parameters are listed in `Script2Game::MsgType` comments.
    * @return True if the message was pushed, false if it was rejected.
    */
    bool pushMessage(MsgType type, AngelScript::CScriptDictionary* dict);

    /// @}

    /// @name GUI
    /// @{

    /**
     * DEPRECATED: use message()
     * shows a message to the user
     */
    void flashMessage(Ogre::String& txt, float time, float charHeight);

    /**
     * shows a message to the user over the console system
     */
    void message(Ogre::String& txt, Ogre::String& icon);

    /**
     * OBSOLETE, returns 0;
     */
    int getChatFontSize();

    /**
     * OBSOLETE, does nothing.
     */
    void setChatFontSize(int size);

    void showMessageBox(Ogre::String& title, Ogre::String& text, bool use_btn1, Ogre::String& btn1_text, bool allow_close, bool use_btn2, Ogre::String& btn2_text);

    void showChooser(const Ogre::String& type, const Ogre::String& instance, const Ogre::String& box);

    /**
     * set direction arrow
     * @param text text to be displayed. "" to hide the text
     */
    void updateDirectionArrow(Ogre::String& text, Ogre::Vector3& vec);

    void hideDirectionArrow();

    /**
    * @param world_pos The world position to be converted, in meters.
    * @param out_screen_pos The resulting screen position, in pixels.
    * @return true if the world position is in front of the camera and the resulting screen position is valid.
    */
    bool getScreenPosFromWorldPos(Ogre::Vector3 const& world_pos, Ogre::Vector2& out_screen_pos);

    /**
    * Gets screen size in pixels.
    */
    Ogre::Vector2 getDisplaySize();

    /**
    * Gets mouse position in pixels.
    */
    Ogre::Vector2 getMouseScreenPosition();

    /// @}

    /// @name Script management
    /// @{

    /**
     * registers for a new event to be received by the scripting system
     * @param eventValue \see enum scriptEvents
     */
    void registerForEvent(int eventValue);

    /**
     * unregisters from receiving event.
     * @param eventValue \see enum scriptEvents
     */
    void unRegisterEvent(int eventValue);

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

    void clearEventCache();

    /**
    * Multiplayer only: sends AngelScript snippet to all players.
    */
    int sendGameCmd(const Ogre::String& message);

    /// @}

    /// @name Terrain
    /// @{
    
    void loadTerrain(const Ogre::String& terrain);

    /**
     * gets the name of current terrain.
     * @return 1 on success, 0 if terrain not loaded
     */
    int getLoadedTerrain(Ogre::String& result);

    bool getCaelumAvailable();

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
    * Gets terrain height at given coordinates.
    */
    float getGroundHeight(Ogre::Vector3& v);

    /**
     * returns the current base water level (without waves)
     * @return water height in meters
     */
    float getWaterHeight();

    /**
     * sets the base water height
     * @param value base height in meters
     */
    void setWaterHeight(float value);

    /**
    * This spawns a static terrain object (.ODEF file)
    * @param objectName The name of the object (~the name of the odef file, but without the .odef extension)
    * @param instanceName A unique name for this object (you can choose one, but make sure that you don't use the same name twice)
    * @param pos The position where the object should be spawned
    * @param rot The rotation in which the object should be spawned
    * @param eventhandler A name of a function that should be called when an event happens (events, as defined in the object definition file)
    * @param uniquifyMaterials Set this to true if you need to uniquify the materials
    */
    void spawnObject(const Ogre::String& objectName, const Ogre::String& instanceName, const Ogre::Vector3& pos, const Ogre::Vector3& rot, const Ogre::String& eventhandler, bool uniquifyMaterials);

    /**
    * This moves an object to a new position
    * @note This doesn't update the collision box!
    * @param instanceName The unique name that you chose when spawning this object
    * @param pos The position where the object should be moved to
    */
    void moveObjectVisuals(const Ogre::String& instanceName, const Ogre::Vector3& pos);

    /**
    * This destroys an object
    * @param instanceName The unique name that you chose when spawning this object
    * @see spawnObject
    */
    void destroyObject(const Ogre::String& instanceName);

    /**
    * Calculates mouse cursor position on terrain.
    * @param out_pos Calculated position, in meters.
    * @return true if mouse points to the terrain and output coordinates are valid.
    */
    bool getMousePositionOnTerrain(Ogre::Vector3& out_pos);

    TerrainPtr getTerrain();

    /// @}

    /// @name Character
    /// @{

    Ogre::Vector3 getPersonPosition();
    
    /**
     * sets the character position
     * @param vec position vector on the terrain
     */
    void setPersonPosition(const Ogre::Vector3& vec);

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

    /// @}

    /// @name Actors
    /// @{

    /**
     * moves the person relative
     * @param vec translation vector
     */
    void activateAllVehicles();

    /**
     * moves the person relative
     * @param vec translation vector
     */
    void setTrucksForcedAwake(bool forceActive);

    //anglescript test
    void boostCurrentTruck(float factor);

    /**
     * returns the current selected truck, 0 if in person mode
     * @return reference to Beam object that is currently in use
     */
    ActorPtr getCurrentTruck();

    /**
     * returns a truck by index, get max index by calling getNumTrucks
     * @return reference to Beam object that the selected slot
     */
    ActorPtr getTruckByNum(int num);

    /**
     * returns the current amount of loaded trucks
     * @return integer value representing the amount of loaded trucks
     */
    int getNumTrucks();

    /**
     * returns the current truck number. >=0 when using a truck, -1 when in person mode
     * @return integer truck number
     */
    int getCurrentTruckNumber();

    ActorPtr spawnTruck(Ogre::String& truckName, Ogre::Vector3& pos, Ogre::Vector3& rot);
    ActorPtr spawnTruckAI(Ogre::String& truckName, Ogre::Vector3& pos, Ogre::String& truckSectionConfig, std::string& truckSkin, int x);
    AngelScript::CScriptArray* getWaypoints(int x);
    void addWaypoint(const Ogre::Vector3& pos);
    int getAIVehicleCount();
    int getAIVehicleDistance();
    int getAIVehiclePositionScheme();
    int getAIVehicleSpeed();
    Ogre::String getAIVehicleName(int x);
    Ogre::String getAIVehicleSectionConfig(int x);
    std::string getAIVehicleSkin(int x);
    int getAIRepeatTimes();

    void repairVehicle(const Ogre::String& instance, const Ogre::String& box, bool keepPosition);

    void removeVehicle(const Ogre::String& instance, const Ogre::String& box);

    int getNumTrucksByFlag(int flag);

    VehicleAIPtr getCurrentTruckAI();
    VehicleAIPtr getTruckAIByNum(int num);

    ///@}

    /// @name Camera
    /// @{

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

    ///@}

    /// @name Race system
    /// @{

    void stopTimer();
    void startTimer(int id);
    void setTimeDiff(float diff);
    void setBestLapTime(float time);

    ///@}

    /// @name Material helpers
    /// @{

    int setMaterialAmbient(const Ogre::String& materialName, float red, float green, float blue);
    int setMaterialDiffuse(const Ogre::String& materialName, float red, float green, float blue, float alpha);
    int setMaterialSpecular(const Ogre::String& materialName, float red, float green, float blue, float alpha);
    int setMaterialEmissive(const Ogre::String& materialName, float red, float green, float blue);
    int getSafeTextureUnitState(Ogre::TextureUnitState** tu, const Ogre::String materialName, int techniqueNum, int passNum, int textureUnitNum);
    int setMaterialTextureName(const Ogre::String& materialName, int techniqueNum, int passNum, int textureUnitNum, const Ogre::String& textureName);
    int setMaterialTextureRotate(const Ogre::String& materialName, int techniqueNum, int passNum, int textureUnitNum, float rotation);
    int setMaterialTextureScroll(const Ogre::String& materialName, int techniqueNum, int passNum, int textureUnitNum, float sx, float sy);
    int setMaterialTextureScale(const Ogre::String& materialName, int techniqueNum, int passNum, int textureUnitNum, float u, float v);

    ///@}

private:

    bool HaveSimTerrain(const char* func_name); //!< Helper; Check if SimController instance exists, log warning if not.
    bool HavePlayerAvatar(const char* func_name); //!< Helper; Check if local Character instance exists, log warning if not.
    bool HaveMainCamera(const char* func_name); //!< Helper; Check if main camera exists, log warning if not.

    template<typename T> bool GetValueFromDict(const std::string& log_msg, AngelScript::CScriptDictionary* dict, bool required, std::string const& key, const char* type_decl, T& out_value); //!< Helper; get value from dict safely or log warning.

    /**
     * writes a message to the games log (RoR.log)
     * @param msg string to log
     */
    void logFormat(const char* fmt, ...);
};

/// @}   //addtogroup Scripting

} // namespace RoR
