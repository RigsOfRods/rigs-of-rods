#include "base.as"

array<int> waypoints_speed = game.getWaypointsSpeed();

void main()
{
    int offset = 0;
    float translation_x = 0;
    float translation_z = 0;
    int position_scheme = game.getAIVehiclePositionScheme();

    for (int x = 0; x < game.getAIVehicleCount(); x++)
    {
        string spawn_vehiclename = game.getAIVehicleName(x);
        array<vector3> waypoints = game.getWaypoints(x);
        if (waypoints.length() == 0)
        {
            game.log("Vehicle AI: No waypoints defined for vehicle '"+spawn_vehiclename+"' ("+(x+1)+"/"+game.getAIVehicleCount()+"), skipping it...");
            continue; // Skip this vehicle
        }        

        vector3 spawn_pos = vector3(waypoints[0].x + translation_x, waypoints[0].y, waypoints[0].z + translation_z);
        string spawn_sectionconfig = game.getAIVehicleSectionConfig(x);
        string spawn_skin = game.getAIVehicleSkin(x);
        BeamClass@ CurrentTruck = game.spawnTruckAI(spawn_vehiclename, spawn_pos, spawn_sectionconfig, spawn_skin, x);
        if (@CurrentTruck == null)
        {
            game.log("Vehicle AI: Could not spawn vehicle '"+spawn_vehiclename+"' ("+(x+1)+"/"+game.getAIVehicleCount()+"), skipping it...");
            continue; // Skip this vehicle
        }
        VehicleAIClass @CurrentTruckai = CurrentTruck.getVehicleAI();

        for (int t = 0; t < game.getAIRepeatTimes(); t++)
        {
            for (uint i = 0; i < waypoints.length(); i++)
            {
                if (position_scheme == 0) // Vehicle behind vehicle
                {
                    if (i == 0 || i == waypoints.length() - 1) // First and last waypoint, set offset for multiple vehicles so they don't crash each other trying to reach the same waypoint
                    {
                        CurrentTruckai.addWaypoint("way"+i, vector3(waypoints[i].x + CurrentTruckai.getTranslation(offset, i).x, waypoints[i].y, waypoints[i].z + CurrentTruckai.getTranslation(offset, i).z));
                    }
                    else
                    {
                        CurrentTruckai.addWaypoint("way"+i, vector3(waypoints[i].x, waypoints[i].y, waypoints[i].z));
                    }
                }
                else if (position_scheme == 1) // Vehicle parallel to vehicle, offset all waypoints
                {
                    CurrentTruckai.addWaypoint("way"+i, vector3(waypoints[i].x + CurrentTruckai.getTranslation(offset, i).x, waypoints[i].y, waypoints[i].z + CurrentTruckai.getTranslation(offset, i).z));
                }
                else if (position_scheme == 2) // Vehicle opposite to vehicle, add the reversed waypoints
                {
                    CurrentTruckai.addWaypoint("way"+i, vector3(waypoints[i].x, waypoints[i].y, waypoints[i].z));
                }
            }
        }

        CurrentTruckai.setActive(true);

        for (uint i = 0; i < waypoints_speed.length(); i++)
        {
            if (waypoints_speed[i] >= 5)
            {
                CurrentTruckai.setValueAtWaypoint("way"+i, AI_SPEED, waypoints_speed[i]);
            }
            else
            {
                CurrentTruckai.setValueAtWaypoint("way"+i, AI_SPEED, -1);
            }
        }

        offset -= game.getAIVehicleDistance();
        translation_x = CurrentTruckai.getTranslation(offset, 0).x;
        translation_z = CurrentTruckai.getTranslation(offset, 0).z;
    }
}
