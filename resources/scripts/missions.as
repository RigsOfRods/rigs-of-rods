/*
* Rigs of Rods mission system
* https://forum.rigsofrods.org/threads/mission-system-prototype.3846/
*
* The design of this script is derived from Neorej16's race system,
* because consistency is always the best design choice, see 'races.as':
* - MissionManager - like `racesManager`, it lets you easily add/edit/delete
*                    scenarios for a terrain. You can use it either directly
*                    in scripts or via mission definition file format.
* - MissionBuilder - like `raceBuilder`, represents a single (!) mission
*                    and allows fine control where `MissionManager` doesn't
*                    suit creator's needs.
*/

#include "road_utils.as"
#include "races.as"

// Mandatory global variable, referenced by race callbacks:
//  - raceEvent(int, string, string, int)
//  - raceCancelPointHandler(int, string, string, int)
racesManager races();

class MissionManager
{
    array<MissionBuilder@> loadedMissions;

    bool loadMission(string filename, string resourceGroup)
    {
        GenericDocumentClass@ doc = GenericDocumentClass();
        int docFlags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS;
        if (!doc.loadFromResource(filename, resourceGroup, docFlags))
        {
            game.log("MissionManager: Failed to load mission - cannot open file");
            return false;
        }
        
        MissionBuilder@ mission = MissionBuilder();
        mission.fileName = filename;
        mission.resourceGroup = resourceGroup;
        GenericDocReaderClass@ reader = GenericDocReaderClass(doc);
        
        while (!reader.endOfFile())
        {
            // Look for keywords
            if (reader.tokenType() == TOKEN_TYPE_KEYWORD)
            {
                // Basic properties
                if (reader.getTokKeyword() == "mission_format_version")
                {
                    mission.missionFormatVersion = int(reader.getTokFloat(1));
                }
                else if (reader.getTokKeyword() == "mission_name")
                {
                    mission.missionName = reader.getTokString(1);
                }
                else if (reader.getTokKeyword() == "mission_type")
                {
                    mission.missionType = reader.getTokString(1);
                }
                // Race properties
                else if (reader.getTokKeyword() == "checkpoint_obj_name")
                {
                    mission.checkpointObjName = reader.getTokString(1);
                }
                else if (reader.getTokKeyword() == "start_obj_name")
                {
                    mission.startObjName = reader.getTokString(1);
                }
                else if (reader.getTokKeyword() == "finish_obj_name")
                {
                    mission.finishObjName = reader.getTokString(1);
                }
                else if (reader.getTokKeyword() == "race_laps")
                {
                    mission.raceLaps = int(reader.getTokFloat(1));
                }
                // Procedural road
                else if (reader.getTokKeyword() == "begin_procedural_roads")
                {
                    ProceduralObjectClass@ road = ParseProceduralRoadFromFile(reader);
                    if (@road != null) // Errors already logged
                    {
                        mission.proceduralRoads.insertLast(road);
                    }
                }
                // Race checkpoints
                else if (reader.getTokKeyword() == "begin_checkpoints")
                {
                    vector3 set_extra_rotation(0,0,0);
                    while (!reader.endOfFile() && (reader.tokenType() != TOKEN_TYPE_KEYWORD || reader.getTokKeyword() != "end_checkpoints"))
                    {
                        if ((reader.tokenType(0) == TOKEN_TYPE_KEYWORD)
                            && (reader.getTokKeyword() == "set_extra_rotation")
                            && (reader.tokenType(1) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(2) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(3) == TOKEN_TYPE_NUMBER))
                        {
                            set_extra_rotation.x = reader.getTokFloat(1);
                            set_extra_rotation.y = reader.getTokFloat(2);
                            set_extra_rotation.z = reader.getTokFloat(3);
                        }
                        else if (
                            (reader.tokenType(0) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(1) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(2) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(3) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(4) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(5) == TOKEN_TYPE_NUMBER))
                        {
                            mission.checkpoints.insertLast({
                                // position
                                reader.getTokFloat(0), reader.getTokFloat(1), reader.getTokFloat(2),
                                // rotation
                                reader.getTokFloat(3) + set_extra_rotation.x,
                                reader.getTokFloat(4) + set_extra_rotation.y,
                                reader.getTokFloat(5) + set_extra_rotation.z 
                            });
                            
                        }
                        else if ((reader.tokenType(0) != TOKEN_TYPE_LINEBREAK) && (reader.tokenType(0) != TOKEN_TYPE_COMMENT))
                        {
                            game.log("MissionManager: Warning - skipping invalid checkpoint line");
                        }
                        
                        reader.seekNextLine();
                    }
                }
            }
            
            reader.seekNextLine();
        }
        
        if (!mission.build())
        {
            game.log("MissionManager: Failed to load mission - error modifying terrain");
            return false;
        }
        this.loadedMissions.insertLast(mission);
        return true;
    }
}

/*
represents a single (!) mission.
Used internally by `MissionManager` - only use it manually if you know what you're doing.
*/
class MissionBuilder
{
    // Generic properties
    string fileName;
    string resourceGroup;
    int missionFormatVersion = 1;
    string missionName;
    string missionType;
    // Terrain elements
    array<ProceduralObjectClass@> proceduralRoads;
    // Race properties
    array<array<double>> checkpoints; // The format used by 'races.as': {'posX, posY, posZ, rotX, rotY, rotZ'}
    string checkpointObjName;
    string startObjName;
    string finishObjName;
    int raceLaps = 0; // -1=NoLaps, 0=Unlimited, 1=One
    int raceID = -1;
    
    bool build()
    {
        if (this.missionType == "race")
        {
            return this.buildRace();
        }
        else
        {
            game.log("MissionBuilder: cannot load mission - unknown type '"+this.missionType+"'");
            return false;
        }
    }
    
    // --------------
    // Mission types:
    
    bool buildRace()
    {
        // Procedural roads
        this.buildProceduralRoads();
        
        // Race checkpoints
        if (this.checkpointObjName == "")
            this.checkpointObjName = "chp-checkpoint";
        if (this.startObjName == "")
            this.startObjName = "chp-start"; // A "start/finish" sign
        if (this.finishObjName == "")
            this.finishObjName = "chp-start"; // A "start/finish" sign            
        this.raceID = races.addRace(
            this.missionName, this.checkpoints, this.raceLaps,
            this.checkpointObjName, this.startObjName, this.finishObjName, "");
        races.finalize(); // Load race times from cache
        
        return true;
    }
    
    // --------------
    // Utilities:
    
    void buildProceduralRoads()
    {
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roadManager = terrain.getProceduralManager();
        for (uint i = 0; i < this.proceduralRoads.length(); i++)
        {
            roadManager.addObject(this.proceduralRoads[i]);
        }
    }
}



