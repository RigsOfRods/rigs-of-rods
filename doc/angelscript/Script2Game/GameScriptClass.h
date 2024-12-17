/*
This source file is part of Rigs of Rods

For more information, see http://www.rigsofrods.org/

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
// created on 23th May 2011 by neorej16

namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

/**
 * @brief Binding of RoR::GameScript; A general class that will provide you with general functions.
 * @note Created automatically as global variable `game`.
 *       E.g.: you can log a message using game.log("Hi, I'm a message");
 */
class GameScriptClass
{
public:
    // PLEASE maintain the same order as in GameScript.h and GameScriptAngelscript.cpp!

    /// @name General
    /// @{

	/**
	 * writes a message to the scripting logbook (AngelScript.log)
	 * @param message string to log
	 */
	void log(string message);

	/**
	 * returns the time in seconds since the game was started
	 * @return time in seconds
	 */
	double getTime();
    
	/**
	 * Generates a random number between from and to
	 * @param from The generated number will be higher than this number
	 * @param to The generated number will be lower than this number
	 * @return The random number between from and to
	 */
	float rangeRandom(float from, float to);

	/**
	 * Sends or request information from the master server
	 */
	int useOnlineAPI(const string apiquery, const dictionary dict, string result);
    
	/**
	*  Gets the Curent frames per second (FPS)
	*  @return The Current FPS
	*/
	void getFPS();
    
	/**
	*  Gets the average frames per second (FPS)
	*  @return The average FPS
	*/
	void getAvgFPS();

	/**
	*  Back to menu
	*/
	void backToMenu();

	/**
	*  Quits the game
	*/
	void quitGame();
    
    /**
    * Pushes a message to internal message queue. Parameters are listed in `Script2Game::MsgType` comments.
    * @return True if the message was pushed, false if it was rejected.
    */
    bool pushMessage(MsgType type, dictionary@ dict);
    
    /**
    * Checks if the resource file exists in the given group.
    * KNOWN LIMITATION: If existing file is deleted externally, this function will still report it exists until the resource group is reloaded.
    * @see https://ogrecave.github.io/ogre/api/latest/_resource-_management.html
    */
    bool checkResourceExists(const string &in filename, const string &in resource_group);

    /**
    * Deletes a resource from the given group.
    * @see https://ogrecave.github.io/ogre/api/latest/_resource-_management.html
    */
    bool deleteResource(const string &in filename, const string &in resource_group);

    /**
    * Loads a text file resource as string.
    * @see https://ogrecave.github.io/ogre/api/latest/_resource-_management.html
    * @param filename Resource name within the resource group, equivalent to filename without path.
    * @param resource_group Name of resource group to load from
    */
    std::string loadTextResourceAsString(const string &in filename, const string &in resource_group);

    /**
    * Saves a string as a text file resource.
    * @see https://ogrecave.github.io/ogre/api/latest/_resource-_management.html
    * @param data The file data.
    * @param filename Resource name within the resource group, equivalent to filename without path.
    * @param resource_group Name of resource group to save to. If the group has multiple locations, first writable location is used.
    * @param overwrite By default existing resources are not overwriten.
    * @return True on successful write. False if file not writable or exists and overwrite is disabled.
    */
    bool createTextResourceFromString(const string &in, const string &in filename, const string &in resource_group, bool overwrite=false);

    /**
    * Retrieves the OGRE scene manager object.
    */
    AngelOgre::SceneManager@ getSceneManager();

    /**
    * Opens URL (must start with 'http://' or 'https://') in system's default web browser.
    */
    void openUrlInDefaultBrowser(const std::string& url);

    /**
    * Invokes a background thread to fetch data using CURL; when finished, sends MSG_APP_SCRIPT_THREAD_STATUS + Payload = RoR::ScriptEventArgs* (owner) - see `SE_ANGELSCRIPT_THREAD_STATUS` for arguments.
    */
    void fetchUrlAsStringAsync(const std::string& url, const std::string& display_filename);

    /// @}

    /// @name GUI
    /// @{

	/**
	 * shows a message to the user
	 * @deprecated Use the game.message function instead.
	 * @param message The message to be shown
	 * @param time How long the message should be shown (in seconds)
	 * @param charHeight The font size to be used (use -1 for the default size)
	 */
	void flashMessage(string message, float time, float charHeight);

