// \title example - OGRE - ManualObject
// \brief shows how to make custom mesh and place it to scene
// ===================================================


array<    Ogre::ManualObject@ > gManualObjects;
Ogre::SceneNode@ gGroupingSceneNode = game.getSceneManager().getRootSceneNode().createChildSceneNode(makeID(""));

string makeID(string name)
{
    // to avoid clash with leftover scene nodes created before, we include the NID in the name - using automatic global var `thisScript`.
    return "example-OGRE-ManuaObject.as(NID:"+thisScript+"): "+name;
}

Ogre::ManualObject@ makePlane(string meshName, string matName)
{
    Ogre::ManualObject@ mo  = game.getSceneManager().createManualObject(meshName);
    mo.begin(matName, Ogre::OT_TRIANGLE_LIST, game.getTerrain().getHandle().getTerrainFileResourceGroup());
    // verts
    mo.position(vector3(-1, -1, -1));
    mo.textureCoord(vector2(0,0));
    mo.position(vector3(-1, -1, 1));
    mo.textureCoord(vector2(0,1));
    mo.position(vector3(1, -1, 1));
    mo.textureCoord(vector2(1,1));
    mo.position(vector3(1, -1, -1));
    mo.textureCoord(vector2(1,0));
    //  tris
    mo.index(0); mo.index(1); mo.index(2);
    mo.index(0); mo.index(2); mo.index(3);
    
    mo.end();
    return mo;
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    ImGui::Text("Num manual objects: "+gManualObjects.length());
    
    if (ImGui::Button("make plane at character position"))
    {
        string moname = makeID("mo-"+gManualObjects.length());
        string matname = "sign-autostrasse";
        Ogre::ManualObject@ mo  = makePlane(moname, matname);
        Ogre::SceneNode@ snode = gGroupingSceneNode.createChildSceneNode(moname+"-node");
        snode.attachObject(cast<Ogre::MovableObject@>(mo));
        snode.setPosition(game.getPersonPosition()+vector3(0,2,0));
        gManualObjects.insertLast(mo);
    }
    
    
}
