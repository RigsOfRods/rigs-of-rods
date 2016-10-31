
#include "benchmark/benchmark.h"
#include <regex>
#include <iostream>

    enum Keyword
    {
    KEYWORD_ADD_ANIMATION = 1,                KEYWORD_AIRBRAKES,                    
    KEYWORD_ANIMATORS,                        KEYWORD_ANTI_LOCK_BRAKES,             
    KEYWORD_AXLES,                            KEYWORD_AUTHOR,                       
    KEYWORD_BACKMESH,                         KEYWORD_BEAMS,                        
    KEYWORD_BRAKES,                           KEYWORD_CAB,                          
    KEYWORD_CAMERARAIL,                       KEYWORD_CAMERAS,                      
    KEYWORD_CINECAM,                          KEYWORD_COLLISIONBOXES,               
    KEYWORD_COMMANDS,                         KEYWORD_COMMANDS2,                    
    KEYWORD_CONTACTERS,                       KEYWORD_CRUISECONTROL,                
    KEYWORD_DESCRIPTION,                      KEYWORD_DETACHER_GROUP,               
    KEYWORD_DISABLEDEFAULTSOUNDS,             KEYWORD_ENABLE_ADVANCED_DEFORMATION,  
    KEYWORD_END,                              KEYWORD_END_SECTION,                  
    KEYWORD_ENGINE,                           KEYWORD_ENGOPTION,                    
    KEYWORD_ENGTURBO,                         KEYWORD_ENVMAP,                       
    KEYWORD_EXHAUSTS,                         KEYWORD_EXTCAMERA,                    
    KEYWORD_FILEFORMATVERSION,                KEYWORD_FILEINFO,                     
    KEYWORD_FIXES,                            KEYWORD_FLARES,                       
    KEYWORD_FLARES2,                          KEYWORD_FLEXBODIES,                   
    KEYWORD_FLEXBODY_CAMERA_MODE,             KEYWORD_FLEXBODYWHEELS,               
    KEYWORD_FORWARDCOMMANDS,                  KEYWORD_FUSEDRAG,                     
    KEYWORD_GLOBALS,                          KEYWORD_GUID,                         
    KEYWORD_GUISETTINGS,                      KEYWORD_HELP,                         
    KEYWORD_HIDE_IN_CHOOSER,                  KEYWORD_HOOKGROUP,                    
    KEYWORD_HOOKS,                            KEYWORD_HYDROS,                       
    KEYWORD_IMPORTCOMMANDS,                   KEYWORD_LOCKGROUPS,                   
    KEYWORD_LOCKGROUP_DEFAULT_NOLOCK,         KEYWORD_MANAGEDMATERIALS,             
    KEYWORD_MATERIALFLAREBINDINGS,            KEYWORD_MESHWHEELS,                   
    KEYWORD_MESHWHEELS2,                      KEYWORD_MINIMASS,                     
    KEYWORD_NODECOLLISION,                    KEYWORD_NODES,                        
    KEYWORD_NODES2,                           KEYWORD_PARTICLES,                    
    KEYWORD_PISTONPROPS,                      KEYWORD_PROP_CAMERA_MODE,             
    KEYWORD_PROPS,                            KEYWORD_RAILGROUPS,                   
    KEYWORD_RESCUER,                          KEYWORD_RIGIDIFIERS,                  
    KEYWORD_ROLLON,                           KEYWORD_ROPABLES,                     
    KEYWORD_ROPES,                            KEYWORD_ROTATORS,                     
    KEYWORD_ROTATORS2,                        KEYWORD_SCREWPROPS,                   
    KEYWORD_SECTION,                          KEYWORD_SECTIONCONFIG,                
    KEYWORD_SET_BEAM_DEFAULTS,                KEYWORD_SET_BEAM_DEFAULTS_SCALE,      
    KEYWORD_SET_COLLISION_RANGE,              KEYWORD_SET_INERTIA_DEFAULTS,         
    KEYWORD_SET_MANAGEDMATERIALS_OPTIONS,     KEYWORD_SET_NODE_DEFAULTS,            
    KEYWORD_SET_SHADOWS,                      KEYWORD_SET_SKELETON_SETTINGS,        
    KEYWORD_SHOCKS,                           KEYWORD_SHOCKS2,                      
    KEYWORD_SLIDENODE_CONNECT_INSTANTLY,      KEYWORD_SLIDENODES,                   
    KEYWORD_SLOPE_BRAKE,                      KEYWORD_SOUNDSOURCES,                 
    KEYWORD_SOUNDSOURCES2,                    KEYWORD_SPEEDLIMITER,                 
    KEYWORD_SUBMESH,                          KEYWORD_SUBMESH_GROUNDMODEL,          
    KEYWORD_TEXCOORDS,                        KEYWORD_TIES,                         
    KEYWORD_TORQUECURVE,                      KEYWORD_TRACTION_CONTROL,             
    KEYWORD_TRIGGERS,                         KEYWORD_TURBOJETS,                    
    KEYWORD_TURBOPROPS,                       KEYWORD_TURBOPROPS2,                  
    KEYWORD_VIDEOCAMERA,                      KEYWORD_WHEELDETACHERS,               
    KEYWORD_WHEELS,                           KEYWORD_WHEELS2,                      
    KEYWORD_WINGS,                        
    KEYWORD_INVALID = 0xFFFFFFFF
    };

