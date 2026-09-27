/// \title Audio test script
/// \brief Inspect loaded sounds and soundscript templates, and of course play sounds!
// ---------------------------------------------------------------------------

#include "imgui_utils.as"

CVarClass@  g_app_state = console.cVarFind("app_state"); // 0=bootstrap, 1=main menu, 2=simulation, see AppState in Application.h
SoundScriptInstanceClass@ g_playing_soundscript = null;
SoundClass@ g_playing_sound = null;
bool g_sound_follows_player = true;


// Main window state
imgui_utils::CloseWindowPrompt closeBtnHandler;

void frameStep(float dt)
{
    // Open demo window
    if (ImGui::Begin("Audio API test", closeBtnHandler.windowOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        closeBtnHandler.draw();
        
        drawAudioButtons();
        
        // End window
        ImGui::End();
    }
    
}

vector3 detectPlayerPosition()
{
    if (g_app_state.getInt() == 2) // simulation
    {
        
        // get current pos
        vector3 pos;
        BeamClass@ actor = game.getCurrentTruck();
        if (@actor != null)
        pos = actor.getVehiclePosition();
        else
        pos = game.getPersonPosition();
        
        return pos;
        
    }
    else // main menu
    {
        return vector3(0,0,0);
    }
}

void drawAudioButtons()
{
    ImGui::PushID("AudioTest");
    
    if (g_app_state.getInt() == 1) // main menu
    {
        ImGui::TextDisabled("You are in main menu - spatial (3D) audio is off");
    }
    else if (g_app_state.getInt() == 2) // simulation
    {
        ImGui::TextDisabled("You are in simulation - spatial (3D) audio is on");
        ImGui::Checkbox("Sound follows player", g_sound_follows_player);
        
        // Update sound positions
        if (g_sound_follows_player)
        {
            vector3 pos = detectPlayerPosition();
            if (@g_playing_sound != null)
            g_playing_sound.setPosition(pos);
            if (@g_playing_soundscript != null)
            g_playing_soundscript.setPosition(pos);
        }
    }
    
    array<SoundScriptTemplateClass@>@ templates = game.getAllSoundScriptTemplates();
    string templates_title = "Sound script templates (" + templates.length() + ")";
    if (ImGui::CollapsingHeader(templates_title))
    {
        ImGui::PushID("templates");
        
        for (uint i = 0; i < templates.length(); i++)
        {
            ImGui::PushID(i);
            
            SoundScriptTemplateClass@ template = game.getSoundScriptTemplate(templates[i].getName()); // Look up again by name, just to test the API
            
            if (@template != null)
            {
                ImGui::Text(template.getName());
                if (template.isBaseTemplate())
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled(" [base]");
                }
                ImGui::SameLine();
                
                if (@g_playing_soundscript == null)
                {
                    if (ImGui::Button("Play"))
                    {
                        @g_playing_soundscript = game.createSoundScriptInstance(template.getName());
                        if (@g_playing_soundscript != null)
                        {
                            g_playing_soundscript.setPosition(detectPlayerPosition());
                            g_playing_soundscript.start();
                        }
                        else
                        {
                            game.log("Demo script: could not create sound script instance from template '" + template.getName() + "'");
                        }
                    }
                }
                else
                {
                    if (ImGui::Button("Stop"))
                    {
                        g_playing_soundscript.kill();
                        @g_playing_soundscript = null;
                    }
                }
            }
            else
            {
                ImGui::Text("(Lookup failed for template "+i+"/"+templates.length()+")");
            }
            
            ImGui::PopID(); // i
        }
        
        ImGui::PopID(); // "templates"
    }
    
    array<SoundScriptInstanceClass@>@ instances = game.getAllSoundScriptInstances();
    string instances_title = "Sound script instances (" + instances.length() + ")";
    if (ImGui::CollapsingHeader(instances_title))
    {
        ImGui::PushID("instances");
        
        for (uint i = 0; i < instances.length(); i++)
        {
            ImGui::PushID(i);
            
            SoundScriptInstanceClass@ instance = instances[i];
            
            ImGui::Text(instance.getInstanceName());
            ImGui::SameLine();
            ImGui::TextDisabled("(show tooltip with details)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                drawSoundScriptInstanceDiagPanel(instance);
                ImGui::EndTooltip();
            }
            
            ImGui::PopID(); // i
        }
        
        ImGui::PopID(); // "instances"
    }
    
    ImGui::TextDisabled("Some builtin sounds");
    
    drawWavPreviewBulletButton("default_horn.wav");
    drawWavPreviewBulletButton("default_police.wav");
    drawWavPreviewBulletButton("default_pump.wav");
    drawWavPreviewBulletButton("default_shift.wav");
    drawWavPreviewBulletButton("default_starter.wav");
    
    ImGui::PopID(); // "AudioTest"
}