	/**
	 * shows a message to the user
	 * @param txt The message to be shown
	 * @param icon The filename of the icon to be shown in front of the message.
	 * @param timeMilliseconds How long the message should be shown (in milliseconds)
	 * @param forceVisible Set this to true if you want the message to be forced on the user's screen (~it will show, even when the GUI is hidden).
	 */
	void message(string txt, string icon, float timeMilliseconds, bool forceVisible);

	/**
	 * OBSOLETE - returns 0.
	 * @deprecated
	 * @return always 0
	 */
	int getChatFontSize();

	/**
	 * OBSOLETE - does nothing.
	 * @deprecated
	 * @param size font size in pixels
	 */
	void setChatFontSize(int size);

	/**
	 *  Shows a message box
	 *  
	 *  @param mTitle The box title
	 *  @param mText The box content text
	 *  @param button1 Set to true to show the first button
	 *  @param mButton1 The text in the first button
	 *  @param AllowClose If set to true the user can close the box by pressing the X in the top-right
	 *  @param button2 Set to true to show the second button
	 *  @param mButton2 The text in the second button
	 *  
	 *  @see scriptEvents
	 */
	void showMessageBox(string mTitle, stringmText, bool button1, stringmButton1, bool AllowClose, bool button2, stringmButton2);

	/** 
	 * This shows a vehicle chooser
	 * @param type A string specifying the type of the chooser.
	 *      You can choose out of the following:
	 *           - "vehicle"
	 *           - "truck"
	 *           - "car"
	 *           - "boat"
	 *           - "airplane"
	 *           - "heli"
	 *           - "trailer"
	 *           - "load"
	 *           - "extension"
	 * @param instance the unique name of the object with the event box in which the truck/load should be spawned
	 * @param box the name of the box in which the truck will be spawned
	*/
	void showChooser(string type, string instance, string box);

	/**
	 * set direction arrow
	 * @param text text to be displayed. "" to hide the text
	 * @param position The position to which the arrow should point.
	 * @see hideDirectionArrow
	 */
	void updateDirectionArrow(string text, vector3 position);

	/**
	 * Hides the direction arrow
	 * @see UpdateDirectionArrow
	 */
	void hideDirectionArrow();

    /**
    * @param world_pos The world position to be converted, in meters.
    * @param out_screen_pos The resulting screen position, in pixels.
    * @return true if the world position is in front of the camera and the resulting screen position is valid.
    */
    bool getScreenPosFromWorldPos(const vector3&in, vector2&out);
    
    /**
    * Gets screen size in pixels.
    */
    vector2 getDisplaySize();
    
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
    * Gets event mask for a specific running script. Intended for diagnostic and monitoring purposes.
    * @param nid ScriptUnitID, obtain one from global var `thisScript` or callback parameters.
    */
    BitMask_t getRegisteredEventsMask(ScriptUnitId_t nid);

    /**
     * Overwrites event mask for a specific running script. Intended for debugging tools - use with caution.
     * @param nid ScriptUnitID, obtain one from global var `thisScript` or callback parameters.
     * @param eventMask \see enum scriptEvents
     */
    void setRegisteredEventsMask(ScriptUnitId_t nid, BitMask_t eventMask);
    
	/**
	 * Adds a global function to the script.
	 * @param func the function to be added, e.g.: "void func() { log('works'); }"
     * @param nid ScriptUnitID to act upon, or -2 to fallback to the terrain script (defined in .terrn2 [Scripts], or 'default.as')
     * @return 0 on success, negative number on error.
	 */
	ScriptRetCode addScriptFunction(const string func, ScriptUnitId_t nid = -2);
	
	/**
	 * Checks if a global function exists
	 * @param func the declaration of the function that should be checked for existance, e.g.: "void func()"
     * @param nid ScriptUnitID to act upon, or -2 to fallback to the terrain script (defined in .terrn2 [Scripts], or 'default.as')
     * @return 0 on success, negative number on error.
	 */
	ScriptRetCode scriptFunctionExists(const string func, ScriptUnitId_t nid = -2);
	
	/**
	 * Removes a global function from the script.
	 * @param func the declaration of the function that should be removed, e.g.: "void func()"
     * @param nid ScriptUnitID to act upon, or -2 to fallback to the terrain script (defined in .terrn2 [Scripts], or 'default.as')
     * @return 0 on success, negative number on error.
	 */
	ScriptRetCode deleteScriptFunction(const string func, ScriptUnitId_t nid = -2);
	