// Example truckfile
const char* trucklines[] = {
"2012 Saf-T-Liner HDX (FPS)           ",
"                                     ",
"fileinfo 4513UID , 107, 1            ",
"                                     ",
"fileformatversion 3                  ",
"                                     ",
"author chassis -1 Sirendude 2011     ","author texture -1 Graysonk95         ",
"author mesh -1 Graysonk95            ","author wheels -1 Nickster7           ",
"author engine_sounds -1 TC2000FE     ","author exterior_mirrors -1 Carsonj493",
"",
"description                                                           ",
"This is a 2012 Thomas Saf-T-Liner HDX                                 ",
"and my first vehicle I have done most of the work getting it in game! ",
"Enjoy!                                                                ",
"                                                                      ",
"SHIFT+B toggles ABS.                                                  ",
"SHIFT+V toggles TC                                                    ",
"end_description                                                       ",
"                                ",
"                                ",
"                                ",
"extera node 44                  ",
"                                ",
"globals                         ",
"1000, 0.0, tracks/semi          ",
"                                ",
"help                            ",
"tracks/HDXhelp                  ",
"                                ",
"disabledefaultsounds            ",
"                                ",
"soundsources                    ",
"116, tracks/default_force       ","116, tracks/default_starter     ",
"116, tracks/default_turbo       ","116, HDX_engine                 ",
"49, HDX_ign                     ","55, HDX_air_purge               ",
"49, tracks/default_horn         ","96, HDX_pump                    ",
"55, HDX_air                     ","14, tracks/default_shift        ",
"64, tracks/default_break        ","64, tracks/default_creak        ",
"14, tracks/default_gear_slide   ","116, HDX_rbeep                  ",
"49, HDX_signal                  ","14, tracks/default_screetch     ",
"14, HDX_brakes                  ","14, HDX_pbrake                  ",
"5, tracks/default_screetch      ","5, HDX_brakes                   ",
"5, HDX_pbrake                   ",
"                                ",
"                                ",
"brakes                          ",
"25000, 45000                    ",
"                                ",
"SlopeBrake 10, 6, 12            ",
"",
"managedmaterials                                                                                        ",
";new_material			effect				parameters                                                   ",
"HDX_busbody   mesh_standard        busbody.png        medium.png                                        ",
"HDX_buswindow   mesh_transparent buswindow.png       bustintedwindow.png                                ",
"edoor_busbody   mesh_standard        busbody.png        medium.png                                      ",
"edoor_buswindow   mesh_transparent buswindow.png       bustintedwindow.png                              ",
"HDX_white   mesh_standard        whiteroof.png        lightgray.png                                     ",
"HDXlmirror_video-camera1   mesh_standard       mirror.png       whiteroof.png                           ",
"HDXrmirror_video-camera2   mesh_standard       mirror.png       whiteroof.png                           ",
"                                                                                                        ",
"HDXrearview_mirror   mesh_standard       mirror.png       whiteroof.png                                 ",
"HDXdblrim_rimcolor   mesh_standard       rimcolor.png       medium.png                                  ",
"HDXrim_rimcolor   mesh_standard       rimcolor.png       medium.png                                     ",
"Speaker_speaker   mesh_standard        speaker.png        -                                             ",
"Speaker_speakers   mesh_standard        speakers.png        -                                           ",
"//number_busnumber   mesh_standard        busnumber.png        -                                        ",
"HDXseats_HDXseat   mesh_standard        HDXseat.png        -                                            ",
"HDXdseat_HDXseat   mesh_standard        HDXseat.png        -                                            ",
"                                                                                                        ",
"                                                                                                        ",
";AntiLockBrakes regulation-force, minspeed, pulse/sec, mode                                             ",
"AntiLockBrakes         0.2,           8,       500, mode: OFF                                           ",
"                                                                                                        ",
";TractionControl regulation-force, wheelslip, fadespeed, pulse/sec, mode                                ",
"TractionControl       2,         0.6,          0.3,       1000, mode: ON                                ",
"                                                                                                        ",
"engine                                                                                                  ",
";min rpm, max rpm, torque, differential ratio, rear gear, first, second, third, fourth, fifth, sixth    ",
"1000.1, 1900.0, 6300.0, 4.00, 4.9,  4.0, 3.22, 2.4, 1.9, 1.5, 1.3, 1.13                                 ",
"                                                                                                        ",
"                                                                                                        ",
";set_beam_defaults spring, damping, deform, break, diameter, material                                   ",
"set_beam_defaults 4000000, 7000, 2000, 80000, -1, tracks/beam                                           ",
"                                  ",
"//enable_advanced_deformation     ",
"                                  ",
"nodes                             ",
"                                  ",
"0, -1.6, 0.6, -1.2, n     ","1, -2, 0, 0.5, n          ","2, -1.6, 0.6, 1.2, n      ","3, -2, 0, -0.5, n         ",
"4, -0.4, 0.6, -1.2, n     ","5, -0.25, 0, 0.5, n       ","6, -0.4, 0.6, 1.2, n      ","7, -0.25, 0, -0.5, n      ",
"8, 2, 0.5, 0.5, n         ","9, 2, 0.5, -0.5, n        ","10, 2, 0, 0.5, n          ","11, 2, 0, -0.5, n         ",
"12, 4.5, 0, -0.5, n       ","13, 4.5, 0.55, 0.5, n     ","14, 4.5, 0, 0.5, n        ","15, 4.5, 0.55, -0.5, n    ",
"16, 6, 0.55, -0.5, n      ","17, 6, 0, 0.5, n          ","18, 6, 0.55, 0.5, n       ","19, 6, 0, -0.5, n         ",
"20, 6.5, 0.5, -0.5, n     ","21, 6.5, 0, 0.5, n        ","22, 6.5, 0.5, 0.5, n      ","23, 6.5, 0, -0.5, n       ",
"24, -1, 0.1, 0.95, n      ","25, -1, 0.1, 1.45, n      ","26, -1, 0.1, -0.95, n     ","27, -1, 0.1, -1.45, n     ",
"28, -0.5, 0.1, 0.95, n    ","29, -1.5, 0.1, 0.95, n    ","30, -0.5, 0.1, -0.95, n   ","31, -1.5, 0.1, -0.95, n   ",
"32, -1, 0.6, 1.05, n      ","33, -1, 0.6, -1.05, n     ","34, 5.25, 0.05, 1.45, n   ","35, 5.25, 0.05, -1.45, n  ",
"36, 5.25, 0.05, 0.7, n    ","37, 5.25, 0.05, -0.7, n   ","38, 5.25, 0.55, 1.5, n    ","39, 5.25, 0.55, -1.5, n   ",
"40, 4.75, 0.05, 1.45, n   ","41, 4.75, 0.05, -1.45, n  ","42, 5.75, 0.05, 1.45, n   ","43, 5.75, 0.05, -1.45, n  ",
"44, 2, 1.5, 0, n          ","45, -0.5, 1, 1.5, n       ","46, -0.25, 0.25, 1.5, n   ","47, -0.25, 0.25, -1.5, n  ",
"48, -0.5, 1, -1.5, n      ","49, -3.25, 1.5, 1, n      ","50, -3.25, 1.5, -1, n     ","51, -3.25, 2.7, 1.5, n    ",
"52, -3.25, 2.7, -1.5, n   ","53, 3.25, 0, 1.5, n       ","54, 3.25, 0, -1.5, n      ","55, 0.75, 0, 1.5, n       ",
"56, 0.75, 0, -1.5, n      ","57, 4.5, 0.25, 1.5, n     ","58, 4.5, 0.25, -1.5, n    ","59, 4.75, 1, 1.5, n       ",
"60, 5.25, 1, 1.5, n       ","61, 5.75, 1, 1.5, n       ","62, 5.75, 1, -1.5, n      ","63, 5.25, 1, -1.5, n      ",
"64, 4.75, 1, -1.5, n      ","65, 6, 0.25, 1.5, n       ","66, 6, 0.25, -1.5, n      ","67, 8.75, 0.35, 1.5, n    ",
"68, 8.75, 0.35, -1.5, nx  ","69, 8.75, 2.55, 1.5, n    ","70, 8.75, 2.55, -1.5, n   ","71, -3.25, 3.15, 0.75, n  ",
"72, -3.25, 3.15, -0.75, n ","73, 8.75, 3.3, 0.75, n    ","74, 8.75, 3.3, -0.75, n   ","75, 8.75, 1.5, 1.5, n     ",
"76, 8.75, 1.5, -1.5, n    ","77, -3.25, 1.5, 1.5, n    ","78, -3.25, 1.5, -1.5, n   ","79, -1.5, 1, -1.5, n      ",
"80, -1.75, 0.25, -1.5, n  ","81, -1.75, 0.25, -1.5, n  ","82, -0.25, 0, -1.5, n     ","83, -1.5, 1, 1.5, n       ",
"84, -1.75, 0.25, 1.5, n   ","85, -0.25, 0, 1.5, n      ","86, -3.25, 0, 1.5, n      ","87, -3.25, 0, -1.5, n     ",
"88, -1.75, 0, -1.5, n     ","89, -2.1, -0.1, -1.5, n   ","90, -3.2, -0.1, -1.5, n   ","91, -2.7, -0.1, -1.5, n   ",
"92, -2.65, -0.1, -1.5, n  ","93, -3.2, 2.5, -1.5, n    ","94, -2.15, 2.5, -1.5, n   ","95, -2.65, 2.5, -1.5, n   ",
"96, -2.7, 2.5, -1.5, n    ","97, -2.15, 2.5, 0, n      ","98, -3.3, 0.1, 1.1, n     ","99, -3.3, 0.3, 1.1, n     ",
"100, -3.35, 0.3, -1.4, n  ","101, -3.35, -0.05, -1.4, n","102, -3.4, 0.3, 1.1, n    ","103, -3.4, 0.1, 1.1, n    ",
"104, -1.8, 1.65, 1.5, n   ","105, -1.2, 1.2, 1.5, n    ","106, -1.2, 1.65, 1.5, n   ","107, -1.8, 1.2, 1.5, n    ",
"108, -1.2, 1.2, 1.65, n   ","109, -1.2, 1.65, 1.65, n  ","110, 5.85, 1.65, 1.5, n   ","111, 5.85, 1.2, 1.5, n    ",
"112, 6.45, 1.65, 1.5, n   ","113, 6.45, 1.2, 1.5, n    ","114, 6.45, 1.65, 1.65, n  ","115, 6.45, 1.2, 1.65, n   ",
"116, 8.25, 1, 0, n        ","117, -3.2, 1.7, 0.35, n   ","118, -2.95, 1.4, 0.35, n  ","119, -2.95, 1.4, 1.2, n   ",
"120, -2.7, 0.95, 1.5, n   ","121, -2.7, 0.95, 0.75, n  ","122, -2.25, 1, 0.75, n    ","123, -3.3, 2.6, -0.7, n   ",
"124, -3.3, 2.6, 0.7, n    ","125, 7.55, 3.5, 0, n      ","126, 7.5, 3.35, 0.05, n   ","127, 7.6, 3.35, -0.05, n  ",
"128, 7.5, 3.35, -0.05, n  ","129, 7.6, 3.35, 0.05, n   ","130, -4.35, 2.7, 1.8, n   ","131, -4.35, 2.7, -1.8, n  ",
"                ",
"beams           ",
"                ",
"19, 17, i ","16, 18, i ","17, 18, i","19, 16, i",
"19, 18, i ","17, 16, i ","20, 22, i","21, 22, i",
"23, 20, i ","21, 23, i ","20, 21, i","22, 23, i",
"12, 14, i ","13, 15, i ","12, 15, i","14, 13, i",
"15, 14, i ","13, 12, i ","11, 10, i","8, 10, i ",
"8, 9, i   ","11, 9, i  ","10, 9, i ","11, 8, i ",
"7, 4, i   ","6, 5, i   ","0, 3, i  ","2, 1, i  ",
"3, 1, i   ","0, 2, i   ","3, 2, i  ","1, 0, i  ",
"6, 4, i   ","7, 5, i   ","4, 5, i  ","6, 7, i  ",
"5, 3, i   ","7, 1, i   ","2, 4, i  ","6, 0, i  ",
"5, 1, i   ","3, 7, i   ","4, 0, i  ","6, 2, i  ",
"20, 16, i ","23, 19, i ","21, 17, i","22, 18, i",
"16, 22, i ","20, 18, i ","21, 19, i","17, 23, i",
"17, 12, i ","14, 19, i ","17, 14, i","12, 19, i",
"18, 13, i ","16, 15, i ","18, 15, i","13, 16, i",
"19, 15, i ","16, 12, i ","18, 14, i","13, 17, i",
"22, 17, i ","18, 21, i ","20, 19, i","16, 23, i",
"15, 11, i ","9, 12, i  ","14, 11, i","10, 12, i",
"13, 8, i  ","15, 9, i  ","13, 9, i ","8, 15, i ",
"12, 11, i ","14, 10, i ","8, 14, i ","13, 10, i",
"7, 0, i   ","3, 4, i   ","5, 2, i  ","1, 6, i  ",
"5, 10, i  ","8, 6, i   ","4, 9, i  ","11, 7, i ",
"4, 8, i   ","9, 6, i   ","5, 11, i ","10, 7, i ",
"10, 6, i  ","5, 8, i   ","9, 7, i  ","4, 11, i ",
"10, 4, i  ","7, 8, i   ","11, 6, i ","5, 9, i  ",
"24, 32, i ","25, 32, i ","29, 32, i","25, 24, i",
"29, 24, i ","25, 29, i ","32, 28, i","24, 28, i",
"25, 28, i ","30, 26, i ","31, 26, i","27, 26, i",
"30, 27, i ","31, 27, i ","33, 27, i","33, 26, i",
"31, 33, i ","30, 33, i ","2, 32, i ","6, 32, i ",
"33, 0, i  ","4, 33, i  ","39, 37, i","35, 37, i",
"43, 37, i ","41, 37, i ","35, 41, i","43, 35, i",
"39, 35, i ","39, 43, i ","39, 41, i","19, 35, i",
"16, 39, i ","35, 12, i ","15, 39, i","40, 36, i",
"42, 36, i ","34, 36, i ","42, 34, i","40, 34, i",
"38, 34, i ","38, 36, i ","38, 42, i","38, 40, i",
"38, 18, i ","17, 34, i ","14, 34, i","38, 13, i",
"41, 12, i ","43, 19, i ","40, 14, i","42, 17, i",
"8, 44, i  ","10, 44, i ","9, 44, i ","11, 44, i",
"6, 44, i  ","5, 44, i  ","4, 44, i ","7, 44, i ",
"9, 14, i  ","13, 11, i ","8, 12, i ","15, 10, i",
"46, 45, i ","5, 46, i  ","6, 46, i ","6, 45, i ",

"47, 48, i  ","47, 7, i ","4, 47, i ","4, 48, i ",
"48, 45, i  ","49, 50, i","6, 49, i ","4, 50, i ",
"4, 49, i   ","50, 6, i ","51, 52, i","7, 56, i ",
"47, 56, i  ","56, 11, i","54, 56, i","11, 54, i",
"12, 54, i  ","53, 14, i","53, 10, i","55, 10, i",
"53, 55, i  ","55, 5, i ","55, 46, i","10, 46, i",
"47, 11, i  ","56, 12, i","14, 55, i","5, 53, i ",
"7, 54, i   ","8, 53, i ","8, 55, i ","9, 54, i ",
"9, 56, i   ","14, 57, i","13, 57, i","58, 12, i",
"15, 58, i  ","54, 58, i","15, 54, i","13, 53, i",
"57, 53, i  ","16, 66, i","19, 66, i","62, 66, i",
"16, 62, i  ","16, 63, i","62, 63, i","64, 63, i",
"64, 58, i  ","63, 15, i","64, 15, i","65, 61, i",
"60, 61, i  ","13, 59, i","13, 60, i","18, 60, i",
"18, 61, i  ","65, 17, i","18, 65, i","60, 59, i",
"57, 59, i  ","63, 60, i","61, 62, i","59, 64, i",
"64, 60, i  ","59, 63, i","61, 63, i","62, 60, i",
"67, 65, i  ","68, 66, i","68, 19, i","67, 17, i",
"65, 21, i  ","66, 23, i","20, 66, i","22, 65, i",
"22, 67, i  ","20, 68, i","23, 68, i","21, 67, i",
"68, 67, i  ","22, 68, i","20, 67, i","21, 68, i",
"23, 67, i  ","52, 72, i","51, 71, i","72, 71, i",
"52, 71, i  ","51, 72, i","70, 69, i","73, 69, i",
"74, 70, i  ","74, 73, i","69, 74, i","70, 73, i",
"68, 69, i  ","67, 70, i","74, 72, i","73, 72, i",
"71, 74, i  ","70, 52, i","69, 51, i","70, 72, i",
"52, 74, i  ","71, 73, i","71, 69, i","73, 51, i",
"62, 70, i  ","66, 70, i","63, 70, i","56, 52, i",
"47, 52, i  ","56, 70, i","54, 70, i","54, 52, i",
"64, 52, i  ","65, 69, i","61, 69, i","60, 69, i",
"59, 69, i  ","53, 69, i","53, 51, i","55, 51, i",
"46, 51, i  ","59, 51, i","69, 75, i","70, 76, i",
"74, 76, i  ","73, 75, i","74, 75, i","73, 76, i",
"75, 67, i  ","75, 76, i","68, 76, i","65, 75, i",
"66, 76, i  ","68, 75, i","67, 76, i","48, 78, i",
"50, 78, i  ","78, 4, i ","48, 50, i","77, 45, i",
"49, 77, i  ","45, 49, i","77, 6, i ","51, 77, i",
"78, 52, i  ","77, 75, i","76, 78, i","78, 51, i",
"52, 77, i  ","51, 75, i","69, 77, i","70, 78, i",
"52, 76, i  ","77, 55, i","78, 56, i","56, 82, i",
"48, 79, i  ","81, 79, i","55, 85, i","6, 85, i ",
"82, 47, i  ","82, 11, i","10, 85, i","45, 83, i",
"84, 83, i  ","83, 79, i","48, 83, i","79, 45, i",
"2, 84, i   ","1, 84, i ","81, 0, i ","3, 81, i ",
"81, 84, i  ","83, 81, i","84, 79, i","83, 2, i ",
"79, 0, i   ","0, 83, i ","2, 79, i ","84, 86, i",
"77, 86, i  ","83, 86, i","84, 77, i","78, 87, i",
"81, 87, i  ","79, 87, i","81, 78, i","0, 87, i ",
"3, 87, i   ","1, 86, i ","2, 86, i ","87, 86, i",
"78, 86, i  ","77, 87, i","86, 3, i ","1, 87, i ",
"2, 87, i   ","0, 86, i ","79, 52, i","83, 51, i",
"49, 87, i  ","50, 86, i","85, 46, i","5, 85, i ",
"7, 82, i   ","74, 86, i","73, 87, i","70, 86, i",
"87, 69, i  ","76, 86, i","75, 87, i","67, 77, i",
"78, 68, i  ","67, 71, i","72, 68, i","52, 67, i",
"68, 51, i  ","88, 88, i","88, 87, i","88, 81, i",
"88, 86, i  ","88, 84, i","89, 90, i","90, 90, i",
"90, 90, i  ","87, 90, i","89, 88, i","90, 86, i",
"89, 86, i  ","84, 89, i","92, 89, i","91, 90, i",
"95, 92, i  ","91, 96, i","96, 93, i","93, 51, i",
"95, 94, i  ","94, 89, i","97, 72, i","97, 71, i",
"97, 51, i  ","97, 52, i","97, 83, i","97, 84, i",
"97, 86, i  ","97, 87, i","97, 94, i","95, 89, i",
"90, 96, i  ","93, 91, i","92, 94, i","94, 83, i",

"93, 86, i  ","94, 86, i","93, 83, i  ","93, 52, i  ",

"89, 83, i  ","100, 99, i ","101, 98, i ","99, 98, i  ",
"100, 101, i","99, 101, i ","100, 98, i ","101, 87, i ",
"100, 78, i ","100, 81, i ","101, 88, i ","100, 1, i  ",
"101, 86, i ","100, 87, i ","101, 78, i ","103, 103, i",
"99, 102, i ","103, 102, i","103, 98, i ","99, 103, i ",
"102, 103, i","102, 98, i ","100, 102, i","103, 101, i",
"100, 103, i","102, 101, i","100, 51, i ","101, 87, i ",
"100, 87, i ","101, 90, i ","100, 90, i ","100, 50, i ",
"101, 50, i ","93, 94, i  ","93, 94, i  ","71, 94, i  ",
"93, 93, i  ","71, 93, i  ","93, 72, i  ","93, 51, i  ",
"93, 97, i  ","94, 90, i  ","93, 89, i  ","93, 90, i  ",
"90, 93, i  ","104, 77, i ","104, 49, i ","104, 45, i ",
"104, 86, i ","104, 87, i ","83, 104, i ","5, 104, i  ",
"105, 107, i","107, 104, i","106, 105, i","106, 104, i",
"107, 86, i ","107, 49, i ","107, 45, i ","106, 107, i",
"104, 105, i","107, 87, i ","107, 77, i ","106, 109, i",
"108, 105, i","108, 109, i","108, 107, i","104, 109, i",
"109, 107, i","108, 104, i","109, 105, i","108, 106, i",
"61, 111, i ","110, 111, i","111, 113, i","113, 112, i",
"112, 110, i","110, 113, i","112, 111, i","110, 61, i ",
"111, 59, i ","110, 59, i ","111, 64, i ","111, 62, i ",
"110, 64, i ","110, 62, i ","110, 75, i ","111, 67, i ",
"112, 114, i","114, 115, i","115, 113, i","115, 112, i",
"114, 113, i","115, 111, i","114, 110, i","110, 115, i",
"114, 111, i","114, 109, i","108, 115, i","114, 108, i",
"109, 115, i","80, 88, i  ","80, 3, i   ","80, 1, i   ",
"80, 84, i  ","116, 76, i ","116, 75, i ","116, 68, i ",
"116, 67, i ","74, 116, i ","116, 73, i ","117, 118, i",
"118, 119, i","119, 117, i","119, 45, i ","117, 45, i ",
"118, 116, i","117, 44, i ","117, 116, i","118, 44, i ",
"117, 97, i ","119, 49, i ","117, 49, i ","118, 49, i ",
"118, 118, i","119, 104, i","119, 86, i ","120, 121, i",
"120, 86, i ","121, 86, i ","120, 1, i  ","121, 3, i  ",
"121, 101, i","120, 101, i","122, 120, i","122, 121, i",
"122, 101, i","124, 123, i","123, 72, i ","124, 71, i ",
"71, 123, i ","72, 124, i ","124, 69, i ","123, 70, i ",
"123, 52, i ","124, 51, i ","129, 127, i","127, 128, i",
"128, 126, i","126, 129, i","125, 129, i","125, 126, i",
"125, 128, i","125, 127, i","129, 73, i ","127, 74, i ",
"129, 128, i","126, 127, i","64, 128, i ","59, 126, i ",
"129, 67, i ","68, 127, i ","125, 116, i","129, 69, i ",
"127, 70, i ","126, 75, i ","128, 76, i ","130, 51, i ",
"130, 77, i ","52, 131, i ","131, 78, i ","130, 124, i",
"130, 71, i ","131, 123, i","131, 72, i ","32, 33, i  ",
"24, 26, i  ","28, 30, i  ","29, 31, i  ","32, 26, i  ",
"33, 24, i  ","cameras    ","9, 15, 8   ","9, 15, 8   ",
"15, 9, 8   ","9, 15, 8   ",
"                                                         ",
"cinecam                                                  ",
";x,y,z, 8 bindings, spring, damping                      ",
"-2.3, 2.15, 0.78, 8,9,6,4,5,7,10,11, 800000.0, 8000.0    ",
"-1.1, 2.20, -1.0, 8,9,6,4,5,7,10,11, 80000.0, 8000.0     ",
"-2.6, 3.00, 0, 8,9,6,4,5,7,10,11, 80000.0, 8000.0        ",
"2.0, 1.0, 1.75, 8,9,6,4,5,7,10,11, 80000.0, 8000.0       ",
"//2.0, 2.80, 1.75, 8,9,6,4,5,7,10,11, 80000.0, 8000.0    ",
"                                                         ",
"set_beam_defaults 105000, 2000, 60000000, 10000000       ",
"                                                                                                                                                                       ",
"flexbodywheels                                                                                                                                                         ",
";radius, rimradius, width, rays, n1, n2, ref-n, braked, propulsed, force-n, weight, tire-spring, tire-damp, rim-spring, rim-damp, rim-orientation, rim-mesh, tire-mesh ",
"0.65, 0.5, 0.3, 14, 26, 27, 9999, 1, 1, 55, 180.0, 750000.0, 900.0, 700000.0, 3000.0, r, HDXrim.mesh  HDXtire.mesh                                                     ",
"0.65, 0.5, 0.3, 14, 24, 25, 9999, 1, 1, 56, 180.0, 750000.0, 900.0, 700000.0, 3000.0, l, HDXrim.mesh  HDXtire.mesh                                                     ",
"0.65, 0.5, 0.3, 14, 37, 35, 36, 1, 1, 53, 280.0, 800000.0, 1600.0, 700000.0, 5000.0, r, HDXdblrim.mesh  HDXdbltire.mesh                                                ",
"0.65, 0.5, 0.3, 14, 36, 34, 37, 1, 1, 54, 280.0, 800000.0, 1600.0, 700000.0, 5000.0, l, HDXdblrim.mesh  HDXdbltire.mesh                                                ",
"                                                                                                     ",
"set_beam_defaults 700000, 350, 60000000, 80000000                                                    ",
"//wheels                                                                                             ",
"                                                                                                     ",
"//0.65, 0.3, 18, 37, 35, 36, 1, 1, 53, 280.0, 700000.0, 3000.0, tracks/wheelface tracks/wheelband2   ",
"//0.65, 0.3, 18, 36, 34, 37, 1, 1, 54, 280.0, 700000.0, 3000.0, tracks/wheelface tracks/wheelband2   ",
"//0.65, 0.3, 18, 24, 25, 9999, 1, 0, 55, 180.0, 700000.0, 3000.0, tracks/wheelface tracks/wheelband1 ",
"//0.65, 0.3, 18, 26, 27, 9999, 1, 0, 56, 180.0, 700000.0, 3000.0, tracks/wheelface tracks/wheelband1 ",
"                                                                                                     ",
";R/L                                                           ",
"shocks                                                         ",
"                                                               ",
"16, 35, 200000, 10000,  0.5,        0.6,       0.67, i         ",
"18, 34, 200000, 10000,  0.5,        0.6,       0.67, i         ",
"37, 18, 200000, 10000,  0.5,        0.6,       0.67, i         ",
"36, 16, 200000, 10000,  0.5,        0.6,       0.67, i         ",
"24, 6, 200000, 10000,  0.5,        0.6,       1.0, i           ",
"24, 2, 200000, 10000,  0.5,        0.6,       1.0, i           ",
"0, 26, 200000, 10000,  0.5,        0.6,       1.0, i           ",
"4, 26, 200000, 10000,  0.5,        0.6,       1.0, i           ",
"4, 24, 200000, 10000,  0.5,        0.6,       1.0, i           ",
"0, 24, 200000, 10000,  0.5,        0.6,       1.0, i           ",
"2, 26, 200000, 10000,  0.5,        0.6,       1.0, i           ",
"6, 26, 200000, 10000,  0.5,        0.6,       1.0, i           ",
"hydros                                                         ",
"                                                               ",
"24, 31, -0.18                                                  ",
"26, 29, 0.18                                                   ",
"24, 30, 0.18                                                   ",
"28, 26, -0.18                                                  ",
"//set_inertia_defaults 1.5, 2.5, smoothcrane revprogressiv     ",
"",
"commands2",
";id1, id2, rateShort, rateLong, short, long, keyS, keyL, options descr...                  ",
"95, 77, 0.25,0.4,1.0,1.18,3,4,ipf,Door 1.5,1.5, smooth revprogressiv 0                     ",
"92, 77, 0.25,0.4,1.0,1.17,3,4,ipf,Door 1.5,1.5, smooth revprogressiv 0                     ",
"                                                                                           ",
"96, 83, 0.24,0.42,1.0,1.17,3,4,ipf,Door 1.5,1.5, smooth revprogressiv 0                    ",
"91, 83, 0.24,0.42,1.0,1.18,3,4,ipf,Door 1.5,1.5, smooth revprogressiv 0                    ",
"                                                                                           ",
"99, 84, 1.0,1.0,1.0,3.1,1,2,ipf,Crossing guard 0,0, smooth revprogressiv 0                 ",
"98, 84, 1.0,1.0,1.0,3.1,1,2,ipf,Crossing_guard 0,0, smooth revprogressiv 0                 ",
"                                                                                           ",
"108, 45, 0.2,0.25,1.0,2.1,1,2,ipf,Stop_sign&crossing_guard 0, 0, smooth revprogressiv 0    ",
"                                                                                           ",
"                                                                                           ",
"                                                                                           ",
"//Example:                                                                                 ",
"//1,       2,        0.1,            1.0,                 3.0,              1,         ... ",
"                                                                                           ",
"                                                                                           ",
"rotators                                                                      ",
"                                                                              ",
"ropes                                                                         ",
"                                                                              ",
"ties                                                                          ",
"                                                                              ",
"fixes                                                                         ",
"                                                                              ",
"contacters",
"86 ","87 ","102","103","78 ","77 ","72 ","71 ","52 ","51 ","73 ","74 ",
"70 ","69 ","75 ","76 ","67 ","68 ","53 ","55 ","82 ","56 ","54 ",
"                                                                              ",
"                                                                              ",
"                                                                              ",
"ropables                                                                      ",
"14                                                                            ",
"15                                                                            ",
"32                                                                            ",
"33                                                                            ",
"                                                                              ",
"flares                                                                        ",
";    f (default mode when not stated): frontlight                             ",
";   b : brakelight                                                            ",
";    l : left blinker                                                         ",
";    r : right blinker                                                        ",
";    R : reverse light (on when driving in R gear)                            ",
";    u : user controlled light (i.e. fog light) (see controlnumber))          ",
";RefNode,X,Y,OffsetX,OffsetY, Type, ControlNumber, BlinkDelay, size Mat...",
"0,1,2, 0, 0, u, 2, 361, 0.1                                                   ",
"0,1,2, 0, 0, u, 1, 450, 0.1                                                   ",
"0,1,2, 0, 0, u, 2, 0, 0.1                                                     ",
"0,1,2, 0, 0, u, 4, 0, 0.1                                                     ",
"0,1,2, 0, 0,u,2,0,0.1 tracks/HDXstrobe                                        ",
"0,1,2, 0, 0,u,3,0,0.1 tracks/HDXstrobe                                        ",
"                                                                              ",
";Front                                                                        ",
"87,86,77,-0.18,0.28,f,-1,0,2.5                                                ",
"87,86,78,0.9,0.28,f,-1,0,2.5                                                  ",
"87,86,77,-0.08,0.29,f,-1,0,1.1 tracks/HDXyellow1                              ",
"87,86,78,0.8,0.29,f,-1,0,-1.1 tracks/HDXyellow1                               ",
"                                                                              ",
";8-ways                                                                       ",
"52,51,72,0.06,0.35,u,1, 0, 1.5 tracks/HDX8yellow1                             ",
"52,51,72,0.76,0.35,u,1, 0, 1.5 tracks/HDX8yellow2                             ",
"52,51,72,0.0,0.2,u,2, 0, 1.5 tracks/HDX8red1                                  ",
"52,51,72,0.9,0.2,u,2, 0, 1.5 tracks/HDX8red2                                  ",
"                                                                              ",
"                                                                              ",
"86,55,51,0.76,0.58,l,-1,400, 0.12                                             ",
"86,55,51,0.75,0.58,l,-1,400, 0.12                                             ",
"86,55,51,0.74,0.58,l,-1,400, 0.12                                             ",
"86,55,51,0.76,0.58,f,-1,0, 0.1 tracks/HDXyellow2                              ",
"86,55,51,0.75,0.58,f,-1,0, 0.1 tracks/HDXyellow2                              ",
"86,55,51,0.74,0.58,f,-1,0, 0.1 tracks/HDXyellow2                              ",
"                                                                              ",
"86,55,51,2.795,0.63,l,-1,400, 0.12 tracks/HDXred                              ",
"86,55,51,2.785,0.63,l,-1,400, 0.12 tracks/HDXred                              ",
"86,55,51,2.775,0.63,l,-1,400, 0.12 tracks/HDXred                              ",
"86,55,51,2.795,0.63,f,-1,0, 0.1 tracks/HDXred                                 ",
"86,55,51,2.785,0.63,f,-1,0, 0.1 tracks/HDXred                                 ",
"86,55,51,2.775,0.63,f,-1,0, 0.1 tracks/HDXred                                 ",
"                                                                              ",
"87,52,56,0.58,0.76,r,-1,400, 0.12                                             ",
"87,52,56,0.58,0.75,r,-1,400, 0.12                                             ",
"87,52,56,0.58,0.74,r,-1,400, 0.12                                             ",
"87,52,56,0.58,0.76,f,-1,0, 0.1 tracks/HDXyellow2                              ",
"87,52,56,0.58,0.75,f,-1,0, 0.1 tracks/HDXyellow2                              ",
"87,52,56,0.58,0.74,f,-1,0, 0.1 tracks/HDXyellow2                              ",
"                                                                              ",
"87,52,56,0.63,2.795,r,-1,400, 0.12 tracks/HDXred                              ",
"87,52,56,0.63,2.785,r,-1,400, 0.12 tracks/HDXred                              ",
"87,52,56,0.63,2.775,r,-1,400, 0.12 tracks/HDXred                              ",
"87,52,56,0.63,2.795,f,-1,0, 0.1 tracks/HDXred                                 ",
"87,52,56,0.63,2.785,f,-1,0, 0.1 tracks/HDXred                                 ",
"87,52,56,0.63,2.775,f,-1,0, 0.1 tracks/HDXred                                 ",
"                                                                              ",
"87,52,56,0.19,0.315,u,2,0, 0.95                                               ",
"                                                                              ",
"//84,47,56,6.28,-0.04,f,-1,0, 0.1 tracks/HDXyellow2                           ",
"//82,47,56,6.28,0.0,f,-1,0, 0.1 tracks/HDXyellow2                             ",
"//82,47,56,6.28,0.04,f,-1,0, 0.1 tracks/HDXyellow2                            ",
"                                                                              ",
"                                                                              ",
";FR turn signals                                                              ",
"87,86,77,-0.08,0.29,r,-1,400, -1                                              ",
";Row 1 center                                                                 ",
"87,86,77,-0.42,0.6,f,-1,0, 0.15 tracks/HDXyellow2                             ",
"87,86,77,-0.43,0.6,r,-1,400, 0.1                                              ",
"87,86,77,-0.44,0.6,r,-1,400, 0.1                                              ",
"87,86,77,-0.45,0.6,r,-1,400, 0.1                                              ",
"87,86,77,-0.41,0.6,r,-1,400, 0.1                                              ",
"87,86,77,-0.40,0.6,r,-1,400, 0.1                                              ",
"87,86,77,-0.39,0.6,r,-1,400, 0.1                                              ",
";Row 2 (down 1)                                                               ",
"87,86,77,-0.4,0.58,r,-1,400, 0.1                                              ",
"87,86,77,-0.41,0.58,r,-1,400, 0.1                                             ",
"87,86,77,-0.42,0.58,r,-1,400, 0.1                                             ",
"87,86,77,-0.43,0.58,r,-1,400, 0.1                                             ",
"87,86,77,-0.39,0.58,r,-1,400, 0.1                                             ",
"87,86,77,-0.38,0.58,r,-1,400, 0.1                                             ",
"87,86,77,-0.37,0.58,r,-1,400, 0.1                                             ",
";Row 3 (down 2)                                                               ",
"87,86,77,-0.38,0.56,r,-1,400, 0.1                                             ",
"87,86,77,-0.37,0.56,r,-1,400, 0.1                                             ",
"87,86,77,-0.36,0.56,r,-1,400, 0.1                                             ",
"87,86,77,-0.39,0.56,r,-1,400, 0.1                                             ",
"87,86,77,-0.40,0.56,r,-1,400, 0.1                                             ",
";Row 4 (up 1)                                                                 ",
"87,86,77,-0.44,0.62,r,-1,400, 0.1                                             ",
"87,86,77,-0.45,0.62,r,-1,400, 0.1                                             ",
"87,86,77,-0.46,0.62,r,-1,400, 0.1                                             ",
"87,86,77,-0.47,0.62,r,-1,400, 0.1                                             ",
"87,86,77,-0.43,0.62,r,-1,400, 0.1                                             ",
"87,86,77,-0.42,0.62,r,-1,400, 0.1                                             ",
"87,86,77,-0.41,0.62,r,-1,400, 0.1                                             ",
";Row 5 (up 2)                                                                 ",
"87,86,77,-0.46,0.64,r,-1,400, 0.1                                             ",
"87,86,77,-0.47,0.64,r,-1,400, 0.1                                             ",
"87,86,77,-0.48,0.64,r,-1,400, 0.1                                             ",
"87,86,77,-0.45,0.64,r,-1,400, 0.1                                             ",
"87,86,77,-0.44,0.64,r,-1,400, 0.1                                             ",
";Row 6 (up 3)                                                                 ",
"87,86,77,-0.49,0.66,r,-1,400, 0.1                                             ",
"87,86,77,-0.47,0.66,r,-1,400, 0.1                                             ",
"87,86,77,-0.48,0.66,r,-1,400, 0.1                                             ",
";Row 7 (down 3)                                                               ",
"87,86,77,-0.35,0.54,r,-1,400, 0.1                                             ",
"87,86,77,-0.36,0.54,r,-1,400, 0.1                                             ",
"87,86,77,-0.37,0.54,r,-1,400, 0.1                                             ",
"                                                                              ",
";FRts running                                                                 ",
"87,86,77,-0.43,0.6,f,-1,0, 0.1 tracks/HDXyellow3 ","87,86,77,-0.44,0.6,f,-1,0, 0.1 tracks/HDXyellow3 ",
"87,86,77,-0.45,0.6,f,-1,0, 0.1 tracks/HDXyellow3 ","87,86,77,-0.41,0.6,f,-1,0, 0.1 tracks/HDXyellow3 ",
"87,86,77,-0.40,0.6,f,-1,0, 0.1 tracks/HDXyellow3 ","87,86,77,-0.39,0.6,f,-1,0, 0.1 tracks/HDXyellow3 ",
";Row 2 (down 1)                                  ",
"87,86,77,-0.4,0.58,f,-1,0, 0.1 tracks/HDXyellow3 ","87,86,77,-0.41,0.58,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.42,0.58,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.43,0.58,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.39,0.58,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.38,0.58,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.37,0.58,f,-1,0, 0.1 tracks/HDXyellow3",
";Row 3 (down 2)                                  ",
"87,86,77,-0.38,0.56,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.37,0.56,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.36,0.56,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.39,0.56,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.40,0.56,f,-1,0, 0.1 tracks/HDXyellow3",
";Row 4 (up 1)                                    ",
"87,86,77,-0.44,0.62,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.45,0.62,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.46,0.62,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.47,0.62,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.43,0.62,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.42,0.62,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.41,0.62,f,-1,0, 0.1 tracks/HDXyellow3",
";Row 5 (up 2)                                    ",
"87,86,77,-0.46,0.64,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.47,0.64,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.48,0.64,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.45,0.64,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.44,0.64,f,-1,0, 0.1 tracks/HDXyellow3",
";Row 6 (up 3)                                    ",
"87,86,77,-0.49,0.66,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.47,0.66,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.48,0.66,f,-1,0, 0.1 tracks/HDXyellow3",
";Row 7 (dowf 3)                                  ",
"87,86,77,-0.35,0.54,f,-1,0, 0.1 tracks/HDXyellow3","87,86,77,-0.36,0.54,f,-1,0, 0.1 tracks/HDXyellow3",
"87,86,77,-0.37,0.54,f,-1,0, 0.1 tracks/HDXyellow3",
"                                                                              ",
";FL turn signals                                                              ",
"87,86,78,0.8,0.29,l,-1,400, -1                                                ",
";Row 1 center                                                                 ",
"87,86,78,0.85,0.6,l,-1,400, 0.1                                               ",
"87,86,78,0.84,0.6,l,-1,400, 0.1                                               ",
"87,86,78,0.83,0.6,l,-1,400, 0.1                                               ",
"87,86,78,0.82,0.6,f, 0,0, 0.15 tracks/HDXyellow2                              ",
"87,86,78,0.81,0.6,l,-1,400, 0.1                                               ",
"87,86,78,0.8,0.6,l,-1,400, 0.1                                                ",
"87,86,78,0.79,0.6,l,-1,400, 0.1                                               ",
";low 2 (down 1)                                                               ",
"87,86,78,0.85,0.58,l,-1,400, 0.1                                              ",
"87,86,78,0.84,0.58,l,-1,400, 0.1                                              ",
"87,86,78,0.83,0.58,l,-1,400, 0.1                                              ",
"87,86,78,0.82,0.58,l,-1,400, 0.1                                              ",
"87,86,78,0.81,0.58,l,-1,400, 0.1                                              ",
"87,86,78,0.8,0.58,l,-1,400, 0.1                                               ",
"87,86,78,0.79,0.58,l,-1,400, 0.1                                              ",
";low 3 (down 2)                                                               ",
"87,86,78,0.84,0.56,l,-1,400, 0.1                                              ",
"87,86,78,0.83,0.56,l,-1,400, 0.1                                              ",
"87,86,78,0.82,0.56,l,-1,400, 0.1                                              ",
"87,86,78,0.81,0.56,l,-1,400, 0.1                                              ",
"87,86,78,0.8,0.56,l,-1,400, 0.1                                               ",
";low 7                                                                        ",
";low 3 (down 2)                                                               ",
"87,86,78,0.83,0.54,l,-1,400, 0.1                                              ",
"87,86,78,0.82,0.54,l,-1,400, 0.1                                              ",
"87,86,78,0.81,0.54,l,-1,400, 0.1                                              ",
";low 4 (up 1)                                                                 ",
"87,86,78,0.85,0.62,l,-1,400, 0.1                                              ",
"87,86,78,0.84,0.62,l,-1,400, 0.1                                              ",
"87,86,78,0.83,0.62,l,-1,400, 0.1                                              ",
"87,86,78,0.82,0.62,l,-1,400, 0.1                                              ",
"87,86,78,0.81,0.62,l,-1,400, 0.1                                              ",
"87,86,78,0.8,0.62,l,-1,400, 0.1                                               ",
"87,86,78,0.79,0.62,l,-1,400, 0.1                                              ",
";low 5 (up 2)                                                                 ",
"87,86,78,0.84,0.64,l,-1,400, 0.1                                              ",
"87,86,78,0.83,0.64,l,-1,400, 0.1                                              ",
"87,86,78,0.82,0.64,l,-1,400, 0.1                                              ",
"87,86,78,0.81,0.64,l,-1,400, 0.1                                              ",
"87,86,78,0.8,0.64,l,-1,400, 0.1                                               ",
";low 6 (up 3)                                                                 ",
"87,86,78,0.83,0.66,l,-1,400, 0.1                                              ",
"87,86,78,0.82,0.66,l,-1,400, 0.1                                              ",
"87,86,78,0.81,0.66,l,-1,400, 0.1                                              ",
"                                                                              ",
";FLts Running                                                                 ",
"87,86,78,0.85,0.6,f,-1,0, 0.1 tracks/HDXyellow3                               ",
"87,86,78,0.84,0.6,f,-1,0, 0.1 tracks/HDXyellow3                               ",
"87,86,78,0.83,0.6,f,-1,0, 0.1 tracks/HDXyellow3                               ",
"87,86,78,0.81,0.6,f,-1,0, 0.1 tracks/HDXyellow3                               ",
"87,86,78,0.8,0.6,f,-1,0, 0.1 tracks/HDXyellow3                                ",
"87,86,78,0.79,0.6,f,-1,0, 0.1 tracks/HDXyellow3                               ",
";low 2 (down 1)                                                               ",
"87,86,78,0.85,0.58,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.84,0.58,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.83,0.58,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.82,0.58,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.81,0.58,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.8,0.58,f,-1,0, 0.1 tracks/HDXyellow3                               ",
"87,86,78,0.79,0.58,f,-1,0, 0.1 tracks/HDXyellow3                              ",
";low 3 (down 2)                                                               ",
"87,86,78,0.84,0.56,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.83,0.56,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.82,0.56,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.81,0.56,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.8,0.56,f,-1,0, 0.1 tracks/HDXyellow3                               ",
";low 7                                                                        ",
";low 3 (down 2)                                                               ",
"87,86,78,0.83,0.54,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.82,0.54,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.81,0.54,f,-1,0, 0.1 tracks/HDXyellow3                              ",
";low 4 (up 1)                                                                 ",
"87,86,78,0.85,0.62,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.84,0.62,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.83,0.62,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.82,0.62,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.81,0.62,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.8,0.62,f,-1,0, 0.1 tracks/HDXyellow3                               ",
"87,86,78,0.79,0.62,f,-1,0, 0.1 tracks/HDXyellow3                              ",
";low 5 (up 2)                                                                 ",
"87,86,78,0.84,0.64,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.83,0.64,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.82,0.64,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.81,0.64,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.8,0.64,f,-1,0, 0.1 tracks/HDXyellow3                               ",
";low 6 (up 3)                                                                 ",
"87,86,78,0.83,0.66,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.82,0.66,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"87,86,78,0.81,0.66,f,-1,0, 0.1 tracks/HDXyellow3                              ",
"                                                                              ",
"                                                                              ",
";Rear                                                                         ",
";LR LED brake, turn, reverse squares                                          ",
";RR left brake                                                                ",
";Row 0 CENTER                                                                 ",
"75,76,70,0.05,0.08,b,-1,0,0.1                                                 ",
"75,76,70,0.06,0.08,b,-1,0,0.1                                                 ",
"75,76,70,0.02,0.08,b,-1,0,0.1                                                 ",
"75,76,70,0.03,0.08,b,-1,0,0.1                                                 ",
"75,76,70,0.04,0.08,b,-1,0,0.1                                                 ",
";Row 1 (up 1)                                                                 ",
"75,76,70,0.00,0.1,b,-1,0,0.1                                                  ",
"75,76,70,0.01,0.1,b,-1,0,0.1                                                  ",
"75,76,70,0.02,0.1,b,-1,0,0.1                                                  ",
"75,76,70,0.03,0.1,b,-1,0,0.1                                                  ",
"75,76,70,0.04,0.1,b,-1,0,0.1                                                  ",
";Row 2 (up 2)                                                                 ",
"75,76,70,0.0,0.12,b,-1,0,0.1                                                  ",
"75,76,70,0.01,0.12,b,-1,0,0.1                                                 ",
"75,76,70,0.02,0.12,b,-1,0,0.1                                                 ",
"75,76,70,-0.02,0.12,b,-1,0,0.1                                                ",
"75,76,70,-0.01,0.12,b,-1,0,0.1                                                ",
";Row 3 (up 3)                                                                 ",
"75,76,70,0.0,0.14,b,-1,0,0.1                                                  ",
"75,76,70,-0.04,0.14,b,-1,0,0.1                                                ",
"75,76,70,-0.03,0.14,b,-1,0,0.1                                                ",
"75,76,70,-0.02,0.14,b,-1,0,0.1                                                ",
"75,76,70,-0.01,0.14,b,-1,0,0.1                                                ",
";Row 4 (up 4)                                                                 ",
"75,76,70,-0.05,0.16,b,-1,0,0.1                                                ",
"75,76,70,-0.04,0.16,b,-1,0,0.1                                                ",
"75,76,70,-0.03,0.16,b,-1,0,0.1                                                ",
"75,76,70,-0.02,0.16,b,-1,0,0.1                                                ",
"75,76,70,-0.06,0.16,b,-1,0,0.1                                                ",
";Row 5 (up 5)                                                                 ",
"75,76,70,-0.05,0.18,b,-1,0,0.1                                                ",
"75,76,70,-0.04,0.18,b,-1,0,0.1                                                ",
"75,76,70,-0.08,0.18,b,-1,0,0.1                                                ",
"75,76,70,-0.07,0.18,b,-1,0,0.1                                                ",
"75,76,70,-0.06,0.18,b,-1,0,0.1                                                ",
";Row 6 (up 6)                                                                 ",
"75,76,70,-0.1,0.2,b,-1,0,0.1                                                  ",
"75,76,70,-0.09,0.2,b,-1,0,0.1                                                 ",
"75,76,70,-0.08,0.2,b,-1,0,0.1                                                 ",
"75,76,70,-0.07,0.2,b,-1,0,0.1                                                 ",
"75,76,70,-0.06,0.2,b,-1,0,0.1                                                 ",
";Row 7 (up 7)                                                                 ",
"75,76,70,-0.1,0.22,b,-1,0,0.1                                                 ",
"75,76,70,-0.09,0.22,b,-1,0,0.1                                                ",
"75,76,70,-0.08,0.22,b,-1,0,0.1                                                ",
"75,76,70,-0.12,0.22,b,-1,0,0.1                                                ",
"75,76,70,-0.11,0.22,b,-1,0,0.1                                                ",
";Row 8 (up 8)                                                                 ",
"75,76,70,-0.1,0.24,b,-1,0,0.1                                                 ",
"75,76,70,-0.14,0.24,b,-1,0,0.1                                                ",
"75,76,70,-0.13,0.24,b,-1,0,0.1                                                ",
"75,76,70,-0.12,0.24,b,-1,0,0.1                                                ",
"75,76,70,-0.11,0.24,b,-1,0,0.1                                                ",
";Row 9 (up 9)                                                                 ",
"75,76,70,-0.15,0.26,b,-1,0,0.1                                                ",
"75,76,70,-0.14,0.26,b,-1,0,0.1                                                ",
"75,76,70,-0.13,0.26,b,-1,0,0.1                                                ",
"75,76,70,-0.12,0.26,b,-1,0,0.1                                                ",
"75,76,70,-0.16,0.26,b,-1,0,0.1                                                ",
";Row 10 (up 10)                                                               ",
"75,76,70,-0.15,0.28,b,-1,0,0.1                                                ",
"75,76,70,-0.14,0.28,b,-1,0,0.1                                                ",
"75,76,70,-0.18,0.28,b,-1,0,0.1                                                ",
"75,76,70,-0.17,0.28,b,-1,0,0.1                                                ",
"75,76,70,-0.16,0.28,b,-1,0,0.1                                                ",
"                                                                              ",
";RRbs Running                                                                 ",
";Row 0 CENTER                                                                 ",
"75,76,70,0.05,0.08,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,0.06,0.08,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,0.02,0.08,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,0.03,0.08,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,0.04,0.08,f,-1,0,0.1 tracks/HDXred1                                  ",
";Row 1 (up 1)                                                                 ",
"75,76,70,0.00,0.1,f,-1,0,0.1 tracks/HDXred1                                   ",
"75,76,70,0.01,0.1,f,-1,0,0.1 tracks/HDXred1                                   ",
"75,76,70,0.02,0.1,f,-1,0,0.1 tracks/HDXred1                                   ",
"75,76,70,0.03,0.1,f,-1,0,0.1 tracks/HDXred1                                   ",
"75,76,70,0.04,0.1,f,-1,0,0.1 tracks/HDXred1                                   ",
";Row 2 (up 2)                                                                 ",
"75,76,70,0.0,0.12,f,-1,0,0.1 tracks/HDXred1                                   ",
"75,76,70,0.01,0.12,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,0.02,0.12,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,-0.02,0.12,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.01,0.12,f,-1,0,0.1 tracks/HDXred1                                 ",
";Row 3 (up 3)                                                                 ",
"75,76,70,0.0,0.14,f,-1,0,0.1 tracks/HDXred1                                   ",
"75,76,70,-0.04,0.14,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.03,0.14,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.02,0.14,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.01,0.14,f,-1,0,0.1 tracks/HDXred1                                 ",
";Row 4 (up 4)                                                                 ",
"75,76,70,-0.05,0.16,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.04,0.16,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.03,0.16,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.02,0.16,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.06,0.16,f,-1,0,0.1 tracks/HDXred1                                 ",
";Row 5 (up 5)                                                                 ",
"75,76,70,-0.05,0.18,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.04,0.18,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.08,0.18,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.07,0.18,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.06,0.18,f,-1,0,0.1 tracks/HDXred1                                 ",
";Row 6 (up 6)                                                                 ",
"75,76,70,-0.1,0.2,f,-1,0,0.1 tracks/HDXred1                                   ",
"75,76,70,-0.09,0.2,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,-0.08,0.2,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,-0.07,0.2,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,-0.06,0.2,f,-1,0,0.1 tracks/HDXred1                                  ",
";Row 7 (up 7)                                                                 ",
"75,76,70,-0.1,0.22,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,-0.09,0.22,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.08,0.22,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.12,0.22,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.11,0.22,f,-1,0,0.1 tracks/HDXred1                                 ",
";Row 8 (up 8)                                                                 ",
"75,76,70,-0.1,0.24,f,-1,0,0.1 tracks/HDXred1                                  ",
"75,76,70,-0.14,0.24,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.13,0.24,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.12,0.24,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.11,0.24,f,-1,0,0.1 tracks/HDXred1                                 ",
";Row 9 (up 9)                                                                 ",
"75,76,70,-0.15,0.26,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.14,0.26,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.13,0.26,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.12,0.26,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.16,0.26,f,-1,0,0.1 tracks/HDXred1                                 ",
";Row 10 (up 10)                                                               ",
"75,76,70,-0.15,0.28,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.14,0.28,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.18,0.28,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.17,0.28,f,-1,0,0.1 tracks/HDXred1                                 ",
"75,76,70,-0.16,0.28,f,-1,0,0.1 tracks/HDXred1                                 ",
"                                                                              ",
";RR turn signal (center)                                                      ",
"75,76,70,0.0,0.08,l,-1,400,0.1                                                ",
"75,76,70,-0.01,0.08,l,-1,400,0.1                                              ",
"75,76,70,-0.02,0.08,l,-1,400,0.1                                              ",
"75,76,70,-0.03,0.08,l,-1,400,0.1                                              ",
"75,76,70,-0.04,0.08,l,-1,400,0.1                                              ",
";low 1 (up 1)                                                                 ",
"75,76,70,-0.05,0.1,l,-1,400,0.1                                               ",
"75,76,70,-0.06,0.1,l,-1,400,0.1                                               ",
"75,76,70,-0.02,0.1,l,-1,400,0.1                                               ",
"75,76,70,-0.03,0.1,l,-1,400,0.1                                               ",
"75,76,70,-0.04,0.1,l,-1,400,0.1                                               ",
";low 2 (up 2)                                                                 ",
"75,76,70,-0.05,0.12,l,-1,400,0.1                                              ",
"75,76,70,-0.06,0.12,l,-1,400,0.1                                              ",
"75,76,70,-0.07,0.12,l,-1,400,0.1                                              ",
"75,76,70,-0.08,0.12,l,-1,400,0.1                                              ",
"75,76,70,-0.04,0.12,l,-1,400,0.1                                              ",
";low 3 (up 3)                                                                 ",
"75,76,70,-0.10,0.14,l,-1,400,0.1                                              ",
"75,76,70,-0.06,0.14,l,-1,400,0.1                                              ",
"75,76,70,-0.07,0.14,l,-1,400,0.1                                              ",
"75,76,70,-0.08,0.14,l,-1,400,0.1                                              ",
"75,76,70,-0.09,0.14,l,-1,400,0.1                                              ",
";low 4 (up 4)                                                                 ",
"75,76,70,-0.10,0.16,l,-1,400,0.1                                              ",
"75,76,70,-0.11,0.16,l,-1,400,0.1                                              ",
"75,76,70,-0.12,0.16,l,-1,400,0.1                                              ",
"75,76,70,-0.08,0.16,l,-1,400,0.1                                              ",
"75,76,70,-0.09,0.16,l,-1,400,0.1                                              ",
";low 5 (up 5)                                                                 ",
"75,76,70,-0.10,0.18,l,-1,400,0.1                                              ",
"75,76,70,-0.11,0.18,l,-1,400,0.1                                              ",
"75,76,70,-0.12,0.18,l,-1,400,0.1                                              ",
"75,76,70,-0.13,0.18,l,-1,400,0.1                                              ",
"75,76,70,-0.14,0.18,l,-1,400,0.1                                              ",
";low 6 (up 6)                                                                 ",
"75,76,70,-0.15,0.2,l,-1,400,0.1                                               ",
"75,76,70,-0.16,0.2,l,-1,400,0.1                                               ",
"75,76,70,-0.12,0.2,l,-1,400,0.1                                               ",
"75,76,70,-0.13,0.2,l,-1,400,0.1                                               ",
"75,76,70,-0.14,0.2,l,-1,400,0.1                                               ",
";low 7 (up 7)                                                                 ",
"75,76,70,-0.15,0.22,l,-1,400,0.1                                              ",
"75,76,70,-0.16,0.22,l,-1,400,0.1                                              ",
"75,76,70,-0.17,0.22,l,-1,400,0.1                                              ",
"75,76,70,-0.18,0.22,l,-1,400,0.1                                              ",
"75,76,70,-0.14,0.22,l,-1,400,0.1                                              ",
";low 8 (up 8)                                                                 ",
"75,76,70,-0.20,0.24,l,-1,400,0.1                                              ",
"75,76,70,-0.16,0.24,l,-1,400,0.1                                              ",
"75,76,70,-0.17,0.24,l,-1,400,0.1                                              ",
"75,76,70,-0.18,0.24,l,-1,400,0.1                                              ",
"75,76,70,-0.19,0.24,l,-1,400,0.1                                              ",
";low 9 (up 9)                                                                 ",
"75,76,70,-0.20,0.26,l,-1,400,0.1                                              ",
"75,76,70,-0.21,0.26,l,-1,400,0.1                                              ",
"75,76,70,-0.22,0.26,l,-1,400,0.1                                              ",
"75,76,70,-0.18,0.26,l,-1,400,0.1                                              ",
"75,76,70,-0.19,0.26,l,-1,400,0.1                                              ",
";low 10 (up 10)                                                               ",
"75,76,70,-0.20,0.28,l,-1,400,0.1                                              ",
"75,76,70,-0.21,0.28,l,-1,400,0.1                                              ",
"75,76,70,-0.22,0.28,l,-1,400,0.1                                              ",
"75,76,70,-0.23,0.28,l,-1,400,0.1                                              ",
"75,76,70,-0.24,0.28,l,-1,400,0.1                                              ",
"                                        ",
";LR reverse (center)                    ",
"75,76,70,0.12,0.08,R,-1,0,0.1           ",
"75,76,70,0.11,0.08,R,-1,0,0.1           ",
"75,76,70,0.1,0.08,R,-1,0,0.1            ",
"75,76,70,0.09,0.08,R,-1,0,0.1           ",
"75,76,70,0.08,0.08,R,-1,0,0.1           ",
";Row 1 (up 1)                           ",
"75,76,70,0.1,0.1,R,-1,0,0.1             ",
"75,76,70,0.09,0.1,R,-1,0,0.1            ",
"75,76,70,0.08,0.1,R,-1,0,0.1            ",
"75,76,70,0.07,0.1,R,-1,0,0.1            ",
"75,76,70,0.06,0.1,R,-1,0,0.1            ",
";Row 2 (up 2)                           ",
"75,76,70,0.08,0.12,R,-1,0,0.1           ",
"75,76,70,0.07,0.12,R,-1,0,0.1           ",
"75,76,70,0.06,0.12,R,-1,0,0.1           ",
"75,76,70,0.05,0.12,R,-1,0,0.1           ",
"75,76,70,0.04,0.12,R,-1,0,0.1           ",
";Row 3 (up 3)                           ",
"75,76,70,0.06,0.14,R,-1,0,0.1           ",
"75,76,70,0.05,0.14,R,-1,0,0.1           ",
"75,76,70,0.04,0.14,R,-1,0,0.1           ",
"75,76,70,0.03,0.14,R,-1,0,0.1           ",
"75,76,70,0.02,0.14,R,-1,0,0.1           ",
";Row 4 (up 4)                           ",
"75,76,70,0.04,0.16,R,-1,0,0.1           ",
"75,76,70,0.03,0.16,R,-1,0,0.1           ",
"75,76,70,0.02,0.16,R,-1,0,0.1           ",
"75,76,70,0.01,0.16,R,-1,0,0.1           ",
"75,76,70,0.0,0.16,R,-1,0,0.1            ",
";Row 5 (up 5)                           ",
"75,76,70,0.02,0.18,R,-1,0,0.1           ",
"75,76,70,0.01,0.18,R,-1,0,0.1           ",
"75,76,70,0.0,0.18,R,-1,0,0.1            ",
"75,76,70,-0.01,0.18,R,-1,0,0.1          ",
"75,76,70,-0.02,0.18,R,-1,0,0.1          ",
";Row 6 (up 6)                           ",
"75,76,70,0.0,0.2,R,-1,0,0.1             ",
"75,76,70,-0.01,0.2,R,-1,0,0.1           ",
"75,76,70,-0.02,0.2,R,-1,0,0.1           ",
"75,76,70,-0.03,0.2,R,-1,0,0.1           ",
"75,76,70,-0.04,0.2,R,-1,0,0.1           ",
";Row 7 (up 7)                           ",
"75,76,70,-0.02,0.22,R,-1,0,0.1          ",
"75,76,70,-0.03,0.22,R,-1,0,0.1          ",
"75,76,70,-0.04,0.22,R,-1,0,0.1          ",
"75,76,70,-0.05,0.22,R,-1,0,0.1          ",
"75,76,70,-0.06,0.22,R,-1,0,0.1          ",
";Row 8 (up 8)                           ",
"75,76,70,-0.04,0.24,R,-1,0,0.1          ",
"75,76,70,-0.05,0.24,R,-1,0,0.1          ",
"75,76,70,-0.06,0.24,R,-1,0,0.1          ",
"75,76,70,-0.07,0.24,R,-1,0,0.1          ",
"75,76,70,-0.08,0.24,R,-1,0,0.1          ",
";Row 9 (up 9)                           ",
"75,76,70,-0.06,0.26,R,-1,0,0.1          ",
"75,76,70,-0.07,0.26,R,-1,0,0.1          ",
"75,76,70,-0.08,0.26,R,-1,0,0.1          ",
"75,76,70,-0.09,0.26,R,-1,0,0.1          ",
"75,76,70,-0.10,0.26,R,-1,0,0.1          ",
";Row 10 (up 10)                         ",
"75,76,70,-0.08,0.28,R,-1,0,0.1          ",
"75,76,70,-0.09,0.28,R,-1,0,0.1          ",
"75,76,70,-0.10,0.28,R,-1,0,0.1          ",
"75,76,70,-0.11,0.28,R,-1,0,0.1          ",
"75,76,70,-0.12,0.28,R,-1,0,0.1          ",
"                                        ",
";-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X.....",
"                                               ",
";Right rear stuff                              ",
";RR reverse (center)                           ",
"75,76,69,0.80,0.08,R,-1,0,0.1                  ",
"75,76,69,0.81,0.08,R,-1,0,0.1                  ",
"75,76,69,0.82,0.08,R,-1,0,0.1                  ",
"75,76,69,0.83,0.08,R,-1,0,0.1                  ",
"75,76,69,0.84,0.08,R,-1,0,0.1                  ",
"                                               ",
"75,76,69,0.80,0.1,R,-1,0,0.1                   ",
"75,76,69,0.81,0.1,R,-1,0,0.1                   ",
"75,76,69,0.82,0.1,R,-1,0,0.1                   ",
"75,76,69,0.83,0.1,R,-1,0,0.1                   ",
"75,76,69,0.84,0.1,R,-1,0,0.1                   ",
"                                               ",
"75,76,69,0.80,0.12,R,-1,0,0.1                  ",
"75,76,69,0.81,0.12,R,-1,0,0.1                  ",
"75,76,69,0.82,0.12,R,-1,0,0.1                  ",
"75,76,69,0.83,0.12,R,-1,0,0.1                  ",
"75,76,69,0.84,0.12,R,-1,0,0.1                  ",
"                                               ",
"75,76,69,0.80,0.14,R,-1,0,0.1                  ",
"75,76,69,0.81,0.14,R,-1,0,0.1                  ",
"75,76,69,0.82,0.14,R,-1,0,0.1                  ",
"75,76,69,0.83,0.14,R,-1,0,0.1                  ",
"75,76,69,0.84,0.14,R,-1,0,0.1                  ",
"                                               ",
"75,76,69,0.80,0.16,R,-1,0,0.1                  ",
"75,76,69,0.81,0.16,R,-1,0,0.1                  ",
"75,76,69,0.82,0.16,R,-1,0,0.1                  ",
"75,76,69,0.83,0.16,R,-1,0,0.1                  ",
"75,76,69,0.84,0.16,R,-1,0,0.1                  ",
"                                               ",
"75,76,69,0.80,0.18,R,-1,0,0.1                  ",
"75,76,69,0.81,0.18,R,-1,0,0.1                  ",
"75,76,69,0.82,0.18,R,-1,0,0.1                  ",
"75,76,69,0.83,0.18,R,-1,0,0.1                  ",
"75,76,69,0.84,0.18,R,-1,0,0.1                  ",
"                                               ",
"75,76,69,0.80,0.2,R,-1,0,0.1                   ",
"75,76,69,0.81,0.2,R,-1,0,0.1                   ",
"75,76,69,0.82,0.2,R,-1,0,0.1                   ",
"75,76,69,0.83,0.2,R,-1,0,0.1                   ",
"75,76,69,0.84,0.2,R,-1,0,0.1                   ",
"                                               ",
"75,76,69,0.80,0.22,R,-1,0,0.1                  ",
"75,76,69,0.81,0.22,R,-1,0,0.1                  ",
"75,76,69,0.82,0.22,R,-1,0,0.1                  ",
"75,76,69,0.83,0.22,R,-1,0,0.1                  ",
"75,76,69,0.84,0.22,R,-1,0,0.1                  ",
"                                               ",
"75,76,69,0.80,0.24,R,-1,0,0.1                  ",
"75,76,69,0.81,0.24,R,-1,0,0.1                  ",
"75,76,69,0.82,0.24,R,-1,0,0.1                  ",
"75,76,69,0.83,0.24,R,-1,0,0.1                  ",
"75,76,69,0.84,0.24,R,-1,0,0.1                  ",
"                                               ",
"75,76,69,0.80,0.26,R,-1,0,0.1                  ",
"75,76,69,0.81,0.26,R,-1,0,0.1                  ",
"75,76,69,0.82,0.26,R,-1,0,0.1                  ",
"75,76,69,0.83,0.26,R,-1,0,0.1                  ",
"75,76,69,0.84,0.26,R,-1,0,0.1                  ",
"                                               ",
"75,76,69,0.80,0.28,R,-1,0,0.1                  ",
"75,76,69,0.81,0.28,R,-1,0,0.1                  ",
"75,76,69,0.82,0.28,R,-1,0,0.1                  ",
"75,76,69,0.83,0.28,R,-1,0,0.1                  ",
"75,76,69,0.84,0.28,R,-1,0,0.1                  ",
"                                               ",
";RR BRAKE (center)                             ",
"75,76,69,0.86,0.08,b,-1,0,0.1                  ",
"75,76,69,0.87,0.08,b,-1,0,0.1                  ",
"75,76,69,0.88,0.08,b,-1,0,0.1                  ",
"75,76,69,0.89,0.08,b,-1,0,0.1                  ",
"75,76,69,0.90,0.08,b,-1,0,0.1                  ",
"                                               ",
"75,76,69,0.86,0.1,b,-1,0,0.1                   ",
"75,76,69,0.87,0.1,b,-1,0,0.1                   ",
"75,76,69,0.88,0.1,b,-1,0,0.1                   ",
"75,76,69,0.89,0.1,b,-1,0,0.1                   ",
"75,76,69,0.9,0.1,b,-1,0,0.1                    ",
"                                               ",
"75,76,69,0.86,0.12,b,-1,0,0.1                  ",
"75,76,69,0.87,0.12,b,-1,0,0.1                  ",
"75,76,69,0.88,0.12,b,-1,0,0.1                  ",
"75,76,69,0.89,0.12,b,-1,0,0.1                  ",
"75,76,69,0.9,0.12,b,-1,0,0.1                   ",
"                                               ",
"75,76,69,0.86,0.14,b,-1,0,0.1                  ",
"75,76,69,0.87,0.14,b,-1,0,0.1                  ",
"75,76,69,0.88,0.14,b,-1,0,0.1                  ",
"75,76,69,0.89,0.14,b,-1,0,0.1                  ",
"75,76,69,0.9,0.14,b,-1,0,0.1                   ",
"                                               ",
"75,76,69,0.86,0.16,b,-1,0,0.1                  ",
"75,76,69,0.87,0.16,b,-1,0,0.1                  ",
"75,76,69,0.88,0.16,b,-1,0,0.1                  ",
"75,76,69,0.89,0.16,b,-1,0,0.1                  ",
"75,76,69,0.9,0.16,b,-1,0,0.1                   ",
"                                               ",
"75,76,69,0.86,0.18,b,-1,0,0.1                  ",
"75,76,69,0.87,0.18,b,-1,0,0.1                  ",
"75,76,69,0.88,0.18,b,-1,0,0.1                  ",
"75,76,69,0.89,0.18,b,-1,0,0.1                  ",
"75,76,69,0.9,0.18,b,-1,0,0.1                   ",
"                                               ",
"75,76,69,0.86,0.2,b,-1,0,0.1                   ",
"75,76,69,0.87,0.2,b,-1,0,0.1                   ",
"75,76,69,0.88,0.2,b,-1,0,0.1                   ",
"75,76,69,0.89,0.2,b,-1,0,0.1                   ",
"75,76,69,0.9,0.2,b,-1,0,0.1                    ",
"                                               ",
"75,76,69,0.86,0.22,b,-1,0,0.1                  ",
"75,76,69,0.87,0.22,b,-1,0,0.1                  ",
"75,76,69,0.88,0.22,b,-1,0,0.1                  ",
"75,76,69,0.89,0.22,b,-1,0,0.1                  ",
"75,76,69,0.9,0.22,b,-1,0,0.1                   ",
"                                               ",
"75,76,69,0.86,0.24,b,-1,0,0.1                  ",
"75,76,69,0.87,0.24,b,-1,0,0.1                  ",
"75,76,69,0.88,0.24,b,-1,0,0.1                  ",
"75,76,69,0.89,0.24,b,-1,0,0.1                  ",
"75,76,69,0.9,0.24,b,-1,0,0.1                   ",
"                                               ",
"75,76,69,0.86,0.26,b,-1,0,0.1                  ",
"75,76,69,0.87,0.26,b,-1,0,0.1                  ",
"75,76,69,0.88,0.26,b,-1,0,0.1                  ",
"75,76,69,0.89,0.26,b,-1,0,0.1                  ",
"75,76,69,0.9,0.26,b,-1,0,0.1                   ",
"                                               ",
"75,76,69,0.86,0.28,b,-1,0,0.1                  ",
"75,76,69,0.87,0.28,b,-1,0,0.1                  ",
"75,76,69,0.88,0.28,b,-1,0,0.1                  ",
"75,76,69,0.89,0.28,b,-1,0,0.1                  ",
"75,76,69,0.9,0.28,b,-1,0,0.1                   ",
"                                               ",
";;;;;RR Bbs running>>><<<<                     ",
"75,76,69,0.86,0.08,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.87,0.08,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.88,0.08,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.89,0.08,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.90,0.08,f,-1,0,0.1 tracks/HDXred1   ",
"                                               ",
"75,76,69,0.86,0.1,f,-1,0,0.1 tracks/HDXred1    ",
"75,76,69,0.87,0.1,f,-1,0,0.1 tracks/HDXred1    ",
"75,76,69,0.88,0.1,f,-1,0,0.1 tracks/HDXred1    ",
"75,76,69,0.89,0.1,f,-1,0,0.1 tracks/HDXred1    ",
"75,76,69,0.9,0.1,f,-1,0,0.1 tracks/HDXred1     ",
"                                               ",
"75,76,69,0.86,0.12,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.87,0.12,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.88,0.12,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.89,0.12,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.9,0.12,f,-1,0,0.1 tracks/HDXred1    ",
"                                               ",
"75,76,69,0.86,0.14,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.87,0.14,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.88,0.14,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.89,0.14,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.9,0.14,f,-1,0,0.1 tracks/HDXred1    ",
"                                               ",
"75,76,69,0.86,0.16,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.87,0.16,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.88,0.16,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.89,0.16,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.9,0.16,f,-1,0,0.1 tracks/HDXred1    ",
"                                               ",
"75,76,69,0.86,0.18,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.87,0.18,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.88,0.18,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.89,0.18,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.9,0.18,f,-1,0,0.1 tracks/HDXred1    ",
"                                               ",
"75,76,69,0.86,0.2,f,-1,0,0.1 tracks/HDXred1    ",
"75,76,69,0.87,0.2,f,-1,0,0.1 tracks/HDXred1    ",
"75,76,69,0.88,0.2,f,-1,0,0.1 tracks/HDXred1    ",
"75,76,69,0.89,0.2,f,-1,0,0.1 tracks/HDXred1    ",
"75,76,69,0.9,0.2,f,-1,0,0.1 tracks/HDXred1     ",
"                                               ",
"75,76,69,0.86,0.22,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.87,0.22,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.88,0.22,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.89,0.22,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.9,0.22,f,-1,0,0.1 tracks/HDXred1    ",
"                                               ",
"75,76,69,0.86,0.24,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.87,0.24,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.88,0.24,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.89,0.24,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.9,0.24,f,-1,0,0.1 tracks/HDXred1    ",
"                                               ",
"75,76,69,0.86,0.26,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.87,0.26,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.88,0.26,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.89,0.26,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.9,0.26,f,-1,0,0.1 tracks/HDXred1    ",
"                                               ",
"75,76,69,0.86,0.28,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.87,0.28,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.88,0.28,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.89,0.28,f,-1,0,0.1 tracks/HDXred1   ",
"75,76,69,0.9,0.28,f,-1,0,0.1 tracks/HDXred1    ",
"                                               ",
";RR TURN SIGNAL (center)                       ",
"75,76,69,0.92,0.08,r,-1,400,0.1                ",
"75,76,69,0.93,0.08,r,-1,400,0.1                ",
"75,76,69,0.94,0.08,r,-1,400,0.1                ",
"75,76,69,0.95,0.08,r,-1,400,0.1                ",
"75,76,69,0.96,0.08,r,-1,400,0.1                ",
"                                               ",
"75,76,69,0.92,0.1,r,-1,400,0.1                 ",
"75,76,69,0.93,0.1,r,-1,400,0.1                 ",
"75,76,69,0.94,0.1,r,-1,400,0.1                 ",
"75,76,69,0.95,0.1,r,-1,400,0.1                 ",
"75,76,69,0.96,0.1,r,-1,400,0.1                 ",
"                                               ",
"75,76,69,0.92,0.12,r,-1,400,0.1                ",
"75,76,69,0.93,0.12,r,-1,400,0.1                ",
"75,76,69,0.94,0.12,r,-1,400,0.1                ",
"75,76,69,0.95,0.12,r,-1,400,0.1                ",
"75,76,69,0.96,0.12,r,-1,400,0.1                ",
"                                               ",
"75,76,69,0.92,0.14,r,-1,400,0.1                ",
"75,76,69,0.93,0.14,r,-1,400,0.1                ",
"75,76,69,0.94,0.14,r,-1,400,0.1                ",
"75,76,69,0.95,0.14,r,-1,400,0.1                ",
"75,76,69,0.96,0.14,r,-1,400,0.1                ",
"                                               ",
"75,76,69,0.92,0.16,r,-1,400,0.1                ",
"75,76,69,0.93,0.16,r,-1,400,0.1                ",
"75,76,69,0.94,0.16,r,-1,400,0.1                ",
"75,76,69,0.95,0.16,r,-1,400,0.1                ",
"75,76,69,0.96,0.16,r,-1,400,0.1                ",
"                                               ",
"75,76,69,0.92,0.18,r,-1,400,0.1                ",
"75,76,69,0.93,0.18,r,-1,400,0.1                ",
"75,76,69,0.94,0.18,r,-1,400,0.1                ",
"75,76,69,0.95,0.18,r,-1,400,0.1                ",
"75,76,69,0.96,0.18,r,-1,400,0.1                ",
"                                               ",
"75,76,69,0.92,0.2,r,-1,400,0.1                 ",
"75,76,69,0.93,0.2,r,-1,400,0.1                 ",
"75,76,69,0.94,0.2,r,-1,400,0.1                 ",
"75,76,69,0.95,0.2,r,-1,400,0.1                 ",
"75,76,69,0.96,0.2,r,-1,400,0.1                 ",
"                                               ",
"75,76,69,0.92,0.22,r,-1,400,0.1                ",
"75,76,69,0.93,0.22,r,-1,400,0.1                ",
"75,76,69,0.94,0.22,r,-1,400,0.1                ",
"75,76,69,0.95,0.22,r,-1,400,0.1                ",
"75,76,69,0.96,0.22,r,-1,400,0.1                ",
"                                               ",
"75,76,69,0.92,0.24,r,-1,400,0.1                ",
"75,76,69,0.93,0.24,r,-1,400,0.1                ",
"75,76,69,0.94,0.24,r,-1,400,0.1                ",
"75,76,69,0.95,0.24,r,-1,400,0.1                ",
"75,76,69,0.96,0.24,r,-1,400,0.1                ",
"                                               ",
"75,76,69,0.92,0.26,r,-1,400,0.1                ",
"75,76,69,0.93,0.26,r,-1,400,0.1                ",
"75,76,69,0.94,0.26,r,-1,400,0.1                ",
"75,76,69,0.95,0.26,r,-1,400,0.1                ",
"75,76,69,0.96,0.26,r,-1,400,0.1                ",
"                                               ",
"75,76,69,0.92,0.28,r,-1,400,0.1                ",
"75,76,69,0.93,0.28,r,-1,400,0.1                ",
"75,76,69,0.94,0.28,r,-1,400,0.1                ",
"75,76,69,0.95,0.28,r,-1,400,0.1                ",
"75,76,69,0.96,0.28,r,-1,400,0.1                ",
"",
"",
"                                                                                                                                 ",
"flares2                                                                                                                          ",
";RefNode,X,Y,OffsetX, OffsetY, OffsetZ, Type, ControlNumber, BlinkDelay, Size, MaterialName                                      ",
"71, 72, 73, 0.05, 0.04, 1.85, u, 4, 0, 1 tracks/HDXdome                                                                          ",
"                                                                                                                                 ",
"71, 72, 73, 0.12, 0.11, 1.88, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"71, 72, 73, 0.12, 0.30, 1.88, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"71, 72, 73, 0.12, 0.50, 1.89, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"71, 72, 73, 0.12, 0.70, 1.90, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"71, 72, 73, 0.12, 0.90, 1.91, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"                                                                                                                                 ",
"71, 72, 73, 0.92, 0.11, 1.89, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"71, 72, 73, 0.92, 0.30, 1.89, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"71, 72, 73, 0.92, 0.50, 1.90, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"71, 72, 73, 0.92, 0.70, 1.91, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"71, 72, 73, 0.92, 0.90, 1.92, u, 4, 0, 1.0 tracks/HDXdome                                                                        ",
"                                                                                                                                 ",
"69,70,73,-0.04,0.75, -2.3, u,1, 0, 1.1 tracks/HDX8yellow1                                                                        ",
"69,70,73,0.66,0.75, -2.3, u,1, 0, 1.1 tracks/HDX8yellow2                                                                         ",
"69,70,73,-0.1,0.6, -2.3, u,2, 0, 1.1 tracks/HDX8red1                                                                             ",
"69,70,73,0.8,0.6, -2.3, u,2, 0, 1.1 tracks/HDX8red2                                                                              ",
"67,68,75,0.09,0.62,1.3,b,-1,0,1                                                                                                  ",
"67,68,75,0.09,0.62,1.2,f,-1,0,1 tracks/HDXred                                                                                    ",
"67,68,75,0.91,0.62,1.3,b,-1,0,1                                                                                                  ",
"67,68,75,0.91,0.62,1.2,f,-1,0,1 tracks/HDXred                                                                                    ",
"67,68,75,0.18,0.62,1.2,R,-1,0,1                                                                                                  ",
"67,68,75,0.82,0.62,1.2,R,-1,0,1                                                                                                  ",
"                                                                                                                                 ",
"125,129,127,0,0,0,u,2,0,2 tracks/HDXstrobe                                                                                       ",
"125,129,127,0,0,0,u,3,0,2 tracks/HDXstrobe                                                                                       ",
"125,127,128,0,0,0,u,2,0,2 tracks/HDXstrobe                                                                                       ",
"125,127,128,0,0,0,u,3,0,2 tracks/HDXstrobe                                                                                       ",
"125,128,126,0,0,0,u,2,0,2 tracks/HDXstrobe                                                                                       ",
"125,128,126,0,0,0,u,3,0,2 tracks/HDXstrobe                                                                                       ",
"125,126,129,0,0,0,u,2,0,2 tracks/HDXstrobe                                                                                       ",
"125,126,129,0,0,0,u,3,0,2 tracks/HDXstrobe                                                                                       ",
"                                                                                                                                 ",
"                                                                                                                                 ",
"materialflarebindings                                                                                                            ",
"0, sign_STOP                                                                                                                     ",
"1, Indicator_yellowindicator                                                                                                     ",
"2, Indicator_redindicator                                                                                                        ",
"3, dome_domelight                                                                                                                ",
"//4, strobe_strobe                                                                                                               ",
"//4, strobe_strobeI                                                                                                              ",
"//5, strobe_strobe                                                                                                               ",
"//5, strobe_strobeI                                                                                                              ",
"                                                                                                                                 ",
"                                                                                                                                 ",
"                                                                                                                                 ",
"props                                                                                                                            ",
";ref,x,y,offsetx,offsety,offsetz,rotx,roty,rotz,mesh                                                                             ",
"                                                                                                                                 ",
"86,87,1,0,0,0,0,180,0, HDXbumper.mesh                                                                                            ",
"111,110,59,-0.2,-0.56,-0.02,0,-90,180, signback.mesh                                                                             ",
"107,104,45,-0.01,0.47,0.02,0,90,0, signback.mesh                                                                                 ",
"111,113,112,1,0.05,-0.02,0,0,180, sign.mesh                                                                                      ",
"107,105,106,1.26,-0.22,-0.02,0,0,180, sign.mesh                                                                                  ",
"53,54,55, -.32, 2.092, -1.13, 1, 0, 180, edoor.mesh                                                                              ",
"add_animation -90, 0, 0, source: event, mode: y-rotation,eventlock, event: COMMANDS_06                                           ",
"add_animation -1.42, 0, 0, source: event, mode: y-offset,eventlock, event: COMMANDS_06                                           ",
"add_animation 1.82, 0, 0, source: event, mode: x-offset,eventlock, event: COMMANDS_06                                            ",
"                                                                                                                                 ",
"//add_animation, 0.05, -1, 85, source: event, mode: x-rotation, autoanimate, noflip, bounce, eventlock, event: TRUCK_LIGHTTOGGLE5",
"                                                                                                                                 ",
"                                                                                                                                 ",
"53,54,55, -.32, 2.095, -1.145, 0.5, 0, 180, windowb.mesh                                                                         ",
"53,54,55, -.32, 2.095, -1.140, 0.5, 0, 180, windowf.mesh                                                                         ",
"add_animation, -0.18, 0, -0.18, source: event, mode: y-offset, autoanimate, bounce, eventlock, event: COMMANDS_05                ",
"                                                                                                                                 ",
"                                                                                                                                 ",
"101,100,98, 3.9, 0.35, -1.28, 90, -92, 180, HDXguard.mesh                                                                        ",
"                                                                                                                                 ",
";buttons!                                                                                                                        ",
";f/b, u/d, l/r                                                                                                                   ",
"86,84,77,0.219,0.865,0.201,27,-10,178, doorrocker.mesh                                                                           ",
"add_animation -8, 0, 0, source: event, mode: y-rotation, noflip, event: COMMANDS_03                                              ",
"add_animation 8, 0, 0, source: event, mode: x-rotation, noflip, event: COMMANDS_03                                               ",
"add_animation 8, 0, 0, source: event, mode: y-rotation, noflip, event: COMMANDS_04                                               ",
"add_animation -8, 0, 0, source: event, mode: x-rotation, noflip, event: COMMANDS_04                                              ",
"                                                                                                                                 ",
"86,84,77,0.280,0.850,0.201,18,-30,220, Indicator.mesh                                                                            ",
"                                                                                                                                 ",
"86,84,77,0.280,0.825,0.27,27,-10,181, button2.mesh                                                                               ",
"add_animation 10, 0, 0, source: event, mode: y-rotation, noflip, event: TRUCK_LIGHTTOGGLE1                                       ",
"add_animation -10, 0, 0, source: event, mode: x-rotation, noflip, event: TRUCK_LIGHTTOGGLE1                                      ",
"                                                                                                                                 ",
"86,84,77,0.335,0.818,0.270,-17,30,46, button3.mesh                                                                               ",
"add_animation -15, 0, 0, source: event, mode: y-rotation, noflip, event: TRUCK_LIGHTTOGGLE3                                      ",
"//add_animation 10, 0, 0, source: event, mode: x-rotation, noflip, event: TRUCK_LIGHTTOGGLE3                                     ",
"",
";wipers                                                                                                                            ",
"123, 72, 124, 0, 0, 0, 35, 180, -90, wiper.mesh                                                                                    ",
"add_animation, 0.05, -85, 1, source: event, mode: x-rotation, autoanimate, noflip, bounce, eventlock, event: TRUCK_LIGHTTOGGLE5    ",
"                                                                                                                                   ",
"124, 71, 123, 0, 0, 0, -35, 0, -90, wiper.mesh                                                                                     ",
"add_animation, 0.05, -1, 85, source: event, mode: x-rotation, autoanimate, noflip, bounce, eventlock, event: TRUCK_LIGHTTOGGLE5    ",
"                                                                                                                                   ",
"49, 77, 51, -2.4, 0.943, -0.02, 90, 0, -90, HDXrearview.mesh               ",
"                                                                           ",
"52,131,78,-0.5,0,0.1,86.5,180,160, HDXrmirror.mesh                         ",
"                                                                           ",
"51,130,77,-0.5,0,-0.1,89.5,180,200, HDXlmirror.mesh                        ",
"                                                                           ",
";interior                                                                  ",
"83,79,61,0.5,-0.06,-0.36,0,90,0, HDXseats.mesh                             ",
"120,121,122,1.2,0,0,0,90,0, HDXdseat.mesh                                  ",
"                                                                           ",
";ceiling                                                                   ",
"71, 72, 73, 0, 0.04, -0.10, 90, 90, 8, dome.mesh                           ",
"                                                                           ",
"71, 72, 73, 0.1, 0.11, -0.08, 90, 90, 8, dome.mesh                         ",
"71, 72, 73, 0.1, 0.30, -0.07, 90, 90, 8, dome.mesh                         ",
"71, 72, 73, 0.1, 0.50, -0.055, 90, 90, 8, dome.mesh                        ",
"71, 72, 73, 0.1, 0.70, -0.042, 90, 90, 8, dome.mesh                        ",
"71, 72, 73, 0.1, 0.90, -0.03, 90, 90, 8, dome.mesh                         ",
"                                                                           ",
"71, 72, 73, 0.9, 0.11, -0.08, 90, 90, -8, dome.mesh                        ",
"71, 72, 73, 0.9, 0.30, -0.07, 90, 90, -8, dome.mesh                        ",
"71, 72, 73, 0.9, 0.50, -0.055, 90, 90, -8, dome.mesh                       ",
"71, 72, 73, 0.9, 0.70, -0.042, 90, 90, -8, dome.mesh                       ",
"71, 72, 73, 0.9, 0.90, -0.03, 90, 90, -8, dome.mesh                        ",
"                                                                           ",
"71, 72, 73, 0.1, 0.21, -0.08, 0, 0, 8, speaker.mesh                        ",
"71, 72, 73, 0.1, 0.40, -0.07, 0, 0, 8, speaker.mesh                        ",
"71, 72, 73, 0.1, 0.595, -0.055, 0, 0, 8, speaker.mesh                      ",
"71, 72, 73, 0.1, 0.79, -0.042, 0, 0, 8, speaker.mesh                       ",
"                                                                           ",
"71, 72, 73, 0.9, 0.16, -0.08, 0, 0, -8, speaker.mesh                       ",
"71, 72, 73, 0.9, 0.35, -0.07, 0, 0, -8, speaker.mesh                       ",
"71, 72, 73, 0.9, 0.545, -0.055, 0, 0, -8, speaker.mesh                     ",
"71, 72, 73, 0.9, 0.79, -0.042, 0, 0, -8, speaker.mesh                      ",
"                                                                           ",
"86,55,51, 0.55,0.62, -0.01, 0, 90, 180, number.mesh                        ",
"77, 49, 62, -0.03, 0, 1.25, 0, 184, 90, number.mesh                        ",
"78, 50, 52, 0.1, -0.26, -0.03, 183.5, 90, 0, number.mesh                   ",
"87,56,52, 0.475,0.62, 0.01, 0, 90, 0, number.mesh                          ",
"67,68,75 0.425, 0.8, -0.039, 0, 90, 180, number.mesh                       ",
"51,77,52 -0.1, 0.75, -0.008, 0, -180, 180, number.mesh                     ",
"                                                                           ",
"//75,76,69, 0.04, 0.049, -0.032, 90, 0, -90, leftrearsignal.mesh           ",
"//85, 55, 46, 0.05, 6.3, 0, 90, 90, -90, leftsidesignal1.mesh              ",
"                                                                           ",
"                                                                           ",
";dashboard                                                                 ",
"49,77,51,-0.38,-0.10,0.14,55,0,180, 20cmspedo.mesh                         ",
"49,77,51,0.15,-.11,0.14,55,0,180, 20cmtach.mesh                            ",
"49,77,51,-1.10,-.11,0.18,55,0,180, 20cmselector.mesh                       ",
"                                                                           ",
"118,119,117,0.615,0.08,0.05,-90,0,0, swheel.mesh                           ",
"add_animation, -500, -500, 500, source: steeringwheel, mode: y-rotation    ",
"                                                                           ",
"//add_animation 200, 0, 0, source: steeringwheel, mode: z-rotation         ",
"//add_animation -200, 0, 0, source: steeringwheel, mode: z-rotation        ",
"                                                                           ",
"73,74,71,0.5,0.1,-0.11,0,0,180, strobe.mesh                                ",
"                                                                           ",
"                                                                           ",
"53,54,55, -0.320, 2.095, -1.135, 1, 0, 180, HDX.mesh                       ",
"                                                                           ",
"//add_animation -0.007, 0, 0, source: parking, mode:z-offset               ",
"                                                                           ",
"                                                                           ",
"flexbodies                                                                 ",
"91,90,93,-3.1,0.46,1,91,0,90,HDXdoor.mesh                                  ",
"forset 90, 91, 96, 93                                                      ",
"                                                                           ",
"92,89,94,2.85,0.465,-1,91,0,-90,HDXdoor.mesh                               ",
"forset 89, 92, 94, 95                                                      ",
"                ",
"                ",
"submesh         ",
";sides          ",
"87,68,52,cu     ",
"70,52,68,cu     ",
"86,51,67,cu     ",
"69,67,51,cu     ",
"                ",
"76,75,78,cu     ",
"75,77,76,cu     ",
"                ",
"                ",
"82,54,48,cu     ",
"58,64,56,cu     ",
"54,58,56,cu     ",
"56,54,47,cu     ",
"                ",
"85,45,55,cu     ",
"57,55,59,cu     ",
"53,55,57,cu     ",
"55,57,53,cu     ",
"                ",
";back           ",
"68,67,70,cu     ",
"76,75,68,cu     ",
"69,70,67,cu     ",
"75,76,67,cu     ",
"116,75,67,cu    ",
"116,76,68,cu    ",
"                ",
"11,10,9,cu      ",
"10,11,8,cu      ",
"66,62,68,cu     ",
"65,60,67,cu     ",
"                ",
"88,87,79,cu     ",
"84,86,52,cu     ",
"                ",
";roof           ",
"73,74,71,cu     ",
"74,73,72,cu     ",
"                ",
";bottom         ",
"86,87,67,cu     ",
"87,86,68,cu     ",
"                ",
";front          ",
"87,52,86,cu     ",
"86,87,51,cu     ",
"77,87,78,cu     ",
"78,77,87,cu     ",
"49,86,78,cu     ",
"50,77,87,cu     ",
"                ",
";props          ",
"101,100,103,cu  ",
"114,110,115,cu  ",
"113,111,112,cu  ",
"109,104,108,cu  ",
"105,106,107,cu  ",
"                ",
"130,51,71,cu    ",
"131,52,72,cu    ",
"                ",
"end             "};

