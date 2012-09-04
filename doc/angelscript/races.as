/** @file
 * The race manager
 * All functions needed to organize a race are in this file.
 * @author neorej16
 */

/**
 * A function signature for the callback pointers.
 * When you need to use a RACE_EVENT_CALLBACK as parameter, then
 * you should create a function that receives a dictionary@ as parameter.
 * Example:
 * @code // example
 *		void myCallback(dictionary@ d)
 *		{
 *			// ...
 *		}
 * @endcode
 */
funcdef void RACE_EVENT_CALLBACK(dictionary@);

/**
 * called when the user drives through a checkpoint
 * @remarks Do not call this function directly!
 */
void raceEvent(int trigger_type, string inst, string box, int nodeid);

/**
 * called when the user drives through a cancel point
 * @remarks Do not call this function directly!
 */
void raceCancelPointHandler(int trigger_type, string inst, string box, int nodeid);


/**
 * This class allows you to organize races.
 * This class will handle all the race logic for you.
 * @attention
 *     To use this class, you'll need to add the following code
 *     to your scripting file:
 *		\code
 *          #include "races.as"
 *          raceManager races()
 *		\endcode
 */
class racesManager {
public:
	int raceCount;			//!< the raceID+1 of the last race.
	int currentRace;		//!< the ID of the current race. (-1 is there's no race running at the moment)
	int currentLap;			//!< the number of the current lap. (starts from 0)
	int truckNum;			//!< The number of the (last) used truck in the race.
	int lastCheckpoint;		//!< The number of the last passed checkpoint.

	bool obligatedFinish;	//!< if true: if you drive through the start checkpoint of another race, while racing, it will be ignored. (default=false)
	bool showTimeDiff;		//!< if true: Show + or - [best time minus current time] when passing a checkpoint. (default=true)
	bool showBestLap;		//!< if true: If a race is started or a new best lap is set, the best lap will be shown. (default=true)
	bool showBestRace;		//!< if true: If a race is started or a new best race is set, the best race will be shown. (default=true)
	bool submitScore;		//!< if true: If the user has a new best lap or new best race, this is submitted to the master server. (default=true)
	bool showCheckPointInfoWhenNotInRace; //!< if true: if the user drives through a checkpoint of a race that isn't running, a message will be shown, saying "this is checkpoint xx of race myRaceName". (default=false)
	bool silentMode;			//!< if true: No flashmessages will be shown. (default=false)
	bool allowVehicleChanging;	//!< if false: if the user changes vehicle, the race will be aborted. (default=false)
	bool abortOnVehicleExit;	//!< if true: if the user exits his vehicle, the race will be aborted. (default=false)
	int arrowMethod;			//!< What checkpoint instance should the directional arrow point to. (default=ARROW_AUTO=point to the same instance as last checkpoint)

	bool penaltyGiven;
	int actionOnTruckExit;
	int state;					//!< In which state are we? \see STATE_NotInRace, STATE_Waiting, STATE_Racing
	int cancelPointCount;
	double raceStartTime;
	double lapStartTime;
	string lastCheckpointInstance;
	string lastRaceEventInstance;
	double lastRaceEventTime;
	string raceManagerVersion;	//!< the version of the raceManager
	
	
// public constants
	int LAPS_Unlimited;		//!< Keep looping the race for an unlimited time.
	int LAPS_NoLaps;		//!< This race has a seperate start line and finish line.
	int LAPS_One;			//!< This race consist of 1 lap, and the start and finish line are the same object.
	
	int ACTION_DoNothing;	//!< Not used
	int ACTION_SuspendRace; //!< Not used
	int ACTION_StopRace;    //!< Not used
	int ACTION_RestartRace; //!< Not used
	
	int STATE_NotInRace;	//!< We're not racing at the moment
	int STATE_Waiting;		//!< A race is suspended, and we're waiting until the race continues
	int STATE_Racing;		//!< We're in the middle of a race :D.
	
	int ARROW_AUTO;			//!< This arrow method will always point to the checkpoint instance that comes closest to the previous one.
		
private:
	private raceBuilder[] raceList;
	dictionary callbacks;

private:

	/**
	 * This will get called when a truck is at a checkpoint
	 * You shouldn't call this manually (use the "Checkpoint" callback instead)
	 * @see setCallback
	 **/
	void raceEvent(int trigger_type, string inst, string box, int nodeid);
	
