/// \title Managed material diag demo
/// \brief List actor's managedmats and edit shader params
//
// API ON DISPLAY:
// -- `BeamClass.getManagedMaterialNames()` -> `array<string>@`
// -- `BeamClass.getManagedMaterialInstance()` -> `Ogre::MaterialPtr`
// -- `Ogre::Pass.__getNamedConstants()` -> `array<string>@`
// -- `Ogre::Pass.getFragmentProgramParameters()` -> `Ogre::GpuProgramParametersPtr`
// ===================================================

// We can't reaad from `Ogre::GpuProgramParametersPtr` at the moment :/
dictionary knownConstantValues;

void frameStep(float dt)
{
    BeamClass@ playerActor = game.getCurrentTruck();
    if (@playerActor == null)
    {
        ImGui::Text("you are on foot.");
        ImGui::Text("enter a vehicle to view it's managedmaterials.");
        return;
    }
    
    array<string>@ managedMatNames = playerActor.getManagedMaterialNames();
    ImGui::TextDisabled("you're driving: "); ImGui::SameLine(); ImGui::Text(playerActor.getTruckName());
    ImGui::Separator();
    
    for (uint iMat = 0; iMat < managedMatNames.length(); iMat++)
    {
        ImGui::PushID(iMat);
        Ogre::MaterialPtr mat = playerActor.getManagedMaterialInstance(managedMatNames[iMat]);
        if (ImGui::TreeNode(managedMatNames[iMat]))
        {
            ImGui::SameLine();
            if (mat.isNull()) { ImGui::TextDisabled("null"); continue; }
            if (mat.getTechniques().length() < 1) { ImGui::TextDisabled("no `Techniques` in mat");ImGui::TreePop(); continue; }
            Ogre::Technique@ matTech = mat.getTechniques()[0];
            if (matTech.getPasses().length() < 1) { ImGui::TextDisabled("no `Passes` in mat"); ImGui::TreePop(); continue; }
            Ogre::Pass@ matPass = matTech.getPasses()[0];       
            if (@matPass == null) { ImGui::TextDisabled("null `Pass #0` in mat"); ImGui::TreePop(); continue; }   // paranoia      
            Ogre::GpuProgramParametersPtr fragParams = matPass.getFragmentProgramParameters();
            if (fragParams.isNull()) { { ImGui::TextDisabled("no fragment shader in mat"); ImGui::TreePop(); continue; } }
            array<string>@ namedConstants = fragParams.__getNamedConstants();
            if (@namedConstants == null) { { ImGui::TextDisabled("INTERNAL ERROR - `__getNamedConstants()` returned null"); ImGui::TreePop(); continue; } }
            ImGui::TextDisabled("[" + namedConstants.length() + " named constants]");
            
            for (uint iCon = 0; iCon < namedConstants.length(); iCon++)
            {
                ImGui::PushID(iCon);
                ImGui::BulletText("[" + (iCon+1) + "/" + namedConstants.length() + "]"); ImGui::SameLine();
                float valMin = -1.f;
                float valMax = 1.f;
                float tmpVal = float(knownConstantValues[namedConstants[iCon]]);
                if (ImGui::SliderFloat(namedConstants[iCon], /*[inout]*/tmpVal, valMin, valMax))
                {
                    knownConstantValues[namedConstants[iCon]] = tmpVal;
                    fragParams.setNamedConstant(namedConstants[iCon], tmpVal);
                }
                ImGui::PopID(); // iCon
                
            }
            
            ImGui::TreePop();
        } // TreeNode())
        
        ImGui::PopID(); // iMat
    }
}
