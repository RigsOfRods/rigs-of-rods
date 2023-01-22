/*
Hi,

You've found your way to the race system of Rigs of Rods.

If you are looking for more information on how to use this
class or what arguments should be given to certain functions,
please visit http://docs.rigsofrods.org/

If you are looking for examples on how to use this class,
please look at the scripting files of other terrains or
search the forum.

If you have questions that cannot be answerred using the
methods mentioned above, then please ask your question in
the scripting section of the Rigs of Rods forum:
http://www.rigsofrods.org/forums/167-Scripting

This class was designed to be as simple as possible for
normal users, while being as flexible as possible for
advanced users.
If you're here because this class doesn't provide the
functionality that you need for your terrain, then try
to improve the class and submit your additions using
the issue tracker of Rigs of Rods.
http://redmine.rigsofrods.org

Don't forget to increase the version numbers after every
edit! (racesManager::raceManagerVersion and
raceBuilder::raceBuilderVersion)

-- neorej16
*/

// Define a function signature for the callback pointers
funcdef void RACE_EVENT_CALLBACK(dictionary@);

// called when a vehicle is in a checkpoint
void raceEvent(int trigger_type, string inst, string box, int nodeid) {
	races.raceEvent(trigger_type, inst, box, nodeid);
}

void raceCancelPointHandler(int trigger_type, string inst, string box, int nodeid) {
	races.raceCancelPointHandler(trigger_type, inst, box, nodeid);
}

// This class will handle the race logic for you
// this class shouldn't be edited!
class racesManager {
// public properties
	int raceCount;
	int currentRace;
	int currentLap;
	int truckNum;
	int lastCheckpoint;
	bool obligatedFinish;
	bool showTimeDiff;
	bool showBestLap;
	bool showBestRace;
	bool submitScore;
	bool showCheckPointInfoWhenNotInRace;
	bool silentMode;
	bool allowVehicleChanging;
	bool abortOnVehicleExit;
	bool restartRaceOnStart;
	bool penaltyGiven;
	int actionOnTruckExit;
	int state;
	int cancelPointCount;
	double raceStartTime;
	double lapStartTime;
	string lastCheckpointInstance;
	string lastRaceEventInstance;
	double lastRaceEventTime;
	string raceManagerVersion;
	int arrowMethod;
	LocalStorage raceDataFile;
	array<int> penaltyTime;
	
// public constants
	int LAPS_Unlimited;
	int LAPS_NoLaps;
	int LAPS_One;
	
	int ACTION_DoNothing;
	int ACTION_SuspendRace;
	int ACTION_StopRace;
	int ACTION_RestartRace;
	
	int STATE_NotInRace;
	int STATE_Waiting;
	int STATE_Racing;
	
	int ARROW_AUTO;
		
// private properties
	array<raceBuilder@> raceList;
	dictionary callbacks;
	
// public functions
	
	// constructor
	racesManager() {
	
		// We initialize our "constants"
		this.LAPS_Unlimited = -1;
		this.LAPS_NoLaps = 0;
		this.LAPS_One = 1;
		
		this.ACTION_DoNothing   = 0;
		this.ACTION_SuspendRace = 1;
		this.ACTION_StopRace    = 2;
		this.ACTION_RestartRace = 3;
		
		this.STATE_NotInRace = 0;
		this.STATE_Waiting   = 1;
		this.STATE_Racing    = 2;
		
		this.ARROW_AUTO = -1;
				
		// we initialize the callbacks dictionary
		this.callbacks.set("RaceFinish", null); // when a race was finished
		this.callbacks.set("RaceCancel", null); // when a race was canceled
		this.callbacks.set("RaceStart",  null); // when a race starts
		this.callbacks.set("AdvanceLap", null); // when a lap is done, but not when the race is done
		this.callbacks.set("Checkpoint", null); // when a checkpoint is taken (excluding start and finish)
		this.callbacks.set("NewBestLap", null); // When a new best lap time is set
		this.callbacks.set("NewBestRace", null);// When a new best race time is set
		this.callbacks.set("LockedRace", null); // When the user passes the start line of a locked race
		this.callbacks.set("RaceEvent", null); // When the user passes the start line of a locked race
		this.callbacks.set("PenaltyEvent", null); // When the user gets in a race_penalty box, handled by the raceEvent method
		this.callbacks.set("AbortEvent", null); // When the user gets in a race_abort box, handled by the raceEvent method

		// initialize the default settings
		this.obligatedFinish   = false; // if true: if you drive through the start checkpoint of another race, while racing, it will be ignored
		this.showTimeDiff      = true;  // if true: Show + or - <best time minus current time> when passing a checkpoint.
		this.showBestLap       = true;  // if true: If a race is started or a new best lap is set, the best lap will be shown
		this.showBestRace      = true;  // if true: If a race is started or a new best race is set, the best race will be shown
		this.submitScore       = true; // if true: If the user has a new best lap or new best race, this is submitted to the master server.
		this.silentMode        = false; // if true: No messages will be shown
		this.allowVehicleChanging = false; // if false: if the user changes vehicle, the race will be aborted.
		this.abortOnVehicleExit = false;  // if true: if the user exits his vehicle, the race will be aborted
		this.showCheckPointInfoWhenNotInRace = false; // if true: if the user drives through a checkpoint of a race that isn't running, a message will be shown, saying "this is checkpoint xx of race myRaceName"
		this.arrowMethod       = this.ARROW_AUTO;
		this.restartRaceOnStart = true; // if true: the race will be restarted when you pass the start line of the same race
		
		// we initialize the other variables (do not edit these manually)
		this.state           = this.STATE_NotInRace;
		this.raceCount       = 0;
		this.currentRace     = -1;
		this.lastCheckpoint  = -1;
		this.raceStartTime   = 0.0;
		this.lapStartTime    = 0.0;
		this.cancelPointCount= 0;
		this.lastCheckpointInstance = "";
		this.lastRaceEventInstance = ""; // we only use this to boost the FPS
		this.raceManagerVersion = "RoR_raceManager_v0.02";
		this.penaltyGiven = true;
		
		// register the required callbacks
		game.registerForEvent(SE_TRUCK_ENTER);
		game.registerForEvent(SE_TRUCK_EXIT);
		game.registerForEvent(SE_TRUCK_RESET);
		game.registerForEvent(SE_TRUCK_TELEPORT);
        game.registerForEvent(SE_TRUCK_MOUSE_GRAB);
		game.registerForEvent(SE_GENERIC_DELETED_TRUCK);
		game.registerForEvent(SE_ANGELSCRIPT_MANIPULATIONS);
		
		// add the eventcallback method if it doesn't exist
		if(game.scriptFunctionExists("void eventCallback(int, int)")<0)
			game.addScriptFunction("void eventCallback(int key, int value) { races.eventCallback(key, value); }");
			
		
		// Load the file containing the race data
		this.raceDataFile = LocalStorage("raceTimes");
	}

