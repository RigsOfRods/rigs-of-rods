AngelScript: OGRE3D renderer bindings                  {#Ogre3DBindingsPage}
=====================================

The strong point of OGRE renderer is that the internal API is very well organized and documented. 
To display a 3D model in scene, you load `Ogre::Mesh`, use `Ogre::SceneManager` to create `Ogre::Entity` of the mesh and attach it to `Ogre::SceneNode`. 
To do the skeletal anim, you get `Ogre::AnimationState` from the entity and that's it - you call `setTimePosition()` or `addTime()` to play it. It's all in the docs: https://ogrecave.github.io/ogre/api/1.11/. 
OGRE also allows user to enumerate all it's objects and traverse hierarchies. 
Basically this is something modders could easily do directly, if it just wasn't C++.
The AngelScript bindings mimic the C++ side almost indistinguishably.
Note that to separate C++ from AngelScript in this [Doxygen](https://doxygen.nl/) documentation, the namespace is spelled AngelOgre.

## Scene traversal

In OGRE, everything you can render is an `Ogre::MovableObject` - most often 'Entity' but also 'ManualObject', 'ParticleSystem', 'BillboardSet' etc. It also controls the visibility and shadows, so I added checkboxes for it.

You can use the 'example_ogre_inspector.as' script to view our scene graph - it's pretty well organized:
![obrazek](https://user-images.githubusercontent.com/491088/227726352-89baa7f9-6a1d-469f-bdf2-7a6ef00d27ef.png)

## Displaying 3d models

You can load/position/rotate/animate an arbitrary mesh from just AngelScript.
Use `Ogre::SceneManager` to create `Ogre::Entity` given a mesh name, you can also set material by name.
To display and move the entity in scene, you need to create `Ogre::SceneNode` from the manager and attach the entity to it.

## Skeletal animation playback.

To do the skeletal anim, you get `Ogre::AnimationState` from the entity and that's it - you call `setTimePosition()` or `addTime()` to play it.
![obrazek](https://user-images.githubusercontent.com/491088/227746629-1cabfd62-06db-4729-887d-e267c83f5b80.png)

## Manual mesh creation

Relevant script API:
* `enum Ogre::RenderOperation`
* `class Ogre::ManualObject` REF | NOCOUNT object type. This is castable to MovableObject. Official tutorial for Ogre::ManualObject: https://ogrecave.github.io/ogre/api/latest/manual-mesh-creation.html
* Functions in OGRE SceneManager: `createManualObject()`, `getManualObject()`, `destroyManualObject()`
* Extra feature: function `TerrainClass::getHeightAt()` for the MeshedConcrete example to work.
* To save the generated mesh as §.mesh§ file, use `game.serializeMeshResource()`

The screenshot below showcases an example script 'example_ogre_MeshedConcrete.as' (run it using `loadscript` command in console) which shows how to generate a mesh which follows the shape of the terrain, effectivelly forming a meshed decal. The material used in the example is "taxiwayconcrete" but you can use any material.
![obrazek](https://github.com/RigsOfRods/rigs-of-rods/assets/491088/8a0501c0-5052-4ea0-b0bb-5923c2e0c0c4)

## Image processing (texture blitting)

It's possible to edit textures via AngelScript. This literally modifies the pixels in the texture by specifying source image and destination box. OGRE can do more but this is a start.

Angelscript API:
* `game.loadImageResource(filename, rg)` - see GameScript.cpp & GameScriptAngelscript.cpp
* `ImDrawList::AddImage()` - see ImGuiAngelscript.cpp
* Ogre objects: `box; PixelBox; HardwarePixelBufferSharedPtr`, Texture functions `getBuffer(); getNumMipmaps()`

The screenshot showcases a new example script 'example_ogre_textureBlitting.as' - you can try it out by opening console (Hotkey ~ or topMenubar->tools->ShowConsole) and using `loadscript` command. NOTE: the example script only blits to topmost mipmap, so to see the effect, you must be close to the object! This will be improved.
![obrazek](https://github.com/RigsOfRods/rigs-of-rods/assets/491088/ed554830-333d-4bb2-a693-f392f3bf69c2)

Example code:
```
    // src image
    Ogre::Image gSrcImg = game.loadImageResource("sign-roadnarrows.dds", "TexturesRG");
    // dst texture
    Ogre::TexturePtr gDstTex = Ogre::TextureManager::getSingleton().load("character.dds", "TexturesRG");
    // let the MAGIC happen
    Ogre::HardwarePixelBufferPtr pixbuf = gDstTex.getBuffer(/*cubemap face index:*/0, /*mipmap:*/0);
    Ogre::PixelBox srcPixbox = gSrcImg.getPixelBox(0,0); // getPixelBox(cubemapFaceIndex, mipmapIndex);
    box gDstBox; // where you want to put it.
    pixbuf.blitFromMemory(srcPixbox, gDstBox);
```

## Vertex data reading

This is done via game-specific helper functions because the raw vertex/index buffer API is involved and most importantly relies on retyping void* pointers which isn't a thing in AngelScript.

Relevant script API:
* objects: `MeshPtr, SubMesh, MeshManager`
* to get vert positions, use "array<vector3>@ __getVertexPositions()" on SubMesh
* ditto for vert texcoords, note the extra index param because mesh can have more than 1 texcoord layer: "array<vector2>@ __getVertexTexcoords(uint index)"

See example script "example_ogre_vertexData.as" (on screenshot) - notice the texture contains a complete face but only one half of it is covered in texcoords. This isn't a bug in the tool but a quirk of the mesh - it actually uses that one half of texture to cover both halves of the face.
![obrazek](https://github.com/RigsOfRods/rigs-of-rods/assets/491088/591ed02e-0206-42ec-a8c0-87cd73b8cbca)

## Acess to shader parameters

Script API:
* `BeamClass.getManagedMaterialNames()` -> `array<string>@`
* `BeamClass.getManagedMaterialInstance()` -> `Ogre::MaterialPtr`
* `Ogre::Pass.__getNamedConstants()` -> `array<string>@`
* `Ogre::Pass.getFragmentProgramParameters()` -> `Ogre::GpuProgramParametersPtr`
