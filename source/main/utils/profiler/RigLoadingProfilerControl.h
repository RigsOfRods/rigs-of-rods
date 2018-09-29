/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2015 Petr Ohlidal

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

/**
    @file   
    @author Petr Ohlidal
    @date   05/2015
    @brief  Allows configuration of various profiling mechanisms.

    Primary profiler creates output file "profiler/rig_loading.html"
    The output HTML file may contain information from various sources.
    Any number of sources may be enabled, but the mixed results may be hard to read.
    Further, some sources may be incompatible with each other.
    If no source is activated, the HTML report is not generated at all.

    There are some simple additional profilers, also controlled here.
*/

#pragma once

// ============================================================================
// SETUP
// ============================================================================

#define ROR_PROFILE_RIG_LOADING_OUTFILE "rig_loading.html"

// HTML profiler
// Profile creating flexbody, fine-grained
//#define FLEXBODY_USE_PROFILER

// RoR.log only
// Prints simple, per-flexbody stats to RoR.log
//#define FLEXBODY_LOG_LOADING_TIMES

// ============================================================================
// END SETUP
// ============================================================================

// Removed
#define SPAWNER_PROFILE_SCOPED()

#ifdef FLEXBODY_USE_PROFILER
#   define ROR_PROFILE_RIG_LOADING
#   include "Profiler.h"
// Use root namespace ::
#   define FLEXBODY_PROFILER_LABEL(NAME)          ("FlexBody | " NAME)
#   define FLEXBODY_PROFILER_START(NAME)          ::PROFILE_START_RAW(FLEXBODY_PROFILER_LABEL(NAME))
#   define FLEXBODY_PROFILER_ENTER(NAME)          ::PROFILE_STOP(); FLEXBODY_PROFILER_START(NAME)
#   define FLEXBODY_PROFILER_EXIT()               ::PROFILE_STOP()
#   define FLEXBODY_PROFILER_SCOPED(NAME)         ::PROFILE_SCOPED_RAW(FLEXBODY_PROFILER_LABEL(NAME))
#else
#   define FLEXBODY_PROFILER_START(NAME) 
#   define FLEXBODY_PROFILER_LABEL(NAME) 
#   define FLEXBODY_PROFILER_EXIT()
#   define FLEXBODY_PROFILER_SCOPED(NAME)
#   define FLEXBODY_PROFILER_ENTER(NAME)
#endif