	// add a race
	//  pre: nothing
	// post: A new race was added and built
	int addRace(const string &in raceName, const double[][] &in checkpoints, int laps = 0, const string &in objName_checkpoint = "chp-checkpoint", const string &in objName_start = "chp-start", const string &in objName_finish = "chp-start", const string &in version = "unknown")
	{
		// debug: game.log("racesManager::addRace(\"" + raceName + "\", \"" + objName_start + "\", \"" + objName_checkpoint + "\", \"" + objName_finish + "\", checkpoints, loop) called.");
				
		// check input
		if( checkpoints.length() < 2 )
		{
			game.log("Error in racesManager::addRace: A race should have at least 2 checkpoints.");
			return -1;
		}
		
		int raceID = this.getNewRaceID();
		this.raceList.resize(this.raceCount);
		@this.raceList[raceID] = @raceBuilder(raceID);
		this.raceList[raceID].raceName = raceName;
		this.raceList[raceID].setLaps(laps);
		this.raceList[raceID].addChpCoordinates(checkpoints, objName_checkpoint, objName_start, objName_finish, 0);
		this.setVersion(raceID, version);
		this.raceList[raceID].loadRace(@this.raceDataFile);
		return raceID;
	}
	
	// Advanced users only
	int addNewEmptyRace()
	{
		int raceID = this.getNewRaceID();
		this.raceList.resize(this.raceCount);
		@this.raceList[raceID] = @raceBuilder(raceID);
		return raceID;
	}
	
	int getNewRaceID()
	{
		// try to recycle a race ID
		for( int i = 0; i < this.raceCount; i++ )
		{
			if( this.raceList[i].awaitingRecycling )
				return i;
		}
		return this.raceCount++;
	}
	
	//  pre: nothing
	// post: A callback function has been registered
	void setCallback(const string &in event, RACE_EVENT_CALLBACK @func)
	{
		if( not callbacks.exists(event) )
			game.log("Error in racesManager::setCallback: Event '" + event + "' does not exist.");
		callbacks.set(event, @func);
	}

	// I don't immediately see a use for this function, but maybe someone will come up with a use for it
	RACE_EVENT_CALLBACK@ getCallback(const string &in event)
	{
		RACE_EVENT_CALLBACK @func;
		if( callbacks.get(event, @func) )
			return @func;
		return null;
	}

	//  pre: nothing
	// post: The callback unregistered
	void removeCallback(const string &in event)
	{
		if( not callbacks.exists(event) )
			game.log("Error in racesManager::removeCallback: Event '" + event + "' does not exist.");
		callbacks.set(event, null);
	}
	
	// This will get called when a truck is at a checkpoint
	// You shouldn't call this manually (use the callback instead)
	void raceEvent(int trigger_type, string inst, const string &in box, int nodeid)
	{
		// debug: game.log("racesManager::raceEvent(" + trigger_type + ", \"" + inst + "\", \"" + box + "\", " + nodeid + ") called.");
		
		if( box == "race_penalty" and !this.penaltyGiven and this.state == this.STATE_Racing )
		{
			// the inst string contains the information about the event
			array<string>@ tmp = inst.split("|");
			if( tmp.length() >= 3 )
			{
				int checkpointNum = parseInt(tmp[2]);
				int raceID        = parseInt(tmp[1]);
				if( raceID == this.currentRace or raceID == -1 )
				{
					int penaltyTime = this.getPenaltyTime(raceID);
					
					// call the callback function
					RACE_EVENT_CALLBACK @handle;
					if( callbacks.get("PenaltyEvent", @handle) and (handle !is null))
					{
						dictionary args;
						args.set("event", "PenaltyEvent");
						args.set("raceID", raceID);
						args.set("checkpointNum", checkpointNum);
						args.set("trigger_type", trigger_type);
						args.set("inst", ""+inst);
						args.set("box", ""+box);
						args.set("nodeid", nodeid);
						args.set("penaltyTime", penaltyTime);
						handle(@args);
						args.get("penaltyTime", penaltyTime);
					}
					this.addPenaltySeconds(penaltyTime);
					this.penaltyGiven = true;
				}
			}
			return;
		}		
		else if( box == "race_abort" and this.state == this.STATE_Racing )
		{
			// the inst string contains the information about the event
			array<string>@ tmp = inst.split("|");
			if( tmp.length() >= 3 )
			{
				int checkpointNum = parseInt(tmp[2]);
				int raceID        = parseInt(tmp[1]);
				if( raceID == this.currentRace or raceID == -1 )
				{
				
					// call the callback function
					RACE_EVENT_CALLBACK @handle;
					if( callbacks.get("AbortEvent", @handle) and not (handle is null))
					{
						dictionary args;
						args.set("event", "AbortEvent");
						args.set("raceID", raceID);
						args.set("checkpointNum", checkpointNum);
						args.set("trigger_type", trigger_type);
						args.set("inst", ""+inst);
						args.set("box", ""+box);
						args.set("nodeid", nodeid);
						args.set("break", false);
						handle(@args);
						bool result = false;
						args.get("break", result);
						if( result )
							return;
					}
				
					this.cancelCurrentRace();
				}
			}
			return;
		}
		
		// We don't want to handle the same checkpoint twice
		if( ( inst == this.lastRaceEventInstance ) )
			return;
		this.lastRaceEventInstance = inst;
		
		// call the callback function
		RACE_EVENT_CALLBACK @handle;
		if( callbacks.get("RaceEvent", @handle) and not (handle is null))
		{
			dictionary args;
			args.set("event", "RaceEvent");
			args.set("raceID", -1);
			args.set("trigger_type", trigger_type);
			args.set("inst", ""+inst);
			args.set("box", ""+box);
			args.set("nodeid", nodeid);
			args.set("break", false);
			handle(@args);
			bool result = false;
			args.get("break", result);
			if( result )
				return;
		}
		
		// the inst string contains the information about the checkpoint
		array<string>@ tmp = inst.split("|");
		if( tmp.length() >= 3 and (tmp[0] == "checkpoint") )
		{
			int checkpointNum = parseInt(tmp[2]);
			int raceID        = parseInt(tmp[1]);
		
			if( checkpointNum == this.lastCheckpoint )
				return;
			else if(this.state == STATE_NotInRace)
			{
				// we're not racing, but maybe we passed the start line?
				if(checkpointNum == 0)
				{
					// yes! We passed the start line, so we'll start the race!
					this.lastCheckpointInstance = inst;
					this.penaltyGiven = false;
					this.startRace(raceID);
				}
				else if( this.showCheckPointInfoWhenNotInRace )
				{
					// passed some not-start checkpoint
					this.message("This is checkpoint "+checkpointNum+" of race "+this.raceList[raceID].raceName+"!", "tick.png");
				}
			}
			else if(this.state == STATE_Racing and currentRace == raceID)
			{
				// we hit a checkpoint from the same race!		
				if( checkpointNum == this.raceList[raceID].finishNum and this.raceList[raceID].finishNum == this.raceList[raceID].getNextCheckpointNum(this.lastCheckpoint))
				{ // passing the finishline
					if( (this.currentLap < this.raceList[raceID].laps) or (this.raceList[raceID].laps == this.LAPS_Unlimited) )
					{
						this.lastCheckpointInstance = inst;
						this.penaltyGiven = false;
						this.advanceLap();
					}
					else if( (this.currentLap >= this.raceList[raceID].laps) or (this.raceList[raceID].laps == this.LAPS_NoLaps) )
					{
						this.lastCheckpointInstance = inst;
						this.penaltyGiven = false;
						this.finishCurrentRace();
					}
					else
						game.log("ERROR: unhandled race event: checkpointNum "+checkpointNum+", finishNum "+this.raceList[raceID].finishNum);
				}
				else if( checkpointNum == this.raceList[raceID].getNextCheckpointNum(this.lastCheckpoint) )
				{ // passing a normal checkpoint
					this.lastCheckpointInstance = inst;
					this.penaltyGiven = false;
					this.advanceCheckpoint(raceID);
				}
				else{
					if( (checkpointNum == 0) && this.restartRaceOnStart )
					{
						this.cancelCurrentRace();
						this.lastCheckpointInstance = inst;
						this.penaltyGiven = false;
						this.startRace(raceID);
					}
					else if( checkpointNum == this.raceList[raceID].getNextCheckpointNum(this.raceList[raceID].getNextCheckpointNum(this.lastCheckpoint)) )
						this.message("You missed a checkpoint! Please go back and pass checkpoint "+this.raceList[raceID].getNextCheckpointNum(this.lastCheckpoint)+" first.", "cross.png");
					else if( checkpointNum == this.raceList[raceID].getPreviousCheckpointNum(this.lastCheckpoint) )
						this.message("Wrong checkpoint! Are you driving in the correct direction?", "cross.png");
					else
						this.message("Wrong checkpoint! You must find and pass checkpoint "+this.raceList[raceID].getNextCheckpointNum(this.lastCheckpoint), "cross.png");
				}
			}
			else
			{
				if(checkpointNum == 0 and not this.obligatedFinish and not this.raceList[raceID].isLocked() )
				{
					// we passed the startline of another race
					this.cancelCurrentRace();
					this.lastCheckpointInstance = inst;
					this.penaltyGiven = false;
					this.startRace(raceID);
				}
			}
		}
	}
	