	/**
	 * Adds a global variable to the script.
	 * @param var the declaration of the variable that should be added, e.g.: "int missionState;"
     * @param nid ScriptUnitID to act upon, or -2 to fallback to the terrain script (defined in .terrn2 [Scripts], or 'default.as')
     * @return 0 on success, negative number on error.
	 */
	ScriptRetCode addScriptVariable(const string var, ScriptUnitId_t nid = -2);
    
	/**
	 * Checks if a global variable exists in the script.
	 * @param var the declaration of the variable that should be removed, e.g.: "int missionState;"
     * @param nid ScriptUnitID to act upon, or -2 to fallback to the terrain script (defined in .terrn2 [Scripts], or 'default.as')
     * @return 0 on success, negative number on error.
	 */
	ScriptRetCode scriptVariableExists(const string var, ScriptUnitId_t nid = -2);    
	
	/**
	 * Removes a global variable from the script.
	 * @param var the declaration of the variable that should be removed, e.g.: "int missionState;"
     * @param nid ScriptUnitID to act upon, or -2 to fallback to the terrain script (defined in .terrn2 [Scripts], or 'default.as')
     * @return 0 on success, negative number on error.
	 */
	ScriptRetCode deleteScriptVariable(const string var, ScriptUnitId_t nid = -2);
    
    /**
     * Retrieves a memory address of a global variable in any script.
     * @param nid ScriptUnitID to act upon, or -2 to fallback to the terrain script (defined in .terrn2 [Scripts], or 'default.as')
     * @param varName Name of the variable. Type must match the reference type.
     * @param ref A variable-type parameter - accepts any reference.
     * @return 0 on success, negative number on error.
     */
    ScriptRetCode getScriptVariable(const string&in varName, ?&ref, ScriptUnitId_t nid = -2);    

	/**
	 * Clears the event cache
	 */
	void clearEventCache();
    
    /**
    * Multiplayer only: sends AngelScript snippet to all players.
    */
    int sendGameCmd(const string& message);    
    
    /**
    * Returns active ScriptUnitIDs; check agains global var `thisScript` or use `getScriptDetails()` to get name etc...
    */
    array<int> getRunningScripts();

    /**
    * Returns all info about running script; obtain the NID from `getRunningScripts()`, global var `thisScript` or event callbacks:
    *   * "uniqueId" (int64)
    *   * "scriptName" (string)
    *   * "scriptCategory" (enum ScriptCategory)
    *   * "eventMask" (int64)
    *   * "scriptBuffer" (string)
    */
    dictionary@ getScriptDetails(int nid);    

    /// @}

    /// @name Terrain
    /// @{
    
	/**
	 * Loads a terrain
	 * @warning
	 *      This doesn't unload the previous terrain!
	 *      Always make sure that there's no terrain loaded before
	 *      you use this method.
	 * @param terrain The name of the terrain
	 */
	void loadTerrain(string terrain);
    
	/**
	 * Gets the currently loaded terrain name
	 * @param result This string will contain the name of the terrain after you called this function
	 * @return 1 on success, 0 if terrain not loaded
	 */
	int getLoadedTerrain(string result);
    
	/**
	 * Gets the currently loaded terrain instance
	 */    
	TerrainClass@ getTerrain();
    
	/**
	 * Checks if Caleum is enabled.
	 * @return true if Caleum is available
	 */
	bool getCaelumAvailable();
    
	/**
	 * gets the time of the day in seconds
	 * @return string with HH::MM::SS format
	 */
	string getCaelumTime();
	
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
	 * returns the current base water level (without waves)
	 * @return water height in meters
	 */
	float getWaterHeight();

	/**
	 * returns the ground height for a position
	 * @param position The position for which the height should be returned
	 * @return ground height in meters
	 */
	float getGroundHeight(vector3 position);

	/**
	 * sets the base water height
	 * @param value base height in meters
	 */
	void setWaterHeight(float value);
    
