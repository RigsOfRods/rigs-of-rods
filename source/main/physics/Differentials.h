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

#pragma once

#include <vector>
#include <OgreUTFString.h>

namespace RoR {

struct DifferentialData
{
    float speed[2];
    float delta_rotation; // sign is first relative to the second
    float out_torque[2];
    float in_torque;
    float dt;
};

class TransferCase
{
public:
    TransferCase(int a1, int a2, bool has_2wd, bool has_2wd_lo, std::vector<float> grs):
        tr_ax_1(a1), tr_ax_2(a2), tr_2wd(has_2wd), tr_2wd_lo(has_2wd_lo), tr_4wd_mode(false), tr_gear_ratios(grs) {};

    int   tr_ax_1;                      //!< This axle is always driven
    int   tr_ax_2;                      //!< This axle is only driven in 4WD mode
    bool  tr_2wd;                       //!< Does it support 2WD mode?
    bool  tr_2wd_lo;                    //!< Does it support 2WD Lo mode?
    bool  tr_4wd_mode;                  //!< Enables 4WD mode
    std::vector<float> tr_gear_ratios;  //!< Gear reduction ratios
};

enum DiffType
{
    SPLIT_DIFF = 0,
    OPEN_DIFF,
    VISCOUS_DIFF,
    LOCKED_DIFF,
    INVALID_DIFF
};

class Differential
{
public:
    Differential(): di_idx_1(0), di_idx_2(0), di_delta_rotation(0.0f) {};

    int       di_idx_1;          //!< array location of wheel / axle 1
    int       di_idx_2;          //!< array location of wheel / axle 2
    float     di_delta_rotation; //!< difference of rotational position between two wheels/axles... a kludge at best

    void             AddDifferentialType(DiffType diff) { m_available_diffs.push_back(diff); }
    void             ToggleDifferentialMode();
    void             CalcAxleTorque(DifferentialData& diff_data);
    Ogre::UTFString  GetDifferentialTypeName();
    DiffType         GetActiveDiffType() const { return m_available_diffs[0]; }
    int              GetNumDiffTypes() { return static_cast<int>(m_available_diffs.size()); }
    
    static void      CalcSeparateDiff(DifferentialData& diff_data);  //!< a differential that always splits the torque evenly, this is the original method
    static void      CalcOpenDiff(DifferentialData& diff_data );     //!< more power goes to the faster spining wheel
    static void      CalcViscousDiff(DifferentialData& diff_data );  //!< more power goes to the slower spining wheel
    static void      CalcLockedDiff(DifferentialData& diff_data );   //!< ensures both wheels rotate at the the same speed

private:
    std::vector<DiffType> m_available_diffs;
};

} // namespace RoR

