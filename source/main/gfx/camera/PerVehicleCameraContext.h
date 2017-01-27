
#pragma once

namespace RoR
{

struct PerVehicleCameraContext
{
    enum CameraCtxBehavior
    {
        CAMCTX_BEHAVIOR_INVALID,
        CAMCTX_BEHAVIOR_EXTERNAL,
        CAMCTX_BEHAVIOR_VEHICLE_3rdPERSON,
        CAMCTX_BEHAVIOR_VEHICLE_SPLINE,
        CAMCTX_BEHAVIOR_VEHICLE_CINECAM
    };

    PerVehicleCameraContext():
        last_cinecam_index(0),
        behavior(CAMCTX_BEHAVIOR_EXTERNAL)
    {}

    int                last_cinecam_index;
    CameraCtxBehavior  behavior;
};

} // namespace RoR