	string formatTime(double seconds)
	{
		if( seconds > 60.0 )
			return floor(seconds/60.0)+" minutes and "+(seconds%60.0)+" seconds";
		else
			return seconds+" seconds";
	}
	
	//  pre: The race corresponding with the raceID exists
	//       There's no other race running
	// post: The race is running
	void startRace(int raceID)
	{
		// debug: game.log("racesManager::startRace(" + raceID + ") called.");

		// if the race is locked, then we do nothing
		if( this.raceList[raceID].isLocked() )
		{
			// call the callback function
			RACE_EVENT_CALLBACK @handle;
			if( callbacks.get("LockedRace", @handle) and not (handle is null))
			{
				dictionary args;
				args.set("event", "LockedRace");
				args.set("raceID", raceID);
				handle(args);
			}
			return;
		}
						
		this.state = STATE_Racing;
		this.currentRace = raceID;
		this.currentLap = 1;
		this.lastCheckpoint = 0;
		this.raceStartTime = game.getTime();
		this.lapStartTime = this.raceStartTime;
		game.startTimer(raceID);
		game.setBestLapTime(this.raceList[raceID].bestLapTime);
		this.recalcArrow();
		this.truckNum = game.getCurrentTruckNumber();
		this.raceList[raceID].lastTimeTillPoint[0] = 0.0;
		this.penaltyTime.resize(0);
		this.penaltyTime.resize(this.raceList[raceID].checkPointsCount);
				
		// build the message
		this.message("Race "+this.raceList[raceID].raceName+" started!", "bullet_go.png");
		if( (this.raceList[raceID].laps > 1) )
			this.message("Laps: "+this.raceList[raceID].laps, "arrow_rotate_clockwise.png");
		if( this.showBestRace and this.raceList[raceID].bestRaceTime > 0.0 )
			this.message("Best race time: "+this.formatTime(this.raceList[raceID].bestRaceTime)+"!", "information.png");
		if( this.showBestLap and this.raceList[raceID].bestLapTime > 0.0 and this.raceList[raceID].laps != this.LAPS_NoLaps and this.raceList[raceID].laps != this.LAPS_One)
			this.message("Best lap time: "+this.formatTime(this.raceList[raceID].bestLapTime)+"!", "information.png");
		this.message("Good Luck!", "emoticon_smile.png");
		
		// call the callback function
		RACE_EVENT_CALLBACK @handle;
		if( callbacks.get("RaceStart", @handle) and not (handle is null))
		{
			dictionary args;
			args.set("event", "RaceStart");
			args.set("raceID", raceID);
			handle(args);
		}
	}
	
