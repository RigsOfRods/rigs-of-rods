================================================================================
    Softbody definition file format "Truck" specification
    Project Rigs of Rods (http://www.rigsofrods.org)
================================================================================

INTRO
=====

"Truck" is a human-friendly custom format for defining softbody game entities
- trucks, cars, boats, airplanes, loads and fixed objects.

VERSION
=======

This spec applies to Rigs of Rods version 0.4.5 and further.
This spec covers all prior versions of the format: 1, 2, 3, 450

SYNTAX
======

The format is line-based.

Lines starting with semicolon ";" or slash "/" are comments.
NOTE: The "/" comments were documented as "//", but the parser
only ever checked the first character.
IMPORTANT: Comments MUST be on separate line! Trailing comments are not supported. 

There are several syntaxes for parsing a line:

    Default
        The most usual and relaxed one
        Entire line is treated as values separated by separators. 
        Possible separators: space, tabulator, comma ",", colon ":" or pipe "|".
        Whitespace and "," are well known, others weren't documented as generic.
        Multiple separators in a row squash into one, i.e. this is a valid line:
        set_beam_defaults |,| -1 -1 ,,,,, -1, -1

    Keyword-space-CSV
        More complex sections
        Line consists of: keyword, space (separator), CSV (comma sep. values)
        The keyword is cut away, the rest is split along ",".

    Keyword-CSV
        Like above, except the space is optional.
    
    CSV
        Classic comma-separated-values. Only separator is ','
    
    CommaSpaceSV
        Separators are space or comma.

The format consists of these elements:

    Title
        The first line of the file is a title. 
	
    Inline-section
        Example: author.
        Has a keyword at start of line and data follow on the same line.
        Following lines do not belong to any section.

    Block-section
        Example: nodes.
        Begins with a line containing nothing but a keyword.
        All subsequent lines belong to it until another
          section (any type) is encountered or module closed.
          
    Animator-section
        Keyword: `animators`
        Works like block-section, except lines have special syntax.
        See chapter "SECTION ANIMATORS" for details.                

    Multiline-section
        Single instance: "description/end_description".
        Begins with a line containing nothing but "description".
        Ends with a line containing nothing but "end_description".
        All lines in between belong to it. Keywords inside it are ignored.
        Subsequent lines belong to no section.
    	
    Multiline-comment
        Single instance: "comment/end_comment".
        Begins with a line containing nothing but "description".
        Ends with a line containing nothing but "end_description".
        All lines in between belong to it. Keywords inside it are ignored.
        Can appear inside any block-section, does not affect it.

    Directive
        Example: set_beam_defaults
        Consists of keyword at the beginning of the line and data on the same line.
        May perform various task, usualy set global attributes or change
          behavior of the parsing.
        Directive may appear in any block-section.

    Module
        Begins with a line containing nothing but "section".
        Ends with a line containing nothing but "end_section".
        A chunk of file which represents an optional modification of the vehicle.
        See below for details.

COMPATIBILITY
=============

Since RoR v0.4.5, all elements fully support named nodes.
In previous RoR versions, support was partial and undocumented.

Since RoR v0.4.5 the order of elements in truckfile doesn't matter.
The spawn process follows a pre-defined order of elements. 
Exceptions:
    add_animation (must be after section 'prop')
    flexbody_camera_mode (must be after section 'flexbody')
In previous versions, the order of elements defined the final result
and some combinations resulted in a crash.

Since v0.4.5, all elements can be modularized using 'section' keyword, except:
    author
    description
    disabledefaultsounds
    enable_advanced_deformation
    fileformatversion
    fileinfo
    forwardcommands
    guid
    hideInChooser
    importcommands
    lockgroup_default_nolock
    minimass
    rescuer
    rollon
In previous versions, this was undocumented.

Since v0.4.5, keyword 'sectionconfig' is no longer necessary and parser silently ignores it.
Modules are created as keyword 'section' is encountered.    

Since v0.4.5 sections 'nodes' and 'nodes2' both support named nodes.

Since v0.4.5 sections 'commands' & 'commands2' are unified.
The only difference is in compress & expand ratios.
Both support the same flags and can use multiple flags at once.

Since v0.4.5 subsection 'flexbodies'/'forset' supports named nodes.
However, ranges are only supported for nubered nodes.

Since v0.4.5 the 'end' keyword is no longer needed. Parser silently ignores it.
In previous versions, it was critical - without it, RoR crashed.

Since v0.4.5 the optional param 'wings/control_type' now has default 'n'
Old parser had no default, though the param is optional.

DOCUMENTATION
=============

See http://docs.rigsofrods.org/vehicle-creation/fileformat-truck

Below is a list of all supported elements.
NOTE: All keywords are case insensitive, 
the lettercase in this list corresponds to original documentation.

    NAME                         TYPE       SYNTAX
                                            
    advdrag                      BLOCK      
    add_animation                DIRECTIVE  Keyword-Space-CSV + custom proc.
    airbrakes                    BLOCK      Default
    animators                    ANIMATORS  
    AntiLockBrakes               DIRECTIVE  Keyword-CSV      
    author                       DIRECTIVE  Default
    axles                        BLOCK      CSV + custom processing
    beams                        BLOCK      
    brakes                       BLOCK      
    camerarail                   BLOCK      
    cameras                      BLOCK      
    cinecam                      BLOCK      
    collisionboxes               BLOCK      
    commands                     BLOCK      
    commands2                    BLOCK      
    comment                      COMMENT    
    contacters                   BLOCK      
    cruisecontrol                BLOCK      Default
    description                  BLOCK      
    detacher_group               DIRECTIVE  Default
    disabledefaultsounds         DIRECTIVE  
    enable_advanced_deformation  DIRECTIVE  
    engine                       BLOCK      
    engoption                    BLOCK      
    engturbo                     BLOCK      
    envmap                       BLOCK      
    exhausts                     BLOCK      
    extcamera                    INLINE     
    forwardcommands              DIRECTIVE  
    fileformatversion            INLINE     
    fileinfo                     DIRECTIVE     
    fixes                        BLOCK      
    flares                       BLOCK      
    flares2                      BLOCK      
    flexbodies                   BLOCK      
    flexbody_camera_mode         DIRECTIVE  Default
    flexbodywheels               BLOCK      
    forset                       INLINE     Keyword-CSV
    fusedrag                     BLOCK      
    globals                      BLOCK      Default
    guid                         DIRECTIVE  Default
    guisettings                  BLOCK      
    help                         BLOCK      Default
    hideInChooser                DIRECTIVE  
    hookgroup                    BLOCK      
    hooks                        BLOCK      
    hydros                       BLOCK      
    importcommands               DIRECTIVE  
    interaxles                   BLOCK      CSV + custom processing
    lockgroups                   BLOCK      
    lockgroup_default_nolock     DIRECTIVE  
    managedmaterials             BLOCK      
    materialflarebindings        BLOCK      Default
    meshwheels                   BLOCK      
    meshwheels2                  BLOCK      
    minimass                     BLOCK      Default
    nodecollision                BLOCK      Default
    nodes                        BLOCK      
    nodes2                       BLOCK      
    particles                    BLOCK      Default
    pistonprops                  BLOCK      
    prop_camera_mode             DIRECTIVE  Default
    props                        BLOCK      
    railgroups                   BLOCK      CSV
    rescuer                      DIRECTIVE  
    rigidifiers                  BLOCK      
    rollon                       DIRECTIVE  
    ropables                     BLOCK      
    ropes                        BLOCK      
    rotators                     BLOCK      
    rotators2                    BLOCK      
    screwprops                   BLOCK      
    scripts                      BLOCK      Default
    sectionconfig                MODULE     
    section                      MODULE     
    set_beam_defaults            DIRECTIVE  Default
    set_beam_defaults_scale      DIRECTIVE  Default
    set_collision_range          DIRECTIVE  Default
    set_inertia_defaults         DIRECTIVE  Default
    set_managedmaterials_options DIRECTIVE  Default
    set_node_defaults            DIRECTIVE  Default
    set_shadows                  DIRECTIVE  
    set_skeleton_settings        DIRECTIVE  Default
    shocks                       BLOCK      
    shocks2                      BLOCK      
    shocks3                      BLOCK      
    slidenode_connect_instantly  DIRECTIVE  
    slidenodes                   BLOCK      CommaSpaceSV
    SlopeBrake                   BLOCK      
    soundsources                 BLOCK      
    soundsources2                BLOCK      
    soundsources3                BLOCK      
    speedlimiter                 DIRECTIVE  Default      
    submesh                      BLOCK      
    submesh_groundmodel          DIRECTIVE  Default      
    ties                         BLOCK      
    torquecurve                  BLOCK      CSV
    TractionControl              DIRECTIVE  Keyword-Space-CSV      
    transfercase                 BLOCK      Default      
    triggers                     BLOCK      
    turbojets                    BLOCK      
    turboprops                   BLOCK      
    turboprops2                  BLOCK      
    videocamera                  BLOCK      
    wheels                       BLOCK      
    wheels2                      BLOCK      
    wings                        BLOCK      


SPECIAL SYNTAXES
================

SECTION ANIMATOR
----------------

All whitespace is ignored.
Args 0 - 2 are processed normally.
Arg 3 is split along "|", valid elements are (written as regexes):
    * /(throttle|rpm|aerotorq|aeropit|aerostatus)([12345678])/ ~ For example `throttle2`
    * /shortlimit[:]+([.]*)/ ~ Value should be real number, example: `shortlimit: 0.5`
    * /longlimit[:]+([.]*)/  ~ Value should be real number, example: `longlimit: 0.5`
    * KEYWORD:  vis,inv,airspeed,vvi,
                altimeter100k,altimeter10k,altimeter1k,
                aoa,flap,airbrake,roll,pitch,brakes,accel,clutch,
                speedo,tacho,turbo,parking,shifterman1,shifterman2,
                sequential,shifterlin,torque,difflock,
                rudderboat,throttleboat            

SECTION `FORSET`
----------------

Keyword: `forset`

Parsing as pseudocode:
~~~~~~~~~~~~~~~~~~~~~
    line.trim()
    IF (line == "forset"):  // Keyword "forset" alone
        abort("Invalid line.")
    line = line.substring(6) // Strip "forset"
    FOR EACH tok IN line.split(","): // Including empty strings
        IF tok.contains("-"):
            values = tok.split("-")
            node_a = 0;
            IF (values[0] == ""):
                node_a = values[0].to_uint();
            ADD_RANGE(node_a, values[1].to_uint())
        ELSE:
            ADD_NODE(tok.to_uint())
~~~~~~~~~~~~~~~~~~~~~

All whitespace on the line is ignored. 
Int parsing: `std::strtoul()`, bad trailing chars are silently ignored.
Syntax: 
    1.  "forset" string (6 characters), no space at the end.
    2.  Items delimited by ",". On each side of each "," there is
        max 1 item. Empty/invalid string parses as node num `0`.
        Examples: "forset," -> 2 items `0`, "forset,," -> 3 items '0'
Acceptable item forms:
    *  Single node number (int >= 0): 123
    *  Node range (int >= 0): "123-456"
    *  Single node ID (string) 
Line containing only "forset" is skipped with warning.
When multiple nodes are separated just by space/tab, the 1st node
    is parsed, the others ignored: `forset1 2 3, 4` -> "1, 4"
When a node range has more than 2 nodes, they're ignored.
    `forset1-2-3,4-5-6` -> [1-2],[4-5]

Hyphen rules:
    * Item "---" (at least 1 hyphen) parse as [0]
    * Item "-1" parses as [0-1], "--1" and longer parse as `-1` and return
        value around  4294967286 (unsigned int overflow).
    * Item "1-" (any number of "-") parse as [1-0]
    Valid examples:
        
        `forset1-5, 10 - 20 ,roof123, 1 2,3 4, 1-2-3,4-5-6,-----,`
            -> [1-5],[10-20],[roof123],[1],[3],[1-2],[4-5],[0],[0]
        
        `forset 1-2-3 , 4-5-6,   -7,   8-,  -9-,          --10,  11--, 100`
            -> [1-2]  ,[4-5] ,[0-7],[8-0],[0-9],[0-4294967286],[11-0],[100]

        `forset6--,-66--,--66--,---6--,--7-, ---,7-`
            -> [6-0], [0-66], [0-4294967230],[0-0],[0-4294967289],[0-0],[7-0]
