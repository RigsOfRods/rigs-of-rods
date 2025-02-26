RIGS OF RODS - bundled scripts
==============================

TIP: you can run any script in game using 'loadscript' command in the console. To display console, press '~' anytime or in simulation go to top menu > Tools > Show console.

FOR DEVELOPERS: Visit https://developer.rigsofrods.org/ and click "Script-side APIs" link on main page for detailed docs.

Conventions:
* Scripts named 'example_*.as' are short introductory scripts that perform one thing, usually showing UI with tips and controls. You can run them using 'loadscript'.
* Scripts named '*_utils.as' are includes - running them via 'loadscript' does nothing, they are to be `#include`-d into other scripts.

Special scripts:
* 'default.as' - fallback terrain script, loaded if modder didn't provide custom script.
* 'base.as' - an include script for terrain, handles some basic object events.
* 'races.as' - an include script for terrain, implements waypoint-based driving challenges.
* 'AI.as' - a backend script for the "Vehicle AI" menu in top menubar. Spawns an AI-driven vehicle, as configured in the menu.
* 'terrain_project_importer.as' - Launched automatically with terrain-editor mode with a read-only (ZIPped) terrain, provides info and import controls.