	// this is private as this should only be called when the user drives through the finish checkpoint
	// If you need to abort a race, use racesManager::cancelCurrentRace() instead.
	void finishCurrentRace()
	{
		// debug: game.log("racesManager::finishCurrentRace() called.");
		
		int rid = this.currentRace;
	
		// get the lap time
		double lapTime = game.getTime() - this.lapStartTime;
		// get the race time
		double raceTime = game.getTime() - this.raceStartTime;
		
		// calculate race time difference
		string raceTimeDiff = "";
		if(this.showTimeDiff and this.raceList[rid].bestRaceTime > 0.0)
		{
			if( (raceTime-this.raceList[rid].bestRaceTime) > 0 )
				raceTimeDiff = " (+"+ (raceTime-this.raceList[rid].bestRaceTime) +")";
			else if( (raceTime-this.raceList[rid].bestRaceTime) < 0 )
				raceTimeDiff = " ("+ (raceTime-this.raceList[rid].bestRaceTime) +")";
		}
		
		// calculate lap time difference
		string lapTimeDiff = "";
		if(this.showTimeDiff and this.raceList[rid].bestLapTime > 0.0)
		{
			if( (lapTime-this.raceList[rid].bestLapTime) > 0 )
				lapTimeDiff = " (+"+ (lapTime-this.raceList[rid].bestLapTime) +")";
			else if( (lapTime-this.raceList[rid].bestLapTime) < 0 )
				lapTimeDiff = " ("+ (lapTime-this.raceList[rid].bestLapTime) +")";
		}
		
		// do more time stuff
		this.raceList[rid].lastTimeTillPoint[this.raceList[rid].checkPointsCount-1] = lapTime;
		bool newBestRace;
		this.addRaceTime(rid, raceTime, newBestRace);
		bool newBestLap;
		this.addLapTime(rid, lapTime, newBestLap);

		game.stopTimer();

		// reset some values
		this.lastCheckpoint = -1;
		this.currentRace = -1;
		this.currentLap = -1;
		this.state = this.STATE_NotInRace;
		this.removeArrow();
		this.lastRaceEventInstance = "";
		
		// race completed!
		this.raceList[rid].completed = true;

		// build the message
		this.message("Finished in "+this.formatTime(raceTime)+"!"+raceTimeDiff, "flag_green.png");
		if( this.showBestRace and newBestRace )
			this.message("New best race time!", "flag_green.png");
		if( this.showBestLap and newBestLap  and this.raceList[rid].laps != this.LAPS_NoLaps and this.raceList[rid].laps != this.LAPS_One)
			this.message("New best lap time!"+lapTimeDiff, "flag_green.png");
		
		// store the new race times
		saveRace(rid);
		
		// call the callback function
		RACE_EVENT_CALLBACK @handle;
		if( callbacks.get("RaceFinish", @handle) and not (handle is null))
		{
			dictionary args;
			args.set("event", "RaceFinish");
			args.set("raceID", rid);
			args.set("newBestLap", newBestLap);
			args.set("newBestRace", newBestRace);
			handle(args);
		}
	}
	
	// This is private, as you shouldn't manually advance a lap
	void advanceLap()
	{
		// debug: game.log("racesManager::advanceLap() called.");
	
		int rid = this.currentRace;
		
		// get the lapTime
		double lapTime = game.getTime() - this.lapStartTime;
				
		// calculate time difference
		string timeDiff = "";
		if(this.showTimeDiff and this.raceList[rid].bestLapTime > 0.0)
		{
			if( (lapTime-this.raceList[rid].bestLapTime) > 0 )
				timeDiff = " (+"+ (lapTime-this.raceList[rid].bestLapTime) +")";
			else if( (lapTime-this.raceList[rid].bestLapTime) < 0 )
				timeDiff = " ("+ (lapTime-this.raceList[rid].bestLapTime) +")";
		}
		
		// do time stuff
		this.raceList[rid].lastTimeTillPoint[this.raceList[rid].checkPointsCount-1] = lapTime;
		bool newBestLap;
		this.addLapTime(rid, lapTime, newBestLap);
		game.stopTimer();
		game.startTimer(rid);
		this.lapStartTime = game.getTime();
		
		// advance the lap
		this.currentLap++;
		this.lastCheckpoint = 0;
		this.recalcArrow();
				
		// build the message
		if( this.raceList[rid].laps != this.LAPS_Unlimited )
			this.message("Lap "+(this.currentLap-1)+" done!", "flag_green.png");
		if( this.showBestLap and newBestLap )
			this.message("New best lap time: "+this.formatTime(lapTime)+"!"+timeDiff, "flag_green.png");
		else
			this.message("Lap time: "+this.formatTime(lapTime)+"!"+timeDiff, "flag_green.png");
		
		// store the new race times
		saveRace(rid);
		
		// call the callback function
		RACE_EVENT_CALLBACK @handle;
		if( callbacks.get("AdvanceLap", @handle) and not (handle is null))
		{
			dictionary args;
			args.set("event", "AdvanceLap");
			args.set("raceID", rid);
			handle(args);
		}
	}
	
	// called by raceEvent when the user drives through a checkpoint that is not a finishline and not a startline
	//  pre: The race corresponding with the raceID exists
	//       The race corresponding with the raceID is running at the moment
	// post: We have advanced 1 checkpoint
	void advanceCheckpoint(int raceID)
	{
		// debug: game.log("racesManager::advanceCheckpoint(" + raceID + ") called.");

		this.lastCheckpoint = this.raceList[raceID].getNextCheckpointNum(this.lastCheckpoint);
		
		this.recalcArrow();
		
		double time = game.getTime() - this.lapStartTime;
		
		// calculate time difference		
		string timeDiff = "";
		if(this.showTimeDiff and this.raceList[raceID].bestTimeTillPoint[this.lastCheckpoint] > 0.0)
		{
			double diff = time-this.raceList[raceID].bestTimeTillPoint[this.lastCheckpoint];
			if( diff > 0 )
				timeDiff = " (+"+ diff +")";
			else if( diff < 0 )
			{
				timeDiff = " ("+ diff +")";
				this.raceList[raceID].bestTimeTillPoint[this.lastCheckpoint] = time;
			}
			game.setTimeDiff(diff);
		}
		else
			this.raceList[raceID].bestTimeTillPoint[this.lastCheckpoint] = time;
		
		this.raceList[raceID].lastTimeTillPoint[this.lastCheckpoint] = time;

		// build the message
		if( this.raceList[raceID].laps == this.LAPS_NoLaps )
			this.message("Passed checkpoint "+this.lastCheckpoint+" of "+(this.raceList[raceID].checkPointsCount-1)+" after "+this.formatTime(time)+"."+timeDiff, "flag_orange.png");
		else
			this.message("Passed checkpoint "+this.lastCheckpoint+" of "+(this.raceList[raceID].checkPointsCount)+" after "+this.formatTime(time)+"."+timeDiff, "flag_orange.png");
		if( this.currentLap >= this.raceList[raceID].laps and this.raceList[raceID].finishNum == this.raceList[raceID].getNextCheckpointNum(this.lastCheckpoint) )
			this.message("Go for the finish!", "flag_orange.png");

		// call the callback function
		RACE_EVENT_CALLBACK @handle;
		if( callbacks.get("Checkpoint", @handle) and not (handle is null))
		{
			dictionary args;
			args.set("event", "Checkpoint");
			args.set("raceID", raceID);
			handle(args);
		}
	}
	
	void setBestLapTime(int raceID, double time)
	{
		this.raceList[raceID].bestLapTime = time;
	}
	void setBestRaceTime(int raceID, double time)
	{
		this.raceList[raceID].bestRaceTime = time;
	}
	
