// \title example - OGRE - MeshedConcrete
// \brief shows how to make custom mesh along the ground
// ===================================================


array<    Ogre::ManualObject@ > gManualObjects;
Ogre::SceneNode@ gGroupingSceneNode = game.getSceneManager().getRootSceneNode().createChildSceneNode(makeID(""));
int gNumTilesX = 2;
int gNumTilesZ = 2;
float gTileSizeX = 1.f;
float gTileSizeZ = 1.f;
float gHeightBias = 0.1; // 10cm above ground
bool gDebugLog = true;

string makeID(string name)
{
    // to avoid clash with leftover scene nodes created before, we include the NID in the name - using automatic global var `thisScript`.
    return "example-OGRE-ManuaObject.as(NID:"+thisScript+"): "+name;
}

Ogre::ManualObject@ makeGrass(string meshName, string matName)
{
    Ogre::ManualObject@ mo  = game.getSceneManager().createManualObject(meshName);
    mo.begin(matName, Ogre::OT_TRIANGLE_LIST, game.getTerrain().getHandle().getTerrainFileResourceGroup());
    
    TerrainClass@ terrn = game.getTerrain();
    int numVerts = 0;
    vector3 startPos = game.getPersonPosition() - vector3(gTileSizeX*(gNumTilesX/2), 0, gTileSizeZ*(gNumTilesZ/2));
    float tileSizeU = (1.f/gNumTilesX); // texcoords, aka UV coords
    float tileSizeV = (1.f/gNumTilesZ);    
    for (int x = 0; x < gNumTilesX; x++)
    {
        for (int z = 0; z < gNumTilesZ; z++)
        {
        if (gDebugLog) { game.log("DBG: adding tile X="+x+"/"+gNumTilesX+" Z="+z+"/"+gNumTilesZ); }
            
            // world horizontal position
            float tileX = x*gTileSizeX + startPos.x;
            float tileZ = z*gTileSizeZ + startPos.z;
            // texcoords, aka UV coords
            float tileU = x*tileSizeU;
            float tileV = z*tileSizeV;
            
        if (gDebugLog){game.log("DBG \tTileX="+tileX+" tileZ="+tileZ+" tileU="+tileU+" tileV="+tileV);}
            
            // verts
            mo.position(vector3 (tileX, terrn.getHeightAt(tileX, tileZ)+gHeightBias, tileZ));
            mo.textureCoord(vector2(tileU, tileV));
            
            mo.position(vector3 (tileX, terrn.getHeightAt(tileX, tileZ+gTileSizeZ)+gHeightBias, tileZ+gTileSizeZ));
            mo.textureCoord(vector2(tileU, tileV+tileSizeV));            
            
            mo.position(vector3 (tileX+gTileSizeX, terrn.getHeightAt(tileX+gTileSizeX, tileZ+gTileSizeZ)+gHeightBias, tileZ+gTileSizeZ));
            mo.textureCoord(vector2(tileU+tileSizeU, tileV+tileSizeV));
            
            mo.position(vector3 (tileX+gTileSizeX, terrn.getHeightAt(tileX+gTileSizeX, tileZ)+gHeightBias, tileZ));
            mo.textureCoord(vector2(tileU+tileSizeU, tileV));            
            
            //  tris
            mo.index(numVerts+0); mo.index(numVerts+1); mo.index(numVerts+2);
            mo.index(numVerts+0); mo.index(numVerts+2); mo.index(numVerts+3);
            
            numVerts+= 4;
        }
    }
    
    mo.end();
    return mo;    
    
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    ImGui::TextDisabled("MESHED CONCRETE EXAMPLE");
    ImGui::TextDisabled("Generates one-layer mesh along ground");
    ImGui::TextDisabled("Remember, XZ are horizontal, Y is up.");
    
    ImGui::Separator();    
    
    ImGui::Text("Num manual objects: "+gManualObjects.length());
    
    
    ImGui::InputInt("Num tiles X", gNumTilesX);
    ImGui::InputInt("Num tiles Z", gNumTilesZ);
    ImGui::InputFloat("Tile size X", gTileSizeX);
    ImGui::InputFloat("Tile size Z", gTileSizeZ);
    ImGui::InputFloat("Height bias (m)", gHeightBias);
    ImGui::Checkbox("Debug logging", gDebugLog);
    
    if (ImGui::Button("add mesh"))
    {
        string moname = makeID("mo-"+gManualObjects.length());
        string matname = "taxiwayconcrete";
        Ogre::ManualObject@ mo  =   makeGrass(moname, matname);
        Ogre::SceneNode@ snode = gGroupingSceneNode.createChildSceneNode(moname+"-node");
        snode.attachObject(cast<Ogre::MovableObject@>(mo));
        // unlike the ManualObject example, we don't position the node - verts are already in world position.
        gManualObjects.insertLast(mo);
    }
    
    
    
}
