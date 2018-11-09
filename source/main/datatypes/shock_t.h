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

/**
* SIM-CORE; Shock.
*/
struct shock_t
{
    shock_t() { memset(this, 0, sizeof(shock_t)); }

    int beamid;
    int flags;

    bool trigger_enabled;       //!< general trigger,switch and blocker state
    float trigger_switch_state; //!< needed to avoid doubleswitch, bool and timer in one
    float trigger_boundary_t;   //!< optional value to tune trigger_switch_state autorelease
    int trigger_cmdlong;        //!< F-key for trigger injection longbound-check
    int trigger_cmdshort;       //!< F-key for trigger injection shortbound-check
    int last_debug_state;       //!< smart debug output

    float springin;  //!< shocks2 & shocks3
    float dampin;    //!< shocks2 & shocks3
    float springout; //!< shocks2 & shocks3
    float dampout;   //!< shocks2 & shocks3

    float sprogin;   //!< shocks2
    float dprogin;   //!< shocks2
    float sprogout;  //!< shocks2
    float dprogout;  //!< shocks2
    float lastpos;   //!< shocks2

    float splitin;   //!< shocks3
    float dslowin;   //!< shocks3
    float dfastin;   //!< shocks3
    float splitout;  //!< shocks3
    float dslowout;  //!< shocks3
    float dfastout;  //!< shocks3

    float sbd_spring;           //!< set beam default for spring
    float sbd_damp;             //!< set beam default for damping
};