	// Set a new best laptime (if it's better than the old best laptime)
	//  pre: The race corresponding with the raceID exists
	// post: The new time is checked and stored if necessary
	void addLapTime(int raceID, double time, bool &out newBestLap)
	{
		if( this.raceList[raceID].bestLapTime > time or this.raceList[raceID].bestLapTime == 0.0)
		{
			// call the callback function
			RACE_EVENT_CALLBACK @handle;
			if( callbacks.get("NewBestLap", @handle) and not (handle is null))
			{
				dictionary args;
				args.set("event", "NewBestLap");
				args.set("raceID", raceID);
				args.set("oldTime", this.raceList[raceID].bestLapTime);
				args.set("newTime", time);
				handle(args);
			}
			
			this.raceList[raceID].bestLapTime = time;
			game.setBestLapTime(time);
			newBestLap = true;
		}
		else
			newBestLap = false;
			
		if( this.submitScore )
		{				
			string api_return;
			dictionary dict;
			dict.set("race-name", ""+this.raceList[raceID].raceName);
			dict.set("race-version", ""+this.raceList[raceID].raceVersion);
			dict.set("lap-time", ""+time);
			string times = "0.0";
			for( uint i = 1; i < this.raceList[raceID].lastTimeTillPoint.length() ; i++ )
			{
				times += ";"+this.raceList[raceID].lastTimeTillPoint[i];
				// dict.set("chptime"+i, this.raceList[raceID].lastTimeTillPoint[i]);
			}
			dict.set("split-times", ""+times);
			
			int res = game.useOnlineAPI("/races", dict, api_return);
			// debug: game.log("useOnlineAPI returned: " + res);
			// debug: game.log("useOnlineAPI return string: " + api_return);
		}
	}
	
	// Set a new best racetime (if it's better than the old best racetime)
	//  pre: The race corresponding with the raceID exists
	// post: The new time is checked and stored if necessary
	void addRaceTime(int raceID, double time, bool &out newBestRace)
	{
		if( this.raceList[raceID].bestRaceTime > time or this.raceList[raceID].bestRaceTime == 0.0)
		{
			// call the callback function
			RACE_EVENT_CALLBACK @handle;
			if( callbacks.get("NewBestRace", @handle) and not (handle is null))
			{
				dictionary args;
				args.set("event", "NewBestRace");
				args.set("raceID", raceID);
				args.set("oldTime", this.raceList[raceID].bestRaceTime);
				args.set("newTime", time);
				handle(args);
			}
		
			this.raceList[raceID].bestRaceTime = time;
			
			newBestRace = true;
		}
		else
			newBestRace = false;
	}
	
	void eventCallback(int eventnum, int value)
	{
		// debug: game.log("raceManager::eventCallback("+eventnum+", "+value+") called");
		
		if( this.state != this.STATE_Racing )
			return;

		// this never gets called
		if( eventnum == SE_TRUCK_EXIT )
		{
			if( this.abortOnVehicleExit )
			{
				this.cancelCurrentRace();
				this.message("Race aborted.", "stop.png");
			}
			else if( !this.silentMode )
				this.message("Get back in the vehicle!", "stop.png");
		}
		else if( eventnum == SE_TRUCK_ENTER and this.truckNum != value and !this.allowVehicleChanging)
		{
			this.cancelCurrentRace();
			this.message("You cannot switch vehicles during a race! Race aborted.", "stop.png");
		}
		else if( eventnum == SE_TRUCK_RESET and this.truckNum == value)
		{
			this.cancelCurrentRace();
			this.message("You must not reset the vehicle during a race! Race aborted.", "stop.png");
		}
		else if( eventnum == SE_TRUCK_TELEPORT and this.truckNum == value)
		{
			this.cancelCurrentRace();
			this.message("You must not teleport the vehicle during a race! Race aborted.", "stop.png");
		}
		else if( eventnum == SE_GENERIC_DELETED_TRUCK and this.truckNum == value and !this.allowVehicleChanging)
		{
			this.cancelCurrentRace();
			this.message("You must finish the race with the vehicle you started it! Race aborted.", "stop.png");
		}
		else if( eventnum == SE_TRUCK_MOUSE_GRAB and this.truckNum == value )
		{
			this.cancelCurrentRace();
			this.message("You must not grab the vehicle during a race! Race aborted.", "stop.png");
		}
		else if( eventnum == SE_ANGELSCRIPT_MANIPULATIONS )
		{
			this.cancelCurrentRace();
			this.message("AngelScript injection is not allowed during races! Race aborted.", "stop.png");
		}
	}
	
	void message(const string &in msg, const string &in icon)
	{
		if(!this.silentMode)
			game.message(msg, icon, 10000, true); // 10 seconds visible, enforce visibility
	}
	
	void unlockRace(int raceID)
	{
		this.raceList[raceID].locked = false;
	}

	void lockRace(int raceID)
	{
		this.raceList[raceID].locked = true;
	}
	
	void deleteRace(int raceID)
	{
		// debug: game.log("raceManager::deleteRace("+raceID+") called.");
		if( this.currentRace == raceID )
			this.cancelCurrentRace();
		this.raceList[raceID].destroy();
		this.raceList[raceID].awaitingRecycling = true;
		
		if( raceID == this.raceCount-1 )
		{
			this.raceCount--;
			this.raceList.resize(this.raceCount);
		}
	}
	
	int getRaceIDbyName(const string &in raceName_in)
	{
		for( int i = 0; i<this.raceCount; i++ )
		{
			if( not (this.raceList[i] is null) and (raceName_in == this.raceList[i].raceName) )
				return i;
		}
		return -1;
	}
	
	void cancelCurrentRace()
	{		
		// call the callback function
		RACE_EVENT_CALLBACK @handle;
		if( callbacks.get("RaceCancel", @handle) and not (handle is null))
		{
			dictionary args;
			args.set("event", "RaceCancel");
			args.set("raceID", this.currentRace);
			handle(args);
		}

		this.lastCheckpoint = -1;
		this.currentRace = -1;
		this.currentLap = -1;
		this.lastRaceEventInstance = "";
		this.state = this.STATE_NotInRace;
		game.stopTimer();
		this.removeArrow();
	}
	
	bool raceCompleted(int raceID)
	{
		return this.raceList[raceID].completed;
	}
		
	int getCurrentRaceID()
	{
		return this.currentRace;
	}
	
	double getBestLapTime(int raceID)
	{
		return this.raceList[raceID].bestLapTime;
	}
	
	double getBestRaceTime(int raceID)
	{
		return this.raceList[raceID].bestRaceTime;
	}
	
	bool addPenaltySeconds(int seconds)
	{
		if( this.state == this.STATE_NotInRace )
			return false;
		this.raceStartTime -= seconds;
		this.lapStartTime  -= seconds;
		this.penaltyTime[this.lastCheckpoint] += seconds;
		return true;
	}
	
	void setLaps(int raceID, int laps)
	{
		this.raceList[raceID].laps = laps;
	}
	
	void setRaceName(int raceID, const string &in raceName)
	{
		this.raceList[raceID].raceName = raceName;
	}
	
	int getLaps(int raceID)
	{
		return this.raceList[raceID].laps;
	}
	
