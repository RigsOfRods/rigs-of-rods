/// \title Shader parameters demo
/// \brief Adjust constant shader parameters - DEMO uses sky material.
// ===================================================

Ogre::MaterialPtr sky_material = Ogre::MaterialManager::getSingleton().getByName("tracks/skyboxcol", "General");
Ogre::GpuProgramParametersPtr sky_params = sky_material.getTechniques()[0].getPasses()[0].getFragmentProgramParameters();

// Sky
float sun_size = 1.0;
float cloud_density = 0.0;
float sky_light = 0.7;

void frameStep(float dt)
{
    
    if (ImGui::SliderFloat("Sun size", /*[inout]*/ sun_size, 0.1f, 3.0f))
    {
        
        sky_params.setNamedConstant("sun_size", sun_size);
        
    }
    if (ImGui::SliderFloat("Cloud density", /*[inout]*/ cloud_density, -0.3f, 0.3f))
    {
        
        sky_params.setNamedConstant("cloud_density", cloud_density);
        
    }
    if (ImGui::SliderFloat("Sky lightness", /*[inout]*/ sky_light, 0.0f, 0.9f))
    {
        
        sky_params.setNamedConstant("sky_light", sky_light);
        //TBD  game.getSceneManager().setAmbientLight(Ogre::ColourValue(sky_light, sky_light, sky_light));
        
    }      
    
    
}