	/**
	 * Formats the time
	 * @param seconds An amount of seconds
	 * @return The time, formatted as: "X minutes and X seconds"
	 **/
	string formatTime(double seconds);

	/**
	 * Finishes a race
	 * @see cancelCurrentRace
	 **/
	void finishCurrentRace();
	
	/**
	 * Advances a lap
	 **/
	void advanceLap();

	void addLapTime(int raceID, double time, bool &out newBestLap);
	
	void addRaceTime(int raceID, double time, bool &out newBestRace);

	/**
	 * event handler for cancel points
	 */
	void raceCancelPointHandler(int trigger_type, string inst, string box, int nodeid);
	
public:
	
	/**
	 * The constructor
	 * This initializes everything.
	 */
	racesManager();
	
	/**
	 * Adds a race to the terrain.
	 * @param raceName The name of the race. This needs to be unique for your terrain, and shouldn't contain points.
	 * @param checkpoints An array of coordinates ( a coordinate exists out of 6 numbers: { position.x, position.y, position.z, rotation.x, rotation.y, rotation.z }
	 * @param laps The amount of laps. This can be one of the following: LAPS_Unlimited, LAPS_NoLaps, LAPS_One or another number representing the amount of laps (>=1)
	 * @param objName_checkpoint The name of the object that should be used as checkpoint
	 * @param objName_start The name of the object that should be used as start line
	 * @param objName_finish The name of the object that should be used as finish
	 * @return The unique ID of the race.
	 * @code // example
	 *		double[][] checkpoints = {
	 *			{5980.50, 0, 5999.89, 0, 90, 0}, // this is checkpoint 0 (this is the start line)
	 *			{5959.52, 0, 6000.18, 0, 90, 0}, // this is checkpoint 1
	 *			{5912.66, 0, 6000.06, 0, 90, 0}, // this is checkpoint 2
	 *			{5871.04, 0, 6008.94, 0, 90, 0}, // this is checkpoint 3
	 *			{5826.71, 0, 6009.09, 0, 90, 0}, // this is checkpoint 4
	 *			{5759.27, 0, 6000.11, 0, 90, 0}, // this is checkpoint 5
	 *			{5700.36, 0, 6009.43, 0, 90, 0}, // this is checkpoint 6
	 *			{5664.99, 0, 6023.90, 0, 90, 0}, // this is checkpoint 7
	 *			{5620.49, 0, 6008.88, 0, 90, 0}, // this is checkpoint 8
	 *			{5586.43, 0, 5999.97, 0, 90, 0}, // this is checkpoint 9
	 *			{5539.42, 0, 5999.96, 0, 90, 0}  // this is checkpoint 10 (this is the finish line)
	 *		}
	 *		int myRaceID = races.addRace("Example Race", checkpoints, races.LAPS_NoLaps, "chp-checkpoint", "chp-start", "chp-start");
	 * @endcode
	 */
	int addRace(string raceName, double[][] checkpoints, int laps, string objName_checkpoint, string objName_start, string objName_finish);

	/**
	 * Add a race to the terrain
	 * @param raceName The name of the race. This needs to be unique for your terrain, and shouldn't contain points.
	 * @param checkpoints An array of coordinates ( a coordinate exists out of 6 numbers: { position.x, position.y, position.z, rotation.x, rotation.y, rotation.z }
	 * @param laps The amount of laps. This can be one of the following: LAPS_Unlimited, LAPS_NoLaps, LAPS_One or another number representing the amount of laps (>=1)
	 * @param objName_checkpoint The name of the object that should be used as checkpoint
	 * @param objName_start_finish The name of the object that should be used as start line and as finish line
	 * @return The unique ID of the race.
	 */
	int addRace(string raceName, double[][] checkpoints, int laps, string objName_checkpoint, string objName_start_finish);

	/**
	 * Add a race to the terrain
	 * @param raceName The name of the race. This needs to be unique for your terrain, and shouldn't contain points.
	 * @param checkpoints An array of coordinates ( a coordinate exists out of 6 numbers: { position.x, position.y, position.z, rotation.x, rotation.y, rotation.z }
	 * @param laps The amount of laps. This can be one of the following: LAPS_Unlimited, LAPS_NoLaps, LAPS_One or another number representing the amount of laps (>=1)
	 * @param objName_checkpoint The name of the object that should be used as checkpoint, start line and finish line
	 * @return The unique ID of the race.
	 */
	int addRace(string raceName, double[][] checkpoints, int laps, string objName_checkpoint);

