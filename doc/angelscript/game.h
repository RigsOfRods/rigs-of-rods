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

/*! @mainpage

	This is the documentation for the scripting interface of Rigs of Rods.

	If you're wondering what arguments to pass to a function or what a function does, then this is where you should be.

	If you cannot find the answer here, then search the forum for your question.
	If that didn't provide an answer either, then ask your question in the Scripting forum:
	http://www.rigsofrods.org/forums/167-Scripting

	Please note that the documentation is work in progress.
*/

/**
 * @brief A general class that will provide you with general functions
 * @note The game object is already created for you, you don't need to create it yourself.
 *       E.g.: you can log a message using game.log("Hi, I'm a message");
 */
class game
{
public:
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
	 * sets the character position
	 * @param vec X, Y and Z coordinate of the position on the terrain
	 */
	void setPersonPosition(vector3 vec);

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
	 * moves the person relative
	 * @param relative_movement X, Y and Z coordinate of the translation
	 */
	void movePerson(vector3 relative_movement);

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
	 * returns the current selected truck, null if in person mode
	 * @remarks returns null if the truck does not exist
	 * @return reference to Beam object that is currently in use
	 */
	BeamClass@ getCurrentTruck();

	/**
	 * returns a truck by index, get max index by calling getNumTrucks
	 * @param truck_number A truck number.
	 * @see getCurrentTruckNumber()
	 * @see getNumTrucks()
	 * @remarks returns null if the truck does not exist
	 * @return reference to Beam object that the selected slot
	 */
	BeamClass@ getTruckByNum(int truck_number);

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
	 * set direction arrow
	 * @param text text to be displayed. "" to hide the text
	 * @param position The position to which the arrow should point.
	 * @see hideDirectionArrow
	 */
	void UpdateDirectionArrow(string text, vector3 position);


	/**
	 * returns the size of the font used by the chat box
	 * @deprecated
	 * @return pixel size of the chat text
	 */
	int getChatFontSize();

	/**
	 * changes the font size of the chat box
	 * @deprecated
	 * @param size font size in pixels
	 */
	void setChatFontSize(int size);
	
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
	 * This method repairs the vehicle in the box
	 */
	void repairVehicle(string instance, string box, bool keepPosition);

	/**
	 * This method removes the vehicle in the box
	 */
	void removeVehicle(string instance, string box);

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
	 * Number of trucks with flag
	 */
	int getNumTrucksByFlag(int flag);

	/**
	 * Checks if Caleum is enabled.
	 * @return true if Caleum is available
	 */
	bool getCaelumAvailable();

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

	/**
	 * Gets a setting
	 * @param str The name of the setting
	 * @return the value of the setting
	 */
	string getSetting(string str);

	/**
	 * Hides the direction arrow
	 * @see UpdateDirectionArrow
	 */
	void hideDirectionArrow();


	int setMaterialAmbient(const string materialName, float red, float green, float blue);
	int setMaterialDiffuse(const string materialName, float red, float green, float blue, float alpha);
	int setMaterialSpecular(const string materialName, float red, float green, float blue, float alpha);
	int setMaterialEmissive(const string materialName, float red, float green, float blue);
	
	int setMaterialTextureName(const string materialName, int techniqueNum, int passNum, int textureUnitNum, const string textureName);
	int setMaterialTextureRotate(const string materialName, int techniqueNum, int passNum, int textureUnitNum, float rotation);
	int setMaterialTextureScroll(const string materialName, int techniqueNum, int passNum, int textureUnitNum, float sx, float sy);
	int setMaterialTextureScale(const string materialName, int techniqueNum, int passNum, int textureUnitNum, float u, float v);

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
	 * Gets the currently loaded terrain
	 * @param result This string will contain the name of the terrain after you called this function
	 * @return 0 on success
	 */
	int getLoadedTerrain(string result);

	/**
	 * Returns the current position of the person
	 * @return A vector containing the X, Y and Z coordinate of the person or an empty vector if the user is in a truck
	 */
	vector3 getPersonPosition();

	/**
	 * Clears the event cache
	 */
	void clearEventCache();
	
	/**
	 * Gives the currently used truck a boost in RPM.
	 * @param factor This factor determines by how much that the RPM of the truck will be increased ( rpm += 2000.0f * factor ).
	 */
	void boostCurrentTruck(float factor);
	
	/**
	 * Adds a global function to the script.
	 * @param func the function to be added, e.g.: "void func() { log('works'); }"
	 */
	int addScriptFunction(const string func);
	
	/**
	 * Checks if a global function exists
	 * @param func the declaration of the function that should be checked for existance, e.g.: "void func()"
	 */
	int scriptFunctionExists(const string func);
	
	/**
	 * Removes a global function from the script.
	 * @param func the declaration of the function that should be removed, e.g.: "void func()"
	 */
	int deleteScriptFunction(const string func);
	
	/**
	 * Adds a global variable to the script.
	 * @param var the declaration of the variable that should be added, e.g.: "int missionState;"
	 */
	int addScriptVariable(const string var);
	
	/**
	 * Removes a global variable from the script.
	 * @param var the declaration of the variable that should be removed, e.g.: "int missionState;"
	 */
	int deleteScriptVariable(const string var);

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
	 *  Spawns a truck by filename
	 *  
	 *  @param truckName The filename of the truck
	 *  @param pos The position where the truck should be spawned
	 *  @param rot The rotation in which the truck should be spawned
     *  @param skinName Designated name of the SkinZIP to use. Ignored if not found.
	 *  @return reference to Beam object
	 */
	BeamClass @spawnTruck(string truckName, vector3 pos, vector3 rot, string skinName);

	/**
	*  Gets the Curent frames per second (FPS)
	*  @return The Current FPS
	*/
	void getFPS();

	/**
	*  Back to menu
	*/
	void backToMenu();

	/**
	*  Quits the game
	*/
	void quitGame();
};
