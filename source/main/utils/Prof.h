#pragma once

/// @file
#include "Application.h"
#include "OgreImGui.h"
#include "ForwardDeclarations.h"
#include <OgreTimer.h>
#include <array>
#include <chrono>
#include <fmt/format.h>

namespace RoR
{
    enum ProfStat
    {
        PROF_CALCBEAMS_TOTAL,
        PROF_CALCNODES_TOTAL,
        PROF_INTRACOL_UPDATE,
        PROF_INTRACOL_RESOLVE,
        PROF_INTERCOL_UPDATE,
        PROF_INTERCOL_RESOLVE,

        // Flexbodies (per frame, not per physics tick); summed over all vertex-range tasks of the actor.
        PROF_FLEXBODY_BASES,   //!< Main thread: `FlexBody::calcNodeBases()` (shared REF/X/Y bases, before tasks start)
        PROF_FLEXBODY_QUEUED,  //!< Threadpool: time from task submit to task start (contention indicator)
        PROF_FLEXBODY_BLEND,   //!< Threadpool: `FlexBody::updateBlend()` (contact/wet vertex colors)
        PROF_FLEXBODY_VERTS,   //!< Threadpool: `FlexBody::computeFlexbodyVertices()`
        PROF_FLEXBODY_TASK_MAX,//!< Threadpool: longest task (blend+verts) - the best case for JOIN; compare with VERTS/num_tasks for balance
        PROF_FLEXBODY_JOIN,    //!< Main thread: blocked waiting for flexbody tasks (>0 means flexbodies are on the critical path)
        PROF_FLEXBODY_LOCK,    //!< Main thread: `FlexBody::lockVertexBuffers()` (mapping back buffers for direct write by tasks)
        PROF_FLEXBODY_UPLOAD,  //!< Main thread: `FlexBody::updateFlexbodyVertexBuffers()` (unlock + swap back buffers, or CPU copy upload)

        PROF_Count_
    };

    struct Prof // mini profiler - data from one actor in one `ActorManager::UpdatePhysicsSimulation()` run.
    {
        typedef unsigned long long Timeval_t;
        const Timeval_t PROF_NOBEGIN = std::numeric_limits<Timeval_t>::max();

        static bool prof_globaltimer_initialized;
        static void ProfGlobalTimerInit();
        ActorInstanceID_t prof_actor = ACTORINSTANCEID_INVALID;

        // Num physics ticks
        int prof_ticks = 0;
        // microseconds
        std::array<Timeval_t, PROF_Count_> prof_begin_us;
        std::array<Timeval_t, PROF_Count_> prof_totals_us;
        std::array<double, PROF_Count_> prof_smoothtot_us;

        Prof(ActorInstanceID_t actor)
        {
            if (!Prof::prof_globaltimer_initialized)
            {
                // Be dramatic!
                throw std::runtime_error("ProfGlobalTimerInit() must be called before creating any Prof instances");
            }
            prof_begin_us.fill(PROF_NOBEGIN);
            prof_totals_us.fill(0);
            prof_smoothtot_us.fill(1000.0);
            prof_actor = actor;
        }

        static Timeval_t ProfNow();

        /// For measurements done elsewhere, i.e. on threadpool where `ProfBegin()/ProfEnd()` would race.
        void ProfAdd(ProfStat stat, Timeval_t duration_us)
        {
            prof_totals_us[stat] += duration_us;
        }

        void ProfBegin(ProfStat stat)
        {
            prof_begin_us[stat] = this->ProfNow();
        }

        void ProfEnd(ProfStat stat)
        {
            if (prof_begin_us[stat] == PROF_NOBEGIN)
            {
                return;
            }
            prof_totals_us[stat] += this->ProfNow() - prof_begin_us[stat];
            prof_begin_us[stat] = PROF_NOBEGIN;
        }

        static const char* ProfStatToStr(ProfStat stat)
        {
            switch (stat)
            {
            case PROF_CALCBEAMS_TOTAL: return "PROF_CALCBEAMS_TOTAL";
            case PROF_CALCNODES_TOTAL: return "PROF_CALCNODES_TOTAL";
            case PROF_INTRACOL_UPDATE: return "PROF_INTRACOL_UPDATE";
            case PROF_INTRACOL_RESOLVE: return "PROF_INTRACOL_RESOLVE";
            case PROF_INTERCOL_UPDATE: return "PROF_INTERCOL_UPDATE";
            case PROF_INTERCOL_RESOLVE: return "PROF_INTERCOL_RESOLVE";
            case PROF_FLEXBODY_BASES: return "PROF_FLEXBODY_BASES";
            case PROF_FLEXBODY_QUEUED: return "PROF_FLEXBODY_QUEUED";
            case PROF_FLEXBODY_BLEND: return "PROF_FLEXBODY_BLEND";
            case PROF_FLEXBODY_VERTS: return "PROF_FLEXBODY_VERTS";
            case PROF_FLEXBODY_TASK_MAX: return "PROF_FLEXBODY_TASK_MAX";
            case PROF_FLEXBODY_JOIN: return "PROF_FLEXBODY_JOIN";
            case PROF_FLEXBODY_LOCK: return "PROF_FLEXBODY_LOCK";
            case PROF_FLEXBODY_UPLOAD: return "PROF_FLEXBODY_UPLOAD";
            default: return "Unknown stat";
            }
        }

        void ProfDraw()
        {
            for (int i = 0; i < PROF_Count_; ++i)
            {
                prof_smoothtot_us[i] = 0.9 * prof_smoothtot_us[i] + 0.1 * prof_totals_us[i];
                ImGui::Text("%s: %.2fus", ProfStatToStr(static_cast<ProfStat>(i)), prof_smoothtot_us[i]);
            }
        }

        void ProfLog()
        {
            LOG(fmt::format("Actor {} ({} ticks)", prof_actor, prof_ticks));
            for (int i = 0; i < PROF_Count_; ++i)
            {
                LOG(fmt::format("\t{}: {:.2f}us", ProfStatToStr(static_cast<ProfStat>(i)), prof_smoothtot_us[i]));
            }
        }

        void ProfClear()
        {
            prof_begin_us.fill(PROF_NOBEGIN);
            prof_totals_us.fill(0);
            prof_ticks = 0;
        }
    };

    struct ProfUI
    {
        std::array<double, PROF_Count_> prof_smooth_avgs;
        
        float prof_smooth_ticks = 0.f;
        bool prof_show_details = false;

        ProfUI()
        {
            prof_smooth_avgs.fill(0.0);
        }

        void DrawProfUI();
    };
}