	/**
	 * Add a race to the terrain, using the default checkpoint objects
	 * @param raceName The name of the race. This needs to be unique for your terrain, and shouldn't contain points.
	 * @param checkpoints An array of coordinates ( a coordinate exists out of 6 numbers: { position.x, position.y, position.z, rotation.x, rotation.y, rotation.z }
	 * @param laps The amount of laps. This can be one of the following: LAPS_Unlimited, LAPS_NoLaps, LAPS_One or another number representing the amount of laps (>=1)
	 * @return The unique ID of the race.
	 */
	int addRace(string raceName, double[][] checkpoints, int laps);

	/**
	 * Add a not-looping race to the terrain, using the default checkpoint objects
	 * @param raceName The name of the race. This needs to be unique for your terrain, and shouldn't contain points.
	 * @param checkpoints An array of coordinates ( a coordinate exists out of 6 numbers: { position.x, position.y, position.z, rotation.x, rotation.y, rotation.z }
	 * @return The unique ID of the race.
	 */
	int addRace(string raceName, double[][] checkpoints);

	/**
	 * Add a new empty race (advanced users only!)
	 * @return The unique ID of the race.
	 * @code // example
	 *		double[][] checkpoints = {
	 *			{5980.50, 0, 5999.89, 0, 90, 0}, // this is checkpoint 0 (this is the start line)
	 *			{5959.52, 0, 6000.18, 0, 90, 0}, // this is checkpoint 1
	 *			{5912.66, 0, 6000.06, 0, 90, 0}, // this is checkpoint 2
	 *			{5871.04, 0, 6008.94, 0, 90, 0}, // this is checkpoint 3
	 *			{5826.71, 0, 6009.09, 0, 90, 0}, // this is checkpoint 4
	 *			{5759.27, 0, 6000.11, 0, 90, 0}, // this is checkpoint 5
	 *			{5700.36, 0, 6009.43, 0, 90, 0}, // this is checkpoint 6
	 *			{5664.99, 0, 6023.90, 0, 90, 0}, // this is checkpoint 7
	 *			{5620.49, 0, 6008.88, 0, 90, 0}, // this is checkpoint 8
	 *			{5586.43, 0, 5999.97, 0, 90, 0}, // this is checkpoint 9
	 *			{5539.42, 0, 5999.96, 0, 90, 0}  // this is checkpoint 10 (this is the finish line)
	 *		}
	 *		int myRaceID = races.addNewEmptyRace();
	 *		races.setRaceName(myRaceID, "my advanced race");
	 *		races.setLaps(myRaceID, races.LAPS_NoLaps);
	 *		races.addCheckpointList(myRaceID, 0, "chp-checkpoint", checkpoints);
	 *		races.setVersion(myRaceID, "author-v1.0");
	 *		races.finalize();
	 * @endcode
	 */
	int addNewEmptyRace();
	
	/**
	 * Creates a new free race ID
	 * @attention Internal or extremely advanced use only
	 * @warning
	 *     This does not only return a new race ID,
	 *     it may also increment the race count, so only
	 *     call this function when you're about to add a
	 *     new race, without using the built in functions
	 *     to add a race
	 * @return The unique ID of the race.
	 */
	int getNewRaceID();
	
	/**
	 * Adds a callback funciton
	 * @param event
	 *      The name of the event.
	 *      This can only be one of the following:
	 *          - RaceFinish  -> called when a race was finished (not aborted before finish)
	 *          - RaceCancel  -> called when a race was aborted (not when the user finished successfully)
	 *          - RaceStart   -> called when a race was started
	 *          - AdvanceLap  -> called when the user passes the finishline and still has more laps to do
	 *          - Checkpoint  -> called when the user passes a checkpoint that is not the start or finish line
	 *          - NewBestLap  -> called when the user sets a personal new best lap time record
	 *          - NewBestRace -> called when the user sets a personal new best race time record
	 *          - LockedRace  -> called when the user passes the startline of a locked race
	 *          - RaceEvent   -> called when the user passes through a checkpoint (including start of finish lines)
	 *                           (this is called before the processing is done, you can set "break" to false to ignore the event)
	 *          - PenaltyEvent-> called when the user drives through a penalty event box
	 *          - AbortEvent  -> called when the user drives through an abort race event box
	 * @param func A reference to the function
	 */
	void setCallback(string event, RACE_EVENT_CALLBACK @func);