	/**
	 * This spawns an object
	 * @param objectName The name of the object (~the name of the odef file, but without the .odef extension)
	 * @param instanceName A unique name for this object (you can choose one, but make sure that you don't use the same name twice)
	 * @param pos The position where the object should be spawned
	 * @param rot The rotation in which the object should be spawned
	 * @param eventhandler A name of a function that should be called when an event happens (events, as defined in the object definition file). Enter empty string to ignore events.
	 * @param uniquifyMaterials Set this to true if you need to uniquify the materials
	 */
	void spawnObject(const string objectName, const string instanceName, vector3 pos, vector3 rot, const string eventhandler, bool uniquifyMaterials);
    
	/**
	 * This moves an object to a new position
	 * @note This doesn't update the collision box!
	 * @param instanceName The unique name that you chose when spawning this object
	 * @param pos The position where the object should be moved to
	 */
	void moveObjectVisuals(const string instanceName, const vector3 pos);

	/**
	 * This destroys an object
	 * @param instanceName The unique name that you chose when spawning this object
	 * @see spawnObject
	 */
	void destroyObject(const string instanceName);
    
    /**
    * Returns all static objects on map (from any source).
    */
    array<TerrainEditorObjectClassPtr@>@ getEditorObjects();

    
    /**
    * Calculates mouse cursor position on terrain.
    * @param out_pos Calculated position, in meters.
    * @return true if mouse points to the terrain and output coordinates are valid.
    */
    bool getMousePositionOnTerrain(vector3 &out);

    /**
    * Returns `array<Ogre::MovableObjects@>` in no particular order; works using bounding boxes so large/generated meshes like roads get matched all the time.
    */
    array<Ogre::MovableObjects@> getMousePointedMovableObjects();

    /// @}

    /// @name Character
    /// @{

	/**
	 * Returns the current position of the person
	 * @return A vector containing the X, Y and Z coordinate of the person or an empty vector if the user is in a truck
	 */
	vector3 getPersonPosition();

	/**
	 * sets the character position
	 * @param vec X, Y and Z coordinate of the position on the terrain
	 */
	void setPersonPosition(vector3 vec);

	/**
	 * moves the person relative
	 * @param relative_movement X, Y and Z coordinate of the translation
	 */
	void movePerson(vector3 relative_movement);
    
    void setPersonRotation(radian &in);
    
    radian getPersonRotation();

    /// @}

    /// @name Actors
    /// @{
        
    void activateAllVehicles();

    void setTrucksForcedAwake(bool forceActive);        
        
	/**
	 * Gives the currently used truck a boost in RPM.
	 * @param factor This factor determines by how much that the RPM of the truck will be increased ( rpm += 2000.0f * factor ).
	 */
	void boostCurrentTruck(float factor);        

	/**
	 * returns the current selected truck, null if in person mode
	 * @remarks returns null if the truck does not exist
	 * @return reference to Beam object that is currently in use
	 */
	BeamClass@ getCurrentTruck();

	/**
	 * Get an actor by Instance ID, get one from `eventCallbackEx()` or manually by `BeamClass::getInstanceId()`.
	 * @param truck_number Actor instance ID.
	 * @see GameScriptClass.getCurrentTruckNumber()
	 * @return returns null if the actor does not exist
	 */
	BeamClass@ getTruckByNum(int truck_number);

    /**
     * returns an array of all currently existing actors.
     */
    array<BeamClass@> getAllTrucks();

	/**
	 * returns the instance ID of current player vehicle, or -1 when in person mode
	 */
	int getCurrentTruckNumber();
    
	/**
	 *  Spawns a truck by filename
	 *  
	 *  @param truckName The filename of the truck
	 *  @param pos The position where the truck should be spawned
	 *  @param rot The rotation in which the truck should be spawned
	 *  @return reference to Beam object
	 */
	BeamClass @spawnTruck(stringtruckName, vector3 pos, vector3 rot);
    
	/**
	 * This method repairs the vehicle in the box
	 */
	void repairVehicle(string instance, string box, bool keepPosition);
    
	/**
	 * This method removes the vehicle in the box
	 */
	void removeVehicle(string instance, string box);

	/**
	 * Number of trucks with flag
	 */
	int getNumTrucksByFlag(int flag);
	
    ///@}    
    
    /// @name Waypoint AI for Actors
    /// @brief to understand these values, run the game and look in TopMenubar UI->VehicleAI tab.
    /// @{

