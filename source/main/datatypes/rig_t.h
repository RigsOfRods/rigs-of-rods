/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2017 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Application.h"
#include "RoRPrerequisites.h"
#include "BeamData.h"
#include <vector>

#include <OgrePrerequisites.h>

struct rig_t ///< A simulation actor; typically a vehicle
{
    rig_t(): nodes(nullptr), free_node(0), beams(nullptr), free_beam(0),
             shocks(nullptr), free_shock(0), has_active_shocks(false),
             rotators(nullptr), free_rotator(0), wings(nullptr), free_wing(0)
    {}

    ~rig_t()
    {
        delete nodes;
        delete beams;
        delete shocks;
        delete rotators;
        delete wings;
    }

    node_t*                    nodes;
    int                        free_node;            ///< Number of nodes; name is historical (free index in static array)

    beam_t*                    beams;
    int                        free_beam;            ///< Number of beams; name is historical (free index in static array)
    std::vector<beam_t*>       interTruckBeams;

    shock_t*                   shocks;               ///< Shock absorbers
    int                        free_shock;           ///< Number of shock absorbers; name is historical (free index in static array)
    bool                       has_active_shocks;    ///< Are there active stabilizer shocks?

    rotator_t*                 rotators;
    int                        free_rotator;         ///< Number of rotators; name is historical (free index in static array)

    wing_t*                    wings;
    int                        free_wing;            ///< Number of wings; name is historical (free index in static array)

    std::vector<exhaust_t>     exhausts;
    std::vector<rope_t>        ropes;
    std::vector<ropable_t>     ropables;
    std::vector<tie_t>         ties;
    std::vector<hook_t>        hooks;
    std::vector<flare_t>       flares;

    contacter_t contacters[MAX_CONTACTERS];
    int free_contacter;

    wheel_t wheels[MAX_WHEELS];
    vwheel_t vwheels[MAX_WHEELS];
    int free_wheel;

    command_t commandkey[MAX_COMMANDS + 10]; // 0 for safety

    prop_t props[MAX_PROPS];
    prop_t *driverSeat;
    int free_prop;

    cparticle_t cparticles[MAX_CPARTICLES];
    int free_cparticle;

    std::vector<debugtext_t>nodes_debug, beams_debug;

    soundsource_t soundsources[MAX_SOUNDSCRIPTS_PER_TRUCK];
    int free_soundsource;

    int pressure_beams[MAX_PRESSURE_BEAMS];
    int free_pressure_beam;

    AeroEngine *aeroengines[MAX_AEROENGINES];
    int free_aeroengine;

    Screwprop *screwprops[MAX_SCREWPROPS];
    int free_screwprop;

    int cabs[MAX_CABS*3];
    int free_cab;

    int hydro[MAX_HYDROS];
    int free_hydro;

    int collcabs[MAX_CABS];
    collcab_rate_t inter_collcabrate[MAX_CABS];
    collcab_rate_t intra_collcabrate[MAX_CABS];
    int free_collcab;

    int buoycabs[MAX_CABS];
    int buoycabtypes[MAX_CABS];
    int free_buoycab;

    Airbrake *airbrakes[MAX_AIRBRAKES];
    int free_airbrake;

    RoR::Skidmark *skidtrails[MAX_WHEELS*2];
    bool useSkidmarks;

    FlexBody *flexbodies[MAX_FLEXBODIES];
    int free_flexbody;

    std::vector<std::string> description;

    int cameraRail[MAX_CAMERARAIL];
    int free_camerarail;

    bool hideInChooser;

    Ogre::String realtruckname;

    bool forwardcommands;
    bool importcommands;
    bool wheel_contact_requested;
    bool rescuer;
    bool disable_default_sounds;

    // Antilockbrake + Tractioncontrol
    bool has_slope_brake;
    float slopeBrakeFactor;
    float slopeBrakeAttAngle;
    float slopeBrakeRelAngle;
    float previousCrank;
    float alb_ratio;        //!< Anti-lock brake attribute: Regulating force
    float alb_minspeed;     //!< Anti-lock brake attribute;
    int alb_mode;           //!< Anti-lock brake status; Enabled? {1/0}
    float alb_pulse_time;   //!< Anti-lock brake attribute;
    bool alb_pulse_state;   //!< Anti-lock brake status;
    bool alb_present;       //!< Anti-lock brake attribute: Display the dashboard indicator?
    bool alb_notoggle;      //!< Anti-lock brake attribute: Disable in-game toggle?
    float tc_ratio;
    float tc_wheelslip;
    float tc_fade;
    int tc_mode;           //!< Traction control status; Enabled? {1/0}
    float tc_pulse_time;   //!< Traction control attribute;
    bool tc_pulse_state;
    bool tc_present;       //!< Traction control attribute; Display the dashboard indicator?
    bool tc_notoggle;      //!< Traction control attribute; Disable in-game toggle?
    float tc_timer;
    float alb_timer;
    int antilockbrake;
    int tractioncontrol;
    float animTimer;