	/**
	 * Gets a callback funciton
	 * @param event The name of the event. \see setCallback
	 * @return A reference to the function
	 */
	RACE_EVENT_CALLBACK@ getCallback(string event);

	/**
	 * Remove a callback function
	 * @param event The name of the event. \see setCallback
	 **/
	void removeCallback(string event);
	
	/**
	 * Starts a race
	 * @param raceID the ID of the race
	 **/
	void startRace(int raceID);
	
	/**
	 * Advances a checkpoint
	 * @param raceID the ID of the race
	 **/
	void advanceCheckpoint(int raceID);

	/**
	 * Sets a best lap time
	 * @param raceID the ID of the race
	 * @param time the lap time
	 **/
	void setBestLapTime(int raceID, double time);

	/**
	 * Sets a best race time
	 * @param raceID the ID of the race
	 * @param time the race time
	 **/
	void setBestRaceTime(int raceID, double time);
	
	/**
	 * This should be called from within your RoR eventCallback function
	 * @param eventnum the number of the event \see enum scriptEvents
	 * @param value the value of the event
	 **/
	void eventCallback(int eventnum, int value);
	
	/**
	 * Unlocks a race
	 * @param raceID the ID of the race
	 **/
	void unlockRace(int raceID);

	/**
	 * Locks a race
	 * @param raceID the ID of the race
	 **/
	void lockRace(int raceID);
	
	/**
	 * Deletes a race
	 * @param raceID the ID of the race
	 **/
	void deleteRace(int raceID);
	
	/**
	 * Returns the race ID of a race name
	 * @param raceName_in the race name
	 * @return the ID of the race
	 **/
	int getRaceIDbyName(string raceName_in);
	
	/**
	 * Aborts the current race
	 **/
	void cancelCurrentRace();
	
	/**
	 * Is a certain race completed?
	 * @param raceID the ID of the race
	 * @return true if the race has been completed before, else, false
	 **/
	bool raceCompleted(int raceID);

	/**
	 * Gets the ID of the current race
	 * @return the ID of the race
	 **/
	int getCurrentRaceID();
	
	/**
	 * Gets the best lap time of a certain race
	 * @param raceID the ID of the race
	 * @return the time in seconds
	 **/
	double getBestLapTime(int raceID);
	
	/**
	 * Gets the best race time of a certain race
	 * @param raceID the ID of the race
	 * @return the time in seconds
	 **/
	double getBestRaceTime(int raceID);
	
	/**
	 * Add an amount of penalty time to the current race
	 * @param seconds the amount of seconds to add to the time
	 * @return true on success
	 **/
	bool addPenaltySeconds(int seconds);
	
	/**
	 * Sets the amount of laps for a race
	 * @param raceID the ID of the race
	 * @param laps The amount of laps
	 **/
	void setLaps(int raceID, int laps);
	
	/**
	 * Sets the name for a race
	 * @param raceID the ID of the race
	 * @param raceName The name of the race (needs to be unique, and shouldn't contain points)
	 **/
	void setRaceName(int raceID, string raceName);
	
	/**
	 * Gets the amount of laps for a race
	 * @param raceID the ID of the race
	 * @return The amount of laps
	 **/
	int getLaps(int raceID);
	
	/**
	 * Gets the raceBuilder object of a race
	 * @param raceID the ID of the race
	 * @return a reference to the raceBuilder object for that race
	 * @see raceBuilder
	 **/
	raceBuilder@ getRace(int raceID);

	/**
	 * Adds a checkpoint to a race.
	 * This allows you to have duplicate checkpoints (useful to create shortcuts or splitted tracks).
	 * @param raceID the ID of the race
	 * @param number The number of the checkpoint in the race.
	 *           0 is the start line
	 *           1 would be the first checkpoint
	 *           2 is the second checkpoint
	 *           etc.
	 *           You can have multiple checkpoints for the same number.
	 *           The user will then be able to choose which checkpoint he takes.
	 * @param objName the name of the object that should be used for the checkpoint
	 * @param coords the coordinate for the race (6 numbers! \see addRace)
	 * @see addCheckpointList
	 **/
	void addCheckpoint(int raceID, int number, string objName, double[] coords);
	
