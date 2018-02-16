/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include "Differentials.h"
#include <OgreLogManager.h>
#include "Language.h"

Axle::Axle() :
    ax_wheel_1(-1),
    ax_wheel_2(-1),
    ax_delta_rotation(0.0f),
    m_torsion_rate(1000000.0f),
    m_torsion_damp(m_torsion_rate / 100),
    m_which_diff(-1)
{
}

void Axle::AddDifferentialType(DiffType diff)
{
    m_available_diffs.push_back(diff);
    if (m_which_diff == -1)
    {
        m_which_diff = 0;
    }
}

void Axle::ToggleDifferentialMode()
{
    m_which_diff++;
    m_which_diff %= m_available_diffs.size();
}

void Axle::CalcAxleTorque(DifferentialData& diff_data)
{
    if (m_which_diff == -1)
    {
        return;
    }

    switch (m_available_diffs[m_which_diff])
    {
    case SPLIT_DIFF:   this->CalcSeparateDiff(diff_data);  return;
    case OPEN_DIFF:    this->CalcOpenDiff(diff_data);       return;
    case LOCKED_DIFF:  this->CalcLockedDiff(diff_data);     return;
    }
}

Ogre::UTFString Axle::GetDifferentialTypeName()
{
    if (m_which_diff == -1)
    {
        return _L("invalid");
    }

    switch (m_available_diffs[m_which_diff])
    {
    case SPLIT_DIFF:   return _L("Split");
    case OPEN_DIFF:    return _L("Open");
    case LOCKED_DIFF:  return _L("Locked");
    default:           return _L("invalid");
    }
}

void Axle::CalcSeparateDiff(DifferentialData& diff_data)
{
    diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque;
}

void Axle::CalcOpenDiff(DifferentialData& diff_data)
{
    /* Open differential calculations *************************
     * These calculation are surprisingly tricky
     * the power ratio is based on normalizing the speed of the
     * wheel. Normalizing is when the sum of all values equals
     * one. more detail provided below
     */

    // velocity at which open diff calculations are used 100%
    // this value is twice the speed of the actual threshold
    // the factor of 2 comes from finding the average speed and
    // is done so to remove unessecary calculations
    const Ogre::Real threshold_vel = 10.0f;

    // combined total velocity
    const Ogre::Real sum_of_vel =
        fabs(diff_data.speed[0]) + fabs(diff_data.speed[1]);

    // normalizing the wheel speeds the wheels to sputter at near 0
    // speeds, to over come this we gradually transition from even
    // power distribution to splitting power based on speed. The
    // Transition ratio defines how much of each formula we take from,
    // Until a threshold has been reached
    const Ogre::Real transition_ratio = (sum_of_vel / threshold_vel < 1) ?
                                            sum_of_vel / threshold_vel : 1;

    // normalize the wheel speed, at a speed of 0 power is split evenly
    const Ogre::Real power_ratio = (fabs(sum_of_vel) > 0.0) ?
                                       fabs(diff_data.speed[0]) / sum_of_vel : 0.5;

    diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque;

    // Diff model taken from Torcs, ror needs to model reaction torque for this to work.
    //const Ogre::Real spider_acc = (diff_data.speed[0] - diff_data.speed[1])/diff_data.dt;
    //DrTq0 = DrTq*0.5f + spiderTq;
    //DrTq1 = DrTq*0.5f - spiderTq;

    // get the final ratio based on the speed of the wheels
    diff_data.out_torque[0] *= 2 * (transition_ratio * power_ratio + (1 - transition_ratio) * 0.5f);
    diff_data.out_torque[1] *= 2 * (transition_ratio * (1 - power_ratio) + (1 - transition_ratio) * 0.5f);

    diff_data.delta_rotation = 0.0f;
}

void Axle::CalcLockedDiff(DifferentialData& diff_data)
{
    /* Locked axle calculation ********************************
     * This is straight forward, two wheels are joined together
     * by a torsion spring. the torsion spring keeps the two
     * wheels in the same orientation as when they were first
     * locked.
     */

    // Torsion spring rate that holds axles together when locked
    // keep as variable for now since this value will be user configurable
    const Ogre::Real m_torsion_rate = 1000000.0f;
    const Ogre::Real m_torsion_damp = m_torsion_rate / 100.0f;
    const Ogre::Real delta_speed = diff_data.speed[0] - diff_data.speed[1];

    diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque;

    // derive how far wheels traveled relative to each during the last time step
    diff_data.delta_rotation += (delta_speed) * diff_data.dt;

    // torque cause by axle shafts
    diff_data.out_torque[0] -= diff_data.delta_rotation * m_torsion_rate;
    // damping
    diff_data.out_torque[0] -= (delta_speed) * m_torsion_damp;

    // torque cause by axle shafts
    diff_data.out_torque[1] += diff_data.delta_rotation * m_torsion_rate;
    // damping
    diff_data.out_torque[1] -= -(delta_speed) * m_torsion_damp;
}
