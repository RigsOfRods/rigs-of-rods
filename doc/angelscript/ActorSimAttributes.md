AngelScript: ActorSimAttributes (live tinkering system)               {#ActorSimAttributesPage}
=======================================================

Allows advanced users to set actor physics attributes directly, including some not accessible from .truck file format.

In AngelScript, the actor (BeamClass) got 2 new methods:
```
    void setSimAttribute(ActorSimAttr attr, float val);
    float getSimAttribute(ActorSimAttr attr);
```
The set* function always logs _"[RoR|Actor] setSimAttribute: '{attr}' = {val}"_, where {attr} is the attribute name as string.

See available settings in Game2Script::ActorSimAttr enum.


Tip: To set this value automatically on each spawn, you can attach a script via .truck file format, see https://github.com/RigsOfRods/rigs-of-rods/pull/3001 and enter this code:
```
void main() { thisActor.setSimAttribute(ACTORSIMATTR_TC_WHEELSLIP_CONSTANT, 0.2); }
```

Measures to prevent cheating:
* When any script uses `setSimAttribute()`, game sends script event `SE_ANGELSCRIPT_MANIPULATIONS` which aborts the  current race.
* When in multiplayer, `setSimAttribute()` logs a warning to console ( appears on screen in the chatbox area) and does nothing.

This page was archived from GitHub PRs [#3210](https://github.com/RigsOfRods/rigs-of-rods/pull/3210) and [#3248](https://github.com/RigsOfRods/rigs-of-rods/pull/3248).