	raceBuilder@ getRace(int raceID)
	{
		return @this.raceList[raceID];
	}
	
	// this allows you to have duplicate checkpoints (useful to create shortcuts or splitted tracks)
	void addCheckpoint(int raceID, int number, const string &in objName, double[] coords)
	{
		this.raceList[raceID].addCheckpoint(number, objName, coords);
	}
	
	void addCheckpointList(int raceID, uint startNumber, const string &in objName, double[][] coords)
	{
		this.raceList[raceID].addChpCoordinates(coords, objName, objName, objName, startNumber);
	}

	// Advanced users only!
	// this deletes a checkpoint
	// Warning: if you delete a checkpoint in the middle of your race, then it will be impossible
	//          to finish the race, as it will be impossible to pass that checkpoint.
	// Also don't use this on running races (unless you know what you're doing)
	void deleteCheckPoint(int raceID, int number, int instance)
	{
		this.raceList[raceID].deleteCheckpoint(number, instance);
	}
		
	// if the user comes in this event box, the current race will be cancelled
	// example usage: if the users drives off the track
	// (race will be stopped)
	void addCancelPoint(int raceID, const string &in objName, const vector3 &in pos, const vector3 &in rot, RACE_EVENT_CALLBACK @callback)
	{
		dictionary dict;
		dict.set("raceID", raceID);
		dict.set("oname", ""+objName);
		// dict.set("position", vector3(pos));
		// dict.set("rotation", vector3(rot));
		dict.set("callback", @callback);
		this.callbacks.set("race_cancel_"+cancelPointCount, dict);
		game.spawnObject(objName, "race_cancel_"+cancelPointCount, pos, rot, "raceCancelPointHandler", false);
	}
	void addCancelPoint(int raceID, const string &in objName, const double[] &in v, RACE_EVENT_CALLBACK @callback)
	{
		addCancelPoint(raceID, objName, vector3(v[0], v[1], v[2]), vector3(v[3], v[4], v[5]), @callback);
	}
	void addCancelPoint(int raceID, const string &in objName, const vector3 &in pos, const vector3 &in rot)
	{
		dictionary dict;
		dict.set("raceID", raceID);
		dict.set("oname", ""+objName);
		// dict.set("position", vector3(pos));
		// dict.set("rotation", vector3(rot));
		dict.set("callback", null);
		this.callbacks.set("race_cancel_"+cancelPointCount, dict);
		game.spawnObject(objName, "race_cancel_"+cancelPointCount, pos, rot, "raceCancelPointHandler", false);
	}
	void addCancelPoint(int raceID, const string &in objName, const double[] &in v)
	{
		addCancelPoint(raceID, objName, vector3(v[0], v[1], v[2]), vector3(v[3], v[4], v[5]));
	}
	
	void raceCancelPointHandler(int trigger_type, const string &in inst, const string &in box, int nodeid)
	{
		if( this.state == this.STATE_NotInRace )
			return;

		dictionary dict;
		if( not this.callbacks.get(inst, dict) )
			return;

		int raceID;
		dict.get("raceID", raceID);
		if( raceID != -1 and this.currentRace != raceID )
			return;
		
		this.cancelCurrentRace();
		
		// call the callback function
		RACE_EVENT_CALLBACK @handle;
		if( dict.get("callback", @handle) and not (handle is null) )
		{
			dictionary args;
			args.set("event", "cancel_point");
			args.set("raceID", this.currentRace);
			args.set("inst", ""+inst);
			args.set("trigger_type", trigger_type);
			args.set("box", ""+box);
			args.set("nodeid", nodeid);
			handle(args);
		}
	}
	
	void recalcArrow()
	{
		if( this.state == this.STATE_Racing )
			this.setupArrow(this.raceList[this.currentRace].getNextCheckpointNum(this.lastCheckpoint));
		else
			this.removeArrow();
	}

	// set a navigational arrow
	void setupArrow(int position)
	{		
		if( (position < 0) or (position > this.raceList[this.currentRace].checkPointsCount-1) )
		{ // hide the arrow
			this.removeArrow();
			return;
		}

		double[] v;
		int instanceNum = this.arrowMethod;
		if( this.arrowMethod == this.ARROW_AUTO )
		{
			array<string>@ tmp = this.lastCheckpointInstance.split("|");
			// int checkpointNum = parseInt(tmp[2]);
			int raceID        = parseInt(tmp[1]);
			if( raceID == this.currentRace )
				instanceNum = parseInt(tmp[3]);
		}
		
		if( this.raceList[this.currentRace].checkpoints[position].length() > uint(instanceNum) )
			v = this.raceList[this.currentRace].checkpoints[position][instanceNum];
		else if( this.raceList[this.currentRace].checkpoints[position].length() > 0 )
			v = this.raceList[this.currentRace].checkpoints[position][0];
		else
		{
			this.removeArrow();
			return;
		}
		
		if( this.raceList[this.currentRace].laps == this.LAPS_NoLaps )
			game.updateDirectionArrow(this.raceList[this.currentRace].raceName+" checkpoint "+position+" / "+(this.raceList[this.currentRace].checkPointsCount-1), vector3(v[0], v[1], v[2]));
		else
		{
			if( position == 0 )
				position = this.raceList[this.currentRace].checkPointsCount;
			game.updateDirectionArrow(this.raceList[this.currentRace].raceName+" checkpoint "+position+" / "+(this.raceList[this.currentRace].checkPointsCount), vector3(v[0], v[1], v[2]));
		}
	}
	
	void removeArrow()
	{
		game.hideDirectionArrow();
	}
	
	void setPenaltyTime(int raceID, int seconds)
	{
		this.raceList[raceID].penaltyTime = seconds;
	}
	
	int getPenaltyTime(int raceID)
	{
		return this.raceList[raceID].penaltyTime;
	}
	
	void setVersion(int raceID, const string &in version)
	{
		this.raceList[raceID].setVersion(version);
	}
	
	void hideRace(int raceID)
	{
		this.raceList[raceID].hide();
	}
	
	void unhideRace(int raceID)
	{
		this.raceList[raceID].unhide();
	}
	
	void setStartNumber(int raceID, int startNum)
	{
		this.raceList[raceID].startNum = startNum;
	}
	
	void resetEventCallback()
	{
		this.lastRaceEventInstance = "";
	}
	
	void finalize()
	{
		for( int raceID = 0; raceID < this.raceCount; ++raceID )
		{
			this.raceList[raceID].loadRace(@this.raceDataFile);
		}
	}
	
	void finalize(int raceID)
	{
		loadRace(raceID);
	}

	// Internal function
	void saveRaces()
	{
		for( int raceID = 0; raceID < this.raceCount; ++raceID )
		{
			this.raceList[raceID].saveRace(@this.raceDataFile);
		}

		this.raceDataFile.save();		
	}
	
