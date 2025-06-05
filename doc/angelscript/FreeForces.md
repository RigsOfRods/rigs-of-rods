AngelScript: FreeForces (physics affectors)                     {#FreeForcesPage}
===========================================

FreeForces are global, persistent forces which act upon one node of one actor each. They exist separately from actors and their existing hook/tie/rope interactions. This means they do not interfere with actor N/B or any of the existing linking logic. They're intended for creating special physics effects which aren't supported otherwise, like making party baloons fly. They're intentionally only accessible from scripts to maximize their flexibility and limit maintenance of the codebase.

![desperadoBaloonsV2](https://github.com/RigsOfRods/rigs-of-rods/assets/491088/61d1763d-5f3e-4263-97cd-972ca1701080)

To manage FreeForces, you use `game.pushMessage()` with `MSG_SIM_ADD_FREEFORCE_REQUESTED`, `MSG_SIM_MODIFY_FREEFORCE_REQUESTED` or `MSG_SIM_REMOVE_FREEFORCE_REQUESTED`. 

Parameters common to ADD/MODIFY requests:
* 'id' (int) - unique ID of this freeforce, use `game.getFreeForceNextId()` to obtain an unique sequential ID.
* 'type' (FreeForceType) - One of `FREEFORCETYPE_DUMMY`, `FREEFORCETYPE_CONSTANT`, `FREEFORCETYPE_TOWARDS_COORDS`, `FREEFORCETYPE_TOWARDS_NODE`
* 'base_actor' (int) - Unique actor instance ID, obtain it by `game.getCurrentTruck().getInstanceId()`, or if you're in an actor-script, then you can do `thisActor.getInstanceId()` // `BeamClass@ thisActor`is automatic global variable for scripts bound to actors.
* 'base_node' (int) - Node number (0 - 65535)
* 'force_magnitude' (float) - the amount of force to apply, in Newton/meters Parameters for `FREEFORCETYPE_CONSTANT`:
* 'force_const_direction' (vector3) - constant force direction. Note that  Y=up, so to make things float, use vector3(0, 1, 0). 

Additional parameters for `FREEFORCETYPE_TOWARDS_COORDS`:
* 'target_coords' (vector3) - world position to target. For example `game.getPersonPosition()` will target it at position where character is standing AT THE MOMENT. 

Additional parameters for `FREEFORCETYPE_TOWARDS_NODE`:
* 'target_actor' (int) - Unique actor instance ID.
* 'target_node' (int) - Node number (0 - 65535).

Note when any of the actors involved in a FreeForce (be it the base one or target one), the freeforce is deleted.

Example from the PartyBaloon demo mod:
```
    // Create the up-force
    upforce_assigned_id = game.getFreeForceNextId();
    game.pushMessage(MSG_SIM_ADD_FREEFORCE_REQUESTED, {
        {'id', upforce_assigned_id },
        {'type', FREEFORCETYPE_CONSTANT },
        {'base_actor', thisActor.getInstanceId() },  // `BeamClass@ thisActor`is automatic global variable for scripts bound to actors.
        {'base_node', UPFORCE_NODE },
        {'force_const_direction', vector3(0, 1, 0) }, // Y=up
        {'force_magnitude', 0.4f } // Newton/meters
    });
```

Keep in mind that the physics simulation runs at 2khz (2000 steps per second), while scripts run in sync with FPS. That's why there's no option to add temporary/one-off force impulses from scripts - you are responsible for adding the persistent force, monitoring and adjusting it as needed, and then removing it when no longer necessary. You can create the forces as dummy and then change their type later as needed.

Below is the demo PartyBaloon mod. It weights only 12 grams (0.012 Kg) and under our simulation it moves very slowly, not bypasing a certain threshold, regardless of pull force applied to it. By default, it will just fly off (you can grab it with mouse) but you can use console variables to bind it somewhere:
![obrazek](https://github.com/RigsOfRods/rigs-of-rods/assets/491088/1cc90f9b-a26d-497e-aee6-237a3e75b315)
The script bundled with the baloon will read these variables and create an additional FreeForce towards either the spawn position or given node on given actor.
[Download PartyBaloon-12grams.zip](https://github.com/user-attachments/files/15875016/PartyBaloon-12grams.zip)

**Note about a quirk:** Chaining object access like `game.getCurrentTruck().getInstanceId()` will not work. 
You must do either `game.getCurrentTruck().getHandle().getInstanceId()` or `BeamClass@ b = game.getCurrentTruck(); b.getInstanceId()` 
because the return type isn't `BeamClass@` but rather `BeamClassPtr@`.
This is a quirk of our present reference-counting mechanism. More info is at https://github.com/ohlidalp/RefCountingObject-AngelScript. 

This page is archived from [original GitHub PR](https://github.com/RigsOfRods/rigs-of-rods/pull/3159) and respective commit messages.
