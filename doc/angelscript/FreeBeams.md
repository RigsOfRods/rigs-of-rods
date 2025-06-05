AngelScript: FreeBeams (FreeForces type family `HALFBEAM_*`)                     {#FreeBeamsPage}
============================================================

This extends the [FreeForces system](FreeForces.md) to be able to behave exactly like `beams` in truck fileformat.
2 new freeforce types are available: `HALFBEAM_GENERIC` and `HALFBEAM_ROPE`.
Because one freeforce can only affect one node (strict rule), to fully simulate a beam, you need 2 equivalent `HALFBEAM_*` freeforces in opposite directions.
For example of use, see the new script 'example_freeforce_halfbeam.as'. _TIP: To make this GIF, I had to stop the engine on the other DAF otherwise it refused to move even with handbrake off._
![halfbeams_DAFs](https://github.com/user-attachments/assets/18f2c5db-9037-48e0-a0c0-f4bd244d25b1)

To diagnose deforming and breaking of these freeforces, a new script event `SE_GENERIC_FREEFORCES_ACTIVITY` was added, with these activity types:
```
/// Argument https://github.com/RigsOfRods/rigs-of-rods/issues/1 of script event `RoR::SE_GENERIC_FREEFORCES_ACTIVITY`
enum freeForcesActivityType
{
    FREEFORCESACTIVITY_NONE,

    FREEFORCESACTIVITY_ADDED,
    FREEFORCESACTIVITY_MODIFIED,
    FREEFORCESACTIVITY_REMOVED,

    FREEFORCESACTIVITY_DEFORMED, //!< Only with `HALFBEAM_*` types; arg #5 (string containing float) the actual stress, arg #6 (string containing float) maximum stress.
    FREEFORCESACTIVITY_BROKEN, //!< Only with `HALFBEAM_*` types; arg #5 (string containing float) the applied force, arg #6 (string containing float) breaking threshold force.
};
```
When adding a HALFBEAM, you use the same arguments as with TOWARDS_NODE, all 'set_beam_defaults' params are optional with the same defaults as in truck file:
```
    game.pushMessage(MSG_SIM_ADD_FREEFORCE_REQUESTED, {
    {'id', id},
    {'type', FREEFORCETYPE_HALFBEAM_GENERIC},
    {'force_magnitude', 0.f}, // unused by HALFBEAM but still required.
    {'base_actor', ffBaseActorId},
    {'base_node', ffBaseNodeNum},
    {'target_actor', ffTargetActorId},
    {'target_node', ffTargetNodeNum}
// 'set_beam_defaults' params are optional with the same defaults as in truck file:
// * 'halfb_spring'
// * 'halfb_damp'
// * 'halfb_strength' ~ breaking threshold
// * 'halfb_deform' ~ deformation threshold.
// * 'halfb_plastic_coef'
// * 'halfb_diameter' ~ Visual diameter in meters - for future use.
    });
```

Like any freeforce, freebeams are invisible by default, but you can archieve the same look and feel of the classic beam visuals. Because they exist outside of any actor, a whole new code and setup logic was needed.
There are 3 new messages:
```
    MSG_EDI_ADD_FREEBEAMGFX_REQUESTED,     //!< Payload = RoR::FreeBeamGfxRequest* (owner)
    MSG_EDI_MODIFY_FREEBEAMGFX_REQUESTED,  //!< Payload = RoR::FreeBeamGfxRequest* (owner)
    MSG_EDI_DELETE_FREEBEAMGFX_REQUESTED,  //!< Payload = RoR::FreeBeamGfxID_t* (owner)
```
All are pushable via `game.pushMessage()`.
Like with freeforces, you need to manually supply ID when creating/modifying freebeamGfx - use `game.getFreeBeamGfxNextId()`
The example script 'example_freeforce_halfbeam.as' was updated to add the freebeamGfx, and also to enter 'set_beam_defaults' values (for the sake of demo).

This page is archived from [Original GitHub PR](https://github.com/RigsOfRods/rigs-of-rods/pull/3285) and respective commit messages.
