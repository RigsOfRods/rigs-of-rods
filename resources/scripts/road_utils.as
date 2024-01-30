/*
* Helpers for working with procedural roads
* See https://docs.rigsofrods.org/terrain-creation/terrn2-subsystem/#procedural-roads
* The objects used are defined by the game API:
* - `ProceduralObjectClass` - https://developer.rigsofrods.org/df/d88/class_script2_game_1_1_procedural_object_class.html
* - `ProceduralPointClass` - https://developer.rigsofrods.org/df/d16/struct_script2_game_1_1_procedural_point_class.html
*/

/**
* Parses a procedural road from TOBJ or MISSION file.
* @param reader Must be positioned on the 'begin_procedural_road' keyword, otherwise function returns null and leaves reader unchanged.
* @return on success, returns the road and leaves reader on 'end_procedural_road'. On unexpected end-of-file, returns null and leaves reader at the end-of-file.
*/
ProceduralObjectClass@ ParseProceduralRoadFromFile(GenericDocReaderClass@ reader)
{
    if (reader.getTokKeyword() == "begin_procedural_roads")
    {
        ProceduralObjectClass@ road = ProceduralObjectClass();
        reader.seekNextLine();
        while (!reader.endOfFile() && (reader.tokenType() != TOKEN_TYPE_KEYWORD || reader.getTokKeyword() != "end_procedural_roads"))
        {
            if ((reader.tokenType() == TOKEN_TYPE_KEYWORD) 
                && (reader.getTokKeyword() == "smoothing_num_splits"))
            {
                road.smoothing_num_splits = int(reader.getTokFloat(1));
            }
            else
            {
                ProceduralPointClass@ point = ParseProceduralPointLine(reader);
                if (@point != null)
                {
                    road.addPoint(point);
                }
                else
                {
                    game.log("ParseProceduralRoadFromFile(): Warning - skipping bad procedural point line");
                }
            }
            
            reader.seekNextLine();
        }
        
        // Finalize
        if (!reader.endOfFile())
        {
            return road;
        }
        else
        {
            game.log("ParseProceduralRoadFromFile(): Fatal error - unexpected end of file");
        }
    }
    else
    {
        game.log("ParseProceduralRoadFromFile(): Fatal error - no 'begin_procedural_road'");
    }
    return null;
}

/**
* Parses one line from procedural road definition in TOBJ/MISSION file.
* @param reader Must be positioned at the start of the line.
* @return null if the line could not be parsed (bad syntax or unexpected end-of-file).
*/
ProceduralPointClass@ ParseProceduralPointLine(GenericDocReaderClass@ reader)
{
    if ( //Syntax: 'position x,y,z rotation rx,ry,rz, width, border width, border height, type'
        (reader.tokenType(0) == TOKEN_TYPE_NUMBER)
        && (reader.tokenType(1) == TOKEN_TYPE_NUMBER)
        && (reader.tokenType(2) == TOKEN_TYPE_NUMBER)
        && (reader.tokenType(3) == TOKEN_TYPE_NUMBER)
        && (reader.tokenType(4) == TOKEN_TYPE_NUMBER)
        && (reader.tokenType(5) == TOKEN_TYPE_NUMBER)
        && (reader.tokenType(6) == TOKEN_TYPE_NUMBER)
        && (reader.tokenType(7) == TOKEN_TYPE_NUMBER)
        && (reader.tokenType(8) == TOKEN_TYPE_NUMBER)
        && (reader.tokenType(9) == TOKEN_TYPE_STRING)
        )
    {
        ProceduralPointClass@ point = ProceduralPointClass();
        
        point.position.x = reader.getTokFloat(0);
        point.position.y = reader.getTokFloat(1);
        point.position.z = reader.getTokFloat(2);
        
        vector3 rot;
        rot.x = reader.getTokFloat(3);
        rot.y = reader.getTokFloat(4);
        rot.z = reader.getTokFloat(5);
        point.rotation = CalcProceduralPointRotation(rot);        
        
        point.width = reader.getTokFloat(6);
        point.border_width = reader.getTokFloat(7);
        point.border_height = reader.getTokFloat(8);
        
        // See `TObjParser::ProcessProceduralLine()` - https://github.com/RigsOfRods/rigs-of-rods/blob/master/source/main/resources/tobj_fileformat/TObjFileFormat.cpp#L201-L224
        string typeStr = reader.getTokString(9);
             if (typeStr == "flat"             ) { point.type = ROAD_FLAT;  }
        else if (typeStr == "left"             ) { point.type = ROAD_LEFT;  }
        else if (typeStr == "right"            ) { point.type = ROAD_RIGHT; }
        else if (typeStr == "both"             ) { point.type = ROAD_BOTH;  }
        else if (typeStr == "bridge"           ) { point.type = ROAD_BRIDGE;    point.pillar_type = 1; }
        else if (typeStr == "monorail"         ) { point.type = ROAD_MONORAIL;  point.pillar_type = 2; }
        else if (typeStr == "monorail2"        ) { point.type = ROAD_MONORAIL;  point.pillar_type = 0; }
        else if (typeStr == "bridge_no_pillars") { point.type = ROAD_BRIDGE;    point.pillar_type = 0; }
        else                                     { point.type = ROAD_AUTOMATIC; point.pillar_type = 0; }
        
        return point;
    }
    return null;
}

quaternion CalcProceduralPointRotation(vector3 rot)
{
    // See `TOBjParser::CalcRotation` - https://github.com/RigsOfRods/rigs-of-rods/blob/master/source/main/resources/tobj_fileformat/TObjFileFormat.cpp#L344-L349
    return quaternion(degree(rot.x).valueRadians(), vector3(1,0,0))
         * quaternion(degree(rot.y).valueRadians(), vector3(0,1,0))
         * quaternion(degree(rot.z).valueRadians(), vector3(0,0,1));    
}