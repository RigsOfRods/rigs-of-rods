Addons
======

Addons are AngelScript-powered feature packages for Rigs of Rods.

Addons are mods (ZIPs or directories under 
  'userprofile/Documents/My Games/Rigs of Rods/mods')
  with an INI-syntax config file '*.addon'.

All addons found during cache update are assembled under
  menu "Addons" in the console window (hotkey ~)
  with actions "Load" and "AutoLoad". 

Addons work the same way as terrain scripts:
 * on load, function `main()` is invoked.
 * on each rendered frame, function `frameStep(float dt)` is invoked.
 * on game event `eventCallback(int type, int arg)` is invoked.
 * on user-defined event `defaultEventCallback(int, string, string, int)` is invoked.
  
------------------- example.addon -------------------
[General]
Name = Example

[Scripts]
example.as

------------------- example.as -------------------

void main()
{
    log("Hello, I'm an addon!");
}

int frame_num = 0;

void frameStep(float dt)
{
    if ((frame_num % 25) == 0)
    {
        string msg;
        msg = "loop() frame: " + frame_num;
        log(msg);
    }
    frame_num++;
}  