    // Cruise Control
    bool cc_mode;
    bool cc_can_brake;
    float cc_target_rpm;
    float cc_target_speed;
    float cc_target_speed_lower_limit;
    std::deque<float> cc_accs;

    // Speed Limiter
    bool sl_enabled; //!< Speed limiter;
    float sl_speed_limit; //!< Speed limiter;
    int categoryid;
    int truckversion;
    int externalcameramode, externalcameranode;
    std::vector<authorinfo_t> authors;
    float collrange;
    int masscount; //!< Number of nodes loaded with l option
    bool disable_smoke;
    int smokeId;  //!< Old-format exhaust (one per vehicle) emitter node
    int smokeRef; //!< Old-format exhaust (one per vehicle) backwards direction node
    char truckname[256];
    bool networking;
    bool beambreakdebug, beamdeformdebug, triggerdebug;
    CmdKeyInertia *rotaInertia;
    CmdKeyInertia *hydroInertia;
    CmdKeyInertia *cmdInertia;
    float truckmass;
    float loadmass;
    int trucknum;
    RoR::SkinDef* usedSkin;
    Buoyance *buoyance;

    int driveable;
    BeamEngine *engine;
    int hascommands;
    int hashelp;
    char helpmat[256];
    int cinecameranodepos[MAX_CAMERAS]; //!< Cine-camera node indexes
    int freecinecamera; //!< Number of cine-cameras (lowest free index)
    RoR::GfxFlaresMode m_flares_mode;
    std::vector<Ogre::Entity*> deletion_Entities; //!< For unloading vehicle; filled at spawn.
    std::vector<Ogre::MovableObject *> deletion_Objects; //!< For unloading vehicle; filled at spawn.
    std::vector<Ogre::SceneNode*> deletion_sceneNodes; //!< For unloading vehicle; filled at spawn.
    unsigned int netCustomLightArray[4];
    unsigned char netCustomLightArray_counter;
    bool ispolice;
    int state;
    bool collisionRelevant;
    bool heathaze;
    Autopilot *autopilot;
    HeightFinder *hfinder;
    Airfoil *fuseAirfoil;
    node_t *fuseFront;
    node_t *fuseBack;
    float fuseWidth;
    float brakeforce;
    float hbrakeforce;
    //! Dbg. overlay type { NODES: 1-Numbers, 4-Mass, 5-Locked | BEAMS: 2-Numbers, 6-Compression, 7-Broken, 8-Stress, 9-Strength, 10-Hydros, 11-Commands, OTHER: 3-N&B numbers, 12-14 unknown }
    int debugVisuals;

    float speedoMax;
    bool useMaxRPMforGUI;
    float minimass;
    bool cparticle_enabled;
    bool advanced_drag;
    float advanced_node_drag;
    float advanced_total_drag;

    Axle *axles[MAX_WHEELS/2];
    int free_axle;

    int free_fixes;
    int propwheelcount;
    int free_commands;
    int fileformatversion;

    Ogre::Vector3 origin;
    Ogre::SceneNode *beamsRoot;
    //! Stores all the SlideNodes available on this truck
    std::vector< SlideNode > mSlideNodes;

    int proped_wheels; //!< Number of propelled wheels.
    int braked_wheels; //!< Number of braked wheels.

    int proppairs[MAX_WHEELS]; //!< For inter-differential locking

    //! try to connect slide-nodes directly after spawning
    bool slideNodesConnectInstantly;

    //! Stores all the available RailGroups for this truck
    std::vector< RailGroup* > mRailGroups;

    Ogre::Camera *mCamera;
    int freecamera;
    int cameranodepos[MAX_CAMERAS];
    int cameranodedir[MAX_CAMERAS];
    int cameranoderoll[MAX_CAMERAS];
    bool revroll[MAX_CAMERAS];
    bool shadowOptimizations;
    FlexObj *cabMesh;
    Ogre::SceneNode *cabNode;
    Ogre::Entity *cabEntity;
    Ogre::AxisAlignedBox boundingBox; //!< standard bounding box (surrounds all nodes of a truck)
    Ogre::AxisAlignedBox predictedBoundingBox;
    std::vector<Ogre::AxisAlignedBox> collisionBoundingBoxes; //!< smart bounding boxes, used for determining the state of a truck (every box surrounds only a subset of nodes)
    std::vector<Ogre::AxisAlignedBox> predictedCollisionBoundingBoxes;
    bool freePositioned;
    int lowestnode; //!< never updated after truck init!?!
    int lowestcontactingnode;

    float posnode_spawn_height;

    Ogre::String subMeshGroundModelName;

    float odometerTotal;
    float odometerUser;

    std::vector<std::pair<Ogre::String, bool> > dashBoardLayouts;

    VehicleAI *vehicle_ai;
};
