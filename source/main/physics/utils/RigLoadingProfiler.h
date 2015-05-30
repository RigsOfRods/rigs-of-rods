/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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
	@date   5/2015
*/

#pragma once

#include "Timer.h"

#include <cstdio>

namespace RoR
{

class RigLoadingProfiler
{
    public:
    enum EntryId
    {
        ENTRY_BEAMFACTORY_CREATELOCAL_POSTPROCESS,
        ENTRY_BEAM_CTOR_INITTHREADS,
        ENTRY_BEAM_CTOR_PREPARE_LOADTRUCK,
        ENTRY_BEAM_LOADTRUCK_OPENFILE,
        ENTRY_BEAM_LOADTRUCK_PARSER_CREATE,
        ENTRY_BEAM_LOADTRUCK_PARSER_PREPARE,
        ENTRY_BEAM_LOADTRUCK_PARSER_RUN,
        ENTRY_BEAM_LOADTRUCK_PARSER_FINALIZE,
        ENTRY_BEAM_LOADTRUCK_POST_PARSE,
        ENTRY_BEAM_LOADTRUCK_VALIDATOR_INIT,
        ENTRY_BEAM_LOADTRUCK_VALIDATOR_RUN,
        ENTRY_BEAM_LOADTRUCK_POST_VALIDATION,
        ENTRY_BEAM_LOADTRUCK_SPAWNER_SETUP,
        ENTRY_BEAM_LOADTRUCK_SPAWNER_ADDMODULES,
        ENTRY_BEAM_LOADTRUCK_SPAWNER_RUN,
        ENTRY_BEAM_LOADTRUCK_SPAWNER_LOG,
        ENTRY_BEAM_LOADTRUCK_FIXES,
        ENTRY_BEAM_LOADTRUCK_CALC_MASSES,
        ENTRY_BEAM_LOADTRUCK_SET_DEFAULT_SND_SOURCES,
        ENTRY_BEAM_LOADTRUCK_CALC_NODE_CONNECT_GRAPH,
        ENTRY_BEAM_LOADTRUCK_RECALC_BOUNDING_BOXES,
        ENTRY_BEAM_LOADTRUCK_GROUNDMODEL_AND_STATS,
        ENTRY_BEAM_LOADTRUCK_LOAD_DASHBOARDS,

        ENTRY_COUNT // Special -> used as array size const
    };

    RigLoadingProfiler()                   { memset(m_entries, 0, sizeof(double)*ENTRY_COUNT); m_report[0] = '\0'; }
    void   Restart()                       { m_timer.restart(); }
    void   Checkpoint(EntryId entry_id)    { m_entries[entry_id] = m_timer.elapsed(); m_timer.restart(); }
    char*  Report()
    {
        this->Restart();
        char* dst = m_report;
        dst += sprintf(dst, "Rig loading profiler report:");
        
        dst += sprintf(dst, "\n\tBeam::Beam()                 | init threads:    %f sec", m_entries[ENTRY_BEAM_CTOR_INITTHREADS]);
        dst += sprintf(dst, "\n\tBeam::Beam()                 | prepare loading: %f sec", m_entries[ENTRY_BEAM_CTOR_PREPARE_LOADTRUCK]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | open file:       %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_OPENFILE]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | create parser:   %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_PARSER_CREATE]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | prepare parser:  %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_PARSER_PREPARE]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | run parser:      %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_PARSER_RUN]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | finalize parser: %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_PARSER_FINALIZE]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | post-parse:      %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_POST_PARSE]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | setup validator: %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_VALIDATOR_INIT]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | run validator:   %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_VALIDATOR_RUN]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | post-validation: %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_POST_VALIDATION]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | setup spawner:   %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_SPAWNER_SETUP]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | add modules:     %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_SPAWNER_ADDMODULES]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | run spawner:     %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_SPAWNER_RUN]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | log spawner:     %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_SPAWNER_LOG]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | process fixes:   %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_FIXES]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | calc masses:     %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_CALC_MASSES]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | soundsources:    %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_SET_DEFAULT_SND_SOURCES]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | calc node graph: %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_CALC_NODE_CONNECT_GRAPH]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | bounding boxes:  %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_RECALC_BOUNDING_BOXES]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | groundmodel:     %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_GROUNDMODEL_AND_STATS]);
        dst += sprintf(dst, "\n\tBeam::LoadTruck()            | load dashboards: %f sec", m_entries[ENTRY_BEAM_LOADTRUCK_LOAD_DASHBOARDS]);
        dst += sprintf(dst, "\n\tBeamFactory::createLocal()   | post-process:    %f sec", m_entries[ENTRY_BEAMFACTORY_CREATELOCAL_POSTPROCESS]);
        dst += sprintf(dst, "\n\tRigLoadingProfiler::Report() | compose report:  %f sec", m_timer.elapsed());
        return m_report;
    }

    private:
        PrecisionTimer    m_timer;
        double            m_entries[ENTRY_COUNT];
        char              m_report[(ENTRY_COUNT * 100) + 100];
};

} // namespace RoR