    BeamClass@ spawnTruckAI(string truckName, vector3 pos, string truckSectionConfig, string truckSkin, int x);
    array<vector3>@ getWaypoints(int x);
    void addWaypoint(vector3 pos);
    int getAIVehicleCount();
    int getAIVehicleDistance();
    int getAIVehiclePositionScheme();
    int getAIVehicleSpeed();
    Ogre::String getAIVehicleName(int x);
    Ogre::String getAIVehicleSectionConfig(int x);
    std::string getAIVehicleSkin(int x);
    int getAIRepeatTimes();
    int getAIMode();
    VehicleAIClass@ getCurrentTruckAI();
    VehicleAIClass@ getTruckAIByNum(int num);
    
    // AI: set
    void setAIVehicleCount(int count);
    void setAIVehicleDistance(int dist);
    void setAIVehiclePositionScheme(int scheme);
    void setAIVehicleSpeed(int speed);
    void setAIVehicleName(int x, string name);
    void setAIVehicleSectionConfig(int x, string config);
    void setAIVehicleSkin(int x, string skin);
    void setAIRepeatTimes(int times);  
    void setAIMode(int mode);    

    ///@}

    /// @name Camera
    /// @{
	
	/**
	 * Sets the camera's position.
	 * @param position The new position of the camera.
	 */
	void setCameraPosition(vector3 position);
	
	/**
	 * Sets the camera's direction vector.
	 * @param direction A vector representing the direction of the vector.
	 */
	void setCameraDirection(vector3 direction);
	
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
	vector3 getCameraPosition();
	
	/**
	 * Gets the camera's direction.
	 * @return A vector representing the direction of the camera
	 */
	vector3 getCameraDirection();
	
	/** 
	 * Points the camera at a location in worldspace.
	 * @remarks
	 *      This is a helper method to automatically generate the
	 *      direction vector for the camera, based on it's current position
	 *      and the supplied look-at point.
	 * @param targetPoint A vector specifying the look at point.
	*/
	void cameraLookAt(vector3 targetPoint);

    ///@}

    /// @name Race system
    /// @{

	/**
	 * Stops the timer
	 * @see startTimer
	 * @return The time that passed since the timer started
	 */
	float stopTimer();

	/**
	 * Starts a timer (useful for races etc)
	 * @see stopTimer
	 */
	void startTimer();

    ///@}

    /// @name Material helpers
    /// @{

	int setMaterialAmbient(const string materialName, float red, float green, float blue);
	int setMaterialDiffuse(const string materialName, float red, float green, float blue, float alpha);
	int setMaterialSpecular(const string materialName, float red, float green, float blue, float alpha);
	int setMaterialEmissive(const string materialName, float red, float green, float blue);
	
	int setMaterialTextureName(const string materialName, int techniqueNum, int passNum, int textureUnitNum, const string textureName);
	int setMaterialTextureRotate(const string materialName, int techniqueNum, int passNum, int textureUnitNum, float rotation);
	int setMaterialTextureScroll(const string materialName, int techniqueNum, int passNum, int textureUnitNum, float sx, float sy);
	int setMaterialTextureScale(const string materialName, int techniqueNum, int passNum, int textureUnitNum, float u, float v);

    ///@}
    
    /// @name Audio
    /// @{

    array<SoundScriptTemplateClass> getAllSoundScriptTemplates();
    
    /**
    * Retrieves one sound script template by name.
    */
    SoundScriptTemplateClass getSoundScriptTemplate(const string &in name);
    
    /**
    * Diagnostic function, returns all existing sound script instances.
    */
    array<SoundScriptInstanceClass> getAllSoundScriptInstances();
    
    /**
    * @param filename WAV file; This WAV file MUST be mono, and exported as 16-bit PCM with no metadata.
    * @param resource_group_name Leave empty to auto-search all groups (classic behavior).
    */    
    SoundClass createSoundFromResource(const string &in filename, const string &in resource_group_name = string());

    /**
    * @param template_name Name defined in the '.soundscript' file.
    * @param actor_instance_id Values 0 and larger are reserved as actor instance IDs, see `getTruckByNum()`; -1 means undefined, -2 is reserved for terrain objects.
    * @see https://docs.rigsofrods.org/vehicle-creation/fileformat-soundscript
    */
    SoundScriptInstanceClass createSoundScriptInstance(const string &in template_name, int actor_instance_id = -1); 

    ///@}    
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
