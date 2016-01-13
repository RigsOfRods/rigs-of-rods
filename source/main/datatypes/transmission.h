
#pragma once

namespace RoR
{

struct Gearbox
{
    enum shiftmodes {
        SHIFTMODE_AUTOMATIC,    //!< Automatic shift
        SHIFTMODE_SEMIAUTO,     //!< Manual shift - Auto clutch
        SHIFTMODE_MANUAL,       //!< Fully Manual: sequential shift
        SHIFTMODE_MANUAL_STICK, //!< Fully manual: stick shift
        SHIFTMODE_MANUAL_RANGES, //!< Fully Manual: stick shift with ranges

        SHIFTMODE_DEFAULT = 0xFFFFFFFD,
        SHIFTMODE_UNKNOWN = 0xFFFFFFFE,
        SHIFTMODE_INVALID = 0xFFFFFFFF
    };

    enum autoswitch {
        AUTOSWITCH_REAR,
        AUTOSWITCH_NEUTRAL,
        AUTOSWITCH_DRIVE,
        AUTOSWITCH_TWO,
        AUTOSWITCH_ONE,
        AUTOSWITCH_MANUALMODE
    };
};

} // namespace RoR
