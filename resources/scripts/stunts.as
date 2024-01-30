/*
This is a stunt script system of Rigs of Rods.
A 'stunt' is a type of mission where player drives a vehicle between 2 eventboxes
and the parameters of the travel (time, speed, distance, angles) are measured.
Measuring starts when the 'startBox' loses contact and stops when the 'endBox' gains contact.

The API is modelled after Neorej16's race system (for consistency), see 'races.as':
- StuntsManager: The standard interface for regular user.
- StuntBuilder: Internal, advanced interface, for tinkerers.
*/

class StuntsManager
{
    array <StuntBuilder@> stuntList;
    
    StuntsManager()
    {
        // register the required callbacks
        game.registerForEvent(SE_EVENTBOX_ENTER);
        game.registerForEvent(SE_EVENTBOX_EXIT);
    
        // add the eventcallback method if it doesn't exist
        if(game.scriptFunctionExists("void eventCallbackEx(scriptEvents, int, int, int, int, string, string, string, string)")<0)
        {
            game.log("DEBUG StuntsManager.StuntManager(): binding `eventCallbackEx()`");
            game.addScriptFunction("void eventCallbackEx(scriptEvents event, int arg1, int arg2ex, int arg3ex, int arg4ex, string arg5ex, string arg6ex, string arg7ex, string arg8ex){ stunts.eventCallbackEx(event, arg1, arg2ex, arg3ex, arg4ex, arg5ex, arg6ex, arg7ex, arg8ex); }");
        }
            
        // add the framestep function if it doesn't exist yet
        if (game.scriptFunctionExists("void frameStep(float)")<0)
        {
            game.log("DEBUG StuntsManager.StuntManager(): binding `frameStep()`");
            game.addScriptFunction("void frameStep(float dt) { stunts.frameStep(dt); }");
        }
    
    }
    
    void eventCallbackEx(scriptEvents event, int arg1, int arg2ex, int arg3ex, int arg4ex, string arg5ex, string arg6ex, string arg7ex, string arg8ex)
    {
        if (event == SE_EVENTBOX_EXIT)
        {
            // Arguments of `eventCallbackEx()`: #1 type, #2 Actor Instance ID (use `game.getTruckByNum()`), #3 unused, #4 unused, #5 object instance name, #6 eventbox name #7 unused #8 unused.
            this.onEventboxExitCallback(arg2ex, arg5ex, arg6ex);
        }
        else if (event == SE_EVENTBOX_ENTER)
        {
            // Arguments of `eventCallbackEx()`: #1 type, #2 Actor Instance ID (use `game.getTruckByNum()`), #3 Actor node ID, #4 unused, #5 object instance name, #6 eventbox name #7 unused #8 unused.
            this.onEventboxEnterCallback(arg2ex, arg3ex, arg5ex, arg6ex);
        }        
    }
    
    void onEventboxExitCallback(int truckNum, string objInstance, string boxName)
    {
        game.log("DEBUG >> StuntManager.onEventBoxExitCallback(): truckNum="+truckNum+", objInstance="+objInstance+", boxName="+boxName);
        for (uint i = 0; i < this.stuntList.length(); i++)
        {
            StuntBuilder@ stunt = this.stuntList[i];
            // Stunt begins when vehicle stops touching the starting box.
            if (!stunt.awaitingRecycling
                && !stunt.isOngoing
                && objInstance == stunt.startObjInstance
                && boxName == stunt.startBoxName)
            {
                game.log("DEBUG: Stunt '" + stunt.stuntName +  "' (ID: " + i + ") started!");
                stunt.beginStunt(truckNum);
            }
        }
        game.log("DEBUG << StuntManager.onEventBoxExitCallback()");
    }
    
    void onEventboxEnterCallback(int truckNum, int nodeNum, string objInstance, string boxName)
    {
        game.log("DEBUG >> StuntManager.onEventBoxEnterCallback(): truckNum="+truckNum+", nodeNum="+nodeNum+", objInstance="+objInstance+", boxName="+boxName);
        for (uint i = 0; i < this.stuntList.length(); i++)
        {
            StuntBuilder@ stunt = this.stuntList[i];
            // Stunt ends when vehicle touches the finish box.
            if (stunt.isOngoing
                && truckNum == stunt.truckNum
                && objInstance == stunt.endObjInstance
                && boxName == stunt.endBoxName)
            {
                game.log("DEBUG: Stunt '" + stunt.stuntName +  "' (ID: " + i + ") finished!");
                stunt.finishStunt();
            }
        }
        game.log("DEBUG << StuntManager.onEventBoxEnterCallback()");
    }
    
    void frameStep(float dt)
    {
        for (uint i = 0; i < this.stuntList.length(); i++)
        {
            StuntBuilder@ stunt = this.stuntList[i];
            if (stunt.isOngoing)
            {
                stunt.updateStunt(dt);
            }
        }        
    }
    
