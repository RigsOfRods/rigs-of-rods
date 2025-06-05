AngelScript: Procedural Roads                        {#ProceduralRoadsPage}
=============================

This introduces scripting API for full procedural road management and also bundled script "road_editor.as" which allows interactive road editing!

![screenshot_2022-11-28_12-26-39_1](https://user-images.githubusercontent.com/491088/204269590-8e6ab222-d7c0-4aa2-8c0d-7def57748339.png)

## Features

* add or remove roads
* add or remove road points
* move existing points using mouse
* add new points at character position
* export a specific range of road points as AI waypoints
* import a specific range of AI waypoints as road points
* save the created roads to .TOBJ files
* road smoothing by specifying number of extra splits
* Hotkeys: ROAD_EDITOR_{POINT_ADD, POINT_DELETE, POINT_SET_POS, POINT_GOTO, REBUILD_MESH} - all shown in the UI.
* select road segment type (pillars, borders...)
* edit all available parameters, see https://docs.rigsofrods.org/terrain-creation/old-terrn-subsystem/#procedural-roads
* adjustable min height above terrain
* editable point height above terrain

You can also script custom meshes using `addQuad()` and `addCollisionQuad()`.

## API

The central object is TerrainClass, obtainable by `game.getTerrain()`.
Historically we already have a lot of terrain-related functions in the `GameScriptClass` (global object `game`), see https://developer.rigsofrods.org/dc/d63/class_script2_game_1_1_game_script_class.html, but under the hood those are serviced by `RoR::Terrain` anyway.
Please mind the additional `getHandle()` function in the code snippets - that's a necessary evil to manage AngelScript reference counting simply and safely using https://github.com/only-a-ptr/RefCountingObject-AngelScript. Without it, adding new script feats is either laborous and bug-prone, or unsafe.

Road specific objects: ProceduralPointClass, ProceduralRoadClass, ProceduralObjectClass, ProceduralManagerClass.

## Road editor script

To start the editor script, open console and say "loadscript road_editor.as".

To export road points as AI waypoints, use 2 sliders to specify a range, and press an export button:
![screenshot_2022-11-29_01-07-15_1](https://user-images.githubusercontent.com/491088/204407665-b9319949-2a33-485c-9b9e-989718542da5.png)

There's also an import of AI waypoints as procedural road points, with the same controls:
![screenshot_2022-11-29_00-59-10_1](https://user-images.githubusercontent.com/491088/204408301-caa23cac-bb90-416c-86c4-242487269a48.png)

Hotkeys:
```
    // Road editing
    {"ROAD_EDITOR_POINT_INSERT",        "Keyboard EXPL+INSERT",         _LC("InputEvent", "insert road point") },
    {"ROAD_EDITOR_POINT_GOTO",            "Keyboard EXPL+G",              _LC("InputEvent", "go to road point") },
    {"ROAD_EDITOR_POINT_SET_POS",      "Keyboard EXPL+M",              _LC("InputEvent", "set road point position") },
    {"ROAD_EDITOR_POINT_DELETE",        "Keyboard EXPL+DELETE",         _LC("InputEvent", "delete road point") },
    {"ROAD_EDITOR_REBUILD_MESH",        "Keyboard EXPL+B",              _LC("InputEvent", "regenerate road mesh") },
```
<img width="1705" alt="finished" src="https://user-images.githubusercontent.com/491088/205351675-0e35c5f0-cf65-478c-b04e-fe8d7b07ee95.png">

This page is archived from GitHub PRs [#2976](https://github.com/RigsOfRods/rigs-of-rods/pull/2976) and [#2971](https://github.com/RigsOfRods/rigs-of-rods/pull/2971)

