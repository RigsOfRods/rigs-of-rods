
#pragma once

namespace RoR
{

struct PerVehicleCameraContext
{
    enum CameraBehavior
    {
        CAMERA_BEHAVIOR_EXTERNAL,
        CAMERA_BEHAVIOR_VEHICLE_3rdPERSON,
        CAMERA_BEHAVIOR_VEHICLE_SPLINE,
        CAMERA_BEHAVIOR_VEHICLE_CINECAM
    };

    PerVehicleCameraContext():
        last_cinecam_index(0),
        behavior(CAMERA_BEHAVIOR_EXTERNAL)
    {}

    int                last_cinecam_index;
    CameraBehavior     behavior;
};

} // namespace RoR