    /**
    * @return stunt ID.
    */
    int addStunt(string stuntName, string startObj, string startBox, string endObj, string endBox)
    {
        game.log("DEBUG >> StuntsManager.addStunt()");
        StuntBuilder b;
        b.stuntName = stuntName;
        b.startObjInstance = startObj;
        b.startBoxName = startBox;
        b.endObjInstance = endObj;
        b.endBoxName = endBox;
        
        int stuntID = this.getNewStuntID();
        @this.stuntList[stuntID] = @b;
        game.log("DEBUG << StuntsManager.addStunt(): added stunt '"+stuntName+"' with ID " + stuntID + ", stuntCount=" + this.stuntList.length());
        return stuntID;
    }
    
    void deleteStunt(int stuntID)
    {
        const int stuntCount = this.stuntList.length();
        if (stuntID > 0 && stuntID < stuntCount)
        {
            if (stuntID == stuntCount - 1)
                this.stuntList.removeLast();
            else
                this.stuntList[stuntID].awaitingRecycling = true;
        }
    }
    
	private int getNewStuntID()
	{
        game.log("DEBUG >> StuntsManager.getNewStuntID(): stuntCount="+this.stuntList.length());
		// try to recycle a stunt ID
		for( uint i = 0; i < this.stuntList.length(); i++ )
		{
			if( this.stuntList[i].awaitingRecycling )
            {
                game.log("DEBUG << StuntsManager.getNewStuntID(): recycling ID "+i+", stuntCount="+this.stuntList.length());
				return int(i);
            }
		}
		// resize list and return new ID
        int newID = int(this.stuntList.length());
        stuntList.resize(this.stuntList.length() + 1);
        game.log("DEBUG << StuntsManager.getNewStuntID(): new ID "+newID+", stuntCount="+this.stuntList.length());
        return newID;
	}    
}

// Represents a single (!) stunt, used internally by `StuntsManager`
// Only use manually if you know what you're doing!
class StuntBuilder
{
    // Params
    string stuntName;
    string startObjInstance;
    string startBoxName;
    string endObjInstance;
    string endBoxName;
    // State
    bool awaitingRecycling = false; // A 'disposed' flag.
    bool isOngoing = false;
    int truckNum = -1;
    vector3 startPos;
    vector3 prevPos;
    float prevYaw;
    // Stats
    float topSpeed;
    float lowestSpeed;
    vector3 visitedAreaMin;
    vector3 visitedAreaMax;
    float totalDuration;
    float totalPositiveYaw;
    float totalNegativeYaw;
    float pointToPointDistance;
    
    void beginStunt(int truckNum)
    {
        BeamClass@ actor = game.getTruckByNum(truckNum);
    
        // State
        this.isOngoing = true;
        this.truckNum = truckNum;
        this.startPos = actor.getVehiclePosition();
        this.prevPos = actor.getVehiclePosition();
        this.prevYaw = actor.getHeadingDirectionAngle();   
        // Stats
        this.topSpeed = actor.getSpeed();
        this.lowestSpeed = actor.getSpeed();
        this.visitedAreaMin = actor.getVehiclePosition();
        this.visitedAreaMax = actor.getVehiclePosition();
        this.totalDuration = 0.f;
        this.totalPositiveYaw = 0.f;
        this.totalNegativeYaw = 0.f;
    }
    
    void finishStunt()
    {
        BeamClass@ actor = game.getTruckByNum(this.truckNum);
    
        this.isOngoing = false;
        this.pointToPointDistance += (actor.getVehiclePosition() - this.startPos).length();
        
        string text 
            = "Distance: " + this.pointToPointDistance + " meters"
            + "\nDuration: " + this.totalDuration + " seconds"
            + "\nTop speed: " + this.topSpeed*3.6f + " km/h"
            + "\nTop height: " + this.visitedAreaMax.y + " meters";
        game.showMessageBox("*** STUNT COMPLETE ***", text, /*button1:*/false, "", /*allowClose:*/true, /*button2:*/false, "");
    }
    
    void updateStunt(float dt)
    {
        BeamClass@ actor = game.getTruckByNum(truckNum);

        this.topSpeed = fmax(this.topSpeed, actor.getSpeed());
        this.lowestSpeed = fmin(this.lowestSpeed, actor.getSpeed());
        this.visitedAreaMin = v3min(this.visitedAreaMin, actor.getVehiclePosition());
        this.visitedAreaMax = v3max(this.visitedAreaMax, actor.getVehiclePosition());
        this.totalDuration += dt;
        float yaw = actor.getHeadingDirectionAngle();
        this.totalPositiveYaw += (yaw > 0.f) ? yaw : 0.f;
        this.totalNegativeYaw += (yaw < 0.f) ? yaw : 0.f;
    }
}

// Helpers

float fmin(float a, float b) { return (a < b) ? a : b; }
float fmax(float a, float b) { return (a > b) ? a : b; }

vector3 v3min(vector3 a, vector3 b) { return vector3(fmin(a.x, b.x), fmin(a.y, b.y), fmin(a.z, b.z)); }
vector3 v3max(vector3 a, vector3 b) { return vector3(fmax(a.x, b.x), fmax(a.y, b.y), fmax(a.z, b.z)); }
