// \title Truck physics
// \brief Demo of new physics functions added to BeamClass
// New functions:
// - getDryMass()
// - getInitialDryMass()
// - getLoadedMass()
// - getInitialLoadedMass()
// - setLoadedMass()
// - getNodeMassOptions()
// - setNodeMassOptions()
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

int G_setLoadNodeFrom = 0;
int G_setLoadNodeTo = 0;
bool G_setNodesAsLoaded = true;

int G_selectedNode = 0;
int G_lastNode = 0;
float G_selectedNodeNewMass = 0;
float G_selectedNodeAddedMassAfterRecalc = 0;

string YesOrNo(bool b)
{
    return b ? "Yes" : "No";
}

string Vec3ToString(vector3 vec)
{
    return "X: " + formatFloat(vec.x, '', 0, 2) + ", Y: " + formatFloat(vec.y, '', 0, 2) + ", Z: " + formatFloat(vec.z, '', 0, 2);
}

void frameStep(float dt)
{
    ImGui::SetNextWindowSize(vector2(500, 620));
    if (ImGui::Begin("Truck physics", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        BeamClass@ truck = game.getCurrentTruck();
        if (@truck != null)
        {
            int nodeCount = truck.getNodeCount();
            ImGui::Text("Vehicle: " + truck.getTruckName());

            ImGui::Separator();
            
            ImGui::Text("Truck weights:");
            ImGui::BulletText("Total: " + formatFloat(truck.getTotalMass(true), '', 0, 2) + " kg");
            ImGui::BulletText("Dry: " + formatFloat(truck.getDryMass(), '', 0, 2) + " kg");
            ImGui::BulletText("Load: " + formatFloat(truck.getLoadedMass(), '', 0, 2) + " kg");

            float dryMass = truck.getDryMass();
            if (ImGui::SliderFloat("Dry weight (kg)", dryMass, truck.getInitialDryMass() * 0.1, truck.getInitialDryMass() * 2))
            {
                truck.setMass(dryMass);
                truck.recalculateNodeMasses();
            }

            float initialLoadMass = truck.getInitialLoadedMass();
            // Allow the load mass to be set on vehicles with no load mass.
            if (initialLoadMass == 0)
                initialLoadMass = truck.getDryMass() / 2;

            float loadWeight = truck.getLoadedMass();
            if (ImGui::SliderFloat("Load weight (kg)", loadWeight, 0, initialLoadMass * 2))
            {
                truck.setLoadedMass(loadWeight);
                truck.recalculateNodeMasses();
            }

            ImGui::Separator();

            ImGui::Text("Node information:");
            ImGui::InputInt("Node number (0-" + formatInt(nodeCount - 1) + ")", G_selectedNode);
            if (G_selectedNode >= 0 && G_selectedNode < nodeCount)
            {
                if (G_selectedNode != G_lastNode)
                {
                    G_selectedNodeAddedMassAfterRecalc = 0;
                    G_lastNode = G_selectedNode;
                }

                bool loadedNode = false, weightOverridableNode = false;
                truck.getNodeMassOptions(G_selectedNode, loadedNode, weightOverridableNode);
                float initialMass = truck.getNodeInitialMass(G_selectedNode);
                ImGui::BulletText("Initial mass: " + formatFloat(initialMass, '', 0, 2) + " kg");
                ImGui::BulletText("Current mass: " + formatFloat(truck.getNodeMass(G_selectedNode), '', 0, 2) + " kg");
                ImGui::BulletText("Loaded: " + YesOrNo(loadedNode));
                ImGui::BulletText("Weight-overridable: " + YesOrNo(weightOverridableNode));
                ImGui::BulletText("Velocity vector: " + Vec3ToString(truck.getNodeVelocity(G_selectedNode)));
                ImGui::BulletText("Force vector: " + Vec3ToString(truck.getNodeForces(G_selectedNode)));
                ImGui::BulletText("Added mass after recalculation: " + formatFloat(G_selectedNodeAddedMassAfterRecalc, '', 0, 2) + " kg");

                ImGui::Text("Node mass (before recalculation)");
                ImGui::InputFloat("", G_selectedNodeNewMass);
                if (G_selectedNodeNewMass >= 0)
                {
                    if (ImGui::Button("Change node mass"))
                    {
                        // Allow the node mass to be overriden.
                        truck.setNodeMassOptions(G_selectedNode, true, true);
                        truck.setNodeMass(G_selectedNode, G_selectedNodeNewMass);
                        truck.recalculateNodeMasses();
                        G_selectedNodeAddedMassAfterRecalc = truck.getNodeMass(G_selectedNode) - G_selectedNodeNewMass;
                    }
                }
                else
                {
                    ImGui::Text("Invalid node mass");
                }
            }
            else
            {
                ImGui::Text("Invalid node number");
            }

            ImGui::Separator();

            ImGui::Text("Set nodes as loaded nodes dependent on the globals load mass.");
            ImGui::InputInt("From", G_setLoadNodeFrom);
            ImGui::InputInt("To", G_setLoadNodeTo);
            ImGui::Checkbox("Set nodes as loaded", G_setNodesAsLoaded);
            if (G_setLoadNodeFrom >= 0 && G_setLoadNodeFrom < nodeCount && 
                G_setLoadNodeTo >= 0 && G_setLoadNodeTo < nodeCount &&
                G_setLoadNodeFrom <= G_setLoadNodeTo)
            {
                if (ImGui::Button("Change node options"))
                {
                    for (int i = G_setLoadNodeFrom; i <= G_setLoadNodeTo; i++)
                    {
                        // Allow the node mass to depend on load mass.
                        truck.setNodeMassOptions(i, G_setNodesAsLoaded, false);
                    }
                    truck.recalculateNodeMasses();
                }
            }
            else
            {
                ImGui::Text("Invalid node numbers");
            }
        }
        else
        {
            G_setNodesAsLoaded = true;
            G_setLoadNodeFrom = G_setLoadNodeTo = 0;
            G_selectedNodeAddedMassAfterRecalc = 0;
            ImGui::Text("You are on foot.");
        }

        ImGui::End();
    }
}