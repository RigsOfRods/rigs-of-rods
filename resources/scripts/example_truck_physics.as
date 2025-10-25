// \title Truck physics
// \brief Demo of new physics functions added to BeamClass
// New functions:
// - getLoadedMass()
// - setLoadedMass()
// - getNodeInitialMass()
// - getNodeMass()
// - setNodeMass()
// - getNodeVelocity()
// - getNodeForces()
// - recalculateNodeMasses()
// ===================================================

#include "imgui_utils.as"

// Window [X] button handler
imgui_utils::CloseWindowPrompt closeBtnHandler;

int G_selectedNode = 0;
float G_initialLoadMass = -1;

string Vec3ToString(vector3 vec)
{
    return "X: " + formatFloat(vec.x, '', 0, 2) + ", Y: " + formatFloat(vec.y, '', 0, 2) + ", Z: " + formatFloat(vec.z, '', 0, 2);
}

void frameStep(float dt)
{
    ImGui::SetNextWindowSize(vector2(500, 400));
    bool windowDummy = true;
    if (ImGui::Begin("Truck physics", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        BeamClass@ truck = game.getCurrentTruck();
        if (@truck != null)
        {
            if (G_initialLoadMass == -1)
                G_initialLoadMass = truck.getLoadedMass();

            int nodeCount = truck.getNodeCount();
            ImGui::Text("Vehicle: " + truck.getTruckName());

            ImGui::Separator();
            
            ImGui::Text("Truck weights:");
            ImGui::BulletText("Total: " + formatFloat(truck.getTotalMass(true), '', 0, 2) + " kg");
            float loadWeight = truck.getLoadedMass();
            if (ImGui::SliderFloat("Load weight (kg)", loadWeight, G_initialLoadMass * 0.1, G_initialLoadMass * 2))
            {
                truck.setLoadedMass(loadWeight);
                truck.recalculateNodeMasses();
            }

            ImGui::Separator();

            ImGui::Text("Node information:");
            ImGui::InputInt("Node number (0-" + formatInt(nodeCount - 1) + ")", G_selectedNode);
            if (G_selectedNode >= 0 && G_selectedNode < nodeCount)
            {
                float initialMass = truck.getNodeInitialMass(G_selectedNode);
                ImGui::BulletText("Initial mass: " + formatFloat(initialMass, '', 0, 2) + " kg");
                ImGui::BulletText("Current mass: " + formatFloat(truck.getNodeMass(G_selectedNode), '', 0, 2) + " kg");
                ImGui::BulletText("Velocity vector: " + Vec3ToString(truck.getNodeVelocity(G_selectedNode)));
                ImGui::BulletText("Force vector: " + Vec3ToString(truck.getNodeForces(G_selectedNode)));
                float mass = truck.getNodeMass(G_selectedNode);
                if (ImGui::SliderFloat("Node mass", mass, initialMass * 0.1, initialMass * 2))
                {
                    truck.setNodeMass(G_selectedNode, mass);
                    truck.recalculateNodeMasses();
                }
            }
            else
            {
                ImGui::BulletText("Invalid node number");
            }
        }
        else
        {
            G_initialLoadMass = -1;
            ImGui::Text("You are on foot.");
        }

        ImGui::End();
    }
}