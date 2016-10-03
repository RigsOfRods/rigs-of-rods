/*
    This source file is part of Rigs of Rods
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

/* maximum limits */
static const int   MAX_TRUCKS                 = 5000;            //!< maximum number of trucks for the engine

static const int   MAX_NODES                  = 1000;            //!< maximum number of nodes per truck
static const int   MAX_BEAMS                  = 5000;            //!< maximum number of beams per truck
static const int   MAX_ROTATORS               = 20;              //!< maximum number of rotators per truck
static const int   MAX_CONTACTERS             = 2000;            //!< maximum number of contacters per truck
static const int   MAX_HYDROS                 = 1000;            //!< maximum number of hydros per truck
static const int   MAX_WHEELS                 = 64;              //!< maximum number of wheels per truck
static const int   MAX_SUBMESHES              = 500;             //!< maximum number of submeshes per truck
static const int   MAX_TEXCOORDS              = 3000;            //!< maximum number of texture coordinates per truck
static const int   MAX_CABS                   = 3000;            //!< maximum number of cabs per truck
static const int   MAX_SHOCKS                 = MAX_BEAMS;       //!< maximum number of shocks per truck
static const int   MAX_ROPES                  = 64;              //!< maximum number of ropes per truck
static const int   MAX_ROPABLES               = 64;              //!< maximum number of ropables per truck
static const int   MAX_TIES                   = 64;              //!< maximum number of ties per truck
static const int   MAX_PROPS                  = 200;             //!< maximum number of props per truck
static const int   MAX_COMMANDS               = 84;              //!< maximum number of commands per truck
static const int   MAX_CAMERAS                = 10;              //!< maximum number of cameras per truck
static const int   MAX_RIGIDIFIERS            = 100;             //!< maximum number of rigifiers per truck
static const int   MAX_FLEXBODIES             = 64;              //!< maximum number of flexbodies per truck
static const int   MAX_AEROENGINES            = 8;               //!< maximum number of aero engines per truck
static const int   MAX_SCREWPROPS             = 8;               //!< maximum number of boat screws per truck
static const int   MAX_AIRBRAKES              = 20;              //!< maximum number of airbrakes per truck
static const int   MAX_SOUNDSCRIPTS_PER_TRUCK = 128;             //!< maximum number of soundsscripts per truck
static const int   MAX_WINGS                  = 40;              //!< maximum number of wings per truck
static const int   MAX_CPARTICLES             = 10;              //!< maximum number of custom particles per truck
static const int   MAX_PRESSURE_BEAMS         = 4000;            //!< maximum number of pressure beams per truck
static const int   MAX_CAMERARAIL             = 50;              //!< maximum number of camera rail points

static const float RAD_PER_SEC_TO_RPM         = 9.5492965855137f; //!< Convert radian/second to RPM (60/2*PI)

/* other global static definitions */
static const int   TRUCKFILEFORMATVERSION     = 3;               //!< truck file format version number

/* physics defaults */
static const float DEFAULT_RIGIDIFIER_SPRING    = 1000000.0f;
static const float DEFAULT_RIGIDIFIER_DAMP      = 50000.0f;
static const float DEFAULT_SPRING               = 9000000.0f;
static const float DEFAULT_DAMP                 = 12000.0f;
static const float DEFAULT_GRAVITY              = -9.8f;         //!< earth gravity
static const float DEFAULT_DRAG                 = 0.05f;
static const float DEFAULT_BEAM_DIAMETER        = 0.05f;         //!< 5 centimeters default beam width
static const float DEFAULT_COLLISION_RANGE      = 0.02f;
static const float MIN_BEAM_LENGTH              = 0.1f;          //!< minimum beam lenght is 10 centimeters
static const float INVERTED_MIN_BEAM_LENGTH     = 1.0f / MIN_BEAM_LENGTH;
static const float BEAM_SKELETON_DIAMETER       = 0.01f;
static const float DEFAULT_WATERDRAG            = 10.0f;
static const float IRON_DENSITY                 = 7874.0f;
static const float BEAM_BREAK                   = 1000000.0f;
static const float BEAM_DEFORM                  = 400000.0f;
static const float BEAM_CREAK_DEFAULT           = 100000.0f;
static const float WHEEL_FRICTION_COEF          = 2.0f;
static const float CHASSIS_FRICTION_COEF        = 0.5f; //!< Chassis has 1/4 the friction of wheels.
static const float SPEED_STOP                   = 0.2f;
static const float STAB_RATE                    = 0.025f;
static const float NODE_FRICTION_COEF_DEFAULT   = 1.0f;
static const float NODE_VOLUME_COEF_DEFAULT     = 1.0f;
static const float NODE_SURFACE_COEF_DEFAULT    = 1.0f;
static const float NODE_LOADWEIGHT_DEFAULT      = -1.0f;
static const float SUPPORT_BEAM_LIMIT_DEFAULT   = 4.0f;
static const float ROTATOR_FORCE_DEFAULT        = 10000000.0f;
static const float ROTATOR_TOLERANCE_DEFAULT    = 0.0f;
static const float HOOK_FORCE_DEFAULT		    = 10000000.0f;
static const float HOOK_RANGE_DEFAULT           = 0.4f;
static const float HOOK_SPEED_DEFAULT           = 0.00025f;
static const float HOOK_LOCK_TIMER_DEFAULT      = 5.0;
static const int   NODE_LOCKGROUP_DEFAULT		= -1; // all hooks scan all nodes