void drawWavPreviewBulletButton(string wav_file)
{
    ImGui::PushID(wav_file);
    
    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::Text(wav_file);
    ImGui::SameLine();
    if (@g_playing_sound == null)
    {
        if (ImGui::Button("Play loop"))
        {        
            @g_playing_sound = game.createSoundFromResource(wav_file);
            g_playing_sound.setEnabled(true);
            g_playing_sound.setGain(1.f);
            g_playing_sound.setLoop(true);
            g_playing_sound.setPosition(detectPlayerPosition());
            g_playing_sound.play();
            game.log("Demo script: playing file " + wav_file);
        }
    }
    else
    {
        if (ImGui::Button("Stop"))
        {
            g_playing_sound.stop();
            @g_playing_sound = null;
            game.log("Demo script: stopping file " + wav_file);
        }
    }
    
    ImGui::PopID(); // wav_file
}

void drawSoundObjectDiag(SoundClass@ snd)
{
    string txt 
    = "\t enabled:"+snd.getEnabled()+", playing:"+snd.isPlaying()
    +"\n\t audibility:"+snd.getAudibility()
    +"\n\t gain:"+snd.getGain() +", pitch:"+snd.getPitch()
    +"\n\t loop:"+snd.getLoop()
    +"\n\t currentHardwareIndex:"+snd.getCurrentHardwareIndex()
    +"\n\t OpenAL buffer ID:"+snd.getBuffer()
    +"\n\t position: X="+snd.getPosition().x+" Y="+snd.getPosition().y+" Z="+snd.getPosition().z
    +"\n\t velocity: X="+snd.getVelocity().x+" Y="+snd.getVelocity().y+" Z="+snd.getVelocity().z;
    ImGui::Text(txt);
}

void drawSoundScriptInstanceDiagPanel(SoundScriptInstanceClass@ instance)
{
    SoundScriptTemplateClass@ template = instance.getTemplate();
    
    // START sound
    SoundClass@ startSnd = instance.getStartSound();
    if (@startSnd != null)
    {
        ImGui::Text("START sound: '" + template.getStartSoundName() + "' (pitchgain: "+instance.getStartSoundPitchgain()+")");
        drawSoundObjectDiag(startSnd);
    }
    else
    {
        ImGui::TextDisabled("[no START sound]");
    }
    
    // SOUNDS (running)
    int numSounds = template.getNumSounds();
    ImGui::Text("SOUNDS (count: " + numSounds + ")");
    for (int i = 0; i < numSounds; i++)
    {
        SoundClass@ snd = instance.getSound(i);
        ImGui::Text("SOUND: '" + template.getSoundName(i) + "' (pitchgain: "+instance.getSoundPitchgain(i)+")");
        drawSoundObjectDiag(snd);
    }
    
    // STOP sound
    SoundClass@ stopSnd = instance.getStopSound();
    if (@stopSnd != null)
    {
        ImGui::Text("STOP sound: '" + template.getStopSoundName() + "' (pitchgain: "+instance.getStopSoundPitchgain()+")");
        drawSoundObjectDiag(stopSnd);
    }
    else
    {
        ImGui::TextDisabled("[no STOP sound]");
    }    
}


string formatVector3(vector3 val, int total, int frac)
{
    return "X:" + formatFloat(val.x, "", total, frac)
    + " Y:" + formatFloat(val.y, "", total, frac)
    + " Z:" + formatFloat(val.z, "", total, frac);
}



