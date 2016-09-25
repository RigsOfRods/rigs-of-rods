ROR CONFIGURATION FILES
=======================

RoR.cfg
-------

    Main config file.
    Syntax: NAME = VALUE
    Data types:
        String - line of text (without quotes)
        Enumeration - one of strings listed below.
        Boolean (true/false) - written as "Yes" or "No"
        Integer - whole number
        Foat - real number
        
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Threads (enumeration)
        * "1 (Standard CPU)"
        * "2 (Hyper-Threading or Dual core CPU)"
        * "3 (multi core CPU, one thread per beam)"

    Shadow technique (enumeration)
        * "None"
        * "Stencil shadows"
        * "Texture shadows"
        * "Parallel-split Shadow Maps"

    Shadow distance (integer)

    Vegetation (enumeration)
        * "None (fastest)"
        * "20%"
        * "50%"
        * "Full (best looking, slower)"

    Lights (enumeration)
        * "None (fastest)"
        * "No light sources"
        * "Only current vehicle, main lights"
        * "All vehicles, main lights"
        * "All vehicles, all lights"

    Input Grab (enumeration)
        Determines how RoR controls mouse input
        * "All" - Always grabs the mouse, cannot leave the screen (alt+tab)
        * "Dynamically" - Grabs the mouse while the window is in focus, can leave screen (alt+tab)
        * "None"

    Texture Filtering (enumeration)
        * "Bilinear"
        * "Trilinear"
        * "Anisotropic (best looking)"
                
    3D Sound renderer (enumeration)
        * "Hardware"
        * "Software"
        * "No sound"
                
    Engine smoke (boolean)
    
    Controler Force Feedback (enumeration)
        * "Enable"
        * "Disable"
                 
    Paged (boolean)
        Display grass and Trees?
    
    Bloom (boolean)
    
    Motion blur (boolean)
    
    Sunburn (boolean)
    
    Waves (boolean)
    
    Water effects (enumeration)
        * "Basic"
        * "Reflection"
        * "Reflection + refraction (speed optimized)"
        * "Reflection + refraction (quality optimized)"
    
    Mirrors (boolean)
    
    Dashboard (boolean)
    
    Dust (boolean)
    
    Spray (boolean)
    
    Controler Deadzone (float)
        possible Values: 0 - 0.5 (0.1 = 10%, 0.5 = 50%, default: 0.1)
    
    FarClip Distance (float)
        possible Values: 100-10000 (visible Distance in meters, default: 1000)
        
    network enable (boolean)
    
    Server port (integer)
    
    Server name (string)
    
    Nickname (string)

    Caelum Fog Density (float)
        how dense the fog with Caelum sky is, default: 0.005
        possible Values: 0.05-0.000001

    Sandstorm Fog Start
        where the fog with Sandstorm starts, default: 500
        possible Values: 100-10000
    
    RigImporter_Debug_TraverseAndLogAllNodes (bool, default false | 0.4.5+)
    
    RigImporter_PrintMessagesToLog (bool, default false | 0.4.5+)
        Affects both RoR.log + RigSpawner report window
        
    RigImporter_PrintNodeStatsToLog (bool, default false | 0.4.5+)
    
    Flexbody_EnableLODs (bool, default false | 0.4.5+)
        LODs are rarely used and their processing is the slowest part of vehicle spawn, really.
        
    Flexbody_UseCache (bool, default false | 0.4.5+)
        EXPERIMENTAL!!!
    
    Camera_EnterVehicle_KeepFixedFreeCam (bool, default false)
    
    Camera_ExitVehicle_KeepFixedFreeCam (bool, default false)
