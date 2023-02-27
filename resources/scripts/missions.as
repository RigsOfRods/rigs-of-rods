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

// Mission type includes:
#include "races.as"
#include "stunts.as"
// Utility includes:
#include "road_utils.as"

// Mandatory global variable, referenced by race callbacks:
//  - raceEvent(int, string, string, int)
//  - raceCancelPointHandler(int, string, string, int)
//  - eventCallback(int, int) *installed using `addFunction()`*
racesManager@ races = null;
// Other managers:
StuntsManager@ stunts = null;

class MissionManager
{
    MissionBuilder@ loadedMission; // currently just 1
    int uniqueIdCounter = 0;

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
                // Stunt properties
                else if (reader.getTokKeyword() == "stunt_start_obj_instance")
                {
                    mission.stuntStartObjInstance = reader.getTokString(1);
                }
                else if (reader.getTokKeyword() == "stunt_start_box_name")
                {
                    mission.stuntStartBoxName = reader.getTokString(1);
                }
                else if (reader.getTokKeyword() == "stunt_end_obj_instance")
                {
                    mission.stuntEndObjInstance = reader.getTokString(1);
                }
                else if (reader.getTokKeyword() == "stunt_end_box_name")
                {
                    mission.stuntEndBoxName = reader.getTokString(1);
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
                // Static terrain objects
                else if (reader.getTokKeyword() == "begin_terrain_objects")
                {
                    while (!reader.endOfFile() && (reader.tokenType() != TOKEN_TYPE_KEYWORD || reader.getTokKeyword() != "end_terrain_objects"))
                    {
                        if (
                            (reader.tokenType(0) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(1) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(2) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(3) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(4) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(5) == TOKEN_TYPE_NUMBER)
                            && (reader.tokenType(6) == TOKEN_TYPE_STRING))
                        {
                            TerrainObject sobj;
                            sobj.position = vector3(reader.getTokFloat(0), reader.getTokFloat(1), reader.getTokFloat(2));
                            sobj.rotation = vector3(reader.getTokFloat(3), reader.getTokFloat(4), reader.getTokFloat(5));
                            sobj.odefName = reader.getTokString(6);
                            if ((reader.tokenType(7) == TOKEN_TYPE_STRING))
                            {
                                sobj.type = reader.getTokString(7);
                                if ((reader.tokenType(8) == TOKEN_TYPE_STRING))
                                {
                                    sobj.instanceName = reader.getTokString(8);
                                }                                 
                            }
                            // An unique instance name is needed for proper unloading later.
                            // If none or empty specified in .mission file, generate one.
                            if (sobj.instanceName == "")
                            {
                                sobj.instanceName = filename + "/" + this.uniqueIdCounter++;
                            }
                            mission.terrainObjects.insertLast(sobj);
                        }
                        else if ((reader.tokenType(0) != TOKEN_TYPE_LINEBREAK) && (reader.tokenType(0) != TOKEN_TYPE_COMMENT))
                        {
                            game.log("MissionManager: Warning - skipping invalid static-objects line");
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
        game.log ("DEBUG  MissionManager::loadMission() - setting `loadedMission`");
        @loadedMission = @mission;
        return true;
    }
    
    void unloadMission()
    {
        game.log ("DEBUG >> MissionManager::unloadMission()");
        if (@loadedMission != null)
        {
            loadedMission.destroy();
            game.pruneCollisionElements();
        }
        else
        {
            game.log ("DEBUG  MissionManager::unloadMission() - `loadedMission` is null!");
        }
        game.log ("DEBUG << MissionManager::unloadMission()");
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
    array<TerrainObject@> terrainObjects;
    
    // mission type specific properties:
    // note: These will be refactored to dedicated builders, like RaceMissionBuilder and StuntMissonBuilder, after the prototype phase is concluded.
    
    // Race properties
    array<array<double>> checkpoints; // The format used by 'races.as': {'posX, posY, posZ, rotX, rotY, rotZ'}
    string checkpointObjName;
    string startObjName;
    string finishObjName;
    int raceLaps = 0; // -1=NoLaps, 0=Unlimited, 1=One
    int raceID = -1;
    
    // Stunt properties
    string stuntStartObjInstance;
    string stuntStartBoxName;
    string stuntEndObjInstance;
    string stuntEndBoxName;
    int stuntID = -1;
    
    bool build()
    {
        if (this.missionType == "race")
        {
            return this.buildRace();
        }
        else if (this.missionType == "stunt")
        {
            return this.buildStunt();
        }        
        else
        {
            game.log("MissionBuilder: cannot load mission - unknown type '"+this.missionType+"'");
            return false;
        }
    }
    
    void destroy()
    {
        game.log ("DEBUG >> MissionBuilder::destroy()");
        this.destroyTerrainObjects();
        this.destroyProceduralRoads();
        if (this.raceID != -1)
        {
            races.deleteRace(this.raceID);
        }
        if (this.stuntID != -1)
        {
            stunts.deleteStunt(this.stuntID);
        }
        game.log ("DEBUG << MissionBuilder::destroy()");
    }
    
    // --------------
    // Mission types:
    // Note: These will be refactored to dedicated builders, like RaceMissionBuilder and StuntMissonBuilder, after the prototype phase is concluded.
    
    bool buildRace()
    {
        // Make sure racemanager exists
        if (@races == null)
            @races = racesManager();
    
        // Prep terrain
        this.spawnTerrainObjects();
        this.spawnProceduralRoads();
        
        // Race checkpoints - set defaults
        if (this.checkpointObjName == "")
            this.checkpointObjName = "chp-checkpoint";
        if (this.startObjName == "")
            this.startObjName = "chp-start"; // A "start/finish" sign
        if (this.finishObjName == "")
            this.finishObjName = "chp-start"; // A "start/finish" sign    

        // Create the race
        this.raceID = races.addRace(
            this.missionName, this.checkpoints, this.raceLaps,
            this.checkpointObjName, this.startObjName, this.finishObjName, "");
        races.finalize(); // Load race times from cache
        
        return true;
    }
    
    bool buildStunt()
    {
        game.log("DEBUG >> MissionBuilder.buildStunt()");
    
        // make sure stuntmanager exits
        if (@stunts == null)
            @stunts = StuntsManager();
          
        // Prep terrain
        this.spawnTerrainObjects();
        this.spawnProceduralRoads();

        // Create the stunt
        this.stuntID = stunts.addStunt(
            this.missionName,
            this.stuntStartObjInstance, this.stuntStartBoxName,
            this.stuntEndObjInstance, this.stuntEndBoxName);
            
        game.log("DEBUG << MissionBuilder.buildStunt()");
        return true;
    }
    
    // --------------
    // Utilities:
    
    void spawnProceduralRoads()
    {
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roadManager = terrain.getProceduralManager();
        for (uint i = 0; i < this.proceduralRoads.length(); i++)
        {
            roadManager.addObject(this.proceduralRoads[i]);
        }
    }
    
    void destroyProceduralRoads()
    {
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roadManager = terrain.getProceduralManager();
        for (uint i = 0; i < this.proceduralRoads.length(); i++)
        {
            roadManager.removeObject(this.proceduralRoads[i]);
        }
    }
    
    void spawnTerrainObjects()
    {
        for (uint i = 0; i < this.terrainObjects.length(); i++)
        {
            game.spawnObject(
                this.terrainObjects[i].odefName,
                this.terrainObjects[i].instanceName,
                this.terrainObjects[i].position,
                this.terrainObjects[i].rotation,
                "", false);
        }
    }
    
    void destroyTerrainObjects()
    {
        for (uint i = 0; i < this.terrainObjects.length(); i++)
        {
            game.destroyObject(this.terrainObjects[i].instanceName);
        }
    }    
}

// Helper class - represents a line in TOBJ file - can be a static object (ODEF) or an actor (TRUCK/FIXED).
class TerrainObject
{
    vector3 position;
    vector3 rotation;
    string odefName;
    string type;
    string instanceName;
}