static int keyword;

/// ########################## SOLUTION1 - REGEXES  #################################

#define E_DELIMITER_SPACE "[[:blank:]]+"

#define DEFINE_REGEX_IGNORECASE(_NAME_,_REGEXP_) \
    const std::regex _NAME_ = std::regex( _REGEXP_, std::regex::ECMAScript | std::regex::icase);

/// A keyword which should be on it's own line. Used in IDENTIFY_KEYWORD.
#define E_KEYWORD_BLOCK(_NAME_) \
    "(^" _NAME_ "[[:blank:]]*$)?"

/// A keyword which should have values following it. Used in IDENTIFY_KEYWORD.
#define E_KEYWORD_INLINE(_NAME_) \
    "(^" _NAME_ E_DELIMITER_SPACE ".*$)?"
    
/// Inline keyword, tolerant version: keyword and values can be delimited by either space or comma
#define E_KEYWORD_INLINE_TOLERANT(_NAME_) \
    "(^" _NAME_ "[[:blank:],]+" ".*$)?"

// IMPORTANT! If you add a value here, you must also modify File::Keywords enum, it relies on positions in this regex
#define IDENTIFY_KEYWORD_REGEX_STRING          \
    E_KEYWORD_INLINE_TOLERANT("add_animation") \
    E_KEYWORD_BLOCK("airbrakes")                   E_KEYWORD_BLOCK("animators")       \
    E_KEYWORD_INLINE("AntiLockBrakes")             E_KEYWORD_BLOCK("axles")                   \
    E_KEYWORD_INLINE("author")                     E_KEYWORD_BLOCK("backmesh")                \
    E_KEYWORD_BLOCK("beams")                       E_KEYWORD_BLOCK("brakes")                  \
    E_KEYWORD_BLOCK("cab")                         E_KEYWORD_BLOCK("camerarail")              \
    E_KEYWORD_BLOCK("cameras")                     E_KEYWORD_BLOCK("cinecam")                 \
    E_KEYWORD_BLOCK("collisionboxes")              E_KEYWORD_BLOCK("commands")                \
    E_KEYWORD_BLOCK("commands2")                   E_KEYWORD_BLOCK("contacters")              \
    E_KEYWORD_INLINE("cruisecontrol")              E_KEYWORD_BLOCK("description")             \
    E_KEYWORD_INLINE("detacher_group")             E_KEYWORD_BLOCK("disabledefaultsounds")    \
    E_KEYWORD_BLOCK("enable_advanced_deformation")                \
    E_KEYWORD_BLOCK("end")                         E_KEYWORD_BLOCK("end_section")             \
    E_KEYWORD_BLOCK("engine")                      E_KEYWORD_BLOCK("engoption")               \
    E_KEYWORD_BLOCK("engturbo")                    E_KEYWORD_BLOCK("envmap")                  \
    E_KEYWORD_BLOCK("exhausts")                    E_KEYWORD_INLINE("extcamera")              \
    E_KEYWORD_INLINE("fileformatversion")          E_KEYWORD_INLINE("fileinfo")               \
    E_KEYWORD_BLOCK("fixes")                       E_KEYWORD_BLOCK("flares")                  \
    E_KEYWORD_BLOCK("flares2")                     E_KEYWORD_BLOCK("flexbodies")              \
    E_KEYWORD_INLINE("flexbody_camera_mode")       E_KEYWORD_BLOCK("flexbodywheels")          \
    E_KEYWORD_BLOCK("forwardcommands")             E_KEYWORD_BLOCK("fusedrag")                \
    E_KEYWORD_BLOCK("globals")                     E_KEYWORD_INLINE("guid")                   \
    E_KEYWORD_BLOCK("guisettings")                 E_KEYWORD_BLOCK("help")                    \
    E_KEYWORD_BLOCK("hideInChooser")               E_KEYWORD_BLOCK("hookgroup")               \
    E_KEYWORD_BLOCK("hooks")                       E_KEYWORD_BLOCK("hydros")                  \
    E_KEYWORD_BLOCK("importcommands")              E_KEYWORD_BLOCK("lockgroups")              \
    E_KEYWORD_BLOCK("lockgroup_default_nolock")    E_KEYWORD_BLOCK("managedmaterials")        \
    E_KEYWORD_BLOCK("materialflarebindings")       E_KEYWORD_BLOCK("meshwheels")              \
    E_KEYWORD_BLOCK("meshwheels2")                 E_KEYWORD_BLOCK("minimass")                \
    E_KEYWORD_BLOCK("nodecollision")               E_KEYWORD_BLOCK("nodes")                   \
    E_KEYWORD_BLOCK("nodes2")                      E_KEYWORD_BLOCK("particles")               \
    E_KEYWORD_BLOCK("pistonprops")                 E_KEYWORD_INLINE("prop_camera_mode")       \
    E_KEYWORD_BLOCK("props")                       E_KEYWORD_BLOCK("railgroups")              \
    E_KEYWORD_BLOCK("rescuer")                     E_KEYWORD_BLOCK("rigidifiers")             \
    E_KEYWORD_BLOCK("rollon")                      E_KEYWORD_BLOCK("ropables")                \
    E_KEYWORD_BLOCK("ropes")                       E_KEYWORD_BLOCK("rotators")                \
    E_KEYWORD_BLOCK("rotators2")                     E_KEYWORD_BLOCK("screwprops")                  \
    E_KEYWORD_INLINE("section")                      E_KEYWORD_INLINE("sectionconfig")              \
    E_KEYWORD_INLINE("set_beam_defaults")            E_KEYWORD_INLINE("set_beam_defaults_scale")    \
    E_KEYWORD_INLINE("set_collision_range")          E_KEYWORD_INLINE("set_inertia_defaults")       \
    E_KEYWORD_INLINE("set_managedmaterials_options") E_KEYWORD_INLINE("set_node_defaults")          \
    E_KEYWORD_BLOCK("set_shadows")                   E_KEYWORD_INLINE("set_skeleton_settings")      \
    E_KEYWORD_BLOCK("shocks")                        E_KEYWORD_BLOCK("shocks2")                     \
    E_KEYWORD_BLOCK("slidenode_connect_instantly")   E_KEYWORD_BLOCK("slidenodes")                  \
    E_KEYWORD_INLINE("SlopeBrake")                   E_KEYWORD_BLOCK("soundsources")                \
    E_KEYWORD_BLOCK("soundsources2")                 E_KEYWORD_INLINE("speedlimiter")               \
    E_KEYWORD_BLOCK("submesh")                       E_KEYWORD_INLINE("submesh_groundmodel")        \
    E_KEYWORD_BLOCK("texcoords")                     E_KEYWORD_BLOCK("ties")                        \
    E_KEYWORD_BLOCK("torquecurve")                   E_KEYWORD_INLINE("TractionControl")            \
    E_KEYWORD_BLOCK("triggers")                      E_KEYWORD_BLOCK("turbojets")                   \
    E_KEYWORD_BLOCK("turboprops")                    E_KEYWORD_BLOCK("turboprops2")                 \
    E_KEYWORD_BLOCK("videocamera")                   E_KEYWORD_BLOCK("wheeldetachers")              \
    E_KEYWORD_BLOCK("wheels")                        E_KEYWORD_BLOCK("wheels2")                     \
    E_KEYWORD_BLOCK("wings")

