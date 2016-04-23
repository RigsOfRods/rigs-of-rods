ROR CONFIGURATION FILES
=======================

RoR.cfg
-------

    Main config file.

    OPTION | VALUES | VERSION | DETAIL
    ----------------------------------
    RigImporter_Debug_TraverseAndLogAllNodes | bool, default false | 0.4.5+
    RigImporter_PrintMessagesToLog           | bool, default false | 0.4.5+ | Affects both RoR.log + RigSpawner report window
    RigImporter_PrintNodeStatsToLog          | bool, default false | 0.4.5+
    
    Flexbody_EnableLODs                      | bool, default false | 0.4.5+ | LODs are rarely used and their processing is the slowest part of vehicle spawn, really.
    Flexbody_UseCache                        | bool, default false | 0.4.5+ | EXPERIMENTAL!!!
    
    Camera_EnterVehicle_KeepFixedFreeCam     | bool, default false
    Camera_ExitVehicle_KeepFixedFreeCam      | bool, default false
