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
    float lastpos;
    float springin;
    float dampin;
    float sprogin;
    float dprogin;
    float springout;
    float dampout;
    float sprogout;
    float dprogout;
    float sbd_spring;               //!< set beam default for spring
    float sbd_damp;                 //!< set beam default for damping
    int trigger_cmdlong;            //!< F-key for trigger injection longbound-check
    int trigger_cmdshort;           //!< F-key for trigger injection shortbound-check
    bool trigger_enabled;           //!< general trigger,switch and blocker state
    float trigger_switch_state;     //!< needed to avoid doubleswitch, bool and timer in one
    float trigger_boundary_t;       //!< optional value to tune trigger_switch_state autorelease
    int last_debug_state;           //!< smart debug output
};
