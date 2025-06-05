Gadget system             {#GadgetSystemPage}
=============

![image](https://github.com/user-attachments/assets/66202795-2acc-4606-8d91-cec03fc9428d)


Gadgets are essentially scripts wrapped as modcache entries. The point is to be able to distribute community-created scripts via the repository. The name "gadgets" is inspired by Beyond All Reason RTS (running open source Recoil engine). Usual modcache treatment applies:
* can be zipped or unzipped in directory
* can have preview (aka 'mini') image
* selectable via MainSelectorUI (open from TopMenubarUI, menu Tools, button BrowseGadgets

All methods of loading a script were extended to recognize the .gadget suffix and load the gadget appropriately:
* the 'loadscript' console command
* the cvars 'app_custom_scripts' and 'app_recent_scripts'
* the -runscript command line argument
NOTE that although `ScriptCategory::GADGET` was added, all the above methods still load everything as `ScriptCategory::CUSTOM` and the game adjusts it by detecting the '.gadget' file extension.

The ScriptMonitorUI was extended to display .gadget file name for `ScriptCategory::GADGET` script units.

Example .gadget file:

```
; This is a .gadget mod - basically a script wrapped as a modcache entry.
; The .gadget file must be present (even if empty) to get recognized.
; The script must have same base filename, i.e. 'foo.gadget' -> 'foo.as'
; Any extra include scripts or other resources can be bundled.
; -----------------------------------------------------------------------

; Name to display in Selector UI.
gadget_name "Engine Tool"

; Authors (Syntax: <credit>, <forumID>, <name>, [<email>]) - multiple authors can be given.
gadget_author "base script" 351 ohlidalp

; Description to display in Selector UI.
gadget_description "In-game engine diag and adjustment."

; Category for Selector UI (300 = Generic gadgets, 301 = Actor gadgets, 302 = Terrain gadgets).
gadget_category 301
```

Several gadgets come bundled with the game (Most of the scripts are alpha prototypes, the mature tools: script editor, road editor). 
Browse and start them via TopMenubar/Tools menu/[Browse gadgets...] button. ([view on GitHub](https://github.com/RigsOfRods/rigs-of-rods/tree/master/resources/gadgets)).

![image](https://github.com/user-attachments/assets/5da960fb-71df-487e-a23f-52a81d8955b5)

This page was archived from GitHub PR [#3233](https://github.com/RigsOfRods/rigs-of-rods/pull/3233)