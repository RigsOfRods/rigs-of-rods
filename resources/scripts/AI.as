#include "base.as"

array<vector3> waypoints = game.getWaypoints();

//Stuff that the scripts uses internally. doesn't need to be changed.
vector3 currentWaypoint(0, 0, 0);
int waypoint = 0;

void main()
{
    int offset = 0;
    float translation_x = 0;
    float translation_z = 0;
    int position_scheme = game.getAIVehiclePositionScheme();

    for (int x = 0; x < game.getAIVehicleCount(); x++)
    {
        VehicleAIClass @CurrentTruckai = game.spawnTruckAI(game.getAIVehicleName(), vector3(waypoints[0].x + translation_x, waypoints[0].y, waypoints[0].z + translation_z), game.getAIVehicleSectionConfig(), game.getAIVehicleSkin()).getVehicleAI();

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
            }
        }

        CurrentTruckai.setActive(true);
        CurrentTruckai.setValueAtWaypoint("way0",AI_SPEED, game.getAIVehicleSpeed());

        offset -= game.getAIVehicleDistance();
        translation_x = CurrentTruckai.getTranslation(offset, 0).x;
        translation_z = CurrentTruckai.getTranslation(offset, 0).z;
    }
}
