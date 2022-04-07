
#pragma once

namespace RoR
{

/// @addtogroup Gfx
/// @{

/// @addtogroup Camera
/// @{

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
        behavior(CAMCTX_BEHAVIOR_EXTERNAL)
    {}

    CameraCtxBehavior  behavior;
};

/// @} // addtogroup Camera
/// @} // addtogroup Gfx

} // namespace RoR