	/**
	 * Adds a list of checkpoints to a race.
	 * This allows you to have duplicate checkpoints (useful to create shortcuts or splitted tracks).
	 * @param raceID the ID of the race
	 * @param startNumber The number of the first checkpoint that should be added
	 *           The first checkpoint in the coords list, will be added as number startNumber.
	 *           The second checkpoint in the coords list, will be added as startNumber+1
	 *           etc.
	 *           You can have multiple checkpoints for the same number.
	 *           The user will then be able to choose which checkpoint he takes.
	 * @param objName the name of the object that should be used for the checkpoint
	 * @param coords a list of coordinates for the race (6 numbers per coordinate \see addRace)
	 **/
	void addCheckpointList(int raceID, uint startNumber, string objName, double[][] coords);

	/**
	 * This deletes a checkpoint from a race.
	 * @warning
	 *     If you delete a checkpoint in the middle of your race, it will become impossible to complete the race.
	 *     (so make sure you add a new checkpoint if you do that)
	 * @param raceID the ID of the race
	 * @param number The number of the checkpoint in the race.
	 *           0 is the start line
	 *           1 would be the first checkpoint
	 *           2 is the second checkpoint
	 *           etc.
	 * @param instance which instance of this checkpoint should be deleted?
	 *      (you can have multiple checkpoints with the same number. The first
	 *      added checkpoint for a certain number will have 0 as instance number,
	 *      the second added checkpoint for a certain number will have 1 as instance.)
	 */
	void deleteCheckPoint(int raceID, int number, int instance);

	/**
	 * This allows you to add an object with event boxes that will cause the race to be aborted.
	 * @param raceID the ID of the race
	 * @param objName The name of the object (~the name of the odef file, but without the .odef extension)
	 * @param v The position where the cancel point should be
	 * @param callback A callback function that will be called when the race is canceled because of this cancel point.
	 *         \see RACE_EVENT_CALLBACK
	 */
	void addCancelPoint(int raceID, string objName, double[] v, RACE_EVENT_CALLBACK @callback);
	
	/**
	 * Recalculates where the arrow should point to.
	 * Useful if you changed the arrowMethod variable or if you changed the position of the next checkpoint.
	 */
	void recalcArrow();

	/**
	 * Make the directional arrow point to a certain checkpoint of the current race
	 * @param position The number of the checkpoint to which it should point.
	 *      The start line is position 0, the first checkpoint is position 1, etc.
	 *      It always points to a checkpoint of the current race
	 */
	void setupArrow(int position);
	
	/**
	 * This hides the directional arrow
	 */
	void removeArrow();
	
	/**
	 * This sets the penalty time of a race
	 * @param raceID the ID of the race
	 * @param seconds how much penalty time should be set? (in seconds)
	 */
	void setPenaltyTime(int raceID, int seconds);
	
	/**
	 * This returns the penalty time setting of a race
	 * @param raceID the ID of the race
	 * @return penalty time in seconds
	 */
	int getPenaltyTime(int raceID);
	
	/**
	 * This sets the version of your race.
	 * @param raceID the ID of the race
	 * @param version The version of the race in the format 'username-v1.0' in which
	*        you replace 'username' by your username and '1.0' by the correct version number.
	 */
	void setVersion(int raceID, string version);
	
	/**
	 * This hides your race
	 * @see unhideRace
	 * @param raceID the ID of the race
	 */
	void hideRace(int raceID);
	
	/**
	 * This shows your race again, after it was hidden
	 * @param raceID the ID of the race
	 */
	void unhideRace(int raceID);
	
	/**
	 * This allows you to change the startNumber of a race.
	 * By default, the race is started by driving through checkpoint 0, and,
	 * in a race with multiple laps, after passing the last checkpoint, the
	 * player will have to drive to checkpoint 0 again.
	 * You can change this here (useful to make small loops in a bigger race).
	 * @param raceID the ID of the race
	 * @param startNum The number of the checkpoint that should be used as start point.
	 *      Normally, the start line is position 0, the first checkpoint is position 1, etc.
	 */
	void setStartNumber(int raceID, int startNum);
	
	/**
	 * Resets the race event callback.
	 * If the user drives through a checkpoint, then the script will only
	 * handle the event once, event if the user is in the event box
	 * for multiple frames.
	 * Sometimes, you may want to reset this, and then, you can call this function.
	 */
	void resetEventCallback();
	
	/**
	 * Finalizes your (advanced) race.
	 */
	void finalize(int raceID);

	/**
	 * Saves all races.
	 */
	void saveRaces();
	
	/**
	 * Saves a race.
	 * @param raceID the ID of the race
	 */
	void saveRace(int raceID);

	/**
	 * Loads a race.
	 * @param raceID the ID of the race
	 */
	void loadRace(int raceID);
}
