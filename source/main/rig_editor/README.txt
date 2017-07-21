RigEditor
=========

INTRO

This is RoR's built-in editor for "rigs" (or "actors", or traditionally "trucks")
It's designed to allow users to edit softbody structures using mouse and
keyboard shortcuts. It's heavily inspired by Blender 3D modelling tool.

STATUS

Very early in development - not ready for use! Expect flaws and crashes.

Should be able to import/export truckfiles
and save/load projects in it's own format (JSON-based).
All truckfile data should be kept, if you find anything missing, it's a bug.
There is some support for editing (not adding, yet) of MeshWheels and
FlexBodyWheels.

HISTORY

The RigEditor was started sometime in 2014 as a proof-of-concept which will
be continuously updated. The idea was to quickly provide RoR's modders with
a tool for basic vehicle editing and adjust/improve it's features later as needed.
Then development stopped, and the RigEditor was
removed from upstream codebase because of being incomplete - it truncated
truckfiles due to lack of support for all it's functionality.

In 2017, it was brought back to life as a dev branch "rig-editor-2017".
Missing truckfile support was resolved by stubs - data containers 
without any editing capabilities.
Further, RigEditor's own project format was estabilished - It's based on JSON
for simplicity of development and flexibility. It's not intended for editing
by user.

FEATURE & DEVELOPMENT OVERVIEW

Due to it's fragmented history and the amount of functionality required
(truckfile is a complex format, a lot needs to be done just to
load/save/export/import everything), the design is not really correct or optimal.
A breakdown follows:

* GUI-PRESENT
Made using MyGUI because that's RoR's GUI library of the time. MyGUI wasn't very
handy for this task - RigEditor had it's own complex data structures
(see file RigEditor_RigElementsAggregateData.h) and to effectively map these to
MyGUI's widgets, a whole subsystem of binding structs and macros was estabilished
(see file RigEditor_RigElementGuiPanelBase.h)
Additionally, a system of GUI->Editor "commands" was estabilished. A command
is a function in interface `IMain` (see file 'RigEditor_IMain.h') which performs
some action inside the editor and is meant to be generalized, for example invocable
by both hotkey and GUI button click.

* GUI-FUTURE
RoR's future GUI library is DearIMGUI (currently exists as dev branch "dear-imgui").
This GUI is orders of magnitued less verbose than MyGUI (no boilerplate code!)
and allows composing GUI on-the-fly based on actual data. Fits RigEditor's use-case
perfectly. All the MyGUI-based cruft will be removed and replaced by lightweight
routines composing GUI from the data. The interface `IMain` should be removed
- the complexity is not justified.

TL;DR - MyGUI will go away, we'll use DearIMGUI. MyGUI cruft will be removed.

* INPUT-present & future
RoR's `InputEngine` class is extremely convoluted and firmly tied to simulation.
Thus, RigEditor got it's own input handler (see RigEditor_InputHandler.h).
RoR currently uses OIS for inputs, which is not optimal (i.e. MacOSX support)
Future: RoR will be hopefully updated to use SDL for input. A dev-branch already exists:
"sdl-input" - but there are just some thoughts and cleanups, it's not in progress yet.

* CAMERA-present & future
RoR's `CameraManager` is very tightly coupled with simulation logic and
also very convoluted - a cleanup branch already exists. RigEditor has it's
own camera handler, see file 'RigEditor_CameraHandler.h'. The camera handler
is copy-pasted from OGRE's samples and has flaws (i.e. flickering deadzone
in top-down view) - it needs to be cleaned up and fixed.

* TRUCKFILE SUPPORT - present
Rigs of Rods "truckfile" format was designed to be manually written by modders.
As such, it's readable and meaningful to human, but hard to parse and modify
by tools. It has features only meaningful for manual/semi-manual editing
(i.e. with Editorizer - tool wchich visualizes the softbody but editing is done
by writing/modifying text definitions in a sidebar). In short, it's both a
content fileformat and a project fileformat.
When RigEditor was started, it was intended to work this way - to load
projects from truckfiles and save them to truckfiles. Loading truckfile was done
directly by class `Rig` (see file RigEditor_Rig.h) with assisting data structures
like `RigBuildingReport`.

* TRUCKFILE SUPPORT - future
RigEditor got it's own project format (JSON-based) and Truckfile was demoted to
an external format for import/export. As such, the loading/saving logic should
be moved away from class `Rig`.

TL;DR: RigEditor has it's own project format, truckfiles must be imported/exported.

* N/B (softbody) HANDLING - present & future
Truckfiles have historically numbered nodes (numbers must be consecutive)
with later addition of "named nodes", which was an incremental improvement - named
nodes were still numbered under the hood, and in many occasions, only a numeric
reference was possible. Also, since truckfile was sequential file format (order matters!),
moders used directives 'set_*_defaults' to alter the following definitions.

None of this is usable for edting by mouse (Blender-style).
Thus, RigEditor works exclusively with named nodes 
and converts all numbered nodes to named nodes on import.
This breaks some truckfile features, i.e. attaching beams to generated nodes (from wheels, for example) - to be resolved.
Also, node numeric ranges (for example 0-5) cannot be used anymore. Explicit lists of node names will be added instead.

Also, in a visual editor, you want to group nodes and give them properties
based on mouse+keyboard selection. This doesn't go together with truckfile's
sequential 'set_*_defaults' directives. So instead, RigEditor uses a system
of named 'presets' (set_node_defaults -> NodePreset etc...) and assigns these
presets to elements. When exporting to truckfile, elements are grouped based
on the preset (remember, there are only named nodes, order doesn't matter) and
the presets are duplicated in the truckfile if necessary.

TL;DR: Truckfile's 'nodes/nodes2' and 'set_***_defaults' are useless. RigEditor
has it's own constructs and somehow fits them to truckfile on export. To be
resolved.