	void saveRace(int raceID)
	{
		this.raceList[raceID].saveRace(@this.raceDataFile);
		this.raceDataFile.save();
	}
	
	
	void loadRace(int raceID)
	{
		this.raceList[raceID].loadRace(@this.raceDataFile);
	}
}

// This class manages a race (singular!)
// this class shouldn't be edited!
// You should only use this directly if the races manager above doesn't suit your needs
class raceBuilder {
	string raceName;
	double[][][] checkpoints;
	array<array<string>> objNames;
	int checkPointsCount;
	int id;
	double bestLapTime;
	double bestRaceTime;
	int laps;
	double[] bestTimeTillPoint;
	double[] lastTimeTillPoint;
	int finishNum;
	int startNum;
	int[] chpInstances; // needed to be able to remove races
	bool locked;
	bool completed;
	int penaltyTime;
	string raceVersion;
	bool isBuilt;
	bool awaitingRecycling;
	bool hidden;
	string raceBuilderVersion;
	
	raceBuilder(int id)
	{
		// game.log("raceBuilder::raceBuilder("+id+");");
		this.raceName = "No Name";
		this.checkPointsCount = 0;
		this.id = id;
		this.bestLapTime = 0.0;
		this.bestRaceTime = 0.0;
		this.laps = 0; /* racesManager::LAPS_NoLaps */
		this.finishNum = 0;
		this.startNum = 0;
		this.locked = false;
		this.completed = false;
		this.penaltyTime = 0;
		this.raceVersion = "unknown";
		this.awaitingRecycling = false;
		this.raceBuilderVersion = "RoR_RaceBuilder_v0.01";
		this.hidden = false;
	}
	
	void setVersion(const string &in version)
	{
		this.raceVersion = version;
	}
		
	void addChpCoordinates(double[][] checkpoints_in, const string &in objName_checkpoint, const string &in objName_start, const string &in objName_finish, uint startNumber)
	{
		// Remove empty coordinates at end
		for(int i=checkpoints_in.length()-1; i>=0; --i)
		{
			if(checkpoints_in[i].length()<3)
			{
				checkpoints_in.removeAt(i);
				game.log("Warning in raceBuilder("+this.id+")::addChpCoordinates: Checkpoint "+i+" ignored (did you put a comma too much after your last checkpoint?).");
			}
		}

		string oname = "chp-checkpoint";
		for( uint i = 0; i < checkpoints_in.length() ; i++ )
		{
			// check the coordinates, and try to correct them if possible
			if( checkpoints_in[i].length() < 3 )
			{
				game.log("Error in raceBuilder("+this.id+")::addChpCoordinates: A coordinate exists out of 6 numbers, "+checkpoints_in[i].length()+" found. Ingored checkpoint "+i+".");
				continue;
			}
			if( checkpoints_in[i].length() < 6 )
			{
				game.log("Warning in raceBuilder("+this.id+")::addChpCoordinates: A coordinate should exist out of 6 numbers. Padding with zeros.");
				checkpoints_in[i].resize(6);
				for( int k = checkpoints_in[i].length() ; k <=6 ; k++ )
				{
					checkpoints_in[i][k] = 0.0;
				}
			}
			if( checkpoints_in[i].length() > 6 )
			{
				game.log("Warning in raceBuilder("+this.id+")::addChpCoordinates: A coordinate should exist out of 6 numbers. Extra numbers dropped.");
				checkpoints_in[i].resize(6);
			}
			
			// Get the correct object name
			if( i == 0 )
				oname = objName_start;
			else if( ( i == checkpoints_in.length()-1 ) and ( this.laps == 0 /* racesManager::LAPS_NoLaps */) )
				oname = objName_finish;
			else
				oname = objName_checkpoint;
			
			this.addCheckpoint(startNumber+i, oname, checkpoints_in[i]);
		}
	}
	
	int getNextCheckpointNum(int lastCheckpoint)
	{
		if( lastCheckpoint < this.checkPointsCount-1 )
			return lastCheckpoint+1;
		else if( lastCheckpoint == this.checkPointsCount-1 )
			return this.startNum;
		else
			game.log("ERROR in raceBuilder::getNextCheckpointNum: unhandled situation");
		return -1;
	}

	int getPreviousCheckpointNum(int lastCheckpoint)
	{
		if( lastCheckpoint > this.startNum )
		{
			return lastCheckpoint-1;
		}
		else if( (lastCheckpoint == this.startNum) and (this.laps == 0) /* racesManager::LAPS_NoLaps */)
		{
			return this.checkPointsCount-1;
		}
		else if( (lastCheckpoint == this.startNum) and (this.laps != 0) /* racesManager::LAPS_NoLaps */)
			return this.checkPointsCount-1;
		else
			game.log("ERROR in raceBuilder::getNextCheckpointNum: unhandled situation");
		return -1;
	}

	void addCheckpoint(int number, const string &in objName, const double[] &in v)
	{
		// debug: game.log("raceBuilder::addCheckpoint("+number+", \""+objName+"\", coords) called.");
		if( number > this.checkPointsCount )
		{
			game.log("Error in raceBuilder::addCheckpoint: Trying to add a checkpoint with a too high number ("+number+").");
			return;
		}
		else if( number < 0 )
		{
			game.log("Error in raceBuilder::addCheckpoint: The checkpoint number should positive.");
			return;
		}
		else if( number == this.checkPointsCount )
		{
			this.checkPointsCount++;
			if( this.laps == 0 /*racesManager::LAPS_NoLaps*/ )
				this.finishNum = this.checkPointsCount-1;
			this.bestTimeTillPoint.resize(this.checkPointsCount);
			this.bestTimeTillPoint[number] = 0.0;
			this.lastTimeTillPoint.resize(this.checkPointsCount);
			this.lastTimeTillPoint[number] = 0.0;
			this.chpInstances.resize(this.checkPointsCount);
			this.chpInstances[number] = 0;
			this.checkpoints.resize(this.checkPointsCount);
			this.objNames.resize(this.checkPointsCount);
		}
		this.checkpoints[number].resize(this.chpInstances[number]+1);
		this.checkpoints[number][this.chpInstances[number]] = v;
		this.objNames[number].resize(this.chpInstances[number]+1);
		this.objNames[number][this.chpInstances[number]] = objName;
		if( not this.hidden )
			game.spawnObject(objName, "checkpoint|"+this.id+"|"+number+"|"+this.chpInstances[number]++, vector3(v[0], v[1], v[2]), vector3(v[3], v[4], v[5]), "raceEvent", false);
	}
	
	void deleteCheckpoint(int number)
	{
		for( int i = this.chpInstances[number]-1; i >= 0; i-- )
		{
			this.deleteCheckpoint(number, i);
		}
	}
	