std::vector<std::string> lines_vec;

DEFINE_REGEX_IGNORECASE( IDENTIFY_KEYWORD_IGNORE_CASE,  IDENTIFY_KEYWORD_REGEX_STRING )

void PrepareBench_sol1()
{
    int count = sizeof(trucklines)/sizeof(const char*);
    for (int i = 0; i < count; ++i)
    {
        lines_vec.emplace_back(std::string(trucklines[0]));
    }
}

unsigned FindKeywordMatch(std::smatch& search_results)
{
    // The 'results' array contains a complete match at positon [0] and sub-matches starting with [1], 
    //    so we get exact positions in Regexes::IDENTIFY_KEYWORD, which again match File::Keyword enum members
        
    for (unsigned int i = 1; i < search_results.size(); i++)
    {
        std::ssub_match sub  = search_results[i];
        if (sub.matched)
        {
            // Build enum value directly from result offset 
            return i;
        }
    }
    return INT_MAX;
}

static void Bench_sol1__Regex(benchmark::State& state)
{
    std::smatch results;
    while (state.KeepRunning()) 
    {
        int count = (int) lines_vec.size();
        for (int i = 0; i < count; ++i)
        {
            std::regex_search(lines_vec[i], results, IDENTIFY_KEYWORD_IGNORE_CASE); // Always returns true.
            keyword = FindKeywordMatch(results);
        }
    }
}
BENCHMARK(Bench_sol1__Regex);

