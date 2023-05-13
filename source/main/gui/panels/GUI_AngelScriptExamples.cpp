/*
    This source file is part of Rigs of Rods
    Copyright 2021 tritonas00
    For more information, see http://www.rigsofrods.org/
    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.
    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/


#include "GUI_AngelScriptExamples.h"
#include "ScriptEngine.h"

using namespace RoR;

void AngelScriptExamples::Draw()
{
    this->DrawRowSlider("void scaleTruck()", "game.getCurrentTruck().scaleTruck({})", _LC("AngelScript", "Scales the truck"), 1.f, 1.5f, scale);
    this->DrawRowText("string getTruckName()", "game.log(game.getCurrentTruck().getTruckName())", _LC("AngelScript", "Gets the name of the truck"));
    this->DrawRowText("string getTruckFileName()", "game.log(game.getCurrentTruck().getTruckFileName())", _LC("AngelScript", "Gets the name of the truck file"));
    this->DrawRowText("string getSectionConfig()", "game.log(game.getCurrentTruck().getSectionConfig())", _LC("AngelScript", "Gets the name of the loaded section for a truck"));
    this->DrawRowText("int getTruckType()", "game.log(' ' + game.getCurrentTruck().getTruckType())", _LC("AngelScript", "Gets the type of the truck"));
    this->DrawRowCheckbox("void reset()", "game.getCurrentTruck().reset({})", _LC("AngelScript", "Resets the truck"), reset, "Keep position");
    this->DrawRowText("void parkingbrakeToggle()", "game.getCurrentTruck().parkingbrakeToggle()", _LC("AngelScript", "Toggles the parking brake"));
    this->DrawRowText("void tractioncontrolToggle()", "game.getCurrentTruck().tractioncontrolToggle()", _LC("AngelScript", "Toggles the tracktion control"));
    this->DrawRowText("void antilockbrakeToggle()", "game.getCurrentTruck().antilockbrakeToggle()", _LC("AngelScript", "Toggles the anti-lock brakes"));
    this->DrawRowText("void beaconsToggle()", "game.getCurrentTruck().beaconsToggle()", _LC("AngelScript", "Toggles the beacons"));
    this->DrawRowText("void toggleCustomParticles()", "game.getCurrentTruck().toggleCustomParticles()", _LC("AngelScript", "Toggles the custom particles"));
    this->DrawRowText("int getNodeCount()", "game.log(' ' + game.getCurrentTruck().getNodeCount())", _LC("AngelScript", "Gets the total amount of nodes of the truck"));
    this->DrawRowCheckbox("float getTotalMass()", "game.log(' ' + game.getCurrentTruck().getTotalMass({}))", _LC("AngelScript", "Gets the total mass of the truck"), locked, "With locked");
    this->DrawRowText("int getWheelNodeCount()", "game.log(' ' + game.getCurrentTruck().getWheelNodeCount())", _LC("AngelScript", "Gets the total amount of nodes of the wheels of the truck"));
    this->DrawRowSlider("void setMass()", "game.getCurrentTruck().setMass({})", _LC("AngelScript", "Sets the mass of the truck"), 1000.f, 10000.f, mass);
    this->DrawRowText("bool getBrakeLightVisible()", "game.log(' ' + game.getCurrentTruck().getBrakeLightVisible())", _LC("AngelScript", "Returns true if the brake light is enabled"));
    this->DrawRowInt("bool getCustomLightVisible()", "game.log(' ' + game.getCurrentTruck().getCustomLightVisible({}))", _LC("AngelScript", "Returns true if the custom light with the number is enabled"), light);
    this->DrawRowIntCheckbox("void setCustomLightVisible()", "game.getCurrentTruck().setCustomLightVisible({}, {})", _LC("AngelScript", "Enables or disables the custom light"), custom_light, visible, "On");
    this->DrawRowText("bool getBeaconMode()", "game.log(' ' + game.getCurrentTruck().getBeaconMode())", _LC("AngelScript", "Gets the mode of the beacon"));
    this->DrawRowInt("void setBlinkType()", "game.getCurrentTruck().setBlinkType({})", _LC("AngelScript", "Sets the blinking type"), blink);
    this->DrawRowText("int getBlinkType()", "game.log(' ' + game.getCurrentTruck().getBlinkType())", _LC("AngelScript", "Gets the blinking type"));
    this->DrawRowText("bool getCustomParticleMode()", "game.log(' ' + game.getCurrentTruck().getCustomParticleMode())", _LC("AngelScript", "Gets the custom particles mode"));
    this->DrawRowText("bool getReverseLightVisible()", "game.log(' ' + game.getCurrentTruck().getReverseLightVisible())", _LC("AngelScript", "Returns true if the reverse lights are enabled"));
    this->DrawRowText("float getHeadingDirectionAngle()", "game.log(' ' + game.getCurrentTruck().getHeadingDirectionAngle())", _LC("AngelScript", "Returns the angle in which the truck is heading"));
    this->DrawRowText("bool isLocked()", "game.log(' ' + game.getCurrentTruck().isLocked())", _LC("AngelScript", "Returns true if a hook of this truck is locked"));
    this->DrawRowText("float getWheelSpeed()", "game.log(' ' + game.getCurrentTruck().getWheelSpeed())", _LC("AngelScript", "Gets the current wheel speed of the vehicle"));
    this->DrawRowText("float getSpeed()", "game.log(' ' + game.getCurrentTruck().getSpeed())", _LC("AngelScript", "Gets the current speed of the vehicle"));
    this->DrawRowText("vector3 getGForces()", "game.log(' ' + game.getCurrentTruck().getGForces().x + ' ' + game.getCurrentTruck().getGForces().y + ' ' + game.getCurrentTruck().getGForces().z )", _LC("AngelScript", "Gets the G-forces that this truck is currently experiencing"));
    this->DrawRowText("float getRotation()", "game.log(' ' + game.getCurrentTruck().getRotation())", _LC("AngelScript", "Gets the current rotation of the vehicle"));
    this->DrawRowText("vector3 getVehiclePosition()", "game.log(' ' + game.getCurrentTruck().getVehiclePosition().x + ' ' + game.getCurrentTruck().getVehiclePosition().y + ' ' + game.getCurrentTruck().getVehiclePosition().z )", _LC("AngelScript", "Gets the current position of the vehicle"));
    this->DrawRowIntNode("vector3 getNodePosition()", "game.log(' ' + game.getCurrentTruck().getNodePosition({}).x + ' ' + game.getCurrentTruck().getNodePosition({}).y + ' ' + game.getCurrentTruck().getNodePosition({}).z )", _LC("AngelScript", "Gets the node position"), node, node, node, node);
}

void AngelScriptExamples::DrawRowSlider(const char* nameStr, std::string codeStr, const char* descStr, float min, float max, float &var_ref)
{
    ImGui::PushID(nameStr);
    ImGui::AlignFirstTextHeightToWidgets();
    if (ImGui::Selectable(nameStr)) { this->ExecuteString(fmt::format(codeStr, var_ref)); }
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::PushItemWidth(-1);
    ImGui::SliderFloat(_LC("Console", ""), &var_ref, min, max);
    ImGui::PopItemWidth();
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::Text("%s", descStr);
    ImGui::PopID();
    ImGui::NextColumn();
}

void AngelScriptExamples::DrawRowText(const char* nameStr, std::string codeStr, const char* descStr)
{
    ImGui::AlignFirstTextHeightToWidgets();
    if (ImGui::Selectable(nameStr)) { this->ExecuteString(codeStr); }
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::Text("%s", "");
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::Text("%s", descStr);
    ImGui::NextColumn();
}

void AngelScriptExamples::DrawRowCheckbox(const char* nameStr, std::string codeStr, const char* descStr, bool &var_ref, const char* label)
{
    ImGui::AlignFirstTextHeightToWidgets();
    if (ImGui::Selectable(nameStr)) { this->ExecuteString(fmt::format(codeStr, var_ref)); }
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::Checkbox(label, &var_ref);
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::Text("%s", descStr);
    ImGui::NextColumn();
}

void AngelScriptExamples::DrawRowInt(const char* nameStr, std::string codeStr, const char* descStr, int &var_ref)
{
    ImGui::PushID(nameStr);
    ImGui::AlignFirstTextHeightToWidgets();
    if (ImGui::Selectable(nameStr)) { this->ExecuteString(fmt::format(codeStr, var_ref)); }
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::PushItemWidth(-1);
    ImGui::InputInt("", &var_ref, 1, 1);
    ImGui::PopItemWidth();
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::Text("%s", descStr);
    ImGui::PopID();
    ImGui::NextColumn();
}

void AngelScriptExamples::DrawRowIntNode(const char* nameStr, std::string codeStr, const char* descStr, int &var_ref, int &node_x, int &node_y, int &node_z)
{
    ImGui::PushID(nameStr);
    ImGui::AlignFirstTextHeightToWidgets();
    if (ImGui::Selectable(nameStr)) { this->ExecuteString(fmt::format(codeStr, node_x, node_y, node_z)); }
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::PushItemWidth(-1);
    ImGui::InputInt("", &var_ref, 1, 1);
    ImGui::PopItemWidth();
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::Text("%s", descStr);
    ImGui::PopID();
    ImGui::NextColumn();
}

void AngelScriptExamples::DrawRowIntCheckbox(const char* nameStr, std::string codeStr, const char* descStr, int &var_ref, bool &on, const char* label)
{
    ImGui::PushID(nameStr);
    ImGui::AlignFirstTextHeightToWidgets();
    if (ImGui::Selectable(nameStr)) { this->ExecuteString(fmt::format(codeStr, var_ref, on)); }
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::PushItemWidth(96);
    ImGui::InputInt("", &var_ref, 1, 1);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Checkbox(label, &on);
    ImGui::NextColumn();
    ImGui::AlignFirstTextHeightToWidgets();
    ImGui::Text("%s", descStr);
    ImGui::PopID();
    ImGui::NextColumn();
}

void AngelScriptExamples::ExecuteString(std::string const& code)
{
    // Use console command 'as' because it echoes the entered code,
    // which allows the user to see it and learn.
    // Also reports error when not usable, i.e. in main menu.
    App::GetConsole()->doCommand(fmt::format("as {}", code));
}