	// this gets the actual number of instances
	// (if you remove a checkpoint instance, then it may
	// get destroyed ingame, but the array may not always
	// get shortened.)
	uint getRealInstanceCount(int chpNum)
	{
		uint count = 0;
		for( int i = 0; i < this.chpInstances[chpNum]; i++ )
		{
			if( this.checkpoints[chpNum][i].length() != 0 )
				++count;
		}
		return count;
	}
	
	bool checkpointExists(int chpNum, int instance)
	{
		if(
			   ( uint(chpNum) >= this.checkpoints.length() )
			or ( chpNum < 0 )
			or ( instance >= this.chpInstances[chpNum] )
			or ( instance < 0 )
			or ( this.checkpoints[chpNum][instance].length() == 0 )
		)
			return false;
		else
			return true;
	}
	
	void deleteCheckpoint(int number, int instance)
	{
		if( not this.checkpointExists(number, instance) )
			return;

		// if this is the last instance of this checkpoint
		// and this is the last checkpoint
		// then we can really remove this checkpoint
		else if( ( this.getRealInstanceCount(number) == 1 ) and ( number == this.checkPointsCount-1 ) )
		{
			this.checkPointsCount--;
			this.bestTimeTillPoint.resize(this.checkPointsCount);
			this.lastTimeTillPoint.resize(this.checkPointsCount);
			this.chpInstances.resize(this.checkPointsCount);
			this.checkpoints.resize(this.checkPointsCount);
			this.objNames.resize(this.checkPointsCount);
			if( this.laps == 0 )
				this.finishNum = this.checkPointsCount-1;
		}

		// if the instance is the last added instance for this checkpoint
		// then we can remove it fully as well
		else if( this.chpInstances[number] == instance+1 )
		{
			this.chpInstances[number]--;
			this.checkpoints[number].resize(this.chpInstances[number]);
			this.objNames[number].resize(this.chpInstances[number]);
			while( ( this.chpInstances[number] > 0 ) and ( this.checkpoints[number][this.chpInstances[number]-1].length() == 0 ) )
			{
				this.chpInstances[number]--;
				this.checkpoints[number].resize(this.chpInstances[number]);
				this.objNames[number].resize(this.chpInstances[number]);
			}
		}

		else
		{
			this.checkpoints[number][instance].resize(0);
		}
			
		game.destroyObject("checkpoint|"+this.id+"|"+number+"|"+instance);
	}

	// this function removes all checkpoints again
	void destroy()
	{
		// game.log("raceBuilder::destroy() called for raceID "+this.id+".");
		this.hide();
		this.bestTimeTillPoint.resize(0);
		this.lastTimeTillPoint.resize(0);
		this.chpInstances.resize(0);
		this.checkpoints.resize(0);
		this.objNames.resize(0);
		this.checkPointsCount = 0;
		this.finishNum = 0;
		this.startNum = 0;
	}
	
	void hide()
	{
		if( this.hidden )
			return;
		else
			this.hidden = true;
		
		for(int i = 0 ; i<this.checkPointsCount ; i++)
		{
			for(int k = 0; k<this.chpInstances[i]; k++)
			{
				// game.log("game.destroyObject(checkpoint|"+this.id+"|"+i+"|"+k+");");
				if( this.checkpoints[i][k].length() != 0 )
					game.destroyObject("checkpoint|"+this.id+"|"+i+"|"+k);
			}
		}
	}
	
	void unhide()
	{
		if( not this.hidden )
			return;
		else
			this.hidden = false;

		for( int i = 0; i < this.checkPointsCount; i++ )
		{
			for( int k = 0; k < this.chpInstances[i]; k++ )
			{
				game.spawnObject(objNames[i][k], "checkpoint|"+this.id+"|"+i+"|"+k, vector3(this.checkpoints[i][k][0], this.checkpoints[i][k][1], this.checkpoints[i][k][2]), vector3(this.checkpoints[i][k][3], this.checkpoints[i][k][4], this.checkpoints[i][k][5]), "raceEvent", false);
			}
		}
	}
	
	void setLaps(int laps_in)
	{
		if( this.laps == laps_in )
			return;
		else if( this.checkPointsCount == 0 )
			this.laps = laps_in;
		else if( laps_in != 0 )
			this.finishNum = 0;
		else if( laps_in == 0 )
			this.finishNum = this.checkPointsCount-1;
	}
	
	bool isLocked()
	{
		return this.locked;
	}
	
	void saveRace(LocalStorage @d)
	{
		// Go to the correct section
		string terrain;
		game.getLoadedTerrain(terrain);
		int terrnPos = terrain.findFirst(".", 0);
		int racePos = raceName.findFirst(".", 0);
		d.changeSection(terrain.substr(0, terrnPos) + "--" + raceName.substr(0, racePos));
		
		// save all the data
		d.set("raceName", raceName);
		d.set("terrain", terrain);
		d.set("raceBuilderVersion", raceBuilderVersion);
		d.set("raceVersion", raceVersion);
		d.set("bestLapTime", bestLapTime);
		d.set("bestRaceTime", bestRaceTime);
		d.set("checkPointsCount", checkPointsCount);
		d.set("completed", completed);

		string tmp = "0.0";
		for( int i = 1; i < checkPointsCount; ++i )
		{
			tmp += ";"+bestTimeTillPoint[i];
		}
		d.set("bestTimeTillPoint", tmp);
		
		// For performance reasons, we don't save here yet.
	}
	
	void loadRace(LocalStorage@ d)
	{
		// Go to the correct section
		string terrain;
		game.getLoadedTerrain(terrain);
		int terrnPos = terrain.findFirst(".", 0);
		int racePos = raceName.findFirst(".", 0);
		d.changeSection(terrain.substr(0, terrnPos) + "--" + raceName.substr(0, racePos));

		// Only load the race if it's exactly the same
		// (this also returns if the race was never saved before)
		if( (d.get("raceName") != raceName)
			|| (d.get("terrain") != terrain)
			|| (d.get("raceBuilderVersion") != raceBuilderVersion)
			|| (d.get("raceVersion") != raceVersion)
			|| (d.getInt("checkPointsCount") != checkPointsCount)
		)
			return;

		// Load all the data
		bestLapTime = d.getFloat("bestLapTime");
		bestRaceTime = d.getFloat("bestRaceTime");
		completed = d.getBool("completed");
		string tmp = d.get("bestTimeTillPoint");
		
		int p1 = -1;
		uint p2 = 0;
		for( int i = 0; i < checkPointsCount; ++i )
		{
			p2 = tmp.findFirst(";", p1+1);
			bestTimeTillPoint[i] = parseFloat(tmp.substr(p1+1, p2-p1-1));
			p1 = p2;
		}
	}
}