static void Bench_sol1b_RegexPreCond(benchmark::State& state)
{
    using namespace std;
    std::smatch results;
    while (state.KeepRunning()) 
    {
        int count = (int) lines_vec.size();
        for (int i = 0; i < count; ++i)
        {
            // precondition
            char c = lines_vec[i][0];
            //if ((c >= (int)'0' && c <= (int)'9'))
            if (! ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
            {
                continue;
            }
            // end precond.

            std::regex_search(lines_vec[i], results, IDENTIFY_KEYWORD_IGNORE_CASE); // Always returns true.
            keyword = FindKeywordMatch(results);
        }
    }
}
BENCHMARK(Bench_sol1b_RegexPreCond);

static void Bench_sol1c_RegexPreCondIsdigit(benchmark::State& state)
{
    using namespace std;
    std::smatch results;
    while (state.KeepRunning()) 
    {
        int count = (int) lines_vec.size();
        for (int i = 0; i < count; ++i)
        {
            // precondition - use isdigit()
            char c = lines_vec[i][0];
            if (isdigit(c))
            {
                continue;
            }
            // end precond.

            std::regex_search(lines_vec[i], results, IDENTIFY_KEYWORD_IGNORE_CASE); // Always returns true.
            keyword = FindKeywordMatch(results);
        }
    }
}
BENCHMARK(Bench_sol1c_RegexPreCondIsdigit);
    // Scores ~2x worse than non-isdigit() test (Win7/MSVC2015/IntelCore i5-3570K@3.4Ghz) --only_a_ptr, 10/2016

static void Bench_sol1d_RegexPreCondIsAlpha(benchmark::State& state)
{
    using namespace std;
    std::smatch results;
    while (state.KeepRunning()) 
    {
        int count = (int) lines_vec.size();
        for (int i = 0; i < count; ++i)
        {
            // precondition - use isdigit()
            char c = lines_vec[i][0];
            if (!isalpha(c))
            {
                continue;
            }
            // end precond.

            std::regex_search(lines_vec[i], results, IDENTIFY_KEYWORD_IGNORE_CASE); // Always returns true.
            keyword = FindKeywordMatch(results);
        }
    }
}
BENCHMARK(Bench_sol1d_RegexPreCondIsAlpha);
    // Scores ~2x worse than non-isdigit() test (Win7/MSVC2015/IntelCore i5-3570K@3.4Ghz) --only_a_ptr, 10/2016

// ################################# Solution 2 - switch ######################################

#ifndef WIN32 
  #define stricmp strcasecmp
  #define strnicmp strncasecmp
#endif

// Compare both upper&lower-case ASCII character
#define CASE(_LOWER_)               case(_LOWER_): case((_LOWER_)-32)
// Match line against keyword (start with 2nd character)
#define MATCH(_STR_, _KWORD_)       if (stricmp(line+1, _STR_+1) == 0) return _KWORD_;

Keyword IdentifyKeywordSwitch(const char* line)
{
    switch (line[0])
    {
    CASE('a'):
        MATCH("add_animation",                KEYWORD_ADD_ANIMATION               );
        MATCH("airbrakes",                    KEYWORD_AIRBRAKES                   );
        MATCH("animators",                    KEYWORD_ANIMATORS                   );
        MATCH("AntiLockBrakes",               KEYWORD_ANTI_LOCK_BRAKES            );
        MATCH("axles",                        KEYWORD_AXLES                       );
        MATCH("author",                       KEYWORD_AUTHOR                      );
        break;
    CASE('b'):
        MATCH("backmesh",                     KEYWORD_BACKMESH                    );
        MATCH("beams",                        KEYWORD_BEAMS                       );
        MATCH("brakes",                       KEYWORD_BRAKES                      );
        break;
    CASE('c'):
        MATCH("cab",                          KEYWORD_CAB                         );
        MATCH("camerarail",                   KEYWORD_CAMERARAIL                  );
        MATCH("cameras",                      KEYWORD_CAMERAS                     );
        MATCH("cinecam",                      KEYWORD_CINECAM                     );
        MATCH("collisionboxes",               KEYWORD_COLLISIONBOXES              );
        MATCH("commands",                     KEYWORD_COMMANDS                    );
        MATCH("commands2",                    KEYWORD_COMMANDS2                   );
        MATCH("contacters",                   KEYWORD_CONTACTERS                  );
        MATCH("cruisecontrol",                KEYWORD_CRUISECONTROL               );
        break;
    CASE('d'):
        MATCH("description",                  KEYWORD_DESCRIPTION                 );
        MATCH("detacher_group",               KEYWORD_DETACHER_GROUP              );
        MATCH("disabledefaultsounds",         KEYWORD_DISABLEDEFAULTSOUNDS        );
        break;
    CASE('e'):
        MATCH("enable_advanced_deformation",  KEYWORD_ENABLE_ADVANCED_DEFORMATION );
        MATCH("end",                          KEYWORD_END                         );
        MATCH("end_section",                  KEYWORD_END_SECTION                 );
        MATCH("engine",                       KEYWORD_ENGINE                      );
        MATCH("engoption",                    KEYWORD_ENGOPTION                   );
        MATCH("engturbo",                     KEYWORD_ENGTURBO                    );
        MATCH("envmap",                       KEYWORD_ENVMAP                      );
        MATCH("exhausts",                     KEYWORD_EXHAUSTS                    );
        MATCH("extcamera",                    KEYWORD_EXTCAMERA                   );
        break;
    CASE('f'):
        MATCH("fileformatversion",            KEYWORD_FILEFORMATVERSION           );
        MATCH("fileinfo",                     KEYWORD_FILEINFO                    );
        MATCH("fixes",                        KEYWORD_FIXES                       );
        MATCH("flares",                       KEYWORD_FLARES                      );
        MATCH("flares2",                      KEYWORD_FLARES2                     );
        MATCH("flexbodies",                   KEYWORD_FLEXBODIES                  );
        MATCH("flexbody_camera_mode",         KEYWORD_FLEXBODY_CAMERA_MODE        );
        MATCH("flexbodywheels",               KEYWORD_FLEXBODYWHEELS              );
        MATCH("forwardcommands",              KEYWORD_FORWARDCOMMANDS             );
        MATCH("fusedrag",                     KEYWORD_FUSEDRAG                    );
        break;
    CASE('g'):
        MATCH("globals",                      KEYWORD_GLOBALS                     );
        MATCH("guid",                         KEYWORD_GUID                        );
        MATCH("guisettings",                  KEYWORD_GUISETTINGS                 );
        break;
    CASE('h'):
        MATCH("help",                         KEYWORD_HELP                        );
        MATCH("hideInChooser",                KEYWORD_HIDE_IN_CHOOSER             );
        MATCH("hookgroup",                    KEYWORD_HOOKGROUP                   );
        MATCH("hooks",                        KEYWORD_HOOKS                       );
        MATCH("hydros",                       KEYWORD_HYDROS                      );
        break;
    CASE('i'):
        MATCH("importcommands",               KEYWORD_IMPORTCOMMANDS              );
        break;
    CASE('l'):
        MATCH("lockgroups",                   KEYWORD_LOCKGROUPS                  );
        MATCH("lockgroup_default_nolock",     KEYWORD_LOCKGROUP_DEFAULT_NOLOCK    );
        break;
    CASE('m'):
        MATCH("managedmaterials",             KEYWORD_MANAGEDMATERIALS            );
        MATCH("materialflarebindings",        KEYWORD_MATERIALFLAREBINDINGS       );
        MATCH("meshwheels",                   KEYWORD_MESHWHEELS                  );
        MATCH("meshwheels2",                  KEYWORD_MESHWHEELS2                 );
        MATCH("minimass",                     KEYWORD_MINIMASS                    );
        break;
    CASE('n'):
        MATCH("nodecollision",                KEYWORD_NODECOLLISION               );
        MATCH("nodes",                        KEYWORD_NODES                       );
        MATCH("nodes2",                       KEYWORD_NODES2                      );
        break;
    CASE('p'):
        MATCH("particles",                    KEYWORD_PARTICLES                   );
        MATCH("pistonprops",                  KEYWORD_PISTONPROPS                 );
        MATCH("prop_camera_mode",             KEYWORD_PROP_CAMERA_MODE            );
        MATCH("props",                        KEYWORD_PROPS                       );
        break;
    CASE('r'):
        MATCH("railgroups",                   KEYWORD_RAILGROUPS                  );
        MATCH("rescuer",                      KEYWORD_RESCUER                     );
        MATCH("rigidifiers",                  KEYWORD_RIGIDIFIERS                 );
        MATCH("rollon",                       KEYWORD_ROLLON                      );
        MATCH("ropables",                     KEYWORD_ROPABLES                    );
        MATCH("ropes",                        KEYWORD_ROPES                       );
        MATCH("rotators",                     KEYWORD_ROTATORS                    );
        MATCH("rotators2",                    KEYWORD_ROTATORS2                   );
        break;
    CASE('s'):
        MATCH("screwprops",                   KEYWORD_SCREWPROPS                  );
        MATCH("section",                      KEYWORD_SECTION                     );
        MATCH("sectionconfig",                KEYWORD_SECTIONCONFIG               );
        MATCH("set_beam_defaults",            KEYWORD_SET_BEAM_DEFAULTS           );
        MATCH("set_beam_defaults_scale",      KEYWORD_SET_BEAM_DEFAULTS_SCALE     );
        MATCH("set_collision_range",          KEYWORD_SET_COLLISION_RANGE         );
        MATCH("set_inertia_defaults",         KEYWORD_SET_INERTIA_DEFAULTS        );
        MATCH("set_managedmaterials_options", KEYWORD_SET_MANAGEDMATERIALS_OPTIONS);
        MATCH("set_node_defaults",            KEYWORD_SET_NODE_DEFAULTS           );
        MATCH("set_shadows",                  KEYWORD_SET_SHADOWS                 );
        MATCH("set_skeleton_settings",        KEYWORD_SET_SKELETON_SETTINGS       );
        MATCH("shocks",                       KEYWORD_SHOCKS                      );
        MATCH("shocks2",                      KEYWORD_SHOCKS2                     );
        MATCH("slidenode_connect_instantly",  KEYWORD_SLIDENODE_CONNECT_INSTANTLY );
        MATCH("slidenodes",                   KEYWORD_SLIDENODES                  );
        MATCH("SlopeBrake",                   KEYWORD_SLOPE_BRAKE                 );
        MATCH("soundsources",                 KEYWORD_SOUNDSOURCES                );
        MATCH("soundsources2",                KEYWORD_SOUNDSOURCES2               );
        MATCH("speedlimiter",                 KEYWORD_SPEEDLIMITER                );
        MATCH("submesh",                      KEYWORD_SUBMESH                     );
        MATCH("submesh_groundmodel",          KEYWORD_SUBMESH_GROUNDMODEL         );
        break;
    CASE('t'):
        MATCH("texcoords",                    KEYWORD_TEXCOORDS                   );
        MATCH("ties",                         KEYWORD_TIES                        );
        MATCH("torquecurve",                  KEYWORD_TORQUECURVE                 );
        MATCH("TractionControl",              KEYWORD_TRACTION_CONTROL            );
        MATCH("triggers",                     KEYWORD_TRIGGERS                    );
        MATCH("turbojets",                    KEYWORD_TURBOJETS                   );
        MATCH("turboprops",                   KEYWORD_TURBOPROPS                  );
        MATCH("turboprops2",                  KEYWORD_TURBOPROPS2                 );
        break;
    CASE('v'):
        MATCH("videocamera",                  KEYWORD_VIDEOCAMERA                 );
        break;
    CASE('w'):
        MATCH("wheeldetachers",               KEYWORD_WHEELDETACHERS              );
        MATCH("wheels",                       KEYWORD_WHEELS                      );
        MATCH("wheels2",                      KEYWORD_WHEELS2                     );
        MATCH("wings",                        KEYWORD_WINGS                       );
        break;

    default:
        return KEYWORD_INVALID;
    }
}

static void Bench_sol2__Switch(benchmark::State& state)
{
    while (state.KeepRunning()) 
    {
        int count = sizeof(trucklines)/sizeof(const char*);
        for (int i = 0; i < count; ++i)
        {
            keyword = (int) IdentifyKeywordSwitch(trucklines[i]);
        }
    }
}
BENCHMARK(Bench_sol2__Switch);

static void Bench_sol2b_SwitchPreCond(benchmark::State& state)
{
    while (state.KeepRunning()) 
    {
        int count = sizeof(trucklines)/sizeof(const char*);
        for (int i = 0; i < count; ++i)
        {
            // precondition
            char c = trucklines[i][0];
            if (! ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
            {
                keyword = (int) KEYWORD_INVALID;
                continue;
            }
            // precondition

            keyword = (int) IdentifyKeywordSwitch(trucklines[i]);
        }
    }
}
BENCHMARK(Bench_sol2b_SwitchPreCond);

int main(int argc, char** argv)
{
    using namespace std;

    // prepare
    cout << "Preparing..." << endl;
    PrepareBench_sol1();


    // benchmark
    ::benchmark::Initialize(&argc, argv);  
    ::benchmark::RunSpecifiedBenchmarks(); 
#ifdef _MSC_VER
    system("pause");
#endif
    return (int) keyword;
}


